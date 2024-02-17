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
uint64 g_ufree_page = USER_FREE_ADDRESS_START; // comment:堆的起始虚拟地址

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

// ADD: 注意虚拟地址和物理地址的转换

// 打印链表
static void print_ptr(uint64 head) {
  sprint("lgm:print_ptr:head: %0x\n", head);
  while (head != -1) {
    m_rib* p = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)head));
    sprint("lgm:print_ptr:va: %0x pa: %0x, cap: %0x\n", head, p, p->cap);
    head = p->next;
  }
}

static void insert_into_free(uint64 rib_va) {
  if (current->rib_free == -1) {
    m_rib* rib_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)rib_va));
    rib_pa->next = -1;
    current->rib_free = rib_va;
    return;
  }
  m_rib* rib_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)rib_va));
  uint64 current_va = current->rib_free;
  m_rib* current_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)current->rib_free));
  // 不为空 & 大小小于第一个s
  if (current_pa->cap > rib_pa->cap) {
    rib_pa->next = current->rib_free;
    current->rib_free = rib_va;
    return;
  }
  while (current_pa->next != -1) {
    uint64 next_va = current_pa->next;
    m_rib* next_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)next_va));
    if (next_pa->cap > rib_pa->cap) {
      break;
    }
    current_va = next_va;
    current_pa = next_pa;
  }
  rib_pa->next = current_pa->next;
  current_pa->next = rib_va;
  return;
}

static void insert_into_used(uint64 rib_va) {
  // 头插法即可
  m_rib* rib_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)rib_va));
  rib_pa->next = current->rib_used;
  current->rib_used = rib_va;
}

static void remove_ptr(uint64* head, uint64 rib_va) {
  if (*head == rib_va) {
    *head = ((m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)rib_va)))->next;
    return;
  }
  m_rib* p = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)(*head)));
  while (p->next != rib_va) {
    p = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)p->next));
  }
  p->next = ((m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)rib_va)))->next;
  return;
}

// 计算下一个m_rib的可行地址
static uint64 get_next_rib_addr(uint64 addr) {
  uint64 ret = addr + sizeof(m_rib) - addr % sizeof(m_rib);
  return ret;
}

static void alloc_from_free(uint64 va, uint64 n) {
  uint64 next_rib_addr = get_next_rib_addr(va + sizeof(m_rib) + n);
  m_rib* pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)va));
  if (va + pa->cap > next_rib_addr + sizeof(m_rib)) {
    // 此时需要分割
    uint64 free_rib = next_rib_addr;
    m_rib* free_rib_pa = (m_rib*)user_va_to_pa((pagetable_t)current->pagetable, (void*)free_rib);
    free_rib_pa->cap = va + pa->cap - next_rib_addr;
    pa->cap = next_rib_addr - va;
    remove_ptr(&(current->rib_free), va);
    insert_into_used(va);
    insert_into_free(free_rib);
  }
  else {
    // 不需要分割 直接插入到used中
    remove_ptr(&(current->rib_free), va);
    insert_into_used(va);
  }
}

#define NEED 1
#define NOT_NEED 0
static int is_need_new_page(uint64 n, uint64* va_ptr) { // va为返回的虚拟地址，只有当为false是va有效
  if (current->rib_free == -1)
    return NEED;
  uint64 va = current->rib_free;
  while (va != -1) {
    m_rib* pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)va));
    if (pa->cap >= n + sizeof(m_rib)) { // 需要减去m_rib的大小
      // 设置好返回的va
      *va_ptr = va + sizeof(m_rib); 
      // 进行分配
      alloc_from_free(va, n);
      return NOT_NEED;
    }
    va = pa->next;
  }
  return NEED;
}

static void* alloc_from_new_page(uint64 n) {
  // 计算需要多少页面
  uint64 pages = (n + sizeof(m_rib) + PGSIZE - 1) / PGSIZE;
  uint64 first_page_va = g_ufree_page;
  uint64 last_page_va = g_ufree_page + pages - 1;
  // 分配所有页面
  for (int _ = 1; _ <= pages; _ ++) {
    uint64 pa = (uint64)alloc_page();
    uint64 va = g_ufree_page;
    g_ufree_page += PGSIZE;
    memset((void*)pa, 0, PGSIZE); // 页面置'\0'
    user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, pa,
        prot_to_type(PROT_WRITE | PROT_READ, 1));
  }
  // 操纵物理内存
  uint64 last_page_used = (n + sizeof(m_rib)) % PGSIZE; // 最后一页的使用量
  uint64 next_rib_addr = get_next_rib_addr(last_page_va + last_page_used);
  if (next_rib_addr + sizeof(m_rib) >= last_page_va + PGSIZE) {
    // 本页不能再or恰好只能存储一个rib时 不需要分割
    m_rib* use_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)first_page_va));
    use_pa->cap = pages * PGSIZE;
    insert_into_used(first_page_va);
  }
  else {
    // 需要分割
    m_rib* use_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)first_page_va));
    use_pa->cap = next_rib_addr - first_page_va;
    insert_into_used(first_page_va);
    m_rib* free_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)next_rib_addr));
    free_pa->cap = last_page_va + PGSIZE - next_rib_addr;
    insert_into_free(next_rib_addr);
  }
  // 返回虚拟地址
  return (void*)(first_page_va + sizeof(m_rib));
}

void* better_alloc(uint64 n) {
  uint64 va;
  if (is_need_new_page(n, &va) == NEED) {
    // sprint("lgm:need new page\n");
    return alloc_from_new_page(n);
  }
  else {
    // sprint("lgm:don't need new page\n");
    return (void*)va;
  }
}

void better_free(uint64 va) {
  // 暂且不考虑释放整个页面 + 合并空闲块
  uint64 p = va - sizeof(m_rib); // 得到m_rib的地址
  remove_ptr(&(current->rib_used), p);
  insert_into_free(p);
  return;
}

static void merge(uint64 va) {
  m_rib* pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)va));
  uint64 free_ptr = current->rib_free;
  while (free_ptr != -1) {
    m_rib* free_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)free_ptr));
    if (free_ptr + free_pa->cap == va) {
      // 合并
      free_pa->cap += pa->cap;
      remove_ptr(&(current->rib_free), va);
      // 修改pa & va
      pa = free_pa;
      va = free_ptr;
      return;
    }
    if (va + pa->cap == free_ptr) {
      // 合并
      pa->cap += free_pa->cap;
      remove_ptr(&(current->rib_free), free_ptr);
      return;
    }
    free_ptr = free_pa->next;
  }
}

static uint64 free_cnt = 0;
static uint64 free_freq = 5;

static void free_entry(uint64 va) {
    m_rib* pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)va));
    uint64 st = va / PGSIZE;
    uint64 ed = (va + pa->cap) / PGSIZE;
    if (st == ed) { // 同一页中
      if (pa->cap == PGSIZE) { // 恰好为一页
        remove_ptr(&(current->rib_free), va);
        user_vm_unmap((pagetable_t)current->pagetable, va, pa->cap, 1);
        free_page((void*)pa);
      }
    }
    else if (st + 1 == ed) { // 两页
      if (va % PGSIZE == 0 && (va + pa->cap + 1) % PGSIZE == 0) {
        // 可以释放st + ed页
        remove_ptr(&(current->rib_free), va);
        free_page((void*)pa);
        free_page((void*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)(va + PGSIZE))));
        user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
        user_vm_unmap((pagetable_t)current->pagetable, va + PGSIZE, PGSIZE, 1);
      }
      else if (va % PGSIZE == 0 && (va + pa->cap + 1) % PGSIZE != 0) {
        // 可以释放st页
        remove_ptr(&(current->rib_free), va);
        free_page((void*)pa);
        user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
        uint64 new_free = ed * PGSIZE;
        m_rib* new_free_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)new_free));
        new_free_pa->cap = va + pa->cap - new_free;
        insert_into_free(new_free);
      }
      else if (va % PGSIZE != 0 && (va + pa->cap + 1) % PGSIZE == 0) {
        // 可以释放ed页
        pa->cap -= PGSIZE;
        free_page((void*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)(va + PGSIZE))));
        user_vm_unmap((pagetable_t)current->pagetable, va + PGSIZE, PGSIZE, 1);        
      }
      else; // 不能释放
    }
    else { // 存在中间页 -> 一定需要remove
      remove_ptr(&(current->rib_free), va); // 直接remove
      for (uint64 i = st + 1; i < ed; i++) {
        uint64 umap_pa = (uint64)user_va_to_pa((pagetable_t)current->pagetable, (void*)(i * PGSIZE));
        free_page((void*)umap_pa);
        user_vm_unmap((pagetable_t)current->pagetable, i * PGSIZE, PGSIZE, 1);
      }
      if (va % PGSIZE == 0) { // 释放st页
        free_page((void*)pa);
        user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
      }
      else { // 增设st页的空闲块
        pa->cap = PGSIZE - va % PGSIZE;
        insert_into_free(va);
      }
      if (va + pa->cap + 1 % PGSIZE == 0) { // 释放ed页
        free_page((void*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)(va + pa->cap))));
        user_vm_unmap((pagetable_t)current->pagetable, va + pa->cap, PGSIZE, 1);
      }
      else { // 增设ed页的空闲块
        uint64 new_free = ed * PGSIZE;
        m_rib* new_free_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)new_free));
        new_free_pa->cap = va + pa->cap - new_free;
        insert_into_free(new_free);
      }
    }
}

static void free_pages() {
  uint64 free_ptr = current->rib_free;
  while (free_ptr != -1) {
    m_rib* free_pa = (m_rib*)(user_va_to_pa((pagetable_t)current->pagetable, (void*)free_ptr));
    uint64 next_ptr = free_pa->next; // 存储
    free_entry(free_ptr);
    free_ptr = free_pa->next;
  }
  return;
}

void best_free(uint64 va) {
  better_free(va);
  merge(va - sizeof(m_rib)); // 合并
  free_cnt++;
  if (free_cnt % free_freq == 0)
    free_pages(); // 释放页面
  return;
}