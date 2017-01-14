#include <ulib.h>
#include <stdio.h>
#include <mount.h>
#include <string.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)

void usage(void)
{
	printf("usage: mount <device_file> <mountpoint> <filesystem>\n");
}

int main(int argc, char **argv)
{
  if(argc < 4)
  {
    usage();
    return -1;
  }

	int ret = mount(argv[1], argv[2], argv[3], 0, NULL);
	if (ret == 0) {
		printf("mounted file system %s on %s.\n", argv[2], argv[3]);
	} else {
		printf("mount failed.\n");
	}
	return ret;
}
