#ifndef	_FREEBSD_COMPAT_BIO_H_
#define	_FREEBSD_COMPAT_BIO_H_

/* bio_cmd */
#define BIO_READ        0x01    /* Read I/O data */
#define BIO_WRITE       0x02    /* Write I/O data */
#define BIO_DELETE      0x04    /* TRIM or free blocks, i.e. mark as unused */
#define BIO_GETATTR     0x08    /* Get GEOM attributes of object */
#define BIO_FLUSH       0x10    /* Commit outstanding I/O now */
#define BIO_CMD0        0x20    /* Available for local hacks */
#define BIO_CMD1        0x40    /* Available for local hacks */
#define BIO_CMD2        0x80    /* Available for local hacks */

/* bio_flags */
#define BIO_ERROR       0x01    /* An error occurred processing this bio. */
#define BIO_DONE        0x02    /* This bio is finished. */
#define BIO_ONQUEUE     0x04    /* This bio is in a queue & not yet taken. */
/*
 * This bio must be executed after all previous bios in the queue have been
 * executed, and before any successive bios can be executed.
 */
#define BIO_ORDERED     0x08
#define BIO_UNMAPPED    0x10
#define BIO_TRANSIENT_MAPPING   0x20
#define BIO_VLIST       0x40

#endif
