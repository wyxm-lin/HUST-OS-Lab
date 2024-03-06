#ifndef _PMM_H_
#define _PMM_H_

#include "util/types.h"
#include "riscv.h"

// Initialize phisical memeory manager
void pmm_init();
// Allocate a free phisical page
void* alloc_page();
// Free an allocated page
void free_page(void* pa);

void ref_insert(uint64 pa);

#endif