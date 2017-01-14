#include <types.h>
#include <string.h>
#include <vfs.h>
#include <inode.h>
#include <iobuf.h>
#include <stat.h>
#include <proc.h>
#include <error.h>
#include <assert.h>

#include "vfsmount.h"

void vfs_simplify_path(char* path) {
  int write_pos = 0;
  int last_slash_pos = 0;
  int current_pos = 1;
  for(;;) {
    char c = path[current_pos];
    if(c == '/' || c == '\0') {
      if(current_pos - last_slash_pos == 1) {
      }
      else if(current_pos - last_slash_pos == 2 &&
      path[last_slash_pos + 1] == '.') {
      }
      else if(current_pos - last_slash_pos == 3 &&
      path[last_slash_pos + 1] == '.' &&
      path[last_slash_pos + 2] == '.') {
        if(write_pos != 0) {
          do {
              write_pos--;
          }
          while(path[write_pos] != '/');
        }
      }
      else {
        if(write_pos < last_slash_pos) {
          write_pos++;
          for(int i = last_slash_pos + 1; i <= current_pos; i++, write_pos++) {
             path[write_pos] = path[i];
          }
          write_pos--;
        }
        else write_pos = current_pos;
      }
      last_slash_pos = current_pos;
    }
    current_pos++;
    if(c == '\0')break;
  }
  if(write_pos > 0) path[write_pos] = '\0';
  else path[1] = '\0';
}

void vfs_expand_path(const char* path, char* full_path_buffer, int max_length)
{
  //TODO: Security issue: this may lead to buffer overflow.
  struct inode *node;
  //Firstly, get current working directory inode
  full_path_buffer[0] = '\0';
  if (vfs_get_curdir(&node) == 0) {
    if(path[0] != '/') {
      struct iobuf full_path_iob;
      iobuf_init(&full_path_iob, full_path_buffer, max_length, 0);
      vfs_getcwd(&full_path_iob);
      strcat(full_path_buffer, "/");
      strcat(full_path_buffer, path);
    }
    else {
      strcpy(full_path_buffer, path);
    }
  }
}

static struct inode *get_cwd_nolock(void)
{
	return current->fs_struct->pwd;
}

static void set_cwd_nolock(struct inode *pwd)
{
	current->fs_struct->pwd = pwd;
}

static void lock_cfs(void)
{
	lock_fs(current->fs_struct);
}

static void unlock_cfs(void)
{
	unlock_fs(current->fs_struct);
}

/*
 * Get current directory as a inode.
 *
 * We do not synchronize current->fs_struct->pwd, because it belongs exclusively
 * to its own process(or threads) with the holding lock.
 */
int vfs_get_curdir(struct inode **dir_store)
{
	struct inode *node;
	if ((node = get_cwd_nolock()) != NULL) {
		vop_ref_inc(node);
		*dir_store = node;
		return 0;
	}
	return -E_NOENT;
}

/*
 * Set current directory as a vnode.
 * The passed inode must in fact be a directory.
 */
int vfs_set_curdir(struct inode *dir)
{
	int ret = 0;
	lock_cfs();
	struct inode *old_dir;
	if ((old_dir = get_cwd_nolock()) != dir) {
		if (dir != NULL) {
			uint32_t type;
			if ((ret = vop_gettype(dir, &type)) != 0) {
				goto out;
			}
			if (!S_ISDIR(type)) {
				ret = -E_NOTDIR;
				goto out;
			}
			vop_ref_inc(dir);
		}
		set_cwd_nolock(dir);
		if (old_dir != NULL) {
			vop_ref_dec(old_dir);
		}
	}
out:
	unlock_cfs();
	return ret;
}

//This function is used to initialize cwd for a proc.
int vfs_path_init_cwd(char* mountpoint)
{
  //First set root to "/", after this, vfs_chdir can be used.
  assert(mountpoint[0] == '/');
  vfs_simplify_path(mountpoint);
  struct mount_record* root_mount_record;
  struct inode* root_inode;
  vfs_mount_find_record_by_mountpoint(mountpoint, &root_mount_record);
  if(root_mount_record == NULL) {
    panic("%s is not a mount point!", mountpoint);
    return -E_INVAL;
  }
  root_inode = fsop_get_root(vfs_mount_get_record_fs(root_mount_record));
  return vfs_set_curdir(root_inode);
}

/*
 * Set current directory, as a pathname. Use vfs_lookup to translate
 * it to a inode.
 */
int vfs_chdir(char *path)
{
  int ret;
  struct inode *node;
  //Try to lookup from current inode
	if ((ret = vfs_lookup(path, &node)) == 0) {
    //If found, change directory.
		ret = vfs_set_curdir(node);
		vop_ref_dec(node);
	}
	return ret;
}

/*
 * Get current directory, as a pathname.
 * Use vop_namefile to get the pathname.
 */
int vfs_getcwd(struct iobuf *iob)
{
	int ret;
	struct inode *current_dir_node;
  struct fs *current_dir_fs;

  //Get the inode and fs of current working directory.
  ret = vfs_get_curdir(&current_dir_node);
  if(ret != 0) return ret;
  current_dir_fs = vop_fs(current_dir_node);
	assert(current_dir_fs != NULL);

  //TODO: Security issue: this may lead to buffer overflow.
  static char full_path[1024];
  static char partial_path[1024];

  //Get the current mountpoint
  struct vfs_mount_record* mount_record;
  vfs_mount_find_record_by_fs(current_dir_fs, &mount_record);
  assert(mount_record != NULL);
  strcpy(full_path, mount_record->mountpoint);

  //Get the file path relative to the mount point.
  struct iobuf partial_path_iob;
  iobuf_init(&partial_path_iob, partial_path, 1024, 0);
  ret = vop_namefile(current_dir_node, &partial_path_iob);
  if(ret != 0) goto out;

  //Joint them together and simplify
  //TODO: string operations here can be optimized.
  strcat(full_path, "/");
  strcat(full_path, partial_path);
  vfs_simplify_path(full_path);
  iobuf_move(iob, full_path, strlen(full_path) + 1, 1, NULL);

out:
	vop_ref_dec(current_dir_node);
	return ret;
}
