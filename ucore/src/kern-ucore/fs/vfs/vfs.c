#include <types.h>
#include <stdio.h>
#include <string.h>
#include <slab.h>
#include <sem.h>
#include <error.h>
#include <stat.h>

#include <vfs.h>
#include <inode.h>
#include <vfsmount.h>

extern void vfs_devlist_init(void);

struct fs *__alloc_fs(int type)
{
	struct fs *fs;
	if ((fs = kmalloc(sizeof(struct fs))) != NULL) {
		fs->fs_type = type;
	}
	return fs;
}

void vfs_init(void)
{
	vfs_devlist_init();
	file_system_type_list_init();
  vfs_mount_init();
}

#define le2fstype(le, member)                         \
    to_struct((le), struct file_system_type, member)

static list_entry_t file_system_type_list;
static semaphore_t file_system_type_sem;

static void lock_file_system_type_list(void)
{
	down(&file_system_type_sem);
}

static void unlock_file_system_type_list(void)
{
	up(&file_system_type_sem);
}

static bool check_file_system_type_name_conflict(const char *name)
{
	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
		struct file_system_type *fstype =
		    le2fstype(le, file_system_type_link);
		if (strcmp(fstype->name, name) == 0) {
			return 0;
		}
	}
	return 1;
}

void file_system_type_list_init(void)
{
	list_init(&file_system_type_list);
	sem_init(&file_system_type_sem, 1);
}

int register_filesystem(struct file_system_type* fs_type)
{
	assert(fs_type->name != NULL);

	lock_file_system_type_list();
	if (!check_file_system_type_name_conflict(fs_type->name)) {
		unlock_file_system_type_list();
		return -E_EXISTS;
	}

	list_add(&file_system_type_list, &(fs_type->file_system_type_link));
	unlock_file_system_type_list();
	return 0;
}

int unregister_filesystem(struct file_system_type* fs_type)
{
	int ret = -E_INVAL;
	lock_file_system_type_list();

	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
		if (le2fstype(le, file_system_type_link) == fs_type) {
			list_del(le);
			ret = 0;
			break;
		}
	}

	unlock_file_system_type_list();
	return ret;
}

int vfs_find_filesystem_by_name(const char* name, struct file_system_type** fs_type_store)
{
	lock_file_system_type_list();

	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
    struct file_system_type *fs_type = le2fstype(le, file_system_type_link);
		if (strcmp(fs_type->name, name) == 0) {
      unlock_file_system_type_list();
      (*fs_type_store) = fs_type;
			return 0;
		}
	}

	unlock_file_system_type_list();
	return -E_INVAL;
}

/**
 * Perform mount with no extra check.
 *
 * When calling vfs_do_mount_nocheck, as long as fs_name is valid, and the
 * the mount of file_system_type returns 0, mount record will be added.
 *
 * This function may lead to weird behaviour, i.e. you can mount to
 * an non-existing directory, it won't be found by looking up the file system
 * but you can change directory into it. So this function is mainly intended for
 * internal use. However, this function is necessary to be called directly
 * for mounting devfs to /dev, as at that time "/" still doesn't exist.
 *
 */
int vfs_do_mount_nocheck(const char *devname, const char* mountpoint,
  const char *fs_name, int flags, void* data)
{
  int ret;
  struct file_system_type *fs_type;
  //TODO: Security issue: This may lead to buffer overflow and memor
  char *mountpoint_full_path = kmalloc(1024);
  if(mountpoint[0] != '/') {
    vfs_expand_path(mountpoint, mountpoint_full_path, 1024);
  }
  else {
    strcpy(mountpoint_full_path, mountpoint);
  }
  vfs_simplify_path(mountpoint_full_path);
  ret = vfs_find_filesystem_by_name(fs_name, &fs_type);
  if(ret != 0) return ret;
  struct fs *filesystem;
  ret = fs_type->mount(fs_type, flags, devname, data, &filesystem);
  if(ret != 0) return ret;
  ret = vfs_mount_add_record(mountpoint_full_path, filesystem);
  kfree(mountpoint_full_path);
  return ret;
}

int vfs_do_umount(const char* target, unsigned long flags)
{
  //TODO: Security issue: This may lead to buffer overflow and memor
  char *mountpoint_full_path = kmalloc(1024);

  //Get the full path for the mount point.
  vfs_expand_path(target, mountpoint_full_path, 1024);
  vfs_simplify_path(mountpoint_full_path);

  //Forbid unmount the root
  if(strlen(mountpoint_full_path) == 1) return -E_INVAL;

  //Get the mount record.
  int ret;
  struct vfs_mount_record* mount_record;
  ret = vfs_mount_find_record_by_mountpoint(mountpoint_full_path, &mount_record);
  if(ret != 0) return ret;

  //Perform unmounting
  ret = fsop_sync(mount_record->filesystem);
  if(ret != 0) return ret;
  ret = fsop_unmount(mount_record->filesystem);
  if(ret != 0) return ret;
  ret = vfs_mount_remove_record(mount_record);
  assert(ret == 0);
  return ret;
}

/**
 * Perform mounting, ensuring devname points to a valid directory.
 *
 * Before calling vfs_do_mount_nocheck, this function will check that devname
 * points to a valid inode, and that inode is a directory.
 *
 * @note: do_mount doesn't prevents mounting on a non-empty directory - This is
 * a valid operation on Linux system.
 *
 */
int do_mount(const char *devname, const char* mountpoint, const char *fs_name,
  unsigned long flags, void* data)
{
  int ret;
  struct inode* mountpoint_inode;
  ret = vfs_lookup(mountpoint, &mountpoint_inode);
  if(ret != 0) return -E_NOENT;
  uint32_t mountpoint_type;
  ret = vop_gettype(mountpoint_inode, &mountpoint_type);
  if(mountpoint_type != S_IFDIR) {
    return -E_NOTDIR;
  }
  return vfs_do_mount_nocheck(devname, mountpoint, fs_name, flags, data);
}

int do_umount(const char *target)
{
	return vfs_do_umount(target, 0);
}
