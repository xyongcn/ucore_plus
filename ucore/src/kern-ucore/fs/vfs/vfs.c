#include <types.h>
#include <stdio.h>
#include <string.h>
#include <slab.h>
#include <sem.h>
#include <error.h>

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

int vfs_do_mount_nocheck(const char *devname, const char* mountpoint,
  const char *fs_name, int flags, void* data)
{
  int ret;
  struct file_system_type *fs_type;
  ret = vfs_find_filesystem_by_name(fs_name, &fs_type);
  if(ret != 0) return ret;
  struct fs *filesystem;
  ret = fs_type->mount(fs_type, flags, devname, data, &filesystem);
  if(ret != 0) return ret;
  return vfs_mount_add_record(mountpoint, filesystem);
}

int do_mount(const char *devname, const char* mountpoint, const char *fs_name)
{
  vfs_do_mount_nocheck(devname, mountpoint, fs_name, 0, NULL);
  /*const char* fsname = filesystem;
	int ret = -E_EXISTS;
	lock_file_system_type_list();
	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
		struct file_system_type *fstype =
		    le2fstype(le, file_system_type_link);
		if (strcmp(fstype->name, fsname) == 0) {
			assert(fstype->mount);
			ret = (fstype->mount) (devname);
			break;
		}
	}
	unlock_file_system_type_list();
	return ret;*/
}

int do_umount(const char *devname)
{
	return vfs_unmount(devname);
}
