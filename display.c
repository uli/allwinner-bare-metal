#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ccu.h"
#include "display.h"
#include "interrupts.h"
#include "mmu.h"
#include "system.h"
#include "tve.h"
#include "uart.h"
#include "util.h"
#include "fixed_addr.h"

int display_is_digital;

static uint32_t *framebuffer1 = 0;
static uint32_t *framebuffer2 = 0;

volatile uint32_t *display_active_buffer;
volatile uint32_t *display_visible_buffer;

struct virt_mode_t dsp;

static int filter_enabled = 0;

static void de2_update_filter(int sub)
{
  if (!filter_enabled)
    display_scaler_nearest_neighbour();
  else
    display_scaler_set_coeff(DE_MIXER0_VS_C_HSTEP, sub);

  DE_MIXER0_VS_CTRL = 1 | BIT(4);
}

void display_enable_filter(int onoff)
{
  filter_enabled = !!onoff;
  de2_update_filter(onoff - 1);
  DE_MIXER0_GLB_DBUFFER = 1;
}

// This function configured DE2 as follows:
// MIXER0 -> WB -> MIXER1 -> HDMI
static void de2_init()
{
  DE_AHB_RESET |= BIT(0);
  DE_SCLK_GATE |= BIT(0);
  DE_HCLK_GATE |= BIT(0);
  DE_DE2TCON_MUX &= ~BIT(0);

  // Erase the whole of MIXER0. This contains uninitialized data.
  for (uint32_t addr = DE_MIXER0 + 0x0000; addr < DE_MIXER0 + 0xC000; addr += 4)
    *(volatile uint32_t *)(addr) = 0;

  DE_MIXER0_GLB_CTL  = 1;
  DE_MIXER0_GLB_SIZE = ((DISPLAY_PHYS_RES_Y - 1) << 16) | (DISPLAY_PHYS_RES_X - 1);

  DE_MIXER0_BLD_FILL_COLOR_CTL = 0x100;
  DE_MIXER0_BLD_CH_RTCTL       = 0;
  DE_MIXER0_BLD_SIZE           = ((DISPLAY_PHYS_RES_Y - 1) << 16) | (DISPLAY_PHYS_RES_X - 1);
  DE_MIXER0_BLD_CH_ISIZE(0)    = ((DISPLAY_PHYS_RES_Y - 1) << 16) | (DISPLAY_PHYS_RES_X - 1);

  // The output takes a dsp.x*dsp.y area from a total (dsp.x+dsp.ovx)*(dsp.y+dsp.ovy) buffer
  DE_MIXER0_OVL_V_ATTCTL(0)    = BIT(15) | BIT(0);
  DE_MIXER0_OVL_V_MBSIZE(0)    = ((dsp.y - 1) << 16) | (dsp.x - 1);
  DE_MIXER0_OVL_V_COOR(0)      = 0;
  DE_MIXER0_OVL_V_PITCH0(0)    = dsp.fb_width * 4;  // Scan line in bytes including overscan
  DE_MIXER0_OVL_V_TOP_LADD0(0) = (uint32_t)(framebuffer1 + dsp.fb_width * dsp.ovy + dsp.ovx);

  DE_MIXER0_OVL_V_SIZE = ((dsp.y - 1) << 16) | (dsp.x - 1);

  DE_MIXER0_VS_CTRL     = 1;
  DE_MIXER0_VS_OUT_SIZE = ((DISPLAY_PHYS_RES_Y - 1) << 16) | (DISPLAY_PHYS_RES_X - 1);
  DE_MIXER0_VS_Y_SIZE   = ((dsp.y - 1) << 16) | (dsp.x - 1);
  double scale_x        = (double)dsp.x * (double)0x100000 / (double)DISPLAY_PHYS_RES_X;
  DE_MIXER0_VS_Y_HSTEP  = (uint32_t)scale_x;
  double scale_y        = (double)dsp.y * (double)0x100000 / (double)DISPLAY_PHYS_RES_Y;
  DE_MIXER0_VS_Y_VSTEP  = (uint32_t)scale_y;
  DE_MIXER0_VS_C_SIZE   = ((dsp.y - 1) << 16) | (dsp.x - 1);
  DE_MIXER0_VS_C_HSTEP  = (uint32_t)scale_x;
  DE_MIXER0_VS_C_VSTEP  = (uint32_t)scale_y;

  de2_update_filter(0);

  DE_MIXER0_GLB_DBUFFER = 1;
}

#include <../device/fb/display_timing.h>

int h3_de2_init(struct display_timing *timing, uint32_t fbbase);

struct display_timing default_timing;

int display_init(const struct display_phys_mode_t *mode)
{
  if (mode != NULL && !display_is_digital)
    return 0;

  if (mode) {
    default_timing.hdmi_monitor     = mode->hdmi;
    default_timing.pixelclock.typ   = mode->pixclk;
    default_timing.hactive.typ      = mode->hactive;
    default_timing.hback_porch.typ  = mode->hback_porch;
    default_timing.hfront_porch.typ = mode->hfront_porch;
    default_timing.hsync_len.typ    = mode->hsync_width;
    default_timing.vactive.typ      = mode->vactive;
    default_timing.vback_porch.typ  = mode->vback_porch;
    default_timing.vfront_porch.typ = mode->vfront_porch;
    default_timing.vsync_len.typ    = mode->vsync_width;
    default_timing.flags            = 0;
    if (mode->hsync_pol)
      default_timing.flags |= DISPLAY_FLAGS_HSYNC_HIGH;
    else
      default_timing.flags |= DISPLAY_FLAGS_HSYNC_LOW;
    if (mode->vsync_pol)
      default_timing.flags |= DISPLAY_FLAGS_VSYNC_HIGH;
    else
      default_timing.flags |= DISPLAY_FLAGS_VSYNC_LOW;
  } else {
    memset(&default_timing, 0, sizeof(struct display_timing));

    default_timing.hdmi_monitor     = true;
    default_timing.pixelclock.typ   = 40000000;
    default_timing.hactive.typ      = 800;
    default_timing.hback_porch.typ  = 88;
    default_timing.hfront_porch.typ = 40;
    default_timing.hsync_len.typ    = 128;
    default_timing.vactive.typ      = 600;
    default_timing.vback_porch.typ  = 23;
    default_timing.vfront_porch.typ = 1;
    default_timing.vsync_len.typ    = 4;
    default_timing.flags            = (DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW);
  };

  if (h3_de2_init(&default_timing, (uint32_t)AWBM_BASE_ADDR) == 0) {
    display_is_digital = 1;
    LCD0_GINT1         = 1;
    LCD0_GINT0         = BIT(30);
    irq_enable(118);  // LCD0
    return 0;
  } else {
    display_is_digital = 0;
    return -1;
  }
}

// Allocates frame buffers and configures the display engine
// to scale from the given resolution to the HDMI resolution.
void display_set_mode(int x, int y, int ovx, int ovy)
{
  free(framebuffer1);
  free(framebuffer2);

  dsp.ovx       = ovx;
  dsp.ovy       = ovy;
  dsp.x         = x;
  dsp.y         = y;
  dsp.fb_width  = x + ovx * 2;
  dsp.fb_height = y + ovy;
  dsp.fb_bytes  = (x + ovx * 2) * (y + ovy) * 4;

  framebuffer1 = (uint32_t *)calloc(1, dsp.fb_bytes);
  framebuffer2 = (uint32_t *)calloc(1, dsp.fb_bytes);

  display_active_buffer = framebuffer1;

  if (display_is_digital)
    de2_init();
  else
    tve_de2_init();  // XXX: refactor
}

int display_single_buffer = 0;

void display_swap_buffers()
{
  if (display_single_buffer)
    display_active_buffer = framebuffer1;

  display_visible_buffer = display_active_buffer;

  // Make sure whatever is in the active buffer is committed to memory.
  mmu_flush_dcache_range((void *)display_visible_buffer, dsp.fb_bytes, MMU_DCACHE_CLEAN);

  if (display_is_digital) {
    DE_MIXER0_OVL_V_TOP_LADD0(0) =
      (uint32_t)(display_active_buffer + dsp.fb_width * dsp.ovy + dsp.ovx);
  } else {
    // XXX: refactor
    tve_set_visible_buffer(display_active_buffer + dsp.fb_width * dsp.ovy + dsp.ovx);
  }

  if (!display_single_buffer) {
    if (display_active_buffer == framebuffer1) {
      display_active_buffer = framebuffer2;
    } else if (display_active_buffer == framebuffer2) {
      display_active_buffer = framebuffer1;
    }
  }

  if (display_is_digital)
    DE_MIXER0_GLB_DBUFFER = 1;
  else
    tve_update_buffer();  // XXX: refactor
}

void display_clear_active_buffer(void)
{
  memset((void *)display_active_buffer, 0, dsp.fb_bytes);
}

// Coefficients for RT-WB fine scaling. It seems FS is always required when
// using semi-planar output formats, even if input and output are the same
// size.
// XXX: different coefficients are used for downscaling.
static const uint32_t wb_fs_coeffs[] = {
  0x00004000, 0x00033FFE, 0x00063EFC, 0x000A3BFB,
  0xFF0F37FB, 0xFE1433FB, 0xFD192FFB, 0xFD1F29FB,
  0xFC2424FC, 0xFB291FFD, 0xFB2F19FD, 0xFB3314FE,
  0xFB370FFF, 0xFB3B0A00, 0xFC3E0600, 0xFE3F0300,
};

void display_capture_set_out_bufs(void *luma_buf, void *chroma_buf)
{
#ifdef AWBM_PLATFORM_h3
  DE_WB_A_CH0_ADDR = (uint32_t)luma_buf;
  DE_WB_B_CH0_ADDR = (uint32_t)luma_buf;
  DE_WB_A_CH1_ADDR = (uint32_t)chroma_buf;
  DE_WB_B_CH1_ADDR = (uint32_t)chroma_buf;
#endif
}

void display_capture_init(int x, int y)
{
#ifdef AWBM_PLATFORM_h3
  // XXX: should we do that?
  //75 POKED sclk_gate,1:POKED hclk_gate,1:POKED ahb_reset,1:VSYNC

  // Turn on clocks.
  DE_SCLK_GATE |= BIT(2);
  DE_HCLK_GATE |= BIT(2);
  DE_AHB_RESET |= BIT(2);

  // ungate clocks, assert reset
  DE_WB_GCTRL = BIT(29) | BIT(28) | BIT(4);

  // input size
  DE_WB_SIZE = DE_SIZE(x, y);
  DE_WB_CROP_SIZE = DE_SIZE(x, y);

  // For NV12 it seems that FS, CS and CSC all need to be turned on, even if
  // input and output dimensions are the same.
  DE_WB_FS_INSIZE = DE_SIZE(x, y);
  DE_WB_FS_OUTSIZE = DE_SIZE(x, y);
  // These values apparently mean "scale 1:1" (from Linux vendor driver):
  DE_WB_FS_HSTEP = BIT(20);
  DE_WB_FS_VSTEP = BIT(20);
  DE_WB_CS_HORZ = 0;
  DE_WB_CS_VERT = 0;
  // enable FS, CS, CSC
  DE_WB_BYPASS = BIT(2) | BIT(1) | BIT(0);

  // Output addresses
  DE_WB_CH0_PITCH = x;
  DE_WB_CH12_PITCH = x;

  DE_WB_FORMAT = 0xc;	// NV12

  // Load coefficients for fine scaling.
  for (int i = 0; i < 15; ++i) {
    DE_WB_CH0_HCOEF(i) = wb_fs_coeffs[i];
    DE_WB_CH1_HCOEF(i) = wb_fs_coeffs[i];
  };

  // deassert reset, clear IRQ flag
  DE_WB_GCTRL = BIT(29) | BIT(28);
  DE_WB_STATUS = BIT(0);
#endif
}

void display_capture_kick(void)
{
#ifdef AWBM_PLATFORM_h3
  DE_WB_GCTRL |= BIT(0);
#endif
}

int display_capture_frame_ready(void **luma, void **chroma)
{
#ifdef AWBM_PLATFORM_h3
  if (DE_WB_STATUS & BIT(0)) {
    *luma = (void *)DE_WB_A_CH0_ADDR;
    *chroma = (void *)DE_WB_A_CH1_ADDR;
    return 1;
  }
#endif

  return 0;
}

void display_capture_ack_frame(void)
{
#ifdef AWBM_PLATFORM_h3
  DE_WB_STATUS |= BIT(0);
#endif
}

void display_capture_stop(void)
{
#ifdef AWBM_PLATFORM_h3
  // gate clocks, assert reset
  DE_WB_GCTRL = BIT(4);

  // turn off clocks
  DE_SCLK_GATE &= ~BIT(2);
  DE_HCLK_GATE &= ~BIT(2);
  DE_AHB_RESET &= ~BIT(2);
#endif
}
