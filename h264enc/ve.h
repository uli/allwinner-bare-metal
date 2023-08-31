/*
 * Copyright (c) 2013 Jens Kuske <jenskuske@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __VE_H__
#define __VE_H__

#include <stdint.h>

int ve_open(void);
void ve_close(void);
int ve_get_version(void);
int ve_wait(int timeout);
void *ve_get(int engine, uint32_t flags);
void ve_put(void);

void *ve_malloc(int size);
void ve_free(void *ptr);
uint32_t ve_virt2phys(void *ptr);
void ve_flush_cache(void *start, int len);

static inline void writeb(uint8_t val, void *addr)
{
	*((volatile uint8_t *)addr) = val;
}

static inline void writel(uint32_t val, void *addr)
{
	*((volatile uint32_t *)addr) = val;
}

static inline uint32_t readl(void *addr)
{
	return *((volatile uint32_t *) addr);
}

#define VE_ENGINE_MPEG			0x0
#define VE_ENGINE_H264			0x1
#define VE_ENGINE_AVC			0xb

#define VE_CTRL				0x000
#define VE_RESET			0x004
#define VE_VERSION			0x0f0

#define VE_MPEG_PIC_HDR			0x100
#define VE_MPEG_VOP_HDR			0x104
#define VE_MPEG_SIZE			0x108
#define VE_MPEG_FRAME_SIZE		0x10c
#define VE_MPEG_MBA			0x110
#define VE_MPEG_CTRL			0x114
#define VE_MPEG_TRIGGER			0x118
#define VE_MPEG_STATUS			0x11c
#define VE_MPEG_TRBTRD_FIELD		0x120
#define VE_MPEG_TRBTRD_FRAME		0x124
#define VE_MPEG_VLD_ADDR		0x128
#define VE_MPEG_VLD_OFFSET		0x12c
#define VE_MPEG_VLD_LEN			0x130
#define VE_MPEG_VLD_END			0x134
#define VE_MPEG_MBH_ADDR		0x138
#define VE_MPEG_DCAC_ADDR		0x13c
#define VE_MPEG_NCF_ADDR		0x144
#define VE_MPEG_REC_LUMA		0x148
#define VE_MPEG_REC_CHROMA		0x14c
#define VE_MPEG_FWD_LUMA		0x150
#define VE_MPEG_FWD_CHROMA		0x154
#define VE_MPEG_BACK_LUMA		0x158
#define VE_MPEG_BACK_CHROMA		0x15c
#define VE_MPEG_IQ_MIN_INPUT		0x180
#define VE_MPEG_QP_INPUT		0x184
#define VE_MPEG_JPEG_SIZE		0x1b8
#define VE_MPEG_JPEG_RES_INT		0x1c0
#define VE_MPEG_ROT_LUMA		0x1cc
#define VE_MPEG_ROT_CHROMA		0x1d0
#define VE_MPEG_SDROT_CTRL		0x1d4
#define VE_MPEG_RAM_WRITE_PTR		0x1e0
#define VE_MPEG_RAM_WRITE_DATA		0x1e4

#define VE_H264_FRAME_SIZE		0x200
#define VE_H264_PIC_HDR			0x204
#define VE_H264_SLICE_HDR		0x208
#define VE_H264_SLICE_HDR2		0x20c
#define VE_H264_PRED_WEIGHT		0x210
#define VE_H264_QP_PARAM		0x21c
#define VE_H264_CTRL			0x220
#define VE_H264_TRIGGER			0x224
#define VE_H264_STATUS			0x228
#define VE_H264_CUR_MB_NUM		0x22c
#define VE_H264_VLD_ADDR		0x230
#define VE_H264_VLD_OFFSET		0x234
#define VE_H264_VLD_LEN			0x238
#define VE_H264_VLD_END			0x23c
#define VE_H264_SDROT_CTRL		0x240
#define VE_H264_OUTPUT_FRAME_IDX	0x24c
#define VE_H264_EXTRA_BUFFER1		0x250
#define VE_H264_EXTRA_BUFFER2		0x254
#define VE_H264_BASIC_BITS		0x2dc
#define VE_H264_RAM_WRITE_PTR		0x2e0
#define VE_H264_RAM_WRITE_DATA		0x2e4

#define VE_SRAM_H264_PRED_WEIGHT_TABLE	0x000
#define VE_SRAM_H264_FRAMEBUFFER_LIST	0x400
#define VE_SRAM_H264_REF_LIST0		0x640
#define VE_SRAM_H264_REF_LIST1		0x664
#define VE_SRAM_H264_SCALING_LISTS	0x800

#define VE_ISP_INPUT_SIZE		0xa00
#define VE_ISP_INPUT_STRIDE		0xa04
#define VE_ISP_CTRL			0xa08
#define VE_ISP_INPUT_LUMA		0xa78
#define VE_ISP_INPUT_CHROMA		0xa7c

#define VE_AVC_PARAM			0xb04
#define VE_AVC_QP			0xb08
#define VE_AVC_MOTION_EST		0xb10
#define VE_AVC_CTRL			0xb14
#define VE_AVC_TRIGGER			0xb18
#define VE_AVC_STATUS			0xb1c
#define VE_AVC_BASIC_BITS		0xb20
#define VE_AVC_UNK_BUF			0xb60
#define VE_AVC_VLE_ADDR			0xb80
#define VE_AVC_VLE_END			0xb84
#define VE_AVC_VLE_OFFSET		0xb88
#define VE_AVC_VLE_MAX			0xb8c
#define VE_AVC_VLE_LENGTH		0xb90
#define VE_AVC_REF_LUMA			0xba0
#define VE_AVC_REF_CHROMA		0xba4
#define VE_AVC_REC_LUMA			0xbb0
#define VE_AVC_REC_CHROMA		0xbb4
#define VE_AVC_REF_SLUMA		0xbb8
#define VE_AVC_REC_SLUMA		0xbbc
#define VE_AVC_MB_INFO			0xbc0

#endif
