#include "memory.h"
#include "vmm.h"
#include "pmm.h"
#include "util/string.h"
#include "spike_interface/spike_utils.h"
#include "process.h"

extern process *current[NCPU];

void print_ptr(uint64 head)
{
    uint64 hartid = read_tp();
    sprint("lgm:print_ptr:head: %0x\n", head);
    while (head != -1)
    {
        m_rib *p = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)head));
        sprint("lgm:print_ptr:va: %0x pa: %0x, cap: %0x\n", head, p, p->cap);
        head = p->next;
    }
}

void cow(process *parent, process *child)
{
    uint64 free_va = parent->user_heap.rib_free;
    while (free_va != -1)
    {
        uint64 free_pa = (uint64)(user_va_to_pa((pagetable_t)parent->pagetable, (void *)free_va));
        cow_vm_map((pagetable_t)child->pagetable, free_va, free_pa);
        free_va = ((m_rib *)free_pa)->next;
    }
    uint64 used_va = parent->user_heap.rib_used;
    while (used_va != -1)
    {
        uint64 used_pa = (uint64)(user_va_to_pa((pagetable_t)parent->pagetable, (void *)used_va));
        cow_vm_map((pagetable_t)child->pagetable, used_va, used_pa);
        used_va = ((m_rib *)used_pa)->next;
    }
}

void insert_into_free(uint64 rib_va)
{
    uint64 hartid = read_tp();
    if (current[hartid]->user_heap.rib_free == -1)
    {
        m_rib *rib_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)rib_va));
        rib_pa->next = -1;
        current[hartid]->user_heap.rib_free = rib_va;
        return;
    }
    m_rib *rib_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)rib_va));
    uint64 current_va = current[hartid]->user_heap.rib_free;
    m_rib *current_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)current[hartid]->user_heap.rib_free));
    // 不为空 & 大小小于第一个s
    if (current_pa->cap > rib_pa->cap)
    {
        rib_pa->next = current[hartid]->user_heap.rib_free;
        current[hartid]->user_heap.rib_free = rib_va;
        return;
    }
    while (current_pa->next != -1)
    {
        uint64 next_va = current_pa->next;
        m_rib *next_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)next_va));
        if (next_pa->cap > rib_pa->cap)
        {
            break;
        }
        current_va = next_va;
        current_pa = next_pa;
    }
    rib_pa->next = current_pa->next;
    current_pa->next = rib_va;
    return;
}

void insert_into_used(uint64 rib_va)
{
    uint64 hartid = read_tp();
    m_rib *rib_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)rib_va));
    rib_pa->next = current[hartid]->user_heap.rib_used;
    current[hartid]->user_heap.rib_used = rib_va;
}

void remove_ptr(uint64 *head, uint64 rib_va)
{
    uint64 hartid = read_tp();
    if (*head == rib_va)
    {
        *head = ((m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)rib_va)))->next;
        return;
    }
    m_rib *p = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)(*head)));
    while (p->next != rib_va)
    {
        p = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)p->next));
    }
    p->next = ((m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)rib_va)))->next;
    return;
}

uint64 get_next_rib_addr(uint64 addr)
{
    uint64 ret = addr + sizeof(m_rib) - addr % sizeof(m_rib);
    return ret;
}

void alloc_from_free(uint64 va, uint64 n)
{
    uint64 hartid = read_tp();
    uint64 next_rib_addr = get_next_rib_addr(va + sizeof(m_rib) + n);
    m_rib *pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)va));
    if (va + pa->cap > next_rib_addr + sizeof(m_rib))
    {
        // 此时需要分割
        uint64 free_rib = next_rib_addr;
        m_rib *free_rib_pa = (m_rib *)user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)free_rib);
        free_rib_pa->cap = va + pa->cap - next_rib_addr;
        pa->cap = next_rib_addr - va;
        remove_ptr(&(current[hartid]->user_heap.rib_free), va);
        insert_into_used(va);
        insert_into_free(free_rib);
    }
    else
    {
        // 不需要分割 直接插入到used中
        remove_ptr(&(current[hartid]->user_heap.rib_free), va);
        insert_into_used(va);
    }
}

void alloc_from_free_and_g_ufree_page(uint64 va, uint64 n)
{
    uint64 hartid = read_tp();

    remove_ptr(&(current[hartid]->user_heap.rib_free), va);
    m_rib *pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)va));
    uint64 remain = n - (pa->cap - sizeof(m_rib));
    uint64 new_pages = remain / PGSIZE + (remain % PGSIZE != 0);

    // 分配这些页
    for (int _ = 1; _ <= new_pages; _++)
    {
        uint64 new_page_pa = (uint64)alloc_page();
        uint64 new_page_va = current[hartid]->user_heap.g_ufree_page;
        current[hartid]->user_heap.g_ufree_page += PGSIZE;
        memset((void *)new_page_pa, 0, PGSIZE);
        user_vm_map((pagetable_t)current[hartid]->pagetable, new_page_va, PGSIZE, new_page_pa, prot_to_type(PROT_WRITE | PROT_READ, 1));
    }
    uint64 last_page_use = remain % PGSIZE;
    if (last_page_use == 0)
    {
        // do nothing
    }
    else
    {
        uint64 next_rib_addr = get_next_rib_addr(current[hartid]->user_heap.g_ufree_page - PGSIZE + last_page_use);
        if (PGSIZE - next_rib_addr > sizeof(m_rib))
        {
            m_rib *free_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)next_rib_addr));
            free_pa->next = 0;
            free_pa->cap = PGSIZE - next_rib_addr % PGSIZE;
            insert_into_free(next_rib_addr);
            pa->cap = next_rib_addr - va;
        }
        else
        {
            pa->cap += new_pages * PGSIZE;
        }
    }
    // 插入used
    insert_into_used(va);
}

uint64 alloc_from_g_ufree_page(uint64 n)
{
    uint64 hartid = read_tp();

    // 计算需要多少页面
    uint64 pages = (n + sizeof(m_rib) + PGSIZE - 1) / PGSIZE;

    uint64 first_page_va = current[hartid]->user_heap.g_ufree_page;
    uint64 last_page_va = current[hartid]->user_heap.g_ufree_page + pages - 1;
    // 分配所有页面
    for (int _ = 1; _ <= pages; _++)
    {
        uint64 pa = (uint64)alloc_page();
        uint64 va = current[hartid]->user_heap.g_ufree_page;
        current[hartid]->user_heap.g_ufree_page += PGSIZE;
        memset((void *)pa, 0, PGSIZE); // 页面置'\0'
        user_vm_map((pagetable_t)current[hartid]->pagetable, va, PGSIZE, pa,
                    prot_to_type(PROT_WRITE | PROT_READ, 1));
    }
    // 操纵物理内存
    uint64 last_page_used = (n + sizeof(m_rib)) % PGSIZE; // 最后一页的使用量
    uint64 next_rib_addr = get_next_rib_addr(last_page_va + last_page_used);
    if (next_rib_addr + sizeof(m_rib) >= last_page_va + PGSIZE)
    {
        // 本页不能再or恰好只能存储一个rib时 不需要分割
        m_rib *use_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)first_page_va));
        use_pa->cap = pages * PGSIZE;
        insert_into_used(first_page_va);
    }
    else
    {
        // 需要分割
        m_rib *use_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)first_page_va));
        use_pa->cap = next_rib_addr - first_page_va;
        insert_into_used(first_page_va);
        m_rib *free_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)next_rib_addr));
        free_pa->cap = last_page_va + PGSIZE - next_rib_addr;
        insert_into_free(next_rib_addr);
    }
    // 返回虚拟地址
    return (uint64)(first_page_va + sizeof(m_rib));
}

MyStatus is_need_st_is_g_ufree_page(uint64 n, uint64 *va_ptr)
{
    uint64 hartid = read_tp();

    // va为返回的虚拟地址，只有当为false是va有效
    if (current[hartid]->user_heap.rib_free == -1)
        return Yes;
    uint64 va = current[hartid]->user_heap.rib_free;
    uint64 max_va = va; // 记录最大的va
    while (va != -1)
    {
        max_va = va > max_va ? va : max_va;
        m_rib *pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)va));
        if (pa->cap >= n + sizeof(m_rib))
        {
            *va_ptr = va + sizeof(m_rib);
            alloc_from_free(va, n);
            return No;
        }
        va = pa->next;
    }
    m_rib *max_pa = (m_rib *)user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)max_va);
    if (max_va + max_pa->cap == current[hartid]->user_heap.g_ufree_page)
    {
        *va_ptr = max_va + sizeof(m_rib);
        alloc_from_free_and_g_ufree_page(max_va, n);
        return No;
    }
    else
    {
        panic("Invalid\n");
    }
    return Yes;
}

uint64 better_alloc(uint64 n)
{
    uint64 va;
    if (is_need_st_is_g_ufree_page(n, &va) == Yes)
    {
        return alloc_from_g_ufree_page(n); // NOTE:第三种分配方式
    }
    else
    {
        return va;
    }
}

void merge(uint64 va)
{
    uint64 hartid = read_tp();
    m_rib *pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)va));
    uint64 free_ptr = current[hartid]->user_heap.rib_free;
    while (free_ptr != -1)
    {
        m_rib *free_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)free_ptr));
        if (free_ptr + free_pa->cap == va)
        {
            free_pa->cap += pa->cap;
            remove_ptr(&(current[hartid]->user_heap.rib_free), va);
            pa = free_pa;
            va = free_ptr;
            return;
        }
        if (va + pa->cap == free_ptr)
        {
            pa->cap += free_pa->cap;
            remove_ptr(&(current[hartid]->user_heap.rib_free), free_ptr);
            return;
        }
        free_ptr = free_pa->next;
    }
}

static uint64 free_cnt = 0;
static uint64 free_freq = 5;

void free_entry(uint64 va)
{
    uint64 hartid = read_tp();

    m_rib *pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)va));
    uint64 st = va / PGSIZE;
    uint64 ed = (va + pa->cap) / PGSIZE;
    if (st == ed)
    { // 同一页中
        if (pa->cap == PGSIZE)
        { // 恰好为一页
            remove_ptr(&(current[hartid]->user_heap.rib_free), va);
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, va, pa->cap, 1);
            free_page((void *)pa);
        }
    }
    else if (st + 1 == ed)
    { // 两页
        if (va % PGSIZE == 0 && (va + pa->cap + 1) % PGSIZE == 0)
        {
            // 可以释放st + ed页
            remove_ptr(&(current[hartid]->user_heap.rib_free), va);
            free_page((void *)pa);
            free_page((void *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)(va + PGSIZE))));
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, va, PGSIZE, 1);
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, va + PGSIZE, PGSIZE, 1);
        }
        else if (va % PGSIZE == 0 && (va + pa->cap + 1) % PGSIZE != 0)
        {
            // 可以释放st页
            remove_ptr(&(current[hartid]->user_heap.rib_free), va);
            free_page((void *)pa);
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, va, PGSIZE, 1);
            uint64 new_free = ed * PGSIZE;
            m_rib *new_free_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)new_free));
            new_free_pa->cap = va + pa->cap - new_free;
            insert_into_free(new_free);
        }
        else if (va % PGSIZE != 0 && (va + pa->cap + 1) % PGSIZE == 0)
        {
            // 可以释放ed页
            pa->cap -= PGSIZE;
            free_page((void *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)(va + PGSIZE))));
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, va + PGSIZE, PGSIZE, 1);
        }
        else
            ; // 不能释放
    }
    else
    {                                                           // 存在中间页 -> 一定需要remove
        remove_ptr(&(current[hartid]->user_heap.rib_free), va); // 直接remove
        for (uint64 i = st + 1; i < ed; i++)
        {
            uint64 umap_pa = (uint64)user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)(i * PGSIZE));
            free_page((void *)umap_pa);
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, i * PGSIZE, PGSIZE, 1);
        }
        if (va % PGSIZE == 0)
        { // 释放st页
            free_page((void *)pa);
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, va, PGSIZE, 1);
        }
        else
        { // 增设st页的空闲块
            pa->cap = PGSIZE - va % PGSIZE;
            insert_into_free(va);
        }
        if (va + pa->cap + 1 % PGSIZE == 0)
        { // 释放ed页
            free_page((void *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)(va + pa->cap))));
            user_vm_unmap((pagetable_t)current[hartid]->pagetable, va + pa->cap, PGSIZE, 1);
        }
        else
        { // 增设ed页的空闲块
            uint64 new_free = ed * PGSIZE;
            m_rib *new_free_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)new_free));
            new_free_pa->cap = va + pa->cap - new_free;
            insert_into_free(new_free);
        }
    }
}

void real_free()
{
    uint64 hartid = read_tp();

    uint64 free_ptr = current[hartid]->user_heap.rib_free;
    while (free_ptr != -1)
    {
        m_rib *free_pa = (m_rib *)(user_va_to_pa((pagetable_t)current[hartid]->pagetable, (void *)free_ptr));
        uint64 next_ptr = free_pa->next; // 存储
        free_entry(free_ptr);
        free_ptr = free_pa->next;
    }
    return;
}

uint64 better_free(uint64 va)
{
    uint64 hartid = read_tp();
    uint64 p = va - sizeof(m_rib);
    remove_ptr(&(current[hartid]->user_heap.rib_used), p);
    insert_into_free(p);
    merge(va - sizeof(m_rib)); // 合并
    free_cnt++;
    if (free_cnt % free_freq == 0)
        real_free(); // 释放页面
    return 0;
}
