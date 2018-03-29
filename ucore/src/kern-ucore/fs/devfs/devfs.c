#include <types.h>
#include <sfs.h>
#include <error.h>
#include <assert.h>
#include <vfs.h>
#include <mm/slab.h>

#include "devfs.h"

list_entry_t devfs_device_list;

void devfs_init(void)
{
	int ret = register_filesystem(&devfs_fs_type);
	if (ret != 0) {
		panic("failed: devfs: register_filesystem: %e.\n", ret);
	}
  list_init(&devfs_device_list);
  return ret;
}

int devfs_register_device(const char *devname, struct inode *devnode, bool mountable)
{
  if(strcmp("dev", devname) == 0) return 0;
  struct devfs_device *device = kmalloc(sizeof(struct devfs_device));
  device->name = devname;
  device->inode = devnode;
  device->mountable = mountable;
  list_add(&devfs_device_list, &device->list_entry);
  return 0;
}
