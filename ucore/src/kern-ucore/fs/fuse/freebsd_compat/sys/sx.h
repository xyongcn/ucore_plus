#ifndef _FREEBSD_COMPAT_SX_H_
#define	_FREEBSD_COMPAT_SX_H_

#include "types.h"

struct sx {
	//struct lock_object	lock_object;
	volatile uintptr_t	sx_lock;
};

#ifndef MPASS
#define MPASS(x) assert(x)
#endif

static void sx_init(struct sx *sx, const char *description) {
}

static void sx_destroy(struct sx *sx) {
}

static void sx_xlock(struct sx *sx) {
}

static void sx_unlock(struct sx *sx) {
}

#endif
