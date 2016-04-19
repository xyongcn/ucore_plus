#ifndef _FREEBSD_COMPAT_CONF_H_
#define	_FREEBSD_COMPAT_CONF_H_

#include <dev.h>
#include <string.h>
#include <vfs.h>
#include <inode.h>
#include "proc.h"
#include "ucred.h"

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
  struct device *ucore_device;
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

/*
 * Used to pass a stub thread ID and ucred object to FUSE character device I/O
 * functions.
 * TODO: This is a temporary solution and may lead to severe bug.
 */

static struct ucred freebsd_char_device_stub_ucred = {
  .cr_uid = UID_ROOT,
  .cr_ruid = UID_ROOT,
  .cr_svuid = UID_ROOT,
  .cr_rgid = GID_WHEEL,
  .cr_svgid = GID_WHEEL,
  .cr_groups = {GID_WHEEL}
};

//TODO : Seems only tid and ucred is used in fuse_device operations.
static struct thread freebsd_stub_thread = {
  .td_proc = NULL,
  .td_tid = 0,
  .td_ucred = &freebsd_char_device_stub_ucred,
  .td_fpop = NULL
  //sigqueue_t	td_sigqueue;
};

/*
 * Used to pass character device operations to uCore.
 * TODO: with this method, make_device can be only called once in a compiling
 * unit, but this seems to be enough for FUSE.
 */
static int ucore_char_device_open(struct device *dev, uint32_t open_flags);
static int ucore_char_device_close(struct device *dev);
static int ucore_char_device_io(struct device *dev, struct iobuf *iob, bool write);
static int ucore_char_device_ioctl(struct device *dev, int op, void *data);
static struct cdev _freebsd_char_device = {
  .si_devsw = NULL
};
static struct cdev* freebsd_char_device = &_freebsd_char_device;


static int devfs_set_cdevpriv(void *priv, d_priv_dtor_t *dtr);
static int devfs_set_cdevpriv(void *priv, d_priv_dtor_t *dtr) {
  dev_set_private_data(freebsd_char_device->ucore_device, priv);
  dtr(priv);
  return 0;
}

static int devfs_get_cdevpriv(void **datap);
static int devfs_get_cdevpriv(void **datap) {
  *datap = dev_get_private_data(freebsd_char_device->ucore_device);
  return 0;
}

static int ucore_char_device_open(struct device *dev, uint32_t open_flags)
{
  kprintf("TODO! FreeBSD-compat: ucore_char_device_open is passing"
  " an stub thread object to si_devsw->d_open, and 0 as devtype.\r\n");
  return freebsd_char_device->si_devsw->d_open(freebsd_char_device, open_flags, 0, &freebsd_stub_thread);
}

static int ucore_char_device_close(struct device *dev)
{
  kprintf("TODO! FreeBSD-compat: ucore_char_device_close is passing"
  " an stub thread object to si_devsw->d_close, and 0 as devtype and fflags.\r\n");
  return freebsd_char_device->si_devsw->d_close(freebsd_char_device, 0, 0, &freebsd_stub_thread);
}

static int ucore_char_device_io(struct device *dev, struct iobuf *iob, bool write)
{
  return 0;
}

static int ucore_char_device_ioctl(struct device *dev, int op, void *data)
{
  return 0;
}

static struct cdev *make_dev(struct cdevsw *_devsw, int _unit, uid_t _uid, gid_t _gid,
int _perms, const char *_fmt, ...)
{
  if(freebsd_char_device->si_devsw != NULL)
  {
    panic("TODO! FreeBSD-compat: make_dev called more than once in a compiling "
    "unit is not supported!\r\n");
  }
  kprintf("TODO! FreeBSD-compat: make_dev only support param _devsw and _fmt\r\n");
  freebsd_char_device->si_devsw = _devsw;
  strcpy(freebsd_char_device->si_name, _fmt);
  kprintf("TODO! FreeBSD-compat: poll operations is not supported in uCore.\r\n");
  struct inode *node = dev_create_inode();
  if (node == NULL) {
    panic("ERROR! FreeBSD-compat: make_dev() failed due to dev_create_inode()."
    "\r\nreturns NULL.");
  }
  struct device* ucore_char_device = vop_info(node, device);
  freebsd_char_device->ucore_device = ucore_char_device;
  memset(ucore_char_device, 0, sizeof(struct device));
  ucore_char_device->d_blocks = 0;
  ucore_char_device->d_blocksize = 1;
  ucore_char_device->d_open = ucore_char_device_open;
  ucore_char_device->d_close = ucore_char_device_close;
  ucore_char_device->d_io = ucore_char_device_ioctl;
  ucore_char_device->d_ioctl = ucore_char_device_ioctl;

  if (vfs_add_dev(_fmt, node, 0) != 0) {
    panic("ERROR! FreeBSD-compat: make_dev() failed due to vfs_add_dev()."
    "\r\nreturns non-zero.");
  }

  return freebsd_char_device;
}

static void destroy_dev(struct cdev *_dev)
{
}

static void dev_ref(struct cdev *dev) {
}

static void dev_rel(struct cdev *dev) {
}

#endif
