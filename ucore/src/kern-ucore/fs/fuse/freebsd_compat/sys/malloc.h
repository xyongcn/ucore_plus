#ifndef _FREEBSD_COMPAT_MALLOC_H_
#define	_FREEBSD_COMPAT_MALLOC_H_

#include <mm/slab.h>

#define M_NOWAIT        0x0001          /* do not block */
#define M_WAITOK        0x0002          /* ok to block */
#define M_ZERO          0x0100          /* bzero the allocation */
#define M_NOVM          0x0200          /* don't ask VM for pages */
#define M_USE_RESERVE   0x0400          /* can alloc out of reserve memory */
#define M_NODUMP        0x0800          /* don't dump pages in this allocation */
#define M_FIRSTFIT      0x1000          /* Only for vmem, fast fit. */
#define M_BESTFIT       0x2000          /* Only for vmem, low fragmentation. */

#define M_MAGIC         877983977       /* time when first defined :-) */

/*
 * FreeBSD introduces "type" for memory allocation for memory usage statistics.
 * But we will simply ignore them.
 */
#define MALLOC_DEFINE(type, shortdesc, longdesc)
#define malloc(size, type, flags) __malloc(size, flags)
#define realloc(addr, size, type, flags) __realloc(addr, size, flags)
#define free(addr, type) __free(addr)

static void* __malloc(unsigned long size, int flags) {
  kprintf("TODO! FreeBSD-compat: __malloc flags are ignored.\r\n");
  return kmalloc(size);
}

static void* __realloc(void* ptr, unsigned long size, int flags) {
  panic("TODO! FreeBSD-compat: realloc not implemented.\r\n");
  return NULL;
}

static void __free(void* ptr) {
  kfree(ptr);
}

#endif
