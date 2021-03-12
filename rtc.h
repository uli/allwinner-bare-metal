// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

void rtc_set_date(int day, int month, int year);
void rtc_set_time(int hour, int minute, int second);
void rtc_set_weekday(int wday);

int rtc_get_year(void);
int rtc_get_month(void);
int rtc_get_day(void);
int rtc_get_weekday(void);

int rtc_get_hour(void);
int rtc_get_minute(void);
int rtc_get_second(void);

#ifdef __cplusplus
}
#endif
