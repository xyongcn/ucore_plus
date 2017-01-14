#include <types.h>
#include <sfs.h>
#include <error.h>
#include <assert.h>
#include <vfs.h>

void sfs_init(void)
{
	int ret = register_filesystem(&sfs_fs_type);
	if (ret != 0) {
		panic("failed: sfs: register_filesystem: %e.\n", ret);
	}
}
