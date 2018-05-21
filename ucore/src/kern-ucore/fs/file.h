#ifndef __KERN_FS_FILE_H__
#define __KERN_FS_FILE_H__

#include <types.h>
#include <atomic.h>
#include <assert.h>
#include <vfs.h>

#include "fs.h"
#include "kernel_file_pool.h"

struct inode;
struct stat;
struct dirent;

#ifdef __NO_UCORE_FILE__
struct ucore_file {
#else
struct file {
#endif
	bool readable;
	bool writable;
	off_t pos;
  int io_flags;
	struct inode *node;
	atomic_t open_count;
};

void filemap_acquire(struct file *file);
void filemap_release(struct file *file);

bool file_testfd(int fd, bool readable, bool writable);

int file_init(struct file *file);
int file_open(char *path, uint32_t open_flags);
int file_stat(const char *path, struct stat *stat);
int file_lstat(const char *path, struct stat *stat);
int file_close(int fd);
int file_read(int fd, void *base, size_t len, size_t * copied_store);
int file_write(int fd, void *base, size_t len, size_t * copied_store);
int file_seek(int fd, off_t pos, int whence);
int file_fstat(int fd, struct stat *stat);
int file_fsync(int fd);
int file_getdirentry(int fd, struct dirent *dirent);
int file_dup(int fd1, int fd2);
int file_pipe(int fd[]);
int file_mkfifo(const char *name, uint32_t open_flags);
int fd2file(int fd, struct file **file_store);

int linux_devfile_read(int fd, void *base, size_t len, size_t * copied_store);
int linux_devfile_write(int fd, void *base, size_t len, size_t * copied_store);
int linux_devfile_ioctl(int fd, unsigned int cmd, unsigned long arg);
void *linux_devfile_mmap2(void *addr, size_t len, int prot, int flags, int fd,
			  size_t pgoff);

static inline int fopen_count(struct file *file)
{
	return atomic_read(&(file->open_count));
}

static inline int fopen_count_inc(struct file *file)
{
	return atomic_add_return(&(file->open_count), 1);
}

static inline int fopen_count_dec(struct file *file)
{
	int ret = atomic_sub_return(&(file->open_count), 1);
  if(ret == 0) {
    vfs_close(file->node);
    kernel_file_pool_free(file);
  }
  return ret;
}
struct file* fd2file_onfs(int fd, struct fs_struct *fs_struct);
void *linux_regfile_mmap2(void *addr, size_t len, int prot, int flags, int fd,
			  size_t off);

#endif /* !__KERN_FS_FILE_H__ */
