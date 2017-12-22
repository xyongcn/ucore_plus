#include <types.h>
#include <stdio.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <inode.h>
#include <unistd.h>
#include <error.h>
#include <assert.h>
#include <string.h>
#include <poll.h>
#include <wait.h>
#include <mersenne_twister_random.h>

static int urandom_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int urandom_close(struct device *dev)
{
	return 0;
}

static int urandom_io(struct device *dev, struct iobuf *iob, bool write)
{
	if (write) {
    iob->io_resid = 0;
		return 0;
	}
  else {
		int ret = iob->io_resid;
		for (int i = 0; iob->io_resid != 0; iob->io_resid--, i++) {
      ((uint8_t*)iob->io_base)[i] = (uint8_t)mersenne_twister_generate();
		}
		return ret;
	}
}

static int urandom_ioctl(struct device *dev, int op, void *data)
{
	return -E_INVAL;
}

static int urandom_poll(struct device *dev, wait_t *wait, int io_requests)
{
  return io_requests & (POLL_READ_AVAILABLE | POLL_WRITE_AVAILABLE);
}

static void urandom_device_init(struct device *dev)
{
	memset(dev, 0, sizeof(*dev));
	dev->d_blocks = 0;
	dev->d_blocksize = 1;
	dev->d_open = urandom_open;
	dev->d_close = urandom_close;
	dev->d_io = urandom_io;
	dev->d_ioctl = urandom_ioctl;
  dev->d_poll = urandom_poll;
  mersenne_twister_set_seed(4357U);
}

void dev_init_urandom(void)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("urandom: dev_create_node.\n");
	}
	urandom_device_init(vop_info(node, device));

	int ret;
	if ((ret = vfs_add_dev("urandom", node, 0)) != 0) {
		panic("urandom: vfs_add_dev: %e.\n", ret);
	}
}
