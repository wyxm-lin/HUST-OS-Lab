#ifndef _CORE_H_
#define _CORE_H_

#include "config.h"
#include "riscv.h"

enum core_status
{
    CORE_STATUS_IDLE,
    CORE_STATUS_BUSY,
    CORE_STATUS_ERROR
};

typedef struct CoreInfo_t {
    int status;
    uint64 TaskRunCount; // FIXME 暂且以完成的任务数作为负载指标
}CoreInfo_t;

void core_info_init();
uint64 choose_core();
void set_idle(uint64 hartid);
void set_busy(uint64 hartid);
MyStatus is_all_idle();
void idle_process(uint64 hartid);
enum core_status get_core_status(uint64 hartid);

#endif 
