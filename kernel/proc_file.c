/*
 * Interface functions between file system and kernel/processes. added @lab4_1
 */

#include "proc_file.h"

#include "hostfs.h"
#include "pmm.h"
#include "process.h"
#include "ramdev.h"
#include "rfs.h"
#include "riscv.h"
#include "spike_interface/spike_file.h"
#include "spike_interface/spike_utils.h"
#include "util/functions.h"
#include "util/string.h"
#include "elf.h"
#include "vmm.h"

//
// initialize file system
//
void fs_init(void)
{
	// initialize the vfs
	vfs_init();

	// register hostfs and mount it as the root
	if (register_hostfs() < 0)
		panic("fs_init: cannot register hostfs.\n");
	struct device *hostdev = init_host_device("HOSTDEV");
	vfs_mount("HOSTDEV", MOUNT_AS_ROOT);

	// register and mount rfs
	if (register_rfs() < 0)
		panic("fs_init: cannot register rfs.\n");
	struct device *ramdisk0 = init_rfs_device("RAMDISK0");
	rfs_format_dev(ramdisk0);
	vfs_mount("RAMDISK0", MOUNT_DEFAULT);
}

//
// initialize a proc_file_management data structure for a process.
// return the pointer to the page containing the data structure.
//
proc_file_management *init_proc_file_management(void)
{
	uint64 hartid = read_tp();

	proc_file_management *pfiles = (proc_file_management *)alloc_page();
	pfiles->cwd = vfs_root_dentry; // by default, cwd is the root
	pfiles->nfiles = 0;

	for (int fd = 0; fd < MAX_FILES; ++fd)
		pfiles->opened_files[fd].status = FD_NONE;

	sprint("hartid = %lld: FS: created a file management struct for a process.\n", hartid);
	return pfiles;
}

//
// reclaim the open-file management data structure of a process.
// note: this function is not used as PKE does not actually reclaim a process.
//
void reclaim_proc_file_management(proc_file_management *pfiles)
{
	free_page(pfiles);
	return;
}

//
// get an opened file from proc->opened_file array.
// return: the pointer to the opened file structure.
//
struct file *get_opened_file(int fd)
{
	uint64 hartid = read_tp();

	struct file *pfile = NULL;

	// browse opened file list to locate the fd
	for (int i = 0; i < MAX_FILES; ++i)
	{
		pfile = &(current[hartid]->pfiles->opened_files[i]); // file entry
		if (i == fd)
			break;
	}
	if (pfile == NULL)
		panic("do_read: invalid fd!\n");
	return pfile;
}

//
// open a file named as "pathname" with the permission of "flags".
// return: -1 on failure; non-zero file-descriptor on success.
//
int do_open(char *pathname, int flags)
{
	uint64 hartid = read_tp();

	struct file *opened_file = NULL;
	if ((opened_file = vfs_open(pathname, flags)) == NULL)
		return -1;

	int fd = 0;
	if (current[hartid]->pfiles->nfiles >= MAX_FILES)
	{
		panic("do_open: no file entry for current process!\n");
	}
	struct file *pfile;
	for (fd = 0; fd < MAX_FILES; ++fd)
	{
		pfile = &(current[hartid]->pfiles->opened_files[fd]);
		if (pfile->status == FD_NONE)
			break;
	}

	// initialize this file structure
	memcpy(pfile, opened_file, sizeof(struct file));

	++current[hartid]->pfiles->nfiles;
	return fd;
}

//
// read content of a file ("fd") into "buf" for "count".
// return: actual length of data read from the file.
//
int do_read(int fd, char *buf, uint64 count)
{
	struct file *pfile = get_opened_file(fd);

	if (pfile->readable == 0)
		panic("do_read: no readable file!\n");

	char buffer[count + 1];
	int len = vfs_read(pfile, buffer, count);
	buffer[count] = '\0';
	strcpy(buf, buffer);
	return len;
}

//
// write content ("buf") whose length is "count" to a file "fd".
// return: actual length of data written to the file.
//
int do_write(int fd, char *buf, uint64 count)
{
	struct file *pfile = get_opened_file(fd);

	if (pfile->writable == 0)
		panic("do_write: cannot write file!\n");

	int len = vfs_write(pfile, buf, count);
	return len;
}

//
// reposition the file offset
//
int do_lseek(int fd, int offset, int whence)
{
	struct file *pfile = get_opened_file(fd);
	return vfs_lseek(pfile, offset, whence);
}

//
// read the vinode information
//
int do_stat(int fd, struct istat *istat)
{
	struct file *pfile = get_opened_file(fd);
	return vfs_stat(pfile, istat);
}

//
// read the inode information on the disk
//
int do_disk_stat(int fd, struct istat *istat)
{
	struct file *pfile = get_opened_file(fd);
	return vfs_disk_stat(pfile, istat);
}

//
// close a file
//
int do_close(int fd)
{
	struct file *pfile = get_opened_file(fd);
	return vfs_close(pfile);
}

//
// open a directory
// return: the fd of the directory file
//
int do_opendir(char *pathname)
{
	uint64 hartid = read_tp();

	struct file *opened_file = NULL;
	if ((opened_file = vfs_opendir(pathname)) == NULL)
		return -1;

	int fd = 0;
	struct file *pfile;
	for (fd = 0; fd < MAX_FILES; ++fd)
	{
		pfile = &(current[hartid]->pfiles->opened_files[fd]);
		if (pfile->status == FD_NONE)
			break;
	}
	if (pfile->status != FD_NONE) // no free entry
		panic("do_opendir: no file entry for current process!\n");

	// initialize this file structure
	memcpy(pfile, opened_file, sizeof(struct file));

	++current[hartid]->pfiles->nfiles;
	return fd;
}

//
// read a directory entry
//
int do_readdir(int fd, struct dir *dir)
{
	struct file *pfile = get_opened_file(fd);
	return vfs_readdir(pfile, dir);
}

//
// make a new directory
//
int do_mkdir(char *pathname)
{
	return vfs_mkdir(pathname);
}

//
// close a directory
//
int do_closedir(int fd)
{
	struct file *pfile = get_opened_file(fd);
	return vfs_closedir(pfile);
}

//
// create hard link to a file
//
int do_link(char *oldpath, char *newpath)
{
	return vfs_link(oldpath, newpath);
}

//
// remove a hard link to a file
//
int do_unlink(char *path)
{
	return vfs_unlink(path);
}

int do_exec(char *path_, char *arg_)
{
	uint64 hartid = read_tp();
	
	// 在释放内存前，将参数保存
	int PathLen = strlen(path_);
	char path[PathLen + 1];
	strcpy(path, path_);
	int ArgLen = strlen(arg_);
	char arg[ArgLen + 1];
	strcpy(arg, arg_);

	// 释放当前进程的资源
	exec_clean(current[hartid]);

	// 一级指针
	uint64 argv_va = current[hartid]->trapframe->regs.sp - ArgLen - 1;
	argv_va = argv_va - argv_va % 8; // 按8字节对齐(方便指针指向该位置)
	uint64 argv_pa = (uint64)user_va_to_pa(current[hartid]->pagetable, (void *)argv_va);
	strcpy((char *)argv_pa, arg);

	// 二级指针
	uint64 argvs_va = argv_va - 8; // 因为目前只考虑一个参数，故而一级指针只构建一个，二级指针的位置目前就设定在一级指针后面，并且这一区域的大小刚好只是一个指针的大小
	uint64 argvs_pa = (uint64)user_va_to_pa(current[hartid]->pagetable, (void *)argvs_va);
	*(uint64 *)argvs_pa = argv_va; // 存储一级指针的虚地址

	current[hartid]->trapframe->regs.a0 = 1;						// 设置argc的值(此处为1)
	current[hartid]->trapframe->regs.a1 = argvs_va;					// 设置argv的值
	current[hartid]->trapframe->regs.sp = argvs_va - argvs_va % 16; // 按照16对齐

	load_bincode_from_host_elf(current[hartid], path); // 此处修改了s态切换到用户态的返回地址

	return -1;
}
