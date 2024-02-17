/*
 * implementing the scheduler
 */

#include "sched.h"
#include "spike_interface/spike_utils.h"
#include "spike_interface/atomic.h"

process* ready_queue_head = NULL;

//
// insert a process, proc, into the END of ready queue.
//
void insert_to_ready_queue( process* proc ) {
  sprint( "going to insert process %d to ready queue.\n", proc->pid );
  // sprint("lgm:proc is %p\n", proc);
  // if the queue is empty in the beginning
  if( ready_queue_head == NULL ){
    proc->status = READY;
    proc->queue_next = NULL;
    ready_queue_head = proc;
    return;
  }

  // ready queue is not empty
  process *p;
  // browse the ready queue to see if proc is already in-queue
  for( p=ready_queue_head; p->queue_next!=NULL; p=p->queue_next )
    if( p == proc ) return;  //already in queue

  // p points to the last element of the ready queue
  if( p==proc ) return;
  p->queue_next = proc;
  proc->status = READY;
  proc->queue_next = NULL;

  return;
}

//
// choose a proc from the ready queue, and put it to run.
// note: schedule() does not take care of previous current process. If the current
// process is still runnable, you should place it into the ready queue (by calling
// ready_queue_insert), and then call schedule().
//
extern process procs[NPROC];
void schedule() {
  if ( !ready_queue_head ){
    // by default, if there are no ready process, and all processes are in the status of
    // FREE and ZOMBIE, we should shutdown the emulated RISC-V machine.
    int should_shutdown = 1;

    for( int i = 0; i < NPROC; i++ )
      if( (procs[i].status != FREE) && (procs[i].status != ZOMBIE) ){
        should_shutdown = 0;
        sprint( "ready queue empty, but process %d is not in free/zombie state:%d\n", 
          i, procs[i].status );
      }

    if( should_shutdown ){
      sprint( "no more ready processes, system shutdown now.\n" );
      shutdown( 0 );
    }else{
      panic( "Not handled: we should let system wait for unfinished processes.\n" );
    }
  }

  current = ready_queue_head;
  // sprint("lgm:current is %d\n", current->pid);
  assert( current->status == READY );
  ready_queue_head = ready_queue_head->queue_next;

  current->status = RUNNING;
  sprint( "going to schedule process %d to run.\n", current->pid );
  switch_to( current );
}

// ADD
int sems[SEM_MAX];
int cnt = -1;
process* waiting_queue_head[SEM_MAX];
spinlock_t sem_lock[SEM_MAX];

static void insert_into_waiting_queue(process* proc, int sem) {
  // sprint( "going to insert process %d to waiting queue.\n", proc->pid );
  // if the queue is empty in the beginning
  if( waiting_queue_head[sem] == NULL ){
    proc->status = BLOCKED; // 
    proc->queue_next = NULL;
    waiting_queue_head[sem] = proc;
    return;
  }

  // ready queue is not empty
  process *p;
  // browse the ready queue to see if proc is already in-queue
  for( p = waiting_queue_head[sem]; p->queue_next != NULL; p = p->queue_next )
    if( p == proc ) return;  //already in queue

  // p points to the last element of the ready queue
  if( p == proc ) return;
  p->queue_next = proc;
  proc->status = BLOCKED;
  proc->queue_next = NULL;

  return;
}

int sem_build(int val) {
  sems[++cnt] = val;
  return cnt;
}

static void print_ready() {
  process* p = ready_queue_head;
  while (p) {
    sprint("print_ready:pid is %d\n", p->pid);
    p = p->queue_next;
  }
}

void P(uint64 sem) {
  // sprint("P start, pid is %d, sem is %d\n", current->pid, sem);
  spinlock_lock(&sem_lock[sem]);
  sems[sem]--;
  if (sems[sem] < 0) {
    spinlock_unlock(&sem_lock[sem]);
    // print_ready();
    insert_into_waiting_queue(current, sem);
    // print_ready();
    schedule();
  }
  spinlock_unlock(&sem_lock[sem]);
} 

void V(uint64 sem) {
  // sprint("V start, pid is %d, sem is %d\n", current->pid, sem);
  spinlock_lock(&sem_lock[sem]);
  sems[sem]++;
  if (sems[sem] <= 0) {
    spinlock_unlock(&sem_lock[sem]);
    process* p = waiting_queue_head[sem];
    waiting_queue_head[sem] = waiting_queue_head[sem]->queue_next;
    p->status = READY;
    insert_to_ready_queue(p);
  }
  spinlock_unlock(&sem_lock[sem]);
}

