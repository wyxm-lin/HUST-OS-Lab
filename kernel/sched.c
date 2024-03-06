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
	sprint("hartid = %lld >> going to insert process %d to ready queue.\n", hartid, proc->pid);

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
		if (p == proc)
		{
			spinlock_unlock(&ready_queue_lock);
			return;
		}

	if (p == proc)
	{
		spinlock_unlock(&ready_queue_lock);
		return;
	}

	p->queue_next = proc;
	proc->status = READY;
	proc->queue_next = NULL;

	spinlock_unlock(&ready_queue_lock);
	return;
}

// hartid从busy->idle 都在外面
// hartid从idle->busy 只有可能在这里
// 为所有处于idle的core分配任务
// 成功获得锁后，本进程可能会是busy(因为在抢占锁的时候 可能会被别的core设置为busy，尽管每次调用schedule前都设置为idle了)
void schedule()
{
	spinlock_lock(&ready_queue_lock);
	uint64 hartid = read_tp();
	// sprint("hartid = %lld >> enter the scheduler.\n", hartid);

	if (!ready_queue_head)
	{
		sprint("no more ready processes\n");
		if (is_all_idle() == Yes)
		{
			for (int i = 0; i < NPROC; i++)
			{
				if ((procs[i].status != FREE) && (procs[i].status != ZOMBIE))
				{
					sprint("ready queue empty, but process %d is not in free/zombie state:%d\n", i, procs[i].status);
					panic("Not handled: we should let system wait for unfinished processes.\n");
				}
			}
			shutdown(0);
		}
		else
		{
			if (get_core_status(hartid) == CORE_STATUS_IDLE)
			{
				// sprint("hartid = %lld >> will enter idle.\n", hartid);
				spinlock_unlock(&ready_queue_lock);
				// sprint("hartid = %lld >> leave the scheduler.\n", hartid);
				idle_process(hartid);
			}
			else
			{
				// sprint("hartid = %lld >> has been busy by other core.\n", hartid);
				spinlock_unlock(&ready_queue_lock);
				// sprint("hartid = %lld >> leave the scheduler.\n", hartid);
				switch_to(current[hartid]);
			}
		}
		return; // never
	}

	while (TRUE)
	{
		uint64 choice = choose_core(); // 选中的核一定是idle的核心，所以current无须上锁
		if (choice == -1)
		{
			asm volatile("ecall");
			break;
		}
		current[choice] = ready_queue_head;
		assert((current[choice]->status == READY));
		current[choice]->status = RUNNING;
		current[choice]->trapframe->regs.tp = choice;
		set_busy(choice);
		sprint("hartid = %lld >> hartid = %lld going to schedule process %d to run.\n", hartid, choice, current[choice]->pid);
		ready_queue_head = ready_queue_head->queue_next;
		if (ready_queue_head == NULL)
		{
			asm volatile("ecall");
			break;
		}
	}

	if (get_core_status(hartid) == CORE_STATUS_IDLE)
	{
		// sprint("hartid = %lld >> will enter idle.\n", hartid);
		spinlock_unlock(&ready_queue_lock);
		// sprint("hartid = %lld >> leave the scheduler.\n", hartid);
		idle_process(hartid);
	}
	else
	{
		spinlock_unlock(&ready_queue_lock);
		// sprint("hartid = %lld >> leave the scheduler.\n", hartid);
		switch_to(current[hartid]);
	}
}

int sems[SEM_MAX];
int cnt = -1;
spinlock_t sem_lock[SEM_MAX];

int sem_new(int value)
{
	spinlock_lock(&sem_lock[++cnt]);
	sems[cnt] = value;
	int ret = cnt;
	spinlock_unlock(&sem_lock[cnt]);
	return ret;
}

int sem_P(uint64 sem)
{
	spinlock_lock(&sem_lock[sem]);
	sems[sem]--;
	if (sems[sem] < 0)
	{
		uint64 hartid = read_tp();
		current[hartid]->status = BLOCKED;
		current[hartid]->waitsem = sem; // 别把waitsem 写成 waitpid了
		set_idle(hartid);
		spinlock_unlock(&sem_lock[sem]);
		schedule();
	}
	else
	{
		spinlock_unlock(&sem_lock[sem]);
	}
	return 0;
}

int sem_V(uint64 sem)
{
	spinlock_lock(&sem_lock[sem]);
	sems[sem]++;
	if (sems[sem] <= 0)
	{
		for (int i = 0; i < NPROC; i++)
		{
			if (procs[i].status == BLOCKED && procs[i].waitsem == sem)
			{
				procs[i].waitsem = -1;
				insert_to_ready_queue(&procs[i]);
				spinlock_unlock(&sem_lock[sem]);
				break;
			}
		}
	}
	spinlock_unlock(&sem_lock[sem]);
	return 0;
}