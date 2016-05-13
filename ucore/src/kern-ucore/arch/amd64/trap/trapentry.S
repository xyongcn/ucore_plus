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
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
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
    #This is used as a workaround for syscalls on amd64 that changes
    #tf_rip or tf_rsp. sysret instruction doesn't really recover rip and rsp
    #from stack, it recovers them from rip from %rcx, and rip have to be set
    #manually.
    #TODO: Should have better way to do this (e.g. update reg_rcx and reg_rdx)
    #in syscall_linux, but seems it doesn't work and I can't figure out why.
    #TODO: Not sure about Intel/AMD's sysenter/sysexit, syscall/sysret, need
    #further research.
    add $16, %rsp #tf_trapno, tf_err
    popq %rcx #tf_rip
    add $16, %rsp #tf_cs (and its paddings), tf_rflags
    popq %rsp #tf_rsp

    # restore user rsp
	#movq %gs:0x10, %rsp

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
    movq %es, %rax
    pushq %rax
    movq %ds, %rax
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
