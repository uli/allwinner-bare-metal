#include <stdio.h>
#include <stdlib.h>

#include "fatfs/ff.h"
#include "fatfs/diskio.h"
#include "ports.h"
#include "rtc.h"

FATFS Fatfs;

int fs_init(void)
{
#ifdef LIBH3_MMC
  // lib-h3 driver does not seem to do that
  for (int i = 0; i <= 5; ++i) {
    set_pin_mode(PORTF, i, 2);
    set_pin_pull(PORTF, i, PIO_PULL_UP);
    set_pin_drive(PORTF, i, 2);
  }
#endif

  DSTATUS st = disk_initialize(0);
  if (st != 0)
    return st;
  printf("SD initialized\n");
  f_mount(&Fatfs, "/sd", 0);
  return 0;
}

int fs_deinit(void)
{
  f_unmount(0);
  return 0;
}

DWORD get_fattime(void)
{
  return
    ((DWORD)(rtc_get_year() - 1980) << 25) |
    ((DWORD)rtc_get_month() << 21) |
    ((DWORD)rtc_get_day() << 16) |
    ((DWORD)rtc_get_hour() << 11) |
    ((DWORD)rtc_get_minute() << 5) |
    ((DWORD)rtc_get_second() >> 1);
}

void *ff_memalloc(UINT msize)
{
  return malloc(msize);
}

void ff_memfree(void *mblock)
{
  return free(mblock);
}

int sd_detect(void)
{
  static int detected = 0;

  if (!detected && !get_pin_data(PORTF, 6)) {
    printf("card in\n");
    if (!fs_init())
      detected = 1;
  } else if (detected && get_pin_data(PORTF, 6)) {
    printf("card out\n");
    fs_deinit();
    detected = 0;
  }

  return detected;
}

#define FFAPI1(n)             \
  DSTATUS mmc_##n(BYTE pdrv); \
  DSTATUS usb_##n(BYTE pdrv); \
  DSTATUS n(BYTE pdrv)        \
  {                           \
    switch (pdrv) {           \
      case 0:                 \
      case 1:                 \
        return mmc_##n(pdrv); \
      default:                \
        return usb_##n(pdrv); \
    }                         \
  }

FFAPI1(disk_status)
FFAPI1(disk_initialize)

#define FFAPI2(n, t)                                                  \
  DRESULT mmc_##n(BYTE pdrv, t BYTE *buff, DWORD sector, UINT count); \
  DRESULT usb_##n(BYTE pdrv, t BYTE *buff, DWORD sector, UINT count); \
  DRESULT n(BYTE pdrv, t BYTE *buff, DWORD sector, UINT count)        \
  {                                                                   \
    switch (pdrv) {                                                   \
      case 0:                                                         \
      case 1:                                                         \
        return mmc_##n(pdrv, buff, sector, count);                    \
      default:                                                        \
        return usb_##n(pdrv, buff, sector, count);                    \
    }                                                                 \
  }

FFAPI2(disk_read, )
FFAPI2(disk_write, const)

DRESULT mmc_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);
DRESULT usb_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  switch (pdrv) {
    case 0:
    case 1:
      return mmc_disk_ioctl(pdrv, cmd, buff);
    default:
      return usb_disk_ioctl(pdrv, cmd, buff);
  }
}
