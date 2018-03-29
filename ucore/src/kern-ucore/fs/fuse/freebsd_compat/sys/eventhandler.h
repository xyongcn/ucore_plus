#ifndef	_FREEBSD_COMPAT_EVENTHANDLER_H_
#define	_FREEBSD_COMPAT_EVENTHANDLER_H_

#include "queue.h"

struct eventhandler_entry {
        TAILQ_ENTRY(eventhandler_entry) ee_link;
        int                             ee_priority;
#define EHE_DEAD_PRIORITY       (-1)
        void                            *ee_arg;
};

typedef struct eventhandler_entry *eventhandler_tag;

#endif
