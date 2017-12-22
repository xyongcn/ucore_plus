#include <types.h>
#include <list.h>

#define PTRACE_TRACEME             0
#define PTRACE_PEEKTEXT            1
#define PTRACE_PEEKDATA            2
#define PTRACE_PEEKUSR             3
#define PTRACE_POKETEXT            4
#define PTRACE_POKEDATA            5
#define PTRACE_POKEUSR             6
#define PTRACE_CONT                7
#define PTRACE_KILL                8
#define PTRACE_SINGLESTEP          9

#define PTRACE_ATTACH             16
#define PTRACE_DETACH             17

#define PTRACE_SYSCALL            24

#ifdef ARCH_AMD64

#define PTRACE_GETREGS            12
#define PTRACE_SETREGS            13
#define PTRACE_GETFPREGS          14
#define PTRACE_SETFPREGS          15
#define PTRACE_GETFPXREGS         18
#define PTRACE_SETFPXREGS         19

struct user_regs_struct {
  unsigned long   r15;
  unsigned long   r14;
  unsigned long   r13;
  unsigned long   r12;
  unsigned long   bp;
  unsigned long   bx;
  unsigned long   r11;
  unsigned long   r10;
  unsigned long   r9;
  unsigned long   r8;
  unsigned long   ax;
  unsigned long   cx;
  unsigned long   dx;
  unsigned long   si;
  unsigned long   di;
  unsigned long   orig_ax;
  unsigned long   ip;
  unsigned long   cs;
  unsigned long   flags;
  unsigned long   sp;
  unsigned long   ss;
  unsigned long   fs_base;
  unsigned long   gs_base;
  unsigned long   ds;
  unsigned long   es;
  unsigned long   fs;
  unsigned long   gs;
};

#endif // ARCH_AMD64

#ifdef ARCH_MIPS

typedef struct user_regs_struct {
  /* FIXME: check this definition */
  uint32_t regs[32];
  uint32_t status;
  uint32_t lo;
  uint32_t hi;
  uint32_t badvaddr;
  uint32_t cause;
  uint32_t epc;
};

#define PTRACE_GETREGS            12
#define PTRACE_SETREGS            13
#define PTRACE_GETFPREGS          14
#define PTRACE_SETFPREGS          15
#define PTRACE_GETFPXREGS         18
#define PTRACE_SETFPXREGS         19

#endif //ARCH_MIPS

struct proc_struct;

struct ptrace_process_pair {
  struct proc_struct *tracer;
  struct proc_struct *tracee;
  list_entry_t list_entry;
};

void ptrace_init();
void ptrace_execve_hook();
long ptrace_syscall(long request, long pid, unsigned long addr, unsigned long data);
