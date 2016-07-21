#include <types.h>
#include "fd_set.h"

bool linux_fd_set_is_set(linux_fd_set_t *fd_set, uint32_t fd)
{
  unsigned long long_type_bits = sizeof(unsigned long) * 8;
  unsigned long index = fd / long_type_bits;
  unsigned long bit = fd % long_type_bits;
  return (fd_set->bits[index] & (1UL << bit)) != 0;
}

void linux_fd_set_set(linux_fd_set_t *fd_set, uint32_t fd)
{
  unsigned long long_type_bits = sizeof(unsigned long) * 8;
  unsigned long index = fd / long_type_bits;
  unsigned long bit = fd % long_type_bits;
  fd_set->bits[index] |= (1UL << bit);
}

void linux_fd_set_unset(linux_fd_set_t *fd_set, uint32_t fd)
{
  unsigned long long_type_bits = sizeof(unsigned long) * 8;
  unsigned long index = fd / long_type_bits;
  unsigned long bit = fd % long_type_bits;
  fd_set->bits[index] &= ~(1UL << bit);
}
