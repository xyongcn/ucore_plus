/*
 * VFS operations relating to pathname translation
 */

#include <types.h>
#include <string.h>
#include <vfs.h>
#include <inode.h>
#include <error.h>
#include <assert.h>

/*
 * Common code to pull the device name, if any, off the front of a
 * path and choose the inode to begin the name lookup relative to.
 */
static int get_device(char *path, char **subpath, struct inode **node_store)
{
  int ret;

  //Get the inode and fs of current directory.
  struct inode* current_dir_node;
  ret = vfs_get_curdir(&current_dir_node);
  if(ret != 0) return ret;
  struct fs* current_dir_fs = vop_fs(current_dir_node);

  //Get the path and fs we are changing into.
  //TODO: This may lead to incorrect behavious when call get_device twice.
  static char full_path[1024];
  vfs_expand_path(path, full_path, 1024);
  char* new_path;
  struct fs* new_path_fs;
  vfs_mount_parse_full_path(full_path, &new_path, &new_path_fs);

  //If we are actually changing fs
  if(new_path_fs != current_dir_fs) {
    //Then anyway we need to lookup from the root of the new fs.
    (*node_store) = fsop_get_root(new_path_fs);
    if(*new_path == '\0') {
      (*subpath) = ".";
    }
    else {
      (*subpath) = new_path;
    }
    return 0;
  }
  //Otherwise, check whether path is absolute or relative
  else {
    if(path[0] == '/') {
      //For absolute path, look up from root inode.
      (*node_store) = fsop_get_root(current_dir_fs);
      (*subpath) = path + 1;
    }
    else {
      //Otherwise, lookup from current directory
      (*node_store) = current_dir_node;
      (*subpath) = path;
    }
    return 0;
  }
}

/*
 * Name-to-inode translation.
 * (In BSD, both of these are subsumed by namei().)
 */

int vfs_lookup(char *path, struct inode **node_store)
{
	int ret;
	struct inode *node;
	if ((ret = get_device(path, &path, &node)) != 0) {
		return ret;
	}
	if (*path != '\0') {
		ret = vop_lookup(node, path, node_store);
		vop_ref_dec(node);
		return ret;
	}
	*node_store = node;
	return 0;
}

int vfs_lookup_parent(char *path, struct inode **node_store, char **endp)
{
	int ret;
	struct inode *node;
	if ((ret = get_device(path, &path, &node)) != 0) {
		return ret;
	}
	ret =
	    (*path != '\0') ? vop_lookup_parent(node, path, node_store,
						endp) : -E_INVAL;
	vop_ref_dec(node);
	return ret;
}
