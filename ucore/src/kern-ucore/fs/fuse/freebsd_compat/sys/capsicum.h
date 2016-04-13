#ifndef	_FREEBSD_COMPAT_CAPSICUM_H_
#define	_FREEBSD_COMPAT_CAPSICUM_H_

#define CAPRIGHT(idx, bit)      ((1ULL << (57 + (idx))) | (bit))

/*
 * Possible rights on capabilities.
 *
 * Notes:
 * Some system calls don't require a capability in order to perform an
 * operation on an fd.  These include: close, dup, dup2.
 *
 * sendfile is authorized using CAP_READ on the file and CAP_WRITE on the
 * socket.
 *
 * mmap() and aio*() system calls will need special attention as they may
 * involve reads or writes depending a great deal on context.
 */
/* INDEX 0 */
/*
 * General file I/O.
 */
/* Allows for openat(O_RDONLY), read(2), readv(2). */
#define CAP_READ                CAPRIGHT(0, 0x0000000000000001ULL)
/* Allows for openat(O_WRONLY | O_APPEND), write(2), writev(2). */
#define CAP_WRITE               CAPRIGHT(0, 0x0000000000000002ULL)
/* Allows for lseek(fd, 0, SEEK_CUR). */
#define CAP_SEEK_TELL           CAPRIGHT(0, 0x0000000000000004ULL)
/* Allows for lseek(2). */
#define CAP_SEEK                (CAP_SEEK_TELL | 0x0000000000000008ULL)
/* Allows for aio_read(2), pread(2), preadv(2). */
#define CAP_PREAD               (CAP_SEEK | CAP_READ)

#define cap_rights_init(...)                                            \
        __cap_rights_init(CAP_RIGHTS_VERSION, __VA_ARGS__, 0ULL)
static cap_rights_t *__cap_rights_init(int version, cap_rights_t *rights, ...) {
  return NULL;
}

#endif
