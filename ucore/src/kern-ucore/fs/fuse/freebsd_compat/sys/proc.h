#ifndef _FREEBSD_COMPAT_PROC_H_
#define	_FREEBSD_COMPAT_PROC_H_

#include "priority.h"
#include "types.h"
#include "ucred.h"
#include "signalvar.h"

#define curthread __curthread()

struct proc;
struct thread;
struct file;

struct proc {
  pid_t	p_pid;
  fuse_sigqueue_t	p_sigqueue;	/* (c) Sigs not delivered to a td. */
  struct mtx p_mtx;
  #define p_siglist	p_sigqueue.sq_signals
};

struct thread {
  struct proc	*td_proc;	/* (*) Associated process. */
  lwpid_t td_tid;		/* (b) Thread ID. */
  struct ucred	*td_ucred;
  struct file* td_fpop;
  fuse_sigqueue_t	td_sigqueue;	/* (c) Sigs arrived, not delivered. */
#define	td_siglist	td_sigqueue.sq_signals
};

static struct thread* __curthread() {

}

/* Lock and unlock a process. */
#define PROC_LOCK(p)    mtx_lock(&(p)->p_mtx)
#define PROC_TRYLOCK(p) mtx_trylock(&(p)->p_mtx)
#define PROC_UNLOCK(p)  mtx_unlock(&(p)->p_mtx)
#define PROC_LOCKED(p)  mtx_owned(&(p)->p_mtx)
#define PROC_LOCK_ASSERT(p, type)       mtx_assert(&(p)->p_mtx, (type))

#endif
