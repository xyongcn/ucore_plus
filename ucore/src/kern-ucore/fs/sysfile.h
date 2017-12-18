#ifndef __KERN_FS_SYSFILE_H__
#define __KERN_FS_SYSFILE_H__

#include <types.h>
#include <fd_set.h>
#include <linux_misc_struct.h>

struct stat;
struct linux_stat;
struct linux_stat64;
struct dirent;
struct dirent64;
struct iovec;

int sysfile_open(const char *path, uint32_t open_flags);
int sysfile_close(int fd);
int sysfile_read(int fd, void *base, size_t len);
int sysfile_write(int fd, void *base, size_t len);
int sysfile_readv(int fd, const struct iovec __user *iov, int iovcnt);
int sysfile_writev(int fd, const struct iovec __user *iov, int iovcnt);
int sysfile_seek(int fd, off_t pos, int whence);
int sysfile_fstat(int fd, struct stat *stat);
int sysfile_stat(const char *fn, struct stat *stat);
int sysfile_linux_fstat(int fd, struct linux_stat __user * linux_stat_store);
int sysfile_linux_stat(const char __user * fn, struct linux_stat *__user buf);
int sysfile_linux_lstat(const char __user * fn, struct linux_stat *__user buf);
size_t sysfile_readlink(const char *pathname, char *base, size_t len);
int sysfile_fsync(int fd);
int sysfile_chdir(const char *path);
int sysfile_mkdir(const char *path);
int sysfile_link(const char *path1, const char *path2);
int sysfile_rename(const char *path1, const char *path2);
int sysfile_unlink(const char *path);
int sysfile_getcwd(char *buf, size_t len);
int sysfile_getdirentry(int fd, struct dirent *direntp, uint32_t * len);
int sysfile_getdirentry64(int fd, struct dirent64 *__direntp, uint32_t * len_store);
int sysfile_dup1(int fd);
int sysfile_dup(int fd1, int fd2);
int sysfile_pipe(int *fd_store);
int sysfile_mkfifo(const char *name, uint32_t open_flags);
int sysfile_linux_fcntl64(int fd, int cmd, int arg);

int sysfile_ioctl(int fd, unsigned int cmd, unsigned long arg);
void *sysfile_linux_mmap2(void *addr, size_t len, int prot, int flags, int fd,
			  size_t pgoff);

int sysfile_linux_select(int nfds, linux_fd_set_t *readfds, linux_fd_set_t *writefds,
  linux_fd_set_t *exceptfds, struct linux_timeval *timeout);

#ifndef __UCORE_64__
int sysfile_linux_fstat64(int fd, struct linux_stat64 __user * linux_stat_store);
int sysfile_linux_stat64(const char __user * fn, struct linux_stat64 *__user buf);
int sysfile_linux_lstat64(const char __user * fn, struct linux_stat64 *__user buf);
#endif

#endif /* !__KERN_FS_SYSFILE_H__ */
