/*
 * Utility functions for process management. 
 *
 * Note: in Lab1, only one process (i.e., our user application) exists. Therefore, 
 * PKE OS at this stage will set "current" to the loaded user application, and also
 * switch to the old "current" process after trap handling.
 */

#include "riscv.h"
#include "strap.h"
#include "config.h"
#include "process.h"
#include "elf.h"
#include "string.h"

#include "spike_interface/spike_utils.h"

//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe*);

// current points to the currently running user-mode application.
process* current = NULL; // ANNOTATE:声明在process.h中的 extern process* current = NULL;
uint64 symtab_addr_global = 0; // 符号表加载至内存的地址
uint64 strtab_addr_global = 0; // 字符串表加载至内存的地址
uint64 symtab_size_global = 0; // 符号表加载至内存的地址
uint64 strtab_size_global = 0; // 字符串表加载至内存的地址
//
// switch to a user-mode process
//
void switch_to(process* proc) {
  assert(proc);
  current = proc;

  // write the smode_trap_vector (64-bit func. address) defined in kernel/strap_vector.S
  // to the stvec privilege register, such that trap handler pointed by smode_trap_vector
  // will be triggered when an interrupt occurs in S mode.
  write_csr(stvec, (uint64)smode_trap_vector); // ANNOTATE:smode_trap_vector定义在 strap_Vector.S中，是一个64位的函数地址，将其写入stvec寄存器，这样当S模式下发生中断时，smode_trap_vector指向的中断处理程序将被触发。
                                               // smode_trap_vector 声明在本文件的第19行， 是一个extern char smode_trap_vector[]; 也就是说，smode_trap_vector是一个字符数组，数组中存放的是一个64位的函数地址。

  // set up trapframe values (in process structure) that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;  // process's kernel stack
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // SSTATUS_SPP and SSTATUS_SPIE are defined in kernel/riscv.h
  // set S Previous Privilege mode (the SSTATUS_SPP bit in sstatus register) to User mode.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode // ANNOTATE:切换为用户模式
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  // write x back to 'sstatus' register to enable interrupts, and sret destination mode.
  write_csr(sstatus, x);

  // set S Exception Program Counter (sepc register) to the elf entry pc.
  write_csr(sepc, proc->trapframe->epc); // ANNOTATE:未懂

  // return_to_user() is defined in kernel/strap_vector.S. switch to user mode with sret.
  return_to_user(proc->trapframe);
}
