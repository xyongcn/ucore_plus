#ifndef _FREEBSD_COMPAT_CONF_H_
#define	_FREEBSD_COMPAT_CONF_H_

#include <assert.h>
#include <dev.h>
#include <string.h>
#include <vfs.h>
#include <inode.h>
#include "proc.h"
#include "ucred.h"
#include "uio.h"

#define D_VERSION_00    0x20011966
#define D_VERSION_01    0x17032005      /* Add d_uid,gid,mode & kind */
#define D_VERSION_02    0x28042009      /* Add d_mmap_single */
#define D_VERSION_03    0x17122009      /* d_mmap takes memattr,vm_ooffset_t */
#define D_VERSION       D_VERSION_03

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

//TODO : Seems only tid and ucred is used in fuse_device operations.
static struct thread freebsd_stub_thread = {
  .td_proc = NULL,
  .td_tid = 0,
  .td_ucred = &freebsd_stub_ucred,
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
  .si_devsw = NULL,
  .ucore_device = NULL
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
  kprintf("TODO! FreeBSD-compat: ucore_char_device_io is passing "
  "0 as flags to si_devsw->d_write/d_close, and the iobuf -> uio convertion "
  "may be not perfect.\r\n");
  assert(dev == freebsd_char_device->ucore_device);
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
    .uio_rw = write ? UIO_WRITE : UIO_READ,
    .uio_td = &freebsd_stub_thread
  };
  if(write) {
    freebsd_char_device->si_devsw->d_write(freebsd_char_device, &freebsd_uio, 0);
  }
  else {
    freebsd_char_device->si_devsw->d_read(freebsd_char_device, &freebsd_uio, 0);
  }
  return 0;
}

static int ucore_char_device_ioctl(struct device *dev, int op, void *data)
{
  kprintf("TODO! FreeBSD-compat: ucore_char_device_ioctl not supported.\r\n");
  return -E_INVAL;
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
