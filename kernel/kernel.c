/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"
#include "util/types.h"
#include "vfs.h"
#include "rfs.h"
#include "ramdev.h"
#include "spike_interface/atomic.h"
#include "config.h"
#include "sync_utils.h"
#include "core.h"

//
// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point of
// S-mode trap vector). added @lab2_1
//
extern char trap_sec_start[];

//
// turn on paging. added @lab2_1
//
void enable_paging()
{
	// write the pointer to kernel page (table) directory into the CSR of "satp".
	write_csr(satp, MAKE_SATP(g_kernel_pagetable));

	// refresh tlb to invalidate its content.
	flush_tlb();
}

typedef union
{
	uint64 buf[MAX_CMDLINE_ARGS];
	char *argv[MAX_CMDLINE_ARGS];
} arg_buf;

//
// returns the number (should be 1) of string(s) after PKE kernel in command line.
// and store the string(s) in arg_bug_msg.
//
static size_t parse_args(arg_buf *arg_bug_msg)
{
	// HTIFSYS_getmainvars frontend call reads command arguments to (input) *arg_bug_msg
	long r = frontend_syscall(HTIFSYS_getmainvars, (uint64)arg_bug_msg,
							  sizeof(*arg_bug_msg), 0, 0, 0, 0, 0);
	kassert(r == 0);

	size_t pk_argc = arg_bug_msg->buf[0];
	uint64 *pk_argv = &arg_bug_msg->buf[1];

	int arg = 1; // skip the PKE OS kernel string, leave behind only the application name
	for (size_t i = 0; arg + i < pk_argc; i++)
		arg_bug_msg->argv[i] = (char *)(uintptr_t)pk_argv[arg + i];

	// returns the number of strings after PKE kernel in command line
	return pk_argc - arg;
}

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
MyStatus HasParsed = No;
arg_buf arg_bug_msg;
size_t argc = 0;

process *load_user_program()
{
	uint64 hartid = read_tp();

	if (argc != 0) {
        argc --;
        if (argc == 0) {
            return NULL;
        }       
    }

	process *proc;

	proc = alloc_process();

    sprint("hartid = %lld >> User application is loading.\n", hartid);

	// retrieve command line arguements
	if (HasParsed == No) {
        HasParsed = Yes;
        argc = parse_args(&arg_bug_msg);
        if (!argc)
            panic("You need to specify the application program!\n");
    }

	strcpy(proc->path, arg_bug_msg.argv[hartid]);

	load_bincode_from_host_elf(proc, arg_bug_msg.argv[hartid]);
	return proc;
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
int CoreBootCount = 0;
extern spinlock_t CoreBootLock;
MyStatus SModeInit = No;

int s_start(void)
{
	uint64 hartid = read_tp();

    sprint("hartid = %lld >> Enter supervisor mode...\n", hartid);
	// in the beginning, we use Bare mode (direct) memory mapping as in lab1.
	// but now, we are going to switch to the paging mode @lab2_1.
	// note, the code still works in Bare mode when calling pmm_init() and kern_vm_init().
	write_csr(satp, 0);

    if (SModeInit == No) {
        SModeInit = Yes;

        // init phisical memory manager
        pmm_init();

        // build the kernel page table
        kern_vm_init();

        // now, switch to paging mode by turning on paging (SV39)
        enable_paging();
        // the code now formally works in paging mode, meaning the page table is now in use.
        sprint("kernel page table is on \n");

        // added @lab3_1
        init_proc_pool();

        // init file system, added @lab4_1
        fs_init();
    }

    sprint("hartid = %lld >> Switch to user mode...\n", hartid);
	// the application code (elf) is first loaded into memory, and then put into execution
	// added @lab3_1
    process* app = load_user_program();
    if (app != NULL) {
        insert_to_ready_queue(app);
    }

	spinlock_unlock(&CoreBootLock);
    sync_barrier(&CoreBootCount, NCPU);
	
	schedule();

	// we should never reach here.
	return 0;
}
