#ifndef __LIBS_DIRENT_H__
#define __LIBS_DIRENT_H__

#include <types.h>
#include <unistd.h>

/*
 * Declaration of dirent and dirent64 structure.
 *
 * Those declarations are compatible with linux getdents and getdents64
 * system calls, and may be NOT compatible with POSIX standard. For such reason
 * they should be only used by the kernel and toy utils.
 *
 */

struct dirent {
	unsigned long d_ino;	/* Inode number */
	unsigned long d_off;	/* Offset to next linux_dirent */
	unsigned short d_reclen;	/* Length of this linux_dirent */
	char d_name[FS_MAX_FNAME_LEN + 1];	/* Filename (null-terminated) */
	/* length is actually (d_reclen - 2 -
	   offsetof(struct linux_dirent, d_name) */
	   //char           pad;       // Zero padding byte
	   //char           d_type;    // File type (only since Linux 2.6.4;
	   // offset is (d_reclen - 1))
};

struct dirent64 {
  uint64_t        d_ino;    /* 64-bit inode number */
  uint64_t        d_off;    /* 64-bit offset to next structure */
  unsigned short d_reclen; /* Size of this dirent */
  unsigned char  d_type;   /* File type */
  char           d_name[FS_MAX_FNAME_LEN + 1]; /* Filename (null-terminated) */
};

#endif /* !__LIBS_DIRENT_H__ */
