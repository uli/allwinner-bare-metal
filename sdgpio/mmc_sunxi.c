/*------------------------------------------------------------------------/
/  sunxi MMCv3/SDv1/SDv2 (in native mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2012, ChaN, all right reserved.
/  Copyright (C) 2019 Ulrich Hecht
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/-------------------------------------------------------------------------*/

//#define DEBUG_SDMMC
#define DEBUG_READ_FIRST_BLOCK

#include "diskio.h"		/* Common include file for FatFs and disk I/O layer */


/*-------------------------------------------------------------------------*/
/* Platform dependent macros and functions needed to be modified           */
/*-------------------------------------------------------------------------*/

#include "../ports.h"
#include "../system.h"
#include "../ccu.h"
#include <stdio.h>

#ifdef DEBUG_SDMMC
#define debug printf
#else
#define debug(...)
#endif

// Base address of SDMMC controllers
#define SUNXI_SD_BASE_BASE	0x01c0f000
// Base address of SDMMC controller n
#define SUNXI_SD_BASE(n)	(SUNXI_SD_BASE_BASE + 0x1000 * (n))

#define SUNXI_SD_CTRL(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x000)
#define SUNXI_SD_CLKDIV(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x004)
#define SUNXI_SD_CTYPE(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x00c)
#define SUNXI_SD_BLKSIZ(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x010)
#define SUNXI_SD_BYTCNT(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x014)
#define SUNXI_SD_CMD(n)		*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x018)
#define SUNXI_SD_CMDARG(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x01c)
#define SUNXI_SD_RESP0(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x020)
#define SUNXI_SD_RINTSTS(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x038)
#define SUNXI_SD_STATUS(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x03c)
#define SUNXI_SD_FIFO(n)	*(volatile uint32_t *)(SUNXI_SD_BASE(n) + 0x200)

// Flags for SUNXI_SD_CMD(n) registers
#define SUNXI_MMC_CMD_DATA_EXPIRE	(1 << 9)
#define SUNXI_MMC_CMD_WRITE		(1 << 10)
#define SUNXI_MMC_CMD_AUTO_STOP		(1 << 12)
#define SUNXI_MMC_CMD_WAIT_PRE_OVER	(1 << 13)

// Flags for SUNXI_SD_STATUS(n) registers
#define SUNXI_MMC_STATUS_FIFO_EMPTY	(1 << 2)
#define SUNXI_MMC_STATUS_FIFO_FULL	(1 << 3)
#define SUNXI_MMC_STATUS_CARD_DATA_BUSY	(1 << 9)

// Flags for SUNXI_SD_RINTSTS(n) registers
#define SUNXI_MMC_RINT_COMMAND_DONE		(1 << 2)
#define SUNXI_MMC_RINT_DATA_OVER		(1 << 3)
#define SUNXI_MMC_RINT_AUTO_COMMAND_DONE	(1 << 14)

#define OCR_HCS		0x40000000
#define OCR_BUSY	0x80000000

// Acceptable card voltages
#define MMC_VDD_32_33	0x00100000
#define MMC_VDD_33_34	0x00200000

static void sd_change_clock(DWORD div)
{
	debug("%s\n", __FUNCTION__);

	// Turn off clock
	SUNXI_SD_CLKDIV(0) = 0;

	SUNXI_SD_CMD(0) = 0x80202000;	// change clock
	while (SUNXI_SD_CMD(0) & 0x80000000) {
		// wait
		// XXX: timeout!
	}
	SUNXI_SD_RINTSTS(0) = SUNXI_SD_RINTSTS(0);	// clear interrupts
	debug("off\n");

	// Set clock source and dividers
	SDMMC0_CLK = 0x8002000e;

	// Turn clock back on
	SUNXI_SD_CLKDIV(0) = 0x10000 | div;

	SUNXI_SD_CMD(0) = 0x80202000;	// change clock
	while (SUNXI_SD_CMD(0) & 0x80000000) {
		// wait
		// XXX: timeout!
	}
	SUNXI_SD_RINTSTS(0) = SUNXI_SD_RINTSTS(0);	// clear interrupts
}

void init_port(void) {
	debug("%s\n", __FUNCTION__);
	BUS_CLK_GATING0 |= (1 << 8);	// pass clock
	BUS_SOFT_RST0 &= ~(1 << 8);
	udelay(200);
	BUS_SOFT_RST0 |= (1 << 8);	// de-assert reset
	for (int i = 0; i <= 5; ++i) {
		set_pin_mode(PORTF, i, 2);
		set_pin_pull(PORTF, i, PIO_PULL_UP);
		set_pin_drive(PORTF, i, 2);
	}

	// FIFO DMA, SDR, IRQ/global DMA/debounce off, DMA/FIFO/soft reset
	SUNXI_SD_CTRL(0) = 0x00000007;
	SUNXI_SD_CTYPE(0) = 0;		// bus width 1 bit
	SDMMC0_CLK = 0x8002000e;

	sd_change_clock(0);
}

void sd_send_init(void)
{
	debug("%s\n", __FUNCTION__);
	SUNXI_SD_CTYPE(0) = 0;	// bus width 1
	SUNXI_SD_CMDARG(0) = 0;
	SUNXI_SD_CMD(0) = 0x80008000;	// CMD0, don't change clock, send init seq
	while (!(SUNXI_SD_RINTSTS(0) & SUNXI_MMC_RINT_COMMAND_DONE)) {
		// wait for completion
		// XXX: timeout!
	}
	SUNXI_SD_RINTSTS(0) = 0xffffffff;	// clear interrupts
}

/* MMC/SD commands */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND */
#define CMD2	(2)			/* ALL_SEND_CID */
#define CMD3	(3)			/* SEND_RELATIVE_ADDR */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD7	(7)			/* SELECT_CARD */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define CMD13	(13)		/* SEND_STATUS */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD41	(41)		/* SEND_OP_COND (ACMD) */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

static
DSTATUS Stat = STA_NOINIT;	/* Disk status */

static
BYTE CardType;			/* b0:MMC, b1:SDv1, b2:SDv2, b3:Block addressing */


/*-----------------------------------------------------------------------*/
/* Send a command packet to the card                                     */
/*-----------------------------------------------------------------------*/

static BYTE send_cmd (BYTE cmd,	DWORD arg);

static
BYTE send_cmd_data (		/* Returns command response (bit7==1:Send failed)*/
	BYTE cmd,		/* Command byte */
	DWORD arg,		/* Argument */
	BYTE *buf,
	int bytes
)
{
	BYTE n;
	debug("%s cmd %d arg %08X buf 0x%08x bytes %d\n", __FUNCTION__, cmd, arg, buf, bytes);
	int is_write = cmd == CMD24 || cmd == CMD25;

	if (cmd == CMD12)
		return 0;

	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		n = send_cmd(CMD55, 0);
		if (n) return n;
	}

	DWORD cmdreg = 0x80000140 | cmd;	// expect response, check CRC

	if (buf) {
		if (is_write)
			cmdreg |= SUNXI_MMC_CMD_WRITE;

		if (bytes > 512)
			cmdreg |= SUNXI_MMC_CMD_AUTO_STOP;

		cmdreg |= SUNXI_MMC_CMD_DATA_EXPIRE|SUNXI_MMC_CMD_WAIT_PRE_OVER;
		
		SUNXI_SD_BLKSIZ(0) = bytes < 512 ? bytes : 512;
		SUNXI_SD_BYTCNT(0) = bytes;
	}
	
	SUNXI_SD_CMDARG(0) = arg;

	debug("cmd reg %08X\n", cmdreg);
	SUNXI_SD_CMD(0) = cmdreg;

	if (buf) {
		const DWORD wait_bit = is_write ? SUNXI_MMC_STATUS_FIFO_FULL :
						  SUNXI_MMC_STATUS_FIFO_EMPTY;
		DWORD *buf32 = (DWORD *)buf;
		for (int i = 0; i < bytes / 4; ++i) {
			while (SUNXI_SD_STATUS(0) & wait_bit) {
				udelay(10);
				debug("wbit %d R %08X S %08X\n", wait_bit,
					SUNXI_SD_RINTSTS(0), SUNXI_SD_STATUS(0));
				// XXX: timeout!
			}
			if (is_write)
				SUNXI_SD_FIFO(0) = buf32[i];
			else
				buf32[i] = SUNXI_SD_FIFO(0);
		}
	}

	while (!(SUNXI_SD_RINTSTS(0) & SUNXI_MMC_RINT_COMMAND_DONE)) {
		// wait
		// XXX: timeout!
	}
	
	if (buf) {
		DWORD waitflag;
		if (bytes > 512)
			waitflag = SUNXI_MMC_RINT_AUTO_COMMAND_DONE;
		else
			waitflag = SUNXI_MMC_RINT_DATA_OVER;
		while (!((n = SUNXI_SD_RINTSTS(0)) & waitflag)) {
			// wait
			// XXX: timeout
		}
	}

	// XXX: global reset on error

	SUNXI_SD_RINTSTS(0) = 0xffffffff;
	SUNXI_SD_CTRL(0) |= 2;	// reset FIFO
	debug("resp0 %08X\n", SUNXI_SD_RESP0(0));
	
	return 0;			/* Return with the response value */
}


static BYTE send_cmd (
	BYTE cmd,
	DWORD arg
)
{
	return send_cmd_data(cmd, arg, 0, 0);
}


/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv			/* Drive number (always 0) */
)
{
	DSTATUS s;

	if (pdrv) return STA_NOINIT;

	/* Check if the card is kept initialized */
	s = Stat;
	if (!(s & STA_NOINIT)) {
		if (send_cmd(CMD13, 0))	/* Read card status */
			s = STA_NOINIT;
	}
	Stat = s;

	return s;
}



/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv		/* Physical drive nmuber (0) */
)
{
	BYTE ty, cmd;
	UINT tmr, rca;
	DSTATUS s;

	if (pdrv) return RES_NOTRDY;

	udelay(1000);
	init_port();

	sd_send_init();

	ty = 0;

	if (send_cmd(CMD0, 0) == 0) {
		if (send_cmd(CMD8, 0x1AA) == 0) {
			if (SUNXI_SD_RESP0(0) == 0x01AA) {
				for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state (ACMD41 with HCS bit) */
					if (send_cmd(ACMD41, OCR_HCS | MMC_VDD_32_33 | MMC_VDD_33_34) == 0 &&
					    (SUNXI_SD_RESP0(0) & OCR_BUSY)) break;
					udelay(1000);
				}
				ty = SUNXI_SD_RESP0(0) & OCR_HCS ? CT_SD2 | CT_BLOCK : CT_SD2;
			}
		} else {							/* SDv1 or MMCv3 */
			if (send_cmd(ACMD41, 0) == 0) 	{
				ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
			} else {
				ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
			}
			for (tmr = 1000; tmr; tmr--) {			/* Wait for leaving idle state */
				if (send_cmd(cmd, 0) == 0 &&
				    (SUNXI_SD_RESP0(0) & OCR_BUSY))
					break;
				udelay(1000);
			}
			if (!tmr || send_cmd(CMD16, 512) != 0)	/* Set R/W block length to 512 */
				ty = 0;
		}
		if (send_cmd(CMD2, 0) == 0) {
			// If we don't wait here, the initialization fails when
			// debug output is switched off.
			// XXX: Find out what the proper procedure is at this point.
			udelay(1000);
			if (send_cmd(CMD3, 0) == 0) {
				rca = SUNXI_SD_RESP0(0) >> 16;
				if (send_cmd(CMD7, rca << 16) != 0)
					ty = 0;
			}
		} else
			ty = 0;
	}
	CardType = ty;
	s = ty ? 0 : STA_NOINIT;
	Stat = s;

#ifdef DEBUG_READ_FIRST_BLOCK
	printf("type %d\n", ty);
	if (!s) {
		BYTE buf[512];
		disk_read(0, buf, 0, 1);
		for (int i = 0; i < 512; ++i) {
			if (!(i&0xf)) {
				printf("\n%08X  ", i);
			}
			printf("%02X ", buf[i]);
		}
	}
#endif

	return s;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..128) */
)
{
	if (disk_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
	if (!count) return RES_PARERR;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1) {
		if (send_cmd_data(CMD17, sector, buff, 512) == 0)	/* READ_SINGLE_BLOCK */
			count = 0;
	}
	else {
		if (send_cmd_data(CMD18, sector, buff, count * 512) == 0) {	/* READ_MULTIPLE_BLOCK */
			count = 0;
		}
	}

	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	DWORD sector,		/* Start sector number (LBA) */
	BYTE count			/* Sector count (1..128) */
)
{
	if (disk_status(pdrv) & STA_NOINIT) return RES_NOTRDY;
	if (!count) return RES_PARERR;
	if (!(CardType & CT_BLOCK)) sector *= 512;	/* Convert LBA to byte address if needed */

	if (count == 1) {	/* Single block write */
		if (send_cmd_data(CMD24, sector, (BYTE *)buff, 512) == 0)	/* WRITE_BLOCK */
			count = 0;
	}
	else {				/* Multiple block write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);
		if (send_cmd_data(CMD25, sector, (BYTE *)buff, count * 512) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			count = 0;
		}
	}

	return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD cs;


	if (disk_status(pdrv) & STA_NOINIT) return RES_NOTRDY;	/* Check if card is in the socket */

	res = RES_ERROR;
	switch (cmd) {
		case CTRL_SYNC :		/* Make sure that no pending write process */
			// XXX: How?
			res = RES_OK;
			break;

		case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			if (send_cmd_data(CMD9, 0, csd, 16) == 0) {
				if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
					cs = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 8) + 1;
					*(DWORD*)buff = cs << 10;
				} else {					/* SDC ver 1.XX or MMC */
					n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
					cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
					*(DWORD*)buff = cs << (n - 9);
				}
				res = RES_OK;
			}
			break;

		case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
			*(DWORD*)buff = 128;
			res = RES_OK;
			break;

		default:
			res = RES_PARERR;
	}

	return res;
}



/*-----------------------------------------------------------------------*/
/* This function is defined for only project compatibility               */

void disk_timerproc (void)
{
	/* Nothing to do */
}

