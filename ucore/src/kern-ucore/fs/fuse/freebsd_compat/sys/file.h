#ifndef _FREEBSD_COMPAT_FILE_H_
#define	_FREEBSD_COMPAT_FILE_H_

#include <file.h>

static int fget(struct thread *td, int fd, cap_rights_t *rightsp, struct file **fpp) {
  int ret;
  ret = fd2file(fd, fpp);
  if(ret != 0) return ret;
  filemap_open(*fpp);
  return 0;
}

static int fdrop(struct file *fp, struct thread *td) {
  filemap_close(fp);
  return 0;
}
/*
#define fdrop(fp, td)                                                   \
       (refcount_release(&(fp)->f_count) ? _fdrop((fp), (td)) : _fnoop())

static __inline int _fnoop(void) {
  return 0;
}

static int _fdrop(struct file *fp, struct thread *td) {
  return 0;
}*/

#endif
