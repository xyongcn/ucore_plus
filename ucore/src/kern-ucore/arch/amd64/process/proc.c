#include <proc.h>
#include <pmm.h>
#include <slab.h>
#include <string.h>
#include <types.h>
#include <stdlib.h>
#include <elf.h>
#include <mp.h>

void forkret(void);
void forkrets(struct trapframe *tf);

// alloc_proc - create a proc struct and init fields
struct proc_struct *alloc_proc(void)
{
	struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
	if (proc != NULL) {
		proc->state = PROC_UNINIT;
		proc->pid = -1;
		proc->runs = 0;
		proc->kstack = 0;
		proc->need_resched = 0;
		proc->parent = NULL;
		proc->mm = NULL;
		memset(&(proc->context), 0, sizeof(struct context));
		proc->tf = NULL;
		proc->cr3 = PADDR(init_pgdir_get());
		proc->flags = 0;
		memset(proc->name, 0, PROC_NAME_LEN);
		proc->wait_state = 0;
		proc->cptr = proc->optr = proc->yptr = NULL;
		list_init(&(proc->thread_group));
		proc->rq = NULL;
		list_init(&(proc->run_link));
		proc->time_slice = 0;
		proc->sem_queue = NULL;
		event_box_init(&(proc->event_box));
		proc->fs_struct = NULL;
		proc->cpu_affinity = myid();
		spinlock_init(&proc->lock);
	}
	return proc;
}

int
copy_thread(uint32_t clone_flags, struct proc_struct *proc, uintptr_t rsp,
	    struct trapframe *tf)
{
	uintptr_t kstacktop = proc->kstack + KSTACKSIZE;
	proc->tf = (struct trapframe *)kstacktop - 1;
	*(proc->tf) = *tf;
	proc->tf->tf_regs.reg_rax = 0;
	proc->tf->tf_rsp = (rsp != 0) ? rsp : kstacktop;
	proc->tf->tf_rflags |= FL_IF;

	proc->context.rip = (uintptr_t) forkret;
	proc->context.rsp = (uintptr_t) (proc->tf);

	return 0;
}

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to
//       proc->tf in do_fork-->copy_thread function
int kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	struct trapframe tf;
	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_cs = KERNEL_CS;
	tf.tf_ds = tf.tf_es = tf.tf_ss = KERNEL_DS;
	tf.tf_regs.reg_rdi = (uint64_t) fn;
	tf.tf_regs.reg_rsi = (uint64_t) arg;
	tf.tf_rip = (uint64_t) kernel_thread_entry;
	return do_fork(clone_flags | CLONE_VM |__CLONE_PINCPU, 0, &tf);
}

// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
void forkret(void)
{
	if (!trap_in_kernel(current->tf)) {
		kern_leave();
	}
	forkrets(current->tf);
}

int kernel_execve(const char *name, const char **argv, const char **kenvp)
{
	int ret;
	asm volatile ("int %1;":"=a" (ret)
		      :"i"(T_SYSCALL), "0"(SYS_exec), "D"(name), "S"(argv),
		      "d"(kenvp)
		      :"memory");
	return ret;
}

/*
 * Initialize a new context for the program on the execve system call.
 * The init_new_context_dynamic
 */
int init_new_context_dynamic(struct proc_struct *proc, struct elfhdr *elf, int argc,
			 char **kargv, int envc, char **kenvp,
			 uint64_t elf_have_interpreter, uint64_t interpreter_entry,
			 uint64_t elf_entry, uint64_t linker_base, void* program_header_address)
{
  //Conclude this by using LD_SHOW_AUXV=1 on my laptop running Linux 4.5.4
  const int ELF_AUXILIARY_VECTOR_COUNT = 19;
  char* stack_top = USTACKTOP;

  size_t stack_param_size = 0;
  stack_param_size += sizeof(uint64_t);
  stack_param_size += (argc + 1) * sizeof(char**);
  stack_param_size += (envc + 1) * sizeof(char**);
  for(int i = 0; i < argc; i++) {
    stack_param_size += strlen(kargv[i]) + 1;
  }
  for(int i = 0; i < envc; i++) {
    stack_param_size += strlen(kenvp[i]) + 1;
  }
  stack_param_size +=
    ELF_AUXILIARY_VECTOR_COUNT * sizeof(struct elf64_auxiliary_vector);

  stack_top -= stack_param_size;

  char* stack_pos = stack_top;
  *((uint64_t*)stack_pos) = argc;
  stack_pos += sizeof(uint64_t);
  char **new_argv = (char**)stack_pos;
  stack_pos += (argc + 1) * sizeof(char**);
  char **new_envp = (char**)stack_pos;
  stack_pos += (envc + 1) * sizeof(char**);
  struct elf64_auxiliary_vector* auxiliary_vector = (struct elf64_auxiliary_vector*)stack_pos;
  stack_pos += ELF_AUXILIARY_VECTOR_COUNT * sizeof(struct elf64_auxiliary_vector);

  //TODO: Seems __strcpy for amd64 has a bug, always returning 0, instead
  //of the end of the destination string.
  //But I don't find the exact reason so the some strlen after strcpy is used.
  for(int i = 0; i < argc; i++) {
    strcpy(stack_pos, kargv[i]);
    new_argv[i] = stack_pos;
    stack_pos += strlen(kargv[i]) + 1;
  }
  new_argv[argc] = NULL;
  for(int i = 0; i < envc; i++) {
    strcpy(stack_pos, kenvp[i]);
    new_envp[i] = stack_pos;
    stack_pos += (strlen(kenvp[i]) + 1);
  }
  new_envp[envc] = NULL;

  auxiliary_vector[0].a_type = AT_IGNORE; //AT_SYSINFO_EHDR
  //We don't have the vDSO
  auxiliary_vector[1].a_type = AT_IGNORE; //AT_HWCAP
  //TODO: Add hardware detection data
  auxiliary_vector[2].a_type = AT_PAGESZ;
  auxiliary_vector[2].a_val = PGSIZE;
  auxiliary_vector[3].a_type = AT_CLKTCK;
  auxiliary_vector[3].a_val = 100;
  if(program_header_address != NULL) {
    auxiliary_vector[4].a_type = AT_PHDR;
    auxiliary_vector[4].a_val = program_header_address;
  }
  else {
    auxiliary_vector[4].a_type = AT_IGNORE;
  }
  auxiliary_vector[5].a_type = AT_PHENT;
  auxiliary_vector[5].a_val = elf->e_phentsize;
  auxiliary_vector[6].a_type = AT_PHNUM;
  auxiliary_vector[6].a_val = elf->e_phnum;
  auxiliary_vector[7].a_type = AT_BASE; //The base address of the program interpreter
  auxiliary_vector[7].a_val = linker_base; //TODO
  auxiliary_vector[8].a_type = AT_FLAGS;
  auxiliary_vector[8].a_val = 0;
  auxiliary_vector[9].a_type = AT_ENTRY;
  auxiliary_vector[9].a_val = elf_entry; //TODO

  //TODO: Currently uCore doesn't support multi-user. So we're simulating
  //The user root and the group root.
  auxiliary_vector[10].a_type = AT_UID;
  auxiliary_vector[11].a_type = AT_EUID;
  auxiliary_vector[12].a_type = AT_GID;
  auxiliary_vector[13].a_type = AT_EGID;
  for(int i = 10; i <= 13; i++) {
    auxiliary_vector[i].a_val = 0;
  }
  auxiliary_vector[14].a_type = AT_SECURE;
  auxiliary_vector[14].a_val = 0;
  auxiliary_vector[15].a_type = AT_RANDOM;
  auxiliary_vector[15].a_val = rand();
  //auxiliary_vector[16].a_type = AT_PLATFORM;
  //auxiliary_vector[17].a_type = AT_EXECFN;
  auxiliary_vector[16].a_type = AT_NULL;
  auxiliary_vector[16].a_val = 0;
  struct trapframe *tf = current->tf;
	memset(tf, 0, sizeof(struct trapframe));
	tf->tf_cs = USER_CS;
	tf->tf_ds = USER_DS;
	tf->tf_es = USER_DS;
	tf->tf_ss = USER_DS;
	tf->tf_rsp = (uint64_t)stack_top;
  if(elf_have_interpreter) {
	   tf->tf_rip = interpreter_entry;
  }
  else {
    tf->tf_rip = elf_entry;
  }
	tf->tf_rflags = FL_IF;

  //Only uCore built-in utilities cares about this - C library likes uClibc only
  //looks up data on the stack.
	tf->tf_regs.reg_rdi = (uint64_t)argc;
	tf->tf_regs.reg_rsi = (uintptr_t)new_argv;
	return 0;
}

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void cpu_idle(void)
{
	while (1) {
		assert((read_rflags() & FL_IF) != 0);
		asm volatile ("hlt");
	}
}

int do_execve_arch_hook(int argc, char **kargv)
{
	return 0;
}

int ucore_kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	kernel_thread(fn, arg, clone_flags);
}

void de_thread_arch_hook(struct proc_struct *proc)
{
}
