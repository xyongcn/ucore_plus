#ifndef _FREEBSD_COMPAT_SYSTM_H_
#define	_FREEBSD_COMPAT_SYSTM_H_

#include <assert.h>
#include "types.h"
#include "mutex.h"
#include "atomic.h"

#ifndef PAGE_SIZE

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1UL << PAGE_SHIFT)
#define PAGE_MASK		(~(PAGE_SIZE-1))

#endif

#define hz 1000000

#define KASSERT(x, y)                                       \
    do {                                                \
        if (!(x)) {                                     \
            kprintf("assertion failed: %s", #y);         \
            panic("assertion failed: %s", #x);          \
        }                                               \
    } while (0)

static void wakeup(void *chan) {
}

static void wakeup_one(void *chan) {

}

static int msleep(void* chan, struct mtx* mtx, int pri, char* wmesg, int timo) {
  return 0;
}

static int tsleep(void *ident, int priority, const char *wmesg, sbintime_t sbt) {
  return 0;
}

static void bzero(void *buf, size_t len) {

}

static __inline int imax(int a, int b) { return (a > b ? a : b); }
static __inline int imin(int a, int b) { return (a < b ? a : b); }
static __inline long lmax(long a, long b) { return (a > b ? a : b); }
static __inline long lmin(long a, long b) { return (a < b ? a : b); }
static __inline u_int max(u_int a, u_int b) { return (a > b ? a : b); }
static __inline u_int min(u_int a, u_int b) { return (a < b ? a : b); }
static __inline quad_t qmax(quad_t a, quad_t b) { return (a > b ? a : b); }
static __inline quad_t qmin(quad_t a, quad_t b) { return (a < b ? a : b); }
static __inline u_long ulmax(u_long a, u_long b) { return (a > b ? a : b); }
static __inline u_long ulmin(u_long a, u_long b) { return (a < b ? a : b); }
static __inline off_t omax(off_t a, off_t b) { return (a > b ? a : b); }
static __inline off_t omin(off_t a, off_t b) { return (a < b ? a : b); }

//TODO: This won't work, these MACROs are related to uCore SMP implementation
#define PCPU_ADD(x, y)
#define PCPU_INC(x)

#endif
