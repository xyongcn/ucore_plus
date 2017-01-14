#ifndef _FREEBSD_COMPAT_PRIV_H_
#define	_FREEBSD_COMPAT_PRIV_H_

#define PRIV_VFS_READ           310     /* Override vnode DAC read perm. */
#define PRIV_VFS_WRITE          311     /* Override vnode DAC write perm. */
#define PRIV_VFS_ADMIN          312     /* Override vnode DAC admin perm. */
#define PRIV_VFS_EXEC           313     /* Override vnode DAC exec perm. */
#define PRIV_VFS_LOOKUP         314     /* Override vnode DAC lookup perm. */
#define PRIV_VFS_BLOCKRESERVE   315     /* Can use free block reserve. */
#define PRIV_VFS_CHFLAGS_DEV    316     /* Can chflags() a device node. */
#define PRIV_VFS_CHOWN          317     /* Can set user; group to non-member. */
#define PRIV_VFS_CHROOT         318     /* chroot(). */
#define PRIV_VFS_RETAINSUGID    319     /* Can retain sugid bits on change. */
#define PRIV_VFS_EXCEEDQUOTA    320     /* Exempt from quota restrictions. */
#define PRIV_VFS_EXTATTR_SYSTEM 321     /* Operate on system EA namespace. */
#define PRIV_VFS_FCHROOT        322     /* fchroot(). */
#define PRIV_VFS_FHOPEN         323     /* Can fhopen(). */
#define PRIV_VFS_FHSTAT         324     /* Can fhstat(). */
#define PRIV_VFS_FHSTATFS       325     /* Can fhstatfs(). */
#define PRIV_VFS_GENERATION     326     /* stat() returns generation number. */
#define PRIV_VFS_GETFH          327     /* Can retrieve file handles. */
#define PRIV_VFS_GETQUOTA       328     /* getquota(). */
#define PRIV_VFS_LINK           329     /* bsd.hardlink_check_uid */
#define PRIV_VFS_MKNOD_BAD      330     /* Can mknod() to mark bad inodes. */
#define PRIV_VFS_MKNOD_DEV      331     /* Can mknod() to create dev nodes. */
#define PRIV_VFS_MKNOD_WHT      332     /* Can mknod() to create whiteout. */
#define PRIV_VFS_MOUNT          333     /* Can mount(). */
#define PRIV_VFS_MOUNT_OWNER    334     /* Can manage other users' file systems. */
#define PRIV_VFS_MOUNT_EXPORTED 335     /* Can set MNT_EXPORTED on mount. */
#define PRIV_VFS_MOUNT_PERM     336     /* Override dev node perms at mount. */
#define PRIV_VFS_MOUNT_SUIDDIR  337     /* Can set MNT_SUIDDIR on mount. */
#define PRIV_VFS_MOUNT_NONUSER  338     /* Can perform a non-user mount. */
#define PRIV_VFS_SETGID         339     /* Can setgid if not in group. */
#define PRIV_VFS_SETQUOTA       340     /* setquota(). */
#define PRIV_VFS_STICKYFILE     341     /* Can set sticky bit on file. */
#define PRIV_VFS_SYSFLAGS       342     /* Can modify system flags. */
#define PRIV_VFS_UNMOUNT        343     /* Can unmount(). */
#define PRIV_VFS_STAT           344     /* Override vnode MAC stat perm. */

static int priv_check(struct thread *td, int priv) {
  return 0;
}

static int priv_check_cred(struct ucred *cred, int priv, int flags) {
  return 0;
}

#endif
