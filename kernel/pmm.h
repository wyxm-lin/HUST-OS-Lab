#ifndef _PMM_H_
#define _PMM_H_

#include "riscv.h"
#include "config.h"
#include "spike_interface/atomic.h"

// Initialize phisical memeory manager
void pmm_init();
// Allocate a free phisical page
void *alloc_page();
// Free an allocated page
void free_page(void *pa);

// added@lab3_challenge3
typedef struct MemoryRefCount {
    uint64 pa;
    uint64 ref;
}MemoryRefCount;
#define MemoryRefCountMAX 0x4000

enum MyStatus ref_find(uint64 pa);
void ref_insert(uint64 pa);
enum MyStatus ref_erase(uint64 pa);

#endif