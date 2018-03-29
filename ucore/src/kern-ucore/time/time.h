/*
 * Time module, used to create time related functions for uCore.
 * Currently this module mainly performs conversion between cmos_rtc_data
 * and timee_t (i.e. evaluates seconds since 1970-01-01 00:00:00)
 */

#ifndef __KERN_TIME_TIME_H__
#define __KERN_TIME_TIME_H__

#include <types.h>

typedef long __time_t;
typedef __time_t time_t;

time_t time_get_current();

#endif
