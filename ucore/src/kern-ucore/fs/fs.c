#include <types.h>
#include <slab.h>
#include <sem.h>
#include <vfs.h>
#include <dev.h>
#include <file.h>
#include <pipe.h>
#include <kernel_file_pool.h>
#include <file_desc_table.h>
#include <inode.h>
#include <assert.h>

#include <sfs/sfs.h>
#include <fatfs/ffs.h>
#include <yaffs2_direct/yaffs_vfs.h>

void fs_init(void)
{
  kernel_file_pool_init();
	vfs_init();
  devfs_init();
	dev_init();
	pipe_init();

#ifdef UCONFIG_HAVE_SFS
	sfs_init();
#endif

#ifdef UCONFIG_HAVE_FATFS
  ffs_init();
#endif

#ifdef UCONFIG_HAVE_YAFFS2
  yaffs_vfs_init();
#endif

  vfs_do_mount_nocheck("none", "/dev", "devfs", 0, NULL);
  vfs_path_init_cwd("/dev");
  vfs_do_mount_nocheck("/dev/disk2", "/", "sfs", 0, NULL);
  //vfs_do_mount_nocheck("/dev/disk2", "/", "yaffs", 0, NULL);
  //vfs_do_mount_nocheck("/dev/thinpad_flashrom", "/", "yaffs", 0, NULL);
}

void fs_cleanup(void)
{
	vfs_unmount_all();
	vfs_cleanup();
}

void lock_fs(struct fs_struct *fs_struct)
{
	down(&(fs_struct->fs_sem));
}

void unlock_fs(struct fs_struct *fs_struct)
{
	up(&(fs_struct->fs_sem));
}

struct fs_struct *fs_create(void)
{
	static_assert((int)FS_STRUCT_NENTRY > 128);
	struct fs_struct *fs_struct = kmalloc(sizeof(struct fs_struct));
  //TODO: This function needs to be rewritten.
	fs_struct->pwd = NULL;
  file_desc_table_init(&fs_struct->desc_table, 1024);
	atomic_set(&(fs_struct->fs_count), 0);
	sem_init(&(fs_struct->fs_sem), 1);
	return fs_struct;
}

struct file_desc_table* fs_get_desc_table(struct fs_struct *fs_struct)
{
  return &fs_struct->desc_table;
}

void fs_destroy(struct fs_struct *fs_struct)
{
	assert(fs_struct != NULL && fs_count(fs_struct) == 0);
	if (fs_struct->pwd != NULL) {
		vop_ref_dec(fs_struct->pwd);
	}
	file_desc_table_uninit(&fs_struct->desc_table);
	kfree(fs_struct);
}

/*void fs_closeall(struct fs_struct *fs_struct)
{
	assert(fs_struct != NULL && fs_count(fs_struct) > 0);
	int i;
	struct file *file = fs_struct->filemap;
	for (i = 3, file += 2; i < FS_STRUCT_NENTRY; i++, file++) {
		if (file->status == FD_OPENED) {
			filemap_close(file);
		}
	}
}*/

int dup_fs(struct fs_struct *to, struct fs_struct *from)
{
	assert(to != NULL && from != NULL);
	assert(fs_count(to) == 0 && fs_count(from) > 0);
	if ((to->pwd = from->pwd) != NULL) {
		vop_ref_inc(to->pwd);
	}
  file_desc_table_copy(&to->desc_table, &from->desc_table);
	/*int i;
	struct file *to_file = to->filemap, *from_file = from->filemap;
	for (i = 0; i < FS_STRUCT_NENTRY; i++, to_file++, from_file++) {
		if (from_file->status == FD_OPENED) {
			// alloc_fd first
			to_file->status = FD_INIT;
			filemap_dup(to_file, from_file);
		}
		else if(from_file->status != FD_NONE) {
			to_file->status = from_file->status;
			filemap_dup_close(to_file, from_file);
		}
	}*/
	return 0;
}
