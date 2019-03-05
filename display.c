#include <stdint.h>
#include "ccu.h"
#include "system.h"
#include "display.h"
#include "uart.h"

volatile uint32_t framebuffer1[512*512] __attribute__ ((section ("UNCACHED")));
volatile uint32_t framebuffer2[512*512] __attribute__ ((section ("UNCACHED")));
volatile uint32_t framebuffer3[512*512] __attribute__ ((section ("UNCACHED")));

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
  HDMI_FC_INHACTIV0 = (1920 & 0xff);    // Horizontal pixels
  HDMI_FC_INHACTIV1 = (1920 >> 8);      // Horizontal pixels
  HDMI_FC_INHBLANK0 = (280 & 0xff);     // Horizontal blanking
  HDMI_FC_INHBLANK1 = (280 >> 8);       // Horizontal blanking

  HDMI_FC_INVACTIV0 = (1080 & 0xff);    // Vertical pixels
  HDMI_FC_INVACTIV1 = (1080 >> 8);      // Vertical pixels
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
  LCD0_TCON1_BASIC0 = (1919<<16) | 1079;
  LCD0_TCON1_BASIC1 = (1919<<16) | 1079;
  LCD0_TCON1_BASIC2 = (1919<<16) | 1079;
  LCD0_TCON1_BASIC3 = (2199<<16) | 191;
  LCD0_TCON1_BASIC4 = (2250<<16) | 40;
  LCD0_TCON1_BASIC5 = (43<<16) | 4;
  
  LCD0_GINT1 = 1;
  LCD0_GINT0 = (1<<28);
}

// This function configured DE2 as follows:
// MIXER0 -> WB -> MIXER1 -> HDMI
void de2_init() {
  DE_AHB_RESET |= (1<<0);
  DE_SCLK_GATE |= (1<<0);
  DE_HCLK_GATE |= (1<<0);
  DE_DE2TCON_MUX &= ~(1<<0);

  // Erase the whole of MIXER0. This contains uninitialized data.
  for(uint32_t addr = DE_MIXER0 + 0x0000; addr < DE_MIXER0 + 0xC000; addr += 4)
   *(volatile uint32_t*)(addr) = 0;

  DE_MIXER0_GLB_CTL = 1;
  DE_MIXER0_GLB_SIZE = (1079<<16) | 1919;

  DE_MIXER0_BLD_FILL_COLOR_CTL = 0x100;
  DE_MIXER0_BLD_CH_RTCTL = 0;
  DE_MIXER0_BLD_SIZE = (1079<<16) | 1919;
  DE_MIXER0_BLD_CH_ISIZE(0) = (1079<<16) | 1919;

  // The output takes a 480x270 area from a total 512x302
  // buffer leaving a 16px overscan on all 4 sides.
  DE_MIXER0_OVL_V_ATTCTL(0) = (1<<15) | (1<<0);
  DE_MIXER0_OVL_V_MBSIZE(0) = (269<<16) | 479;
  DE_MIXER0_OVL_V_COOR(0) = 0;
  DE_MIXER0_OVL_V_PITCH0(0) = 512*4; // Scan line in bytes including overscan
  DE_MIXER0_OVL_V_TOP_LADD0(0) = (uint32_t)&framebuffer1[512*16+16]; // Start at y=16

  DE_MIXER0_OVL_V_SIZE = (269<<16) | 479;

  DE_MIXER0_VS_CTRL = 1;
  DE_MIXER0_VS_OUT_SIZE = (1079<<16) | 1919;
  DE_MIXER0_VS_Y_SIZE = (269<<16) | 479;
  DE_MIXER0_VS_Y_HSTEP = 0x40000;
  DE_MIXER0_VS_Y_VSTEP = 0x40000;
  DE_MIXER0_VS_C_SIZE = (269<<16) | 479;
  DE_MIXER0_VS_C_HSTEP = 0x40000;
  DE_MIXER0_VS_C_VSTEP = 0x40000;
  for(int n=0;n<32;n++) {
    DE_MIXER0_VS_Y_HCOEF0(n) = 0x40000000;
    DE_MIXER0_VS_Y_HCOEF1(n) = 0;
    DE_MIXER0_VS_Y_VCOEF(n)  = 0x00004000;
    DE_MIXER0_VS_C_HCOEF0(n) = 0x40000000;
    DE_MIXER0_VS_C_HCOEF1(n) = 0;
    DE_MIXER0_VS_C_VCOEF(n)  = 0x00004000;
  }
  DE_MIXER0_VS_CTRL = 1 | (1<<4);
  DE_MIXER0_GLB_DBUFFER = 1;
}

// This function initializes the HDMI port and TCON.
// Almost everything here is resolution specific and
// currently hardcoded to 1920x1080@60Hz.
void display_init() {
  active_buffer = framebuffer1;
  display_clocks_init();
  hdmi_init();
  lcd_init();
  de2_init();
}

void buffer_swap() {
  DE_MIXER0_OVL_V_TOP_LADD0(0) = (uint32_t)(active_buffer + 512*16+16);
  if(active_buffer == framebuffer1) {
      active_buffer = framebuffer2;
  } else if(active_buffer == framebuffer2) {
      active_buffer = framebuffer3;
  } else {
      active_buffer = framebuffer1;
  }
  // Blank visible area
  for(int n=512*16; n<512*(270+16); n++)
    active_buffer[n] = 0;
  DE_MIXER0_GLB_DBUFFER = 1;
}