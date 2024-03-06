#ifndef _SCHED_H_
#define _SCHED_H_

#include "process.h"

//length of a time slice, in number of ticks
#define TIME_SLICE_LEN  2

void insert_to_ready_queue( process* proc );
void schedule();

#define SEM_MAX 128
int sem_new(int value);
int sem_P(uint64 sem);
int sem_V(uint64 sem);

#endif
