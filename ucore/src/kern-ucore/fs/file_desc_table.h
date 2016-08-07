/*
 * This file provides management functions on file descriptions of a user-space
 * process (or maybe many process for things like "CLONE_FS" exists).
 *
 * One valid file description can links to one and only one struct file, i.e.
 * An "Open File Description" in kernel space. Different file description
 * (from the same or different process) can points to the same struct file.
 * (via system calls like dup2, fork, etc.)
 */

#ifndef __KERN_FS_FILE_DESC_TABLE_H__
#define __KERN_FS_FILE_DESC_TABLE_H__

#include <types.h>

struct file;

struct file_desc_entry {
  struct file *opened_file;
};

struct file_desc_table {
  struct file_desc_entry *entries;
  int capacity;
  int limit;
  int last_free_desc;
};

int file_desc_table_init(struct file_desc_table *table, int limit);
int file_desc_table_uninit(struct file_desc_table *table);
int file_desc_table_copy(struct file_desc_table *to, struct file_desc_table *from);
struct file* file_desc_table_get_file(struct file_desc_table *table, int file_desc);
int file_desc_table_get_unused(struct file_desc_table *table);
int file_desc_table_associate(struct file_desc_table *table, int file_desc, struct file* file);
int file_desc_table_dissociate(struct file_desc_table *table, int file_desc);

#endif /* __KERN_FS_FILE_DESC_TABLE__ */
