#ifndef	_FREEBSD_COMPAT_REFCOUNT_H_
#define	_FREEBSD_COMPAT_REFCOUNT_H_

#include "atomic.h"

static __inline void
refcount_init(volatile u_int *count, u_int value)
{
        *count = value;
}

static __inline void
refcount_acquire(volatile u_int *count)
{
        //KASSERT(*count < UINT_MAX, ("refcount %p overflowed", count));
        atomic_add_acq_int(count, 1);
}

static __inline int
refcount_release(volatile u_int *count)
{
        u_int old;
        /* XXX: Should this have a rel membar? */
        old = atomic_fetchadd_int(count, -1);
        //KASSERT(old > 0, ("negative refcount %p", count));
        return (old == 1);
}

#endif
