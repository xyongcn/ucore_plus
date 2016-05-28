#ifndef _FREEBSD_COMPAT_RWLOCK_H_
#define	_FREEBSD_COMPAT_RWLOCK_H_

#include <sem.h>
#include "stddef.h"
#include "types.h"
#include "mutex.h"
#include "cdefs.h"

struct rwlock {
        semaphore_t sem;
        //struct lock_object      lock_object;
        volatile uintptr_t      rw_lock;
};

static void rw_wlock(struct rwlock* lock) {
    down(&(lock->sem));
    lock->rw_lock = (uintptr_t)current;
}

static void rw_wunlock(struct rwlock* lock) {
    if (lock->rw_lock == (uintptr_t)current) {
        lock->rw_lock = MA_NOTOWNED;
        up(&(lock->sem));
    }
}

#endif
