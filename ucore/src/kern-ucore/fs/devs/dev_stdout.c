#include <types.h>
#include <string.h>
#include <stdio.h>
#include <console.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <inode.h>
#include <unistd.h>
#include <error.h>
#include <assert.h>
#include <poll.h>

/* *
 * stdout is abstract device built on console
 * console is binding with serial, cga
 * the stdout io is simple, just write a Bit to serial or cga address
 * */
static int stdout_open(struct device *dev, uint32_t open_flags)
{
	if (open_flags != O_WRONLY) {
		return -E_INVAL;
	}
	return 0;
}

static int stdout_close(struct device *dev)
{
	return 0;
}

static int stdout_io(struct device *dev, struct iobuf *iob, bool write)
{
	if (write) {
		char *data = iob->io_base;
		for (; iob->io_resid != 0; iob->io_resid--) {
			cons_putc(*data++);
		}
		return 0;
	}
	return -E_INVAL;
}

static int stdout_ioctl(struct device *dev, int op, void *data)
{
	return -E_INVAL;
}

static int stdout_poll(struct device *dev, wait_t *wait, int io_requests)
{
  return io_requests & POLL_WRITE_AVAILABLE;
}

static void stdout_device_init(struct device *dev)
{
	memset(dev, 0, sizeof(*dev));
	dev->d_blocks = 0;
	dev->d_blocksize = 1;
	dev->d_open = stdout_open;
	dev->d_close = stdout_close;
	dev->d_io = stdout_io;
	dev->d_ioctl = stdout_ioctl;
  dev->d_poll = stdout_poll;
}

void dev_init_stdout(void)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("stdout: dev_create_node.\n");
	}
	stdout_device_init(vop_info(node, device));

	int ret;
	if ((ret = vfs_add_dev("stdout", node, 0)) != 0) {
		panic("stdout: vfs_add_dev: %e.\n", ret);
	}
}
