/*----------------------------------------------------------------------*/
/* FatFs sample project for generic microcontrollers (C)ChaN, 2012      */
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "ff.h"


FATFS Fatfs;		/* File system object */
FIL Fil;			/* File object */
BYTE Buff[128];		/* File read buffer */


void die (		/* Stop with dying message */
	FRESULT rc	/* FatFs return value */
)
{
	printf("Failed with rc=%u.\n", rc);
	exit(0);
}


/*-----------------------------------------------------------------------*/
/* Program Main                                                          */
/*-----------------------------------------------------------------------*/

int u32flag;
int main (void)
{
	FRESULT rc;				/* Result code */
	DIR dir;				/* Directory object */
	FILINFO fno;			/* File information object */
	UINT bw, br, u32i, u32j;
	UINT u32val[3]={121, 253, 199};
        char unitval, tensval, hundredval;
        int i;
        char FileName[32] = "HELLO.TXT";

	f_mount(0, &Fatfs);		/* Register volume work area (never fails) */

	printf("\nOpen an existing file (%s).\n",FileName);
	//rc = f_open(&Fil, "MESSAGE.TXT", FA_READ);
	rc = f_open(&Fil, FileName, FA_READ);
        printf("f_open rc=%d\n",rc);
	if (!rc) {

		printf("\nType the file content.\n");
		for (;;) {
			rc = f_read(&Fil, Buff, sizeof Buff, &br);	/* Read a chunk of file */
			if (rc || !br) break;			/* Error or end of file */
			for (i = 0; i < br; i++)		/* Type the data */
				putchar(Buff[i]);
		}
		if (rc) die(rc);

		printf("\nClose the file.\n");
		rc = f_close(&Fil);
		if (rc) die(rc);
	}

	printf("\nCreate a new file (%s).\n",FileName);
	//rc = f_open(&Fil, "HELLO.TXT", FA_WRITE | FA_CREATE_ALWAYS);
	rc = f_open(&Fil, FileName, FA_WRITE | FA_CREATE_ALWAYS);
	if (rc) die(rc);

	printf("\nWrite a text data. (Hello world!)\n");
	rc = f_write(&Fil, "Hello world!\r\n", 14, &bw);
	if (rc) die(rc);
	printf("%u bytes written.\n", bw);

	rc = f_write(&Fil, "Goodbye world.\r\n", 16, &bw);
	if (rc) die(rc);
	printf("%u bytes written.\n", bw);

	for (u32i=0; u32i < 3; u32i++) {
        	unitval= (((char)(u32val[u32i]%10))+0x30);
		u32val[u32i]= u32val[u32i]/10;
        	tensval= (((char)(u32val[u32i]%10))+0x30);
		hundredval= (((char)(u32val[u32i]/10))+0x30);

		rc = f_write(&Fil, &hundredval, sizeof(char), &bw);
		if (rc) die(rc);
		printf("%u bytes written.\n", bw);
		rc = f_write(&Fil, &tensval, sizeof(char), &bw);
		if (rc) die(rc);
		printf("%u bytes written.\n", bw);
		rc = f_write(&Fil, &unitval, sizeof(char), &bw);
		if (rc) die(rc);
		printf("%u bytes written.\n", bw);

		rc = f_write(&Fil, "\r\n", 2, &bw);
		if (rc) die(rc);
		printf("%u bytes written.\n", bw);
	}

        /* 
        val = (((char)(u32val[0]%10))+0x30);
	rc = f_write(&Fil, &val, sizeof(UINT), &bw);
	if (rc) die(rc);
	printf("%u bytes written.\n", bw);

	rc = f_write(&Fil, "\r\n", 2, &bw);
	if (rc) die(rc);
	printf("%u bytes written.\n", bw);

        val = (((char)(u32val[1]%10))+0x30);
	rc = f_write(&Fil, &val, sizeof(UINT), &bw);
	if (rc) die(rc);
	printf("%u bytes written.\n", bw);

	rc = f_write(&Fil, "\r\n", 2, &bw);
	if (rc) die(rc);
	printf("%u bytes written.\n", bw);
	*/

	printf("\nClose the file.\n");
	rc = f_close(&Fil);
	if (rc) die(rc);

	printf("\nOpen root directory.\n");
	rc = f_opendir(&dir, "");
	if (rc) die(rc);

	printf("\nDirectory listing...\n");
	for (;;) {
		rc = f_readdir(&dir, &fno);	/* Read a directory item */
		if (rc || !fno.fname[0]) break;	/* Error or end of dir */
		if (fno.fattrib & AM_DIR)
			printf("   <dir>  %s\n", fno.fname);
		else
			printf("%8lu  %s\n", fno.fsize, fno.fname);
	}
	if (rc) die(rc);

	printf("\nTest completed.\n");

	return 0;
}



/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/

DWORD get_fattime (void)
{
	return	  ((DWORD)(2012 - 1980) << 25)	/* Year = 2012 */
			| ((DWORD)1 << 21)				/* Month = 1 */
			| ((DWORD)1 << 16)				/* Day_m = 1*/
			| ((DWORD)0 << 11)				/* Hour = 0 */
			| ((DWORD)0 << 5)				/* Min = 0 */
			| ((DWORD)0 >> 1);				/* Sec = 0 */
}

