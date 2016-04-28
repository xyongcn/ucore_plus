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
int vfs_mount_find_record_by_mountpoint(char* mountpoint, struct vfs_mount_record** record_store);
struct fs* vfs_mount_get_record_fs(struct vfs_mount_record* record);

int vfs_mount_parse_full_path(
const char* full_path, char** path_part_store, struct fs** filesystem_store);
