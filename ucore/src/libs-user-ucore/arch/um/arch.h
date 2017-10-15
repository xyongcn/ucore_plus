#ifndef __LIBS_USER_UCORE_ARCH_UM_ARCH_H__
#define __LIBS_USER_UCORE_ARCH_UM_ARCH_H__

/* Copied from the original x86.h */
#define do_div(n, base) ({										\
	unsigned long __upper, __low, __high, __mod, __base;		\
	__base = (base);											\
	__asm__("" : "=a" (__low), "=d" (__high) : "A" (n));			\
	__upper = __high;											\
	if (__high != 0) {											\
		__upper = __high % __base;								\
		__high = __high / __base;								\
	}															\
	__asm__("divl %2" : "=a" (__low), "=d" (__mod)					\
	    : "rm" (__base), "0" (__low), "1" (__upper));			\
	__asm__("" : "=A" (n) : "a" (__low), "d" (__high));				\
	__mod;														\
 })

#endif /* !__LIBS_USER_UCORE_ARCH_UM_ARCH_H__ */
