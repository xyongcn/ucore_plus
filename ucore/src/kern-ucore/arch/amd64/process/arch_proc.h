#ifndef __ARCH_PROC_H__
#define __ARCH_PROC_H__

#include <types.h>

struct context {
	uint64_t rip;
	uint64_t rsp;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t r8;
	uint64_t r9;
	uint64_t r10;
	uint64_t r11;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
};

/* The architecture-dependent part of the PCB */
struct arch_proc_struct {
};

//Bionic C related context initialization code
int init_new_context_dynamic(struct proc_struct *proc, struct elfhdr *elf, int argc,
			 char **kargv, int envc, char **kenvp,
			 uint64_t elf_have_interpreter, uint64_t interpreter_entry,
			 uint64_t elf_entry, uint64_t linker_base, void* program_header_address);

#endif /* !__ARCH_PROC_H__ */
