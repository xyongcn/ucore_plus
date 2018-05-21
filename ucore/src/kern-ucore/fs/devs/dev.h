#ifndef __KERN_FS_DEVS_DEV_H__
#define __KERN_FS_DEVS_DEV_H__

#include <types.h>
//#include <wait.h>

struct inode;
struct iobuf;
struct file_operations;

#ifndef __user
#define __user
#endif

struct __wait_t;

/*
 * Filesystem-namespace-accessible device.
 * For user, device is a special file
 *
 * device is a abstract structure
 * each instance should implement such operation functions:
 *		@ d_open
 *		@ d_close
 *		@ d_io
 *		@ d_ioctl
 * d_io is for both reads and writes; the iob indicates the io buffer, write indicates direction.
 */
#ifdef __NO_UCORE_DEVICE__
struct ucore_device {
#else
struct device {
#endif
	size_t d_blocks;
	size_t d_blocksize;
	/* for Linux */
	/*
	   unsigned long i_rdev;
	   mode_t i_mode;
	   const struct file_operations *i_fops;
	 */
void *linux_file;
	void *linux_dentry;

	int (*d_linux_read) (struct device * dev, const char __user * buf,
			     size_t count, size_t * offset);
	int (*d_linux_write) (struct device * dev, const char __user * buf,
			      size_t count, size_t * offset);
	/* new ioctl */
	int (*d_linux_ioctl) (struct device * dev, unsigned int, unsigned long);
	void *(*d_linux_mmap) (struct device * dev, void *addr, size_t len,
			       int unused1, int unused2, size_t off);

	int (*d_open) (struct device * dev, uint32_t open_flags);
	int (*d_close) (struct device * dev);
	int (*d_io) (struct device * dev, struct iobuf * iob, bool write, int io_flags);
	int (*d_ioctl) (struct device * dev, int op, void *data);
  int (*d_poll) (struct device *dev, struct __wait_t *wait, int io_requests);
  void* private_data;
};

#define __dop_get_macro(_1,_2,_3,_4,name,...) name

#define dop_open(dev, open_flags)           ((dev)->d_open(dev, open_flags))
#define dop_close(dev)                      ((dev)->d_close(dev))
#define dop_io3(dev, iob, write)  ((dev)->d_io(dev, iob, write, 0))
#define dop_io4(dev, iob, write, io_flags)  ((dev)->d_io(dev, iob, write, io_flags))
#define dop_io(...) __dop_get_macro(__VA_ARGS__, dop_io4, dop_io3)(__VA_ARGS__)

#define dop_ioctl(dev, op, data)            ((dev)->d_ioctl(dev, op, data))
#define dop_poll(dev, wait, io_requests)    ((dev)->d_poll(dev, wait, io_requests))

#define dev_is_linux_dev(dev) ((dev)->linux_dentry != NULL)

#ifndef __NO_UCORE_DEVICE__
static inline void* dev_get_private_data(struct device* dev)
  __attribute__((always_inline));
static inline void dev_set_private_data(struct device* dev, void* private_data)
  __attribute__((always_inline));
#endif

void dev_init(void);
/* Create inode for a vfs-level device. */
struct inode *dev_create_inode(void);

#ifndef __NO_UCORE_DEVICE__
static inline void* dev_get_private_data(struct device* dev)
{
  return dev->private_data;
}

static inline void dev_set_private_data(struct device* dev, void* private_data)
{
  dev->private_data = private_data;
}
#endif

#endif /* !__KERN_FS_DEVS_DEV_H__ */
