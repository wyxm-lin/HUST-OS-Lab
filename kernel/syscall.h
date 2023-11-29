/*
 * define the syscall numbers of PKE OS kernel.
 */
#ifndef _SYSCALL_H_
#define _SYSCALL_H_

// syscalls of PKE OS kernel. append below if adding new syscalls.
#define SYS_user_base 64
#define SYS_user_print (SYS_user_base + 0)
#define SYS_user_exit (SYS_user_base + 1)
// DONE: work
#define SYS_user_backtrace (SYS_user_base + 2)
extern uint64 symtab_addr_global; // 符号表加载至内存的地址
extern uint64 strtab_addr_global; // 字符串表加载至内存的地址
extern uint64 symtab_size_global; // 符号表加载至内存的地址
extern uint64 strtab_size_global; // 字符串表加载至内存的地址

long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7);

#endif
