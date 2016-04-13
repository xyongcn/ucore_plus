#ifndef _FREEBSD_COMPAT_CONF_H_
#define	_FREEBSD_COMPAT_CONF_H_

#define D_VERSION_00    0x20011966
#define D_VERSION_01    0x17032005      /* Add d_uid,gid,mode & kind */
#define D_VERSION_02    0x28042009      /* Add d_mmap_single */
#define D_VERSION_03    0x17122009      /* d_mmap takes memattr,vm_ooffset_t */
#define D_VERSION       D_VERSION_03

#define		UID_ROOT	0
#define		UID_BIN		3
#define		UID_UUCP	66
#define		UID_NOBODY	65534

#define		GID_WHEEL	0
#define		GID_KMEM	2
#define		GID_TTY		4
#define		GID_OPERATOR	5
#define		GID_BIN		7
#define		GID_GAMES	13
#define		GID_DIALER	68
#define		GID_NOBODY	65534

struct cdev;
struct cdevsw;
struct thread;
struct uio;

typedef int d_open_t(struct cdev *dev, int oflags, int devtype, struct thread *td);
typedef int d_close_t(struct cdev *dev, int fflag, int devtype, struct thread *td);
typedef int d_read_t(struct cdev *dev, struct uio *uio, int ioflag);
typedef int d_write_t(struct cdev *dev, struct uio *uio, int ioflag);
typedef int d_poll_t(struct cdev *dev, int events, struct thread *td);
typedef void d_priv_dtor_t(void *data);

struct cdev {
  char si_name[SPECNAMELEN + 1];
  struct cdevsw *si_devsw;
};

struct cdevsw {
	int	d_version;
	const char *d_name;
	d_open_t *d_open;
	d_close_t	*d_close;
	d_read_t *d_read;
	d_write_t *d_write;
	d_poll_t *d_poll;
};

static int devfs_set_cdevpriv(void *priv, d_priv_dtor_t *dtr);
static int devfs_set_cdevpriv(void *priv, d_priv_dtor_t *dtr) {
  return 0;
}

static int devfs_get_cdevpriv(void **datap);
static int devfs_get_cdevpriv(void **datap) {
  return 0;
}

static struct cdev *make_dev(struct cdevsw *_devsw, int _unit, uid_t _uid, gid_t _gid,
int _perms, const char *_fmt, ...) {
  return NULL;
}

static void destroy_dev(struct cdev *_dev) {
}

static void dev_ref(struct cdev *dev) {
}

static void dev_rel(struct cdev *dev) {
}

#endif
