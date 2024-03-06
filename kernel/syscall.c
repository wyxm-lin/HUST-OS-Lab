/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "proc_file.h"

#include "spike_interface/spike_utils.h"
#include "vfs.h"
#include "core.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char *buf, size_t n)
{
	// buf is now an address in user space of the given app's user stack,
	// so we have to transfer it into phisical address (kernel is running in direct mapping).
	uint64 hartid = read_tp();

	assert(current[hartid]);
	char *pa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), (void *)buf);
	sprint(pa);
	return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code)
{
	uint64 hartid = read_tp();

	sprint("hartid = %lld: User(pid = %d) exit with code:%d.\n", hartid, current[hartid]->pid, code);
	// reclaim the current process, and reschedule. added @lab3_1

	if (current[hartid]->parent != NULL && current[hartid]->parent->status == BLOCKED)
	{
		if (current[hartid]->parent->waitpid == -1) {
			insert_to_ready_queue(current[hartid]->parent);
		}
		else if (current[hartid]->parent->waitpid == current[hartid]->pid)
		{
			insert_to_ready_queue(current[hartid]->parent);
		}
	}

	free_process(current[hartid]);
	set_idle(hartid);
	schedule();
	return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page()
{
	uint64 hartid = read_tp();

	void *pa = alloc_page();
	uint64 va;
	// if there are previously reclaimed pages, use them first (this does not change the
	// size of the heap)
	if (current[hartid]->user_heap.free_pages_count > 0)
	{
		va = current[hartid]->user_heap.free_pages_address[--current[hartid]->user_heap.free_pages_count];
		assert(va < current[hartid]->user_heap.heap_top);
	}
	else
	{
		// otherwise, allocate a new page (this increases the size of the heap by one page)
		va = current[hartid]->user_heap.heap_top;
		current[hartid]->user_heap.heap_top += PGSIZE;

		current[hartid]->mapped_info[HEAP_SEGMENT].npages++;
	}
	user_vm_map((pagetable_t)current[hartid]->pagetable, va, PGSIZE, (uint64)pa,
				prot_to_type(PROT_WRITE | PROT_READ, 1));
	return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va)
{
	uint64 hartid = read_tp();

	user_vm_unmap((pagetable_t)current[hartid]->pagetable, va, PGSIZE, 1);
	// add the reclaimed page to the free page list
	current[hartid]->user_heap.free_pages_address[current[hartid]->user_heap.free_pages_count++] = va;
	return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork()
{
	uint64 hartid = read_tp();

	sprint("hartid = %lld: User(pid = %d) call fork.\n", hartid, current[hartid]->pid);
	return do_fork(current[hartid]);
}

//
// kerenl entry point of yield. added @lab3_2
//
ssize_t sys_user_yield()
{
	uint64 hartid = read_tp();

	// TODO (lab3_2): implment the syscall of yield.
	// hint: the functionality of yield is to give up the processor. therefore,
	// we should set the status of currently running process to READY, insert it in
	// the rear of ready queue, and finally, schedule a READY process to run.
	// panic( "You need to implement the yield syscall in lab3_2.\n" );
	insert_to_ready_queue(current[hartid]);
	set_idle(hartid);
	schedule();
	return 0;
}

//
// open file
//
ssize_t sys_user_open(char *pathva, int flags)
{
	uint64 hartid = read_tp();

	char *pathpa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), pathva);
	if (pathpa[0] == '/')
	{
		return do_open(pathpa, flags);
	}
	else
	{
		char cwd[256];
		memset(cwd, 0, sizeof(cwd));
		get_pwd(cwd, current[hartid]->pfiles->cwd);
		strcat(cwd, "/");
		strcat(cwd, pathpa);
		return do_open(cwd, flags);
	}
}

//
// read file
//
ssize_t sys_user_read(int fd, char *bufva, uint64 count)
{
	uint64 hartid = read_tp();

	int i = 0;
	while (i < count)
	{ // count can be greater than page size
		uint64 addr = (uint64)bufva + i;
		uint64 pa = lookup_pa((pagetable_t)current[hartid]->pagetable, addr);
		uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
		uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
		uint64 r = do_read(fd, (char *)pa + off, len);
		i += r;
		if (r < len)
			return i;
	}
	return count;
}

//
// write file
//
ssize_t sys_user_write(int fd, char *bufva, uint64 count)
{
	uint64 hartid = read_tp();

	int i = 0;
	while (i < count)
	{ // count can be greater than page size
		uint64 addr = (uint64)bufva + i;
		uint64 pa = lookup_pa((pagetable_t)current[hartid]->pagetable, addr);
		uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
		uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
		uint64 r = do_write(fd, (char *)pa + off, len);
		i += r;
		if (r < len)
			return i;
	}
	return count;
}

//
// lseek file
//
ssize_t sys_user_lseek(int fd, int offset, int whence)
{
	return do_lseek(fd, offset, whence);
}

//
// read vinode
//
ssize_t sys_user_stat(int fd, struct istat *istat)
{
	uint64 hartid = read_tp();

	struct istat *pistat = (struct istat *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), istat);
	return do_stat(fd, pistat);
}

//
// read disk inode
//
ssize_t sys_user_disk_stat(int fd, struct istat *istat)
{
	uint64 hartid = read_tp();

	struct istat *pistat = (struct istat *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), istat);
	return do_disk_stat(fd, pistat);
}

//
// close file
//
ssize_t sys_user_close(int fd)
{
	return do_close(fd);
}

//
// lib call to opendir
//
ssize_t sys_user_opendir(char *pathva)
{
	uint64 hartid = read_tp();

	char *pathpa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), pathva);
	if (pathpa[0] == '/')
	{
		return do_opendir(pathpa);
	}
	else
	{
		char cwd[256];
		memset(cwd, 0, sizeof(cwd));
		get_pwd(cwd, current[hartid]->pfiles->cwd);
		strcat(cwd, "/");
		strcat(cwd, pathpa);
		return do_opendir(cwd);
	}
}

//
// lib call to readdir
//
ssize_t sys_user_readdir(int fd, struct dir *vdir)
{
	uint64 hartid = read_tp();

	struct dir *pdir = (struct dir *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), vdir);
	return do_readdir(fd, pdir);
}

//
// lib call to mkdir
//
ssize_t sys_user_mkdir(char *pathva)
{
	uint64 hartid = read_tp();

	char *pathpa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), pathva);
	if (pathpa[0] == '/')
	{
		return do_mkdir(pathpa);
	}
	else
	{
		char cwd[256];
		memset(cwd, 0, sizeof(cwd));
		get_pwd(cwd, current[hartid]->pfiles->cwd);
		strcat(cwd, "/");
		strcat(cwd, pathpa);
		return do_mkdir(cwd);
	}
}

//
// lib call to closedir
//
ssize_t sys_user_closedir(int fd)
{
	return do_closedir(fd);
}

//
// lib call to link
//
ssize_t sys_user_link(char *vfn1, char *vfn2)
{
	uint64 hartid = read_tp();

	char *pfn1 = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), (void *)vfn1);
	char *pfn2 = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), (void *)vfn2);
	return do_link(pfn1, pfn2);
}

//
// lib call to unlink
//
ssize_t sys_user_unlink(char *vfn)
{
	uint64 hartid = read_tp();

	char *pfn = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), (void *)vfn);
	return do_unlink(pfn);
}

int sys_user_exec(char *pathva, char *arg)
{
	uint64 hartid = read_tp();

	char *pathpa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), pathva);
	char *argpa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), arg);
	int ret = do_exec(pathpa, argpa);
	return ret;
}

extern process procs[NPROC];
int sys_user_wait(int pid)
{
	uint64 hartid = read_tp();

	if (pid == -1)
	{
		current[hartid]->status = BLOCKED;
		current[hartid]->waitpid = -1;
		set_idle(hartid);
		schedule();
		return current[hartid]->waitpid;
	}
	else if (pid > 0)
	{
		if (procs[pid].parent != current[hartid])
		{
			// panic("wait for a process that is not a child.\n");
			return -1;
		}
		current[hartid]->status = BLOCKED;
		current[hartid]->waitpid = pid;
		set_idle(hartid);
		schedule();
		return current[hartid]->waitpid;
	}
	else
	{
		return -1;
	}
}

ssize_t sys_user_pwd(char *buf)
{
	uint64 hartid = read_tp();

	char *pa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), buf);
	get_pwd(pa, current[hartid]->pfiles->cwd);
	return 0;
}

int sys_user_cd(char *path)
{
	uint64 hartid = read_tp();

	char *pa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), path);
	char cwd[256];
	if (pa[0] == '/')
	{
		strcpy(cwd, pa);
	}
	else
	{
		memset(cwd, 0, sizeof(cwd));
		get_pwd(cwd, current[hartid]->pfiles->cwd);
		strcat(cwd, "/");
		strcat(cwd, pa);
	}
	struct dentry *dentry = get_dentry(cwd);
	if (dentry == NULL)
	{
		return -1;
	}
	current[hartid]->pfiles->cwd = dentry;
	return 0;
}

ssize_t sys_user_scanf(char *in)
{
	uint64 hartid = read_tp();

	char *pa = (char *)user_va_to_pa((pagetable_t)(current[hartid]->pagetable), (void *)in);
	spike_file_read(stderr, pa, 256);
	return 0;
}

ssize_t sys_user_shell()
{
	uint64 hartid = read_tp();

	char buf[256];
	get_pwd(buf, current[hartid]->pfiles->cwd);
	sprint("%s:%s$ ", AUTHOR, buf);
	return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7)
{
	switch (a0)
	{
	case SYS_user_print:
		return sys_user_print((const char *)a1, a2);
	case SYS_user_exit:
		return sys_user_exit(a1);
	// added @lab2_2
	case SYS_user_allocate_page:
		return sys_user_allocate_page();
	case SYS_user_free_page:
		return sys_user_free_page(a1);
	case SYS_user_fork:
		return sys_user_fork();
	case SYS_user_yield:
		return sys_user_yield();
	// added @lab4_1
	case SYS_user_open:
		return sys_user_open((char *)a1, a2);
	case SYS_user_read:
		return sys_user_read(a1, (char *)a2, a3);
	case SYS_user_write:
		return sys_user_write(a1, (char *)a2, a3);
	case SYS_user_lseek:
		return sys_user_lseek(a1, a2, a3);
	case SYS_user_stat:
		return sys_user_stat(a1, (struct istat *)a2);
	case SYS_user_disk_stat:
		return sys_user_disk_stat(a1, (struct istat *)a2);
	case SYS_user_close:
		return sys_user_close(a1);
	// added @lab4_2
	case SYS_user_opendir:
		return sys_user_opendir((char *)a1);
	case SYS_user_readdir:
		return sys_user_readdir(a1, (struct dir *)a2);
	case SYS_user_mkdir:
		return sys_user_mkdir((char *)a1);
	case SYS_user_closedir:
		return sys_user_closedir(a1);
	// added @lab4_3
	case SYS_user_link:
		return sys_user_link((char *)a1, (char *)a2);
	case SYS_user_unlink:
		return sys_user_unlink((char *)a1);
	case SYS_user_exec:
		return sys_user_exec((char *)a1, (char *)a2);
	case SYS_user_wait:
		return sys_user_wait(a1);
	case SYS_user_pwd:
		return sys_user_pwd((char *)a1);
	case SYS_user_cd:
		return sys_user_cd((char *)a1);
	case SYS_user_scanf:
		return sys_user_scanf((char *)a1);
	case SYS_user_shell:
		return sys_user_shell();
	default:
		panic("Unknown syscall %ld \n", a0);
	}
}
