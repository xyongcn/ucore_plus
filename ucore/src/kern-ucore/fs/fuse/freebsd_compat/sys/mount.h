#ifndef _FREEBSD_COMPAT_MOUNT_H_
#define	_FREEBSD_COMPAT_MOUNT_H_

#include "module.h"

/*
 * Version numbers.
 */
#define VFS_VERSION_00  0x19660120
#define VFS_VERSION     VFS_VERSION_00

/*
 * NB: these flags refer to IMPLEMENTATION properties, not properties of
 * any actual mounts; i.e., it does not make sense to change the flags.
 */
#define VFCF_STATIC     0x00010000      /* statically compiled into kernel */
#define VFCF_NETWORK    0x00020000      /* may get data over the network */
#define VFCF_READONLY   0x00040000      /* writes are not implemented */
#define VFCF_SYNTHETIC  0x00080000      /* data does not represent real files */
#define VFCF_LOOPBACK   0x00100000      /* aliases some other mounted FS */
#define VFCF_UNICODE    0x00200000      /* stores file names as Unicode */
#define VFCF_JAIL       0x00400000      /* can be mounted from within a jail */

struct mount;
struct statfs;

#define MNT_RDONLY      0x0000000000000001ULL /* read only filesystem */
#define MNT_SYNCHRONOUS 0x0000000000000002ULL /* fs written synchronously */
#define MNT_NOEXEC      0x0000000000000004ULL /* can't exec from filesystem */
#define MNT_NOSUID      0x0000000000000008ULL /* don't honor setuid fs bits */
#define MNT_NFS4ACLS    0x0000000000000010ULL /* enable NFS version 4 ACLs */
#define MNT_UNION       0x0000000000000020ULL /* union with underlying fs */
#define MNT_ASYNC       0x0000000000000040ULL /* fs written asynchronously */
#define MNT_SUIDDIR     0x0000000000100000ULL /* special SUID dir handling */
#define MNT_SOFTDEP     0x0000000000200000ULL /* using soft updates */
#define MNT_NOSYMFOLLOW 0x0000000000400000ULL /* do not follow symlinks */
#define MNT_GJOURNAL    0x0000000002000000ULL /* GEOM journal support enabled */
#define MNT_MULTILABEL  0x0000000004000000ULL /* MAC support for objects */
#define MNT_ACLS        0x0000000008000000ULL /* ACL support enabled */
#define MNT_NOATIME     0x0000000010000000ULL /* dont update file access time */
#define MNT_NOCLUSTERR  0x0000000040000000ULL /* disable cluster read */
#define MNT_NOCLUSTERW  0x0000000080000000ULL /* disable cluster write */
#define MNT_SUJ         0x0000000100000000ULL /* using journaled soft updates */
#define MNT_AUTOMOUNTED 0x0000000200000000ULL /* mounted by automountd(8) */

#define MNT_WAIT        1       /* synchronously wait for I/O to complete */
#define MNT_NOWAIT      2       /* start all I/O, but do not wait for it */
#define MNT_LAZY        3       /* push data not written by filesystem syncer */
#define MNT_SUSPEND     4       /* Suspend file system after sync */

/*
 * vfs_busy specific flags and mask.
 */
#define MBF_NOWAIT      0x01
#define MBF_MNTLSTLOCK  0x02
#define MBF_MASK        (MBF_NOWAIT | MBF_MNTLSTLOCK)

typedef struct fsid { int32_t val[2]; } fsid_t; /* filesystem id type */

/*
 * filesystem statistics
 */
#define MFSNAMELEN      16              /* length of type name including null */
#define MNAMELEN        88              /* size of on/from name bufs */
#define STATFS_VERSION  0x20030518      /* current version number */
struct statfs {
//	uint32_t f_version;		/* structure version number */
//	uint32_t f_type;		/* type of filesystem */
//	uint64_t f_flags;		/* copy of mount exported flags */
//	uint64_t f_bsize;		/* filesystem fragment size */
	uint64_t f_iosize;		/* optimal transfer block size */
//	uint64_t f_blocks;		/* total data blocks in filesystem */
//	uint64_t f_bfree;		/* free blocks in filesystem */
//	int64_t	 f_bavail;		/* free blocks avail to non-superuser */
//	uint64_t f_files;		/* total file nodes in filesystem */
//	int64_t	 f_ffree;		/* free nodes avail to non-superuser */
//	uint64_t f_syncwrites;		/* count of sync writes since mount */
//	uint64_t f_asyncwrites;		/* count of async writes since mount */
//	uint64_t f_syncreads;		/* count of sync reads since mount */
//	uint64_t f_asyncreads;		/* count of async reads since mount */
//	uint64_t f_spare[10];		/* unused spare */
//	uint32_t f_namemax;		/* maximum filename length */
//	uid_t	  f_owner;		/* user that mounted the filesystem */
	fsid_t	  f_fsid;		/* filesystem id */
//	char	  f_charspare[80];	    /* spare string space */
//	char	  f_fstypename[MFSNAMELEN]; /* filesystem type name */
//	char	  f_mntfromname[MNAMELEN];  /* mounted filesystem */
	char	  f_mntonname[MNAMELEN];    /* directory on which mounted */
};

struct mount {
//	struct mtx	mnt_mtx;		/* mount structure interlock */
//	int		mnt_gen;		/* struct mount generation */
//#define	mnt_startzero	mnt_list
//	TAILQ_ENTRY(mount) mnt_list;		/* (m) mount list */
//	struct vfsops	*mnt_op;		/* operations on fs */
//	struct vfsconf	*mnt_vfc;		/* configuration info */
//	struct vnode	*mnt_vnodecovered;	/* vnode we mounted on */
//	struct vnode	*mnt_syncer;		/* syncer vnode */
//	int		mnt_ref;		/* (i) Reference count */
//	struct vnodelst	mnt_nvnodelist;		/* (i) list of vnodes */
//	int		mnt_nvnodelistsize;	/* (i) # of vnodes */
//	struct vnodelst	mnt_activevnodelist;	/* (v) list of active vnodes */
//	int		mnt_activevnodelistsize;/* (v) # of active vnodes */
//	int		mnt_writeopcount;	/* (i) write syscalls pending */
//	int		mnt_kern_flag;		/* (i) kernel only flags */
	uint64_t	mnt_flag;		/* (i) flags shared with user */
//	struct vfsoptlist *mnt_opt;		/* current mount options */
//	struct vfsoptlist *mnt_optnew;		/* new options passed to fs */
//	int		mnt_maxsymlinklen;	/* max size of short symlink */
  struct statfs	mnt_stat;		/* cache of filesystem stats */
//	struct ucred	*mnt_cred;		/* credentials of mounter */
	void *		mnt_data;		/* private data */
//	time_t		mnt_time;		/* last time written*/
//	int		mnt_iosize_max;		/* max size for clusters, etc */
//	struct netexport *mnt_export;		/* export list */
//	struct label	*mnt_label;		/* MAC label for the fs */
//	u_int		mnt_hashseed;		/* Random seed for vfs_hash */
//	int		mnt_lockref;		/* (i) Lock reference count */
//	int		mnt_secondary_writes;   /* (i) # of secondary writes */
//	int		mnt_secondary_accwrites;/* (i) secondary wr. starts */
//	struct thread	*mnt_susp_owner;	/* (i) thread owning suspension */
//#define	mnt_endzero	mnt_gjprovider
//	char		*mnt_gjprovider;	/* gjournal provider name */
//	struct lock	mnt_explock;		/* vfs_export walkers lock */
//	TAILQ_ENTRY(mount) mnt_upper_link;	/* (m) we in the all uppers */
//	TAILQ_HEAD(, mount) mnt_uppers;		/* (m) upper mounts over us*/
};

/*
 * Filesystem configuration information. One of these exists for each
 * type of filesystem supported by the kernel. These are searched at
 * mount time to identify the requested filesystem.
 *
 * XXX: Never change the first two arguments!
 */
struct vfsconf {
        u_int   vfc_version;            /* ABI version number */
        char    vfc_name[MFSNAMELEN];   /* filesystem type name */
        struct  vfsops *vfc_vfsops;     /* filesystem operations vector */
        int     vfc_typenum;            /* historic filesystem type number */
        //int     vfc_refcount;           /* number mounted of this type */
        int     vfc_flags;              /* permanent flags */
        //struct  vfsoptdecl *vfc_opts;   /* mount options */
        //TAILQ_ENTRY(vfsconf) vfc_list;  /* list of vfscons */
};

static int vfs_modevent(module_t m, int what, void *p) {
  return 0;
}

static int vfs_busy(struct mount *m, int i) {
  return 0;
}

static int vfs_unbusy(struct mount *m) {
  return 0;
}

static void vfs_ref(struct mount *m) {

}

static int vfs_rel(struct mount *m) {
  return 0;
}

#endif
