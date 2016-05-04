#ifndef _FREEBSD_COMPAT_VNODE_H_
#define	_FREEBSD_COMPAT_VNODE_H_

#include "mount.h"
#include "namei.h"
#include "timespec.h"
#include "bufobj.h"
#include "stat.h"
#include "dirent.h"
#include "file.h"
#include <inode.h>
#include "../vm/vm_page.h"

struct cdev;

//TODO: This error code doesn't exist in linux and is made up.
#define E_JUSTRETURN 530

/*
 * Flags for accmode_t.
 */
#define VEXEC                   000000000100 /* execute/search permission */
#define VWRITE                  000000000200 /* write permission */
#define VREAD                   000000000400 /* read permission */
#define VADMIN                  000000010000 /* being the file owner */
#define VAPPEND                 000000040000 /* permission to write/append */

/*
 * Flags for ioflag. (high 16 bits used to ask for read-ahead and
 * help with write clustering)
 * NB: IO_NDELAY and IO_DIRECT are linked to fcntl.h
 */
#define IO_UNIT         0x0001          /* do I/O as atomic unit */
#define IO_APPEND       0x0002          /* append write to end */
#define IO_NDELAY       0x0004          /* FNDELAY flag set in file table */
#define IO_NODELOCKED   0x0008          /* underlying node already locked */
#define IO_ASYNC        0x0010          /* bawrite rather then bdwrite */
#define IO_VMIO         0x0020          /* data already in VMIO space */
#define IO_INVAL        0x0040          /* invalidate after I/O */
#define IO_SYNC         0x0080          /* do I/O synchronously */
#define IO_DIRECT       0x0100          /* attempt to bypass buffer cache */
#define IO_EXT          0x0400          /* operate on external attributes */
#define IO_NORMAL       0x0800          /* operate on regular data */
#define IO_NOMACCHECK   0x1000          /* MAC checks unnecessary */
#define IO_BUFLOCKED    0x2000          /* ffs flag; indir buf is locked */
#define IO_RANGELOCKED  0x4000          /* range locked */

#define IO_SEQMAX       0x7F            /* seq heuristic max value */
#define IO_SEQSHIFT     16              /* seq heuristic in upper 16 bits */

/*
 * Flags to various vnode functions.
 */
#define SKIPSYSTEM      0x0001          /* vflush: skip vnodes marked VSYSTEM */
#define FORCECLOSE      0x0002          /* vflush: force file closure */
#define WRITECLOSE      0x0004          /* vflush: only close writable files */
#define DOCLOSE         0x0008          /* vclean: close active files */
#define V_SAVE          0x0001          /* vinvalbuf: sync file first */

//acl.h
typedef int	acl_type_t;

struct vnodeop_desc;

struct vop_generic_args {
	struct vnodeop_desc *a_desc;
	/* other random data follows, presumably */
};

typedef int vop_bypass_t(struct vop_generic_args *);

struct vnodeop_desc {
	char	*vdesc_name;		/* a readable name for debugging */
	int	 vdesc_flags;		/* VDESC_* flags */
	vop_bypass_t	*vdesc_call;	/* Function to call */

	/*
	 * These ops are used by bypass routines to map and locate arguments.
	 * Creds and procs are not needed in bypass routines, but sometimes
	 * they are useful to (for example) transport layers.
	 * Nameidata is useful because it has a cred in it.
	 */
	int	*vdesc_vp_offsets;	/* list ended by VDESC_NO_OFFSET */
	int	vdesc_vpp_offset;	/* return vpp location */
	int	vdesc_cred_offset;	/* cred location, if any */
	int	vdesc_thread_offset;	/* thread location, if any */
	int	vdesc_componentname_offset; /* if any */
};

#define VI_MOUNT        0x0020  /* Mount in progress */
#define VI_AGE          0x0040  /* Insert vnode at head of free list */
#define VI_DOOMED       0x0080  /* This vnode is being recycled */
#define VI_FREE         0x0100  /* This vnode is on the freelist */
#define VI_ACTIVE       0x0200  /* This vnode is on the active list */
#define VI_DOINGINACT   0x0800  /* VOP_INACTIVE is in progress */
#define VI_OWEINACT     0x1000  /* Need to call inactive */

#define VV_ROOT         0x0001  /* root of its filesystem */
#define VV_ISTTY        0x0002  /* vnode represents a tty */
#define VV_NOSYNC       0x0004  /* unlinked, stop syncing */
#define VV_ETERNALDEV   0x0008  /* device that is never destroyed */
#define VV_CACHEDLABEL  0x0010  /* Vnode has valid cached MAC label */
#define VV_TEXT         0x0020  /* vnode is a pure text prototype */
#define VV_COPYONWRITE  0x0040  /* vnode is doing copy-on-write */
#define VV_SYSTEM       0x0080  /* vnode being used by kernel */
#define VV_PROCDEP      0x0100  /* vnode is process dependent */
#define VV_NOKNOTE      0x0200  /* don't activate knotes on this vnode */
#define VV_DELETED      0x0400  /* should be removed */
#define VV_MD           0x0800  /* vnode backs the md device */
#define VV_FORCEINSMQ   0x1000  /* force the insmntque to succeed */

/*
 * Token indicating no attribute value yet assigned.
 */
#define VNOVAL  (-1)

#define	v_mountedhere	v_un.vu_mount
#define	v_socket	v_un.vu_socket
#define	v_rdev		v_un.vu_cdev
#define	v_fifoinfo	v_un.vu_fifoinfo

/* XXX: These are temporary to avoid a source sweep at this time */
#define v_object	v_bufobj.bo_object

enum vtype;
struct vnode;
struct vattr;
struct vop_vector;

enum vtype      { VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO, VBAD,
                  VMARKER };

struct vnode {
  struct inode* ucore_inode;
	/*
	 * Fields which define the identity of the vnode.  These fields are
	 * owned by the filesystem (XXX: and vgone() ?)
	 */
	//const char *v_tag;			/* u type of underlying data */
	struct	vop_vector *v_op;		/* u vnode operations vector */
	void	*v_data;			/* u private data for fs */

	/*
	 * Filesystem instance stuff
	 */
	struct	mount *v_mount;			/* u ptr to vfs we are in */
	//TAILQ_ENTRY(vnode) v_nmntvnodes;	/* m vnodes for mount point */

	/*
	 * Type specific fields, only one applies to any given vnode.
	 * See #defines below for renaming to v_* namespace.
	 */
	union {
		struct mount	*vu_mount;	/* v ptr to mountpoint (VDIR) */
	//	struct socket	*vu_socket;	/* v unix domain net (VSOCK) */
		struct cdev	*vu_cdev; 	/* v device (VCHR, VBLK) */
	//	struct fifoinfo	*vu_fifoinfo;	/* v fifo (VFIFO) */
	} v_un;

	/*
	 * vfs_hash: (mount + inode) -> vnode hash.  The hash value
	 * itself is grouped with other int fields, to avoid padding.
	 */
	//LIST_ENTRY(vnode)	v_hashlist;

	/*
	 * VFS_namecache stuff
	 */
	//LIST_HEAD(, namecache) v_cache_src;	/* c Cache entries from us */
	//TAILQ_HEAD(, namecache) v_cache_dst;	/* c Cache entries to us */
	//struct namecache *v_cache_dd;		/* c Cache entry for .. vnode */

	/*
	 * Locking
	 */
	//struct	lock v_lock;			/* u (if fs don't have one) */
	//struct	mtx v_interlock;		/* lock for "i" things */
	struct	lock *v_vnlock;			/* u pointer to vnode lock */

	/*
	 * The machinery of being a vnode
	 */
	//TAILQ_ENTRY(vnode) v_actfreelist;	/* f vnode active/free lists */
	struct bufobj	v_bufobj;		/* * Buffer cache object */

	/*
	 * Hooks for various subsystems and features.
	 */
	//struct vpollinfo *v_pollinfo;		/* i Poll events, p for *v_pi */
	//struct label *v_label;			/* MAC label for vnode */
	//struct lockf *v_lockf;		/* Byte-level advisory lock list */
	//struct rangelock v_rl;			/* Byte-range lock */

	/*
	 * clustering stuff
	 */
	//daddr_t	v_cstart;			/* v start block of cluster */
	//daddr_t	v_lasta;			/* v last allocation  */
	//daddr_t	v_lastw;			/* v last write  */
	//int	v_clen;				/* v length of cur. cluster */

	//int	v_holdcnt;			/* i prevents recycling. */
	//int	v_usecount;			/* i ref count of users */
	u_int	v_iflag;			/* i vnode flags (see below) */
	u_int	v_vflag;			/* v vnode flags */
	//int	v_writecount;			/* v ref count of writers */
	//u_int	v_hash;
	enum	vtype v_type;			/* u vnode type */
};

/*
 * Vnode attributes.  A field value of VNOVAL represents a field whose value
 * is unavailable (getattr) or which is not to be changed (setattr).
 */
struct vattr {
	enum vtype	va_type;	/* vnode type (for create) */
	u_short		va_mode;	/* files access mode and type */
	short		va_nlink;	/* number of references to file */
	uid_t		va_uid;		/* owner user id */
	gid_t		va_gid;		/* owner group id */
	dev_t		va_fsid;	/* filesystem id */
	long		va_fileid;	/* file id */
	u_quad_t	va_size;	/* file size in bytes */
	long		va_blocksize;	/* blocksize preferred for i/o */
	struct timespec	va_atime;	/* time of last access */
	struct timespec	va_mtime;	/* time of last modification */
	struct timespec	va_ctime;	/* time file changed */
//	struct timespec	va_birthtime;	/* time file created */
//	u_long		va_gen;		/* generation number of file */
	u_long		va_flags;	/* flags defined for file */
	dev_t		va_rdev;	/* device the special file represents */
	u_quad_t	va_bytes;	/* bytes of disk space held by file */
//	u_quad_t	va_filerev;	/* file modification number */
  u_int		va_vaflags;	/* operations flags, see below */
//	long		va_spare;	/* remain quad aligned */
};

/*
 * Set vnode attributes to VNOVAL
 */
static void
vattr_null(struct vattr *vap)
{
/*        vap->va_type = VNON;
        vap->va_size = VNOVAL;
        vap->va_bytes = VNOVAL;
        vap->va_mode = VNOVAL;
        vap->va_nlink = VNOVAL;
        vap->va_uid = VNOVAL;
        vap->va_gid = VNOVAL;
        vap->va_fsid = VNOVAL;
        vap->va_fileid = VNOVAL;
        vap->va_blocksize = VNOVAL;
        vap->va_rdev = VNOVAL;
        vap->va_atime.tv_sec = VNOVAL;
        vap->va_atime.tv_nsec = VNOVAL;
        vap->va_mtime.tv_sec = VNOVAL;
        vap->va_mtime.tv_nsec = VNOVAL;
        vap->va_ctime.tv_sec = VNOVAL;
        vap->va_ctime.tv_nsec = VNOVAL;
        vap->va_birthtime.tv_sec = VNOVAL;
        vap->va_birthtime.tv_nsec = VNOVAL;
        vap->va_flags = VNOVAL;
        vap->va_gen = VNOVAL;
        vap->va_vaflags = 0;*/
}

/*
 * Flags for va_vaflags.
 */
#define VA_UTIMES_NULL  0x01            /* utimes argument was NULL */
#define VA_EXCLUSIVE    0x02            /* exclusive create request */

static enum vtype iftovt_tab[16] = {
        VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
        VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
static int vttoif_tab[10] = {
        0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
        S_IFSOCK, S_IFIFO, S_IFMT, S_IFMT
};

/*
 * Convert between vnode types and inode formats (since POSIX.1
 * defines mode word of stat structure in terms of inode formats).
 */
extern enum vtype       iftovt_tab[];
extern int              vttoif_tab[];
#define IFTOVT(mode)    (iftovt_tab[((mode) & S_IFMT) >> 12])
#define VTTOIF(indx)    (vttoif_tab[(int)(indx)])
#define MAKEIMODE(indx, mode)   (int)(VTTOIF(indx) | (mode))

static void vfs_hash_remove(struct vnode *vp) {
}

/*
 * vputf and vrele are both used for decrease refcount. The following is
 * from FreeBSD Official document, explaining their difference.
 *
 * The vrele() function takes	an unlocked vnode and returns with the vnode
 * unlocked.

 * The vput()	function should	be given a locked vnode	as argument, the vnode
 * is	unlocked after the function returned.  The vput() is operationally
 * equivalent	to calling VOP_UNLOCK(9) followed by vrele(9), with less over-
 * head.
 */

static void vput(struct vnode *vp) {
  //TODO: Unlock the inode.
  inode_ref_dec(vp->ucore_inode);
}

static void vrele(struct vnode *vp) {
  //TODO: Assert the node is locked.
  inode_ref_dec(vp->ucore_inode);
}

static void vref(struct vnode *vp) {
}

static int vn_lock(struct vnode *vp, int flags) {
  return 0;
}

static void cache_purge(struct vnode *vp) {
}

static void cache_purge_negative(struct vnode *vp) {
}

static int vnode_create_vobject(struct vnode *vp, off_t size, struct thread *td) {
  return 0;
}

static void vnode_destroy_vobject(struct vnode *vp) {
}

//TODO: weird thing is that these functions are not generated.
static int OP_LOCK(struct vnode *vp,	int flags) {
  return 0;
}

static int VOP_UNLOCK(struct vnode *vp, int flags) {
  return 0;
}

static int VOP_ISLOCKED(struct vnode *vp) {
  return 0;
}

static int vop_stdfsync(struct vop_fsync_args *fsync_args) {
  return 0;
}

static void assert_vop_elocked(struct vnode *vp, const char *str) {
}
static void assert_vop_locked(struct vnode *vp, const char *str) {
}

static int vinvalbuf(struct vnode *vp, int save, int slpflag, int slptimeo) {
  return 0;
}

#define ASSERT_VOP_ELOCKED(vp, str)     assert_vop_elocked((vp), (str))
#define ASSERT_VOP_LOCKED(vp, str)      assert_vop_locked((vp), (str))

static int vrecycle(struct vnode *vp) {
  return 0;
}

static int vget(struct vnode *vp, int lockflag, struct thread *td) {
  return 0;
}

static int vflush(struct mount *mp, int rootrefs, int flags, struct thread *td) {
  return 0;
}

static void cache_enter(struct vnode *dvp, struct vnode *vp, struct componentname *cnp) {
}

static int cache_lookup(struct vnode *dvp, struct vnode **vpp,
struct componentname *cnp, struct timespec *tsp, int *ticksp) {
  return 0;
}

//vnode_pager.h

static void vnode_pager_setsize(struct vnode *vp, vm_ooffset_t nsize) {
}

static int vtruncbuf(struct vnode *vp, struct ucred *cred, off_t length,
int blksize) {
  return 0;
}

static int insmntque(struct vnode *vp, struct mount *mp) {
  return 0;
}

#include "vnode_if.h"

static int ucore_vop_open(struct inode * node, uint32_t open_flags)
{
  struct vnode* freebsd_vnode = inode_get_private_data(node);
  struct vop_open_args open_args;
  open_args.a_vp = freebsd_vnode;
  open_args.a_mode = open_flags; //TODO: Convertion mgiht be required?
  open_args.a_cred = &freebsd_stub_ucred;
  open_args.a_td = NULL; //TODO: This won't work
#undef vop_open
  return freebsd_vnode->v_op->vop_open(&open_args);
}

static int ucore_vop_close(struct inode * node)
{
  struct vnode* freebsd_vnode = inode_get_private_data(node);
  struct vop_close_args close_args;
  close_args.a_vp = freebsd_vnode;
  close_args.a_fflag = 0; //TODO: Using 0 as flags
  close_args.a_cred = &freebsd_stub_ucred;
#undef vop_close
  return freebsd_vnode->v_op->vop_close(&close_args);
}

static int ucore_vop_read(struct inode * node, struct iobuf * iob)
{
  struct iovec freebsd_iovec = {
    .iov_base = iob->io_base,
    .iov_len = iob->io_len
  };
  struct uio freebsd_uio = {
    .uio_iov = &freebsd_iovec,
    .uio_iovcnt = 1,
    .uio_offset = iob->io_offset,
    .uio_resid = iob->io_resid,
    .uio_segflg = UIO_SYSSPACE,
    .uio_rw = UIO_READ,
    .uio_td = &freebsd_stub_thread
  };
  struct vnode* freebsd_vnode = inode_get_private_data(node);
  struct vop_read_args read_args = {
    .a_vp = freebsd_vnode,
    .a_uio = &freebsd_uio,
    .a_ioflag = 0, //Use 0 as I/O flags
    .a_cred = &freebsd_stub_ucred
  };
#undef vop_read
  return freebsd_vnode->v_op->vop_read(&read_args);
}

static int ucore_vop_write(struct inode * node, struct iobuf * iob)
{
  struct iovec freebsd_iovec = {
    .iov_base = iob->io_base,
    .iov_len = iob->io_len
  };
  struct uio freebsd_uio = {
    .uio_iov = &freebsd_iovec,
    .uio_iovcnt = 1,
    .uio_offset = iob->io_offset,
    .uio_resid = iob->io_resid,
    .uio_segflg = UIO_SYSSPACE,
    .uio_rw = UIO_WRITE,
    .uio_td = &freebsd_stub_thread
  };
  struct vnode* freebsd_vnode = inode_get_private_data(node);
  struct vop_write_args write_args = {
    .a_vp = freebsd_vnode,
    .a_uio = &freebsd_uio,
    .a_ioflag = 0, //Use 0 as I/O flags
    .a_cred = &freebsd_stub_ucred
  };
#undef vop_write
  return freebsd_vnode->v_op->vop_write(&write_args);
}

static struct inode_ops ucore_inode_operation_wrapper = {
	.vop_magic = VOP_MAGIC,
  .vop_open = ucore_vop_open,
  .vop_close = ucore_vop_close,
  .vop_read = ucore_vop_read,
  .vop_write = ucore_vop_write,
	/*int (*vop_fstat) (struct inode * node, struct stat * stat);
	int (*vop_fsync) (struct inode * node);
	int (*vop_mkdir) (struct inode * node, const char *name);
	int (*vop_link) (struct inode * node, const char *name,
			 struct inode * link_node);
	int (*vop_rename) (struct inode * node, const char *name,
			   struct inode * new_node, const char *new_name);
	int (*vop_readlink) (struct inode * node, struct iobuf * iob);
	int (*vop_symlink) (struct inode * node, const char *name,
			    const char *path);
	int (*vop_namefile) (struct inode * node, struct iobuf * iob);
	int (*vop_getdirentry) (struct inode * node, struct iobuf * iob);
	int (*vop_reclaim) (struct inode * node);
	int (*vop_ioctl) (struct inode * node, int op, void *data);
	int (*vop_gettype) (struct inode * node, uint32_t * type_store);
	int (*vop_tryseek) (struct inode * node, off_t pos);
	int (*vop_truncate) (struct inode * node, off_t len);
	int (*vop_create) (struct inode * node, const char *name, bool excl,
			   struct inode ** node_store);
	int (*vop_unlink) (struct inode * node, const char *name);
	int (*vop_lookup) (struct inode * node, char *path,
			   struct inode ** node_store);
	int (*vop_lookup_parent) (struct inode * node, char *path,
				  struct inode ** node_store, char **endp);*/
};


static int getnewvnode(const char *tag, struct mount *mp, struct vop_vector *vops, struct vnode **vpp) {
	//TODO: tag is ignored
	//TODO: mp is ignored, this matters.
	struct inode* inode = alloc_inode(default_inode);
  assert(inode != NULL);
  struct vnode* vnode = kmalloc(sizeof(struct vnode));
	vnode->ucore_inode = inode;
	vnode->v_op = vops;
  (*vpp) = vnode;
  inode_set_private_data(inode, vnode);
  return 0;
}

#endif
