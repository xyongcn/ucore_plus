#ifndef _FREEBSD_COMPAT_TIMESPEC_H_
#define	_FREEBSD_COMPAT_TIMESPEC_H_

#include "types.h"

struct timespec {
	time_t	tv_sec;		/* seconds */
	long	tv_nsec;	/* and nanoseconds */
};

#endif
