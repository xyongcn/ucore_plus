#ifndef _FREEBSD_COMPAT_UMA_H_
#define	_FREEBSD_COMPAT_UMA_H_

#define UMA_ALIGN_PTR   (sizeof(void *) - 1)    /* Alignment fit for ptr */
#define UMA_ALIGN_LONG  (sizeof(long) - 1)      /* "" long */
#define UMA_ALIGN_INT   (sizeof(int) - 1)       /* "" int */
#define UMA_ALIGN_SHORT (sizeof(short) - 1)     /* "" short */
#define UMA_ALIGN_CHAR  (sizeof(char) - 1)      /* "" char */
#define UMA_ALIGN_CACHE (0 - 1)                 /* Cache line size align */

struct uma_zone {
};

typedef struct uma_zone * uma_zone_t;

//TODO: They are actually function pointers.
typedef void* uma_dtor;
typedef void* uma_ctor;
typedef void* uma_init;
typedef void* uma_fini;

static uma_zone_t
uma_zcreate(char *name, int size, uma_ctor	ctor, uma_dtor dtor,
uma_init uminit, uma_fini fini, int align, uint16_t flags) {
  return NULL;
}

static void uma_zdestroy(uma_zone_t zone) {
}

static void* uma_zalloc_arg(uma_zone_t zone, void *arg,	int flags) {
  return NULL;
}

static void uma_zfree(uma_zone_t zone,	void *item) {
}

#endif
