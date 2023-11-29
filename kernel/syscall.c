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

// NOTE:
#include "elf.h"

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

// NOTE:地址的位宽(注意下)
static int backtrace(uint32 addr) {
  // 寻找小于该addr的最大的符号表地址
  uint32 symtab_name = 0;
  uint32 symtab_near_addr = 0; // 最接近的地址
  char* data = (char*)symtab_addr_global;
  for (int i = 0; i < symtab_size_global; ) {
    symtab_entry* entry = (symtab_entry*)(data + i);
    uint8 Type = entry->info & 0x0f; // Type为2表示该符号是函数
    if (Type == 2 && entry->value <= addr && entry->value > symtab_near_addr) {
      symtab_near_addr = entry->value;
      symtab_name = entry->name;
    }
    i += 24;
  }
  // 获取该符号表的名称
  char* strtab = (char*)strtab_addr_global;
  sprint("%s\n", strtab + symtab_name);
  if (strcmp("main", strtab + symtab_name) == 0) return 1;
  else return 0;
}

// DONE:work
// TODO:此处仍然可以使用current变量
ssize_t sys_user_backtrace(int x) { 
  // 获取用户态的帧地址
  uint64 fp = current->trapframe->regs.s0; 
  uint64* fp_data;
  uint64* ra_data;
  // 叶节点的帧(区别于其他栈帧的回溯)
  fp_data = (uint64*)(fp - 8); // 从该地址处读取内容
  fp = *fp_data; // 回到print_backtrace的栈帧
  while (1) {
    ra_data = (uint64*)(fp - 8); // 从该地址处读取内容
    fp_data = (uint64*)(fp - 16); // 从该地址处读取内容
    fp = *fp_data;
    int state = backtrace((uint32)(*ra_data));
    if (state)
      break;
    x --;
    if (x <= 0) break;
  }
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
