#ifndef	_FREEBSD_COMPAT_ATOMIC_H_
#define	_FREEBSD_COMPAT_ATOMIC_H_

#define atomic_add_acq_int atomic_add_int
#define atomic_subtract_acq_int atomic_subtract_int
#define atomic_add_acq_long atomic_add_long
#define atomic_subtract_acq_long atomic_subtract_long

static void atomic_add_int(volatile long *p, long v) {
}

static void atomic_subtract_int(volatile long *p, long v) {
}

static void atomic_add_long(volatile int *p, int v) {
}

static void atomic_subtract_long(volatile int *p, int v) {
}

static unsigned long atomic_fetchadd_long(volatile unsigned long *p, unsigned long v) {
  return 0;
}

static int atomic_fetchadd_int(volatile int *p, int v) {
  return 0;
}

#endif
