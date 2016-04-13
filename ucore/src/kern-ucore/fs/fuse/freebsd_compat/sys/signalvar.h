#ifndef _FREEBSD_COMPAT_SIGNALVAR_H_
#define	_FREEBSD_COMPAT_SIGNALVAR_H_

#include "types.h"

#define _SIG_WORDS      4
#define _SIG_MAXSIG     128
#define _SIG_IDX(sig)   ((sig) - 1)
#define _SIG_WORD(sig)  (_SIG_IDX(sig) >> 5)
#define _SIG_BIT(sig)   (1 << (_SIG_IDX(sig) & 31))
#define _SIG_VALID(sig) ((sig) <= _SIG_MAXSIG && (sig) > 0)

typedef struct __sigset {
        uint32_t __bits[_SIG_WORDS];
} __sigset_t;

typedef __sigset_t sigset_t;

#define SIG_BLOCK       1       /* block specified signal set */
#define SIG_UNBLOCK     2       /* unblock specified signal set */
#define SIG_SETMASK     3       /* set specified signal set */

#define SIGHUP          1       /* hangup */
#define SIGINT          2       /* interrupt */
#define SIGQUIT         3       /* quit */
#define SIGILL          4       /* illegal instr. (not reset when caught) */
#define SIGTRAP         5       /* trace trap (not reset when caught) */
#define SIGABRT         6       /* abort() */
#define SIGIOT          SIGABRT /* compatibility */
#define SIGEMT          7       /* EMT instruction */
#define SIGFPE          8       /* floating point exception */
#define SIGKILL         9       /* kill (cannot be caught or ignored) */
#define SIGBUS          10      /* bus error */
#define SIGSEGV         11      /* segmentation violation */
#define SIGSYS          12      /* non-existent system call invoked */
#define SIGPIPE         13      /* write on a pipe with no one to read it */
#define SIGALRM         14      /* alarm clock */
#define SIGTERM         15      /* software termination signal from kill */
#define SIGURG          16      /* urgent condition on IO channel */
#define SIGSTOP         17      /* sendable stop signal not from tty */
#define SIGTSTP         18      /* stop signal from tty */
#define SIGCONT         19      /* continue a stopped process */
#define SIGCHLD         20      /* to parent on child stop or exit */
#define SIGTTIN         21      /* to readers pgrp upon background tty read */
#define SIGTTOU         22      /* like TTIN if (tp->t_local&LTOSTOP) */
#define SIGIO           23      /* input/output possible signal */
#define SIGXCPU         24      /* exceeded CPU time limit */
#define SIGXFSZ         25      /* exceeded file size limit */
#define SIGVTALRM       26      /* virtual time alarm */
#define SIGPROF         27      /* profiling time alarm */
#define SIGWINCH        28      /* window size changes */
#define SIGINFO         29      /* information request */
#define SIGUSR1         30      /* user defined signal 1 */
#define SIGUSR2         31      /* user defined signal 2 */
#define SIGTHR          32      /* reserved by thread library. */
#define SIGLWP          SIGTHR
#define SIGLIBRT        33      /* reserved by real-time library. */

#define SIGFILLSET(set)                                                 \
        do {                                                            \
                int __i;                                                \
                for (__i = 0; __i < _SIG_WORDS; __i++)                  \
                        (set).__bits[__i] = ~0U;                        \
        } while (0)

#define SIGDELSET(set, signo)                                           \
        ((set).__bits[_SIG_WORD(signo)] &= ~_SIG_BIT(signo))

#endif
