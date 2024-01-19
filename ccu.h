#ifdef __cplusplus
extern "C" {
#endif

#include "util.h"

#ifdef AWBM_PLATFORM_h3

// The CCU registers base address.
#define CCU_BASE 0x01C20000

// Structure of CCU registers.
#define PLL_CPUX_CTRL         MEM(CCU_BASE + 0X000)
#define PLL_AUDIO_CTRL        MEM(CCU_BASE + 0X008)
#define PLL_VIDEO_CTRL        MEM(CCU_BASE + 0X010)
#define PLL_VE_CTRL           MEM(CCU_BASE + 0X018)
#define PLL_DDR_CTRL          MEM(CCU_BASE + 0X020)
#define PLL_PERIPH0_CTRL      MEM(CCU_BASE + 0X028)
#define PLL_GPU_CTRL          MEM(CCU_BASE + 0X038)
#define PLL_PERIPH1_CTRL      MEM(CCU_BASE + 0X044)
#define PLL_DE_CTRL           MEM(CCU_BASE + 0X048)
#define CPUX_AXI_CFG          MEM(CCU_BASE + 0X050)
#define AHB1_APB1_CFG         MEM(CCU_BASE + 0X054)
#define APB2_CFG              MEM(CCU_BASE + 0X058)
#define AHB2_CFG              MEM(CCU_BASE + 0X05C)
#define BUS_CLK_GATING0       MEM(CCU_BASE + 0X060)
#define BUS_CLK_GATING1       MEM(CCU_BASE + 0X064)
#define BUS_CLK_GATING2       MEM(CCU_BASE + 0X068)
#define BUS_CLK_GATING3       MEM(CCU_BASE + 0X06C)
#define BUS_CLK_GATING4       MEM(CCU_BASE + 0X070)
#define THS_CLK               MEM(CCU_BASE + 0X074)
#define NAND_CLK              MEM(CCU_BASE + 0X080)
#define SDMMC0_CLK            MEM(CCU_BASE + 0X088)
#define SDMMC1_CLK            MEM(CCU_BASE + 0X08C)
#define SDMMC2_CLK            MEM(CCU_BASE + 0X090)
#define CE_CLK                MEM(CCU_BASE + 0X09C)
#define SPI0_CLK              MEM(CCU_BASE + 0X0A0)
#define SPI1_CLK              MEM(CCU_BASE + 0X0A4)
#define I2S_PCM0_CLK          MEM(CCU_BASE + 0X0B0)
#define I2S_PCM1_CLK          MEM(CCU_BASE + 0X0B4)
#define I2S_PCM2_CLK          MEM(CCU_BASE + 0X0B8)
#define OWA_CLK               MEM(CCU_BASE + 0X0C0)
#define USBPHY_CFG            MEM(CCU_BASE + 0X0CC)
#define DRAM_CFG              MEM(CCU_BASE + 0X0F4)
#define MBUS_RST              MEM(CCU_BASE + 0X0FC)
#define DRAM_CLK_GATING       MEM(CCU_BASE + 0X100)
#define DE_CLK                MEM(CCU_BASE + 0X104)
#define TCON0_CLK             MEM(CCU_BASE + 0X118)
#define TVE_CLK               MEM(CCU_BASE + 0X120)
#define DEINTERLACE_CLK       MEM(CCU_BASE + 0X124)
#define CSI_MISC_CLK          MEM(CCU_BASE + 0X130)
#define CSI_CLK               MEM(CCU_BASE + 0X134)
#define VE_CLK                MEM(CCU_BASE + 0X13C)
#define AC_DIG_CLK            MEM(CCU_BASE + 0X140)
#define AVS_CLK               MEM(CCU_BASE + 0X144)
#define HDMI_CLK              MEM(CCU_BASE + 0X150)
#define HDMI_SLOW_CLK         MEM(CCU_BASE + 0X154)
#define MBUS_CLK              MEM(CCU_BASE + 0X15C)
#define GPU_CLK               MEM(CCU_BASE + 0X1A0)
#define PLL_STABLE_TIME0      MEM(CCU_BASE + 0X200)
#define PLL_STABLE_TIME1      MEM(CCU_BASE + 0X204)
#define PLL_CPUX_BIAS         MEM(CCU_BASE + 0X220)
#define PLL_AUDIO_BIAS        MEM(CCU_BASE + 0X224)
#define PLL_VIDEO_BIAS        MEM(CCU_BASE + 0X228)
#define PLL_VE_BIAS           MEM(CCU_BASE + 0X22C)
#define PLL_DDR_BIAS          MEM(CCU_BASE + 0X230)
#define PLL_PERIPH0_BIAS      MEM(CCU_BASE + 0X234)
#define PLL_GPU_BIAS          MEM(CCU_BASE + 0X23C)
#define PLL_PERIPH1_BIAS      MEM(CCU_BASE + 0X244)
#define PLL_DE_BIAS           MEM(CCU_BASE + 0X248)
#define PLL_CPUX_TUN          MEM(CCU_BASE + 0X250)
#define PLL_DDR_TUN           MEM(CCU_BASE + 0X260)
#define PLL_CPUX_PAT_CTRL     MEM(CCU_BASE + 0X280)
#define PLL_AUDIO_PAT_CTRL0   MEM(CCU_BASE + 0X284)
#define PLL_VIDEO_PAT_CTRL0   MEM(CCU_BASE + 0X288)
#define PLL_VE_PAT_CTRL       MEM(CCU_BASE + 0X28C)
#define PLL_DDR_PAT_CTRL0     MEM(CCU_BASE + 0X290)
#define PLL_GPU_PAT_CTRL      MEM(CCU_BASE + 0X29C)
#define PLL_PERIPH1_PAT_CTRL1 MEM(CCU_BASE + 0X2A4)
#define PLL_DE_PAT_CTRL       MEM(CCU_BASE + 0X2A8)
#define BUS_SOFT_RST0         MEM(CCU_BASE + 0X2C0)
#define BUS_SOFT_RST1         MEM(CCU_BASE + 0X2C4)
#define BUS_SOFT_RST2         MEM(CCU_BASE + 0X2C8)
#define BUS_SOFT_RST3         MEM(CCU_BASE + 0X2D0)
#define BUS_SOFT_RST4         MEM(CCU_BASE + 0X2D8)
#define CCU_SEC_SWITCH        MEM(CCU_BASE + 0X2F0)
#define PS_CTRL               MEM(CCU_BASE + 0X300)
#define PS_CNT                MEM(CCU_BASE + 0X304)

#define R_PRCM_BASE 0x01F01400
#define APB0_CLK_GATING       MEM(R_PRCM_BASE + 0x28)

#define PLL_CPUX_FACTOR_K_SHIFT 4
#define PLL_CPUX_FACTOR_N_SHIFT 8

#define PLL_CPUX_FACTOR_K_MASK	0x00000030UL
#define PLL_CPUX_FACTOR_N_MASK  0x00001f00UL

#elif defined(AWBM_PLATFORM_h616)

#define CCU_BASE 0x03001000

#define UART_BGR_REG		MEM(CCU_BASE + 0x90c)

#else
#error unknown platform
#endif

#ifdef __cplusplus
}
#endif
