#ifdef ARCH_ARM
#include <board.h>
#endif

#include <dev.h>
#include <slab.h>
#include <string.h>
#include <iobuf.h>

#include "device_wrapper.h"

#include "../yaffs_nandif.h"

#define DATA_SIZE	2048
#define SPARE_SIZE 64
#define PAGE_SIZE	(DATA_SIZE + SPARE_SIZE)
#define PAGES_PER_BLOCK	62
#define DEVICE_BLOCK_SIZE 4096
#define DEVICE_BLOCK_PER_FLASH_BLOCK 32

static int yaffs_ucore_device_wrapper_erase(struct yaffs_dev *dev, unsigned blockId)
{
  //if(blockId < 63) panic("No need to erase %d\n", blockId);
  struct device *ucore_dev = (struct device*)dev->os_context;
  char* disk_buffer = kmalloc(DEVICE_BLOCK_SIZE, 0);
  memset(disk_buffer, 0xFF, DEVICE_BLOCK_SIZE);
  for(int i = blockId * DEVICE_BLOCK_PER_FLASH_BLOCK;
  i < (blockId + 1) * DEVICE_BLOCK_PER_FLASH_BLOCK; i++) {
    struct iobuf iob;
    iob.io_base = disk_buffer;
    iob.io_offset = i * DEVICE_BLOCK_SIZE;
    iob.io_len = DEVICE_BLOCK_SIZE;
    iob.io_resid = DEVICE_BLOCK_SIZE;
    dop_io(ucore_dev, &iob, 1);
  }
  kfree(disk_buffer);
	return 1;
}

static int yaffs_ucore_device_wrapper_initialise(struct yaffs_dev *dev)
{
  struct device *ucore_dev = (struct device*)dev->os_context;
  int ret = dop_open(ucore_dev, 0) == 0;
  return ret;
}

static int yaffs_ucore_device_wrapper_deinitialise(struct yaffs_dev *dev)
{
  struct device *ucore_dev = (struct device*)dev->os_context;
  return dop_close(ucore_dev) == 0;
}

static int yaffs_ucore_device_wrapper_rd_chunk(
            struct yaffs_dev *dev, unsigned pageId,
					  unsigned char *data, unsigned dataLength,
					  unsigned char *spare, unsigned spareLength,
					  int *eccStatus)
{
  assert(dataLength <= DATA_SIZE);
  assert(spareLength <= SPARE_SIZE);
  *eccStatus = 0;
  int block_id = pageId / PAGES_PER_BLOCK;
  int page_in_block = pageId % PAGES_PER_BLOCK;
  int data_block_offset = block_id * DEVICE_BLOCK_PER_FLASH_BLOCK * DEVICE_BLOCK_SIZE +
    page_in_block / 2 * DEVICE_BLOCK_SIZE;
  int data_in_block_offset = page_in_block % 2 == 0 ? 0 : 2048;
  int spare_block_offset = block_id * DEVICE_BLOCK_PER_FLASH_BLOCK * DEVICE_BLOCK_SIZE +
    (DEVICE_BLOCK_PER_FLASH_BLOCK - 1) * DEVICE_BLOCK_SIZE;
  int spare_in_block_offset = page_in_block * SPARE_SIZE;

  char* disk_buffer = kmalloc(DEVICE_BLOCK_SIZE, 0);
  struct device *ucore_dev = (struct device*)dev->os_context;
  struct iobuf iob;
  if(dataLength > 0) {
    iobuf_init(&iob, disk_buffer, DEVICE_BLOCK_SIZE, data_block_offset);
    dop_io(ucore_dev, &iob, 0);
    memcpy(data, disk_buffer + data_in_block_offset, dataLength);
  }
  if(spareLength > 0) {
    iobuf_init(&iob, disk_buffer, DEVICE_BLOCK_SIZE, spare_block_offset);
    dop_io(ucore_dev, &iob, 0);
    memcpy(spare, disk_buffer + spare_in_block_offset, spareLength);
  }
  kfree(disk_buffer);
  return YAFFS_OK;
}

static int yaffs_ucore_device_wrapper_wr_chunk(struct yaffs_dev *dev,unsigned pageId,
					   const unsigned char *data, unsigned dataLength,
					   const unsigned char *spare, unsigned spareLength)
{
  assert(dataLength <= DATA_SIZE);
  assert(spareLength <= SPARE_SIZE);
  int block_id = pageId / PAGES_PER_BLOCK;
  //if(block_id < 63) panic("No need to write %d\n", block_id);
  int page_in_block = pageId % PAGES_PER_BLOCK;
  int data_block_offset = block_id * DEVICE_BLOCK_PER_FLASH_BLOCK * DEVICE_BLOCK_SIZE +
    page_in_block / 2 * DEVICE_BLOCK_SIZE;
  int data_in_block_offset = page_in_block % 2 == 0 ? 0 : 2048;
  int spare_block_offset = block_id * DEVICE_BLOCK_PER_FLASH_BLOCK * DEVICE_BLOCK_SIZE +
    (DEVICE_BLOCK_PER_FLASH_BLOCK - 1) * DEVICE_BLOCK_SIZE;
  int spare_in_block_offset = page_in_block * SPARE_SIZE;

  char* disk_buffer = kmalloc(DEVICE_BLOCK_SIZE, 0);
  struct device *ucore_dev = (struct device*)dev->os_context;
  struct iobuf iob;
  if(dataLength > 0) {
    iobuf_init(&iob, disk_buffer, DEVICE_BLOCK_SIZE, data_block_offset);
    dop_io(ucore_dev, &iob, 0);
    memcpy(disk_buffer + data_in_block_offset, data, dataLength);
    iobuf_init(&iob, disk_buffer, DEVICE_BLOCK_SIZE, data_block_offset);
    dop_io(ucore_dev, &iob, 1);
  }
  if(spareLength > 0) {
    iobuf_init(&iob, disk_buffer, DEVICE_BLOCK_SIZE, spare_block_offset);
    dop_io(ucore_dev, &iob, 0);
    memcpy(disk_buffer + spare_in_block_offset, spare, spareLength);
    iobuf_init(&iob, disk_buffer, DEVICE_BLOCK_SIZE, spare_block_offset);
    dop_io(ucore_dev, &iob, 1);
  }
  kfree(disk_buffer);
  return YAFFS_OK;
}

static int yaffs_ucore_device_wrapper_check_block_ok(struct yaffs_dev *dev,unsigned blockId)
{
  //TODO: A general device may not have "bad block" mark.
  return 1;
}

static int yaffs_ucore_device_wrapper_mark_block_bad(struct yaffs_dev *dev,unsigned blockId)
{
  //TODO: A general device may not have "bad block" mark.
  return 0;
}

struct yaffs_dev *yaffs_ucore_device_wrapper_create(struct device* device, char* device_name)
{
  if(device -> d_blocksize != 4096) {
    kprintf("YAFFS2 : block device wrapper requires blocksize to be 4KB\n");
    return NULL;
  }
  kprintf("YAFFS2 : Emulating MTD device using 4K block devices.\n");
  kprintf("    Using every 128K to create 62 (2K+64B) FlashROM page.\n");
  kprintf("    Note those devices have to be initialized with 0xFF.\n");
  struct yaffs_dev *ret = kmalloc(sizeof(struct yaffs_dev), 0);
  ynandif_Geometry *geometry = kmalloc(sizeof(ynandif_Geometry), 0);
  if(ret == NULL || geometry == NULL) {
    panic("YAFFS2 : Kernel memory used up.\n");
  }
  ret->os_context = device;
  int emulation_blocks = device->d_blocks / DEVICE_BLOCK_PER_FLASH_BLOCK;
  int emulation_page_count = emulation_blocks * 62;
  geometry->start_block = 0;
  geometry->end_block = emulation_blocks - 1;
  geometry->dataSize = DATA_SIZE;
  geometry->spareSize= SPARE_SIZE;
  geometry->pagesPerBlock = PAGES_PER_BLOCK;
  geometry->hasECC = 1;
  geometry->inband_tags = 0;
  geometry->useYaffs2 = 1;
  geometry->initialise = yaffs_ucore_device_wrapper_initialise;
  geometry->deinitialise = yaffs_ucore_device_wrapper_deinitialise;
  geometry->readChunk = yaffs_ucore_device_wrapper_rd_chunk;
  geometry->writeChunk = yaffs_ucore_device_wrapper_wr_chunk;
  geometry->eraseBlock = yaffs_ucore_device_wrapper_erase;
  geometry->checkBlockOk = yaffs_ucore_device_wrapper_check_block_ok;
  geometry->markBlockBad = yaffs_ucore_device_wrapper_mark_block_bad;

  yaffs_add_dev_from_geometry(ret, device_name, geometry);
  ret->os_context = device;
  return ret;
}
