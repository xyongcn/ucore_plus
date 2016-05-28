#ifndef _FREEBSD_COMPAT_SX_H_
#define	_FREEBSD_COMPAT_SX_H_

#include "types.h"
#include <sem.h>
#include "cdefs.h"
#include "stddef.h"
#include "mutex.h"

struct sx {
	//struct lock_object	lock_object;i
    semaphore_t sem;
    char *description;
	volatile uintptr_t	sx_lock;
};

#ifndef MPASS
#define MPASS(x) assert(x)
#endif

static void sx_init(struct sx *sx, const char *description) {
    sem_init(&(sx->sem), 1);
    sx->description = description;
    sx->sx_lock = MA_NOTOWNED;
}

static void sx_destroy(struct sx *sx) {
    sx->sx_lock = MA_DESTROYED;
}

static void sx_xlock(struct sx *sx) {
    down(&(sx->sem));
    sx->sx_lock = (uintptr_t)current;
}

static void sx_unlock(struct sx *sx) {
    if (sx->sx_lock == (uintptr_t)current) {
        sx->sx_lock = MA_NOTOWNED;
        up(&(sx->sem));
    }
}

#endif
