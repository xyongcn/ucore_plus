#ifndef __KERN_FS_DEVFS_DEVFS_H__
#define __KERN_FS_DEVFS_DEVFS_H__

#include <vfs.h>

/*
 * TODO: In the current design of devfs, sync is not considered. However,
 * I'm not sure if it's possible for multiple process to access devfs_register_device.
 * If that might happen, a spinlock/mutex is required to protect the list.
 */

extern struct file_system_type devfs_fs_type;

struct devfs_device {
  const char *name;
  struct inode *inode;
  bool mountable;
  list_entry_t list_entry;
};

extern list_entry_t devfs_device_list;

void devfs_init(void);
int devfs_mount(const char *devname);
struct inode *devfs_get_root_inode(void);
struct fs* devfs_getfs();
int devfs_register_device(const char *devname, struct inode *devnode, bool mountable);

#endif // __KERN_FS_DEVFS_DEVFS_H__
