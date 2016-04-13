#ifndef _FREEBSD_COMPAT_TYPES_H_
#define	_FREEBSD_COMPAT_TYPES_H_

#include <types.h>

typedef int32_t __pid_t;
typedef int64_t __off_t;
typedef uint64_t uintmax_t;
typedef int64_t intmax_t;
typedef	int accmode_t;
typedef int64_t daddr_t;
typedef uint64_t u_int64_t;
typedef uint64_t vm_offset_t;
typedef uint64_t vm_pindex_t;
typedef int64_t sbintime_t;

typedef char *          caddr_t;        /* core address */
typedef const char *    c_caddr_t;      /* core address, pointer to const */

//TODO: for x64 only!
typedef int64_t register_t;
typedef uint64_t uregister_t;
typedef int64_t vm_ooffset_t;

#ifndef pid_t
typedef __pid_t         pid_t;          /* process id */
#define pid_t           __pid_t
#endif
typedef uint16_t      __mode_t;
typedef int32_t         lwpid_t;        /* LWP id */
typedef uint64_t ssize_t;

typedef int64_t time_t;
typedef uint64_t      u_quad_t;       /* quads (deprecated) */
typedef int64_t       quad_t;
typedef quad_t *        qaddr_t;

typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef uint16_t uid_t;
typedef uint16_t gid_t;

//From caprights.h
/*
 * The top two bits in the first element of the cr_rights[] array contain
 * total number of elements in the array - 2. This means if those two bits are
 * equal to 0, we have 2 array elements.
 * The top two bits in all remaining array elements should be 0.
 * The next five bits contain array index. Only one bit is used and bit position
 * in this five-bits range defines array index. This means there can be at most
 * five array elements.
 */
#define CAP_RIGHTS_VERSION_00   0
/*
#define CAP_RIGHTS_VERSION_01   1
#define CAP_RIGHTS_VERSION_02   2
#define CAP_RIGHTS_VERSION_03   3
*/
#define CAP_RIGHTS_VERSION      CAP_RIGHTS_VERSION_00
struct cap_rights {
        uint64_t        cr_rights[CAP_RIGHTS_VERSION + 2];
};
#ifndef _CAP_RIGHTS_T_DECLARED
#define _CAP_RIGHTS_T_DECLARED
typedef struct cap_rights       cap_rights_t;
#endif

#endif
