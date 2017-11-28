#ifndef __KERN_MM_VMM_H__
#define __KERN_MM_VMM_H__

#ifndef __LINUX_GLUE__
#include <types.h>
#include <list.h>
#include <memlayout.h>
#include <rb_tree.h>
#include <sync.h>
#include <shmem.h>
#include <atomic.h>
#include <sem.h>
#endif

//pre define
struct mm_struct;

#ifdef UCONFIG_BIONIC_LIBC
struct mapped_file_struct {
	struct file *file;
	off_t offset;
};
#endif //UCONFIG_BIONIC_LIBC

// the virtual continuous memory area(vma), [vm_start, vm_end),
// addr belong to a vma means  vma.vm_start<= addr <vma.vm_end
struct vma_struct {
	struct mm_struct *vm_mm; // the set of vma using the same PDT
	uintptr_t vm_start;      // start addr of vma
	uintptr_t vm_end;        // end addr of vma, not include the vm_end itself
	uint32_t vm_flags;	// flags of vma
	rb_node rb_link;	// redblack link which sorted by start addr of vma
	list_entry_t list_link;	// linear list link which sorted by start addr of vma
	struct shmem_struct *shmem;
	size_t shmem_off;
#ifdef UCONFIG_BIONIC_LIBC
	struct mapped_file_struct mfile;
#endif				//UCONFIG_BIONIC_LIBC
};

#define le2vma(le, member)                  \
    to_struct((le), struct vma_struct, member)

#define rbn2vma(node, member)               \
    to_struct((node), struct vma_struct, member)

#define VM_READ                 0x00000001
#define VM_WRITE                0x00000002
#define VM_EXEC                 0x00000004
#define VM_STACK                0x00000008
#define VM_SHARE                0x00000010

#define VM_ANONYMOUS			0x00000020

/* must the same as Linux */
#define VM_IO           0x00004000

#define MAP_FAILED      ((void*)-1)

#ifndef ARCH_MIPS
#define MAP_SHARED      0x01	/* Share changes */
#define MAP_PRIVATE     0x02	/* Changes are private */
#define MAP_TYPE        0x0f	/* Mask for type of mapping */
#define MAP_FIXED       0x10	/* Interpret addr exactly */
#define MAP_ANONYMOUS   0x20	/* don't use a file */

#define MAP_STACK		0x20000

#define PROT_READ       0x1	/* page can be read */
#define PROT_WRITE      0x2	/* page can be written */
#define PROT_EXEC       0x4	/* page can be executed */
#define PROT_SEM        0x8	/* page may be used for atomic ops */
#define PROT_NONE       0x0	/* page can not be accessed */
#define PROT_GROWSDOWN  0x01000000	/* mprotect flag: extend change to start of growsdown vma */
#define PROT_GROWSUP    0x02000000	/* mprotect flag: extend change to end of growsup vma */

#else //ARCH_MIPS

#define PROT_NONE       0x00            /* page can not be accessed */
#define PROT_READ       0x01            /* page can be read */
#define PROT_WRITE      0x02            /* page can be written */
#define PROT_EXEC       0x04            /* page can be executed */
/*                      0x08               reserved for PROT_EXEC_NOFLUSH */
#define PROT_SEM        0x10            /* page may be used for atomic ops */
#define PROT_GROWSDOWN  0x01000000      /* mprotect flag: extend change to start of growsdown vma */
#define PROT_GROWSUP    0x02000000      /* mprotect flag: extend change to end of growsup vma */

/*
 * Flags for mmap
 */
#define MAP_SHARED      0x001           /* Share changes */
#define MAP_PRIVATE     0x002           /* Changes are private */
#define MAP_TYPE        0x00f           /* Mask for type of mapping */
#define MAP_FIXED       0x010           /* Interpret addr exactly */

/* not used by linux, but here to make sure we don't clash with ABI defines */
#define MAP_RENAME      0x020           /* Assign page to file */
#define MAP_AUTOGROW    0x040           /* File may grow by writing */
#define MAP_LOCAL       0x080           /* Copy on fork/sproc */
#define MAP_AUTORSRV    0x100           /* Logical swap reserved on demand */

/* These are linux-specific */
#define MAP_NORESERVE   0x0400          /* don't check for reservations */
#define MAP_ANONYMOUS   0x0800          /* don't use a file */
#define MAP_GROWSDOWN   0x1000          /* stack-like segment */
#define MAP_DENYWRITE   0x2000          /* ETXTBSY */
#define MAP_EXECUTABLE  0x4000          /* mark it as an executable */
#define MAP_LOCKED      0x8000          /* pages are locked */
#define MAP_POPULATE    0x10000         /* populate (prefault) pagetables */
#define MAP_NONBLOCK    0x20000         /* do not block on IO */
#define MAP_STACK       0x40000         /* give out an address that is best suited for process/thread stacks */
#define MAP_HUGETLB     0x80000         /* create a huge page mapping */

/*
 * Flags for msync
 */
#define MS_ASYNC        0x0001          /* sync memory asynchronously */
#define MS_INVALIDATE   0x0002          /* invalidate mappings & caches */
#define MS_SYNC         0x0004          /* synchronous memory sync */

/*
 * Flags for mlockall
 */
#define MCL_CURRENT     1               /* lock all current mappings */
#define MCL_FUTURE      2               /* lock all future mappings */
#define MCL_ONFAULT     4               /* lock all pages that are faulted in */

/*
 * Flags for mlock
 */
#define MLOCK_ONFAULT   0x01            /* Lock pages in range after they are faulted in, do not prefault */

#define MADV_NORMAL     0               /* no further special treatment */
#define MADV_RANDOM     1               /* expect random page references */
#define MADV_SEQUENTIAL 2               /* expect sequential page references */
#define MADV_WILLNEED   3               /* will need these pages */
#define MADV_DONTNEED   4               /* don't need these pages */

/* common parameters: try to keep these consistent across architectures */
#define MADV_FREE       8               /* free pages only if memory pressure */
#define MADV_REMOVE     9               /* remove these pages & resources */
#define MADV_DONTFORK   10              /* don't inherit across fork */
#define MADV_DOFORK     11              /* do inherit across fork */

#define MADV_MERGEABLE   12             /* KSM may merge identical pages */
#define MADV_UNMERGEABLE 13             /* KSM may not merge identical pages */
#define MADV_HWPOISON    100            /* poison a page for testing */

#define MADV_HUGEPAGE   14              /* Worth backing with hugepages */
#define MADV_NOHUGEPAGE 15              /* Not worth backing with hugepages */

#define MADV_DONTDUMP   16              /* Explicity exclude from the core dump,
                                            overrides the coredump filter bits */
#define MADV_DODUMP     17              /* Clear the MADV_NODUMP flag */

/* compatibility flags */
#define MAP_FILE        0

/*
 * When MAP_HUGETLB is set bits [26:31] encode the log2 of the huge page size.
 * This gives us 6 bits, which is enough until someone invents 128 bit address
 * spaces.
 *
 * Assume these are all power of twos.
 * When 0 use the default page size.
 */
#define MAP_HUGE_SHIFT  26
#define MAP_HUGE_MASK   0x3f

#endif //ARCH_MIPS

struct mm_struct {
	list_entry_t mmap_list;
	rb_tree *mmap_tree;
	struct vma_struct *mmap_cache;
	pgd_t *pgdir;
#ifdef ARCH_ARM
	//ARM PDT is 16K alignment
	//but our allocator do not support
	//this sort of allocation
	pgd_t *pgdir_alloc_addr;
#endif
	int map_count;
	uintptr_t swap_address;
	atomic_t mm_count;
	int locked_by;
	uintptr_t brk_start, brk;
	list_entry_t proc_mm_link;
	semaphore_t mm_sem;
};

void lock_mm(struct mm_struct *mm);
void unlock_mm(struct mm_struct *mm);
bool try_lock_mm(struct mm_struct *mm);

#define le2mm(le, member)                   \
    to_struct((le), struct mm_struct, member)

#define RB_MIN_MAP_COUNT        32	// If the count of vma >32 then redblack tree link is used

struct vma_struct *find_vma(struct mm_struct *mm, uintptr_t addr);
struct vma_struct *find_vma_intersection(struct mm_struct *mm, uintptr_t start,
					 uintptr_t end);
struct vma_struct *vma_create(uintptr_t vm_start, uintptr_t vm_end,
			      uint32_t vm_flags);
void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma);

struct mm_struct *mm_create(void);
void mm_destroy(struct mm_struct *mm);

void vmm_init(void);
int mm_map(struct mm_struct *mm, uintptr_t addr, size_t len, uint32_t vm_flags,
	   struct vma_struct **vma_store);
int mm_map_shmem(struct mm_struct *mm, uintptr_t addr, uint32_t vm_flags,
		 struct shmem_struct *shmem, struct vma_struct **vma_store);
int mm_unmap(struct mm_struct *mm, uintptr_t addr, size_t len);
int dup_mmap(struct mm_struct *to, struct mm_struct *from);
void exit_mmap(struct mm_struct *mm);
uintptr_t get_unmapped_area(struct mm_struct *mm, size_t len);
int mm_brk(struct mm_struct *mm, uintptr_t addr, size_t len);

int do_pgfault(struct mm_struct *mm, machine_word_t error_code, uintptr_t addr);
bool user_mem_check(struct mm_struct *mm, uintptr_t start, size_t len,
		    bool write);

size_t user_mem_check_size(struct mm_struct *mm, uintptr_t start,
			   size_t len, bool write);

bool copy_from_user(struct mm_struct *mm, void *dst, const void *src,
		    size_t len, bool writable);
bool copy_to_user(struct mm_struct *mm, void *dst, const void *src, size_t len);
bool copy_string(struct mm_struct *mm, char *dst, const char *src, size_t maxn);

static inline int mm_count(struct mm_struct *mm)
{
	return atomic_read(&(mm->mm_count));
}

static inline void set_mm_count(struct mm_struct *mm, int val)
{
	atomic_set(&(mm->mm_count), val);
}

static inline int mm_count_inc(struct mm_struct *mm)
{
	return atomic_add_return(&(mm->mm_count), 1);
}

static inline int mm_count_dec(struct mm_struct *mm)
{
	return atomic_sub_return(&(mm->mm_count), 1);
}

struct mapped_addr {
	struct Page *page;
	off_t offset;
	list_entry_t list;
};

#define le2maddr(le)	\
		to_struct((le), struct mapped_addr, list)

#endif /* !__KERN_MM_VMM_H__ */
