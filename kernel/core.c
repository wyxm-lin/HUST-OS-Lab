#include "core.h"
#include "spike_interface/atomic.h"
#include "spike_interface/spike_utils.h"
#include "process.h"

CoreInfo_t CoreInfo[NCPU];
spinlock_t core_info_lock;

void core_info_init() {
    spinlock_lock(&core_info_lock);
    for (int i = 0; i < NCPU; i++) {
        CoreInfo[i].TaskRunCount = 0;
        CoreInfo[i].status = CORE_STATUS_IDLE;
    }
    spinlock_unlock(&core_info_lock);
}

uint64 choose_core() {
    spinlock_lock(&core_info_lock);
    uint64 MIN = 0x7fffffffffffffff;
    uint64 MIN_CORE = -1;
    for (int i = 0; i < NCPU; i++) {
        // sprint("the count is  %d %lld\n", i, CoreInfo[i].TaskRunCount);
        if ( CoreInfo[i].status == CORE_STATUS_IDLE && CoreInfo[i].TaskRunCount < MIN) {
            MIN = CoreInfo[i].TaskRunCount;
            MIN_CORE = i;
        }
    }
    if (MIN_CORE == -1) {
        spinlock_unlock(&core_info_lock);
        return -1;
    }
    CoreInfo[MIN_CORE].TaskRunCount++;
    spinlock_unlock(&core_info_lock);
    // sprint("the min_core is %lld\n", MIN_CORE);
    return MIN_CORE;
}

void set_idle(uint64 hartid) {
    spinlock_lock(&core_info_lock);
    CoreInfo[hartid].status = CORE_STATUS_IDLE;
    spinlock_unlock(&core_info_lock);
}

void set_busy(uint64 hartid) {
    spinlock_lock(&core_info_lock);
    CoreInfo[hartid].status = CORE_STATUS_BUSY;
    spinlock_unlock(&core_info_lock);
}

MyStatus is_all_idle() {
    spinlock_lock(&core_info_lock);
    for (int i = 0; i < NCPU; i++) {
        if (CoreInfo[i].status != CORE_STATUS_IDLE) {
            spinlock_unlock(&core_info_lock);
            return No;
        }
    }
    spinlock_unlock(&core_info_lock);
    return Yes;
}

enum core_status get_core_status(uint64 hartid) {
    spinlock_lock(&core_info_lock);
    int status = CoreInfo[hartid].status;
    spinlock_unlock(&core_info_lock);
    return status;
}

extern process *current[NCPU];
void idle_process(uint64 hartid) {
    while (1) {
        asm volatile("wfi");
        if (get_core_status(hartid) == CORE_STATUS_BUSY) {
            // sprint("                                        raise\n");
            break;
        }
    }
    switch_to(current[hartid]);
}
