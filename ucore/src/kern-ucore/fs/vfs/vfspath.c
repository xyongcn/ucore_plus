#include <types.h>
#include <string.h>
#include <vfs.h>
#include <inode.h>
#include <iobuf.h>
#include <stat.h>
#include <proc.h>
#include <error.h>
#include <assert.h>

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
  if (vfs_get_curdir(&node) == 0) {
    if(path[0] != '/') {
      struct iobuf full_path_iob;
      iobuf_init(&full_path_iob, full_path_buffer, max_length, 0);
      vop_namefile(node, &full_path_iob);
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

/*
 * Set current directory, as a pathname. Use vfs_lookup to translate
 * it to a inode.
 */
int vfs_chdir(char *path)
{
  int ret;
  struct inode *node;
  //Firstly, get current working directory inode
  if (vfs_get_curdir(&node) == 0) {
    //Then get the full path of the working directory.
    static char full_path_buffer[1024];
    if(path[0] != '/') {
      struct iobuf full_path_iob;
      iobuf_init(&full_path_iob, full_path_buffer, 1024, 0);
      vop_namefile(node, &full_path_iob);
      strcat(full_path_buffer, path);
    }
    else {
      strcpy(full_path_buffer, path);
    }
    //TODO: Security issue: this may lead to buffer overflow.
    vfs_simplify_path(full_path_buffer);
    //kprintf("Full path:%s\r\n", full_path_buffer);
    //TODO: This hard-encoding needs to be removed, and consider vfs mount.
    if(memcmp(full_path_buffer, "/dev", 5) == 0 ||
    memcmp(full_path_buffer, "/dev/", 5) == 0) {
      if ((ret = vfs_get_root("dev", &node)) == 0) {
        ret = vfs_set_curdir(node);
        vop_ref_dec(node);
      }
      return ret;
    }
  }
  //Try to lookup from current inode
	if ((ret = vfs_lookup(path, &node)) == 0) {
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
	struct inode *node;
	if ((ret = vfs_get_curdir(&node)) != 0) {
		return ret;
	}
	/* The current dir must be a directory, and thus it is not a device. */
	assert(node->in_fs != NULL);

  //TODO: After disabling <devname>: syntax, need to consider mount point.
	/*const char *devname = vfs_get_devname(node->in_fs);
	if ((ret =
	     iobuf_move(iob, (char *)devname, strlen(devname), 1, NULL)) != 0) {
		goto out;
	}
	char colon = ':';
	if ((ret = iobuf_move(iob, &colon, sizeof(colon), 1, NULL)) != 0) {
		goto out;
	}*/
	ret = vop_namefile(node, iob);

out:
	vop_ref_dec(node);
	return ret;
}
