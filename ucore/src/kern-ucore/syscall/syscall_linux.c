#include <kio.h>
#include <stdio.h>
#include <string.h>
#include <proc.h>
#include <syscall.h>
#include <trap.h>
#include <pmm.h>
#include <vmm.h>
#include <clock.h>
#include <error.h>
#include <assert.h>
#include <sem.h>
#include <event.h>
#include <stat.h>
#include <dirent.h>
#include <sysfile.h>
#include <file.h>
#include <vfs.h>
#include <linux_misc_struct.h>
#include <fd_set.h>
#include <inode.h>
#include <time/time.h>
#include <network/socket.h>
#include <ptrace/ptrace.h>
#include <syscall_linux.h>

struct iovec;

machine_word_t syscall_linux_exit(machine_word_t arg[])
{
	int error_code = (int)arg[0];
	return do_exit(error_code);
}

machine_word_t syscall_linux_wait4(machine_word_t args[])
{
	int pid = (int)args[0];
	int *store = (int*)args[1];
	int options = (int)args[2];
	void *rusage = (void *)args[3];
	if (options && rusage)
		return -E_INVAL;
	return do_linux_waitpid(pid, store);
}

machine_word_t syscall_linux_getpid(machine_word_t args[])
{
	return current->pid;
}

machine_word_t syscall_linux_getppid(machine_word_t args[])
{
	struct proc_struct *parent = current->parent;
	if (!parent)
		return 0;
	return parent->pid;
}

machine_word_t syscall_linux_getpgrp(machine_word_t args[])
{
  return current->gid;
}

#if defined(ARCH_AMD64) || defined(ARCH_MIPS)
machine_word_t syscall_linux_ptrace(machine_word_t args[])
{
  long request = (long)args[0];
  long pid = (long)args[1];
  unsigned long addr = (unsigned long)args[2];
  unsigned long data = (unsigned long)args[3];
  return ptrace_syscall(request, pid, addr, data);
}
#endif

machine_word_t syscall_linux_sigaction(machine_word_t arg[])
{
	return do_sigaction((int)arg[0], (const struct sigaction *)arg[1],
			    (struct sigaction *)arg[2]);
}

#ifndef ARCH_X86
machine_word_t syscall_linux_sigreturn(machine_word_t arg[])
{
  return do_sigreturn();
}
#endif

machine_word_t syscall_linux_sigprocmask(machine_word_t arg[])
{
	return do_sigprocmask((int)arg[0], (const sigset_t *)arg[1],
			      (sigset_t *) arg[2]);
}


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

machine_word_t syscall_linux_access(machine_word_t args[])
{
	return 0;
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
	return sysfile_linux_lstat(fn, st);
}

machine_word_t syscall_linux_fstat(machine_word_t args[])
{
	int fd = (int)args[0];
	struct linux_stat *st = (struct linux_stat *)args[1];
	return sysfile_linux_fstat(fd, st);
}

machine_word_t syscall_linux_readlink(machine_word_t args[])
{
	const char *pathname = (const char *)args[0];
	char *buf = (char *)args[1];
	size_t bufsiz = (size_t)args[2];
	return sysfile_readlink(pathname, buf, bufsiz);
}

machine_word_t syscall_linux_seek(machine_word_t args[])
{
  int fd = (int)args[0];
  off_t pos = (off_t)args[1];
  int whence = (int)args[2];
  return sysfile_seek(fd, pos, whence);
}

machine_word_t syscall_linux_mkdir(machine_word_t args[])
{
	const char *path = (const char*)args[0];
	return sysfile_mkdir(path);
}

machine_word_t syscall_linux_chdir(machine_word_t args[])
{
  const char *path = (const char*)args[0];
  return sysfile_chdir(path);
}

machine_word_t syscall_linux_getcwd(machine_word_t args[])
{
	char *buf = (char *)args[0];
	size_t len = (size_t)args[1];
	int ret = sysfile_getcwd(buf, len);
  if(ret < 0) return ret;
  return strlen(buf) + 1;
}

machine_word_t syscall_linux_getdents(machine_word_t args[])
{
	unsigned int fd = (unsigned int)args[0];
	struct dirent *dir = (struct dirent *)args[1];
	unsigned int count = (unsigned int)args[2];
	if (count < sizeof(struct dirent))
		return -E_INVAL;
	int ret = sysfile_getdirentry(fd, dir, (uint32_t *)&count);
	if (ret < 0) return ret;
	return count;
}

machine_word_t syscall_linux_getdents64(machine_word_t args[])
{
  unsigned int fd = (unsigned int)args[0];
	struct dirent64 *dir = (struct dirent64 *)args[1];
	unsigned int count = (unsigned int)args[2];
	if (count < sizeof(struct dirent64))
		return -E_INVAL;
//	kprintf("call getdirendry %d %p %p\n", fd, dir, &count);
	int ret = sysfile_getdirentry64(fd, dir, (uint32_t *)&count);
  if (ret < 0) return ret;
	return count;
}

machine_word_t syscall_linux_ioctl(machine_word_t args[])
{
  int fd = (int)args[0];
	unsigned int cmd = (unsigned int)args[1];
	unsigned long data = (unsigned long)args[2];
  if (fd < 3) {
    return 0;
  }
	return sysfile_ioctl(fd, cmd, data);
}

machine_word_t syscall_linux_fcntl(machine_word_t args[])
{
  unsigned int fd = (unsigned int)args[0];
  unsigned int cmd = (unsigned int)args[1];
  unsigned long arg = (unsigned long)args[2];
	return sysfile_linux_fcntl64(fd, cmd, arg);
}

machine_word_t syscall_linux_pipe(machine_word_t args[])
{
	int *fd_store = (int *)args[0];
	return sysfile_pipe(fd_store) ? -1 : 0;
}

machine_word_t syscall_linux_brk(machine_word_t args[])
{
  uintptr_t brk_store = (uintptr_t)args[0];
  return do_linux_brk(brk_store);
}

/*machine_word_t syscall_linux_old_mmap(machine_word_t args[])
{
  struct mmap_arg_struct {
       unsigned long addr;
       unsigned long len;
       unsigned long prot;
       unsigned long flags;
       unsigned long fd;
       unsigned long offset;
  };
  struct mmap_arg_struct *user_args = args[0];
  args[0] = user_args->addr;
  args[1] = user_args->len;
  args[2] = user_args->prot;
  args[3] = user_args->flags;
  args[4] = user_args->fd;
  args[5] = user_args->offset;
  return syscall_linux_mmap(args);
}*/

machine_word_t syscall_linux_mmap(machine_word_t args[])
{
	void *addr = (void *)args[0];
	size_t len = (size_t)args[1];
	unsigned long prot = (unsigned long)args[2];
	unsigned long flags = (unsigned long)args[3];
	int fd = (int)args[4];
	unsigned long off = (unsigned long)args[5];
	//kprintf
	//    ("TODO __sys_linux_mmap2 addr=%08x len=%08x prot=%08x flags=%08x fd=%d off=%08x\n",
	//     addr, len, prot, flags, fd, off);
	if (fd == -1 || flags & MAP_ANONYMOUS) {
		if (flags & MAP_FIXED) {
			return linux_regfile_mmap2(addr, len, prot, flags, fd,
						   off);
		}

		uint32_t ucoreflags = 0;
		if (prot & PROT_WRITE)
			ucoreflags |= MMAP_WRITE;
		int ret = __do_linux_mmap((uintptr_t) & addr, len, ucoreflags);
		//kprintf("@@@ ret=%d %e %08x\n", ret,ret, addr);
		if (ret)
			return (machine_word_t)MAP_FAILED;
		//kprintf("__sys_linux_mmap2 ret=%08x\n", addr);
		return (machine_word_t) addr;
	}
  else {
		return (machine_word_t)sysfile_linux_mmap2(addr, len, prot, flags, fd, off);
	}
}

machine_word_t syscall_linux_munmap(machine_word_t arg[])
{
	uintptr_t addr = (uintptr_t)arg[0];
	size_t len = (size_t)arg[1];
	return do_munmap(addr, len);
}

machine_word_t syscall_linux_mprotect(machine_word_t args[])
{
	void *addr = (void*)args[0];
	size_t len = (size_t)args[1];
	int prot = (int)args[2];
	return do_mprotect(addr, len, prot);
}

machine_word_t syscall_linux_poll(machine_word_t args[])
{
  struct linux_pollfd {
  	int fd;			/* file descriptor */
  	short events;		/* requested events */
  	short revents;		/* returned events */
  };
  //TODO: This is a stub, try to reimplement.
	struct linux_pollfd *fd = (struct linux_pollfd *)args[0]; // can be hacked
	int nfds = (int)args[1];
	int timeout = (int)args[2];	//ms
	fd->revents = fd->events;
	return nfds;
}

machine_word_t syscall_linux_select(machine_word_t args[])
{
  int nfds = (int)args[0];
  linux_fd_set_t *readfds = (linux_fd_set_t*)args[1];
  linux_fd_set_t *writefds = (linux_fd_set_t*)args[2];
  linux_fd_set_t *exceptfds = (linux_fd_set_t*)args[3];
  struct linux_timeval *timeout = (struct linux_timeval*)args[4];
  return sysfile_linux_select(nfds, readfds, writefds, exceptfds, timeout);
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

machine_word_t syscall_linux_nanosleep(machine_word_t args[])
{
	//TODO: handle signal interrupt
	struct linux_timespec *req = (struct linux_timespec *)args[0];
	struct linux_timespec *rem = (struct linux_timespec *)args[1];
	return do_linux_sleep(req, rem);
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

machine_word_t syscall_linux_send(machine_word_t args[])
{
  int fd = (int)args[0];
  void *buff = (void*)args[1];
  size_t size	= (size_t)args[2];
  unsigned int flags = (unsigned int)args[3];
  return socket_sendto(fd, buff, size, flags, NULL, 0);
}

machine_word_t syscall_linux_sendto(machine_word_t args[])
{
  int fd = (int)args[0];
  void *buff = (void*)args[1];
  size_t size	= (size_t)args[2];
  unsigned int flags = (unsigned int)args[3];
  struct linux_sockaddr *addr = (struct linux_sockaddr*)args[4];
  int addr_len = (int)args[5];
  return socket_sendto(fd, buff, size, flags, addr, addr_len);
}

machine_word_t syscall_linux_recv(machine_word_t args[])
{
  int fd = (int)args[0];
  void *ubuf = (void*)args[1];
  size_t size	= (size_t)args[2];
  unsigned int flags = (unsigned int)args[3];
  return socket_recvfrom(fd, ubuf, size, flags, NULL, NULL);
}

machine_word_t syscall_linux_recvfrom(machine_word_t args[])
{
  int fd = (int)args[0];
  void *ubuf = (void*)args[1];
  size_t size	= (size_t)args[2];
  unsigned int flags = (unsigned int)args[3];
  struct linux_sockaddr *addr = (struct linux_sockaddr*)args[4];
  int *addr_len = (int*)args[5];
  return socket_recvfrom(fd, ubuf, size, flags, addr, addr_len);
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

machine_word_t syscall_linux_time(machine_word_t args[])
{
  time_t* time = (time_t*)args[0];
  if(time != NULL) (*time) = time_get_current();
  return time_get_current();
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
  if(uid != UID_ROOT) return -E_INVAL;
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
  if(gid != GID_ROOT) return -E_INVAL;
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

machine_word_t syscall_linux_getrlimit(machine_word_t args[])
{
	int res = (int)args[0];
	struct linux_rlimit *lim = (struct linux_rlimit*)args[1];
	return do_linux_ugetrlimit(res, lim);
}

machine_word_t syscall_linux_setrlimit(machine_word_t args[])
{
	int res = (int)args[0];
	struct linux_rlimit *lim = (struct linux_rlimit*)args[1];
	return do_linux_usetrlimit(res, lim);
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
	return sysfile_linux_lstat64(fn, st);
}

machine_word_t syscall_linux_fstat64(machine_word_t args[])
{
	int fd = (char *)args[0];
	struct linux_stat64 *st = (struct linux_stat *)args[1];
	return sysfile_linux_fstat64(fd, st);
}

#endif /* __UCORE_64__ */
