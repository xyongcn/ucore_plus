#ifndef _FREEBSD_COMPAT_FCNTL_H_
#define	_FREEBSD_COMPAT_FCNTL_H_

#include <unistd.h>

#define FREAD           0x0001
#define FWRITE          0x0002

//#define O_NONBLOCK      0x0004          /* no delay */
//#define O_APPEND        0x0008          /* set append mode */
//#define O_SHLOCK        0x0010          /* open with shared file lock */
//#define O_EXLOCK        0x0020          /* open with exclusive file lock */
//#define O_ASYNC         0x0040          /* signal pgrp when data ready */
//#define O_FSYNC         0x0080          /* synchronous writes */
//#define O_SYNC          0x0080          /* POSIX synonym for O_FSYNC */
//#define O_NOFOLLOW      0x0100          /* don't follow symlinks */
//#define O_CREAT         0x0200          /* create if nonexistent */
//#define O_TRUNC         0x0400          /* truncate to zero length */
//#define O_EXCL          0x0800          /* error if already exists */
//#define FHASLOCK        0x4000          /* descriptor holds advisory lock */
//
//#define O_RDONLY        0x0000          /* open for reading only */
//#define O_WRONLY        0x0001          /* open for writing only */
//#define O_RDWR          0x0002          /* open for reading and writing */
//#define O_ACCMODE       0x0003          /* mask for above modes */
//
//#define O_DIRECTORY     0x00020000      /* Fail if not directory */
//#define O_EXEC          0x00040000      /* Open for execute only */

//TODO: The FreeBSD "O_" macros is different from those on Linux, and it has
// has some "Internal" flags like FREAD and FWIRTE... That's pretty weird, and
// here we ignore the O_EXEC flag, which doesn't exist on linux.
// This may lead to problems.

#define OFLAGS(fflags) ((fflags) - 1)
//#define OFLAGS(fflags)  ((fflags) & O_EXEC ? (fflags) : (fflags) - 1)

#endif
