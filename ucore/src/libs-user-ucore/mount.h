#ifndef __USER_LIBS_MOUNT_H__
#define __USER_LIBS_MOUNT_H__

int mount(const char *source, const char *target, const char *filesystemtype,
	  unsigned long flags, const void *data);
int umount(const char *target);

#endif
