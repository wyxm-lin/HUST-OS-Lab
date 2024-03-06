#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "config.h"
#include "riscv.h"
#include "process.h"

typedef struct m_rib
{
    uint64 cap;
    uint64 next;
} m_rib;

void print_ptr(uint64 head);
void cow(process* parent, process* child);
void insert_into_free(uint64 rib_va);
void insert_into_used(uint64 rib_va);
void remove_ptr(uint64 *head, uint64 rib_va);

uint64 get_next_rib_addr(uint64 addr);

void alloc_from_free(uint64 va, uint64 n);
void alloc_from_free_and_g_ufree_page(uint64 va, uint64 n);
uint64 alloc_from_g_ufree_page(uint64 n);

MyStatus is_need_st_is_g_ufree_page(uint64 n, uint64 *va_ptr);
uint64 better_alloc(uint64 n);

void merge(uint64 va);
void free_entry(uint64 va);
void real_free();
uint64 better_free(uint64 va);

#endif