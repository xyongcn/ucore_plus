#include <types.h>
#include <string.h>
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
#include <thinpad_flashrom.h>

#define DISK_BLKSIZE PGSIZE

//Considering that Thinpad FlashROM have only 64 sectors, while each sector has
//a huge size of 128KBytes, only one sector will be loaded to memory at a time.
struct thinpad_flashrom_private_data {
  char* cache;
  bool cache_dirty;
  int cached_sector;
  semaphore_t mutex;
};

static struct device* _device = NULL;

void thinpad_flashrom_swap_cache(struct device *dev, int new_cached_sector)
{
  assert(new_cached_sector >= 0 && new_cached_sector < THINFLASH_NR_SECTOR);
  struct thinpad_flashrom_private_data *private_data =
    (struct thinpad_flashrom_private_data*)dev_get_private_data(dev);
  kprintf("thinpad_flashrom_swap_cache, from %d to %d\n", private_data->cached_sector, new_cached_sector);
  down(&private_data->mutex);
  if(private_data->cache_dirty) {
    kprintf("It is dirty.\n");
    thinpad_flashrom_write(private_data->cached_sector, private_data->cache, 1);
  }
  if(private_data->cached_sector != new_cached_sector) {
    private_data->cache_dirty = 0;
    private_data->cached_sector = new_cached_sector;
    thinpad_flashrom_read(private_data->cached_sector, private_data->cache, 1);
  }
  up(&private_data->mutex);
}

void thinpad_flashrom_sync()
{
  struct thinpad_flashrom_private_data *private_data =
    (struct thinpad_flashrom_private_data*)dev_get_private_data(_device);
  if(private_data->cache_dirty) {
    thinpad_flashrom_swap_cache(_device, private_data->cached_sector);
  }
}

static int thinpad_flashrom_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int thinpad_flashrom_close(struct device *dev)
{
	return 0;
}

static void disk_io_in_sector(struct device *dev, struct iobuf *iob, bool write, int io_bytes)
{
  struct thinpad_flashrom_private_data *private_data =
    (struct thinpad_flashrom_private_data*)dev_get_private_data(dev);
  char* cache = private_data->cache;
	off_t offset = iob->io_offset;
  int flash_rom_sector = offset / THINFLASH_SECTOR_SIZE;
  if(flash_rom_sector != private_data->cached_sector) {
    thinpad_flashrom_swap_cache(dev, flash_rom_sector);
  }
  int flash_rom_sector_offset = offset % THINFLASH_SECTOR_SIZE;
  down(&private_data->mutex);
  if(write) {
    iobuf_move(iob, cache + flash_rom_sector_offset, io_bytes, 0, NULL);
    private_data->cache_dirty = 1;
  }
  else {
    iobuf_move(iob, cache + flash_rom_sector_offset, io_bytes, 1, NULL);
  }
  up(&private_data->mutex);
}

static int disk_io(struct device *dev, struct iobuf *iob, bool write)
{
  kprintf("Thinpad I/O, size = %x, write = %d\n", iob->io_resid, write);
	off_t offset = iob->io_offset;
	int resid = iob->io_resid;
  // don't allow I/O that isn't block-aligned
  if ((offset % DISK_BLKSIZE) != 0 || (resid % DISK_BLKSIZE) != 0) {
    return -E_INVAL;
  }
  while(resid > 0) {
    int io_bytes = (offset / THINFLASH_SECTOR_SIZE + 1) * THINFLASH_SECTOR_SIZE - offset;
    if(io_bytes > resid) io_bytes = resid;
    resid -= io_bytes;
    offset += io_bytes;
    kprintf("Thinpad I/O sub, size = %x\n", io_bytes);
    disk_io_in_sector(dev, iob, write, io_bytes);
  }
	return 0;
}

static int thinpad_flashrom_ioctl(struct device *dev, int op, void *data)
{
	return -E_UNIMP;
}

static void thinpad_flashrom_device_init(struct device *dev)
{
  _device = dev;
	memset(dev, 0, sizeof(*dev));
	dev->d_blocksize = DISK_BLKSIZE;
  static_assert(THINFLASH_SIZE % DISK_BLKSIZE == 0);
  dev->d_blocks = THINFLASH_SIZE / DISK_BLKSIZE;
	dev->d_open = thinpad_flashrom_open;
	dev->d_close = thinpad_flashrom_close;
	dev->d_io = disk_io;
	dev->d_ioctl = thinpad_flashrom_ioctl;
  struct thinpad_flashrom_private_data *private_data =
  kmalloc(sizeof(struct thinpad_flashrom_private_data));
  private_data->cache_dirty = 0;
  private_data->cached_sector = -1;
  private_data->cache = kmalloc(THINFLASH_SECTOR_SIZE);
  if(private_data->cache == NULL) {
    panic("ThinpadFlashrom : alloc cache failed.\n");
  }
  sem_init(&(private_data->mutex), 1);
  dev_set_private_data(dev, private_data);
}

void dev_init_thinpad_flashrom(void)
{
  struct inode *node = dev_create_inode();
  if (node == NULL) {
    panic("ThinpadFlashrom : Cannot create device inode.\n");
  }
  thinpad_flashrom_device_init(vop_info(node, device));
  int ret = vfs_add_dev("thinpad_flashrom", node, 1);
	if (ret != 0) {
		panic("ThinpadFlashrom: Cannot add device to vfs, error = %d\n", ret);
	}
}
