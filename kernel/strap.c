/*
 * Utility functions for trap handling in Supervisor mode.
 */

#include "riscv.h"
#include "process.h"
#include "strap.h"
#include "syscall.h"
#include "pmm.h"
#include "vmm.h"
#include "util/functions.h"

#include "spike_interface/spike_utils.h"


// DONE:
#include "memlayout.h"

//
// handling the syscalls. will call do_syscall() defined in kernel/syscall.c
//
static void handle_syscall(trapframe *tf) {
  // tf->epc points to the address that our computer will jump to after the trap handling.
  // for a syscall, we should return to the NEXT instruction after its handling.
  // in RV64G, each instruction occupies exactly 32 bits (i.e., 4 Bytes)
  tf->epc += 4;

  // TODO (lab1_1): remove the panic call below, and call do_syscall (defined in
  // kernel/syscall.c) to conduct real operations of the kernel side for a syscall.
  // IMPORTANT: return value should be returned to user app, or else, you will encounter
  // problems in later experiments!
  // panic( "call do_syscall to accomplish the syscall and lab1_1 here.\n" );
  // do_syscall(tf->regs.a0, tf->regs.a1, tf->regs.a2, tf->regs.a3, tf->regs.a4, tf->regs.a5, tf->regs.a6, tf->regs.a7);
  // note:将结果存入a0中
  tf->regs.a0 = do_syscall(tf->regs.a0, tf->regs.a1, tf->regs.a2, tf->regs.a3, tf->regs.a4, tf->regs.a5, tf->regs.a6, tf->regs.a7);
  
}

//
// global variable that store the recorded "ticks". added @lab1_3
static uint64 g_ticks = 0;
//
// added @lab1_3
//
void handle_mtimer_trap() {
  sprint("Ticks %d\n", g_ticks);
  // TODO (lab1_3): increase g_ticks to record this "tick", and then clear the "SIP"
  // field in sip register.
  // hint: use write_csr to disable the SIP_SSIP bit in sip.
  // panic( "lab1_3: increase g_ticks by one, and clear SIP field in sip register.\n" );
  g_ticks ++;
  write_csr(sip, 0);
}

//
// the page fault handler. added @lab2_3. parameters:
// sepc: the pc when fault happens;
// stval: the virtual address that causes pagefault when being accessed.
//
// DONE:修改了此处
void handle_user_page_fault(uint64 mcause, uint64 sepc, uint64 stval) {
  sprint("handle_page_fault: %lx\n", stval);
  switch (mcause) {
    case CAUSE_STORE_PAGE_FAULT:
      // TODO (lab2_3): implement the operations that solve the page fault to
      // dynamically increase application stack.
      // hint: first allocate a new physical page, and then, maps the new page to the
      // virtual address that causes the page fault.
      // panic( "You need to implement the operations that actually handle the page fault in lab2_3.\n" );
      {
        // 合法的虚拟地址应该是当前栈帧的页号的下一个页或者还是本页(栈空间应该是连续分配的) && 在分配的栈区内(在memlayout.h中 重新定义了栈区最多为32个页)
        int flag = 0;
        if (stval <= USER_STACK_TOP && stval > USER_STACK_TOP - MY_STACK_SIZE) {
          uint64 sp = current->trapframe->regs.sp;
          uint64 sp_page = sp >> PGSHIFT;
          uint64 stval_page = stval >> PGSHIFT;
          if (sp_page == stval_page || sp_page - 1 == stval_page) {
            flag = 1;
          }
        }

        // sprint("lgm:flag:%d\n", flag);
        if (flag) {
          void* pa = alloc_page();
          pte_t* pte = page_walk(current->pagetable, stval, 1);
          *pte = PA2PTE(pa) | PTE_V | prot_to_type(PROT_WRITE | PROT_READ, 1);
        }
        else {
          /*
            this address is not available!
            System is shutting down with exit code -1.
            */
          sprint("this address is not available!\n");
          // panic("System is shutting down with exit code -1.\n");
          shutdown(-1);
        }
        
      }
      break;
    default:
      sprint("unknown page fault.\n");
      break;
  }
}

//
// kernel/smode_trap.S will pass control to smode_trap_handler, when a trap happens
// in S-mode.
//
void smode_trap_handler(void) {
  // make sure we are in User mode before entering the trap handling.
  // we will consider other previous case in lab1_3 (interrupt).
  if ((read_csr(sstatus) & SSTATUS_SPP) != 0) panic("usertrap: not from user mode");

  assert(current);
  // save user process counter.
  current->trapframe->epc = read_csr(sepc);

  // if the cause of trap is syscall from user application.
  // read_csr() and CAUSE_USER_ECALL are macros defined in kernel/riscv.h
  uint64 cause = read_csr(scause);

  // use switch-case instead of if-else, as there are many cases since lab2_3.
  switch (cause) {
    case CAUSE_USER_ECALL:
      handle_syscall(current->trapframe);
      break;
    case CAUSE_MTIMER_S_TRAP:
      handle_mtimer_trap();
      break;
    case CAUSE_STORE_PAGE_FAULT:
    case CAUSE_LOAD_PAGE_FAULT:
      // the address of missing page is stored in stval
      // call handle_user_page_fault to process page faults
      handle_user_page_fault(cause, read_csr(sepc), read_csr(stval));
      break;
    default:
      sprint("smode_trap_handler(): unexpected scause %p\n", read_csr(scause));
      sprint("            sepc=%p stval=%p\n", read_csr(sepc), read_csr(stval));
      panic( "unexpected exception happened.\n" );
      break;
  }

  // continue (come back to) the execution of current process.
  switch_to(current);
}
