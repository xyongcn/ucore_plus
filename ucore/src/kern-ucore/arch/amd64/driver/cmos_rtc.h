/*
 * CMOS RTC (Real Time Clock) Module. This module provides access to system
 * time provided by the motherboard.
 * Those code are created with the knowledge and sample codes from this page
 * http://wiki.osdev.org/CMOS in the osdev wiki.
 */

#ifndef __KERN_DRIVER_CMOS_RTC_H__
#define __KERN_DRIVER_CMOS_RTC_H__

#include <types.h>

struct cmos_rtc_data {
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  uint8_t day;
  uint8_t month;
  uint16_t year;
};

void cmos_rtc_read(struct cmos_rtc_data* rtc_data);

#endif
