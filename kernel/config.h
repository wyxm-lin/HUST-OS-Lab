#ifndef _CONFIG_H_
#define _CONFIG_H_

// we use only one HART (cpu) in fundamental experiments
#define SINGLE_CORE 1
#define DOUBLE_CORE 2
#define NCPU DOUBLE_CORE
#define CTL_CORE 0

// interval of timer interrupt. added @lab1_3
#define TIMER_INTERVAL 1000000

// the maximum memory space that PKE is allowed to manage. added @lab2_1
#define PKE_MAX_ALLOWABLE_RAM 128 * 1024 * 1024

// the ending physical address that PKE observes. added @lab2_1
#define PHYS_TOP (DRAM_BASE + PKE_MAX_ALLOWABLE_RAM)

#define AUTHOR "wyxm"

typedef enum MyStatus
{
    No,
    Yes
} MyStatus;

#endif
