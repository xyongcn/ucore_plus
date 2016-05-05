#ifndef	_FREEBSD_COMPAT_ATOMIC_H_
#define	_FREEBSD_COMPAT_ATOMIC_H_

#include <atomic.h>

/*
 * TODO: This porting is based on the source code of FreeBSD 7.1, in more recent
 * version of FreeBSD, atomic_add_acq_int is the alias for atomic_add_barr_int,
 * whose declaration cannot be find (may be it's in generated code?)
 * Better to check again.
 */

#define TYPE_SIZE_EQUAL_ASSERT(type1, type2) \
  ((void)sizeof(char[1 - 2*!!(sizeof(type1) - sizeof(type2))]))

#define atomic_add_acq_int atomic_add_int
#define atomic_subtract_acq_int atomic_subtract_int
#define atomic_add_acq_long atomic_add_long

static inline void atomic_add_int(volatile int *p, int v)
  __attribute__ ((always_inline));

static inline void atomic_subtract_int(volatile int *p, int v)
  __attribute__ ((always_inline));

static inline int atomic_fetchadd_int(volatile int *p, int v)
  __attribute__ ((always_inline));

static inline void atomic_add_long(volatile long *p, int v)
  __attribute__ ((always_inline));

static inline unsigned long atomic_fetchadd_long(volatile unsigned long *p, unsigned long v)
  __attribute__ ((always_inline));

static inline void atomic_add_int(volatile int *p, int v)
{
  TYPE_SIZE_EQUAL_ASSERT(int, atomic_t);
  atomic_add((atomic_t*)p, v);
}

static inline void atomic_subtract_int(volatile int *p, int v)
{
  TYPE_SIZE_EQUAL_ASSERT(int, atomic_t);
  atomic_sub((atomic_t*)p, v);
}

/*
 *Seems the @atomic_add_return function doesn't do the same thing as
 * @atomic_fetchadd_int, so atomic_add_return_orig is created.
 */
static inline int atomic_fetchadd_int(volatile int *p, int v)
{
  TYPE_SIZE_EQUAL_ASSERT(int, atomic_t);
  return atomic_add_return_orig((atomic_t*)p, v);
}

/*
 * TODO: long type on gcc x86_64 platform is 64-bit and currently is not
 * supported by uCore! The current way we handle this DOESNOT guarantee
 * atomic operation by assembly.
 */

static inline void atomic_add_long(volatile long *p, int v) {
  kprintf("TODO! FreeBSD-compat: atomic_add_long doesn't guarantee atomic "
  "assembly-level operation!");
  (*p) += v;
}

static inline unsigned long atomic_fetchadd_long(volatile unsigned long *p, unsigned long v) {
  kprintf("TODO! FreeBSD-compat: atomic_fetchadd_long doesn't guarantee atomic "
  "assembly-level operation!");
  unsigned long ret = *p;
  (*p) += v;
  return ret;
}

#endif
