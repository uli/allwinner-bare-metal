#ifndef _DISPLAY_H
#define _DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"

struct virt_mode_t {
  int fb_width, fb_height, fb_bytes;
  int x, y, ovx, ovy;
};

#include <../device/fb/display_timing.h>
extern struct display_timing default_timing;

extern int display_is_digital;
extern int display_is_pal;

// HDMI controller output resolution
// NB: Any change in resolution requires additional changes in the HDMI
// controller register settings below.
#define DISPLAY_PHYS_RES_X	(display_is_digital ? default_timing.hactive.typ : 720)
#define DISPLAY_PHYS_RES_Y	(display_is_digital ? default_timing.vactive.typ : (display_is_pal ? 576 : 480))

#define VIDEO_RAM_BYTES 0x180000

// The HDMI registers base address.
#ifdef AWBM_PLATFORM_h3
  #define HDMI_BASE     0x01EE0000
#elif defined(AWBM_PLATFORM_h616)
  #define HDMI_BASE     0x06000000
#endif

#define HDMI_PHY_BASE (HDMI_BASE + 0x10000)

#define HDMI_REG8(off)  *(volatile uint8_t *)(HDMI_BASE + (off))
#define HDMI_REG32(off) *(volatile uint32_t *)(HDMI_BASE + (off))

#ifdef AWBM_PLATFORM_h3

// HDMI register helpers.
#define HDMI_PHY_POL          MEM(HDMI_BASE + 0x10000)
#define HDMI_PHY_READ_EN      MEM(HDMI_BASE + 0x10010)
#define HDMI_PHY_UNSCRAMBLE   MEM(HDMI_BASE + 0x10014)
#define HDMI_PHY_CFG1         MEM(HDMI_BASE + 0x10020)
#define HDMI_PHY_CFG2         MEM(HDMI_BASE + 0x10024)
#define HDMI_PHY_CFG3         MEM(HDMI_BASE + 0x10028)
#define HDMI_PHY_PLL1         MEM(HDMI_BASE + 0x1002C)
#define HDMI_PHY_PLL2         MEM(HDMI_BASE + 0x10030)
#define HDMI_PHY_PLL3         MEM(HDMI_BASE + 0x10034)
#define HDMI_PHY_STS          MEM(HDMI_BASE + 0x10038)
#define HDMI_PHY_CEC          MEM(HDMI_BASE + 0x1003C)

#endif

#include "dw_hdmi.h"

// LCD/TCON
#ifdef AWBM_PLATFORM_h3
  #define LCD0_BASE 0x01C0C000
  #define LCD1_BASE 0x01C0D000
#elif defined(AWBM_PLATFORM_h616)
  #define LCD0_BASE 0x06515000
  #define LCD1_BASE 0x06516000
#endif

#define LCD0_GCTL             MEM(LCD0_BASE + 0x000)
#define LCD0_GINT0            MEM(LCD0_BASE + 0x004)
#define LCD0_GINT1            MEM(LCD0_BASE + 0x008)
#define LCD0_TCON1_CTL        MEM(LCD0_BASE + 0x090)
#define LCD0_TCON1_BASIC0     MEM(LCD0_BASE + 0x094)
#define LCD0_TCON1_BASIC1     MEM(LCD0_BASE + 0x098)
#define LCD0_TCON1_BASIC2     MEM(LCD0_BASE + 0x09C)
#define LCD0_TCON1_BASIC3     MEM(LCD0_BASE + 0x0A0)
#define LCD0_TCON1_BASIC4     MEM(LCD0_BASE + 0x0A4)
#define LCD0_TCON1_BASIC5     MEM(LCD0_BASE + 0x0A8)

#define LCD1_GCTL             MEM(LCD1_BASE + 0x000)
#define LCD1_GINT0            MEM(LCD1_BASE + 0x004)
#define LCD1_GINT1            MEM(LCD1_BASE + 0x008)
#define LCD1_TCON1_CTL        MEM(LCD1_BASE + 0x090)
#define LCD1_TCON1_BASIC0     MEM(LCD1_BASE + 0x094)
#define LCD1_TCON1_BASIC1     MEM(LCD1_BASE + 0x098)
#define LCD1_TCON1_BASIC2     MEM(LCD1_BASE + 0x09C)
#define LCD1_TCON1_BASIC3     MEM(LCD1_BASE + 0x0A0)
#define LCD1_TCON1_BASIC4     MEM(LCD1_BASE + 0x0A4)
#define LCD1_TCON1_BASIC5     MEM(LCD1_BASE + 0x0A8)

// XXX: H3 only?
#define LCD1_TCON1_PS_SYNC    MEM(LCD1_BASE + 0x0B0)

#define LCD1_TCON1_IO_POL     MEM(LCD1_BASE + 0x0F0)
#define LCD1_TCON1_IO_TRI     MEM(LCD1_BASE + 0x0F4)
// end H3 only?

#define LCD1_TCON_CEU_CTL          MEM(LCD1_BASE + 0x100)
#define LCD1_TCON_CEU_COEF_MUL(n)  MEM(LCD1_BASE + 0x110 + (n) * 4)
#define LCD1_TCON_CEU_COEF_RANG(n) MEM(LCD1_BASE + 0x140 + (n) * 4)

// XXX: H3 only?
#define LCD1_TCON1_GAMMA_TABLE(n)  MEM(LCD1_BASE + 0x400 + (n) * 4)
// end H3 only?

#ifdef AWBM_PLATFORM_h3
  #define LCD0_IRQ 118
  #define LCD1_IRQ 119
#elif defined(AWBM_PLATFORM_h616)
  #define LCD0_IRQ 98
  #define LCD1_IRQ 99
#endif

// DE2

// same for H616 (DE3.3)
#define DE_BASE 0x01000000

#ifdef AWBM_PLATFORM_h3
  #define DE_CLOCK_BASE		      DE_BASE
#elif defined(AWBM_PLATFORM_h616)
  #define DE_CLOCK_BASE		      (DE_BASE + 0x8000)
#endif

#define DE_SCLK_GATE                  MEM(DE_CLOCK_BASE + 0x000)
#define DE_HCLK_GATE                  MEM(DE_CLOCK_BASE + 0x004)
#define DE_AHB_RESET                  MEM(DE_CLOCK_BASE + 0x008)
#define DE_SCLK_DIV                   MEM(DE_CLOCK_BASE + 0x00C)
#define DE_DE2TCON_MUX                MEM(DE_CLOCK_BASE + 0x010)
#define DE_CMD_CTL                    MEM(DE_CLOCK_BASE + 0x014)

// Mixer 0
#define DE_MIXER0                     (DE_BASE + 0x100000)

#ifdef AWBM_PLATFORM_h3
  #define DE_MIXER0_GLB		      (DE_MIXER0 + 0x0)
#elif defined(AWBM_PLATFORM_h616)
  #define DE_MIXER0_GLB		      (DE_BASE + 0x8100)
#endif

#define DE_MIXER0_GLB_CTL             MEM(DE_MIXER0_GLB + 0x000)
#define DE_MIXER0_GLB_STS             MEM(DE_MIXER0_GLB + 0x004)

#ifdef AWBM_PLATFORM_h3
  #define DE_MIXER0_GLB_DBUFFER       MEM(DE_MIXER0_GLB + 0x008)
  #define DE_MIXER0_GLB_SIZE          MEM(DE_MIXER0_GLB + 0x00C)
#elif defined(AWBM_PLATFORM_h616)
  #define DE_MIXER0_GLB_DBUFFER       MEM(DE_MIXER0_GLB + 0x010)
  #define DE_MIXER0_GLB_SIZE          MEM(DE_MIXER0_GLB + 0x008)
  #define DE_MIXER0_GLB_CLK           MEM(DE_MIXER0_GLB + 0x00C)
#endif

#ifdef AWBM_PLATFORM_h3
  #define DE_MIXER0_BLD		      (DE_MIXER0 + 0x1000)
#elif defined(AWBM_PLATFORM_h616)
  #define DE_MIXER0_BLD		      (DE_MIXER0 + 0x00181000)
#endif

#define DE_MIXER0_BLD_FILL_COLOR_CTL  MEM(DE_MIXER0_BLD + 0x000)
#define DE_MIXER0_BLD_FILL_COLOR(x)   MEM(DE_MIXER0_BLD + 0x004 + x * 0x10)
#define DE_MIXER0_BLD_CH_ISIZE(x)     MEM(DE_MIXER0_BLD + 0x008 + x * 0x10)
#define DE_MIXER0_BLD_CH_OFFSET(x)    MEM(DE_MIXER0_BLD + 0x00C + x * 0x10)
#define DE_MIXER0_BLD_CH_RTCTL        MEM(DE_MIXER0_BLD + 0x080)
#define DE_MIXER0_BLD_PREMUL_CTL      MEM(DE_MIXER0_BLD + 0x084)
#define DE_MIXER0_BLD_BK_COLOR        MEM(DE_MIXER0_BLD + 0x088)
#define DE_MIXER0_BLD_SIZE            MEM(DE_MIXER0_BLD + 0x08C)
#define DE_MIXER0_BLD_CTL(x)          MEM(DE_MIXER0_BLD + 0x090 + x * 0x4)
#define DE_MIXER0_BLD_KEY_CTL         MEM(DE_MIXER0_BLD + 0x0B0)
#define DE_MIXER0_BLD_KEY_CON         MEM(DE_MIXER0_BLD + 0x0B4)
#define DE_MIXER0_BLD_KEY_MAX(x)      MEM(DE_MIXER0_BLD + 0x0C0 + x * 0x4)
#define DE_MIXER0_BLD_KEY_MIN(x)      MEM(DE_MIXER0_BLD + 0x0E0 + x * 0x4)
#define DE_MIXER0_BLD_OUT_COLOR       MEM(DE_MIXER0_BLD + 0x0FC)

// XXX: probably same offset on DE3, not 100% clear in docs
#ifdef AWBM_PLATFORM_h3
  #define DE_MIXER0_OVL_V	      (DE_MIXER0 + 0x2000)
#elif defined(AWBM_PLATFORM_h616)
  #define DE_MIXER0_OVL_V	      (DE_MIXER0 + 0x1000)
#endif

#define DE_MIXER0_OVL_V_ATTCTL(x)     MEM(DE_MIXER0_OVL_V + 0x00 + x * 0x30)
#define DE_MIXER0_OVL_V_MBSIZE(x)     MEM(DE_MIXER0_OVL_V + 0x04 + x * 0x30)
#define DE_MIXER0_OVL_V_COOR(x)       MEM(DE_MIXER0_OVL_V + 0x08 + x * 0x30)
#define DE_MIXER0_OVL_V_PITCH0(x)     MEM(DE_MIXER0_OVL_V + 0x0C + x * 0x30)
#define DE_MIXER0_OVL_V_PITCH1(x)     MEM(DE_MIXER0_OVL_V + 0x10 + x * 0x30)
#define DE_MIXER0_OVL_V_PITCH2(x)     MEM(DE_MIXER0_OVL_V + 0x14 + x * 0x30)
#define DE_MIXER0_OVL_V_TOP_LADD0(x)  MEM(DE_MIXER0_OVL_V + 0x18 + x * 0x30)
#define DE_MIXER0_OVL_V_TOP_LADD1(x)  MEM(DE_MIXER0_OVL_V + 0x1C + x * 0x30)
#define DE_MIXER0_OVL_V_TOP_LADD2(x)  MEM(DE_MIXER0_OVL_V + 0x20 + x * 0x30)
#define DE_MIXER0_OVL_V_BOT_LADD0(x)  MEM(DE_MIXER0_OVL_V + 0x24 + x * 0x30)
#define DE_MIXER0_OVL_V_BOT_LADD1(x)  MEM(DE_MIXER0_OVL_V + 0x28 + x * 0x30)
#define DE_MIXER0_OVL_V_BOT_LADD2(x)  MEM(DE_MIXER0_OVL_V + 0x2C + x * 0x30)
#define DE_MIXER0_OVL_V_FILL_COLOR(x) MEM(DE_MIXER0_OVL_V + 0xC0 + x * 0x4)
#define DE_MIXER0_OVL_V_TOP_HADD0     MEM(DE_MIXER0_OVL_V + 0xD0)
#define DE_MIXER0_OVL_V_TOP_HADD1     MEM(DE_MIXER0_OVL_V + 0xD4)
#define DE_MIXER0_OVL_V_TOP_HADD2     MEM(DE_MIXER0_OVL_V + 0xD8)
#define DE_MIXER0_OVL_V_BOT_HADD0     MEM(DE_MIXER0_OVL_V + 0xDC)
#define DE_MIXER0_OVL_V_BOT_HADD1     MEM(DE_MIXER0_OVL_V + 0xE0)
#define DE_MIXER0_OVL_V_BOT_HADD2     MEM(DE_MIXER0_OVL_V + 0xE4)
#define DE_MIXER0_OVL_V_SIZE          MEM(DE_MIXER0_OVL_V + 0xE8)

#ifdef AWBM_PLATFORM_h3
  #define DE_MIXER0_VS_BASE	      (DE_MIXER0 + 0x20000)
#elif defined(AWBM_PLATFORM_h616)
  #define DE_MIXER0_VS_BASE	      (DE_MIXER0 + 0x4000)
#endif

#define DE_MIXER0_VS_CTRL             MEM(DE_MIXER0_VS_BASE + 0x00)
#define DE_MIXER0_VS_STATUS           MEM(DE_MIXER0_VS_BASE + 0x08)
#define DE_MIXER0_VS_FIELD_CTRL       MEM(DE_MIXER0_VS_BASE + 0x0C)
#define DE_MIXER0_VS_OUT_SIZE         MEM(DE_MIXER0_VS_BASE + 0x40)
#define DE_MIXER0_VS_Y_SIZE           MEM(DE_MIXER0_VS_BASE + 0x80)
#define DE_MIXER0_VS_Y_HSTEP          MEM(DE_MIXER0_VS_BASE + 0x88)
#define DE_MIXER0_VS_Y_VSTEP          MEM(DE_MIXER0_VS_BASE + 0x8C)
#define DE_MIXER0_VS_Y_HPHASE         MEM(DE_MIXER0_VS_BASE + 0x90)
#define DE_MIXER0_VS_Y_VPHASE0        MEM(DE_MIXER0_VS_BASE + 0x98)
#define DE_MIXER0_VS_Y_VPHASE1        MEM(DE_MIXER0_VS_BASE + 0x9C)
#define DE_MIXER0_VS_C_SIZE           MEM(DE_MIXER0_VS_BASE + 0xC0)
#define DE_MIXER0_VS_C_HSTEP          MEM(DE_MIXER0_VS_BASE + 0xC8)
#define DE_MIXER0_VS_C_VSTEP          MEM(DE_MIXER0_VS_BASE + 0xCC)
#define DE_MIXER0_VS_C_HPHASE         MEM(DE_MIXER0_VS_BASE + 0xD0)
#define DE_MIXER0_VS_C_VPHASE0        MEM(DE_MIXER0_VS_BASE + 0xD8)
#define DE_MIXER0_VS_C_VPHASE1        MEM(DE_MIXER0_VS_BASE + 0xDC)
#define DE_MIXER0_VS_Y_HCOEF0(x)      MEM(DE_MIXER0_VS_BASE + 0x200 + x * 4)
#define DE_MIXER0_VS_Y_HCOEF1(x)      MEM(DE_MIXER0_VS_BASE + 0x300 + x * 4)
#define DE_MIXER0_VS_Y_VCOEF(x)       MEM(DE_MIXER0_VS_BASE + 0x400 + x * 4)
#define DE_MIXER0_VS_C_HCOEF0(x)      MEM(DE_MIXER0_VS_BASE + 0x600 + x * 4)
#define DE_MIXER0_VS_C_HCOEF1(x)      MEM(DE_MIXER0_VS_BASE + 0x700 + x * 4)
#define DE_MIXER0_VS_C_VCOEF(x)       MEM(DE_MIXER0_VS_BASE + 0x800 + x * 4)


#ifdef AWBM_PLATFORM_h3

// Mixer 1
#define DE_MIXER1                     (DE_BASE + 0x200000)
#define DE_MIXER1_GLB                 (DE_MIXER1 + 0x0)
#define DE_MIXER1_GLB_CTL             MEM(DE_MIXER1_GLB + 0x000)
#define DE_MIXER1_GLB_STS             MEM(DE_MIXER1_GLB + 0x004)
#define DE_MIXER1_GLB_DBUFFER         MEM(DE_MIXER1_GLB + 0x008)
#define DE_MIXER1_GLB_SIZE            MEM(DE_MIXER1_GLB + 0x00C)

#define DE_MIXER1_BLD                 (DE_MIXER1 + 0x1000)
#define DE_MIXER1_BLD_FILL_COLOR_CTL  MEM(DE_MIXER1_BLD + 0x000)
#define DE_MIXER1_BLD_FILL_COLOR(x)   MEM(DE_MIXER1_BLD + 0x004 + x * 0x10)
#define DE_MIXER1_BLD_CH_ISIZE(x)     MEM(DE_MIXER1_BLD + 0x008 + x * 0x10)
#define DE_MIXER1_BLD_CH_OFFSET(x)    MEM(DE_MIXER1_BLD + 0x00C + x * 0x10)
#define DE_MIXER1_BLD_CH_RTCTL        MEM(DE_MIXER1_BLD + 0x080)
#define DE_MIXER1_BLD_PREMUL_CTL      MEM(DE_MIXER1_BLD + 0x084)
#define DE_MIXER1_BLD_BK_COLOR        MEM(DE_MIXER1_BLD + 0x088)
#define DE_MIXER1_BLD_SIZE            MEM(DE_MIXER1_BLD + 0x08C)
#define DE_MIXER1_BLD_CTL(x)          MEM(DE_MIXER1_BLD + 0x090 + x * 0x4)
#define DE_MIXER1_BLD_KEY_CTL         MEM(DE_MIXER1_BLD + 0x0B0)
#define DE_MIXER1_BLD_KEY_CON         MEM(DE_MIXER1_BLD + 0x0B4)
#define DE_MIXER1_BLD_KEY_MAX(x)      MEM(DE_MIXER1_BLD + 0x0C0 + x * 0x4)
#define DE_MIXER1_BLD_KEY_MIN(x)      MEM(DE_MIXER1_BLD + 0x0E0 + x * 0x4)
#define DE_MIXER1_BLD_OUT_COLOR       MEM(DE_MIXER1_BLD + 0x0FC)

#define DE_MIXER1_OVL_V               (DE_MIXER1 + 0x2000)
#define DE_MIXER1_OVL_V_ATTCTL(x)     MEM(DE_MIXER1_OVL_V + 0x00 + x * 0x30)
#define DE_MIXER1_OVL_V_MBSIZE(x)     MEM(DE_MIXER1_OVL_V + 0x04 + x * 0x30)
#define DE_MIXER1_OVL_V_COOR(x)       MEM(DE_MIXER1_OVL_V + 0x08 + x * 0x30)
#define DE_MIXER1_OVL_V_PITCH0(x)     MEM(DE_MIXER1_OVL_V + 0x0C + x * 0x30)
#define DE_MIXER1_OVL_V_PITCH1(x)     MEM(DE_MIXER1_OVL_V + 0x10 + x * 0x30)
#define DE_MIXER1_OVL_V_PITCH2(x)     MEM(DE_MIXER1_OVL_V + 0x14 + x * 0x30)
#define DE_MIXER1_OVL_V_TOP_LADD0(x)  MEM(DE_MIXER1_OVL_V + 0x18 + x * 0x30)
#define DE_MIXER1_OVL_V_TOP_LADD1(x)  MEM(DE_MIXER1_OVL_V + 0x1C + x * 0x30)
#define DE_MIXER1_OVL_V_TOP_LADD2(x)  MEM(DE_MIXER1_OVL_V + 0x20 + x * 0x30)
#define DE_MIXER1_OVL_V_BOT_LADD0(x)  MEM(DE_MIXER1_OVL_V + 0x24 + x * 0x30)
#define DE_MIXER1_OVL_V_BOT_LADD1(x)  MEM(DE_MIXER1_OVL_V + 0x28 + x * 0x30)
#define DE_MIXER1_OVL_V_BOT_LADD2(x)  MEM(DE_MIXER1_OVL_V + 0x2C + x * 0x30)
#define DE_MIXER1_OVL_V_FILL_COLOR(x) MEM(DE_MIXER1_OVL_V + 0xC0 + x * 0x4)
#define DE_MIXER1_OVL_V_TOP_HADD0     MEM(DE_MIXER1_OVL_V + 0xD0)
#define DE_MIXER1_OVL_V_TOP_HADD1     MEM(DE_MIXER1_OVL_V + 0xD4)
#define DE_MIXER1_OVL_V_TOP_HADD2     MEM(DE_MIXER1_OVL_V + 0xD8)
#define DE_MIXER1_OVL_V_BOT_HADD0     MEM(DE_MIXER1_OVL_V + 0xDC)
#define DE_MIXER1_OVL_V_BOT_HADD1     MEM(DE_MIXER1_OVL_V + 0xE0)
#define DE_MIXER1_OVL_V_BOT_HADD2     MEM(DE_MIXER1_OVL_V + 0xE4)
#define DE_MIXER1_OVL_V_SIZE          MEM(DE_MIXER1_OVL_V + 0xE8)

#define DE_MIXER1_OVL_UI               (DE_MIXER1 + 0x3000)
#define DE_MIXER1_OVL_UI_ATTR_CTL(x)   *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x00 + x * 0x20)
#define DE_MIXER1_OVL_UI_MBSIZE(x)     *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x04 + x * 0x20)
#define DE_MIXER1_OVL_UI_COOR(x)       *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x08 + x * 0x20)
#define DE_MIXER1_OVL_UI_PITCH(x)      *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x0C + x * 0x20)
#define DE_MIXER1_OVL_UI_TOP_LADD(x)   *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x10 + x * 0x20)
#define DE_MIXER1_OVL_UI_BOT_LADD(x)   *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x14 + x * 0x20)
#define DE_MIXER1_OVL_UI_FILL_COLOR(x) *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x18 + x * 0x4)
#define DE_MIXER1_OVL_UI_TOP_HADD      *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x80)
#define DE_MIXER1_OVL_UI_BOT_HADD      *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x84)
#define DE_MIXER1_OVL_UI_SIZE          *(volatile uint32_t *)(DE_MIXER1_OVL_UI + 0x88)

#define DE_MIXER1_VS_BASE             (DE_MIXER1 + 0x20000)
#define DE_MIXER1_VS_CTRL             MEM(DE_MIXER1_VS_BASE + 0x00)
#define DE_MIXER1_VS_STATUS           MEM(DE_MIXER1_VS_BASE + 0x08)
#define DE_MIXER1_VS_FIELD_CTRL       MEM(DE_MIXER1_VS_BASE + 0x0C)
#define DE_MIXER1_VS_OUT_SIZE         MEM(DE_MIXER1_VS_BASE + 0x40)
#define DE_MIXER1_VS_Y_SIZE           MEM(DE_MIXER1_VS_BASE + 0x80)
#define DE_MIXER1_VS_Y_HSTEP          MEM(DE_MIXER1_VS_BASE + 0x88)
#define DE_MIXER1_VS_Y_VSTEP          MEM(DE_MIXER1_VS_BASE + 0x8C)
#define DE_MIXER1_VS_Y_HPHASE         MEM(DE_MIXER1_VS_BASE + 0x90)
#define DE_MIXER1_VS_Y_VPHASE0        MEM(DE_MIXER1_VS_BASE + 0x98)
#define DE_MIXER1_VS_Y_VPHASE1        MEM(DE_MIXER1_VS_BASE + 0x9C)
#define DE_MIXER1_VS_C_SIZE           MEM(DE_MIXER1_VS_BASE + 0xC0)
#define DE_MIXER1_VS_C_HSTEP          MEM(DE_MIXER1_VS_BASE + 0xC8)
#define DE_MIXER1_VS_C_VSTEP          MEM(DE_MIXER1_VS_BASE + 0xCC)
#define DE_MIXER1_VS_C_HPHASE         MEM(DE_MIXER1_VS_BASE + 0xD0)
#define DE_MIXER1_VS_C_VPHASE0        MEM(DE_MIXER1_VS_BASE + 0xD8)
#define DE_MIXER1_VS_C_VPHASE1        MEM(DE_MIXER1_VS_BASE + 0xDC)
#define DE_MIXER1_VS_Y_HCOEF0(x)      MEM(DE_MIXER1_VS_BASE + 0x200 + x * 4)
#define DE_MIXER1_VS_Y_HCOEF1(x)      MEM(DE_MIXER1_VS_BASE + 0x300 + x * 4)
#define DE_MIXER1_VS_Y_VCOEF(x)       MEM(DE_MIXER1_VS_BASE + 0x400 + x * 4)
#define DE_MIXER1_VS_C_HCOEF0(x)      MEM(DE_MIXER1_VS_BASE + 0x600 + x * 4)
#define DE_MIXER1_VS_C_HCOEF1(x)      MEM(DE_MIXER1_VS_BASE + 0x700 + x * 4)
#define DE_MIXER1_VS_C_VCOEF(x)       MEM(DE_MIXER1_VS_BASE + 0x800 + x * 4)

#define DE_MIXER1_UIS_BASE(n)         (DE_MIXER1 + 0x40000 + 0x10000 * (n))
#define DE_MIXER1_UIS_CTRL(n)         MEM(DE_MIXER1_UIS_BASE(n) + 0x00)
#define DE_MIXER1_UIS_STATUS(n)       MEM(DE_MIXER1_UIS_BASE(n) + 0x08)
#define DE_MIXER1_UIS_FIELD_CTRL(n)   MEM(DE_MIXER1_UIS_BASE(n) + 0x0C)
#define DE_MIXER1_UIS_OUT_SIZE(n)     MEM(DE_MIXER1_UIS_BASE(n) + 0x40)
#define DE_MIXER1_UIS_IN_SIZE(n)      MEM(DE_MIXER1_UIS_BASE(n) + 0x80)
#define DE_MIXER1_UIS_HSTEP(n)        MEM(DE_MIXER1_UIS_BASE(n) + 0x88)
#define DE_MIXER1_UIS_VSTEP(n)        MEM(DE_MIXER1_UIS_BASE(n) + 0x8C)
#define DE_MIXER1_UIS_HPHASE(n)       MEM(DE_MIXER1_UIS_BASE(n) + 0x90)
#define DE_MIXER1_UIS_VPHASE0(n)      MEM(DE_MIXER1_UIS_BASE(n) + 0x98)
#define DE_MIXER1_UIS_VPHASE1(n)      MEM(DE_MIXER1_UIS_BASE(n) + 0x9C)
#define DE_MIXER1_UIS_HCOEF(n, x)     MEM(DE_MIXER1_UIS_BASE(n) + 0x200 + x * 4)

#endif // AWBM_PLATFORM_h3

#define DE_SIZE(x, y) ((((y)-1) << 16) | ((x)-1))
#define DE_SIZE_PHYS  DE_SIZE(DISPLAY_PHYS_RES_X, DISPLAY_PHYS_RES_Y)

#ifdef AWBM_PLATFORM_h3

#define DE_WB                         (DE_BASE + 0x10000)

#define DE_WB_GCTRL        MEM(DE_WB + 0x00)
#define DE_WB_SIZE         MEM(DE_WB + 0x04)
#define DE_WB_A_CH0_ADDR   MEM(DE_WB + 0x10)
#define DE_WB_A_CH1_ADDR   MEM(DE_WB + 0x14)
#define DE_WB_B_CH0_ADDR   MEM(DE_WB + 0x20)
#define DE_WB_B_CH1_ADDR   MEM(DE_WB + 0x24)
#define DE_WB_CH0_PITCH    MEM(DE_WB + 0x30)
#define DE_WB_CH12_PITCH   MEM(DE_WB + 0x34)
#define DE_WB_ADDR_SWITCH  MEM(DE_WB + 0x40)
#define DE_WB_FORMAT       MEM(DE_WB + 0x44)
#define DE_WB_CH0_HCOEF(n) MEM(DE_WB + 0x200 + (n) * 4)
#define DE_WB_CH1_HCOEF(n) MEM(DE_WB + 0x280 + (n) * 4)
#define DE_WB_STATUS       MEM(DE_WB + 0x4C)
#define DE_WB_CROP_SIZE    MEM(DE_WB + 0x0C)
#define DE_WB_BYPASS       MEM(DE_WB + 0x54)
#define DE_WB_CS_HORZ      MEM(DE_WB + 0x70)
#define DE_WB_CS_VERT      MEM(DE_WB + 0x74)
#define DE_WB_FS_INSIZE    MEM(DE_WB + 0x80)
#define DE_WB_FS_OUTSIZE   MEM(DE_WB + 0x84)
#define DE_WB_FS_HSTEP     MEM(DE_WB + 0x88)
#define DE_WB_FS_VSTEP     MEM(DE_WB + 0x8C)
#define DE_WB_INT          MEM(DE_WB + 0x48)

#endif	// AWBM_PLATFORM_h3

void display_set_mode(int x, int y, int ovx, int ovy);
void display_swap_buffers();
void display_clear_active_buffer(void);
void display_enable_filter(int onoff);

extern int display_single_buffer;

extern volatile uint32_t *display_active_buffer;
extern volatile uint32_t *display_visible_buffer;

void display_scaler_set_coeff(uint32_t hstep, int sub);
void display_scaler_nearest_neighbour(void);

struct display_phys_mode_t {
  int pixclk;

  int hactive;
  int hfront_porch;
  int hsync_width;
  int hback_porch;
  int hsync_pol;

  int vactive;
  int vfront_porch;
  int vsync_width;
  int vback_porch;
  int vsync_pol;

  int hdmi;
};

void hook_display_vblank(void);

int display_init(const struct display_phys_mode_t *mode);

void display_capture_init(int x, int y);
void display_capture_set_out_bufs(void *luma_buf, void *chroma_buf);
void display_capture_kick(void);
void display_capture_stop(void);
int display_capture_frame_ready(void **luma_buf, void **chroma_buf);
void display_capture_ack_frame(void);

void display_g2d_blit(void *src, int src_total_w, int src_total_h, int src_x, int src_y, int src_w, int src_h, void *dst, int dst_total_w, int dst_total_h, int dst_x, int dst_y, int dst_w, int dst_h);

#ifdef __cplusplus
}
#endif

#endif
