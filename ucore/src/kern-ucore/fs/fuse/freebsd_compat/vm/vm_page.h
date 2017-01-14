#ifndef _FREEBSD_COMPAT_VM_PAGE_H_
#define	_FREEBSD_COMPAT_VM_PAGE_H_

#include "vm_object.h"

#if PAGE_SIZE == 4096
#define VM_PAGE_BITS_ALL 0xffu
typedef uint8_t vm_page_bits_t;
#elif PAGE_SIZE == 8192
#define VM_PAGE_BITS_ALL 0xffffu
typedef uint16_t vm_page_bits_t;
#elif PAGE_SIZE == 16384
#define VM_PAGE_BITS_ALL 0xffffffffu
typedef uint32_t vm_page_bits_t;
#elif PAGE_SIZE == 32768
#define VM_PAGE_BITS_ALL 0xfffffffffffffffflu
typedef uint64_t vm_page_bits_t;
#endif

//TODO: translate to uCore pages.
typedef struct vm_page {
  vm_object_t object;             /* which object am I in (O,P) */
  vm_pindex_t pindex;             /* offset into object (O,P) */
  uint8_t valid;           /* map of valid DEV_BSIZE chunks (O) */
  uint8_t dirty;           /* map of dirty DEV_BSIZE chunks (M) */
} *vm_page_t;

static void vm_page_set_valid_range(vm_page_t m, int base, int size) {
}

static void vm_page_undirty(vm_page_t m) {
}

#endif
