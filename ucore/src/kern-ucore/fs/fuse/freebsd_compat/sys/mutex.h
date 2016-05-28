#ifndef _FREEBSD_COMPAT_MUTEX_H_
#define	_FREEBSD_COMPAT_MUTEX_H_

#include "types.h"
#include <sem.h>
#include "cdefs.h"
#include "stddef.h"
#include <proc.h>

#define mtxlock2mtx(c)  (container_of(c, struct mtx, mtx_lock))

#define	mtx_init(m, n, t, o) _mtx_init(&(m)->mtx_lock, n, t, o)
#define	mtx_destroy(m) _mtx_destroy(&(m)->mtx_lock)
#define	mtx_lock(m) _mtx_lock(&(m)->mtx_lock)
#define	mtx_unlock(m) _mtx_unlock(&(m)->mtx_lock)
#define	mtx_assert(m, n) _mtx_assert(&(m)->mtx_lock, n)

/* Flags passed to witness_assert. */
#define	LA_MASKASSERT	0x000000ff	/* Mask for witness defined asserts. */
#define	LA_UNLOCKED	0x00000000	/* Lock is unlocked. */
#define	LA_LOCKED	0x00000001	/* Lock is at least share locked. */
#define	LA_SLOCKED	0x00000002	/* Lock is exactly share locked. */
#define	LA_XLOCKED	0x00000004	/* Lock is exclusively locked. */
#define	LA_RECURSED	0x00000008	/* Lock is recursed. */
#define	LA_NOTRECURSED	0x00000010	/* Lock is not recursed. */
#define LA_DESTROYED 0x00000020 /* Lock is destroyed */

#define MA_OWNED	LA_XLOCKED
#define MA_NOTOWNED	LA_UNLOCKED
#define MA_RECURSED	LA_RECURSED
#define MA_NOTRECURSED	LA_NOTRECURSED
#define MA_DESTROYED LA_DESTROYED

#define MTX_DEF         0x00000000      /* DEFAULT (sleep) lock */
#define MTX_SPIN        0x00000001      /* Spin lock (disables interrupts) */
#define MTX_RECURSE     0x00000004      /* Option: lock allowed to recurse */
#define MTX_NOWITNESS   0x00000008      /* Don't do any witness checking. */
#define MTX_NOPROFILE   0x00000020      /* Don't profile this lock */
#define MTX_NEW         0x00000040      /* Don't check for double-init */

struct lock_object;
struct mtx;

struct lock_object {

};

struct mtx {
  //struct lock_object lock_object;	/* Common lock properties. */
  volatile uintptr_t mtx_lock;	/* Owner and flags. */
  semaphore_t sem;    
  char *name;
};

static void _mtx_lock(volatile uintptr_t *c) {
    struct mtx *m;
    m = mtxlock2mtx(c);

    down(&(m->sem));
    m->mtx_lock = (uintptr_t)current;
}

static void _mtx_unlock(volatile uintptr_t *c) {
    struct mtx *m;
    m = mtxlock2mtx(c);

    if (m->mtx_lock == (uintptr_t)current) {
        m->mtx_lock = MA_NOTOWNED;
        up(&(m->sem));
    }
}

static void	_mtx_init(volatile uintptr_t *c, const char *name, const char *type, int opts) {
    struct mtx *m;
    m = mtxlock2mtx(c); 
    sem_init(&(m->sem), 1);
    m->mtx_lock = MA_NOTOWNED;
    m->name = name;
}

static void	_mtx_destroy(volatile uintptr_t *c) {
    struct mtx *m;
    m = mtxlock2mtx(c);
    
    m->mtx_lock = MA_DESTROYED;
}

static void	_mtx_assert(const volatile uintptr_t *c, int what) {
    struct mtx *m;
    m = mtxlock2mtx(c);
    switch (what) {
        case MA_OWNED:
            if (m->mtx_lock == MA_NOTOWNED || m->mtx_lock == MA_DESTROYED) {
                panic("mutex %s not owned",m->name);
            }
        break;
    }
}

#ifndef MPASS
#define MPASS(x) assert(x)
#endif

#endif
