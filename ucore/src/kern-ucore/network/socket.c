#include <stdlib.h>
#include <string.h>
#include <file.h>
#include "lwip/sockets.h"
#include "socket_inode.h"
#include "socket.h"

//TODO : Check user mem!
//TODO : Check addr length!

static void linux_sockaddr_to_lwip_sockaddr(struct linux_sockaddr *linux_sockaddr, struct sockaddr *lwip_sockaddr)
{
  lwip_sockaddr->sa_len = sizeof(struct sockaddr);
  lwip_sockaddr->sa_family = (uint8_t)linux_sockaddr->sa_family;
  memcpy(lwip_sockaddr->sa_data, linux_sockaddr->sa_data, 14);
}

int socket_create(int domain, int type, int protocol)
{
  int ret;
  struct file *file;
  if ((ret = filemap_alloc(NO_FD, &file)) < 0) {
    return ret;
  }
  struct inode *node = NULL;
  node = alloc_inode(device);
  assert(node != NULL);
  vop_init(node, &socket_inode_ops, NULL);
  file->pos = 0;
  file->node = node;
  file->readable = 1;
  file->writable = 1;
  filemap_open(file);
  struct socket_inode_private_data *private_data = kmalloc(sizeof(struct socket_inode_private_data));
  node->private_data = private_data;
  private_data->lwip_socket = lwip_socket(domain, type, protocol);
  vop_open_inc(node);
  vop_ref_inc(node);
  return file->fd;
}

int socket_connect(int fd, struct linux_sockaddr __user *uservaddr, int addrlen)
{
  int ret;
  struct file *file;
  if ((ret = fd2file(fd, &file)) != 0) {
    return ret;
  }
  int lwip_fd = ((struct socket_inode_private_data *)(file->node->private_data))->lwip_socket;
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  linux_sockaddr_to_lwip_sockaddr(uservaddr, lwip_sockaddr);
  ret = lwip_connect(lwip_fd, lwip_sockaddr, sizeof(struct sockaddr));
  kfree(lwip_sockaddr);
  return ret;
}

int socket_sendto(int fd, void __user *buff, size_t len, unsigned int flags, struct linux_sockaddr __user *addr, int addr_len)
{
  int ret;
  struct file *file;
  if ((ret = fd2file(fd, &file)) != 0) {
    return ret;
  }
  int lwip_fd = ((struct socket_inode_private_data *)(file->node->private_data))->lwip_socket;
  char* kernel_buff = kmalloc(len);
  memcpy(kernel_buff, buff, len);
  if(addr == NULL) {
    ret = lwip_send(lwip_fd, kernel_buff, len, flags);
  }
  else {
    struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
    linux_sockaddr_to_lwip_sockaddr(addr, lwip_sockaddr);
    ret = lwip_sendto(lwip_fd, kernel_buff, len, flags, lwip_sockaddr, sizeof(struct sockaddr));
    kfree(lwip_sockaddr);
  }
  //TODO: Not sure if lwip_sendto will free the buffer passed to it.
  kfree(kernel_buff);
  return ret;
}

int socket_recvfrom(int fd, void __user *ubuf, size_t size, unsigned int flags, struct linux_sockaddr __user *addr, int __user *addr_len)
{
  int ret;
  struct file *file;
  if ((ret = fd2file(fd, &file)) != 0) {
    return ret;
  }
  int lwip_fd = ((struct socket_inode_private_data *)(file->node->private_data))->lwip_socket;
  char* kernel_buff = kmalloc(size);
  if(addr == NULL) {
    ret = lwip_recv(lwip_fd, kernel_buff, size, flags);
  }
  else {
    int kernel_addr_len = *addr_len;
    struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
    linux_sockaddr_to_lwip_sockaddr(addr, lwip_sockaddr);
    ret = lwip_recvfrom(lwip_fd, kernel_buff, size, flags, lwip_sockaddr, &kernel_addr_len);
    *addr_len = kernel_addr_len;
    kfree(lwip_sockaddr);
  }
  memcpy(ubuf, kernel_buff, size);
  kfree(kernel_buff);
  return ret;
}
