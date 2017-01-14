#ifndef _FREEBSD_COMPAT_NAMEI_H_
#define	_FREEBSD_COMPAT_NAMEI_H_

#include "uio.h"

struct thread;
struct ucred;
struct vnode;

struct componentname {
	/*
	 * Arguments to lookup.
	 */
	u_long	cn_nameiop;	/* namei operation */
	u_int64_t cn_flags;	/* flags to namei */
	struct	thread *cn_thread;/* thread requesting lookup */
	struct	ucred *cn_cred;	/* credentials */
	//int	cn_lkflags;	/* Lock flags LK_EXCLUSIVE or LK_SHARED */
	/*
	 * Shared between lookup and commit routines.
	 */
	//char	*cn_pnbuf;	/* pathname buffer */
	char	*cn_nameptr;	/* pointer to looked up name */
	long	cn_namelen;	/* length of looked up component */
	//long	cn_consume;	/* chars to consume in lookup() */
};

/*
 * Encapsulation of namei parameters.
 */
struct nameidata {
        /*
         * Arguments to namei/lookup.
         */
        //const   char *ni_dirp;          /* pathname pointer */
        //enum    uio_seg ni_segflg;      /* location of pathname */
        //cap_rights_t ni_rightsneeded;   /* rights required to look up vnode */
        /*
         * Arguments to lookup.
         */
        //struct  vnode *ni_startdir;     /* starting directory */
        //struct  vnode *ni_rootdir;      /* logical root directory */
        //struct  vnode *ni_topdir;       /* logical top directory */
        //int     ni_dirfd;               /* starting directory for *at functions */
        //int     ni_strictrelative;      /* relative lookup only; no '..' */
        /*
         * Results: returned from namei
         */
        //struct filecaps ni_filecaps;    /* rights the *at base has */
        /*
         * Results: returned from/manipulated by lookup
         */
        struct  vnode *ni_vp;           /* vnode of result */
        //struct  vnode *ni_dvp;          /* vnode of intermediate directory */
        /*
         * Shared between namei and lookup/commit routines.
         */
        //size_t  ni_pathlen;             /* remaining chars in path */
        //char    *ni_next;               /* next location in pathname */
        //u_int   ni_loopcnt;             /* count of symlinks encountered */
        /*
         * Lookup parameters: this structure describes the subset of
         * information from the nameidata structure that is passed
         * through the VOP interface.
         */
        //struct componentname ni_cnd;
};

/*
 * namei operations
 */
#define	LOOKUP		0	/* perform name lookup only */
#define	CREATE		1	/* setup for file creation */
#define	DELETE		2	/* setup for file deletion */
#define	RENAME		3	/* setup for file renaming */
#define	OPMASK		3	/* mask for operation */
/*
 * namei operational modifier flags, stored in ni_cnd.flags
 */
#define	LOCKLEAF	0x0004	/* lock vnode on return */
#define	LOCKPARENT	0x0008	/* want parent vnode returned locked */
#define	WANTPARENT	0x0010	/* want parent vnode returned unlocked */
#define	NOCACHE		0x0020	/* name must not be left in cache */
#define	FOLLOW		0x0040	/* follow symbolic links */
#define	LOCKSHARED	0x0100	/* Shared lock leaf */
#define	NOFOLLOW	0x0000	/* do not follow symbolic links (pseudo) */
#define	MODMASK		0x01fc	/* mask of operational modifiers */

/*
 * Namei parameter descriptors.
 *
 * SAVENAME may be set by either the callers of namei or by VOP_LOOKUP.
 * If the caller of namei sets the flag (for example execve wants to
 * know the name of the program that is being executed), then it must
 * free the buffer. If VOP_LOOKUP sets the flag, then the buffer must
 * be freed by either the commit routine or the VOP_ABORT routine.
 * SAVESTART is set only by the callers of namei. It implies SAVENAME
 * plus the addition of saving the parent directory that contains the
 * name in ni_startdir. It allows repeated calls to lookup for the
 * name being sought. The caller is responsible for releasing the
 * buffer and for vrele'ing ni_startdir.
 */
#define RDONLY          0x00000200 /* lookup with read-only semantics */
#define HASBUF          0x00000400 /* has allocated pathname buffer */
#define SAVENAME        0x00000800 /* save pathname buffer */
#define SAVESTART       0x00001000 /* save starting directory */
#define ISDOTDOT        0x00002000 /* current component name is .. */
#define MAKEENTRY       0x00004000 /* entry is to be added to name cache */
#define ISLASTCN        0x00008000 /* this is last component of pathname */
#define ISSYMLINK       0x00010000 /* symlink needs interpretation */
#define ISWHITEOUT      0x00020000 /* found whiteout */
#define DOWHITEOUT      0x00040000 /* do whiteouts */
#define WILLBEDIR       0x00080000 /* new files will be dirs; allow trailing / */
#define ISUNICODE       0x00100000 /* current component name is unicode*/
#define ISOPEN          0x00200000 /* caller is opening; return a real vnode. */
#define NOCROSSMOUNT    0x00400000 /* do not cross mount points */
#define NOMACCHECK      0x00800000 /* do not perform MAC checks */
#define AUDITVNODE1     0x04000000 /* audit the looked up vnode information */
#define AUDITVNODE2     0x08000000 /* audit the looked up vnode information */
#define TRAILINGSLASH   0x10000000 /* path ended in a slash */
#define NOCAPCHECK      0x20000000 /* do not perform capability checks */
#define PARAMASK        0x3ffffe00 /* mask of parameter descriptors */

#define NDF_NO_DVP_RELE         0x00000001
#define NDF_NO_DVP_UNLOCK       0x00000002
#define NDF_NO_DVP_PUT          0x00000003
#define NDF_NO_VP_RELE          0x00000004
#define NDF_NO_VP_UNLOCK        0x00000008
#define NDF_NO_VP_PUT           0x0000000c
#define NDF_NO_STARTDIR_RELE    0x00000010
#define NDF_NO_FREE_PNBUF       0x00000020
#define NDF_ONLY_PNBUF          (~NDF_NO_FREE_PNBUF)

static void NDINIT(struct nameidata *ndp, u_long op, u_long flags,
enum uio_seg segflg, const char *namep, int dirfd) {

}

static void NDFREE(struct nameidata *ndp, const u_int flags) {

}

static int namei(struct nameidata *ndp) {
  return 0;
}

#endif
