#ifndef __THIN_FLASH__H
#define __THIN_FLASH__H

#include <types.h>

//About the thinpad FlashROM
//Capacity is 8MBytes = 64Sectors * 128KBytes/Sector.
//Each time
#define THINFLASH_SIZE        (8 << 20)
#define THINFLASH_SECTOR_SIZE (128 << 10)
#define THINFLASH_NR_SECTOR   64
#define THINFLASH_UNIT_SIZE   2
#define THINFLASH_SECTOR_UNIT (THINFLASH_SECTOR_SIZE / THINFLASH_UNIT_SIZE)

bool check_inittf();

int thinpad_flashrom_read(size_t secno, void *dst, size_t nsecs);
int thinpad_flashrom_write(size_t secno, const void *src, size_t nsecs);
int thinpad_flashrom_erase(size_t secno);
void thinpad_flashrom_check();

#endif /*__THIN_FLASH__H*/
