#include <error.h>
#include <assert.h>
#include <slab.h>
#include "file.h"
#include "file_desc_table.h"

const int FILE_DESC_TABLE_INIT_CAPACITY = 32;

int file_desc_table_init(struct file_desc_table *table, int limit)
{
  assert(limit > 0);
  assert(table != NULL);
  struct file_desc_entry *entries = kmalloc(sizeof(struct file_desc_entry) * limit);
  if(entries == NULL) {
    if(entries != NULL) kfree(entries);
    return -E_NOMEM;
  }
  for(int i = 0; i < limit; i++) {
    entries[i].opened_file = NULL;
  }
  table->entries = entries;
  //TODO: Initialize with a smaller size and implement auto expand.
  table->capacity = limit;
  table->limit = limit;
  table->last_free_desc = 0;
  return 0;
}

int file_desc_table_uninit(struct file_desc_table *table)
{
  for(int i = 0; i < table->capacity; i++) {
    if(table->entries[i].opened_file != NULL) {
      file_desc_table_dissociate(table, i);
    }
  }
  kfree(table->entries);
  return 0;
}

int file_desc_table_copy(struct file_desc_table *to, struct file_desc_table *from)
{
  int capacity = to->capacity > from->capacity ? from->capacity : to->capacity;
  for(int i = 0; i < capacity; i++) {
    struct file *file = file_desc_table_get_file(from, i);
    if(file != NULL) {
      file_desc_table_associate(to, i, file);
    }
  }
  return 0;
}

static struct file* file_desc_table_expand(struct file_desc_table *table)
{
  int old_capacity = table->capacity;
  int new_capacity = old_capacity * 2;
  if(new_capacity > table->limit) {
    new_capacity = table->limit;
  }
  //TODO: reallocate and copy old fds.
}

struct file* file_desc_table_get_file(struct file_desc_table *table, int file_desc)
{
  if(file_desc >= table->capacity) return NULL;
  return table->entries[file_desc].opened_file;
}

int file_desc_table_get_unused(struct file_desc_table *table)
{
  if(table->entries[table->last_free_desc].opened_file == NULL)
    return table->last_free_desc;
  int search_begin = table->last_free_desc + 1;
  int search_end = table->last_free_desc;
  if(search_begin >= table->capacity) search_begin = 0;
  for(int i = search_begin; i != search_end;) {
    if(table->entries[i].opened_file == NULL) {
      table->last_free_desc = i;
      return i;
    }
    i++;
    if(i == table->capacity) i = 0;
  }
  return -1;
}

int file_desc_table_associate(struct file_desc_table *table, int file_desc, struct file* file)
{
  assert(file_desc >= 0 && file_desc < table->capacity);
  assert(table->entries[file_desc].opened_file == NULL);
  fopen_count_inc(file);
  table->entries[file_desc].opened_file = file;
  return 0;
}

int file_desc_table_dissociate(struct file_desc_table *table, int file_desc)
{
  assert(file_desc >= 0 && file_desc < table->capacity);
  struct file *file = table->entries[file_desc].opened_file;
  assert(file != NULL);
  fopen_count_dec(file);
  table->entries[file_desc].opened_file = NULL;
  return 0;
}
