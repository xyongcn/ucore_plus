#include <stdlib.h>
#include <string.h>
#include <fd_set.h>
#include <file.h>
#include <error.h>
#include <stat.h>
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

static int socket_ucore_fd_to_lwip_fd(int ucore_fd, int *lwip_fd)
{
  int ret;
  struct file *file;
  if ((ret = fd2file(ucore_fd, &file)) != 0) {
    return -E_BADF;
  }
  int fd_type;
  if(vop_gettype(file->node, &fd_type) != 0 || !S_ISSOCK(fd_type)) {
    return -E_NOTSOCK;
  }
  *lwip_fd = ((struct socket_inode_private_data *)(file->node->private_data))->lwip_socket;
  return 0;
}

int socket_create(int domain, int type, int protocol)
{
  int ret;
  struct file *file;
  if ((ret = filemap_alloc(NO_FD, &file)) < 0) {
    return -E_BADF;
  }
  struct inode *node = NULL;
  node = alloc_inode(default_inode);
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
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  linux_sockaddr_to_lwip_sockaddr(uservaddr, lwip_sockaddr);
  ret = lwip_connect(lwip_fd, lwip_sockaddr, sizeof(struct sockaddr));
  kfree(lwip_sockaddr);
  return ret;
}

int socket_sendto(int fd, void __user *buff, size_t len, unsigned int flags, struct linux_sockaddr __user *addr, int addr_len)
{
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
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
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
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

int socket_set_option(int fd, int level, int optname, char __user *optval, int optlen)
{
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) {
    return ret;
  }
  return lwip_setsockopt(lwip_fd, level, optname, optval, optlen);
}

int socket_get_option(int fd, int level, int optname, char __user *optval, int *optlen)
{
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  return lwip_getsockopt(lwip_fd, level, optname, optval, optlen);
}

int socket_lwip_select_wrapper(int nfds, linux_fd_set_t *readfds, linux_fd_set_t *writefds,
  linux_fd_set_t *exceptfds, struct linux_timeval *timeout)
{
  fd_set *lwip_readfds = kmalloc(sizeof(fd_set));
  fd_set *lwip_writefds = kmalloc(sizeof(fd_set));
  fd_set *lwip_exceptfds = kmalloc(sizeof(fd_set));
  FD_ZERO(lwip_readfds);
  FD_ZERO(lwip_writefds);
  FD_ZERO(lwip_exceptfds);
  for(int i = 0; i < nfds; i++) {
    int lwip_fd;
    if(linux_fd_set_is_set(readfds, i)) {
      assert(socket_ucore_fd_to_lwip_fd(i, &lwip_fd) == 0);
      FD_SET(lwip_fd, lwip_readfds);
    }
    if(linux_fd_set_is_set(writefds, i)) {
      assert(socket_ucore_fd_to_lwip_fd(i, &lwip_fd) == 0);
      FD_SET(lwip_fd, lwip_writefds);
    }
    if(linux_fd_set_is_set(exceptfds, i)) {
      assert(socket_ucore_fd_to_lwip_fd(i, &lwip_fd) == 0);
      FD_SET(lwip_fd, lwip_exceptfds);
    }
  }
  int ret = lwip_select(
    nfds, lwip_readfds, lwip_writefds, lwip_exceptfds, (struct timeval*)timeout
  );
  for(int i = 0; i < nfds; i++) {
    int lwip_fd;
    if(linux_fd_set_is_set(readfds, i)) {
      assert(socket_ucore_fd_to_lwip_fd(i, &lwip_fd) == 0);
      if(!FD_ISSET(lwip_fd, lwip_readfds)) {
        linux_fd_set_unset(readfds, i);
      }
    }
    if(linux_fd_set_is_set(writefds, i)) {
      assert(socket_ucore_fd_to_lwip_fd(i, &lwip_fd) == 0);
      if(!FD_ISSET(lwip_fd, lwip_writefds)) {
        linux_fd_set_unset(writefds, i);
      }
    }
    if(linux_fd_set_is_set(exceptfds, i)) {
      assert(socket_ucore_fd_to_lwip_fd(i, &lwip_fd) == 0);
      if(!FD_ISSET(lwip_fd, lwip_exceptfds)) {
        linux_fd_set_unset(exceptfds, i);
      }
    }
  }
  kfree(lwip_readfds);
  kfree(lwip_writefds);
  kfree(lwip_exceptfds);
  return ret;
}

static void socket_lwip_select_kernel_thread_entry(void* arg) {
  machine_word_t *args = (machine_word_t*)arg;
  int nfds = (int)args[0];
  linux_fd_set_t *_readfds = (linux_fd_set_t*)args[1];
  linux_fd_set_t *_writefds = (linux_fd_set_t*)args[2];
  linux_fd_set_t *_exceptfds = (linux_fd_set_t*)args[3];
  linux_fd_set_t readfds, writefds, exceptfds;
  readfds = *_readfds;
  writefds = *_writefds;
  exceptfds = *_exceptfds;
  struct linux_timeval *timeout = (struct linux_timeval*)args[4];
  struct proc_struct **proc_to_wakeup = (struct proc_struct**)args[5];
  int *result_store = (int*)args[6];
  int ret = socket_lwip_select_wrapper(nfds, &readfds, &writefds, &exceptfds, timeout);
  if((*proc_to_wakeup) != NULL) {
    *result_store = ret;
    *_readfds = readfds;
    *_writefds = writefds;
    *_exceptfds = exceptfds;
    wakeup_proc(*proc_to_wakeup);
  }
  do_exit(0);
}

int socket_lwip_select_wrapper_no_block(
  int nfds, linux_fd_set_t *readfds, linux_fd_set_t *writefds,
  linux_fd_set_t *exceptfds, struct linux_timeval *timeout,
  struct proc_struct **proc_to_wakeup, int *result_store
) {
  machine_word_t *thread_data = kmalloc(sizeof(machine_word_t) * 7);
  thread_data[0] = (machine_word_t)nfds;
  thread_data[1] = (machine_word_t)readfds;
  thread_data[2] = (machine_word_t)writefds;
  thread_data[3] = (machine_word_t)exceptfds;
  thread_data[4] = (machine_word_t)timeout;
  thread_data[5] = (machine_word_t)proc_to_wakeup;
  thread_data[6] = (machine_word_t)result_store;
  return ucore_kernel_thread(
    socket_lwip_select_kernel_thread_entry, thread_data, CLONE_FS);
}
