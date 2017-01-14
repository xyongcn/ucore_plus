#ifndef _FREEBSD_COMPAT_UCRED_H_
#define	_FREEBSD_COMPAT_UCRED_H_

#define		UID_ROOT	0

#define		GID_WHEEL	0
#define		GID_OPERATOR	5

struct ucred;

struct ucred {
#define	cr_startcopy cr_uid
	uid_t	cr_uid;			/* effective user id */
	uid_t	cr_ruid;		/* real user id */
	uid_t	cr_svuid;		/* saved user id */
	gid_t	cr_rgid;		/* real group id */
	gid_t	cr_svgid;		/* saved group id */
	gid_t	*cr_groups;		/* groups */
};

/*
 * Used to pass a stub thread ID and ucred object to FUSE permission-check
 * related functions. UID_ROOT and GID_WHEEL stands for highest permission.
 */

static struct ucred freebsd_stub_ucred = {
  .cr_uid = UID_ROOT,
  .cr_ruid = UID_ROOT,
  .cr_svuid = UID_ROOT,
  .cr_rgid = GID_WHEEL,
  .cr_svgid = GID_WHEEL,
  .cr_groups = {GID_WHEEL}
};

#define NOCRED  ((struct ucred *)0)     /* no credential available */
#define FSCRED  ((struct ucred *)-1)    /* filesystem credential */

/*
 * Those functions are related to cred memory management, but we don't care
 * since we only use one pre-allocated stub ucred object.
 */

static void crfree(struct ucred *cr)
{

}

static struct ucred *crhold(struct ucred *cr)
{
  return cr;
}

#endif
