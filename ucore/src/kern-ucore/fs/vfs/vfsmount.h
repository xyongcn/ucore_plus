#include <list.h>

struct fs;

struct vfs_mount_record {
  const char* mountpoint;
  struct fs* filesystem;
  list_entry_t list_entry;
};

extern list_entry_t vfs_mount_record_list;

int vfs_mount_add_record(const char* mountpoint, struct fs* filesystem);
int vfs_mount_find_record_by_fs(struct fs* filesystem, struct vfs_mount_record** record_store);

int vfs_mount_parse_full_path(
const char* full_path, char** path_part_store, struct fs** filesystem_store);
