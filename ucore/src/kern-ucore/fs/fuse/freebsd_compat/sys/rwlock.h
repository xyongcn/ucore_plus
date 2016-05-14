#ifndef _FREEBSD_COMPAT_RWLOCK_H_
#define	_FREEBSD_COMPAT_RWLOCK_H_

#include <sem.h>
#include "stddef.h"
#include "types.h"
#include "mutex.h"
#include "cdefs.h"

struct rwlock {
        rwlock() { 
            rw_lock = MA_NOTOWNED;
            sem_init(&(sem), 1);
        }


        semaphore_t sem;
        //struct lock_object      lock_object;
        volatile uintptr_t      rw_lock;
};

static void rw_wlock(struct rwlock* lock) {
    down(&(lock->sem));
    lock->rw_lock = (uintptr_t)curthread;
}

static void rw_wunlock(struct rwlock* lock) {
    if (lock->rw_lock == (uintptr_t)curthread) {
        lock->rw_lock = NOTOWNED;
        up(&(lock->sem));
    }
}

#endif
