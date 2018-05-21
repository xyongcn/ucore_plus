#include <types.h>
#include <mmio.h>
#include <error.h>
#include <assert.h>
#include "thinpad_flashrom.h"

#define THINFLASH_CMD_GETCODE 0x90
#define THINFLASH_CMD_READ 0xFF
#define THINFLASH_CMD_WRITE 0x40
#define THINFLASH_CMD_ERASE 0xD0
#define THINFLASH_CMD_QUERY 0x70
#define THINFLASH_MODE_ERASE 0x20
#define THINFLASH_FINISH_BIT 0x80

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static uint16_t test_buf[THINFLASH_SECTOR_UNIT];

volatile uint16_t* THINFLASH_BASE = 0xBE000000;

int thinpad_flashrom_read(size_t secno, void *dst, size_t nsecs)
{
  nsecs = MIN(nsecs, THINFLASH_NR_SECTOR - secno);
  if(nsecs < 0) {
      return -E_INVAL;
  }
  volatile uint16_t *addr = THINFLASH_BASE + secno * THINFLASH_SECTOR_UNIT;
  mmio_write16(THINFLASH_BASE, THINFLASH_CMD_READ);
  int len = nsecs * THINFLASH_SECTOR_UNIT;
  uint16_t *dst_tmp = (uint16_t*)dst;
  for (int i = 0; i < len; ++i) {
    dst_tmp[i] = mmio_read16(addr + i);
  }
  return 0;
}

int thinpad_flashrom_write(size_t secno, const void *src, size_t nsecs)
{
  panic("FlashROM write limited.\n");
  nsecs = MIN(nsecs, THINFLASH_NR_SECTOR - secno);
  if (nsecs < 0) {
      return -E_INVAL;
  }
  uint16_t *src_tmp = (uint16_t*)src;
  int len = nsecs * THINFLASH_SECTOR_UNIT;
  int i;
  volatile uint16_t *addr = THINFLASH_BASE + secno * THINFLASH_SECTOR_UNIT;
  for (i = 0; i < len; ++i) {
    mmio_write16(addr, THINFLASH_CMD_WRITE);
    mmio_write16(addr + i, src_tmp[i]);
    do {
      mmio_write16(addr, THINFLASH_CMD_QUERY);
    } while (!(mmio_read16(addr) & THINFLASH_FINISH_BIT));
  }
  return 0;
}

int thinpad_flashrom_erase(size_t secno) {
  volatile uint16_t *addr = THINFLASH_BASE + secno * THINFLASH_SECTOR_UNIT;
  mmio_write16(addr, THINFLASH_MODE_ERASE);
  mmio_write16(addr, THINFLASH_CMD_ERASE);
  do {
    mmio_write16(addr, THINFLASH_CMD_QUERY);
  } while (!(mmio_read16(addr) & THINFLASH_FINISH_BIT));
  return 0;
}


void thinpad_flashrom_check(){
    //kprintf("%s(): inittf found, magic: 0x%08x, 0x%08x secs\n", __func__, *(uint32_t*)(dev->iobase), dev->size);
    int secno;
    for (secno = 0; secno < THINFLASH_NR_SECTOR; ++secno) {
        kprintf("erase %d ...\n", secno);
        if (thinpad_flashrom_erase(secno) != 0) {
            kprintf("in %s: %d secno fail to erase\n");
            assert(0);
        }
    }

    // check erase
    kprintf("check erase\n");
    for (secno = 0; secno < THINFLASH_NR_SECTOR; ++secno) {
        thinpad_flashrom_read(secno, test_buf, 1);
        int unit;
        for (unit = 0; unit < THINFLASH_SECTOR_UNIT; ++unit) {
            if (test_buf[unit] != 0xFFFF) {
                kprintf("secno %d unit %d: %d\n", secno, unit, test_buf[unit]);
                assert(0);
            }
        }
    }
    kprintf("OK\n");
    return;
    // check wirte & read
    kprintf("check write and read\n");
    for (secno = 0; secno < THINFLASH_NR_SECTOR; ++secno) {
      kprintf("Checking sector %d\n", secno);
        int unit;
        for (unit = 0; unit < THINFLASH_SECTOR_UNIT; ++unit) {
            test_buf[unit] = ((secno & 0xFF) << 8) | (unit & 0xFF) ;
        }
        thinpad_flashrom_write(secno, test_buf, 1);
        thinpad_flashrom_read(secno, test_buf, 1);
        for (unit = 0; unit < THINFLASH_SECTOR_UNIT; ++unit) {
            // kprintf("secno %d unit %d: %x\n", secno, unit, test_buf[unit]);
            if (test_buf[unit] != (((secno & 0xFF) << 8) | (unit & 0xFF))) {
                kprintf("Sector %d %x!=%x Test failed!\n", secno,
                test_buf[unit], ((secno & 0xFF) << 8) | (unit & 0xFF));
                break;
            }
        }
    }
    kprintf("OK\n");
}
