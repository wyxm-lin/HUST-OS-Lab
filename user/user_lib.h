/*
 * header file to be used by applications.
 */

#ifndef _USER_LIB_H_
#define _USER_LIB_H_
#include "util/types.h"
#include "kernel/proc_file.h"

int printu(const char *s, ...);
int exit(int code);
void *naive_malloc(int n);
void naive_free(void *va);
int fork();
void yield();

// added @ lab4_1
int open(const char *pathname, int flags);
int read_u(int fd, void *buf, uint64 count);
int write_u(int fd, void *buf, uint64 count);
int lseek_u(int fd, int offset, int whence);
int stat_u(int fd, struct istat *istat);
int disk_stat_u(int fd, struct istat *istat);
int close(int fd);

// added @ lab4_2
int opendir_u(const char *pathname);
int readdir_u(int fd, struct dir *dir);
int mkdir_u(const char *pathname);
int closedir_u(int fd);

// added @ lab4_3
int link_u(const char *fn1, const char *fn2);
int unlink_u(const char *fn);

// added @ lab4_challenge3
int exec(const char *path, char *arg);
int wait(int pid);
int execPlus(const char *path, char *argv[]);

// added @ challengeX
int pwd_u(char *buf);
int cd_u(char *path);
int scanf_u(const char *s, ...);
int shell();
int work(char *commandlist);
int sem_new(int value);
void sem_P(int sem);
void sem_V(int sem);
void printpa(void* va);
void *better_malloc(int n);
void better_free(void *va);

#endif
