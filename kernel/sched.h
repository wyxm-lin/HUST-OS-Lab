#ifndef _SCHED_H_
#define _SCHED_H_

#include "process.h"

//length of a time slice, in number of ticks
#define TIME_SLICE_LEN  2

void insert_to_ready_queue( process* proc );
void schedule();

// added@lab3_challenge2
#define SEM_MAX 128
uint64 sem_build(int val);
int P(uint64 sem);
int V(uint64 sem);

#endif
