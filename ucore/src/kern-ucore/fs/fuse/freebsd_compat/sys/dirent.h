#ifndef _FREEBSD_COMPAT_DIRENT_H_
#define	_FREEBSD_COMPAT_DIRENT_H_

#include "types.h"

#define MAXNAMLEN 255

struct dirent {
        uint32_t d_fileno;            /* file number of entry */
        uint16_t d_reclen;            /* length of this record */
        uint8_t  d_type;              /* file type, see below */
        uint8_t  d_namlen;            /* length of string in d_name */
        char    d_name[MAXNAMLEN + 1];  /* name must be no longer than this */
};

/*
 * The _GENERIC_DIRSIZ macro gives the minimum record length which will hold
 * the directory entry.  This returns the amount of space in struct direct
 * without the d_name field, plus enough space for the name with a terminating
 * null byte (dp->d_namlen+1), rounded up to a 4 byte boundary.
 *
 * XXX although this macro is in the implementation namespace, it requires
 * a manifest constant that is not.
 */
#define _GENERIC_DIRSIZ(dp) \
   ((sizeof (struct dirent) - (MAXNAMLEN+1)) + (((dp)->d_namlen+1 + 3) &~ 3))
#define GENERIC_DIRSIZ(dp)      _GENERIC_DIRSIZ(dp)

#endif
