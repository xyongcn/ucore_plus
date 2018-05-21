#include <stdlib.h>
#include <string.h>
#include <fd_set.h>
#include <file.h>
#include <error.h>
#include <stat.h>
#include <kernel_file_pool.h>
#include <file_desc_table.h>
#include "lwip/sockets.h"
#include "socket_inode.h"
#include "socket.h"
#include <linux_misc_struct.h>

//TODO : Check user mem!
//TODO : Check addr length!

static void linux_sockaddr_to_lwip_sockaddr(struct linux_sockaddr *linux_sockaddr, struct sockaddr *lwip_sockaddr)
{
  lwip_sockaddr->sa_len = sizeof(struct sockaddr);
  lwip_sockaddr->sa_family = (uint8_t)linux_sockaddr->sa_family;
  memcpy(lwip_sockaddr->sa_data, linux_sockaddr->sa_data, 14);
}

static void lwip_sockaddr_to_linux_sockaddr(struct sockaddr *lwip_sockaddr, struct linux_sockaddr *linux_sockaddr)
{
  linux_sockaddr->sa_family = lwip_sockaddr->sa_family;
  memcpy(linux_sockaddr->sa_data, lwip_sockaddr->sa_data, 14);
}

static int wrap_lwip_socket(int lwip_fd)
{
  struct file_desc_table *desc_table = fs_get_desc_table(current->fs_struct);
  int ret;
  struct file *file = kernel_file_pool_allocate();
  if(file == NULL) {
    return -E_NFILE;
  }
  ret = file_desc_table_get_unused(desc_table);
  if(ret == -1) {
    kernel_file_pool_free(file);
    return -E_MFILE;
  }
  file_init(file);
  struct inode *node = NULL;
  node = alloc_inode(default_inode);
  assert(node != NULL);
  vop_init(node, &socket_inode_ops, NULL);
  file->pos = 0;
  file->node = node;
  file->readable = 1;
  file->writable = 1;
  file_desc_table_associate(desc_table, ret, file);
  struct socket_inode_private_data *private_data = kmalloc(sizeof(struct socket_inode_private_data));
  node->private_data = private_data;
  private_data->lwip_socket = lwip_fd;
  vop_open_inc(node);
  vop_ref_inc(node);
  return ret;
}

static int socket_ucore_fd_to_lwip_fd(int ucore_fd, int *lwip_fd)
{
  int ret;
  struct file *file;
  if ((ret = fd2file(ucore_fd, &file)) != 0) {
    kprintf("BADF\n");
    return -E_BADF;
  }
  int fd_type;
  if(vop_gettype(file->node, &fd_type) != 0 || !S_ISSOCK(fd_type)) {
    kprintf("NTSK %d\n", vop_gettype(file->node, &fd_type));
    return -E_NOTSOCK;
  }
  *lwip_fd = ((struct socket_inode_private_data *)(file->node->private_data))->lwip_socket;
  return 0;
}

int socket_create(int domain, int type, int protocol)
{
#ifdef ARCH_MIPS
  //MIPS is the only architecture that has an reversed definition of SOCK_DGRAM
  //and SOCK_STREAM, so this work-around is used. See
  //http://lxr.free-electrons.com/source/arch/mips/include/asm/socket.h and
  //http://lxr.free-electrons.com/source/include/linux/net.h for
  //for further information.
  if(type == SOCK_DGRAM) {
    type = SOCK_STREAM;
  }
  else if(type == SOCK_STREAM) {
    type = SOCK_DGRAM;
  }
#endif
  int ret = lwip_socket(domain, type, protocol);
  if(ret < 0) return ret;
  return wrap_lwip_socket(ret);
}

int socket_connect(int fd, struct linux_sockaddr __user *uservaddr, int addrlen)
{
  int ret, lwip_fd;
  if(uservaddr->sa_family == 1) return -E_AFNOSUPPORT; //AF_LOCAL is not supported
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  linux_sockaddr_to_lwip_sockaddr(uservaddr, lwip_sockaddr);
  ret = lwip_connect(lwip_fd, lwip_sockaddr, sizeof(struct sockaddr));
  kfree(lwip_sockaddr);
  return ret;
}

int socket_bind(int fd, struct linux_sockaddr __user *uservaddr, int addrlen)
{
  int ret, lwip_fd;
  if(uservaddr->sa_family == 1) return -E_AFNOSUPPORT; //AF_LOCAL is not supported
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  linux_sockaddr_to_lwip_sockaddr(uservaddr, lwip_sockaddr);
  ret = lwip_bind(lwip_fd, lwip_sockaddr, sizeof(struct sockaddr));
  kfree(lwip_sockaddr);
  return ret;
}

int socket_listen(int fd, int backlog)
{
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  ret = lwip_listen(lwip_fd, backlog);
  return ret;
}

int socket_accept(int fd, struct linux_sockaddr __user *upeer_sockaddr, int __user *upeer_addrlen)
{
  kprintf("Entering socket_accept %x %x\n", upeer_sockaddr, upeer_addrlen);
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  int kernel_addr_len;
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  if(upeer_sockaddr != NULL) {
    kernel_addr_len = *upeer_addrlen;
  }
  ret = lwip_accept(lwip_fd, lwip_sockaddr, &kernel_addr_len);
  if(upeer_sockaddr != NULL) {
    lwip_sockaddr_to_linux_sockaddr(lwip_sockaddr, upeer_sockaddr);
    *upeer_addrlen = kernel_addr_len;
  }
  kfree(lwip_sockaddr);
  if(ret < 0) return ret;
  return wrap_lwip_socket(ret);
}

int socket_getsockname(int fd, struct linux_sockaddr __user *usockaddr, int __user *usockaddr_len)
{
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  int kernel_addr_len = *usockaddr_len;
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  ret = lwip_getsockname(lwip_fd, lwip_sockaddr, &kernel_addr_len);
  lwip_sockaddr_to_linux_sockaddr(lwip_sockaddr, usockaddr);
  *usockaddr_len = kernel_addr_len;
  kfree(lwip_sockaddr);
  return ret;
}

int socket_getpeername(int fd, struct linux_sockaddr __user *usockaddr, int __user *usockaddr_len)
{
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  int kernel_addr_len = *usockaddr_len;
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  ret = lwip_getpeername(lwip_fd, lwip_sockaddr, &kernel_addr_len);
  lwip_sockaddr_to_linux_sockaddr(lwip_sockaddr, usockaddr);
  *usockaddr_len = kernel_addr_len;
  kfree(lwip_sockaddr);
  return ret;
}

int socket_sendto(int fd, void __user *buff, size_t len, unsigned int flags, struct linux_sockaddr __user *addr, int addr_len)
{
  kprintf("Kernel Level socket_sendto\n");
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) return ret;
  char* kernel_buff = kmalloc(len);
  memcpy(kernel_buff, buff, len);
  if(addr == NULL) {
    ret = lwip_send(lwip_fd, kernel_buff, len, flags);
    kprintf("**Kernel Level socket_sendto ret %d\n", ret);
  }
  else {
    struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
    linux_sockaddr_to_lwip_sockaddr(addr, lwip_sockaddr);
    ret = lwip_sendto(lwip_fd, kernel_buff, len, flags, lwip_sockaddr, sizeof(struct sockaddr));
    kfree(lwip_sockaddr);
    kprintf("##Kernel Level socket_sendto ret %d\n", ret);
  }
  //TODO: Not sure if lwip_sendto will free the buffer passed to it.
  kfree(kernel_buff);
  kprintf("Kernel Level socket_sendto ret %d\n", ret);
  return ret;
}

int socket_recvfrom(int fd, void __user *ubuf, size_t size, unsigned int flags, struct linux_sockaddr __user *addr, int __user *addr_len)
{
  int ret, lwip_fd;
  ret = socket_ucore_fd_to_lwip_fd(fd, &lwip_fd);
  if(ret != 0) assert(false);
  if(ret != 0) return ret;
  char* kernel_buff = kmalloc(size);
  if(addr == NULL) {
    kprintf("Kernel Level socket_recvfrom\n");
    ret = lwip_recv(lwip_fd, kernel_buff, size, flags);
    kprintf("Leaving Level socket_recvfrom\n");
  }
  else {
    //FIXME: Handle cases where linux_sockaddr is needed to copy to user!
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
  kprintf("Enetring socket_lwip_select_wrapper\n");
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
  if(lwip_readfds) kprintf("readfds: %x\n", *(int*)lwip_readfds);
  if(lwip_writefds) kprintf("readfds: %x\n", *(int*)lwip_writefds);
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
  kprintf("Leaving socket_lwip_select_wrapper\n");
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
  int ret = 0;
  if((*proc_to_wakeup) != NULL) {
    ret = socket_lwip_select_wrapper(nfds, &readfds, &writefds, &exceptfds, timeout);
  }
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
  kprintf("### %x\n", *(int*)exceptfds);
  machine_word_t *thread_data = kmalloc(sizeof(machine_word_t) * 7);
  thread_data[0] = (machine_word_t)nfds;
  thread_data[1] = (machine_word_t)readfds;
  thread_data[2] = (machine_word_t)writefds;
  thread_data[3] = (machine_word_t)exceptfds;
  thread_data[4] = (machine_word_t)timeout;
  thread_data[5] = (machine_word_t)proc_to_wakeup;
  thread_data[6] = (machine_word_t)result_store;
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
  *result_store = lwip_selscan(nfds, lwip_readfds, lwip_writefds, lwip_exceptfds, lwip_readfds, lwip_writefds, lwip_exceptfds);
  if((*result_store) > 0) {
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
    return 0;
  }
  else {
    kfree(lwip_readfds);
    kfree(lwip_writefds);
    kfree(lwip_exceptfds);
    /*kprintf("###2 %x\n", *(int*)exceptfds);
    linux_fd_set_t *_readfds = kmalloc(sizeof(linux_fd_set_t));
    linux_fd_set_t *_writefds = kmalloc(sizeof(linux_fd_set_t));
    linux_fd_set_t *_exceptfds = kmalloc(sizeof(linux_fd_set_t));
    struct linux_timeval *_timeout = kmalloc(sizeof(struct linux_timeval));
    *_readfds = *readfds;
    *_writefds = *writefds;
    *_exceptfds = *exceptfds;
    *_timeout = *timeout;
    thread_data[1] = (machine_word_t)_readfds;
    thread_data[2] = (machine_word_t)_writefds;
    thread_data[3] = (machine_word_t)_exceptfds;
    thread_data[4] = (machine_word_t)_timeout;*/
    return ucore_kernel_thread(
      socket_lwip_select_kernel_thread_entry, thread_data, CLONE_FS);
  }
}
