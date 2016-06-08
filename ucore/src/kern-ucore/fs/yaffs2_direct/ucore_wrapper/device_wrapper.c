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
#define SPARE_SIZE	64
#define PAGE_SIZE	(DATA_SIZE + SPARE_SIZE)

#define PAGES_PER_BLOCK	4

static int yaffs_ucore_device_wrapper_erase(struct yaffs_dev *dev, unsigned blockId)
{
  kprintf("Erasing blockID:%d\n", blockId);
  struct device *ucore_dev = (struct device*)dev->os_context;
  char* disk_buffer = kmalloc(4096, 0);
  memset(disk_buffer, 0xFF, 4096);
  for(int i = (blockId - 1) * PAGES_PER_BLOCK; i < blockId * PAGES_PER_BLOCK; i++) {
    struct iobuf iob;
    iob.io_base = disk_buffer;
    iob.io_offset = i * 4096;
    iob.io_len = 4096;
    iob.io_resid = 4096;
    dop_io(ucore_dev, &iob, 1);
  }
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
  *eccStatus = 0;
  char* disk_buffer = kmalloc(4096, 0);
  struct device *ucore_dev = (struct device*)dev->os_context;
  off_t beginPos = (pageId - PAGES_PER_BLOCK) * 4096;
  struct iobuf iob;
  iob.io_base = disk_buffer;
  iob.io_offset = beginPos;
  iob.io_len = 4096;
  iob.io_resid = 4096;
  dop_io(ucore_dev, &iob, 0);
  if(dataLength > 0) {
    //kprintf("RDSPR\n");
    memcpy(data, disk_buffer, dataLength);
  }
  if(spareLength > 0) {
    memcpy(spare, disk_buffer + 2048, spareLength);
  }
  //panic("rd");
  kfree(disk_buffer);
  return YAFFS_OK;
}

static int yaffs_ucore_device_wrapper_wr_chunk(struct yaffs_dev *dev,unsigned pageId,
					   const unsigned char *data, unsigned dataLength,
					   const unsigned char *spare, unsigned spareLength)
{
  char* disk_buffer = kmalloc(4096, 0);
  struct device *ucore_dev = (struct device*)dev->os_context;
  off_t beginPos = (pageId - PAGES_PER_BLOCK) * 4096;
  struct iobuf iob;
  iob.io_base = disk_buffer;
  iob.io_offset = beginPos;
  iob.io_len = 4096;
  iob.io_resid = 4096;
  dop_io(ucore_dev, &iob, 0);
  if(dataLength > 0) {
    //kprintf("RDSPR\n");
    memcpy(disk_buffer, data, dataLength);
  }
  if(spareLength > 0) {
    memcpy(disk_buffer + 2048, spare, spareLength);
  }
  iob.io_base = disk_buffer;
  iob.io_offset = beginPos;
  iob.io_len = 4096;
  iob.io_resid = 4096;
  dop_io(ucore_dev, &iob, 1);
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

struct yaffs_dev *yaffs_ucore_device_wrapper_create(
struct device* device, char* device_name) {
  struct yaffs_dev *ret = kmalloc(sizeof(struct yaffs_dev), 0);
  ynandif_Geometry *geometry = kmalloc(sizeof(ynandif_Geometry), 0);
  if(ret == NULL) {
    return NULL;
  }
  ret->os_context = device;
  //ret->

  geometry->start_block = 1;
  geometry->end_block = 128;//device->d_blocks * 2 / PAGES_PER_BLOCK;
  geometry->dataSize = DATA_SIZE;
  geometry->spareSize= SPARE_SIZE;
  geometry->pagesPerBlock = PAGES_PER_BLOCK;//PAGES_PER_BLOCK;
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
