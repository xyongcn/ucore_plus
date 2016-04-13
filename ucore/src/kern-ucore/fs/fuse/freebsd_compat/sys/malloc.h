#ifndef _FREEBSD_COMPAT_MALLOC_H_
#define	_FREEBSD_COMPAT_MALLOC_H_

#include "types.h"

#define M_NOWAIT        0x0001          /* do not block */
#define M_WAITOK        0x0002          /* ok to block */
#define M_ZERO          0x0100          /* bzero the allocation */
#define M_NOVM          0x0200          /* don't ask VM for pages */
#define M_USE_RESERVE   0x0400          /* can alloc out of reserve memory */
#define M_NODUMP        0x0800          /* don't dump pages in this allocation */
#define M_FIRSTFIT      0x1000          /* Only for vmem, fast fit. */
#define M_BESTFIT       0x2000          /* Only for vmem, low fragmentation. */

#define M_MAGIC         877983977       /* time when first defined :-) */

struct malloc_type {
        struct malloc_type *ks_next;    /* Next in global chain. */
        u_long           ks_magic;      /* Detect programmer error. */
        const char      *ks_shortdesc;  /* Printable type name. */
        void            *ks_handle;     /* Priv. data, was lo_class. */
};

//TODO : simple ignoring those macros might not work.
#define MALLOC_DEFINE(type, shortdesc, longdesc)
#define malloc(x, y, z) __malloc(x, z)
#define realloc(x, y, z, w) __realloc(x, y, w)
#define free(x, y) __free(x)

static void* __malloc(int size, int flags) {
  return NULL;
}

static void* __realloc(void* ptr, int size, int flags) {
  return NULL;
}

static void* __free(void* ptr) {
  return NULL;
}

#endif
