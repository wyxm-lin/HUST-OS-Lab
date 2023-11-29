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

#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

// DONE:work
// TODO:此处仍然可以使用current变量
ssize_t sys_user_backtrace(int x) { 
  sprint("lgm:start my backtrace\n");
  sprint("lgm:%d\n", x);
  uint64 user_stack_top = current->trapframe->regs.sp; // 获取用户态栈顶
  sprint("lgm:current->trapframe->regs.sp=%p\n", user_stack_top); // 此处的s0是用户程序的s0
  uint64 user_stack_ra = current->trapframe->regs.ra; // 获取用户态ra
  sprint("lgm:current->trapframe->regs.ra=%p\n", user_stack_ra); // 此处的ra是用户程序的ra
  uint64 user_fp = current->trapframe->regs.s0; // 获取用户态fp的地址
  sprint("lgm:current->trapframe->regs.s0=%p\n", user_fp); // 此处的s0是用户程序的s0
  sprint("lgm:--------------------------------------------------------\n");
  
  uint64* fp2 = (uint64*)(user_fp - 8); // 从该地址处读取内容
  uint64* ra;
  sprint("lgm:fp=%0x\n", *fp2);

  user_fp = *fp2; // 获取用户态fp的值
  fp2 = (uint64*)(user_fp - 16); // 从该地址处读取内容
  ra = (uint64*)(user_fp - 8); // 从该地址处读取内容 
  sprint("lgm:fp=%0x ra=%0x\n", *fp2, *ra);

  user_fp = *fp2; // 获取用户态fp的值
  fp2 = (uint64*)(user_fp - 16); // 从该地址处读取内容
  ra = (uint64*)(user_fp - 8); // 从该地址处读取内容 
  sprint("lgm:fp=%0x ra=%0x\n", *fp2, *ra);

  user_fp = *fp2; // 获取用户态fp的值
  fp2 = (uint64*)(user_fp - 16); // 从该地址处读取内容
  ra = (uint64*)(user_fp - 8); // 从该地址处读取内容 
  sprint("lgm:fp=%0x ra=%0x\n", *fp2, *ra);

  user_fp = *fp2; // 获取用户态fp的值
  fp2 = (uint64*)(user_fp - 16); // 从该地址处读取内容
  ra = (uint64*)(user_fp - 8); // 从该地址处读取内容 
  sprint("lgm:fp=%0x ra=%0x\n", *fp2, *ra);

  user_fp = *fp2; // 获取用户态fp的值
  fp2 = (uint64*)(user_fp - 16); // 从该地址处读取内容
  ra = (uint64*)(user_fp - 8); // 从该地址处读取内容 
  sprint("lgm:fp=%0x ra=%0x\n", *fp2, *ra);

  
  // long long* origin_fp = (long long*)user_fp; // 将用户态fp转换为long long*类型
  // sprint("lgm:origin_fp=%p\n", origin_fp);
  // user_fp = *origin_fp; // 获取用户态fp的值
  // sprint("lgm:0 : user_fp=%p\n", user_fp);
  // for (int i = 1; i <= 4; i++) {
  //   user_fp -= 8; // 移动到下一个fp的存储位置 // NOTE:此处为减法
  //   long long* fp = (long long*)user_fp; // 从该地址处读取内容
  //   user_fp = *fp; // 获取用户态fp的值
  //   sprint("lgm: %d : user_fp=%p\n", i, user_fp);
  // }
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
// DONE:work
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_backtrace:
      return sys_user_backtrace(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
