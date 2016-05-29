#include <types.h>
#include <mmu.h>
#include <memlayout.h>
#include <trap.h>
#include <arch.h>
#include <stdio.h>
#include <kdebug.h>
#include <assert.h>
#include <sync.h>
#include <monitor.h>
#include <vmm.h>
#include <proc.h>
#include <sched.h>
#include <unistd.h>
#include <syscall.h>
#include <error.h>
#include <kio.h>
#include <clock.h>
#include <intr.h>
#include <mp.h>
#include <ioapic.h>
#include <sysconf.h>
#include <refcache.h>

#define TICK_NUM 30

static struct gatedesc idt[256] = { {0} };

struct pseudodesc idt_pd = {
	sizeof(idt) - 1, (uintptr_t) idt
};

void idt_init(void)
{
	extern uintptr_t __vectors[];
	int i;
	for (i = 0; i < sizeof(idt) / sizeof(struct gatedesc); i++) {
		SETGATE(idt[i], 0, GD_KTEXT, __vectors[i], DPL_KERNEL);
	}
	SETGATE(idt[T_SYSCALL], 0, GD_KTEXT, __vectors[T_SYSCALL], DPL_USER);
	SETGATE(idt[T_IPI], 0, GD_KTEXT, __vectors[T_IPI], DPL_USER);
	SETGATE(idt[T_IPI_DOS], 0, GD_KTEXT, __vectors[T_IPI_DOS], DPL_USER);
	lidt(&idt_pd);
}

void syscall_init()
{
	extern void syscall_entry();
	//extern void fastcall_entry();
	writemsr(MSR_EFER, readmsr(MSR_EFER) | (1 << 0));
	writemsr(MSR_SFMASK, FL_IF); // set EFLAGS
	writemsr(MSR_LSTAR, (uint64_t)syscall_entry); // set syscall entry
	writemsr(MSR_STAR, ((uint64_t)(GD_UTEXT_32 | 3) << 48) | ((uint64_t)(GD_KTEXT) << 32 ));
}

static const char *trapname(int trapno)
{
	static const char *const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames) / sizeof(const char *const)) {
		return excnames[trapno];
	}
	if (trapno == T_SYSCALL) {
		return "System call";
	}
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16) {
		return "Hardware Interrupt";
	}
	return "(unknown trap)";
}

bool trap_in_kernel(struct trapframe * tf)
{
	return (tf->tf_cs == (uint16_t) KERNEL_CS);
}

static const char *IA32flags[] = {
	"CF", NULL, "PF", NULL, "AF", NULL, "ZF", "SF",
	"TF", "IF", "DF", "OF", NULL, NULL, "NT", NULL,
	"RF", "VM", "AC", "VIF", "VIP", "ID", NULL, NULL,
};

void print_trapframe(struct trapframe *tf)
{
	kprintf("trapframe at %p\n", tf);
	print_regs(&tf->tf_regs);
	kprintf("  trap 0x--------%08x %s\n", tf->tf_trapno,
		trapname(tf->tf_trapno));
	kprintf("  err  0x%016llx\n", tf->tf_err);
	kprintf("  rip  0x%016llx\n", tf->tf_rip);
	kprintf("  cs   0x------------%04x\n", tf->tf_cs);
	kprintf("  ds   0x------------%04x\n", tf->tf_ds);
	kprintf("  es   0x------------%04x\n", tf->tf_es);
	kprintf("  flag 0x%016llx\n", tf->tf_rflags);
	kprintf("  rsp  0x%016llx\n", tf->tf_rsp);
	kprintf("  ss   0x------------%04x\n", tf->tf_ss);

	int i, j;
	for (i = 0, j = 1; i < sizeof(IA32flags) / sizeof(IA32flags[0]);
	     i++, j <<= 1) {
		if ((tf->tf_rflags & j) && IA32flags[i] != NULL) {
			kprintf("%s,", IA32flags[i]);
		}
	}
	kprintf("IOPL=%d\n", (tf->tf_rflags & FL_IOPL_MASK) >> 12);
}

void print_regs(struct pushregs *regs)
{
	kprintf("  rdi  0x%016llx\n", regs->reg_rdi);
	kprintf("  rsi  0x%016llx\n", regs->reg_rsi);
	kprintf("  rdx  0x%016llx\n", regs->reg_rdx);
	kprintf("  rcx  0x%016llx\n", regs->reg_rcx);
	kprintf("  rax  0x%016llx\n", regs->reg_rax);
	kprintf("  r8   0x%016llx\n", regs->reg_r8);
	kprintf("  r9   0x%016llx\n", regs->reg_r9);
	kprintf("  r10  0x%016llx\n", regs->reg_r10);
	kprintf("  r11  0x%016llx\n", regs->reg_r11);
	kprintf("  rbx  0x%016llx\n", regs->reg_rbx);
	kprintf("  rbp  0x%016llx\n", regs->reg_rbp);
	kprintf("  r12  0x%016llx\n", regs->reg_r12);
	kprintf("  r13  0x%016llx\n", regs->reg_r13);
	kprintf("  r14  0x%016llx\n", regs->reg_r14);
	kprintf("  r15  0x%016llx\n", regs->reg_r15);
}

static inline void print_pgfault(struct trapframe *tf)
{
	/* error_code:
	 * bit 0 == 0 means no page found, 1 means protection fault
	 * bit 1 == 0 means read, 1 means write
	 * bit 2 == 0 means kernel, 1 means user
	 * */
	uintptr_t addr = rcr2();
	if ((addr >> 32) & 0x8000) {
		addr |= (0xFFFFLLU << 48);
	}
  //kprintf("page fault at\r\n");
  print_trapframe(tf);
  //print_stackframe();
  //print_stackframe_ext(tf->tf_rsp, 0x1234);
	kprintf("page fault at 0x%016llx: %c/%c [%s].\n", addr,
		(tf->tf_err & 4) ? 'U' : 'K',
		(tf->tf_err & 2) ? 'W' : 'R',
		(tf->tf_err & 1) ? "protection fault" : "no page found");
}

extern uint32_t debug;

void do_debug(struct trapframe *tf)
{
//	print_trapframe(tf);
}


static int pgfault_handler(struct trapframe *tf)
{
	extern struct mm_struct *check_mm_struct;
	struct mm_struct *mm;
	if (check_mm_struct != NULL) {
		assert(current == idleproc);
		mm = check_mm_struct;
	} else {
		if (current == NULL) {
			print_trapframe(tf);
			print_pgfault(tf);
			panic("unhandled page fault.\n");
		}
		mm = current->mm;
	}
uintptr_t addr = rcr2();
if (addr >= 0x20060000 && addr <= 0x00000fffefff2040)
{
	print_pgfault(tf);
	do_debug(tf);
}
	return do_pgfault(mm, tf->tf_err, rcr2());
}

static void trap_dispatch(struct trapframe *tf)
{
	char c;
	int ret;
	int id = myid();
uintptr_t addr;
	switch (tf->tf_trapno) {
	case T_PGFLT:
//addr = rcr2();
//if (addr >= 0x20060000 && addr <= 0x00000fffefff2040)
//	kprintf("in: user rsp %p\n",tf->tf_rsp);
		if ((ret = pgfault_handler(tf)) != 0) {
			print_trapframe(tf);
			if (current == NULL) {
				panic("handle pgfault failed. %e\n", ret);
			} else {
        uintptr_t addr = rcr2();
				if (trap_in_kernel(tf)) {
          print_stackframe();
					panic
					    ("%llx: handle pgfault failed in kernel mode. %e\n",
					     addr, ret);
				}
				print_pgfault(tf);
				kprintf("pgfault_handler return %d\n", ret);
				kprintf("killed by kernel.\n");
				do_exit(-E_KILLED);
			}
		}
//if (addr >= 0x20060000 && addr <= 0x00000fffefff2040)
//		kprintf("out :user rsp %p\n",tf->tf_rsp);
		break;
	case T_SYSCALL:
	case 0x6:
		syscall();
		break;
	case T_FAST_SYSCALL:
		assert(tf->tf_rsp == mycpu()->user_rsp);
	//	kprintf("in: user rsp %p\n",tf->tf_rsp);
		syscall_linux();
	//	kprintf("out :user rsp %p\n",tf->tf_rsp);
		break;
#ifdef UCONFIG_ENABLE_IPI
		/* IPI */
	case T_IPICALL:
		do_ipicall();
		break;
#endif
	case T_TLBFLUSH:
		lcr3(rcr3());
	case IRQ_OFFSET + IRQ_TIMER:
		if(id==0){
			ticks++;
			run_timer_list();
		}
		refcache_tick();

		assert(current != NULL);
		break;
	case IRQ_OFFSET + IRQ_COM1:
	case IRQ_OFFSET + IRQ_KBD:
	case IRQ_OFFSET + IRQ_LPT1:
		c = cons_getc();

		extern void dev_stdin_write(char c);
		dev_stdin_write(c);
		break;
	case IRQ_OFFSET + IRQ_IDE1:
	case IRQ_OFFSET + IRQ_IDE2:
		/* do nothing */
		break;
	default:
		print_trapframe(tf);
		if (current != NULL) {
			kprintf("unhandled trap.\n");
			do_exit(-E_KILLED);
		}
		panic("unexpected trap in kernel.\n");
	}

	if (tf->tf_trapno >= IRQ_OFFSET &&
	    tf->tf_trapno < IRQ_OFFSET + IRQ_COUNT)
		lapic_eoi();
}

int c=0;
void trap(struct trapframe *tf)
{
	// used for previous projects
	if (current == NULL) {
		trap_dispatch(tf);
	} else {
		// keep a trapframe chain in stack
		struct trapframe *otf = current->tf;
		current->tf = tf;

		bool in_kernel = trap_in_kernel(tf);
		if (!in_kernel || (current == idleproc && otf == NULL))
			kern_enter(tf->tf_trapno + 1000);

		// kprintf("%d %d {{{\n", lapic_id, current->pid);
		trap_dispatch(tf);
		in_kernel = trap_in_kernel(current->tf);
		// kprintf("%d %d |||\n", lapic_id, current->pid);

		current->tf = otf;

		if (!in_kernel) {
			may_killed();
			if (current->need_resched) {
				schedule();
			}
		} else if (current == idleproc) {
			schedule();
		}
		// kprintf("%d %d }}}\n", lapic_id, current->pid);
		if (!in_kernel || (current == idleproc && otf == NULL))
			kern_leave();
	}
}

int ucore_in_interrupt()
{
	return 0;
}

void irq_enable(int irq_no)
{
	//XXX
	ioapic_enable(0, irq_no, 0);
}

void irq_disable(int irq_no)
{
	//XXX
	ioapic_disable(0, irq_no);
}

void trap_init(void)
{
	//XXX
	if(!sysconf.lioapic_count)
		return;
	irq_enable(IRQ_KBD);
	irq_enable(IRQ_COM1);
}
