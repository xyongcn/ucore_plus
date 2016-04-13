#ifndef __LIBS_ERROR_H__
#define __LIBS_ERROR_H__

#if 0
/* kernel error codes -- keep in sync with list in lib/printfmt.c */
#define E_UNSPECIFIED       1	// Unspecified or unknown problem
#define E_BAD_PROC          2	// Process doesn't exist or otherwise
#define E_INVAL             3	// Invalid parameter
#define E_NO_MEM            4	// Request failed due to memory shortage
#define E_NO_FREE_PROC      5	// Attempt to create a new process beyond
#define E_FAULT             6	// Memory fault
#define E_SWAP_FAULT        7	// SWAP READ/WRITE fault
#define E_INVAL_ELF         8	// Invalid elf file
#define E_KILLED            9	// Process is killed
#define E_PANIC             10	// Panic Failure
#define E_TIMEOUT           11	// Timeout
#define E_TOO_BIG           12	// Argument is Too Big
#define E_NO_DEV            13	// No such Device
#define E_NA_DEV            14	// Device Not Available
#define E_BUSY              15	// Device/File is Busy
#define E_NOENT             16	// No Such File or Directory
#define E_ISDIR             17	// Is a Directory
#define E_NOTDIR            18	// Not a Directory
#define E_XDEV              19	// Cross Device-Link
#define E_UNIMP             20	// Unimplemented Feature
#define E_SEEK              21	// Illegal Seek
#define E_MAX_OPEN          22	// Too Many Files are Open
#define E_EXISTS            23	// File/Directory Already Exists
#define E_NOTEMPTY          24	// Directory is Not Empty
#endif

#define E_PERM		 1	/* Operation not permitted */
#define E_NOENT		 2	/* No such file or directory */
#define E_SRCH		 3	/* No such process */
#define E_INTR		 4	/* Interrupted system call */
#define E_IO		 5	/* I/O error */
#define E_NXIO		 6	/* No such device or address */
#define E_2BIG		 7	/* Argument list too long */
#define E_NOEXEC		 8	/* Exec format error */
#define E_BADF		 9	/* Bad file number */
#define E_CHILD		10	/* No child processes */
#define E_AGAIN		11	/* Try again */
#define E_NOMEM		12	/* Out of memory */
#define E_ACCES		13	/* Permission denied */
#define E_FAULT		14	/* Bad address */
#define E_NOTBLK		15	/* Block device required */
#define E_BUSY		16	/* Device or resource busy */
#define E_EXIST		17	/* File exists */
#define E_XDEV		18	/* Cross-device link */
#define E_NODEV		19	/* No such device */
#define E_NOTDIR		20	/* Not a directory */
#define E_ISDIR		21	/* Is a directory */
#define E_INVAL		22	/* Invalid argument */
#define E_NFILE		23	/* File table overflow */
#define E_MFILE		24	/* Too many open files */
#define E_NOTTY		25	/* Not a typewriter */
#define E_TXTBSY		26	/* Text file busy */
#define E_FBIG		27	/* File too large */
#define E_NOSPC		28	/* No space left on device */
#define E_SPIPE		29	/* Illegal seek */
#define E_ROFS		30	/* Read-only file system */
#define E_MLINK		31	/* Too many links */
#define E_PIPE		32	/* Broken pipe */
#define E_DOM		33	/* Math argument out of domain of func */
#define E_RANGE		34	/* Math result not representable */

//ucore
#define E_UNIMP   35
#define E_PANIC   36
#define E_KILLED  37
//#define E_UNSPECIFIED  38
//TODO: E_UNSPECIFIED seems never used, and linux standard description is ENOSYS
#define E_NOSYS  38
#define E_SWAP_FAULT   39

#define E_NO_MEM E_NOMEM
#define E_INVAL_ELF E_NOEXEC
#define E_NO_FREE_PROC E_BUSY
#define E_BAD_PROC E_SRCH
#define E_MAX_OPEN  E_MFILE
#define E_NA_DEV  E_BUSY
#define E_EXISTS  E_EXIST
#define E_TOO_BIG E_FBIG
#define E_NOTEMPTY  E_ISDIR
#define E_NO_DEV  E_NODEV
#define E_TIMEOUT  E_BUSY
#define E_SEEK    E_SPIPE

/* the maximum allowed */
#define MAXERROR            39

#define E_BFONT          59      /* Bad font file format */
#define E_NOSTR          60      /* Device not a stream */
#define E_NODATA         61      /* No data available */
#define E_TIME           62      /* Timer expired */
#define E_NOSR           63      /* Out of streams resources */
#define E_NONET          64      /* Machine is not on the network */
#define E_NOPKG          65      /* Package not installed */
#define E_REMOTE         66      /* Object is remote */
#define E_NOLINK         67      /* Link has been severed */
#define E_ADV            68      /* Advertise error */
#define E_SRMNT          69      /* Srmount error */
#define E_COMM           70      /* Communication error on send */
#define E_PROTO          71      /* Protocol error */
#define E_MULTIHOP       72      /* Multihop attempted */
#define E_DOTDOT         73      /* RFS specific error */
#define E_BADMSG         74      /* Not a data message */
#define E_OVERFLOW       75      /* Value too large for defined data type */
#define E_NOTUNIQ        76      /* Name not unique on network */
#define E_BADFD          77      /* File descriptor in bad state */
#define E_REMCHG         78      /* Remote address changed */
#define E_LIBACC         79      /* Can not access a needed shared library */
#define E_LIBBAD         80      /* Accessing a corrupted shared library */
#define E_LIBSCN         81      /* .lib section in a.out corrupted */
#define E_LIBMAX         82      /* Attempting to link in too many shared libraries */
#define E_LIBEXEC        83      /* Cannot exec a shared library directly */
#define E_ILSEQ          84      /* Illegal byte sequence */
#define E_RESTART        85      /* Interrupted system call should be restarted */
#define E_STRPIPE        86      /* Streams pipe error */
#define E_USERS          87      /* Too many users */
#define E_NOTSOCK        88      /* Socket operation on non-socket */
#define E_DESTADDRREQ    89      /* Destination address required */
#define E_MSGSIZE        90      /* Message too long */
#define E_PROTOTYPE      91      /* Protocol wrong type for socket */
#define E_NOPROTOOPT     92      /* Protocol not available */
#define E_PROTONOSUPPORT 93      /* Protocol not supported */
#define E_SOCKTNOSUPPORT 94      /* Socket type not supported */
#define E_OPNOTSUPP      95      /* Operation not supported on transport endpoint */
#define E_PFNOSUPPORT    96      /* Protocol family not supported */
#define E_AFNOSUPPORT    97      /* Address family not supported by protocol */
#define E_ADDRINUSE      98      /* Address already in use */
#define E_ADDRNOTAVAIL   99      /* Cannot assign requested address */
#define E_NETDOWN        100     /* Network is down */
#define E_NETUNREACH     101     /* Network is unreachable */
#define E_NETRESET       102     /* Network dropped connection because of reset */
#define E_CONNABORTED    103     /* Software caused connection abort */
#define E_CONNRESET      104     /* Connection reset by peer */
#define E_NOBUFS         105     /* No buffer space available */
#define E_ISCONN         106     /* Transport endpoint is already connected */
#define E_NOTCONN        107     /* Transport endpoint is not connected */
#define E_SHUTDOWN       108     /* Cannot send after transport endpoint shutdown */
#define E_TOOMANYREFS    109     /* Too many references: cannot splice */
#define E_TIMEDOUT       110     /* Connection timed out */
#define E_CONNREFUSED    111     /* Connection refused */
#define E_HOSTDOWN       112     /* Host is down */
#define E_HOSTUNREACH    113     /* No route to host */
#define E_ALREADY        114     /* Operation already in progress */
#define E_INPROGRESS     115     /* Operation now in progress */
#define E_STALE          116     /* Stale file handle */
#define E_UCLEAN         117     /* Structure needs cleaning */
#define E_NOTNAM         118     /* Not a XENIX named type file */
#define E_NAVAIL         119     /* No XENIX semaphores available */
#define E_ISNAM          120     /* Is a named type file */
#define E_REMOTEIO       121     /* Remote I/O error */
#define E_DQUOT          122     /* Quota exceeded */
#define E_NOMEDIUM       123     /* No medium found */
#define E_MEDIUMTYPE     124     /* Wrong medium type */
#define E_CANCELED       125     /* Operation Canceled */
#define E_NOKEY          126     /* Required key not available */
#define E_KEYEXPIRED     127     /* Key has expired */
#define E_KEYREVOKED     128     /* Key has been revoked */
#define E_KEYREJECTED    129     /* Key was rejected by service */

/* for robust mutexes */
#define E_OWNERDEAD      130     /* Owner died */
#define E_NOTRECOVERABLE 131     /* State not recoverable */
#define E_RFKILL         132     /* Operation not possible due to RF-kill */
#define E_HWPOISON       133     /* Memory page has hardware error */

#endif /* !__LIBS_ERROR_H__ */
