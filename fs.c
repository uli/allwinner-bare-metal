#include <stdio.h>
#include <stdlib.h>
#include "ports.h"
#include "fatfs/ff.h"
#include "fatfs/diskio.h"

FATFS Fatfs;

int fs_init(void)
{
	DSTATUS st = disk_initialize(0);
	if (st != 0)
		return st;
	printf("SD initialized\n");
	f_mount(&Fatfs, "sd", 0);
	return 0;
}

int fs_deinit(void)
{
	f_unmount(0);
	return 0;
}

DWORD get_fattime(void)
{
	/* fake time: 2019-01-01 00:00:00 */
	return ((DWORD)(2019 - 1980) << 25) |
	       ((DWORD)1 << 21) |
	       ((DWORD)1 << 16) |
	       ((DWORD)0 << 11) |
	       ((DWORD)0 << 5)  |
	       ((DWORD)0 >> 1);
}

void* ff_memalloc (UINT msize)
{
	return malloc(msize);
}

void ff_memfree (void* mblock)
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
