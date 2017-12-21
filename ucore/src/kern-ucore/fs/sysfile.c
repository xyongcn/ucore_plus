#include <types.h>
#include <string.h>
#include <slab.h>
#include <vmm.h>
#include <proc.h>
#include <vfs.h>
#include <file.h>
#include <iobuf.h>
#include <linux_compat_stat.h>
#include <stat.h>
#include <dirent.h>
#include <unistd.h>
#include <error.h>
#include <assert.h>
#include <inode.h>
#include <fd_set.h>
#include <poll.h>
#include <socket.h>

#include "sysfile.h"

#define IOBUF_SIZE                          4096

static void ucore_stat_to_linux_stat(const struct stat *ucore_stat, struct linux_stat *linux_stat)
{
	linux_stat->st_ino = ucore_stat->st_ino; //TODO: Some fs have no support for this.
	/* ucore never check access permision */
	linux_stat->st_mode = ucore_stat->st_mode | 0777;
	linux_stat->st_nlink = ucore_stat->st_nlinks;
	linux_stat->st_blksize = 512;
	linux_stat->st_blocks = ucore_stat->st_blocks;
	linux_stat->st_size = ucore_stat->st_size;
	linux_stat->st_uid = 0;
	linux_stat->st_gid = 0;
}

#ifndef __UCORE_64__
static void ucore_stat_to_linux_stat64(const struct stat *ucore_stat, struct linux_stat64 *linux_stat64)
{
	linux_stat64->st_ino = ucore_stat->st_ino; //TODO: Some fs have no support for this.
	/* ucore never check access permision */
	linux_stat64->st_mode = ucore_stat->st_mode | 0777;
	linux_stat64->st_nlink = ucore_stat->st_nlinks;
	linux_stat64->st_blksize = 512;
	linux_stat64->st_blocks = ucore_stat->st_blocks;
	linux_stat64->st_size = ucore_stat->st_size;
	linux_stat64->st_uid = 0;
	linux_stat64->st_gid = 0;
}
#endif

static int copy_path(char **to, const char *from)
{
	struct mm_struct *mm = current->mm;
	char *buffer;
	if ((buffer = kmalloc(FS_MAX_FPATH_LEN + 1)) == NULL) {
		return -E_NO_MEM;
	}
	lock_mm(mm);
	if (!copy_string(mm, buffer, from, FS_MAX_FPATH_LEN + 1)) {
		unlock_mm(mm);
		goto failed_cleanup;
	}
	unlock_mm(mm);
	*to = buffer;
	return 0;

failed_cleanup:
	kfree(buffer);
	return -E_INVAL;
}

int sysfile_open(const char *__path, uint32_t open_flags)
{
	int ret;
	char *path;
	if ((ret = copy_path(&path, __path)) != 0) {
		return ret;
	}
	ret = file_open(path, open_flags);
	kfree(path);
	return ret;
}

int sysfile_close(int fd)
{
	return file_close(fd);
}

/* *
 * sysfile_read - system file read function
 * this function can read from normal file and from device such as console
 * if fd represent a linux dev, it will call function linux_devfile_read
 * if fd represent a normal file, it will call function file_read
 * @fd		file handle
 * @base	buffer address
 * @len		length to be read
 * */
int sysfile_read(int fd, void *base, size_t len)
{
	int ret = 0;
	struct mm_struct *mm = current->mm;
	if (len == 0) {
		return 0;
	}
	if (!file_testfd(fd, 1, 0)) {
		return -E_INVAL;
	}
	/* for linux inode */
	if (__is_linux_devfile(fd)) {
		size_t alen = 0;
		ret = linux_devfile_read(fd, base, len, &alen);
		if (ret)
			return ret;
		return alen;
	}
	void *buffer;
	if ((buffer = kmalloc(IOBUF_SIZE)) == NULL) {
		return -E_NO_MEM;
	}

	size_t copied = 0, alen;
	while (len != 0) {
		if ((alen = IOBUF_SIZE) > len) {
			alen = len;
		}
		ret = file_read(fd, buffer, alen, &alen);
		if (alen != 0) {
			lock_mm(mm);
			{
				if (copy_to_user(mm, base, buffer, alen)) {
					assert(len >= alen);
					base += alen, len -= alen, copied +=
					    alen;
				} else if (ret == 0) {
					ret = -E_INVAL;
				}
			}
			unlock_mm(mm);
		}
		if (ret != 0 || alen == 0) {
			goto out;
		}
	}

out:
	kfree(buffer);
	if (copied != 0) {
		return copied;
	}
	return ret;
}

int sysfile_write(int fd, void *base, size_t len)
{
	int ret = 0;
	struct mm_struct *mm = current->mm;
	if (len == 0) {
		return 0;
	}
	if (!file_testfd(fd, 0, 1)) {
		return -E_INVAL;
	}
	/* for linux inode */
	if (__is_linux_devfile(fd)) {
		/* use 8byte int, in case of 64bit off_t
		 * config in linux kernel */
		size_t alen = 0;
		ret = linux_devfile_write(fd, base, len, &alen);
		if (ret)
			return ret;
		return alen;
	}
	void *buffer;
	if ((buffer = kmalloc(IOBUF_SIZE)) == NULL) {
		return -E_NO_MEM;
	}

	size_t copied = 0, alen;
	while (len != 0) {
		if ((alen = IOBUF_SIZE) > len) {
			alen = len;
		}
		lock_mm(mm);
		{
			if (!copy_from_user(mm, buffer, base, alen, 0)) {
				ret = -E_INVAL;
			}
		}
		unlock_mm(mm);
		if (ret == 0) {
			ret = file_write(fd, buffer, alen, &alen);
			if (alen != 0) {
				assert(len >= alen);
				base += alen, len -= alen, copied += alen;
			}
		}
		if (ret != 0 || alen == 0) {
			goto out;
		}
	}

out:
	kfree(buffer);
	if (copied != 0) {
		return copied;
	}
	return ret;
}

int sysfile_readv(int fd, const struct iovec __user *iov, int iovcnt)
{
  int bytes_read = 0;
	for (int i = 0; i < iovcnt; ++i) {
		int ret = sysfile_read(fd, iov[i].iov_base, iov[i].iov_len);
		if (ret < 0) return ret;
		bytes_read += ret;
    if(ret < iov[i].iov_len) break;
	}
  return bytes_read;
}

int sysfile_writev(int fd, const struct iovec __user * iov, int iovcnt)
{
  int bytes_written = 0;
	for (int i = 0; i < iovcnt; ++i) {
		int ret = sysfile_write(fd, iov[i].iov_base, iov[i].iov_len);
		if (ret < 0) return ret;
		bytes_written += ret;
    if(ret < iov[i].iov_len) break;
	}
  return bytes_written;
}

int sysfile_seek(int fd, off_t pos, int whence)
{
	return file_seek(fd, pos, whence);
}

int sysfile_fstat(int fd, struct stat *__stat)
{
	struct mm_struct *mm = current->mm;
	int ret;
	struct stat __local_stat, *stat = &__local_stat;
	if ((ret = file_fstat(fd, stat)) != 0) {
		return ret;
	}

	lock_mm(mm);
	{
		if (!copy_to_user(mm, __stat, stat, sizeof(struct stat))) {
			ret = -E_INVAL;
		}
	}
	unlock_mm(mm);
	return ret;
}

int sysfile_linux_fstat(int fd, struct linux_stat __user * buf)
{
	struct mm_struct *mm = current->mm;
	int ret;
	struct stat __local_stat, *kstat = &__local_stat;
	if ((ret = file_fstat(fd, kstat)) != 0) {
		return -1;
	}
	struct linux_stat *kls = kmalloc(sizeof(struct linux_stat));
	if (!kls) {
		return -1;
	}
	memset(kls, 0, sizeof(struct linux_stat));
	kls->st_ino = kstat->st_ino; //TODO: Some fs have no support for this.
	/* ucore never check access permision */
	kls->st_mode = kstat->st_mode | 0777;
	kls->st_nlink = kstat->st_nlinks;
	kls->st_blksize = 512;
	kls->st_blocks = kstat->st_blocks;
	kls->st_size = kstat->st_size;

	ret = 0;
	lock_mm(mm);
	{
		if (!copy_to_user(mm, buf, kls, sizeof(struct linux_stat))) {
			ret = -1;
		}
	}
	unlock_mm(mm);
	kfree(kls);
	return ret;
}

int sysfile_linux_stat(const char __user *path, struct linux_stat *__user linux_stat_store)
{
	//Ensure that buf is a valid userspace address
  if(!user_mem_check(current->mm, (uintptr_t)linux_stat_store, sizeof(struct linux_stat), 1)) {
    return -E_FAULT;
  }
	struct stat ucore_stat;
  int ret;
	if ((ret = file_stat(path, &ucore_stat)) != 0) {
		return ret;
	}
  lock_mm(current->mm);
	ucore_stat_to_linux_stat(&ucore_stat, linux_stat_store);
  unlock_mm(current->mm);
	return 0;
}

int sysfile_linux_lstat(const char __user *path, struct linux_stat *__user linux_stat_store)
{
	//Ensure that buf is a valid userspace address
  if(!user_mem_check(current->mm, (uintptr_t)linux_stat_store, sizeof(struct linux_stat), 1)) {
    return -E_FAULT;
  }
	struct stat ucore_stat;
  int ret;
	if ((ret = file_lstat(path, &ucore_stat)) != 0) {
		return ret;
	}
  lock_mm(current->mm);
	ucore_stat_to_linux_stat(&ucore_stat, linux_stat_store);
  unlock_mm(current->mm);
	return 0;
}

#ifndef __UCORE_64__
int sysfile_linux_fstat64(int fd, struct linux_stat64 __user * linux_stat_store)
{
  //Ensure that buf is a valid userspace address
  if(!user_mem_check(current->mm, (uintptr_t)linux_stat_store, sizeof(struct linux_stat64), 1)) {
    return -E_FAULT;
  }
	struct stat ucore_stat;
  int ret;
	if ((ret = file_fstat(fd, &ucore_stat)) != 0) {
		return ret;
	}
  lock_mm(current->mm);
	ucore_stat_to_linux_stat64(&ucore_stat, linux_stat_store);
  unlock_mm(current->mm);
	return 0;
}

int sysfile_linux_stat64(const char __user *path, struct linux_stat64 *__user linux_stat_store)
{
	//Ensure that buf is a valid userspace address
  if(!user_mem_check(current->mm, (uintptr_t)linux_stat_store, sizeof(struct linux_stat64), 1)) {
    return -E_FAULT;
  }
	struct stat ucore_stat;
  int ret;
	if ((ret = file_stat(path, &ucore_stat)) != 0) {
		return ret;
	}
  lock_mm(current->mm);
	ucore_stat_to_linux_stat64(&ucore_stat, linux_stat_store);
  unlock_mm(current->mm);
	return 0;
}

int sysfile_linux_lstat64(const char __user *path, struct linux_stat64 *__user linux_stat_store)
{
	//Ensure that buf is a valid userspace address
  if(!user_mem_check(current->mm, (uintptr_t)linux_stat_store, sizeof(struct linux_stat64), 1)) {
    return -E_FAULT;
  }
	struct stat ucore_stat;
  int ret;
	if ((ret = file_lstat(path, &ucore_stat)) != 0) {
		return ret;
	}
  lock_mm(current->mm);
	ucore_stat_to_linux_stat64(&ucore_stat, linux_stat_store);
  unlock_mm(current->mm);
	return 0;
}
#endif /* __UCORE_64__ */

size_t sysfile_readlink(const char __user *pathname, char __user *base, size_t len)
{
	if(!user_mem_check(current->mm, (uintptr_t)base, len, 1)) {
		return -E_FAULT;
	}
	int ret;
	struct inode *node;
	if ((ret = vfs_lookup(pathname, &node, false)) != 0) {
		return ret;
	}
	int node_type;
	vop_gettype(node, &node_type);
	if(node_type != S_IFLNK) {
		vop_ref_dec(node);
		return -E_INVAL;
	}
	char *buffer = kmalloc(4096);
	int length = 0;
	lock_mm(current->mm);
	ret = vop_readlink(node, buffer);
	unlock_mm(current->mm);
	vop_ref_dec(node);
	if(ret != 0) {
		kfree(buffer);
		return ret;
	}
	length = strlen(buffer);
	length = length < len ? length : len;
	memcpy(base, buffer, len);
	kfree(buffer);
	return length;
}

#define F_DUPFD		0	/* dup */
#define F_GETFD		1	/* get close_on_exec */
#define F_SETFD		2	/* set/clear close_on_exec */
#define F_GETFL		3	/* get file->f_flags */
#define F_SETFL		4	/* set file->f_flags */

int sysfile_linux_fcntl64(int fd, int cmd, int arg)
{
	kprintf("FCNTL: %d\n", cmd);
  //const static int F_DUPFD = 0;
  if(cmd == F_DUPFD) {
    int ret =  file_dup(fd, arg);
		kprintf("DUP: %d %d\n", fd, ret);
    return ret;
    //panic("fd = %d, fd = %d ret = %d", fd, arg, ret);
  }
  else if(cmd == F_SETFL) {
    struct file *file;
    int fd_type;
    if (fd2file(fd, &file) != 0 || vop_gettype(file->node, &fd_type) != 0) {
      return -E_BADF;
    }
    file->io_flags = arg;
    if(S_ISSOCK(fd_type)/* && (arg & O_NONBLOCK)*/) {
      panic("XX");
      /*linux_fd_set_set(lwip_wrapper_readfds, i);
      socket_fds++;*/
    }
    return 0;
  }
  else if(cmd == F_GETFL) {
    struct file *file;
    if (fd2file(fd, &file) != 0) {
      return -E_INVAL;
    }
    return file->io_flags;
  }
	kprintf("Unsupported option for fcntl: %d\n", cmd);
	return 0;
	return -E_INVAL;
}

int sysfile_fsync(int fd)
{
	return file_fsync(fd);
}

int sysfile_chdir(const char *__path)
{
	int ret;
	char *path;
	if ((ret = copy_path(&path, __path)) != 0) {
		return ret;
	}
	ret = vfs_chdir(path);
	kfree(path);
	return ret;
}

int sysfile_mkdir(const char *__path)
{
	int ret;
	char *path;
	if ((ret = copy_path(&path, __path)) != 0) {
		return ret;
	}
	ret = vfs_mkdir(path);
	kfree(path);
	return ret;
}

int sysfile_link(const char *__path1, const char *__path2)
{
	int ret;
	char *old_path, *new_path;
	if ((ret = copy_path(&old_path, __path1)) != 0) {
		return ret;
	}
	if ((ret = copy_path(&new_path, __path2)) != 0) {
		kfree(old_path);
		return ret;
	}
	ret = vfs_link(old_path, new_path);
	kfree(old_path), kfree(new_path);
	return ret;
}

int sysfile_rename(const char *__path1, const char *__path2)
{
	int ret;
	char *old_path, *new_path;
	if ((ret = copy_path(&old_path, __path1)) != 0) {
		return ret;
	}
	if ((ret = copy_path(&new_path, __path2)) != 0) {
		kfree(old_path);
		return ret;
	}
	ret = vfs_rename(old_path, new_path);
	kfree(old_path), kfree(new_path);
	return ret;
}

int sysfile_unlink(const char *__path)
{
	int ret;
	char *path;
	if ((ret = copy_path(&path, __path)) != 0) {
		return ret;
	}
	ret = vfs_unlink(path);
	kfree(path);
	return ret;
}

int sysfile_getcwd(char *buf, size_t len)
{
	kprintf("Entering sysfile_getcwd %x %x %d\n", buf, buf, len);
	struct mm_struct *mm = current->mm;
	if (len == 0) {
		return -E_INVAL;
	}

	lock_mm(mm);
	{
		if (user_mem_check(mm, (uintptr_t) buf, len, 1)) {
			struct iobuf __iob, *iob =
			    iobuf_init(&__iob, buf, len, 0);
			vfs_getcwd(iob);
		}
	}
	unlock_mm(mm);
	kprintf("SScwd %s %d\n", buf, len);
	return 0;
}

/*
 * TODO: The current implementation of sysfile_getdirentry and
 * sysfile_getdirentry64 is very inefficient. It wastes a lot of memory, only
 * returning one entry a time. The following code needs cleanup.
 */
int sysfile_getdirentry(int fd, struct dirent *__direntp, uint32_t * len_store)
{
	struct mm_struct *mm = current->mm;
	struct dirent *direntp;
	if ((direntp = kmalloc(sizeof(struct dirent))) == NULL) {
		return -E_NO_MEM;
	}
	memset(direntp, 0, sizeof(struct dirent));
	direntp->d_reclen = sizeof(struct dirent);
	/* libc will ignore entries with d_ino==0 */
	direntp->d_ino = 1;

	int ret = 0;
	lock_mm(mm);
	{
		if (!copy_from_user
		    (mm, &(direntp->d_off), &(__direntp->d_off),
		     sizeof(direntp->d_off), 1)) {
			ret = -E_INVAL;
		}
	}
	unlock_mm(mm);

	if (ret != 0 || (ret = file_getdirentry(fd, direntp)) != 0) {
		goto out;
	}

	lock_mm(mm);
	{
		if (!copy_to_user
		    (mm, __direntp, direntp, sizeof(struct dirent))) {
			ret = -E_INVAL;
		}
	}
	unlock_mm(mm);
	if (len_store) {
		*len_store = (direntp->d_name[0]) ? direntp->d_reclen : 0;
	}
out:
	kfree(direntp);
	return ret;
}

int sysfile_getdirentry64(int fd, struct dirent64 *__direntp, uint32_t * len_store)
{
	struct mm_struct *mm = current->mm;
	struct dirent64 *direntp;
	if ((direntp = kmalloc(sizeof(struct dirent64))) == NULL) {
		return -E_NO_MEM;
	}
	memset(direntp, 0, sizeof(struct dirent64));
	direntp->d_reclen = sizeof(struct dirent64);
	/* libc will ignore entries with d_ino==0 */
	direntp->d_ino = 1;

	int ret = 0;
	lock_mm(mm);
	{
		if (!copy_from_user
		    (mm, &(direntp->d_off), &(__direntp->d_off),
		     sizeof(direntp->d_off), 1)) {
			ret = -E_INVAL;
		}
	}
	unlock_mm(mm);

	if (ret != 0 || (ret = file_getdirentry64(fd, direntp)) != 0) {
		goto out;
	}

	lock_mm(mm);
	{
		if (!copy_to_user
		    (mm, __direntp, direntp, sizeof(struct dirent64))) {
			ret = -E_INVAL;
		}
	}
	unlock_mm(mm);
	if (len_store) {
		*len_store = (direntp->d_name[0]) ? direntp->d_reclen : 0;
	}
out:
	kfree(direntp);
	return ret;
}

#if 0
int sysfile_linux_getdents(int fd, struct linux_dirent *__dir, uint32_t count)
{
	struct mm_struct *mm = current->mm;
	struct linux_dirent *dir;
	int ret = 0;
	if (count < sizeof(struct linux_dirent))
		return -1;

	if ((dir = kmalloc(sizeof(struct linux_dirent))) == NULL) {
		goto out;
	}
	memset(dir, 0, sizeof(struct linux_dirent));

	lock_mm(mm);
	{
		if (!copy_from_user
		    (mm, &(dir->d_off), &(__dir->d_off), sizeof(dir->d_off),
		     1)) {
			ret = -1;
		}
	}
	unlock_mm(mm);
	direntp->offset = dir->d_off;

	if (ret != 0 || (ret = file_getdirentry(fd, direntp)) != 0) {
		goto put_linuxdir;
	}

	dir->d_reclen = sizeof(struct linux_dirent);
	dir->d_off = direntp->offset;
	dir->d_ino = 1;
	strcpy(dir->d_name, direntp->name);

	lock_mm(mm);
	{
		if (!copy_to_user(mm, __dir, dir, sizeof(struct linux_dirent))) {
			ret = -1;
		}
	}
	unlock_mm(mm);
	ret = dir->d_reclen;
	/* done */
	if (!dir->d_name[0])
		ret = 0;
put_linuxdir:
	kfree(dir);
out:
	kfree(direntp);
	return ret;
}
#endif

int sysfile_dup1(int fd)
{
  return file_dup(fd, NO_FD);
}

int sysfile_dup(int fd1, int fd2)
{
	return file_dup(fd1, fd2);
}

int sysfile_pipe(int *fd_store)
{
	struct mm_struct *mm = current->mm;
	int ret, fd[2];
	if (!user_mem_check(mm, (uintptr_t) fd_store, sizeof(fd), 1)) {
		return -E_INVAL;
	}
	if ((ret = file_pipe(fd)) == 0) {
		lock_mm(mm);
		{
			if (!copy_to_user(mm, fd_store, fd, sizeof(fd))) {
				ret = -E_INVAL;
			}
		}
		unlock_mm(mm);
		if (ret != 0) {
			file_close(fd[0]), file_close(fd[1]);
		}
	}
  kprintf("Creating pipe %d %d!\n", fd[0], fd[1]);
	return ret;
}

int sysfile_mkfifo(const char *__name, uint32_t open_flags)
{
	int ret;
	char *name;
	if ((ret = copy_path(&name, __name)) != 0) {
		return ret;
	}
	ret = file_mkfifo(name, open_flags);
	kfree(name);
	return ret;
}

int sysfile_ioctl(int fd, unsigned int cmd, unsigned long arg)
{
	if (!file_testfd(fd, 1, 0)) {
		return -E_INVAL;
	}
	if (!__is_linux_devfile(fd)) {
		return -E_INVAL;
	}
	return linux_devfile_ioctl(fd, cmd, arg);
}

void *sysfile_linux_mmap2(void *addr, size_t len, int prot, int flags,
			  int fd, size_t pgoff)
{
  //kprintf("sysfile_linux_mmap2, len = %d, pgoff = %d\n", len, pgoff);
	if (!file_testfd(fd, 1, 0)) {
		return MAP_FAILED;
	}
	if (__is_linux_devfile(fd)) {
		return linux_devfile_mmap2(addr, len, prot, flags, fd, pgoff);
	}
	else {
    return linux_regfile_mmap2(addr, len, prot, flags, fd, pgoff);
	}
	warn("mmap not implemented except ARM architecture.\n");
	return MAP_FAILED;
}

int sysfile_linux_select(int nfds, linux_fd_set_t *readfds, linux_fd_set_t *writefds,
  linux_fd_set_t *exceptfds, struct linux_timeval *timeout)
{
  kprintf("Entering sysfile_linux_select\n");
  int ret;
  linux_fd_set_t *lwip_wrapper_readfds = kmalloc(sizeof(linux_fd_set_t));
  linux_fd_set_t *lwip_wrapper_writefds = kmalloc(sizeof(linux_fd_set_t));
  linux_fd_set_t *lwip_wrapper_exceptfds = kmalloc(sizeof(linux_fd_set_t));
  linux_fd_set_t *ucore_readfds = kmalloc(sizeof(linux_fd_set_t));
  linux_fd_set_t *ucore_writefds = kmalloc(sizeof(linux_fd_set_t));
  linux_fd_set_t *ucore_exceptfds = kmalloc(sizeof(linux_fd_set_t));
  struct linux_timeval *ktimeout;
  memset(lwip_wrapper_readfds, 0, sizeof(linux_fd_set_t));
  memset(lwip_wrapper_writefds, 0, sizeof(linux_fd_set_t));
  memset(lwip_wrapper_exceptfds, 0, sizeof(linux_fd_set_t));
  memset(ucore_readfds, 0, sizeof(linux_fd_set_t));
  memset(ucore_writefds, 0, sizeof(linux_fd_set_t));
  memset(ucore_exceptfds, 0, sizeof(linux_fd_set_t));
  if(timeout != NULL) {
    ktimeout = kmalloc(sizeof(struct linux_timeval));
    memcpy(ktimeout, timeout, sizeof(struct linux_timeval));
  }
  else {
    ktimeout = NULL;
  }

  int socket_fds = 0;
  int other_fds = 0;
  for(int i = 0; i < nfds; i++) {
    struct file* file;
    int fd_type;
    if(readfds != NULL && linux_fd_set_is_set(readfds, i)) {
      if (fd2file(i, &file) != 0 || vop_gettype(file->node, &fd_type) != 0) {
        ret = -E_BADF;
        goto out;
      }
      if(S_ISSOCK(fd_type)) {
        linux_fd_set_set(lwip_wrapper_readfds, i);
        socket_fds++;
      }
      else {
        linux_fd_set_set(ucore_readfds, i);
        other_fds++;
      }
    }
    if(writefds != NULL && linux_fd_set_is_set(writefds, i)) {
      if (fd2file(i, &file) != 0 || vop_gettype(file->node, &fd_type) != 0) {
        ret = -E_BADF;
        goto out;
      }
      if(S_ISSOCK(fd_type)) {
        linux_fd_set_set(lwip_wrapper_writefds, i);
        socket_fds++;
      }
      else {
        linux_fd_set_set(ucore_writefds, i);
        other_fds++;
      }
    }
    if(exceptfds != NULL && linux_fd_set_is_set(exceptfds, i)) {
      if (fd2file(i, &file) != 0 || vop_gettype(file->node, &fd_type) != 0) {
        ret = -E_BADF;
        goto out;
      }
      if(S_ISSOCK(fd_type)) {
                kprintf("Adding excfd %d\n", i);
        linux_fd_set_set(lwip_wrapper_exceptfds, i);
        socket_fds++;
      }
      else {
        //TODO: Not implemented.
        /*linux_fd_set_set(ucore_exceptfds, i);
        other_fds++;*/
      }
    }
  }
  if(socket_fds == 0 && other_fds == 0) {
    ret = 0;
    goto out;
  }
  else if(socket_fds != 0 && other_fds == 0) {
      kprintf("XXXX = %x\n", *(uint32_t*)lwip_wrapper_exceptfds);
    ret = socket_lwip_select_wrapper(
      nfds, lwip_wrapper_readfds, lwip_wrapper_writefds, lwip_wrapper_exceptfds,
      ktimeout
    );
  }
  else {
    wait_t *wait = kmalloc(sizeof(wait_t) * other_fds);
    for(int i = 0; i < other_fds; i++) {
      wait_init(&wait[i], current);
      wait[i].wait_queue = NULL;
    }
    int ready_fd_count = 0;
    for(int i = 0, j = 0; i < nfds; i++) {
      struct file* file = NULL;
      if(fd2file(i, &file) != 0) continue;
      struct inode* inode = file->node;
      if(linux_fd_set_is_set(ucore_readfds, i)) {
        if(inode->in_ops->vop_poll(inode, &wait[j], POLL_READ_AVAILABLE) == 0) {
          //linux_fd_set_unset(ucore_readfds, i);
        }
        else {
          ready_fd_count++;
        }
        j++;
      }
      else if(linux_fd_set_is_set(ucore_writefds, i)) {
        if(inode->in_ops->vop_poll(inode, &wait[j], POLL_WRITE_AVAILABLE) == 0) {
          //linux_fd_set_unset(ucore_writefds, i);
        }
        else {
          ready_fd_count++;
        }
        j++;
      }
    }
    volatile struct proc **lwip_notify = kmalloc(sizeof(struct proc**));
    volatile int *lwip_ret = kmalloc(sizeof(int));
    *lwip_notify = current;
    *lwip_ret = 0;
    if(socket_fds > 0) {
      socket_lwip_select_wrapper_no_block(
        nfds, lwip_wrapper_readfds, lwip_wrapper_writefds, lwip_wrapper_exceptfds,
        ktimeout, lwip_notify, lwip_ret
      );
    }
    if(ready_fd_count == 0 && (*lwip_ret) == 0) {
      if(timeout) {
        do_linux_sleep(timeout);
      }
      else {
        bool intr_flag;
        local_intr_save(intr_flag);
        current->state = PROC_SLEEPING;
        current->wait_state = WT_INTERRUPTED;
        local_intr_restore(intr_flag);
        schedule();
      }
    }
    *lwip_notify = NULL;
    for(int i = 0; i < other_fds; i++) {
      if(wait[i].wait_queue != NULL) {
        wait_queue_del(wait[i].wait_queue, &wait[i]);
      }
    }
    kfree(wait);

    ret = 0;
    for(int i = 0, j = 0; i < nfds; i++) {
      struct file* file = NULL;
      if(fd2file(i, &file) != 0) continue;
      struct inode* inode = file->node;
      if(linux_fd_set_is_set(ucore_readfds, i)) {
        if(inode->in_ops->vop_poll(inode, NULL, POLL_READ_AVAILABLE) == 0) {
          linux_fd_set_unset(ucore_readfds, i);
        }
        else ret++;
      }
      else if(linux_fd_set_is_set(ucore_writefds, i)) {
        if(inode->in_ops->vop_poll(inode, NULL, POLL_WRITE_AVAILABLE) == 0) {
          linux_fd_set_unset(ucore_writefds, i);
        }
        else ret++;
      }
    }
    if(readfds != NULL) memcpy(readfds, ucore_readfds, sizeof(linux_fd_set_t));
    if(writefds != NULL) memcpy(writefds, ucore_writefds, sizeof(linux_fd_set_t));
    if(exceptfds != NULL) memset(exceptfds, 0, sizeof(linux_fd_set_t));
    if((*lwip_ret) > 0) {
      if(readfds != NULL) linux_fd_set_or(readfds, lwip_wrapper_readfds);
      if(writefds != NULL) linux_fd_set_or(writefds, lwip_wrapper_writefds);
      if(exceptfds != NULL) linux_fd_set_or(exceptfds, lwip_wrapper_exceptfds);
    }
    ret += *lwip_ret;
  }
out:
  kfree(lwip_wrapper_readfds);
  kfree(lwip_wrapper_writefds);
  kfree(lwip_wrapper_exceptfds);
  kfree(ucore_readfds);
  kfree(ucore_writefds);
  kfree(ucore_exceptfds);
  if(ktimeout != NULL) kfree(ktimeout);
  if(writefds != NULL) {
    kprintf("Leaving sysfile_linux_select %x %x\n", *(int*)readfds, *(int*)writefds);
  }
  else
    kprintf("Leaving sysfile_linux_select %x\n", *(int*)readfds);

  return ret;
}
