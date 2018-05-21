#include <ulib.h>
#include <file.h>
#include <stat.h>
#include <unistd.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)
int string_to_number(char *s)
{
	int length = strlen(s);
	int ret = 0;
	for (int i = 0; i < length; i++) {
		ret = ret * 10 + (s[i] - '0');
	}
	return ret;
}

#define BUFSIZE                         4096

int cat(int fdsrc, int fddst)
{
	static char buffer[BUFSIZE];
	int ret1, ret2;
	while ((ret1 = read(fdsrc, buffer, sizeof(buffer))) > 0) {
		if ((ret2 = write(fddst, buffer, ret1)) != ret1) {
			return ret2;
		}
	}
	return ret1;
}

uint16_t change_byte_order_16(uint16_t val)
{
  char temp;
  char* ret = (char*)&val;
  temp = ret[0];
  ret[0] = ret[1];
  ret[1] = temp;
  return val;
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    printf("Usage: program_fpga bitstream full|partial");
    return 1;
  }
  char op = 0;
  if(strcmp(argv[2], "full") == 0) {
    op = 0;
  }
  else if(strcmp(argv[2], "partial") == 0) {
    op = 1;
  }
  else {
    printf("Usage: program_fpga bitstream full|partial");
    return 1;
  }
    uint16_t length;
    uint8_t designId;
    char *designFileName;
    int fdsrc = open(argv[1], O_RDONLY);
    read(fdsrc, &length, sizeof(length));
    length = change_byte_order_16(length);
    seek(fdsrc, length, LSEEK_CUR);

    // Read desgin name
    read(fdsrc, &length, sizeof(length));
    length = change_byte_order_16(length);
    read(fdsrc, &designId, sizeof(designId));
    if(length != 1 || designId != 0x61) {
      printf("ERROR! Design Name ID is not 0x61!\n");
      exit(1);
    }
    read(fdsrc, &length, sizeof(length));
    length = change_byte_order_16(length);
    designFileName = malloc(length + 1);
    read(fdsrc, designFileName, length);
    designFileName[length] = '\0';
    printf("Design Name: %s\n", designFileName);
    free(designFileName);

    // Read part name
    read(fdsrc, &designId, sizeof(designId));
    if(designId != 0x62) {
      printf("ERROR! Design Part ID is not 0x62!\n");
      exit(1);
    }
    read(fdsrc, &length, sizeof(length));
    length = change_byte_order_16(length);
    designFileName = malloc(length + 1);
    read(fdsrc, designFileName, length);
    designFileName[length] = '\0';
    printf("Part Name: %s\n", designFileName);
    free(designFileName);

    for (int i = 0; i < 2; i++) {
        read(fdsrc, &designId, sizeof(designId));
        read(fdsrc, &length, sizeof(length));
        length = change_byte_order_16(length);
        seek(fdsrc, length, LSEEK_CUR);
    }
    seek(fdsrc, 5, LSEEK_CUR);
    int fddst = open("/dev/zynq_programmable_logic", O_RDWR);
    cat(fdsrc, fddst);
    close(fdsrc);
    close(fddst);
    fddst = open("/dev/zynq_programmable_logic_ctl", O_RDWR);
    write(fddst, &op, 1);
    close(fddst);
	return 0;
}
