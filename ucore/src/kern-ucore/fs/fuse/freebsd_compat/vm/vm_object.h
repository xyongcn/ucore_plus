#ifndef _FREEBSD_COMPAT_VM_OBJECT_H_
#define	_FREEBSD_COMPAT_VM_OBJECT_H_

#include "../sys/rwlock.h"

/*
 *	Types defined:
 *
 *	vm_object_t		Virtual memory object.
 *
 *	The root of cached pages pool is protected by both the per-object lock
 *	and the free pages queue mutex.
 *	On insert in the cache radix trie, the per-object lock is expected
 *	to be already held and the free pages queue mutex will be
 *	acquired during the operation too.
 *	On remove and lookup from the cache radix trie, only the free
 *	pages queue mutex is expected to be locked.
 *	These rules allow for reliably checking for the presence of cached
 *	pages with only the per-object lock held, thereby reducing contention
 *	for the free pages queue mutex.
 *
 * List of locks
 *	(c)	const until freed
 *	(o)	per-object lock
 *	(f)	free pages queue mutex
 *
 */

typedef struct vm_object {
	struct rwlock lock;
	//TAILQ_ENTRY(vm_object) object_list; /* list of all objects */
	//LIST_HEAD(, vm_object) shadow_head; /* objects that this is a shadow for */
	//LIST_ENTRY(vm_object) shadow_list; /* chain of shadow objects */
	//TAILQ_HEAD(respgs, vm_page) memq; /* list of resident pages */
	//struct vm_radix rtree;		/* root of the resident page radix trie*/
	//vm_pindex_t size;		/* Object size */
	//int generation;			/* generation ID */
	//int ref_count;			/* How many refs?? */
	//int shadow_count;		/* how many objects that this is a shadow for */
	//vm_memattr_t memattr;		/* default memory attribute for pages */
	//objtype_t type;			/* type of pager */
	//u_short flags;			/* see below */
	//u_short pg_color;		/* (c) color of first page in obj */
	//u_int paging_in_progress;	/* Paging (in or out) so don't collapse or destroy */
	//int resident_page_count;	/* number of resident pages */
	//struct vm_object *backing_object; /* object that I'm a shadow of */
	//vm_ooffset_t backing_object_offset;/* Offset in backing object */
	//TAILQ_ENTRY(vm_object) pager_object_list; /* list of all objects of this pager type */
	//LIST_HEAD(, vm_reserv) rvq;	/* list of reservations */
	//struct vm_radix cache;		/* (o + f) root of the cache page radix trie */
	//void *handle;
	union {
	//	/*
	//	 * VNode pager
	//	 *
	//	 *	vnp_size - current size of file
	//	 */
		struct {
			off_t vnp_size;
			vm_ooffset_t writemappings;
		} vnp;

	//	/*
	//	 * Device pager
	//	 *
	//	 *	devp_pglist - list of allocated pages
	//	 */
	//	struct {
	//		TAILQ_HEAD(, vm_page) devp_pglist;
	//		struct cdev_pager_ops *ops;
	//		struct cdev *dev;
	//	} devp;

	//	/*
	//	 * SG pager
	//	 *
	//	 *	sgp_pglist - list of allocated pages
	//	 */
	//	struct {
	//		TAILQ_HEAD(, vm_page) sgp_pglist;
	//	} sgp;

	//	/*
	//	 * Swap pager
	//	 *
	//	 *	swp_tmpfs - back-pointer to the tmpfs vnode,
	//	 *		     if any, which uses the vm object
	//	 *		     as backing store.  The handle
	//	 *		     cannot be reused for linking,
	//	 *		     because the vnode can be
	//	 *		     reclaimed and recreated, making
	//	 *		     the handle changed and hash-chain
	//	 *		     invalid.
	//	 *
	//	 *	swp_bcount - number of swap 'swblock' metablocks, each
	//	 *		     contains up to 16 swapblk assignments.
	//	 *		     see vm/swap_pager.h
	//	 */
	//	struct {
	//		void *swp_tmpfs;
	//		int swp_bcount;
	//	} swp;
	} un_pager;
	//struct ucred *cred;
	//vm_ooffset_t charge;
} *vm_object_t;

#define OBJPC_SYNC      0x1                     /* sync I/O */
#define OBJPC_INVAL     0x2                     /* invalidate */
#define OBJPC_NOSYNC    0x4                     /* skip if PG_NOSYNC */

#define VM_OBJECT_WLOCK(object)                                         \
        rw_wlock(&(object)->lock)
#define VM_OBJECT_WUNLOCK(object)                                       \
        rw_wunlock(&(object)->lock)

#define IDX_TO_OFF(idx) (((vm_ooffset_t)(idx)) << PAGE_SHIFT)

static bool vm_object_page_clean(vm_object_t object, vm_ooffset_t start, vm_ooffset_t end, int flags) {
  return 0;
}

#endif
