#include <types.h>
#include <sfs.h>
#include <error.h>
#include <assert.h>
#include <vfs.h>

#include "devfs.h"

void devfs_init(void)
{
	int ret;
	if ((ret = register_filesystem("devfs", devfs_mount)) != 0) {
		panic("failed: sfs: register_filesystem: %e.\n", ret);
	}
  return ret;
}
