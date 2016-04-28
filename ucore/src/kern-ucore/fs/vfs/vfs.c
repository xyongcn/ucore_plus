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

int register_filesystem(const char *name, int (*mount) (const char *devname))
{
	assert(name != NULL);
	if (strlen(name) > FS_MAX_DNAME_LEN) {
		return -E_TOO_BIG;
	}

	int ret = -E_NO_MEM;
	char *s_name;
	if ((s_name = strdup(name)) == NULL) {
		return ret;
	}

	struct file_system_type *fstype;
	if ((fstype = kmalloc(sizeof(struct file_system_type))) == NULL) {
		goto failed_cleanup_name;
	}

	ret = -E_EXISTS;
	lock_file_system_type_list();
	if (!check_file_system_type_name_conflict(s_name)) {
		unlock_file_system_type_list();
		goto failed_cleanup_fstype;
	}
	fstype->name = s_name;
	fstype->mount = mount;

	list_add(&file_system_type_list, &(fstype->file_system_type_link));
	unlock_file_system_type_list();
	return 0;

failed_cleanup_fstype:
	kfree(fstype);
failed_cleanup_name:
	kfree(s_name);
	return ret;
}

int unregister_filesystem(const char *name)
{
	int ret = -E_EXISTS;
	lock_file_system_type_list();
	list_entry_t *list = &file_system_type_list, *le = list;
	while ((le = list_next(le)) != list) {
		struct file_system_type *fstype =
		    le2fstype(le, file_system_type_link);
		if (strcmp(fstype->name, name) == 0) {
			list_del(le);
			kfree((char *)fstype->name);
			kfree(fstype);
			ret = 0;
			break;
		}
	}

	unlock_file_system_type_list();
	return ret;
}

int do_mount(const char *devname, const char *fsname)
{
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
	return ret;
}

int do_umount(const char *devname)
{
	return vfs_unmount(devname);
}
