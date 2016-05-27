/*
 * CMOS RTC (Real Time Clock) Module. This module provides access to system
 * time provided by the motherboard.
 * Those code are created with the knowledge and sample codes from this page
 * http://wiki.osdev.org/CMOS in the osdev wiki.
 */

#include <types.h>
#include <arch.h>
#include <string.h>

#include "cmos_rtc.h"

unsigned char second;
unsigned char minute;
unsigned char hour;
unsigned char day;
unsigned char month;
unsigned int year;

const static int CMOS_ADDRESS_PORT = 0x70;
const static int CMOS_DATA_PORT = 0x71;

static bool get_update_in_progress_flag() {
      outb(CMOS_ADDRESS_PORT, 0x0A);
      return (inb(CMOS_DATA_PORT) & 0x80);
}

static uint8_t bcd2binary(uint8_t value) {
  return (value & 0x0F) + (value / 16) * 10;
}

static uint8_t get_rtc_register(int reg) {
      outb(CMOS_ADDRESS_PORT, reg);
      return inb(CMOS_DATA_PORT);
}

static void read_rtc_direct(struct cmos_rtc_data* rtc_data) {
  rtc_data->second = get_rtc_register(0x00);
  rtc_data->minute = get_rtc_register(0x02);
  rtc_data->hour = get_rtc_register(0x04);
  rtc_data->day = get_rtc_register(0x07);
  rtc_data->year = get_rtc_register(0x09);
  rtc_data->month = get_rtc_register(0x08);
}

void cmos_rtc_read(struct cmos_rtc_data* rtc_data) {
  //TODO: Use ACPI table to get century register.
  while(get_update_in_progress_flag());
  read_rtc_direct(rtc_data);
  //TODO: Read century register.
  struct cmos_rtc_data _last_rtc_data;
  struct cmos_rtc_data *last_rtc_data = &_last_rtc_data;
  do {
    memcpy(last_rtc_data, rtc_data, sizeof(struct cmos_rtc_data));
    while(get_update_in_progress_flag());
    read_rtc_direct(rtc_data);
    //TODO: Read century register.
  }
  while(memcmp(last_rtc_data, rtc_data, sizeof(struct cmos_rtc_data)) != 0);

  uint8_t registerB = get_rtc_register(0x0B);
  if (!(registerB & 0x04)) {
    rtc_data->second = bcd2binary(rtc_data->second);
    rtc_data->minute = bcd2binary(rtc_data->minute);
    rtc_data->hour = ((rtc_data->hour & 0x0F) + (((rtc_data->hour & 0x70) / 16) * 10))
      | (rtc_data->hour & 0x80);
    rtc_data->day = bcd2binary(rtc_data->day);
    rtc_data->month = bcd2binary(rtc_data->month);
    rtc_data->year = bcd2binary(rtc_data->year);
  }

  // Convert 12 hour clock to 24 hour clock if necessary
  if (!(registerB & 0x02) && (rtc_data->hour & 0x80)) {
    rtc_data->hour = ((rtc_data->hour & 0x7F) + 12) % 24;
  }

  //TODO: Check century register and evaluate year.
  rtc_data->year = rtc_data->year + 2000;
}
