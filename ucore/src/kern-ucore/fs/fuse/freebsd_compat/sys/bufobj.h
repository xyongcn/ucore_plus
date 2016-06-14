#ifndef	_FREEBSD_COMPAT_BUFOBJ_H_
#define	_FREEBSD_COMPAT_BUFOBJ_H_

struct ucred;

#define B_AGE           0x00000001      /* Move to age queue when I/O done. */
#define B_NEEDCOMMIT    0x00000002      /* Append-write in progress. */
#define B_ASYNC         0x00000004      /* Start I/O, do not wait. */
#define B_DIRECT        0x00000008      /* direct I/O flag (pls free vmio) */
#define B_DEFERRED      0x00000010      /* Skipped over for cleaning */
#define B_CACHE         0x00000020      /* Bread found us in the cache. */
#define B_VALIDSUSPWRT  0x00000040      /* Valid write during suspension. */
#define B_DELWRI        0x00000080      /* Delay I/O until buffer reused. */
#define B_PERSISTENT    0x00000100      /* Perm. ref'ed while EXT2FS mounted. */
#define B_DONE          0x00000200      /* I/O completed. */
#define B_EINTR         0x00000400      /* I/O was interrupted */
#define B_NOREUSE       0x00000800      /* Contents not reused once released. */
#define B_00001000      0x00001000      /* Available flag. */
#define B_INVAL         0x00002000      /* Does not contain valid info. */
#define B_BARRIER       0x00004000      /* Write this and all preceeding first. */
#define B_NOCACHE       0x00008000      /* Do not cache block after use. */
#define B_MALLOC        0x00010000      /* malloced b_data */
#define B_CLUSTEROK     0x00020000      /* Pagein op, so swap() can count it. */
#define B_00040000      0x00040000      /* Available flag. */
#define B_00080000      0x00080000      /* Available flag. */
#define B_00100000      0x00100000      /* Available flag. */
#define B_DIRTY         0x00200000      /* Needs writing later (in EXT2FS). */
#define B_RELBUF        0x00400000      /* Release VMIO buffer. */
#define B_FS_FLAG1      0x00800000      /* Available flag for FS use. */
#define B_NOCOPY        0x01000000      /* Don't copy-on-write this buf. */
#define B_INFREECNT     0x02000000      /* buf is counted in numfreebufs */
#define B_PAGING        0x04000000      /* volatile paging I/O -- bypass VMIO */
#define B_MANAGED       0x08000000      /* Managed by FS. */
#define B_RAM           0x10000000      /* Read ahead mark (flag) */
#define B_VMIO          0x20000000      /* VMIO flag */
#define B_CLUSTER       0x40000000      /* pagein op, so swap() can count it */
#define B_REMFREE       0x80000000      /* Delayed bremfree */

//buf.h
static int nswbuf = 10;                 /* Number of swap I/O buffer headers. */

/*
 * The buffer header describes an I/O operation in the kernel.
 *
 * NOTES:
 *      b_bufsize, b_bcount.  b_bufsize is the allocation size of the
 *      buffer, either DEV_BSIZE or PAGE_SIZE aligned.  b_bcount is the
 *      originally requested buffer size and can serve as a bounds check
 *      against EOF.  For most, but not all uses, b_bcount == b_bufsize.
 *
 *      b_dirtyoff, b_dirtyend.  Buffers support piecemeal, unaligned
 *      ranges of dirty data that need to be written to backing store.
 *      The range is typically clipped at b_bcount ( not b_bufsize ).
 *
 *      b_resid.  Number of bytes remaining in I/O.  After an I/O operation
 *      completes, b_resid is usually 0 indicating 100% success.
 *
 *      All fields are protected by the buffer lock except those marked:
 *              V - Protected by owning bufobj lock
 *              Q - Protected by the buf queue lock
 *              D - Protected by an dependency implementation specific lock
 */
struct buf {
        //struct bufobj   *b_bufobj;
        long            b_bcount;
        //void            *b_caller1;
        caddr_t         b_data;
        int             b_error;
        uint8_t         b_iocmd;
        uint8_t         b_ioflags;
        //off_t           b_iooffset;
        long            b_resid;
        //void    (*b_iodone)(struct buf *);
        daddr_t b_blkno;                /* Underlying physical block number. */
        //off_t   b_offset;               /* Offset into file. */
        //TAILQ_ENTRY(buf) b_bobufs;      /* (V) Buffer's associated vnode. */
        //uint32_t        b_vflags;       /* (V) BV_* flags */
        //unsigned short b_qindex;        /* (Q) buffer queue index */
        uint32_t        b_flags;        /* B_* flags. */
        //b_xflags_t b_xflags;            /* extra flags */
        //struct lock b_lock;             /* Buffer lock */
        //long    b_bufsize;              /* Allocated buffer size. */
        //int     b_runningbufspace;      /* when I/O is running, pipelining */
        //int     b_kvasize;              /* size of kva for buffer */
        int     b_dirtyoff;             /* Offset in buffer of dirty region. */
        int     b_dirtyend;             /* Offset of end of dirty region. */
        //caddr_t b_kvabase;              /* base kva for buffer */
        //daddr_t b_lblkno;               /* Logical block number. */
        //struct  vnode *b_vp;            /* Device vnode. */
        struct  ucred *b_rcred;         /* Read credentials reference. */
        struct  ucred *b_wcred;         /* Write credentials reference. */
        //union {
        //        TAILQ_ENTRY(buf) b_freelist; /* (Q) */
        //        struct {
        //                void    (*b_pgiodone)(void *, vm_page_t *, int, int);
        //                int     b_pgbefore;
        //                int     b_pgafter;
        //        };
        //};
        //union   cluster_info {
        //        TAILQ_HEAD(cluster_list_head, buf) cluster_head;
        //        TAILQ_ENTRY(buf) cluster_entry;
        //} b_cluster;
        //struct  vm_page *b_pages[btoc(MAXPHYS)];
        //int             b_npages;
        //struct  workhead b_dep;         /* (D) List of filesystem dependencies. */
        //void    *b_fsprivate1;
        //void    *b_fsprivate2;
        //void    *b_fsprivate3;
        //int     b_pin_count;
};

//
struct bufobj {
	//struct rwlock	bo_lock;	/* Lock which protects "i" things */
	//struct buf_ops	*bo_ops;	/* - Buffer operations */
	struct vm_object *bo_object;	/* v Place to store VM object */
	//LIST_ENTRY(bufobj) bo_synclist;	/* S dirty vnode list */
	//void		*bo_private;	/* private pointer */
	//struct vnode	*__bo_vnode;	/*
	//				 * XXX: This vnode pointer is here
	//				 * XXX: only to keep the syncer working
	//				 * XXX: for now.
	//				 */
	//struct bufv	bo_clean;	/* i Clean buffers */
	//struct bufv	bo_dirty;	/* i Dirty buffers */
	//long		bo_numoutput;	/* i Writes in progress */
	//u_int		bo_flag;	/* i Flags */
	//int		bo_bsize;	/* - Block size for i/o */
};

static struct buf *getpbuf(int *pfreecnt) {
  return NULL;
}

static void relpbuf(struct buf *bp, int *pfreecnt) {
}

static void bufdone(struct buf *bp) {
}

static void brelse(struct buf *bp) {
}

static int bwrite(struct buf *bp) {
  return 0;
}

static void bdirty(struct buf *bp) {
}

static struct buf *getblk(struct vnode *vp, daddr_t blkno, int size, int slpflag, int slptimeo,
int flags) {
  return NULL;
}

static int allocbuf(struct buf *bp, int size) {
  return 0;
}

#endif
