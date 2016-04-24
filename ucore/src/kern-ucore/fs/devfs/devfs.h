#ifndef __KERN_FS_DEVFS_DEVFS_H__
#define __KERN_FS_DEVFS_DEVFS_H__

void devfs_init(void);
int devfs_mount(const char *devname);
struct inode *devfs_get_root_inode(void);
struct fs* devfs_getfs();

#endif // __KERN_FS_DEVFS_DEVFS_H__
