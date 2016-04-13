#ifndef _FREEBSD_COMPAT_SELINFO_H_
#define	_FREEBSD_COMPAT_SELINFO_H_

#include "proc.h"

struct selinfo {
	//struct selfdlist	si_tdlist;	/* List of sleeping threads. */
	//struct knlist		si_note;	/* kernel note list */
	struct mtx		*si_mtx;	/* Lock for tdlist. */
};

static void selrecord(struct thread *selector, struct selinfo *sip) {
}

static void selwakeuppri(struct selinfo *sip, int pri) {
}

#endif
