#ifndef __LIBS_X86_H__
#define __LIBS_X86_H__

#include <types.h>

#define do_div(n, base) ({                                          \
            unsigned long __upper, __low, __high, __mod, __base;    \
            __base = (base);                                        \
            __asm__("" : "=a" (__low), "=d" (__high) : "A" (n));        \
            __upper = __high;                                       \
            if (__high != 0) {                                      \
                __upper = __high % __base;                          \
                __high = __high / __base;                           \
            }                                                       \
            __asm__("divl %2" : "=a" (__low), "=d" (__mod)              \
                : "rm" (__base), "0" (__low), "1" (__upper));       \
            __asm__("" : "=A" (n) : "a" (__low), "d" (__high));         \
            __mod;                                                  \
        })

static inline uint8_t inb(uint16_t port) __attribute__ ((always_inline));
static inline void insl(uint32_t port, void *addr, int cnt)
    __attribute__ ((always_inline));
static inline void outb(uint16_t port, uint8_t data)
    __attribute__ ((always_inline));
static inline void outw(uint16_t port, uint16_t data)
    __attribute__ ((always_inline));
static inline void outsl(uint32_t port, const void *addr, int cnt)
    __attribute__ ((always_inline));
static inline uint32_t read_ebp(void) __attribute__ ((always_inline));
static inline void breakpoint(void) __attribute__ ((always_inline));
static inline uint32_t read_dr(unsigned regnum) __attribute__ ((always_inline));
static inline void write_dr(unsigned regnum, uint32_t value)
    __attribute__ ((always_inline));

/* Pseudo-descriptors used for LGDT, LLDT(not used) and LIDT instructions. */
struct pseudodesc {
	uint16_t pd_lim;	// Limit
	uintptr_t pd_base;	// Base address
} __attribute__ ((packed));

static inline void lidt(struct pseudodesc *pd) __attribute__ ((always_inline));
static inline void sti(void) __attribute__ ((always_inline));
static inline void cli(void) __attribute__ ((always_inline));
static inline void ltr(uint16_t sel) __attribute__ ((always_inline));
static inline uint32_t read_eflags(void) __attribute__ ((always_inline));
static inline void write_eflags(uint32_t eflags)
    __attribute__ ((always_inline));
static inline void lcr0(uintptr_t cr0) __attribute__ ((always_inline));
static inline void lcr3(uintptr_t cr3) __attribute__ ((always_inline));
static inline uintptr_t rcr0(void) __attribute__ ((always_inline));
static inline uintptr_t rcr1(void) __attribute__ ((always_inline));
static inline uintptr_t rcr2(void) __attribute__ ((always_inline));
static inline uintptr_t rcr3(void) __attribute__ ((always_inline));
static inline void invlpg(void *addr) __attribute__ ((always_inline));

static inline uint8_t inb(uint16_t port)
{
	uint8_t data;
	__asm__ volatile ("inb %1, %0":"=a" (data):"d"(port));
	return data;
}

static inline void insl(uint32_t port, void *addr, int cnt)
{
	__asm__ volatile ("cld;" "repne; insl;":"=D" (addr), "=c"(cnt)
		      :"d"(port), "0"(addr), "1"(cnt)
		      :"memory", "cc");
}

static inline void outb(uint16_t port, uint8_t data)
{
	__asm__ volatile ("outb %0, %1"::"a" (data), "d"(port));
}

static inline void outw(uint16_t port, uint16_t data)
{
	__asm__ volatile ("outw %0, %1"::"a" (data), "d"(port));
}

static inline void outsl(uint32_t port, const void *addr, int cnt)
{
	__asm__ volatile ("cld;" "repne; outsl;":"=S" (addr), "=c"(cnt)
		      :"d"(port), "0"(addr), "1"(cnt)
		      :"cc");
}

static inline uint32_t read_ebp(void)
{
	uint32_t ebp;
	__asm__ volatile ("movl %%ebp, %0":"=r" (ebp));
	return ebp;
}

static inline void breakpoint(void)
{
	__asm__ volatile ("int $3");
}

static inline uint32_t read_dr(unsigned regnum)
{
	uint32_t value = 0;
	switch (regnum) {
	case 0:
		__asm__ volatile ("movl %%db0, %0":"=r" (value));
		break;
	case 1:
		__asm__ volatile ("movl %%db1, %0":"=r" (value));
		break;
	case 2:
		__asm__ volatile ("movl %%db2, %0":"=r" (value));
		break;
	case 3:
		__asm__ volatile ("movl %%db3, %0":"=r" (value));
		break;
	case 6:
		__asm__ volatile ("movl %%db6, %0":"=r" (value));
		break;
	case 7:
		__asm__ volatile ("movl %%db7, %0":"=r" (value));
		break;
	}
	return value;
}

static void write_dr(unsigned regnum, uint32_t value)
{
	switch (regnum) {
	case 0:
		__asm__ volatile ("movl %0, %%db0"::"r" (value));
		break;
	case 1:
		__asm__ volatile ("movl %0, %%db1"::"r" (value));
		break;
	case 2:
		__asm__ volatile ("movl %0, %%db2"::"r" (value));
		break;
	case 3:
		__asm__ volatile ("movl %0, %%db3"::"r" (value));
		break;
	case 6:
		__asm__ volatile ("movl %0, %%db6"::"r" (value));
		break;
	case 7:
		__asm__ volatile ("movl %0, %%db7"::"r" (value));
		break;
	}
}

static inline void lidt(struct pseudodesc *pd)
{
	__asm__ volatile ("lidt (%0)"::"r" (pd));
}

static inline void sti(void)
{
	__asm__ volatile ("sti");
}

static inline void cli(void)
{
	__asm__ volatile ("cli");
}

static inline void ltr(uint16_t sel)
{
	__asm__ volatile ("ltr %0"::"r" (sel));
}

static inline uint32_t read_eflags(void)
{
	uint32_t eflags;
	__asm__ volatile ("pushfl; popl %0":"=r" (eflags));
	return eflags;
}

static inline void write_eflags(uint32_t eflags)
{
	__asm__ volatile ("pushl %0; popfl"::"r" (eflags));
}

static inline void lcr0(uintptr_t cr0)
{
	__asm__ volatile ("mov %0, %%cr0"::"r" (cr0));
}

static inline void lcr3(uintptr_t cr3)
{
	__asm__ volatile ("mov %0, %%cr3"::"r" (cr3));
}

static inline uintptr_t rcr0(void)
{
	uintptr_t cr0;
	__asm__ volatile ("mov %%cr0, %0":"=r" (cr0));
	return cr0;
}

static inline uintptr_t rcr1(void)
{
	uintptr_t cr1;
	__asm__ volatile ("mov %%cr1, %0":"=r" (cr1));
	return cr1;
}

static inline uintptr_t rcr2(void)
{
	uintptr_t cr2;
	__asm__ volatile ("mov %%cr2, %0":"=r" (cr2));
	return cr2;
}

static inline uintptr_t rcr3(void)
{
	uintptr_t cr3;
	__asm__ volatile ("mov %%cr3, %0":"=r" (cr3));
	return cr3;
}

static inline void invlpg(void *addr)
{
	__asm__ volatile ("invlpg (%0)"::"r" (addr):"memory");
}

static inline int __strcmp(const char *s1, const char *s2)
    __attribute__ ((always_inline));
static inline char *__strcpy(char *dst, const char *src)
    __attribute__ ((always_inline));
static inline void *__memset(void *s, char c, size_t n)
    __attribute__ ((always_inline));
static inline void *__memmove(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));
static inline void *__memcpy(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));

#ifndef __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRCMP
static inline int __strcmp(const char *s1, const char *s2)
{
	int d0, d1, ret;
	__asm__ volatile ("1: lodsb;"
		      "scasb;"
		      "jne 2f;"
		      "testb %%al, %%al;"
		      "jne 1b;"
		      "xorl %%eax, %%eax;"
		      "jmp 3f;"
		      "2: sbbl %%eax, %%eax;"
		      "orb $1, %%al;" "3:":"=a" (ret), "=&S"(d0), "=&D"(d1)
		      :"1"(s1), "2"(s2)
		      :"memory");
	return ret;
}

#endif /* __HAVE_ARCH_STRCMP */

#ifndef __HAVE_ARCH_STRCPY
#define __HAVE_ARCH_STRCPY
static inline char *__strcpy(char *dst, const char *src)
{
	int d0, d1, d2;
	__asm__ volatile ("1: lodsb;"
		      "stosb;"
		      "testb %%al, %%al;"
		      "jne 1b;":"=&S" (d0), "=&D"(d1), "=&a"(d2)
		      :"0"(src), "1"(dst):"memory");
	return dst;
}
#endif /* __HAVE_ARCH_STRCPY */

#ifndef __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMSET
static inline void *__memset(void *s, char c, size_t n)
{
	int d0, d1;
	__asm__ volatile ("rep; stosb;":"=&c" (d0), "=&D"(d1)
		      :"0"(n), "a"(c), "1"(s)
		      :"memory");
	return s;
}
#endif /* __HAVE_ARCH_MEMSET */

#ifndef __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMMOVE
static inline void *__memmove(void *dst, const void *src, size_t n)
{
	if (dst < src) {
		return __memcpy(dst, src, n);
	}
	int d0, d1, d2;
	__asm__ volatile ("std;"
		      "rep; movsb;" "cld;":"=&c" (d0), "=&S"(d1), "=&D"(d2)
		      :"0"(n), "1"(n - 1 + src), "2"(n - 1 + dst)
		      :"memory");
	return dst;
}
#endif /* __HAVE_ARCH_MEMMOVE */

#ifndef __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMCPY
static inline void *__memcpy(void *dst, const void *src, size_t n)
{
	int d0, d1, d2;
	__asm__ volatile ("rep; movsl;"
		      "movl %4, %%ecx;"
		      "andl $3, %%ecx;"
		      "jz 1f;"
		      "rep; movsb;" "1:":"=&c" (d0), "=&D"(d1), "=&S"(d2)
		      :"0"(n / 4), "g"(n), "1"(dst), "2"(src)
		      :"memory");
	return dst;
}
#endif /* __HAVE_ARCH_MEMCPY */

#endif /* !__LIBS_X86_H__ */
