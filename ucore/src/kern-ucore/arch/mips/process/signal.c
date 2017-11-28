#include <proc.h>
#include <assert.h>
#include <error.h>
#include <mips_trapframe.h>

#include "arch_signal.h"

int __sig_setup_frame(int sign, struct sigaction *act, sigset_t oldset,
		      struct trapframe *tf)
{
  goto __signal_return_code_end;
  __signal_return_code_begin:
  __asm__ volatile (
    "li $v0, 119;"
    "syscall;"
    "nop;"
  );
  __signal_return_code_end:;
  size_t signal_return_code_length =
    &&__signal_return_code_end - &&__signal_return_code_begin;
  struct mm_struct *memory_manager = current->mm;
  uintptr_t stack = current->signal_info.sas_ss_sp;
  if (stack == 0) {
    stack = tf->tf_regs.reg_r[MIPS_REG_SP];
  }
  //Alignment
  struct sigframe *frame =
      (struct sigframe *)((stack - sizeof(struct sigframe)) & 0xfffffff0);
  memset(frame, 0x22, sizeof(struct sigframe));
  frame->pretcode = (uintptr_t) frame->retcode;
  //tf->tf_regs.reg_rdi = sign;
  frame->sign = sign;
  frame->tf = *tf;
  frame->old_blocked = oldset;
  assert(signal_return_code_length < 32);
  memset(frame->retcode, 0x00, 32);
  if(sign == 0xFFFFFFF) {
    // TODO: This is an ugly workaround to ensure the compiler not ignoring
    // the inline assembly.
    goto __signal_return_code_begin;
  }
  memcpy(frame->retcode, &&__signal_return_code_begin, signal_return_code_length);

	//tf->tf_regs.reg_rax = sign;
	tf->tf_epc = (uintptr_t)act->sa_handler;
	tf->tf_regs.reg_r[MIPS_REG_SP] = frame;
  //tf->tf_regs.reg_rbp = frame;
  return 0;
}

// do syscall sigreturn, reset the user stack and eip
int do_sigreturn()
{
    panic("MIPS do_sigreturn");
	struct mm_struct *mm = current->mm;
	if (!current)
		return -E_INVAL;
	struct sighand_struct *sighand = current->signal_info.sighand;
	if (!sighand)
		return -E_INVAL;

	struct sigframe _kframe, *kframe = &_kframe;

	struct sigframe *frame =
	    (struct sigframe *)(((void*)current->tf->tf_regs.reg_r[MIPS_REG_SP] - 8));
	lock_mm(mm);
	{
		if (!copy_from_user
		    (mm, kframe, frame, sizeof(struct sigframe), 0)) {
			unlock_mm(mm);
			kfree(kframe);
			return -E_INVAL;
		}
	}
	unlock_mm(mm);

	lock_sig(sighand);
	current->signal_info.blocked = kframe->old_blocked;
	sig_recalc_pending(current);
	unlock_sig(sighand);

	*(current->tf) = kframe->tf;
	return 0;
}
