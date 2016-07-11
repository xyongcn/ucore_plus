#include <vfs.h>
#include <inode.h>
#include <error.h>
#include <stat.h>
#include <iobuf.h>
#include <stddef.h>

#include "lwip/sockets.h"
#include "socket_inode.h"

static int socket_inode_open(struct inode *node, uint32_t open_flags)
{
  panic("socket_inode_open called unexpectedly");
	return 0;
}

static int socket_inode_read(struct inode *node, struct iobuf *iob)
{
  panic("socket_inode_read not implemented");
  return 0;
}

static int socket_inode_write(struct inode *node, struct iobuf *iob)
{
  panic("socket_inode_write not implemented");
  return 0;
}

static int socket_inode_close(struct inode *node)
{
  struct socket_inode_private_data *private_data = (struct socket_inode_private_data*)node->private_data;
  int lwip_fd = private_data->lwip_socket;
  kfree(private_data);
  return lwip_close(lwip_fd);
}

const struct inode_ops socket_inode_ops = {
	.vop_magic = VOP_MAGIC,
	.vop_open = socket_inode_open,
	.vop_close = socket_inode_close,
	.vop_read = socket_inode_read,
	.vop_write = socket_inode_write,
	.vop_fstat = NULL_VOP_INVAL,
	.vop_fsync = NULL_VOP_INVAL,
	.vop_mkdir = NULL_VOP_INVAL,
	.vop_link = NULL_VOP_INVAL,
	.vop_rename = NULL_VOP_INVAL,
	.vop_readlink = NULL_VOP_INVAL,
	.vop_symlink = NULL_VOP_INVAL,
	.vop_namefile = NULL_VOP_INVAL,
	.vop_getdirentry = NULL_VOP_INVAL,
	.vop_reclaim = NULL_VOP_INVAL,
	.vop_ioctl = NULL_VOP_UNIMP,
	.vop_gettype = NULL_VOP_UNIMP,
	.vop_tryseek = NULL_VOP_INVAL,
	.vop_truncate = NULL_VOP_INVAL,
	.vop_create = NULL_VOP_INVAL,
	.vop_unlink = NULL_VOP_INVAL,
	.vop_lookup = NULL_VOP_INVAL,
	.vop_lookup_parent = NULL_VOP_INVAL,
};
