/*
 * implementing the scheduler
 */

#include "sched.h"
#include "spike_interface/spike_utils.h"

process* ready_queue_head = NULL;

//
// insert a process, proc, into the END of ready queue.
//
void insert_to_ready_queue( process* proc ) {
  sprint( "going to insert process %d to ready queue.\n", proc->pid );
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
  for( p = ready_queue_head; p->queue_next != NULL; p = p->queue_next )
    if( p == proc ) return;  //already in queue

  // p points to the last element of the ready queue
  if( p == proc ) return;
  p->queue_next = proc; // ANNOTATE: 此时p已经指向队列的尾部了
  proc->status = READY;
  proc->queue_next = NULL;

  return;
}

void insert_to_ready_queue_for_wait(process * proc) {
    sprint( "going to insert process %d to ready queue.\n", proc->pid );
  // if the queue is empty in the beginning
  proc->wait_pid = 0; // 清除效果
  if( ready_queue_head == NULL ){
    proc->status = READY;
    proc->queue_next = NULL;
    ready_queue_head = proc;
    return;
  }
  proc->status = READY;
  proc->queue_next = ready_queue_head;
  ready_queue_head = proc;
}



//
// choose a proc from the ready queue, and put it to run.
// note: schedule() does not take care of previous current process. If the current
// process is still runnable, you should place it into the ready queue (by calling
// ready_queue_insert), and then call schedule().
//
extern process procs[NPROC];

void schedule() {
  // sprint("lgm:going to schedule\n");
  // // 首先判断其父进程是否处于阻塞状态(NOTE: 需要判断其有父进程 && 甚至需要判断current是否存在 && 并且current进程不为BLOCKED中(区别于wait函数里面的修改))
  // if (current != NULL && current->status != BLOCKED && current->parent != NULL && current->parent->status == BLOCKED) {
  //   if (current->parent->wait_pid == -1) {
  //     // 父进程等待任意一个子线程
  //     current->parent->status = RUNNING;
  //     current->parent->wait_pid = 0; // 清除效果
  //     current = current->parent;
  //     sprint( "going to insert process %d to ready queue.\n", current->pid );
  //     sprint( "going to schedule process %d to run.\n", current->pid );
  //     // sprint("lgm:here return\n");
  //     return; // 直接返回
  //   }
  //   else if (current->parent->wait_pid == current->pid) {
  //     // 父进程等待当前进程
  //     current->parent->status = RUNNING;
  //     current->parent->wait_pid = 0; // 清除效果
  //     current = current->parent;
  //     sprint( "going to insert process %d to ready queue.\n", current->pid );
  //     sprint( "going to schedule process %d to run.\n", current->pid );
  //     return; // 直接返回
  //   }
  // }
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

    if ( should_shutdown ){
      sprint( "no more ready processes, system shutdown now.\n" );
      shutdown( 0 );
    }
    else {
      panic( "Not handled: we should let system wait for unfinished processes.\n" );
    }
  }

  current = ready_queue_head;
  assert( current->status == READY );
  ready_queue_head = ready_queue_head->queue_next;

  current->status = RUNNING;
  sprint( "going to schedule process %d to run.\n", current->pid );
  switch_to( current );
}
