#ifndef __LIBS_LINUX_COMPAT_STAT_H__
#define __LIBS_LINUX_COMPAT_STAT_H__

#include <types.h>

//TODO: Width is not checked

struct linux_stat {
	unsigned	st_dev;
	long		st_pad1[3];		/* Reserved for network id */
	uint32_t		st_ino; //ino_t
	uint32_t		st_mode; //mode_t
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
	unsigned st_rdev;
	long		st_pad2[2];
	uint32_t		st_size; //off_t
	long		st_pad3;
	/*
	 * Actually this should be timestruc_t st_atime, st_mtime and st_ctime
	 * but we don't have it under Linux.
	 */
	uint32_t st_atime; //time_t
	long st_atime_nsec;
	uint32_t st_mtime; //time_t
	long st_mtime_nsec;
	uint32_t st_ctime; //time_t
	long st_ctime_nsec;
	long st_blksize;
	long st_blocks;
	long st_pad4[14];
};

/*
 * This matches struct stat64 in glibc2.1, hence the absolutely insane
 * amounts of padding around dev_t's.  The memory layout is the same as of
 * struct stat of the 64-bit kernel.
 */

struct linux_stat64 {
	unsigned long	st_dev;
	unsigned long	st_pad0[3];	/* Reserved for st_dev expansion  */

	unsigned long long	st_ino;

	uint32_t st_mode; //mode_t
	uint32_t st_nlink;

	uint32_t st_uid; //uid_t
	uint32_t st_gid; //gid_t

	unsigned long	st_rdev;
	unsigned long	st_pad1[3];	/* Reserved for st_rdev expansion  */

	long long	st_size;

	/*
	 * Actually this should be timestruc_t st_atime, st_mtime and st_ctime
	 * but we don't have it under Linux.
	 */
	uint32_t st_atime;// time_t
	unsigned long	st_atime_nsec;	/* Reserved for st_atime expansion  */

	uint32_t st_mtime;// time_t
	unsigned long	st_mtime_nsec;	/* Reserved for st_mtime expansion  */

	uint32_t st_ctime;// time_t
	unsigned long	st_ctime_nsec;	/* Reserved for st_ctime expansion  */

	unsigned long	st_blksize;
	unsigned long	st_pad2;

	long long	st_blocks;
};

#endif /* __LIBS_LINUX_COMPAT_STAT_H__ */
