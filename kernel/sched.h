#ifndef _SCHED_H_
#define _SCHED_H_

#include "process.h"

//length of a time slice, in number of ticks
#define TIME_SLICE_LEN  2

void insert_to_ready_queue( process* proc );
void schedule();

// ADD
#define SEM_MAX 100
int sem_build(int val);
void P(uint64 sem);
void V(uint64 sem);

#endif
