#include <stdlib.h>
#include <string.h>
#include "lwip/sockets.h"
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
  int ret = lwip_socket(domain, type, protocol);
  kprintf("Assign socket %d\n", ret);
  return ret;
}

int socket_connect(int fd, struct linux_sockaddr __user *uservaddr, int addrlen)
{
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  linux_sockaddr_to_lwip_sockaddr(uservaddr, lwip_sockaddr);
  int ret = lwip_connect(fd, lwip_sockaddr, sizeof(struct sockaddr));
  kfree(lwip_sockaddr);
  return ret;
}

int socket_sendto(int fd, void __user *buff, size_t len, unsigned int flags, struct linux_sockaddr __user *addr, int addr_len)
{
  struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
  lwip_sockaddr->sa_len = sizeof(struct sockaddr);
  lwip_sockaddr->sa_family = (uint8_t)addr->sa_family; //This may lead to truncating, but currently there is no no sa_family >= 256
  //Socket data handling is in a kernel thread, in whose context, userspace "addr" is not valid.
  char* kernel_buff = kmalloc(len);
  memcpy(lwip_sockaddr->sa_data, addr->sa_data, 14);
  memcpy(kernel_buff, buff, len);
  int ret = lwip_sendto(fd, kernel_buff, len, flags, lwip_sockaddr, sizeof(struct sockaddr));
  kfree(lwip_sockaddr);
  //TODO: Not sure if lwip_sendto will free the buffer passed to it.
  kfree(kernel_buff);
  return ret;
}

int socket_recvfrom(int fd, void __user *ubuf, size_t size, unsigned int flags, struct linux_sockaddr __user *addr, int __user *addr_len)
{
  int ret;
  char* kernel_buff = kmalloc(size);
  if(addr == NULL) {
    ret = lwip_recv(fd, kernel_buff, size, flags);
  }
  else {
    int kernel_addr_len = *addr_len;
    struct sockaddr *lwip_sockaddr = kmalloc(sizeof(struct sockaddr));
    linux_sockaddr_to_lwip_sockaddr(addr, lwip_sockaddr);
    ret = lwip_recvfrom(fd, kernel_buff, size, flags, lwip_sockaddr, &kernel_addr_len);
    *addr_len = kernel_addr_len;
    kfree(lwip_sockaddr);
  }
  memcpy(ubuf, kernel_buff, size);
  kfree(kernel_buff);
  return ret;
}
