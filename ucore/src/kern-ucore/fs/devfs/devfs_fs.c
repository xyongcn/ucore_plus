#include <types.h>
#include <stdio.h>
#include <string.h>
#include <slab.h>
#include <list.h>
#include <fs.h>
#include <vfs.h>
#include <dev.h>
#include <inode.h>
#include <iobuf.h>
#include <bitmap.h>
#include <error.h>
#include <assert.h>
#include <kio.h>

static int devfs_mount(struct file_system_type* file_system_type, int flags,
const char *devname, void* data, struct fs** fs_store);

struct file_system_type devfs_fs_type = {
  .name = "devfs",
  .mount = devfs_mount
};

static int devfs_sync(struct fs *fs)
{
  return 0;
}

static struct inode *devfs_get_root(struct fs *fs)
{
  extern const struct inode_ops devfs_root_dirops;
  static struct inode *node = NULL;
  if(node != NULL) return node;
  node = alloc_inode(device);
	if(node != NULL) {
		vop_init(node, &devfs_root_dirops, fs);
	}
	return node;
}

static int devfs_cleanup(struct fs *fs)
{
  return -E_INVAL;
}

static int devfs_unmount(struct fs *fs)
{
  return -E_INVAL;
}

static int devfs_mount(struct file_system_type* file_system_type, int flags,
const char *devname, void* data, struct fs** fs_store)
{
  struct fs *fs;
  if ((fs = alloc_fs(sfs)) == NULL) {
    return -E_NO_MEM;
  }
	fs->fs_sync = devfs_sync;
	fs->fs_get_root = devfs_get_root;
	fs->fs_unmount = devfs_unmount;
	fs->fs_cleanup = devfs_cleanup;
  *fs_store = fs;
  return 0;
}
