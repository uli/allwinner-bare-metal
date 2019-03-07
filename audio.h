#include "display.h"

void audio_hdmi_init(void);
void audio_i2s2_init(void);
void audio_i2s2_on(void);

#define I2S_BASE_BASE	0x01c22000
#define I2S_BASE(n)	(I2S_BASE_BASE + (n) * 0x400)

#define I2S_REG(n, reg)	*(volatile uint32_t*)(I2S_BASE(n) + (reg))

#define I2S_CTL(n)		I2S_REG(n, 0x00)
#define I2S_CTL_BCLK_OUT	(1 << 18)
#define I2S_CTL_LRCK_OUT	(1 << 17)
#define I2S_CTL_LRCKR_OUT	(1 << 16)
#define I2S_CTL_SDO0_EN		(1 << 8)
#define I2S_CTL_MODE_SEL(n)	((n) << 4)
#define I2S_CTL_TXEN		(1 << 2)
#define I2S_CTL_RXEN		(1 << 1)
#define I2S_CTL_GEN		(1 << 0)

#define I2S_FAT0(n)		I2S_REG(n, 0x04)
#define I2S_FAT0_SW(n)		((n) << 0)
#define I2S_FAT0_SR(n)		((n) << 4)
#define I2S_FAT0_LRCK_PERIOD(n)	((n) << 8)
#define I2S_FAT0_LRCKR_PERIOD(n) ((n) << 20)

#define I2S_FAT1(n)		I2S_REG(n, 0x08)

#define I2S_FCTL(n)		I2S_REG(n, 0x14)
#define I2S_FCTL_TXTL(n)	((n) << 12)
#define I2S_FCTL_RXTL(n)	((n) << 4)
#define I2S_FCTL_TXIM(n)	((n) << 2)
#define I2S_FCTL_RXOM(n)	((n) << 0)
#define I2S_FCTL_FRX		(1 << 24)
#define I2S_FCTL_FTX		(1 << 25)

#define I2S_FSTA(n)		I2S_REG(n, 0x18)
#define I2S_FSTA_TXE		(1 << 28)

#define I2S_TXFIFO(n)		I2S_REG(n, 0x20)
#define I2S_INT(n)		I2S_REG(n, 0x1c)

#define I2S_CLKD(n)		I2S_REG(n, 0x24)
#define I2S_CLKD_MCLKDIV(n)	((n) << 0)
#define I2S_CLKD_BCLKDIV(n)	((n) << 4)
#define I2S_CLKD_MCLKO_EN	(1 << 8)

#define I2S_TXCNT(n)		I2S_REG(n, 0x28)
#define I2S_RXCNT(n)		I2S_REG(n, 0x2c)

#define I2S_TXCHCFG_H3(n)	I2S_REG(n, 0x30)
#define I2S_CHCFG_RX_SLOT_NUM(n)	((n) << 4)
#define I2S_CHCFG_TX_SLOT_NUM(n)	((n) << 0)

#define I2S_TX0CHSEL_H3(n)	I2S_REG(n, 0x34)
#define I2S_TXnCHSEL_OFFSET_1	(1 << 12)
#define I2S_TXnCHSEL_CHSEL_2	(1 << 0)
#define I2S_TXnCHSEL_CHEN_0	(1 << 4)
#define I2S_TXnCHSEL_CHEN_1	(1 << 5)

#define I2S_TX0CHMAP_H3(n)	I2S_REG(n, 0x44)
#define I2S_TXnCHMAP_CH0_SAMPLE(n)	((n) << 0)
#define I2S_TXnCHMAP_CH1_SAMPLE(n)	((n) << 4)

#define I2S_RXCHSEL(n)		I2S_REG(n, 0x54)
#define I2S_RXCHSEL_OFFSET(n)	((n) << 12)
#define I2S_RXCHSEL_CHSEL(n)	((n) << 0)

#define I2S_RXCHMAP(n)		I2S_REG(n, 0x58)
#define I2S_RXCHMAP_CH_SAMPLE(ch, n)	((n) << ((ch) * 4))
