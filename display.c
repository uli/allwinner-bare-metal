#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ccu.h"
#include "system.h"
#include "display.h"
#include "uart.h"
#include "mmu.h"
#include "util.h"

static uint32_t *framebuffer1 = 0;
static uint32_t *framebuffer2 = 0;

volatile uint32_t *display_active_buffer;
volatile uint32_t *display_visible_buffer;

static struct {
	int fb_width, fb_height, fb_bytes;
	int x, y, ovx, ovy;
} dsp;

void display_clocks_init() {
  // Set up shared and dedicated clocks for HDMI, LCD/TCON and DE2
  PLL_DE_CTRL      = BIT(31) | BIT(24) | (17<<8) | (0<<0); // 432MHz
  PLL_VIDEO_CTRL   = BIT(31) | BIT(25) | BIT(24) | (98<<8) | (7<<0); // 297MHz
  BUS_CLK_GATING1 |= BIT(12) | BIT(11) | BIT(3); // Enable DE, HDMI, TCON0	// check
  BUS_SOFT_RST1   |= BIT(12) | (3<<10) | BIT(3); // De-assert reset of DE, HDMI0/1, TCON0
  DE_CLK           = BIT(31) | BIT(24); // Enable DE clock, set source to PLL_DE
  HDMI_CLK         = BIT(31); // Enable HDMI clk (use PLL3)
  HDMI_SLOW_CLK    = BIT(31); // Enable HDMI slow clk	// check

  // XXX: OPi seems to be running at a lower frequency.
  //TCON0_CLK        = BIT(31) | 3; // Enable TCON0 clk, divide by 4
  // This setting, also used by the Linux kernel, works.
  TCON0_CLK        = 0x80000001;
}

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
static void de2_init() {
  DE_AHB_RESET |= BIT(0);
  DE_SCLK_GATE |= BIT(0);
  DE_HCLK_GATE |= BIT(0);
  DE_DE2TCON_MUX &= ~BIT(0);

  // Erase the whole of MIXER0. This contains uninitialized data.
  for(uint32_t addr = DE_MIXER0 + 0x0000; addr < DE_MIXER0 + 0xC000; addr += 4)
   *(volatile uint32_t*)(addr) = 0;

  DE_MIXER0_GLB_CTL = 1;
  DE_MIXER0_GLB_SIZE = ((DISPLAY_HDMI_RES_Y - 1) << 16) | (DISPLAY_HDMI_RES_X - 1);

  DE_MIXER0_BLD_FILL_COLOR_CTL = 0x100;
  DE_MIXER0_BLD_CH_RTCTL = 0;
  DE_MIXER0_BLD_SIZE = ((DISPLAY_HDMI_RES_Y - 1) << 16) | (DISPLAY_HDMI_RES_X - 1);
  DE_MIXER0_BLD_CH_ISIZE(0) = ((DISPLAY_HDMI_RES_Y - 1) << 16) | (DISPLAY_HDMI_RES_X - 1);

  // The output takes a dsp.x*dsp.y area from a total (dsp.x+dsp.ovx)*(dsp.y+dsp.ovy) buffer
  DE_MIXER0_OVL_V_ATTCTL(0) = BIT(15) | BIT(0);
  DE_MIXER0_OVL_V_MBSIZE(0) = ((dsp.y - 1) << 16) | (dsp.x - 1);
  DE_MIXER0_OVL_V_COOR(0) = 0;
  DE_MIXER0_OVL_V_PITCH0(0) = dsp.fb_width * 4; // Scan line in bytes including overscan
  DE_MIXER0_OVL_V_TOP_LADD0(0) = (uint32_t)
  	(framebuffer1 + dsp.fb_width * dsp.ovy + dsp.ovx);

  DE_MIXER0_OVL_V_SIZE = ((dsp.y - 1) << 16) | (dsp.x - 1);

  DE_MIXER0_VS_CTRL = 1;
  DE_MIXER0_VS_OUT_SIZE = ((DISPLAY_HDMI_RES_Y - 1) << 16) | (DISPLAY_HDMI_RES_X - 1);
  DE_MIXER0_VS_Y_SIZE = ((dsp.y - 1) << 16) | (dsp.x - 1);
  double scale_x = (double)dsp.x * (double)0x100000 / (double)DISPLAY_HDMI_RES_X;
  DE_MIXER0_VS_Y_HSTEP = (uint32_t)scale_x;
  double scale_y = (double)dsp.y * (double)0x100000 / (double)DISPLAY_HDMI_RES_Y;
  DE_MIXER0_VS_Y_VSTEP = (uint32_t)scale_y;
  DE_MIXER0_VS_C_SIZE = ((dsp.y - 1) << 16) | (dsp.x - 1);
  DE_MIXER0_VS_C_HSTEP = (uint32_t)scale_x;
  DE_MIXER0_VS_C_VSTEP = (uint32_t)scale_y;

  de2_update_filter(0);

  DE_MIXER0_GLB_DBUFFER = 1;
}

#include <../device/fb/display_timing.h>

void h3_de2_init(struct display_timing *timing, uint32_t fbbase);

struct display_timing default_timing;

// This function initializes the HDMI port and TCON.
// Almost everything here is resolution specific and
// currently hardcoded to 1920x1080@60Hz.
void display_init(const struct display_phys_mode_t *mode) {
  if (mode) {
    default_timing.hdmi_monitor = mode->hdmi;
    default_timing.pixelclock.typ = mode->pixclk;
    default_timing.hactive.typ = mode->hactive;
    default_timing.hback_porch.typ = mode->hback_porch;
    default_timing.hfront_porch.typ = mode->hfront_porch;
    default_timing.hsync_len.typ = mode->hsync_width;
    default_timing.vactive.typ = mode->vactive;
    default_timing.vback_porch.typ = mode->vback_porch;
    default_timing.vfront_porch.typ = mode->vfront_porch;
    default_timing.vsync_len.typ = mode->vsync_width;
    default_timing.flags = 0;
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

    default_timing.hdmi_monitor = true;
    default_timing.pixelclock.typ = 40000000;
    default_timing.hactive.typ = 800;
    default_timing.hback_porch.typ = 88;
    default_timing.hfront_porch.typ = 40;
    default_timing.hsync_len.typ = 128;
    default_timing.vactive.typ = 600;
    default_timing.vback_porch.typ = 23;
    default_timing.vfront_porch.typ = 1;
    default_timing.vsync_len.typ = 4;
    default_timing.flags = (DISPLAY_FLAGS_HSYNC_LOW | DISPLAY_FLAGS_VSYNC_LOW);
  };

  h3_de2_init(&default_timing, (uint32_t)0x40000000);

  LCD0_GINT1 = 1;
  LCD0_GINT0 = BIT(28);
}


// Allocates frame buffers and configures the display engine
// to scale from the given resolution to the HDMI resolution.
void display_set_mode(int x, int y, int ovx, int ovy)
{
  free(framebuffer1);
  free(framebuffer2);

  dsp.ovx = ovx; dsp.ovy = ovy;
  dsp.x = x; dsp.y = y;
  dsp.fb_width = x + ovx * 2;
  dsp.fb_height = y + ovy;
  dsp.fb_bytes = (x + ovx * 2) * (y + ovy) * 4;

  framebuffer1 = (uint32_t *)calloc(1, dsp.fb_bytes);
  framebuffer2 = (uint32_t *)calloc(1, dsp.fb_bytes);

  display_active_buffer = framebuffer1;

  de2_init();
}

int display_single_buffer = 0;

void display_swap_buffers() {
  // Make sure whatever is in the active buffer is committed to memory.
  mmu_flush_dcache();

  if (display_single_buffer)
    display_active_buffer = framebuffer1;

  display_visible_buffer = display_active_buffer;
  DE_MIXER0_OVL_V_TOP_LADD0(0) = (uint32_t)
  	(display_active_buffer + dsp.fb_width * dsp.ovy + dsp.ovx);

  if (!display_single_buffer) {
    if(display_active_buffer == framebuffer1) {
        display_active_buffer = framebuffer2;
    } else if(display_active_buffer == framebuffer2) {
        display_active_buffer = framebuffer1;
    }
  }

  DE_MIXER0_GLB_DBUFFER = 1;
}

void display_clear_active_buffer(void)
{
  memset((void *)display_active_buffer, 0, dsp.fb_bytes);
}
