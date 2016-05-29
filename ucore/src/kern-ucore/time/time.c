/*
 * Time module, used to create time related functions for uCore.
 * Currently this module mainly performs conversion between cmos_rtc_data
 * and timee_t (i.e. evaluates seconds since 1970-01-01 00:00:00)
 * Currently only amd64 architecture is supported.
 */

#if ARCH_AMD64

#include <driver/cmos_rtc.h>
#include <assert.h>
#include "time.h"

static bool is_leap_year(int year);
static bool leap_year_from_ce_to(int year);
const static int DAYS_OF_MONTH[12] = {
  31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

const static int EPOCH_YEAR = 1970;

static bool is_leap_year(int year) {
  return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static bool leap_year_from_ce_to(int year) {
  return year / 4 - year / 100 + year / 400;
}

time_t time_get_current() {
  int ret = 0;
  //TODO: Only handles date after 1970, but seems this program won't run on
  //any time before 1970-01-01...
  //TODO: This algorithm is not fully checked for all special cases. It may
  //contains bugs.
  struct cmos_rtc_data rtc_data;
  cmos_rtc_read(&rtc_data);
  int years_since_epoch = rtc_data.year - EPOCH_YEAR;
  ret += years_since_epoch * 365 * 24 * 3600;
  int leap_years_since_epoch =
    leap_year_from_ce_to(rtc_data.year - 1) - leap_year_from_ce_to(EPOCH_YEAR - 1);
  ret += leap_years_since_epoch * 24 * 3600;
  for(int i = 0; i < rtc_data.month - 1; i++) {
    ret += DAYS_OF_MONTH[i] * 24 * 3600;
  }
  if(rtc_data.month > 2 && is_leap_year(rtc_data.year)) {
    ret += 24 * 3600;
  }
  ret += (rtc_data.day - 1) * 24 * 3600;
  ret += rtc_data.hour * 3600;
  ret += rtc_data.minute * 60;
  ret += rtc_data.second;
  //panic("%d", ret);
  return ret;
}

#endif // ARCH_AMD64
