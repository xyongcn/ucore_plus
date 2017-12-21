#include <vfs.h>
#include <inode.h>
#include <error.h>
#include <stat.h>
#include <iobuf.h>
#include <string.h>
#include <stddef.h>

#define iovec iovec
#include "lwip/sockets.h"
#include "socket_inode.h"

static int socket_inode_open(struct inode *node, uint32_t open_flags)
{
  panic("socket_inode_open called unexpectedly");
	return 0;
}

static int socket_inode_read(struct inode *node, struct iobuf *iob)
{
  struct socket_inode_private_data *private_data = (struct socket_inode_private_data*)node->private_data;
  int lwip_fd = private_data->lwip_socket;
  char* kernel_buff = kmalloc(iob->io_len);
  int ret = lwip_recvfrom(lwip_fd, kernel_buff, iob->io_len, 0, NULL, NULL);
  memcpy(iob->io_base, kernel_buff, iob->io_len);
  kfree(kernel_buff);
  if(ret < 0) return ret;
  iob->io_resid -= ret;
  return 0;
}

static int socket_inode_write(struct inode *node, struct iobuf *iob)
{
  struct socket_inode_private_data *private_data = (struct socket_inode_private_data*)node->private_data;
  int lwip_fd = private_data->lwip_socket;
  char* kernel_buff = kmalloc(iob->io_len);
  memcpy(kernel_buff, iob->io_base, iob->io_len);
  int ret = lwip_sendto(lwip_fd, kernel_buff, iob->io_len, 0, NULL, 0);
  kfree(kernel_buff);
  if(ret < 0) return ret;
  iob->io_resid -= ret;
  return 0;
}

static int socket_inode_close(struct inode *node)
{
  struct socket_inode_private_data *private_data = (struct socket_inode_private_data*)node->private_data;
  int lwip_fd = private_data->lwip_socket;
  kfree(private_data);
  return lwip_close(lwip_fd);
}

static int socket_inode_gettype(struct inode *node, uint32_t *type_store)
{
  *type_store = S_IFSOCK;
  return 0;
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
	.vop_gettype = socket_inode_gettype,
	.vop_tryseek = NULL_VOP_INVAL,
	.vop_truncate = NULL_VOP_INVAL,
	.vop_create = NULL_VOP_INVAL,
	.vop_unlink = NULL_VOP_INVAL,
	.vop_lookup = NULL_VOP_INVAL,
	.vop_lookup_parent = NULL_VOP_INVAL,
};
