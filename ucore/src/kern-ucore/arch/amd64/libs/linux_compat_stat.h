#ifndef __LIBS_LINUX_COMPAT_STAT_H__
#define __LIBS_LINUX_COMPAT_STAT_H__

#include <types.h>
struct linux_stat {
  uint64_t st_dev;
	uint64_t st_ino;
  uint64_t st_nlink;

  uint32_t st_mode;
	uint32_t st_uid;
	uint32_t st_gid;
	uint32_t __pad0;

  uint64_t st_rdev;
	int64_t st_size;
	int64_t st_blksize;
	int64_t st_blocks;

	unsigned long	st_atime;
	unsigned long	st_atime_nsec;
	unsigned long	st_mtime;
	unsigned long	st_mtime_nsec;
	unsigned long	st_ctime;
	unsigned long	st_ctime_nsec;
	long __unused1[3];
};

#endif /* __LIBS_LINUX_COMPAT_STAT_H__ */
