#ifndef __KERN_NETWORK_SOCKET_H__
#define __KERN_NETWORK_SOCKET_H__

struct linux_sockaddr {
  uint16_t sa_family;
  char sa_data[14];
};

int socket_create(int domain, int type, int protocol);
int socket_sendto(int fd, void __user *buff, size_t len, unsigned int flags, struct linux_sockaddr __user *addr, int addr_len);
int socket_recvfrom(int fd, void __user *ubuf, size_t size, unsigned int flags, struct linux_sockaddr __user *addr, int __user *addr_len);

#endif /* __KERN_NETWORK_SOCKET_H__ */
