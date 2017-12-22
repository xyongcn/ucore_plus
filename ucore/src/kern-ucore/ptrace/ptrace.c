#if defined(ARCH_AMD64) || defined(ARCH_MIPS)

#include <types.h>
#include <assert.h>
#include <list.h>
#include <stddef.h>
#include <proc.h>
#include <pmm.h>
#include <vmm.h>
#include <slab.h>
#include <error.h>
#include <interrupt_manager.h>
#include "ptrace.h"

list_entry_t ptrace_pair_list;

void ptrace_interrupt_hook(struct trapframe* trapframe);

static void ptrace_add_pair(struct proc_struct *tracer, struct proc_struct *tracee)
{
  struct ptrace_process_pair *pair = kmalloc(sizeof(struct ptrace_process_pair));
  pair->tracer = tracer;
  pair->tracee = tracee;
  list_add(&ptrace_pair_list, &pair->list_entry);
}

static struct ptrace_process_pair* ptrace_find_pair_by_tracee(struct proc_struct *tracee)
{
  for(list_entry_t* i = list_next(&ptrace_pair_list);
  i != &ptrace_pair_list; i = list_next(i)) {
    struct ptrace_process_pair* pair = container_of(i, struct ptrace_process_pair, list_entry);
    if(pair->tracee == tracee) {
      return pair;
    }
  }
  return NULL;
}

static uintptr_t ptrace_vaddr_to_kaddr(struct proc_struct* proc, uintptr_t vaddr) {
  uintptr_t la = ROUNDDOWN(vaddr, PGSIZE);
  struct Page * page = get_page(proc->mm->pgdir, la, NULL);
  if(page == NULL)
    return 0;
  return page2kva(page) + (vaddr - la);
}

uintptr_t ptrace_peek_data(long pid, uintptr_t vaddr, uintptr_t *val_store) {
  //TODO: vaddr and vaddr + sizeof(uintptr_t) can be in different pages.
  struct proc_struct *proc = find_proc(pid);
  struct ptrace_process_pair* pair = ptrace_find_pair_by_tracee(proc);
  if(proc == NULL || pair == NULL || pair->tracer != current) {
    return -E_INVAL;
  }
  uintptr_t kaddr = ptrace_vaddr_to_kaddr(proc, vaddr);
  if(kaddr == 0) {
    *val_store = 0L;
    return 0;
  }
  *val_store= *(uintptr_t*)kaddr;
  return 0;
}

uintptr_t ptrace_poke_data(long pid, uintptr_t vaddr, machine_word_t val) {
  //TODO: vaddr and vaddr + sizeof(uintptr_t) can be in different pages.
  struct proc_struct *proc = find_proc(pid);
  struct ptrace_process_pair* pair = ptrace_find_pair_by_tracee(proc);
  if(proc == NULL || pair == NULL || pair->tracer != current) {
    return ~0L;
  }
  uintptr_t kaddr = ptrace_vaddr_to_kaddr(proc, vaddr);
  if(kaddr == 0) {
    return 0;
  }
  *(uintptr_t*)ptrace_vaddr_to_kaddr(proc, vaddr) = val;
  kprintf("ptrace_poke_data : %llx\n", *(uintptr_t*)ptrace_vaddr_to_kaddr(proc, vaddr));
  return 0;
}

long ptrace_continue(long pid)
{
  struct proc_struct *proc = find_proc(pid);
  struct ptrace_process_pair* pair = ptrace_find_pair_by_tracee(proc);
  if(proc == NULL || pair == NULL || pair->tracer != current) {
    return -1;
  }
  wakeup_proc(proc);
  schedule();
  return 0;
}

void ptrace_init()
{
  list_init(&ptrace_pair_list);
#ifdef ARCH_AMD64
  interrupt_manager_register_handler(3, ptrace_interrupt_hook);
  interrupt_manager_register_handler(1, ptrace_interrupt_hook);
#endif
}

void ptrace_execve_hook()
{
  struct ptrace_process_pair* pair = ptrace_find_pair_by_tracee(current);
  if(pair != NULL) {
    bool intr_flag;
    local_intr_save(intr_flag);
    if(pair->tracer->state == PROC_SLEEPING && pair->tracer->wait_state == WT_CHILD) {
      wakeup_proc(pair->tracer);
    }
    current->state = PROC_STOPPED;
    current->wait_state = WT_INTERRUPTED;
    local_intr_restore(intr_flag);
    schedule();
  }
}

void ptrace_interrupt_hook(struct trapframe* trapframe)
{
#ifdef ARCH_AMD64
  if(trapframe->tf_trapno == 1) {
    current->tf->tf_rflags &= ~0x100;
  }
#endif
  ptrace_execve_hook();
}

void ptrace_get_registers(long pid, struct user_regs_struct* user_regs_struct)
{
  kprintf("user_regs_struct = %llx\n", user_regs_struct);
  struct proc_struct *proc = find_proc(pid);
  struct ptrace_process_pair* pair = ptrace_find_pair_by_tracee(proc);
  if(proc == NULL || pair == NULL || pair->tracer != current) {
    memset(user_regs_struct, ~0, sizeof(struct user_regs_struct));
    return;
  }
#if defined(ARCH_AMD64)
  user_regs_struct->r13 = proc->tf->tf_regs.reg_r13;
  user_regs_struct->r15 = proc->tf->tf_regs.reg_r15;
  user_regs_struct->r14 = proc->tf->tf_regs.reg_r14;
  user_regs_struct->r12 = proc->tf->tf_regs.reg_r12;
  user_regs_struct->bp = proc->tf->tf_regs.reg_rbp;
  user_regs_struct->bx = proc->tf->tf_regs.reg_rbx;
  user_regs_struct->r11 = proc->tf->tf_regs.reg_r11;
  user_regs_struct->r10 = proc->tf->tf_regs.reg_r10;
  user_regs_struct->r9 = proc->tf->tf_regs.reg_r9;
  user_regs_struct->r8 = proc->tf->tf_regs.reg_r8;
  user_regs_struct->ax = proc->tf->tf_regs.reg_rax;
  user_regs_struct->cx = proc->tf->tf_regs.reg_rcx;
  user_regs_struct->dx = proc->tf->tf_regs.reg_rdx;
  user_regs_struct->si = proc->tf->tf_regs.reg_rsi;
  user_regs_struct->di = proc->tf->tf_regs.reg_rdi;
  user_regs_struct->orig_ax = ~0L;
  user_regs_struct->ip = proc->tf->tf_rip;
  user_regs_struct->cs = proc->tf->tf_cs;
  user_regs_struct->flags = proc->tf->tf_rflags;
  user_regs_struct->sp = proc->tf->tf_rsp;
  user_regs_struct->ss = proc->tf->tf_ss;
  user_regs_struct->fs_base = ~0L;
  user_regs_struct->gs_base = ~0L;
  user_regs_struct->ds = proc->tf->tf_ds;
  user_regs_struct->es = proc->tf->tf_es;
  user_regs_struct->fs = ~0L;
  user_regs_struct->gs = ~0L;
#elif defined(ARCH_MIPS)
  for(int i = 0; i < 30; i++) {
    user_regs_struct->regs[i + 1] = proc->tf->tf_regs.reg_r[i];
  }
  user_regs_struct->regs[0] = 0;
  user_regs_struct->regs[31] = proc->tf->tf_ra;
  user_regs_struct->status = proc->tf->tf_status;
  user_regs_struct->lo = proc->tf->tf_lo;
  user_regs_struct->hi = proc->tf->tf_hi;
  user_regs_struct->badvaddr = proc->tf->tf_vaddr;
  user_regs_struct->cause = proc->tf->tf_cause;
  user_regs_struct->epc = proc->tf->tf_epc;
#endif
}

void ptrace_set_registers(long pid, struct user_regs_struct* user_regs_struct)
{
  struct proc_struct *proc = find_proc(pid);
  struct ptrace_process_pair* pair = ptrace_find_pair_by_tracee(proc);
  if(proc == NULL || pair == NULL || pair->tracer != current) {
    return;
  }
#if defined(ARCH_AMD64)
  proc->tf->tf_regs.reg_r13 = user_regs_struct->r13;
  proc->tf->tf_regs.reg_r15 = user_regs_struct->r15;
  proc->tf->tf_regs.reg_r14 = user_regs_struct->r14;
  proc->tf->tf_regs.reg_r12 = user_regs_struct->r12;
  proc->tf->tf_regs.reg_rbp = user_regs_struct->bp;
  proc->tf->tf_regs.reg_rbx = user_regs_struct->bx;
  proc->tf->tf_regs.reg_r11 = user_regs_struct->r11;
  proc->tf->tf_regs.reg_r10 = user_regs_struct->r10;
  proc->tf->tf_regs.reg_r9 = user_regs_struct->r9;
  proc->tf->tf_regs.reg_r8 = user_regs_struct->r8;
  proc->tf->tf_regs.reg_rax = user_regs_struct->ax;
  proc->tf->tf_regs.reg_rcx = user_regs_struct->cx;
  proc->tf->tf_regs.reg_rdx = user_regs_struct->dx;
  proc->tf->tf_regs.reg_rsi = user_regs_struct->si;
  proc->tf->tf_regs.reg_rdi = user_regs_struct->di;
  proc->tf->tf_rip = user_regs_struct->ip;
  //proc->tf->tf_cs = user_regs_struct->cs;
  //proc->tf->tf_rflags = user_regs_struct->flags;
  proc->tf->tf_rsp = user_regs_struct->sp;
  //proc->tf->tf_ss = user_regs_struct->ss;
  //proc->tf->tf_ds = user_regs_struct->ds;
  //proc->tf->tf_es = user_regs_struct->es;
#elif defined(ARCH_MIPS)
    for(int i = 0; i < 30; i++) {
      proc->tf->tf_regs.reg_r[i] = user_regs_struct->regs[i + 1];
    }
    proc->tf->tf_ra = user_regs_struct->regs[31];
    proc->tf->tf_status = user_regs_struct->status;
    proc->tf->tf_lo = user_regs_struct->lo;
    proc->tf->tf_hi = user_regs_struct->hi;
    proc->tf->tf_vaddr = user_regs_struct->badvaddr;
    proc->tf->tf_cause = user_regs_struct->cause;
    proc->tf->tf_epc = user_regs_struct->epc;
#endif
}

#ifdef ARCH_MIPS
int ptrace_softmmu_single_step(long pid)
{
  //TODO
  struct proc_struct *proc = find_proc(pid);
  struct trapframe *tf = proc->tf;
  const OPCODE_MASK = 0xFC000000;
  const uint32_t LUI_OPCODE = 0x3C000000;
  const uint32_t JAL_OPCODE = 0x0C000000;
  //const uint32_t JAL_OPCODE = 0xAC000000;
  uint32_t instruction;
  ptrace_peek_data(pid, tf->tf_epc, &instruction);
  kprintf("Instruction is %lx\n", instruction);
  if((instruction & OPCODE_MASK) == LUI_OPCODE) {
    uint32_t imm = instruction & 0xFFFF;
    int rt = (instruction >> 16) & 0x1F;
    tf->tf_regs.reg_r[rt - 1] = imm << 16;
    tf->tf_epc = (void*)((uint32_t)tf->tf_epc + 4);
  }
  else if((instruction & OPCODE_MASK) == JAL_OPCODE) {
    tf->tf_ra = tf->tf_epc + 8;
    tf->tf_epc = (instruction & 0x03FFFFFF) << 2;
  }
  return 0;
}
#endif

int ptrace_single_step(long pid)
{
  struct proc_struct *proc = find_proc(pid);
  struct ptrace_process_pair* pair = ptrace_find_pair_by_tracee(proc);
  if(proc == NULL || pair == NULL || pair->tracer != current) {
    return -E_INVAL;
  }
#if defined(ARCH_AMD64)
  proc->tf->tf_rflags |= 0x100;
  ptrace_continue(pid);
#elif defined(ARCH_MIPS)
  ptrace_softmmu_single_step(pid);
#endif
  return 0;
}

long ptrace_syscall(long request, long pid, unsigned long addr, unsigned long data)
{
  switch(request) {
    case PTRACE_TRACEME:
      assert(current != NULL);
      assert(current->parent != NULL);
      ptrace_add_pair(current->parent, current);
      return 0;
    case PTRACE_GETREGS:
    case PTRACE_GETFPREGS:
      ptrace_get_registers(pid, (struct user_regs_struct*)data);
      return 0;
    case PTRACE_SETREGS:
    case PTRACE_SETFPREGS:
      ptrace_set_registers(pid, (struct user_regs_struct*)data);
      return 0;
    case PTRACE_PEEKTEXT:
    case PTRACE_PEEKDATA:
      return ptrace_peek_data(pid, addr, data);
    case PTRACE_POKETEXT:
    case PTRACE_POKEDATA:
      return ptrace_poke_data(pid, addr, data);
    case PTRACE_CONT:
      return ptrace_continue(pid);
    case PTRACE_SINGLESTEP:
      return ptrace_single_step(pid);
  }
  panic("Not implemented. %d\n", request);
  return 0;
}

#endif
