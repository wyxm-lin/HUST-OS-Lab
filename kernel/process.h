#ifndef _PROC_H_
#define _PROC_H_

#include "riscv.h"

// ADD
typedef struct m_rib {
  uint64 cap; // 内存块实际大小
  uint64 next;
}m_rib;

typedef struct trapframe_t {
  // space to store context (all common registers)
  /* offset:0   */ riscv_regs regs;

  // process's "user kernel" stack
  /* offset:248 */ uint64 kernel_sp;
  // pointer to smode_trap_handler
  /* offset:256 */ uint64 kernel_trap;
  // saved user process counter
  /* offset:264 */ uint64 epc;

  // kernel page table. added @lab2_1
  /* offset:272 */ uint64 kernel_satp;
}trapframe;

// the extremely simple definition of process, used for begining labs of PKE
typedef struct process_t {
  // pointing to the stack used in trap handling.
  uint64 kstack;
  // user page table
  pagetable_t pagetable;
  // trapframe storing the context of a (User mode) process.
  trapframe* trapframe;
  // ADD
  uint64 rib_used; // 存放虚拟地址
  uint64 rib_free; // 存放虚拟地址
}process;

// switch to run user app
void switch_to(process*);

// current running process
extern process* current;

// address of the first free page in our simple heap. added @lab2_2
extern uint64 g_ufree_page;

// ADD:
void* better_alloc(uint64 n); // 分配pa + 映射到pagetable + 返回va
void better_free(uint64 va); // 根据va释放内存(当完整的页被释放时，释放pa)
void best_free(uint64 va); // 释放内存(当完整的页被释放时，释放pa) + 空闲块合并

#endif
