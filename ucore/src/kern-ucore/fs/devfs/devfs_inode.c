#include <vfs.h>
#include <inode.h>
#include <error.h>
#include <stat.h>
#include <iobuf.h>

static int devfs_opendir(struct inode *node, uint32_t open_flags)
{
	switch (open_flags & O_ACCMODE) {
	case O_RDONLY:
		break;
	case O_WRONLY:
	case O_RDWR:
	default:
		return -E_ISDIR;
	}
	if (open_flags & O_APPEND) {
		return -E_ISDIR;
	}
	return 0;
}

static int devfs_openfile(struct inode *node, uint32_t open_flags)
{
	return 0;
}

static int devfs_close(struct inode *node)
{
	return 0;
}

static int devfs_fstat(struct inode *node, struct stat *stat)
{
	memset(stat, 0, sizeof(struct stat));
	stat->st_nlinks = 1;
	stat->st_blocks = 0;
	stat->st_size = 0;
  stat->st_mode = S_IFDIR;
	return 0;
}

static int devfs_fsync(struct inode *node)
{
  return 0;
}

static int devfs_namefile(struct inode *node, struct iobuf *iob)
{
  //TODO:
  return 0;
}

static int devfs_getdirentry(struct inode *node, struct iobuf *iob)
{
  int ret = 0;
  const static int DIR_ENTRY_MAX_LENGTH = 256;
  off_t offset = iob->io_offset;
  if (offset < 0 || offset % DIR_ENTRY_MAX_LENGTH != 0) {
    return -E_INVAL;
  }

  int slot = offset / DIR_ENTRY_MAX_LENGTH;
  static char file_name[256];
  switch (slot) {
    case 0:
      strcpy(file_name, ".");
      break;
    case 1:
      strcpy(file_name, "..");
      break;
    default:
      return -E_NOENT;
  }
  ret = iobuf_move(iob, file_name, DIR_ENTRY_MAX_LENGTH, 1, NULL);
	//TODO:
	return 0;
}

static int devfs_reclaim(struct inode *node)
{
	return 0;
}

static int devfs_lookup(struct inode *node, char *path, struct inode **node_store)
{
  //TODO: need to implement real device lookup.
  vop_ref_inc(node);
  (*node_store) = node;
  return 0;
}

static int devfs_gettype(struct inode *node, uint32_t * type_store)
{
  *type_store = S_IFDIR;
  return 0;
}

static int devfs_lookup_parent(
struct inode *node, char *path, struct inode **node_store, char **endp)
{
  //TODO: need to implement real device lookup.
  panic("");
  return 0;
}

const struct inode_ops devfs_root_dirops = {
	.vop_magic = VOP_MAGIC,
	.vop_open = devfs_opendir,
	.vop_close = devfs_close,
	.vop_read = NULL_VOP_ISDIR,
	.vop_write = NULL_VOP_ISDIR,
	.vop_fstat = devfs_fstat,
	.vop_fsync = devfs_fsync,
	.vop_mkdir = NULL_VOP_INVAL,
	.vop_link = NULL_VOP_INVAL,
	.vop_rename = NULL_VOP_INVAL,
	.vop_readlink = NULL_VOP_ISDIR,
	.vop_symlink = NULL_VOP_UNIMP,
	.vop_namefile = devfs_namefile,
	.vop_getdirentry = devfs_getdirentry,
	.vop_reclaim = devfs_reclaim,
	.vop_ioctl = NULL_VOP_INVAL,
	.vop_gettype = devfs_gettype,
	.vop_tryseek = NULL_VOP_ISDIR,
	.vop_truncate = NULL_VOP_ISDIR,
	.vop_create = NULL_VOP_INVAL,
	.vop_unlink = NULL_VOP_INVAL,
	.vop_lookup = devfs_lookup,
	.vop_lookup_parent = devfs_lookup_parent,
};
