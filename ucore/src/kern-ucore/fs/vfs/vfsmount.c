#include <mm/slab.h>
#include <error.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <vfsmount.h>

list_entry_t vfs_mount_record_list;

void vfs_mount_init()
{
  list_init(&vfs_mount_record_list);
}

int vfs_mount_add_record(const char* mountpoint, struct fs* filesystem)
{
  struct vfs_mount_record* record = kmalloc(sizeof(struct vfs_mount_record));
  record->mountpoint = strdup(mountpoint);
  record->filesystem = filesystem;
  list_add(&vfs_mount_record_list, &record->list_entry);
  return 0;
}

int vfs_mount_remove_record(struct vfs_mount_record* record)
{
  list_del(&record->list_entry);
  kfree(record);
  return 0;
}

struct fs* vfs_mount_get_record_fs(struct vfs_mount_record* record)
{
  return record->filesystem;
}

int vfs_mount_find_record_by_fs(struct fs* filesystem, struct vfs_mount_record** record_store)
{
  for(list_entry_t* i = list_next(&vfs_mount_record_list);
  i != &vfs_mount_record_list; i = list_next(i)) {
    struct vfs_mount_record* record = container_of(i, struct vfs_mount_record, list_entry);
    if(record->filesystem == filesystem) {
      (*record_store) = record;
      return 0;
    }
  }
  (*record_store) = NULL;
  return -E_INVAL;
}

int vfs_mount_find_record_by_mountpoint(char* mountpoint, struct vfs_mount_record** record_store)
{
  for(list_entry_t* i = list_next(&vfs_mount_record_list);
  i != &vfs_mount_record_list; i = list_next(i)) {
    struct vfs_mount_record* record = container_of(i, struct vfs_mount_record, list_entry);
    if(strcmp(record->mountpoint, mountpoint) == 0) {
      (*record_store) = record;
      return 0;
    }
  }
  (*record_store) = NULL;
  return -E_INVAL;
}

int vfs_mount_parse_full_path(
const char* full_path, char** path_part_store, struct fs** filesystem_store)
{
  int best_match_length = -1;
  struct vfs_mount_record* best_match_record;
  for(list_entry_t* i = list_next(&vfs_mount_record_list);
  i != &vfs_mount_record_list; i = list_next(i)) {
    //TODO: String comparsion operations here can be optimized.
    struct vfs_mount_record* record = container_of(i, struct vfs_mount_record, list_entry);
    if(strcmp(full_path, record->mountpoint) == 0) {
      best_match_length = strlen(full_path);
      best_match_record = record;
      break;
    }
    else {
      int mountpoint_length = strlen(record->mountpoint);
      if(memcmp(full_path, record->mountpoint, mountpoint_length) == 0 &&
      (full_path[mountpoint_length] == '/' ||  mountpoint_length == 1)) {
        int match_length = mountpoint_length == 1 ? 1 : mountpoint_length + 1;
        if(match_length > best_match_length) {
          best_match_length = match_length;
          best_match_record = record;
        }
      }
    }
  }
  if(best_match_length == -1)
  {
    panic("vfsmount: Cannot determine mount point for path %s.\r\n", full_path);
    return -E_INVAL;
  }
  (*path_part_store) = full_path + best_match_length;
  (*filesystem_store) = best_match_record->filesystem;
}

void vfs_mount_test()
{
  struct fs* rootFS = (struct fs*)0;
  struct fs* homeFS = (struct fs*)1;
  struct fs* user1FS = (struct fs*)2;
  struct fs* user2FS = (struct fs*)3;
  struct fs* devFS = (struct fs*)4;
  vfs_mount_add_record("/", rootFS);
  vfs_mount_add_record("/home", homeFS);
  vfs_mount_add_record("/home/user1", user1FS);
  vfs_mount_add_record("/home/user2", user2FS);
  vfs_mount_add_record("/dev", devFS);
  char* path;
  char* part;
  struct fs* fs;
  path = "/aaa";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/home";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/home/user1";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/home/user2";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/home/user3";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/home/user1/doc";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/home/user2/doc";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/home/user3/doc";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/dev";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/dev/stdin";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
  path = "/devhh";
  vfs_mount_parse_full_path(path, &part, &fs);
  kprintf("%s=>%s %d\r\n", path, part, fs);
}
