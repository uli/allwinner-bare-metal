#include "audio.h"
#include "ccu.h"

void audio_i2s2_init(void)
{
	// enable audio PLL, default value (24.571 MHz)
	PLL_AUDIO_CTRL = 0x810d0d00;
	while (!(PLL_AUDIO_CTRL & (1 << 28))) {
		// wait
	}

	// enable clock, source PLL_AUDIO (no multiplier)
	// XXX: correct?
	I2S_PCM2_CLK = 0x80030000;
	BUS_SOFT_RST3 &= ~(1 << 14);
	udelay(100);
	BUS_SOFT_RST3 |= 1 << 14;
	udelay(100);
	BUS_CLK_GATING2 |= (1 << 14);
	udelay(100);
}

void audio_i2s2_on(void)
{
	I2S_TX0CHSEL_H3(2) = I2S_TXnCHSEL_OFFSET_1 | I2S_TXnCHSEL_CHSEL_2 |
			     I2S_TXnCHSEL_CHEN_0 | I2S_TXnCHSEL_CHEN_1; // 0x00001031
	I2S_TX0CHMAP_H3(2) = I2S_TXnCHMAP_CH0_SAMPLE(0) | I2S_TXnCHMAP_CH1_SAMPLE(1); // 0x00000010

	// Set clock dividers.
	// BCLK/8 is good for both 44.1 and 48 kHz, must set input
	// clock accordingly (22.5792 or 24.576 MHz).
	I2S_CLKD(2) = I2S_CLKD_MCLKDIV(1) | I2S_CLKD_BCLKDIV(5) | I2S_CLKD_MCLKO_EN; // 0x00000151

	// TX/RX mode PCM, no sign extension
	I2S_FAT1(2) = 0;

	/* "normal bit clock + frame" (BCLK, LRCK polarity 0) */
	// LRCK period 32 BCLKs, LRCKR period 1 BCLK
	// slot width and sample resolution 16 bits
	I2S_FAT0(2) = I2S_FAT0_SW(3) | I2S_FAT0_SR(3) |
		      I2S_FAT0_LRCK_PERIOD(31) | I2S_FAT0_LRCKR_PERIOD(0);	// 0x00001F33

	I2S_TXCHCFG_H3(2) = I2S_CHCFG_RX_SLOT_NUM(1) | I2S_CHCFG_TX_SLOT_NUM(1);// 0x00000011

	I2S_RXCHSEL(2) = I2S_RXCHSEL_OFFSET(1) | I2S_RXCHSEL_CHSEL(1);	// 0x00001001
	I2S_RXCHMAP(2) = I2S_RXCHMAP_CH_SAMPLE(0, 0) | I2S_RXCHMAP_CH_SAMPLE(1, 1); // 0x00000010

	// TX trigger level 0x40
	// RX trigger level 0x0f
	// TX FIFO input mode 1 (valid data aligned with LSB of sample)
	// RX FIFO output mode 1
	// ("Expanding received sample sign bit at MSB of OWA_RXFIFO register")
	I2S_FCTL(2) = I2S_FCTL_TXTL(0x40) | I2S_FCTL_RXTL(0x0f) |
		      I2S_FCTL_TXIM(1) | I2S_FCTL_RXOM(1) |
		      I2S_FCTL_FRX;	// 0x010400F5

	I2S_RXCNT(2) = 0;	// clear RX counter
	I2S_TXCNT(2) = 0;	// clear TX counter
	I2S_INT(2) = 0;		// disable all DMA and interrupt requests

	// BCLK_OUT | LRCK_OUT: "codec clk & frm slave,ap is master"
	// MODE_SEL(1): "Left mode (offset 0: LJ mode; offset 1: I2S mode"
	I2S_CTL(2) = I2S_CTL_BCLK_OUT | I2S_CTL_LRCK_OUT | I2S_CTL_MODE_SEL(1) |
		     I2S_CTL_RXEN | I2S_CTL_TXEN | I2S_CTL_GEN | I2S_CTL_SDO0_EN;	// 0x00060117
}

void audio_i2s2_off(void)
{
	// disable RX
	I2S_FCTL(2) |= I2S_FCTL_FRX;	// flush RX FIFO
	I2S_RXCNT(2) = 0;		// clear RX count
	// don't have to disable DRQ, we don't enable it

	// disable TX
	// don't have to disable DRQ, we don't enable it
	// XXX: doesn't do a flush for some reason

	// XXX: hub enable/disable? Not enabled in register trace.
}
