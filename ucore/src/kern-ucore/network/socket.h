#ifndef __KERN_NETWORK_SOCKET_H__
#define __KERN_NETWORK_SOCKET_H__

struct linux_timeval;

struct linux_sockaddr {
  uint16_t sa_family;
  char sa_data[14];
};

int socket_create(int domain, int type, int protocol);
int socket_connect(int fd, struct linux_sockaddr __user *uservaddr, int addrlen);
int socket_bind(int fd, struct linux_sockaddr __user *uservaddr, int addrlen);
int socket_listen(int fd, int backlog);
int socket_accept(int fd, struct linux_sockaddr __user *upeer_sockaddr, int __user *upeer_addrlen);
int socket_getsockname(int fd, struct linux_sockaddr __user *usockaddr, int __user *usockaddr_len);
int socket_getpeername(int fd, struct linux_sockaddr __user *usockaddr, int __user *usockaddr_len);
int socket_sendto(int fd, void __user *buff, size_t len, unsigned int flags, struct linux_sockaddr __user *addr, int addr_len);
int socket_recvfrom(int fd, void __user *ubuf, size_t size, unsigned int flags, struct linux_sockaddr __user *addr, int __user *addr_len);
int socket_set_option(int fd, int level, int optname, char __user *optval, int optlen);
int socket_get_option(int fd, int level, int optname, char __user *optval, int *optlen);
int socket_lwip_select_wrapper(int nfds, linux_fd_set_t *readfds, linux_fd_set_t *writefds,
  linux_fd_set_t *exceptfds, struct linux_timeval *timeout);
int socket_lwip_select_wrapper_no_block(
  int nfds, linux_fd_set_t *readfds, linux_fd_set_t *writefds,
  linux_fd_set_t *exceptfds, struct linux_timeval *timeout,
  struct proc_struct **proc_to_wakeup, int *result_store
);

#endif /* __KERN_NETWORK_SOCKET_H__ */
