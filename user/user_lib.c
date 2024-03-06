/*
 * The supporting library for applications.
 * Actually, supporting routines for applications are catalogued as the user
 * library. we don't do that in PKE to make the relationship between application
 * and user library more straightforward.
 */

#include "user_lib.h"
#include "util/types.h"
#include "util/snprintf.h"
#include "kernel/syscall.h"
#include "util/string.h"

uint64 do_user_call(uint64 sysnum, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6,
					uint64 a7)
{
	int ret;

	// before invoking the syscall, arguments of do_user_call are already loaded into the argument
	// registers (a0-a7) of our (emulated) risc-v machine.
	asm volatile(
		"ecall\n"
		"sw a0, %0" // returns a 32-bit value
		: "=m"(ret)
		:
		: "memory");

	return ret;
}

//
// printu() supports user/lab1_1_helloworld.c
//
int printu(const char *s, ...)
{
	va_list vl;
	va_start(vl, s);

	char out[256]; // fixed buffer size.
	int res = vsnprintf(out, sizeof(out), s, vl);
	va_end(vl);
	const char *buf = out;
	size_t n = res < sizeof(out) ? res : sizeof(out);

	// make a syscall to implement the required functionality.
	return do_user_call(SYS_user_print, (uint64)buf, n, 0, 0, 0, 0, 0);
}

//
// applications need to call exit to quit execution.
//
int exit(int code)
{
	return do_user_call(SYS_user_exit, code, 0, 0, 0, 0, 0, 0);
}

//
// lib call to naive_malloc
//
void *naive_malloc()
{
	return (void *)do_user_call(SYS_user_allocate_page, 0, 0, 0, 0, 0, 0, 0);
}

//
// lib call to naive_free
//
void naive_free(void *va)
{
	do_user_call(SYS_user_free_page, (uint64)va, 0, 0, 0, 0, 0, 0);
}

//
// lib call to naive_fork
int fork()
{
	return do_user_call(SYS_user_fork, 0, 0, 0, 0, 0, 0, 0);
}

//
// lib call to yield
//
void yield()
{
	do_user_call(SYS_user_yield, 0, 0, 0, 0, 0, 0, 0);
}

//
// lib call to open
//
int open(const char *pathname, int flags)
{
	return do_user_call(SYS_user_open, (uint64)pathname, flags, 0, 0, 0, 0, 0);
}

//
// lib call to read
//
int read_u(int fd, void *buf, uint64 count)
{
	return do_user_call(SYS_user_read, fd, (uint64)buf, count, 0, 0, 0, 0);
}

//
// lib call to write
//
int write_u(int fd, void *buf, uint64 count)
{
	return do_user_call(SYS_user_write, fd, (uint64)buf, count, 0, 0, 0, 0);
}

//
// lib call to seek
//
int lseek_u(int fd, int offset, int whence)
{
	return do_user_call(SYS_user_lseek, fd, offset, whence, 0, 0, 0, 0);
}

//
// lib call to read file information
//
int stat_u(int fd, struct istat *istat)
{
	return do_user_call(SYS_user_stat, fd, (uint64)istat, 0, 0, 0, 0, 0);
}

//
// lib call to read file information from disk
//
int disk_stat_u(int fd, struct istat *istat)
{
	return do_user_call(SYS_user_disk_stat, fd, (uint64)istat, 0, 0, 0, 0, 0);
}

//
// lib call to open dir
//
int opendir_u(const char *dirname)
{
	return do_user_call(SYS_user_opendir, (uint64)dirname, 0, 0, 0, 0, 0, 0);
}

//
// lib call to read dir
//
int readdir_u(int fd, struct dir *dir)
{
	return do_user_call(SYS_user_readdir, fd, (uint64)dir, 0, 0, 0, 0, 0);
}

//
// lib call to make dir
//
int mkdir_u(const char *pathname)
{
	return do_user_call(SYS_user_mkdir, (uint64)pathname, 0, 0, 0, 0, 0, 0);
}

//
// lib call to close dir
//
int closedir_u(int fd)
{
	return do_user_call(SYS_user_closedir, fd, 0, 0, 0, 0, 0, 0);
}

//
// lib call to link
//
int link_u(const char *fn1, const char *fn2)
{
	return do_user_call(SYS_user_link, (uint64)fn1, (uint64)fn2, 0, 0, 0, 0, 0);
}

//
// lib call to unlink
//
int unlink_u(const char *fn)
{
	return do_user_call(SYS_user_unlink, (uint64)fn, 0, 0, 0, 0, 0, 0);
}

//
// lib call to close
//
int close(int fd)
{
	return do_user_call(SYS_user_close, fd, 0, 0, 0, 0, 0, 0);
}

// added @ lab4_challenge3
int exec(const char *path, char *arg)
{
	return do_user_call(SYS_user_exec, (uint64)path, (uint64)arg, 0, 0, 0, 0, 0);
}

int wait(int pid)
{
	return do_user_call(SYS_user_wait, pid, 0, 0, 0, 0, 0, 0);
}

int execPlus(const char *path, char *argv[])
{
	// TODO
	return 0;
}

int pwd_u(char *buf)
{
	return do_user_call(SYS_user_pwd, (uint64)buf, 0, 0, 0, 0, 0, 0);
}

int cd_u(char *path)
{
	return do_user_call(SYS_user_cd, (uint64)path, 0, 0, 0, 0, 0, 0);
}

int scanf_u(const char *s, ...)
{
	va_list vl;
	va_start(vl, s);
	char in[256]; // fixed buffer size.
	memset(in, 0, sizeof(in));
	do_user_call(SYS_user_scanf, (uint64)in, 0, 0, 0, 0, 0, 0);
	vsnscanf(in, s, vl);
	va_end(vl);
	return 0;
}

int shell()
{
	return do_user_call(SYS_user_shell, 0, 0, 0, 0, 0, 0, 0);
}

static void sscanf(char *dst, char *src, int *idx)
{
	int i = *idx;
	while (src[i] == ' ')
		i++;
	while (src[i] != ' ' && src[i] != '\0')
	{
		*dst = src[i];
		dst++;
		i++;
	}
	*idx = i;
	*dst = '\0';
}

void PWD();
void CD(char *command, int *idx);
void MKDIR(char *command, int *idx);
void LS(char *command, int *idx);
void CAT(char *command, int *idx);
void TOUCH(char *command, int *idx);
void ECHO(char *command, int *idx);

int work(char *commandlist)
{
	char command[256];
	int idx = 0;
	sscanf(command, commandlist, &idx);

	if (strcmp(command, "") == 0 || strcmp(command, "\n") == 0)
	{
		// do nothing
	}
	else if (strcmp(command, "exit") == 0)
	{
		return -1;
	}
	else if (strcmp(command, "pwd") == 0)
	{
		PWD();
	}
	else if (strcmp(command, "cd") == 0)
	{
		CD(commandlist, &idx);
	}
	else if (strcmp(command, "mkdir") == 0)
	{
		MKDIR(commandlist, &idx);
	}
	else if (strcmp(command, "ls") == 0)
	{
		LS(commandlist, &idx);
	}
	else if (strcmp(command, "cat") == 0)
	{
		CAT(commandlist, &idx);
	}
	else if (strcmp(command, "touch") == 0)
	{
		TOUCH(commandlist, &idx);
	}
	else if (strcmp(command, "echo") == 0)
	{
		ECHO(commandlist, &idx);
	}
	else
	{
		printu("command not found: %s\n", command);
	}
	return 0;
}

void PWD()
{
	char buf[256];
	memset(buf, 0, sizeof(buf));
	pwd_u(buf);
	printu("%s\n", buf);
}

void CD(char *commandlist, int *idx)
{
	char arg[256];
	sscanf(arg, commandlist, idx);
	int rc = cd_u(arg);
	if (rc == -1)
	{
		printu("cd: %s: No such file or directory\n", arg);
	}
}

void MKDIR(char *command, int *idx)
{
	char arg[256];
	sscanf(arg, command, idx);
	int rc = mkdir_u(arg);
	if (rc == -1)
	{
		printu("mkdir: %s: File exists\n", arg);
	}
}

void LS(char *command, int *idx)
{
	char arg[256];
	sscanf(arg, command, idx);
	int fd = opendir_u(arg);
	if (fd == -1)
	{
		printu("ls: cannot access '%s': No such file or directory\n", arg);
		return;
	}
	struct dir dir;
	while (readdir_u(fd, &dir) != -1)
	{
		printu("%s\n", dir.name);
	}
	closedir_u(fd);
}

void CAT(char *command, int *idx)
{
	char arg[256];
	sscanf(arg, command, idx);
	int fd = open(arg, O_RDWR);
	if (fd == -1)
	{
		printu("cat: %s: No such file or directory\n", arg);
		return;
	}
	char buf[512];
	memset(buf, 0, sizeof(buf));
	read_u(fd, buf, sizeof(buf));
	printu("%s\n", buf);
	close(fd);
}

void TOUCH(char *command, int *idx)
{
	char arg[256];
	sscanf(arg, command, idx);
	int fd = open(arg, O_CREAT | O_RDWR);
	if (fd == -1)
	{
		printu("touch: %s: No such file or directory\n", arg);
		return;
	}
	{
		char buf[] = "hello world\n";
		write_u(fd, buf, sizeof(buf));
	}
	close(fd);
}

void ECHO(char *command, int *idx)
{
	char arg[256];
	while (TRUE)
	{
		sscanf(arg, command, idx);
		if (strcmp(arg, "") == 0)
		{
			break;
		}
		printu("%s ", arg);
	}
	printu("\n");
}

int sem_new(int value)
{
	return do_user_call(SYS_user_sem_new, value, 0, 0, 0, 0, 0, 0);
}

void sem_P(int sem)
{
	do_user_call(SYS_user_sem_P, sem, 0, 0, 0, 0, 0, 0);
}

void sem_V(int sem)
{
	do_user_call(SYS_user_sem_V, sem, 0, 0, 0, 0, 0, 0);
}

void printpa(int* va)
{
  do_user_call(SYS_user_printpa, (uint64)va, 0, 0, 0, 0, 0, 0);
}