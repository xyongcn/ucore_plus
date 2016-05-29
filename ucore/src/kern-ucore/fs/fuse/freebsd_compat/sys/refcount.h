#ifndef	_FREEBSD_COMPAT_REFCOUNT_H_
#define	_FREEBSD_COMPAT_REFCOUNT_H_

#include <atomic.h>

typedef unsigned int uint_t;

static inline void refcount_init(volatile uint_t *count, uint_t value)
  __attribute__ ((always_inline));
static __inline void refcount_acquire(volatile uint_t *count)
  __attribute__ ((always_inline));
static __inline int refcount_release(volatile uint_t *count)
  __attribute__ ((always_inline));

static inline void refcount_init(volatile uint_t *count, uint_t value)
{
  *count = value;
}

static inline void refcount_acquire(volatile uint_t *count)
{
  atomic_inc((atomic_t*)count);
}

static inline int refcount_release(volatile uint_t *count)
{
  uint_t old;
  old = atomic_add_return_orig((atomic_t*)count, -1);
  return (old == 1);
}

#endif
