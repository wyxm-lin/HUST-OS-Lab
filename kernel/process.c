/*
 * Utility functions for process management. 
 *
 * Note: in Lab1, only one process (i.e., our user application) exists. Therefore, 
 * PKE OS at this stage will set "current" to the loaded user application, and also
 * switch to the old "current" process after trap handling.
 */

#include "riscv.h"
#include "strap.h"
#include "config.h"
#include "process.h"
#include "elf.h"
#include "string.h"

#include "spike_interface/spike_utils.h"

//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe*);

// current points to the currently running user-mode application.
process* current = NULL;

//
// switch to a user-mode process
//
void switch_to(process* proc) {
  assert(proc);
  current = proc;

  // write the smode_trap_vector (64-bit func. address) defined in kernel/strap_vector.S
  // to the stvec privilege register, such that trap handler pointed by smode_trap_vector
  // will be triggered when an interrupt occurs in S mode.
  write_csr(stvec, (uint64)smode_trap_vector);

  // set up trapframe values (in process structure) that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;  // process's kernel stack
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // SSTATUS_SPP and SSTATUS_SPIE are defined in kernel/riscv.h
  // set S Previous Privilege mode (the SSTATUS_SPP bit in sstatus register) to User mode.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  // write x back to 'sstatus' register to enable interrupts, and sret destination mode.
  write_csr(sstatus, x);

  // set S Exception Program Counter (sepc register) to the elf entry pc.
  write_csr(sepc, proc->trapframe->epc);

  // return_to_user() is defined in kernel/strap_vector.S. switch to user mode with sret.
  return_to_user(proc->trapframe);
}

// ADD:
char filename[128]; // comment:固定100长度不是很好的写法
char buf[0x1000];
struct stat st;
void debug_info_print(uint64 mepc) {
  // 寻找最近的代码
  // sprint("lgm:debug_info_print begin!\n");
  for (int i = 0; i < current->line_ind; i++) {
    if (current->line[i].addr > mepc) {
      // 打印文件名 + 行号
      sprint("Runtime error at ");
      sprint("%s/", current->dir[current->file[current->line[i - 1].file].dir]);
      sprint("%s:", current->file[current->line[i - 1].file].file);
      sprint("%d\n", current->line[i - 1].line);
      // 打印代码
      
      strcpy(filename, current->dir[current->file[current->line[i - 1].file].dir]);
      // sprint("over\n");
      strcpy(filename + strlen(filename), "/");
      strcpy(filename + strlen(filename), current->file[current->line[i - 1].file].file);
      // sprint("lgm:the file name is %s\n", filename);
      spike_file_t* fp = spike_file_open(filename, O_RDONLY, 0);
      spike_file_stat(fp, &st);
      spike_file_read(fp, buf, st.st_size);
      spike_file_close(fp);
      int idx = 0;
      int cnt = 0;
      while (idx < st.st_size) {
        if (buf[idx] == '\n') {
          cnt++;
          if (cnt == current->line[i - 1].line - 1) {
            for (int j = idx + 1; j < st.st_size; j++) {
              if (buf[j] == '\n') {
                break;
              }
              sprint("%c", buf[j]);
            }
            break;
          }
        }
        idx++;
      }
      sprint("\n");
      break;
    }
  }
  // sprint("lgm:debug_info_print end\n"); 
}