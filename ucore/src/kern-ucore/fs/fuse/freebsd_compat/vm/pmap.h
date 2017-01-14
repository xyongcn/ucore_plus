#ifndef _FREEBSD_COMPAT_PMAP_H_
#define	_FREEBSD_COMPAT_PMAP_H_

#include "types.h"

struct pmap {
};

typedef struct pmap *pmap_t;

void pmap_qenter(vm_offset_t va, vm_page_t *m, int count) {
}

void pmap_remove(pmap_t pmap, vm_offset_t sva, vm_offset_t eva) {
}

void pmap_qremove(vm_offset_t sva, int count) {
}

#endif
