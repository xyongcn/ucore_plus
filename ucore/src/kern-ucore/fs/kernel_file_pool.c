#include <slab.h>
#include <file.h>
#include "kernel_file_pool.h"

void kernel_file_pool_init()
{

}

struct file* kernel_file_pool_allocate()
{
  return kmalloc(sizeof(struct file));
}

void kernel_file_pool_free(struct file* file)
{
  kfree(file);
}
