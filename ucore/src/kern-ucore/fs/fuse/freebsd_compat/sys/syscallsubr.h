#ifndef _FREEBSD_COMPAT_SYSCALLSUBR_H_
#define	_FREEBSD_COMPAT_SYSCALLSUBR_H_

#include "types.h"

struct thread;

static int kern_sigprocmask(struct thread *td, int how, sigset_t *set, sigset_t *oset, int old) {
  return 0;
}

#endif
