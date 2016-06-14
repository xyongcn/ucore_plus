#ifndef _FREEBSD_COMPAT_SYSCALLSUBR_H_
#define	_FREEBSD_COMPAT_SYSCALLSUBR_H_

#include "types.h"
#include <signal.h>

struct thread;

static int kern_sigprocmask(struct thread *td, int how, fuse_sigset_t *set, fuse_sigset_t *oset, int old) {
  return do_sigprocmask(how, set->__bits[0], oset->__bits[0]);
}

#endif
