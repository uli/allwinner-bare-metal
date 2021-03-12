// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "rtc.h"
#include <stdint.h>

#define RTC_REG(n) (*(volatile uint32_t *)(0x01f00000 + (n)))

#define RTC_LOSC_CTRL_REG RTC_REG(0x00)
#define RTC_YY_MM_DD_REG  RTC_REG(0x10)
#define RTC_HH_MM_SS_REG  RTC_REG(0x14)

static void wait_for_time_write(void)
{
    while (RTC_LOSC_CTRL_REG & (1 << 8)) {
        // XXX: timeout
    }
}

static void wait_for_date_write(void)
{
    while (RTC_LOSC_CTRL_REG & (1 << 7)) {
        // XXX: timeout
    }
}

void rtc_set_date(int day, int month, int year)
{
    RTC_YY_MM_DD_REG =
        (RTC_YY_MM_DD_REG & (1 << 22)) | 	// preserve leap year
        // Y2.064K, anyone?
        (((year - 2000) & 0x3f) << 16) |
        ((month 	& 0x0f) << 8) |
        ((day		& 0x1f) << 0);
    // XXX: leap year?
    wait_for_date_write();
}

void rtc_set_weekday(int wday)
{
    RTC_HH_MM_SS_REG =
        (RTC_HH_MM_SS_REG & 0x1fffffff) |	// preserve time
        ((wday & 0x7) << 29);
    wait_for_time_write();
}

void rtc_set_time(int hour, int minute, int second)
{
    RTC_HH_MM_SS_REG =
        (RTC_HH_MM_SS_REG & (7 << 29))	|	// preserve weekday
        ((hour   & 0x1f) << 16) |
        ((minute & 0x3f) << 8) |
        ((second & 0x3f) << 0);
    wait_for_time_write();
}

int rtc_get_year(void) {
    return ((RTC_YY_MM_DD_REG >> 16) & 0x3f) + 2000;
}

int rtc_get_month(void) {
    return (RTC_YY_MM_DD_REG >> 8) & 0x0f;
}

int rtc_get_day(void) {
    return (RTC_YY_MM_DD_REG >> 0) & 0x1f;
}

int rtc_get_weekday(void) {
    return RTC_HH_MM_SS_REG >> 29;
}

int rtc_get_hour(void) {
    return (RTC_HH_MM_SS_REG >> 16) & 0x1f;
}

int rtc_get_minute(void) {
    return (RTC_HH_MM_SS_REG >> 8) & 0x3f;
}

int rtc_get_second(void) {
    return (RTC_HH_MM_SS_REG >> 0) & 0x3f;
}
