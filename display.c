#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ccu.h"
#include "system.h"
#include "display.h"
#include "uart.h"
#include "mmu.h"

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
  PLL_DE_CTRL      = (1<<31) | (1<<24) | (17<<8) | (0<<0); // 432MHz
  PLL_VIDEO_CTRL   = (1<<31) | (1<<25) | (1<<24) | (98<<8) | (7<<0); // 297MHz
  BUS_CLK_GATING1 |= (1<<12) | (1<<11) | (1<<3); // Enable DE, HDMI, TCON0	// check
  BUS_SOFT_RST1   |= (1<<12) | (3<<10) | (1<<3); // De-assert reset of DE, HDMI0/1, TCON0
  DE_CLK           = (1<<31) | (1<<24); // Enable DE clock, set source to PLL_DE
  HDMI_CLK         = (1<<31); // Enable HDMI clk (use PLL3)
  HDMI_SLOW_CLK    = (1<<31); // Enable HDMI slow clk	// check

  // XXX: OPi seems to be running at a lower frequency.
  //TCON0_CLK        = (1<<31) | 3; // Enable TCON0 clk, divide by 4
  // This setting, also used by the Linux kernel, works.
  TCON0_CLK        = 0x80000001;
}

void hdmi_init() {
  // HDMI PHY init, the following black magic is based on the procedure documented at:
  // http://linux-sunxi.org/images/3/38/AW_HDMI_TX_PHY_S40_Spec_V0.1.pdf
  HDMI_PHY_CFG1 = 0;
  HDMI_PHY_CFG1 = 1;
  udelay(5);
  HDMI_PHY_CFG1 |= (1<<16);
  HDMI_PHY_CFG1 |= (1<<1);
  udelay(10);
  HDMI_PHY_CFG1 |= (1<<2);
  udelay(5);
  HDMI_PHY_CFG1 |= (1<<3);
  udelay(40);
  HDMI_PHY_CFG1 |= (1<<19);
  udelay(100);
  HDMI_PHY_CFG1 |= (1<<18);
  HDMI_PHY_CFG1 |= (7<<4);
  while((HDMI_PHY_STS & 0x80) == 0);
  HDMI_PHY_CFG1 |= (0xf<<4);
  HDMI_PHY_CFG1 |= (0xf<<8);
  HDMI_PHY_CFG3 |= (1<<0) | (1<<2);

  HDMI_PHY_PLL1 &= ~(1<<26);
  HDMI_PHY_CEC = 0;

  HDMI_PHY_PLL1 = 0x39dc5040;
  HDMI_PHY_PLL2 = 0x80084381;
  udelay(10000);
  HDMI_PHY_PLL3 = 1;
  HDMI_PHY_PLL1 |= (1<<25);
  udelay(10000);
  uint32_t tmp = (HDMI_PHY_STS & 0x1f800) >> 11;
  HDMI_PHY_PLL1 |= (1<<31) | (1<<30) | tmp;

  HDMI_PHY_CFG1 = 0x01FFFF7F;
  HDMI_PHY_CFG2 = 0x8063A800;
  HDMI_PHY_CFG3 = 0x0F81C485;

  /* enable read access to HDMI controller */
  HDMI_PHY_READ_EN = 0x54524545;
  /* descramble register offsets */
  HDMI_PHY_UNSCRAMBLE = 0x42494E47;

  // HDMI Config, based on the documentation at:
  // https://people.freebsd.org/~gonzo/arm/iMX6-HDMI.pdf
  HDMI_FC_INVIDCONF = (1<<6) | (1<<5) | (1<<4) | (1<<3); // Polarity etc
  HDMI_FC_INHACTIV0 = (DISPLAY_HDMI_RES_X & 0xff);    // Horizontal pixels
  HDMI_FC_INHACTIV1 = (DISPLAY_HDMI_RES_X >> 8);      // Horizontal pixels
  HDMI_FC_INHBLANK0 = (280 & 0xff);     // Horizontal blanking
  HDMI_FC_INHBLANK1 = (280 >> 8);       // Horizontal blanking

  HDMI_FC_INVACTIV0 = (DISPLAY_HDMI_RES_Y & 0xff);    // Vertical pixels
  HDMI_FC_INVACTIV1 = (DISPLAY_HDMI_RES_Y >> 8);      // Vertical pixels
  HDMI_FC_INVBLANK  = 45;               // Vertical blanking

  HDMI_FC_HSYNCINDELAY0 = (88 & 0xff);  // Horizontal Front porch
  HDMI_FC_HSYNCINDELAY1 = (88 >> 8);    // Horizontal Front porch
  HDMI_FC_VSYNCINDELAY  = 4;            // Vertical front porch
  HDMI_FC_HSYNCINWIDTH0 = (44 & 0xff);  // Horizontal sync pulse
  HDMI_FC_HSYNCINWIDTH1 = (44 >> 8);    // Horizontal sync pulse
  HDMI_FC_VSYNCINWIDTH  = 5;            // Vertical sync pulse

  HDMI_FC_CTRLDUR    = 12;   // Frame Composer Control Period Duration
  HDMI_FC_EXCTRLDUR  = 32;   // Frame Composer Extended Control Period Duration
  HDMI_FC_EXCTRLSPAC = 1;    // Frame Composer Extended Control Period Maximum Spacing
  HDMI_FC_CH0PREAM   = 0x0b; // Frame Composer Channel 0 Non-Preamble Data
  HDMI_FC_CH1PREAM   = 0x16; // Frame Composer Channel 1 Non-Preamble Data
  HDMI_FC_CH2PREAM   = 0x21; // Frame Composer Channel 2 Non-Preamble Data
  HDMI_MC_FLOWCTRL   = 0;    // Main Controller Feed Through Control
  HDMI_MC_CLKDIS     = 0x74; // Main Controller Synchronous Clock Domain Disable
}

void lcd_init() {
  // LCD0 feeds mixer0 to HDMI
  LCD0_GCTL         = (1<<31);
  LCD0_GINT0        = 0;
  LCD0_TCON1_CTL    = (1<<31) | (30<<4);
  LCD0_TCON1_BASIC0 = ((DISPLAY_HDMI_RES_X - 1)<<16) | (DISPLAY_HDMI_RES_Y - 1);
  LCD0_TCON1_BASIC1 = ((DISPLAY_HDMI_RES_X - 1)<<16) | (DISPLAY_HDMI_RES_Y - 1);
  LCD0_TCON1_BASIC2 = ((DISPLAY_HDMI_RES_X - 1)<<16) | (DISPLAY_HDMI_RES_Y - 1);
  LCD0_TCON1_BASIC3 = (2199<<16) | 191;
  LCD0_TCON1_BASIC4 = (2250<<16) | 40;
  LCD0_TCON1_BASIC5 = (43<<16) | 4;
  
  LCD0_GINT1 = 1;
  LCD0_GINT0 = (1<<28);
}

static int filter_enabled = 0;

static void de2_update_filter(int sub)
{
  if (!filter_enabled)
    display_scaler_nearest_neighbour();
  else
    display_scaler_set_coeff(DE_MIXER0_VS_C_HSTEP, sub);

  DE_MIXER0_VS_CTRL = 1 | (1<<4);
}

void display_enable_filter(int onoff)
{
  filter_enabled = !!onoff;
  de2_update_filter(onoff - 1);
}

// This function configured DE2 as follows:
// MIXER0 -> WB -> MIXER1 -> HDMI
static void de2_init() {
  DE_AHB_RESET |= (1<<0);
  DE_SCLK_GATE |= (1<<0);
  DE_HCLK_GATE |= (1<<0);
  DE_DE2TCON_MUX &= ~(1<<0);

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
  DE_MIXER0_OVL_V_ATTCTL(0) = (1<<15) | (1<<0);
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

// This function initializes the HDMI port and TCON.
// Almost everything here is resolution specific and
// currently hardcoded to 1920x1080@60Hz.
void display_init() {
  display_clocks_init();
  hdmi_init();
  lcd_init();
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
