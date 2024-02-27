/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"
#include "pmm.h"
#include "vmm.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"
#include "spike_interface/atomic.h"
#include "sync_utils.h"
#include "config.h"

// process is a structure defined in kernel/process.h
process user_app[NCPU];

//
// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point of
// S-mode trap vector). added @lab2_1
//
extern char trap_sec_start[];

//
// turn on paging. added @lab2_1
//
void enable_paging() {
  // int hartid = read_tp();
  // write the pointer to kernel page (table) directory into the CSR of "satp".
  write_csr(satp, MAKE_SATP(g_kernel_pagetable));

  // refresh tlb to invalidate its content.
  flush_tlb(); // comment:tlb Translation Lookaside Buffer
}

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
void load_user_program(process *proc) {
  uint64 hartid = read_tp();

  sprint("hartid = %lld: User application is loading.\n", hartid); // comment:根据doc输出修改此处
  // allocate a page to store the trapframe. alloc_page is defined in kernel/pmm.c. added @lab2_1
  proc->trapframe = (trapframe *)alloc_page();
  memset(proc->trapframe, 0, sizeof(trapframe));

  // allocate a page to store page directory. added @lab2_1
  proc->pagetable = (pagetable_t)alloc_page();
  memset((void *)proc->pagetable, 0, PGSIZE);

  // allocate pages to both user-kernel stack and user app itself. added @lab2_1
  proc->kstack = (uint64)alloc_page() + PGSIZE;   //user kernel stack top
  uint64 user_stack = (uint64)alloc_page();       //phisical address of user stack bottom

  // USER_STACK_TOP = 0x7ffff000, defined in kernel/memlayout.h
  proc->trapframe->regs.sp = USER_STACK_TOP;  //virtual address of user stack top
  proc->trapframe->regs.tp = hartid; // comment: qwq(牢记lab1_challenge3的教训)

  sprint("hartid = %lld: user frame 0x%lx, user stack 0x%lx, user kstack 0x%lx \n", hartid, proc->trapframe,
         proc->trapframe->regs.sp, proc->kstack);

  // load_bincode_from_host_elf() is defined in kernel/elf.c
  load_bincode_from_host_elf(proc);

  // populate the page table of user application. added @lab2_1
  // map user stack in userspace, user_vm_map is defined in kernel/vmm.c
  user_vm_map((pagetable_t)proc->pagetable, USER_STACK_TOP - PGSIZE, PGSIZE, user_stack,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  // map trapframe in user space (direct mapping as in kernel space).
  user_vm_map((pagetable_t)proc->pagetable, (uint64)proc->trapframe, PGSIZE, (uint64)proc->trapframe,
         prot_to_type(PROT_WRITE | PROT_READ, 0));

  // map S-mode trap vector section in user space (direct mapping as in kernel space)
  // here, we assume that the size of usertrap.S is smaller than a page.
  user_vm_map((pagetable_t)proc->pagetable, (uint64)trap_sec_start, PGSIZE, (uint64)trap_sec_start,
         prot_to_type(PROT_READ | PROT_EXEC, 0));
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
static int BootCount = 0;
extern spinlock_t BootLock;
MyStatus KernPmmStatus = No;
MyStatus KernVmmStatus = No;

// NOTE:此函数有点特殊 里面有一个uint64 hartid 而且初始化为0，需要仔细判断是否需要修改
int s_start(void) {
  uint64 hartid = read_tp();
  sprint("hartid = %lld: Enter supervisor mode...\n", hartid); // DoNotUnderstand: lld -> lld (使用llu会Load access fault)
  // panic("stop");
  // in the beginning, we use Bare mode (direct) memory mapping as in lab1.
  // but now, we are going to switch to the paging mode @lab2_1.
  // note, the code still works in Bare mode when calling pmm_init() and kern_vm_init().
  write_csr(satp, 0);

  // init phisical memory manager
  if (KernPmmStatus == No) {
       KernPmmStatus = Yes;
       pmm_init(); // comment: 此函数貌似只需要执行一次
  }

  // NOTE: 这俩语句 根据doc的输出 应该放这
  spinlock_unlock(&BootLock); // 释放锁
  

  // build the kernel page table
  if (KernVmmStatus == No) {
       KernVmmStatus = Yes;
       kern_vm_init(); // comment: 此函数貌似只需要执行一次
       enable_paging();
  }

  // sprint("kernel page table is on \n"); // 此行输出delete(根据doc的输出判断)
  sync_barrier(&BootCount, NCPU); // comment: 设置同步点

  // the application code (elf) is first loaded into memory, and then put into execution
  load_user_program(&user_app[hartid]);

  sprint("hartid = %lld: Switch to user mode...\n", hartid);
  
//   uint64 hartid = 0; // comment: added by teaching assistant
  
  vm_alloc_stage[hartid] = 1; // comment:标志位罢了
  // switch_to() is defined in kernel/process.c
  switch_to(&user_app[hartid]);

  // we should never reach here.
  return 0;
}
