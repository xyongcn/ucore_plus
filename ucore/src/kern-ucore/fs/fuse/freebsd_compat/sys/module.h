#ifndef _FREEBSD_COMPAT_MODULE_H_
#define _FREEBSD_COMPAT_MODULE_H_

typedef enum modeventtype {
        MOD_LOAD,
        MOD_UNLOAD,
        MOD_SHUTDOWN,
        MOD_QUIESCE
} modeventtype_t;

struct module;

typedef struct module *module_t;

typedef int (*modeventhand_t)(module_t, int /* modeventtype_t */, void *);

/*
 * Struct for registering modules statically via SYSINIT.
 */
typedef struct moduledata {
        const char      *name;          /* module name */
        modeventhand_t  evhand;         /* event handler */
        void            *priv;          /* extra data */
} moduledata_t;

#define DECLARE_MODULE(x, y, z, w)
#define MODULE_VERSION(x, y)

#endif
