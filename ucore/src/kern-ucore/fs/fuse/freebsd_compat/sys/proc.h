#ifndef _FREEBSD_COMPAT_PROC_H_
#define	_FREEBSD_COMPAT_PROC_H_

#include "priority.h"
#include "types.h"
#include "ucred.h"

#define curthread __curthread()

struct proc;
struct thread;

struct proc {
  pid_t	p_pid;
};

struct thread {
  struct proc	*td_proc;	/* (*) Associated process. */
  lwpid_t td_tid;		/* (b) Thread ID. */
  struct ucred	*td_ucred;
};

static struct thread* __curthread() {

}

#endif
