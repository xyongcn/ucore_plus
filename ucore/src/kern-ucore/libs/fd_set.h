#ifndef __LIBS_FD_SET_H__
#define __LIBS_FD_SET_H__

#include <types.h>

#define LINUX_FD_SET_SIZE 1024

typedef struct {
  unsigned long bits[LINUX_FD_SET_SIZE / (8 * sizeof(unsigned long))];
} linux_fd_set_t;

typedef linux_fd_set_t fd_set_t;

bool linux_fd_set_is_set(linux_fd_set_t *fd_set, uint32_t fd);
void linux_fd_set_set(linux_fd_set_t *fd_set, uint32_t fd);
void linux_fd_set_unset(linux_fd_set_t *fd_set, uint32_t fd);
void linux_fd_set_or(linux_fd_set_t *fd_set, linux_fd_set_t *fd_set_other);

#endif
