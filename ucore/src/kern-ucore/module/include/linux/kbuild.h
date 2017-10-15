#ifndef __LINUX_KBUILD_H
#define __LINUX_KBUILD_H

#define DEFINE(sym, val) \
        __asm__ volatile("\n->" #sym " %0 " #val : : "i" (val))

#define BLANK() __asm__ volatile("\n->" : : )

#define OFFSET(sym, str, mem) \
	DEFINE(sym, offsetof(struct str, mem))

#define COMMENT(x) \
	__asm__ volatile("\n->#" x)

#endif
