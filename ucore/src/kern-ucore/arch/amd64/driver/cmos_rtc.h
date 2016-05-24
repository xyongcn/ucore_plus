/*
 * CMOS RTC (Real Time Clock) Module. This module provides access to system
 * time provided by the motherboard.
 * Those code are created with the knowledge and sample codes from
 * http://wiki.osdev.org/CMOS and those algorithm we used are from the file
 * http://mirrors.neusoft.edu.cn/rpi-kernel/drivers/rtc/rtc-lib.c
 */

#include <types.h>

struct cmos_rtc_data {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;
};

void read_rtc(struct cmos_rtc_data* rtc_data);
