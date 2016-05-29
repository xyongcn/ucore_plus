#ifndef _FREEBSD_COMPAT_UIO_H_
#define	_FREEBSD_COMPAT_UIO_H_

#include <iobuf.h>
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
	struct iovec *uio_iov;		/* scatter/gather list */
	int	uio_iovcnt;		/* length of scatter/gather list */
	off_t	uio_offset;		/* offset in target object */
	ssize_t	uio_resid;		/* remaining bytes to process */
	enum	uio_seg uio_segflg;	/* address space */
	enum	uio_rw uio_rw;		/* operation */
	struct	thread *uio_td;		/* owner */
};

static int uiomove(void *cp, int n, struct uio *uio)
{
  kprintf("TODO! uiomove is performing direct memcpy, without address space check.\r\n");
	struct iovec *iov;
	size_t cnt;

	while (n > 0 && uio->uio_resid) {
		iov = uio->uio_iov;
		cnt = iov->iov_len;
		if (cnt == 0) {
			uio->uio_iov++;
			uio->uio_iovcnt--;
			continue;
		}
		if (cnt > n)
			cnt = n;

		switch (uio->uio_segflg) {

		case UIO_USERSPACE:
    case UIO_SYSSPACE:
			if (uio->uio_rw == UIO_READ)
        memcpy(iov->iov_base, cp, cnt);
			else
				memcpy(cp, iov->iov_base, cnt);
			break;
		case UIO_NOCOPY:
			break;
		}
		iov->iov_base = (char *)iov->iov_base + cnt;
		iov->iov_len -= cnt;
		uio->uio_resid -= cnt;
		uio->uio_offset += cnt;
		cp = (char *)cp + cnt;
		n -= cnt;
	}
	return 0;
}

#endif
