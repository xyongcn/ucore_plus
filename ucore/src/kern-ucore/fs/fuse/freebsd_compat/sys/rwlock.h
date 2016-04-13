#ifndef _FREEBSD_COMPAT_RWLOCK_H_
#define	_FREEBSD_COMPAT_RWLOCK_H_

struct rwlock {
        //struct lock_object      lock_object;
        volatile uintptr_t      rw_lock;
};

static void rw_wlock(struct rwlock* lock) {
}

static void rw_wunlock(struct rwlock* lock) {
}

#endif
