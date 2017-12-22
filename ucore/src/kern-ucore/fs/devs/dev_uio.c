/*
 * Implementation of a UIO device. A UIO device is a device file that exposes
 * MMIO / Interrupt / DMA interface to an userspace application.
 */

//TODO: Follow the implementation of linux kernel.

#if defined(ARCH_ARM)

#include <types.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <inode.h>
#include <error.h>
#include <assert.h>
#include <slab.h>

struct uio_private_data {
  uintptr_t base_address;
  uintptr_t size;
};

// TODO, Hard-coded register size.
const static int UIO_REGISTER_BYTES = 4;

/* For open() */
static int uio_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

/* For close() */
static int uio_close(struct device *dev)
{
	return 0;
}

/* For dop_io() */
static int uio_io(struct device *dev, struct iobuf *iob, bool write)
{
  struct uio_private_data *private_data = dev_get_private_data(dev);
  uintptr_t base_address = private_data->base_address;
  uintptr_t size = private_data->size;
  // MMIO-based register access must be aligned,
  if(iob->io_len != UIO_REGISTER_BYTES || iob->io_offset % UIO_REGISTER_BYTES != 0 ||
        iob->io_offset < 0 || iob->io_offset >= size ) {
      return -E_INVAL;
  }
  uintptr_t register_address = base_address + iob->io_offset;
	if (write) {
    outw(register_address, *(uintptr_t*)iob->io_base);
	}
  else {
    (*(uintptr_t*)iob->io_base) = inw(register_address);
  }
  iob->io_resid = 0;
	return 0;
}

static int uio_ioctl(struct device *dev, int op, void *data)
{
	return -E_INVAL;
}

static void uio_device_init(struct device *dev, uintptr_t base_address, uintptr_t size)
{
  assert(size % UIO_REGISTER_BYTES == 0);
  assert(size > 0);
	memset(dev, 0, sizeof(*dev));
	dev->d_blocks = size / UIO_REGISTER_BYTES;
	dev->d_blocksize = UIO_REGISTER_BYTES;
	dev->d_open = uio_open;
	dev->d_close = uio_close;
	dev->d_io = uio_io;
	dev->d_ioctl = uio_ioctl;
  base_address = __ucore_ioremap(base_address, PGSIZE, 0); // ROUNDUP(size, PGSIZE)
  struct uio_private_data *private_data = kmalloc(sizeof(struct uio_private_data));
  private_data->base_address = base_address;
  private_data->size = size;
  dev_set_private_data(dev, private_data);
}

/*
 * Function to create and attach null:
 */
void dev_init_uio(void)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("UIO: dev_create_node.\n");
	}
	uio_device_init(vop_info(node, device), 0x43C00000, 16);

	int ret;
	if ((ret = vfs_add_dev("uio", node, 0)) != 0) {
		panic("UIO: vfs_add_dev: %e.\n", ret);
	}
}

#endif
