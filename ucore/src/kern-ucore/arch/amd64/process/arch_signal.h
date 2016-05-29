#ifndef __ARCH_ARM_ARCH_SIGNAL_H
#define __ARCH_ARM_ARCH_SIGNAL_H
#include <signal.h>

struct sigframe {
	uintptr_t pretcode;
	int sign;
	struct trapframe tf;
	sigset_t old_blocked;
	//there's fpstate in linux, but nothing here
	unsigned char retcode[32];
};

#endif
