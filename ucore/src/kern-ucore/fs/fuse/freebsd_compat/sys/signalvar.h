#ifndef _FREEBSD_COMPAT_SIGNALVAR_H_
#define	_FREEBSD_COMPAT_SIGNALVAR_H_

#include "signalvar.h"
#include "types.h"

#define _SIG_WORDS      2
#define _SIG_MAXSIG     128
#define _SIG_IDX(sig)   ((sig) - 1)
#define _SIG_WORD(sig)  (_SIG_IDX(sig) >> 10)
#define _SIG_BIT(sig)   (1 << (_SIG_IDX(sig) & 63))
#define _SIG_VALID(sig) ((sig) <= _SIG_MAXSIG && (sig) > 0)

typedef struct __sigset {
        uint64_t  __bits[_SIG_WORDS];
} __sigset_t;

typedef __sigset_t fuse_sigset_t;

typedef struct fuse_sigqueue {
	fuse_sigset_t	sq_signals;	/* All pending signals. */
	//sigset_t	sq_kill;	/* Legacy depth 1 queue. */
	//TAILQ_HEAD(, ksiginfo)	sq_list;/* Queued signal info. */
	//struct proc	*sq_proc;
	//int		sq_flags;
} fuse_sigqueue_t;

#define SIG_BLOCK       1       /* block specified signal set */
#define SIG_UNBLOCK     2       /* unblock specified signal set */
#define SIG_SETMASK     3       /* set specified signal set */

#define SIGKILL         9       /* kill (cannot be caught or ignored) */

#define SIGFILLSET(set)                                                 \
        do {                                                            \
                int __i;                                                \
                for (__i = 0; __i < _SIG_WORDS; __i++)                  \
                        (set).__bits[__i] = ~0U;                        \
        } while (0)

#define SIGDELSET(set, signo)                                           \
        ((set).__bits[_SIG_WORD(signo)] &= ~_SIG_BIT(signo))

static __inline int
__sigisempty(fuse_sigset_t *set)
{
        int i;

        for (i = 0; i < _SIG_WORDS; i++) {
                if (set->__bits[i])
                        return (0);
        }
        return (1);
}

#define SIGISEMPTY(set)         (__sigisempty(&(set)))
#define SIGNOTEMPTY(set)        (!__sigisempty(&(set)))

#endif
