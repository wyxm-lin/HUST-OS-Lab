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
#include "vmm.h"
#include "pmm.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"

//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe *, uint64 satp);

// current points to the currently running user-mode application.
process* current = NULL;

// points to the first free page in our simple heap. added @lab2_2
uint64 g_ufree_page = USER_FREE_ADDRESS_START;

//
// switch to a user-mode process
//
void switch_to(process* proc) {
  assert(proc);
  current = proc;

  // write the smode_trap_vector (64-bit func. address) defined in kernel/strap_vector.S
  // to the stvec privilege register, such that trap handler pointed by smode_trap_vector
  // will be triggered when an interrupt occurs in S mode.
  write_csr(stvec, (uint64)smode_trap_vector);

  // set up trapframe values (in process structure) that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;      // process's kernel stack
  proc->trapframe->kernel_satp = read_csr(satp);  // kernel page table
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // SSTATUS_SPP and SSTATUS_SPIE are defined in kernel/riscv.h
  // set S Previous Privilege mode (the SSTATUS_SPP bit in sstatus register) to User mode.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  // write x back to 'sstatus' register to enable interrupts, and sret destination mode.
  write_csr(sstatus, x);

  // set S Exception Program Counter (sepc register) to the elf entry pc.
  write_csr(sepc, proc->trapframe->epc);

  // make user page table. macro MAKE_SATP is defined in kernel/riscv.h. added @lab2_1
  uint64 user_satp = MAKE_SATP(proc->pagetable);

  // return_to_user() is defined in kernel/strap_vector.S. switch to user mode with sret.
  // note, return_to_user takes two parameters @ and after lab2_1.
  return_to_user(proc->trapframe, user_satp);
}

// ADD
static void insert_to_memory_rib(m_rib* rib) {
  if (current->memory_rib == NULL) {
    current->memory_rib = rib;
    return;
  }
  if (current->memory_rib->size > rib->size) {
    rib->next = current->memory_rib;
    current->memory_rib = rib;
    return;
  }
  m_rib* p = current->memory_rib;
  while (p->next != NULL) {
    if (p->next->size > rib->size) {
      break;
    }
    p = p->next;
  }
  rib->next = p->next;
  p->next = rib;
  return;
}

static void* alloc_from_new_page(uint64 n) { // 返回虚拟地址
  void* pa = alloc_page();
  void* va = g_ufree_page;
  g_ufree_page += PGSIZE;
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
       prot_to_type(PROT_WRITE | PROT_READ, 1));
  if (n < PGSIZE) {
    m_rib* rib = (m_rib*)(pa + n);
    rib->va = (uint64)(va + n);
    rib->next = NULL;
    rib->size = PGSIZE - n;
    insert_to_memory_rib(rib);
  }
  else if (n == PGSIZE);
  else {
    while (1) {
      n -= PGSIZE;
      void* pa = alloc_page();
      void* va = g_ufree_page;
      g_ufree_page += PGSIZE;
      user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
           prot_to_type(PROT_WRITE | PROT_READ, 1));
      if (n < PGSIZE) {
        m_rib* rib = (m_rib*)(pa + n);
        rib->va = (uint64)(va + n); // 其实就是本身
        rib->next = NULL;
        rib->size = PGSIZE - n;
        insert_to_memory_rib(rib);
        break;
      }
      else if (n == PGSIZE)
        break;
    }
  }
  return va;
}

static void remove_ptr(m_rib* p) {
  if (current->memory_rib == p) {
    current->memory_rib = p->next;
    return;
  }
  m_rib* q = current->memory_rib;
  while (q->next != p) {
    q = q->next;
  }
  q->next = p->next;
  return;
}

#define NEED 1
#define NOT_NEED 0
static int is_need_new_page(uint64 n, uint64* va_ptr) { // va为返回的虚拟地址，只有当为false是va有效
  if (current->memory_rib == NULL)
    return NEED;
  m_rib* p = current->memory_rib;
  while (p != NULL) {
    if (p->size >= n) {
      // 设置好返回的va
      *va_ptr = p->va; 
      // 对该块进行分割
      p->size -= n;
      p->va += n;
      // 将该块从链表中删除
      remove_ptr(p);
      // 将该块插入到链表中
      if (p->size != 0) // 是0的话说明刚好分配完，正在使用
        insert_to_memory_rib(p);
      return NOT_NEED;
    }
    p = p->next;
  }
  return NEED;
}

void* better_alloc(uint64 n) {
  uint64 va;
  if (is_need_new_page(n, &va) == NEED) {
    return alloc_from_new_page(n);
  }
  else 
    return (void*)va;
}

void better_free(void* va) {
  
}