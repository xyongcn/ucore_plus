#ifndef _FREEBSD_COMPAT_NAMEI_H_
#define	_FREEBSD_COMPAT_NAMEI_H_

struct thread;
struct ucred;

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

#endif
