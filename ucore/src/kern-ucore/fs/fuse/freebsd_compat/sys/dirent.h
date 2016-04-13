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

#endif
