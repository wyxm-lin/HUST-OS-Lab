/*
 * implementing the scheduler
 */

#include "sched.h"
#include "spike_interface/spike_utils.h"
#include "sync_utils.h"

process *ready_queue_head[NCPU] = { NULL };

//
// insert a process, proc, into the END of ready queue.
//
void insert_to_ready_queue(process *proc)
{
	uint64 hartid = read_tp();

	if (proc == NULL) {
		return;
	}
	sprint("going to insert process %d to ready queue.\n", proc->pid);
	// if the queue is empty in the beginning
	if (ready_queue_head[hartid] == NULL)
	{
		proc->status = READY;
		proc->queue_next = NULL;
		ready_queue_head[hartid] = proc;
		return;
	}

	// ready queue is not empty
	process *p;
	// browse the ready queue to see if proc is already in-queue
	for (p = ready_queue_head[hartid]; p->queue_next != NULL; p = p->queue_next)
		if (p == proc)
			return; // already in queue

	// p points to the last element of the ready queue
	if (p == proc)
		return;
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
static int ExitCoreCount = 0;

extern process procs[NPROC];
void schedule()
{
	uint64 hartid = read_tp();

	if (!ready_queue_head[hartid])
	{
		// by default, if there are no ready process, and all processes are in the status of
		// FREE and ZOMBIE, we should shutdown the emulated RISC-V machine.
		int should_shutdown = 1;

		for (int i = 0; i < NPROC; i++) 
		{
			if ((procs[i].status != FREE) && (procs[i].status != ZOMBIE) && procs[i].hartid == hartid)
			{
				should_shutdown = 0;
				sprint("hartid = %lld: ready queue empty, but process %d is not in free/zombie state:%d\n",
					   hartid, i, procs[i].status);
			}
		}
		if (should_shutdown)
		{
			sprint("hartid = %lld: no more ready processes\n", hartid);
			sync_barrier(&ExitCoreCount, NCPU);
			if (hartid == CTL_CORE) {
				shutdown(0);
			}
			else {
				return;
			}
		}
		else
		{
			panic("Not handled: we should let system wait for unfinished processes.\n");
		}
	}

	current[hartid] = ready_queue_head[hartid];
	assert((current[hartid]->status == READY));
	ready_queue_head[hartid] = ready_queue_head[hartid]->queue_next;

	current[hartid]->status = RUNNING;
	sprint("going to schedule process %d to run.\n", current[hartid]->pid);
	switch_to(current[hartid]);
}
