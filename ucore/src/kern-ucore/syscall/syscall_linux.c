#include <proc.h>
#include <syscall.h>
#include <trap.h>
#include <stdio.h>
#include <pmm.h>
#include <vmm.h>
#include <clock.h>
#include <error.h>
#include <assert.h>
#include <sem.h>
#include <event.h>
#include <mbox.h>
#include <stat.h>
#include <dirent.h>
#include <sysfile.h>
#include <kio.h>
#include <file.h>
#include <linux_misc_struct.h>
#include <fd_set.h>
#include <inode.h>
#include <time/time.h>
#include <network/socket.h>

struct iovec;

machine_word_t syscall_linux_read(machine_word_t args[])
{
	int fd = (int)args[0];
	void *base = (void *)args[1];
	size_t len = (size_t)args[2];
	return sysfile_read(fd, base, len);
}

machine_word_t syscall_linux_write(machine_word_t args[])
{
	int fd = (int)args[0];
	void *base = (void *)args[1];
	size_t len = (size_t)args[2];
	return sysfile_write(fd, base, len);
}

machine_word_t syscall_linux_readv(machine_word_t args[])
{
  int fd = (int)args[0];
	const struct iovec *iov = (const struct iovec*)args[1];
	int iovcnt = (int)args[2];
	return sysfile_readv(fd, iov, iovcnt);
}

machine_word_t syscall_linux_writev(machine_word_t args[])
{
  int fd = (int)args[0];
	const struct iovec *iov = (const struct iovec*)args[1];
	int iovcnt = (int)args[2];
	return sysfile_writev(fd, iov, iovcnt);
}

machine_word_t syscall_linux_open(machine_word_t args[])
{
	const char *path = (const char *)args[0];
	uint32_t open_flags = (uint32_t)args[1];
	return sysfile_open(path, open_flags);
}

machine_word_t syscall_linux_close(machine_word_t args[])
{
	int fd = (int)args[0];
	return sysfile_close(fd);
}

machine_word_t syscall_linux_stat(machine_word_t args[])
{
	char *fn = (char *)args[0];
	struct linux_stat *st = (struct linux_stat *)args[1];
	return sysfile_linux_stat(fn, st);
}

machine_word_t syscall_linux_lstat(machine_word_t args[])
{
	char *fn = (char *)args[0];
	struct linux_stat *st = (struct linux_stat *)args[1];
  //TODO: lstat should be handling symbolic link in a different way than stat.
  //This is a temporary workaround.
	return sysfile_linux_stat(fn, st);
}

machine_word_t syscall_linux_fstat(machine_word_t args[])
{
	int fd = (int)args[0];
	struct linux_stat *st = (struct linux_stat *)args[1];
	return sysfile_linux_fstat(fd, st);
}

machine_word_t syscall_linux_seek(machine_word_t args[])
{
  int fd = (int)args[0];
  off_t pos = (off_t)args[1];
  int whence = (int)args[2];
  return sysfile_seek(fd, pos, whence);
}

machine_word_t syscall_linux_select(machine_word_t args[])
{
  int nfds = (int)args[0];
  linux_fd_set_t *readfds = (linux_fd_set_t*)args[1];
  linux_fd_set_t *writefds = (linux_fd_set_t*)args[2];
  linux_fd_set_t *exceptfds = (linux_fd_set_t*)args[3];
  struct linux_timeval *timeout = (struct linux_timeval*)args[4];
  sysfile_linux_select(nfds, readfds, writefds, exceptfds, timeout);
}

machine_word_t syscall_linux_dup(machine_word_t args[])
{
  int fd = args[0];
  return sysfile_dup1(fd);
}

machine_word_t syscall_linux_dup2(machine_word_t args[])
{
  int fd1 = args[0];
  int fd2 = args[1];
  return sysfile_dup(fd1, fd2);
}

machine_word_t syscall_linux_times(machine_word_t args[])
{
  struct linux_tms *buf = args[0];
  buf->tms_utime = ticks;
  buf->tms_stime = ticks;
  buf->tms_cutime = ticks;
  buf->tms_cstime = ticks;
  return ticks;
}

machine_word_t syscall_linux_mount(machine_word_t args[])
{
	const char *source = (const char *)args[0];
	const char *target = (const char *)args[1];
	const char *filesystemtype = (const char *)args[2];
  unsigned long mountflags = (unsigned long)args[3];
	const void *data = (const void *)args[4];
	return do_mount(source, target, filesystemtype, mountflags, data);
}

machine_word_t syscall_linux_umount(machine_word_t args[])
{
	const char *target = (const char *)args[0];
  int flags = (int)args[1];
	return do_umount(target);
}

machine_word_t syscall_linux_socket(machine_word_t args[])
{
	int domain = (int)args[0];
	int type = (int)args[1];
  int protocol = (int)args[2];
	return socket_create(domain, type, protocol);
}

machine_word_t syscall_linux_connect(machine_word_t args[])
{
  int fd = (int)args[0];
  struct linux_sockaddr *uservaddr = (struct linux_sockaddr*)args[1];
  int addrlen = (int)args[2];
  return socket_connect(fd, uservaddr, addrlen);
}

machine_word_t syscall_linux_bind(machine_word_t args[])
{
  int fd = (int)args[0];
  struct linux_sockaddr *uservaddr = (struct linux_sockaddr*)args[1];
  int addrlen = (int)args[2];
  return socket_bind(fd, uservaddr, addrlen);
}

machine_word_t syscall_linux_listen(machine_word_t args[])
{
  int fd = (int)args[0];
  int backlog = (int)args[1];
  return socket_listen(fd, backlog);
}

machine_word_t syscall_linux_accept(machine_word_t args[])
{
  int fd = (int)args[0];
  struct linux_sockaddr *upeer_sockaddr = (struct linux_sockaddr*)args[1];
  int *upeer_addrlen = (int*)args[2];
  return socket_accept(fd, upeer_sockaddr, upeer_addrlen);
}

machine_word_t syscall_linux_getsockname(machine_word_t args[])
{
  int fd = (int)args[0];
  struct linux_sockaddr *usockaddr = (struct linux_sockaddr*)args[1];
  int *usockaddr_len = (int*)args[2];
  return socket_getsockname(fd, usockaddr, usockaddr_len);
}

machine_word_t syscall_linux_getpeername(machine_word_t args[])
{
  int fd = (int)args[0];
  struct linux_sockaddr *usockaddr = (struct linux_sockaddr*)args[1];
  int *usockaddr_len = (int*)args[2];
  return socket_getpeername(fd, usockaddr, usockaddr_len);
}

machine_word_t syscall_linux_sendto(machine_word_t args[])
{
  int fd = (int)args[0];
  void *buff = (void*)args[1];
  size_t size	= (size_t)args[2];
  unsigned int flags = (unsigned int)args[3];
  struct linux_sockaddr;
  struct linux_sockaddr *addr = (struct linux_sockaddr*)args[4];
  int addr_len = (int)args[5];
  socket_sendto(fd, buff, size, flags, addr, addr_len);
}

machine_word_t syscall_linux_recvfrom(machine_word_t args[])
{
  int fd = (int)args[0];
  void *ubuf = (void*)args[1];
  size_t size	= (size_t)args[2];
  unsigned int flags = (unsigned int)args[3];
  struct linux_sockaddr;
  struct linux_sockaddr *addr = (struct linux_sockaddr*)args[4];
  int *addr_len = (int*)args[5];
  socket_recvfrom(fd, ubuf, size, flags, addr, addr_len);
}

machine_word_t syscall_linux_setsockopt(machine_word_t args[])
{
  int fd = (int)args[0];
  int level = (int)args[1];
  int optname = (int)args[2];
  char *optval = (char*)args[3];
  int optlen = (int)args[4];
  return socket_set_option(fd, level, optname, optval, optlen);
}

machine_word_t syscall_linux_getsockopt(machine_word_t args[])
{
  int fd = (int)args[0];
  int level = (int)args[1];
  int optname = (int)args[2];
  char *optval = (char*)args[3];
  int *optlen = (int*)args[4];
  return socket_get_option(fd, level, optname, optval, optlen);
}

machine_word_t syscall_linux_gettimeofday(machine_word_t args[])
{
	struct linux_timeval *tv = (struct linux_timeval *)args[0];
	struct linux_timezone *tz = (struct linux_timezone *)args[1];
	return ucore_gettimeofday(tv, tz);
}

machine_word_t syscall_linux_setsid(machine_word_t args[])
{
  if(current->pid == current->gid) {
    return current->pid;
  }
  panic("syscall_linux_setsid not implemented!");
}

machine_word_t syscall_linux_getuid(machine_word_t args[])
{
  //TODO: This is a stub function. uCore now has no support for multiple user,
  //so the UID of root is returned.
  const static int UID_ROOT = 0;
  return UID_ROOT;
}

machine_word_t syscall_linux_setuid(machine_word_t args[])
{
  //TODO: This is a stub function. uCore now has no support for multiple user.
  const static int UID_ROOT = 0;
  int uid = args[0];
  if(uid != 0) return -E_INVAL;
  return 0;
}

machine_word_t syscall_linux_getgid(machine_word_t args[])
{
  //TODO: This is a stub function. uCore now has no support for multiple user,
  //so the GID of root is returned.
  const static int GID_ROOT = 0;
  return GID_ROOT;
}

machine_word_t syscall_linux_setgid(machine_word_t args[])
{
  //TODO: This is a stub function. uCore now has no support for multiple user.
  const static int GID_ROOT = 0;
  int gid = args[0];
  if(gid != 0) return -E_INVAL;
  return 0;
}

machine_word_t syscall_linux_geteuid(machine_word_t args[])
{
  //TODO: This is a stub function. uCore now has no support for multiple user,
  //so the UID of root is returned.
  const static int UID_ROOT = 0;
  return UID_ROOT;
}

machine_word_t syscall_linux_getegid(machine_word_t args[])
{
  //TODO: This is a stub function. uCore now has no support for multiple user,
  //so the UID of root is returned.
  const static int GID_ROOT = 0;
  return GID_ROOT;
}

machine_word_t syscall_linux_setgroups(machine_word_t args[])
{
  //TODO: This is a stub function. uCore now has no support for multiple user.
  size_t size = (size_t)args[0];
  const int *list = (const int*)args[1];
  //panic("%d %d\n", size, list[0]);
  return 0;
}

machine_word_t syscall_linux_uname(machine_word_t args[])
{
  char *system_info = args[0];
  const char* os_name = "uCore";
  for(int i = 0; i < 5; i++) {
    strcpy(system_info, os_name);
    system_info += strlen(os_name) + 1;
  }
  return 0;
}

#ifndef __UCORE_64__

machine_word_t syscall_linux_stat64(machine_word_t args[])
{
	char *fn = (char *)args[0];
	struct linux_stat64 *st = (struct linux_stat *)args[1];
	return sysfile_linux_stat64(fn, st);
}

machine_word_t syscall_linux_lstat64(machine_word_t args[])
{
	char *fn = (char *)args[0];
	struct linux_stat64 *st = (struct linux_stat *)args[1];
  //TODO: lstat should be handling symbolic link in a different way than stat.
  //This is a temporary workaround.
	return sysfile_linux_stat64(fn, st);
}

machine_word_t syscall_linux_fstat64(machine_word_t args[])
{
	int fd = (char *)args[0];
	struct linux_stat64 *st = (struct linux_stat *)args[1];
	return sysfile_linux_fstat64(fd, st);
}

#endif /* __UCORE_64__ */
