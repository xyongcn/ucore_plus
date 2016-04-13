#ifndef _FREEBSD_COMPAT_UCRED_H_
#define	_FREEBSD_COMPAT_UCRED_H_

struct ucred;

struct ucred {
	//u_int	cr_ref;			/* reference count */
#define	cr_startcopy cr_uid
	uid_t	cr_uid;			/* effective user id */
	uid_t	cr_ruid;		/* real user id */
	uid_t	cr_svuid;		/* saved user id */
	//int	cr_ngroups;		/* number of groups */
	gid_t	cr_rgid;		/* real group id */
	gid_t	cr_svgid;		/* saved group id */
	//struct uidinfo	*cr_uidinfo;	/* per euid resource consumption */
	//struct uidinfo	*cr_ruidinfo;	/* per ruid resource consumption */
	//struct prison	*cr_prison;	/* jail(2) */
	//struct loginclass	*cr_loginclass; /* login class */
	//u_int		cr_flags;	/* credential flags */
	//void 		*cr_pspare2[2];	/* general use 2 */
#define	cr_endcopy	cr_label
	//struct label	*cr_label;	/* MAC label */
	//struct auditinfo_addr	cr_audit;	/* Audit properties. */
	gid_t	*cr_groups;		/* groups */
	//int	cr_agroups;		/* Available groups */
};

#define NOCRED  ((struct ucred *)0)     /* no credential available */
#define FSCRED  ((struct ucred *)-1)    /* filesystem credential */

static void crfree(struct ucred *cr) {}
static struct ucred *crhold(struct ucred *cr) {
  return NULL;
}

#endif
