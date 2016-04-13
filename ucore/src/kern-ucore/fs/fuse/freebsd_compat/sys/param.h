#ifndef _FREEBSD_COMPAT_PARAM_H_
#define	_FREEBSD_COMPAT_PARAM_H_

#include "priority.h"

#define	ARG_MAX			262144	/* max bytes for an exec function */
#ifndef CHILD_MAX
#define	CHILD_MAX		   40	/* max simultaneous processes */
#endif
#define	LINK_MAX		32767	/* max file link count */
#define	MAX_CANON		  255	/* max bytes in term canon input line */
#define	MAX_INPUT		  255	/* max bytes in terminal input */
#define	NAME_MAX		  255	/* max bytes in a file name */
#ifndef NGROUPS_MAX
#define	NGROUPS_MAX	 	 1023	/* max supplemental group id's */
#endif
#ifndef OPEN_MAX
#define	OPEN_MAX		   64	/* max open files per process */
#endif
#define	PATH_MAX		 1024	/* max bytes in pathname */
#define	PIPE_BUF		  512	/* max bytes for atomic pipe writes */
#define	IOV_MAX			 1024	/* max elements in i/o vector */

#define	MAXCOMLEN	19		/* max command name remembered */
#define	MAXINTERP	PATH_MAX	/* max interpreter file name length */
#define	MAXLOGNAME	33		/* max login name length (incl. NUL) */
#define	MAXUPRC		CHILD_MAX	/* max simultaneous processes */
#define	NCARGS		ARG_MAX		/* max bytes for an exec function */
#define	NGROUPS		(NGROUPS_MAX+1)	/* max number groups */
#define	NOFILE		OPEN_MAX	/* max open files per process */
#define	NOGROUP		65535		/* marker for empty group set member */
#define MAXHOSTNAMELEN	256		/* max hostname size */
#define SPECNAMELEN	63		/* max length of devicename */

#define	MAXPATHLEN	PATH_MAX
#define MAXSYMLINKS	32

#define PRIMASK 0x0ff
#define PCATCH  0x100           /* OR'd with pri for tsleep to check signals */
#define PDROP   0x200   /* OR'd with pri to stop re-entry of interlock mutex */

#define NZERO   0               /* default "nice" */
#define NBBY    8               /* number of bits in a byte */
#define NBPW    sizeof(int)     /* number of bytes per word (integer) */

#define CMASK   022             /* default file mask: S_IWGRP|S_IWOTH */
#define NODEV   (dev_t)(-1)     /* non-existent device */

#define MAXBSIZE        65536   /* must be power of 2 */
#ifndef MAXBCACHEBUF
#define MAXBCACHEBUF    MAXBSIZE /* must be a power of 2 >= MAXBSIZE */
#endif

/* clicks to bytes */
#ifndef ctob
#define ctob(x) ((x)<<PAGE_SHIFT)
#endif

/* bytes to clicks */
#ifndef btoc
#define btoc(x) (((vm_offset_t)(x)+PAGE_MASK)>>PAGE_SHIFT)
#endif

/* Macros for min/max. */
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define round_page(x)   ((((unsigned long)(x)) + PAGE_MASK) & ~(PAGE_MASK))

//TODO: This error code doesn't exist in linux and is made up.
#define E_NAMETOOLONG 531

//from copystr.c
/*
 * Emulate copyinstr.
 */
static int
copystr(kfaddr, kdaddr, len, done)
        const void *kfaddr;
        void *kdaddr;
        size_t len;
        size_t *done;
{
        const u_char *kfp = kfaddr;
        u_char *kdp = kdaddr;
        size_t l;
        int rv;

        rv = E_NAMETOOLONG;
        for (l = 0; len-- > 0; l++) {
                if (!(*kdp++ = *kfp++)) {
                        l++;
                        rv = 0;
                        break;
                }
        }
        if (done != NULL) {
                *done = l;
        }
        return rv;
}

#endif
