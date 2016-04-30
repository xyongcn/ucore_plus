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

#define DISK_BLKSIZE                   PGSIZE
#define DISK_BUFSIZE                   (4 * DISK_BLKSIZE)
#define DISK_BLK_NSECT                 (DISK_BLKSIZE / SECTSIZE)

struct disk_private_data {
  int device_index;
  char* buffer;
  semaphore_t mutex;
};

static int disk_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int disk_close(struct device *dev)
{
	return 0;
}

static void disk_read_blks_nolock(int device_index, uint32_t blkno, uint32_t nblks, char* buffer)
{
	uint32_t sectno = blkno * DISK_BLK_NSECT;
  uint32_t nsecs = nblks * DISK_BLK_NSECT;
  int ret = ide_read_secs(device_index, sectno, buffer, nsecs);
  if(ret != 0) {
		panic("disk%d: read blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     device_index, blkno, sectno, nblks, nsecs, ret);
	}
}

static void disk_write_blks_nolock(int device_index, uint32_t blkno, uint32_t nblks, char* buffer)
{
  uint32_t sectno = blkno * DISK_BLK_NSECT;
  uint32_t nsecs = nblks * DISK_BLK_NSECT;
	int ret = ide_write_secs(device_index, sectno, buffer, nsecs);
  if(ret != 0) {
		panic("disk%d: write blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     device_index, blkno, sectno, nblks, nsecs, ret);
	}
}

static int disk_io(struct device *dev, struct iobuf *iob, bool write)
{
  struct disk_private_data *private_data = dev_get_private_data(dev);

  uint32_t device_index = private_data->device_index;
  char *buffer = private_data->buffer;
  semaphore_t *device_mutex = &private_data->mutex;
	off_t offset = iob->io_offset;
	size_t resid = iob->io_resid;
	uint32_t blkno = offset / DISK_BLKSIZE;
	uint32_t nblks = resid / DISK_BLKSIZE;

	/* don't allow I/O that isn't block-aligned */
	if ((offset % DISK_BLKSIZE) != 0 || (resid % DISK_BLKSIZE) != 0) {
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
		size_t copied, alen = DISK_BUFSIZE;
		if (write) {
			iobuf_move(iob, buffer, alen, 0, &copied);
			assert(copied != 0 && copied <= resid
			       && copied % DISK_BLKSIZE == 0);
			nblks = copied / DISK_BLKSIZE;
			disk_write_blks_nolock(device_index, blkno, nblks, buffer);
		} else {
			if (alen > resid) {
				alen = resid;
			}
			nblks = alen / DISK_BLKSIZE;
			disk_read_blks_nolock(device_index, blkno, nblks, buffer);
			iobuf_move(iob, buffer, alen, 1, &copied);
			assert(copied == alen && copied % DISK_BLKSIZE == 0);
		}
		resid -= copied, blkno += nblks;
	}
	up(device_mutex);
	return 0;
}

static int disk_ioctl(struct device *dev, int op, void *data)
{
	return -E_UNIMP;
}

static void disk_device_init(struct device *dev, int device_index)
{
	memset(dev, 0, sizeof(*dev));
	static_assert(DISK_BLKSIZE % SECTSIZE == 0);
	if (!ide_device_valid(device_index)) {
		panic("disk%d device isn't available.\n", device_index);
	}
	dev->d_blocks = ide_device_size(device_index) / DISK_BLK_NSECT;
	dev->d_blocksize = DISK_BLKSIZE;
	dev->d_open = disk_open;
	dev->d_close = disk_close;
	dev->d_io = disk_io;
	dev->d_ioctl = disk_ioctl;
  struct disk_private_data *private_data = kmalloc(sizeof(struct disk_private_data));
  private_data->device_index = device_index;
  private_data->buffer = kmalloc(DISK_BUFSIZE);
  if(private_data->buffer == NULL) {
    panic("disk%d alloc buffer failed.\n", device_index);
  }
  sem_init(&(private_data->mutex), 1);
  dev_set_private_data(dev, private_data);

	static_assert(DISK_BUFSIZE % DISK_BLKSIZE == 0);
}

void dev_init_disk(void)
{
  const int MAX_DISK_SCAN = 10;
  for(int i = 0; i < MAX_DISK_SCAN; i++) {
    if(!ide_device_valid(i)) continue;
    struct inode *node = dev_create_inode();
    if (node == NULL) {
      panic("disk%d: dev_create_node.\n", i);
    }
    disk_device_init(vop_info(node, device), i);
    char* device_name = kmalloc(16);
    //TODO: seems no sprintf is available for kernel?
    strcpy(device_name, "disk0");
    device_name[4] = '0' + i;
    int ret = vfs_add_dev(device_name, node, 1);
  	if (ret != 0) {
  		panic("disk0: vfs_add_dev: %e.\n", ret);
  	}
  }
}
