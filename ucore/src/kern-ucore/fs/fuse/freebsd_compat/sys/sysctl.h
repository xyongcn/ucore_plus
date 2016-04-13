#ifndef _FREEBSD_COMPAT_SYSCTL_H_
#define	_FREEBSD_COMPAT_SYSCTL_H_

//TODO : simple ignoring those macros might not work.
#define SYSCTL_DECL(x)
#define SYSCTL_NODE(a, b, c, d, e, f)
#define SYSCTL_INT(a, b, c, d, e, f, g)
#define SYSCTL_UINT(a, b, c, d, e, f, g)
#define SYSCTL_LONG(a, b, c, d, e, f, g)
#define SYSCTL_ULONG(a, b, c, d, e, f, g)
#define SYSCTL_STRING(a, b, c, d, e, f, g)

#endif	/* !_FREEBSD_COMPAT_SYSCTL_H_ */
