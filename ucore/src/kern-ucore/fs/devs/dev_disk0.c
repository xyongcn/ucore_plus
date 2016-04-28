#include <types.h>
#include <mmu.h>
#include <slab.h>
#include <sem.h>
#include <ide.h>
#include <inode.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <error.h>
#include <assert.h>

#define DISK0_BLKSIZE                   PGSIZE
#define DISK0_BUFSIZE                   (4 * DISK0_BLKSIZE)
#define DISK0_BLK_NSECT                 (DISK0_BLKSIZE / SECTSIZE)

struct disk_private_data {
  int device_index;
  char* buffer;
  semaphore_t mutex;
};

static int disk0_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int disk0_close(struct device *dev)
{
	return 0;
}

static void disk0_read_blks_nolock(int device_index, uint32_t blkno, uint32_t nblks, char* buffer)
{
	uint32_t sectno = blkno * DISK0_BLK_NSECT;
  uint32_t nsecs = nblks * DISK0_BLK_NSECT;
  int ret = ide_read_secs(device_index, sectno, buffer, nsecs);
  if(ret != 0) {
		panic("disk%d: read blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     device_index, blkno, sectno, nblks, nsecs, ret);
	}
}

static void disk0_write_blks_nolock(int device_index, uint32_t blkno, uint32_t nblks, char* buffer)
{
  uint32_t sectno = blkno * DISK0_BLK_NSECT;
  uint32_t nsecs = nblks * DISK0_BLK_NSECT;
	int ret = ide_write_secs(device_index, sectno, buffer, nsecs);
  if(ret != 0) {
		panic("disk%d: write blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     device_index, blkno, sectno, nblks, nsecs, ret);
	}
}

static int disk0_io(struct device *dev, struct iobuf *iob, bool write)
{
  struct disk_private_data *private_data = dev_get_private_data(dev);

  uint32_t device_index = private_data->device_index;
  char *buffer = private_data->buffer;
  semaphore_t *device_mutex = &private_data->mutex;
	off_t offset = iob->io_offset;
	size_t resid = iob->io_resid;
	uint32_t blkno = offset / DISK0_BLKSIZE;
	uint32_t nblks = resid / DISK0_BLKSIZE;

	/* don't allow I/O that isn't block-aligned */
	if ((offset % DISK0_BLKSIZE) != 0 || (resid % DISK0_BLKSIZE) != 0) {
		return -E_INVAL;
	}

	/* don't allow I/O past the end of disk0 */
	if (blkno + nblks > dev->d_blocks) {
		return -E_INVAL;
	}

	/* read/write nothing ? */
	if (nblks == 0) {
		return 0;
	}

	down(device_mutex);
	while (resid != 0) {
		size_t copied, alen = DISK0_BUFSIZE;
		if (write) {
			iobuf_move(iob, buffer, alen, 0, &copied);
			assert(copied != 0 && copied <= resid
			       && copied % DISK0_BLKSIZE == 0);
			nblks = copied / DISK0_BLKSIZE;
			disk0_write_blks_nolock(device_index, blkno, nblks, buffer);
		} else {
			if (alen > resid) {
				alen = resid;
			}
			nblks = alen / DISK0_BLKSIZE;
			disk0_read_blks_nolock(device_index, blkno, nblks, buffer);
			iobuf_move(iob, buffer, alen, 1, &copied);
			assert(copied == alen && copied % DISK0_BLKSIZE == 0);
		}
		resid -= copied, blkno += nblks;
	}
	up(device_mutex);
	return 0;
}

static int disk0_ioctl(struct device *dev, int op, void *data)
{
	return -E_UNIMP;
}

static void disk0_device_init(struct device *dev, int device_index)
{
	memset(dev, 0, sizeof(*dev));
	static_assert(DISK0_BLKSIZE % SECTSIZE == 0);
	if (!ide_device_valid(device_index)) {
		panic("disk%d device isn't available.\n", device_index);
	}
	dev->d_blocks = ide_device_size(device_index) / DISK0_BLK_NSECT;
	dev->d_blocksize = DISK0_BLKSIZE;
	dev->d_open = disk0_open;
	dev->d_close = disk0_close;
	dev->d_io = disk0_io;
	dev->d_ioctl = disk0_ioctl;
  struct disk_private_data *private_data = kmalloc(sizeof(struct disk_private_data));
  private_data->device_index = device_index;
  private_data->buffer = kmalloc(DISK0_BUFSIZE);
  if(private_data->buffer == NULL) {
    panic("disk%d alloc buffer failed.\n", device_index);
  }
  sem_init(&(private_data->mutex), 1);
  dev_set_private_data(dev, private_data);

	static_assert(DISK0_BUFSIZE % DISK0_BLKSIZE == 0);
	/*if ((disk0_buffer = kmalloc(DISK0_BUFSIZE)) == NULL) {
		panic("disk0 alloc buffer failed.\n");
	}*/
}

void dev_init_disk0(void)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("disk0: dev_create_node.\n");
	}
	disk0_device_init(vop_info(node, device), 2);

	int ret;
	if ((ret = vfs_add_dev("disk0", node, 1)) != 0) {
		panic("disk0: vfs_add_dev: %e.\n", ret);
	}
}
