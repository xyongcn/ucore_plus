#include <memlayout.h>

.text
.code64

# syscall instructor goes here 
.globl syscall_entry
.type syscall_entry, %function
syscall_entry:
    # swap in kernel GS BASE
	swapgs

	#backup user stack rsp on mycpu structure
	movq %rsp, %gs:0x10
	#switch to kernel stack
	movq %gs:0x8, %rsp

	pushq $USER_DS 	# push ss selector
	pushq %gs:0x10 	# push user rsp
	pushq %r11 		# user rflags sotres at r11
	pushq $USER_CS 	# push cs selector
	pushq %rcx 		# user rip stores at rcx

	pushq $0 # err code
	pushq $0x83 # linux syscall trap id

	# push registers to build a trap frame
    # therefore make the stack look like a struct trapframe

    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rax
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # push ds, es
    movw %es, %rax
    pushq %rax
    movw %ds, %rax
    pushq %rax

    # use %rdi to pass a pointer to the trapframe as an argument to trap()
    movq %rsp, %rdi

    # call trap(tf), where tf=%rsp
    call trap

.globl syscall_exit
syscall_exit:

    popq %rax
    movq %rax, %ds
    popq %rax
    movq %rax, %es

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rax
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi

    # restore user rsp 
	movq %gs:0x10, %rsp

	swapgs
	sysretq

# vectors.S sends all traps here.
.globl __alltraps
__alltraps:
    # push registers to build a trap frame
    # therefore make the stack look like a struct trapframe
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rax
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # push ds, es
    movw %es, %rax
    pushq %rax
    movw %ds, %rax
    pushq %rax

    # swap in kernel GS BASE if we come from user space
    cmpw $USER_CS, 0xA0(%rsp)
    jnz 1f
    swapgs

1:
    # use %rdi to pass a pointer to the trapframe as an argument to trap()
    movq %rsp, %rdi

    # call trap(tf), where tf=%rsp
    call trap

    # return falls through to trapret...
.globl __trapret
__trapret:

    # restore GS BASE if we'll back to userspace
    cmpw $USER_CS, 0xA0(%rsp)
    jnz 1f
    swapgs

1:
    # restore registers from stack

    # pop ds, es
    popq %rax
    movq %rax, %ds
    popq %rax
    movq %rax, %es

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rax
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi

    # get rid of the trap number and error code
    addq $0x10, %rsp
    iretq

.globl forkrets
forkrets:
    # set stack to this new process's trapframe
    movq %rdi, %rsp
    jmp __trapret
