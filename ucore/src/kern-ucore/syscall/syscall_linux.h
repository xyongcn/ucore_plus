#include <types.h>

machine_word_t syscall_linux_read(machine_word_t args[]);
machine_word_t syscall_linux_write(machine_word_t args[]);
machine_word_t syscall_linux_open(machine_word_t args[]);
machine_word_t syscall_linux_close(machine_word_t args[]);
machine_word_t syscall_linux_stat(machine_word_t args[]);
machine_word_t syscall_linux_lstat(machine_word_t args[]);
machine_word_t syscall_linux_fstat(machine_word_t args[]);

machine_word_t syscall_linux_mount(machine_word_t args[]);
machine_word_t syscall_linux_umount(machine_word_t args[]);

#ifndef __UCORE_64__
machine_word_t syscall_linux_stat64(machine_word_t args[]);
machine_word_t syscall_linux_lstat64(machine_word_t args[]);
machine_word_t syscall_linux_fstat64(machine_word_t args[]);
#endif
