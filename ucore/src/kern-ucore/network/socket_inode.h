#include <types.h>
#include <inode.h>

struct socket_inode_private_data {
  int lwip_socket;
};

extern const const struct inode_ops socket_inode_ops;
