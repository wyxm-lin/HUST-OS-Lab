/*
 * implementing the scheduler
 */

#include "sched.h"
#include "spike_interface/spike_utils.h"
#include "sync_utils.h"
#include "spike_interface/atomic.h"
#include "core.h"

process *ready_queue_head = NULL;
spinlock_t ready_queue_lock;
extern process procs[NPROC];

void insert_to_ready_queue(process *proc)
{
	spinlock_lock(&ready_queue_lock);
	uint64 hartid = read_tp();
	sprint("hartid = %lld: going to insert process %d to ready queue.\n", hartid, proc->pid);

	if (ready_queue_head == NULL)
	{
		proc->status = READY;
		proc->queue_next = NULL;
		ready_queue_head = proc;
		spinlock_unlock(&ready_queue_lock);
		return;
	}

	process *p;
	for (p = ready_queue_head; p->queue_next != NULL; p = p->queue_next)
		if (p == proc) {
			spinlock_unlock(&ready_queue_lock);
			return;
		}

	if (p == proc) {
		spinlock_unlock(&ready_queue_lock);
		return;
	}

	p->queue_next = proc;
	proc->status = READY;
	proc->queue_next = NULL;

	spinlock_unlock(&ready_queue_lock);
	return;
}

void schedule()
{
	spinlock_lock(&ready_queue_lock);
	uint64 hartid = read_tp();
	set_idle(hartid);
	current[hartid] = NULL;

	if (!ready_queue_head)
	{
		sprint("no more ready processes\n");
		int should_shutdown = 1;
		if (is_all_idle() == Yes) {
			for (int i = 0; i < NPROC; i++) {
				if ((procs[i].status != FREE) && (procs[i].status != ZOMBIE)) {
					sprint("ready queue empty, but process %d is not in free/zombie state:%d\n", i, procs[i].status);
					panic("Not handled: we should let system wait for unfinished processes.\n");
				}
			}
			shutdown(0);
		}
		else {
			sprint("hartid = %lld: ready queue empty, but some cores are still busy.\n", hartid);
			spinlock_unlock(&ready_queue_lock);
			idle_process(hartid);
		}
	}

	while (TRUE) {
		uint64 choice = choose_core();
		if (choice == -1)
			break;
		current[choice] = ready_queue_head;
		assert((current[choice]->status == READY));
		current[choice]->status = RUNNING;
		current[choice]->trapframe->regs.tp = choice;
		set_busy(choice);
		sprint("hartid = %lld: going to schedule process %d to run.\n", choice, current[choice]->pid);
		ready_queue_head = ready_queue_head->queue_next;
		if (ready_queue_head == NULL)
			break;
	}
	spinlock_unlock(&ready_queue_lock);
	if (current[hartid] != NULL) {
		switch_to(current[hartid]);
	}
}