/*
 * Kernel File Pool is the interface used to allocate / free kernel file objects
 * (i.e. struct file objects).
 * TODO: Currently, there isn't any real "memory pool" alogrithms.
 * You can contribute to it!
 */

 #ifndef __KERN_FS_KERNEL_FILE_POOL_H__
 #define __KERN_FS_KERNEL_FILE_POOL_H__

#include <types.h>

struct file;

void kernel_file_pool_init();
struct file* kernel_file_pool_allocate();
void kernel_file_pool_free(struct file* file);

#endif
