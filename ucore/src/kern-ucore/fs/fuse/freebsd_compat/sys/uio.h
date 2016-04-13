#ifndef _FREEBSD_COMPAT_UIO_H_
#define	_FREEBSD_COMPAT_UIO_H_

#include "proc.h"
#include "types.h"

enum uio_seg {
	UIO_USERSPACE,		/* from user data space */
	UIO_SYSSPACE,		/* from system space */
	UIO_NOCOPY		/* don't copy, already in object */
};

enum	uio_rw { UIO_READ, UIO_WRITE };

struct uio;

struct uio {
	struct	iovec *uio_iov;		/* scatter/gather list */
	int	uio_iovcnt;		/* length of scatter/gather list */
	off_t	uio_offset;		/* offset in target object */
	ssize_t	uio_resid;		/* remaining bytes to process */
	enum	uio_seg uio_segflg;	/* address space */
	enum	uio_rw uio_rw;		/* operation */
	struct	thread *uio_td;		/* owner */
};

static int uiomove(void *cp, int n, struct uio *uio) {
  return 0;
}

#endif
