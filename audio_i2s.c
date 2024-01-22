#include "audio.h"
#include "ccu.h"
#include "system.h"
#include "interrupts.h"
#include <stdio.h>
#include <h3_codec.h>

int sample_count;

void __attribute__((weak)) hook_audio_get_sample(int16_t *l, int16_t *r)
{
	// Create a pilot tone.
	*l = ((sample_count / 50) & 1) * 0x007f;
	*r = ((sample_count / 50) & 1) * 0x007f;
	sample_count++;
}

void audio_queue_samples(void)
{
#ifdef AWBM_PLATFORM_h3
	while (((I2S_FSTA(2) >> 16) & 0xff) > 2) {
		int16_t l, r;
		hook_audio_get_sample(&l, &r);
		I2S_TXFIFO(2) = l;
		I2S_TXFIFO(2) = r;
	}
	// TXEI clears itself when the FIFO is full again
#elif defined(AWBM_PLATFORM_h616)
	while ((AHUB_APBIF_TXFIFO_STS(1) & 0x7f) > 2) {
		int16_t l, r;
		hook_audio_get_sample(&l, &r);

		// Feed AHUB and analog codec FIFOs.
		// We are running this off the AHUB interrupt. We can do
		// that because the AHUB and the codec run off clocks
		// derived from the same source (they are, aren't they?).
		// The audio codec also has a larger FIFO than the AHUB (128
		// vs. 64 samples).

		// XXX: find out the TXDIF no. instead of hardcoding it
		AHUB_APBIF_TXFIFO(1) = l;
		AC_DAC_TXDATA = l;
		AHUB_APBIF_TXFIFO(1) = r;
		AC_DAC_TXDATA = r;
	}
	// TXnE clears itself when the FIFO is no longer empty.
#else
#warning unimplemented
#endif
}

#ifdef AWBM_PLATFORM_h616

#define DMA_BASE		0x3002000
#define DMA_CHAN_BASE(n)	(DMA_BASE + (n) * 0x40)
#define DMA_PAUSE(n)		MEM(DMA_CHAN_BASE(n) + 0x104)
#define DMA_CUR_DEST_REG(n)	MEM(DMA_CHAN_BASE(n) + 0x114)

void audio_ahub_init(void)
{
	// Hijack AHUB and codec by pausing DMA.

	// We assume that the Linux cell has brought the devices up and
	// keeps them running.
	// Linux uses DMA to feed the AHUB and codec FIFOs. We cannot hijack
	// the DMA controller because Linux needs it for other things.
	// Instead, we pause the DMA channels that are feeding the FIFOs and
	// use the AHUB's unused IRQ to feed them manually.

	for (int ch = 0; ch < 16; ++ch) {
		if ((DMA_CUR_DEST_REG(ch) & 0xfffff000) == AHUB_BASE_BASE) {
			DMA_PAUSE(ch) |= 1;
			// XXX: find out the TXDIF no. instead of hardcoding it
		}
		if ((DMA_CUR_DEST_REG(ch) & 0xfffff000) == AC_BASE) {
			DMA_PAUSE(ch) |=1;
		}
	}

	AHUB_APBIF_TXIRQ_CTRL(1) |= 1;	// enable TXnEI_EN (FIFO empty interrupt)

	irq_enable(HDMI_AUDIO_IRQ);
}
#endif

#ifdef AWBM_PLATFORM_h3

void audio_i2s2_init(void)
{
	// enable audio PLL, default value (24.571 MHz)
	// XXX: should be 24.576 MHz for 48 kHz
	PLL_AUDIO_CTRL = 0x80035514;
	while (!(PLL_AUDIO_CTRL & (1 << 28))) {
		// wait
	}

	// enable clock, source PLL_AUDIO (no multiplier)
	// XXX: correct?
	I2S_PCM2_CLK = 0x80030000;
	BUS_SOFT_RST3 &= ~(1 << 14);	// assert reset
	udelay(100);
	BUS_SOFT_RST3 |= 1 << 14;	// deassert reset
	udelay(100);
	BUS_CLK_GATING2 |= (1 << 14);	// pass clock
	udelay(100);
}

void audio_i2s2_on(void)
{
	I2S_TX0CHSEL_H3(2) = I2S_TXnCHSEL_OFFSET_1 | I2S_TXnCHSEL_CHSEL_2 |
			     I2S_TXnCHSEL_CHEN_0 | I2S_TXnCHSEL_CHEN_1; // 0x00001031

	I2S_TX0CHMAP_H3(2) = I2S_TXnCHMAP_CH0_SAMPLE(0) | I2S_TXnCHMAP_CH1_SAMPLE(1);

	// Set clock dividers.
	// BCLK/8 is good for both 44.1 and 48 kHz, must set input
	// clock accordingly (22.5792 or 24.576 MHz).
	I2S_CLKD(2) = I2S_CLKD_MCLKDIV(1) | I2S_CLKD_BCLKDIV(5) | I2S_CLKD_MCLKO_EN; // 0x00000151

	// TX/RX mode PCM, no sign extension
	I2S_FAT1(2) = 0;	// 0x00000000

	/* "normal bit clock + frame" (BCLK, LRCK polarity 0) */
	// LRCK period 32 BCLKs, LRCKR period 1 BCLK
	// slot width and sample resolution 16 bits
	I2S_FAT0(2) = I2S_FAT0_SW(3) | I2S_FAT0_SR(3) |
		      I2S_FAT0_LRCK_PERIOD(31) | I2S_FAT0_LRCKR_PERIOD(0);	// 0x00001F33

	I2S_TXCHCFG_H3(2) = I2S_CHCFG_RX_SLOT_NUM(1) | I2S_CHCFG_TX_SLOT_NUM(1);

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
	I2S_INT(2) = I2S_INT_TXEI_EN;		// enable TX FIFO empty IRQ
	irq_enable(HDMI_AUDIO_IRQ);

	// BCLK_OUT | LRCK_OUT: "codec clk & frm slave,ap is master"
	// MODE_SEL(1): "Left mode (offset 0: LJ mode; offset 1: I2S mode"
	I2S_CTL(2) = I2S_CTL_BCLK_OUT | I2S_CTL_LRCK_OUT | I2S_CTL_MODE_SEL(1) |
		     I2S_CTL_RXEN | I2S_CTL_TXEN | I2S_CTL_GEN | I2S_CTL_SDO0_EN;	// 0x00060117
}

void audio_i2s2_off(void)
{
	irq_disable(HDMI_AUDIO_IRQ);

	// disable RX
	I2S_FCTL(2) |= I2S_FCTL_FRX;	// flush RX FIFO
	I2S_RXCNT(2) = 0;		// clear RX count
	// don't have to disable DRQ, we don't enable it
	I2S_INT(2) = 0;

	// XXX: should we do a flush?
	// XXX: hub enable/disable? Not enabled in register trace.
}

#else	// AWBM_PLATFORM_h3
#warning unimplemented
#endif	// AWBM_PLATFORM_h3

void audio_start(int buf_len)
{
#ifdef AWBM_PLATFORM_h3
  h3_codec_begin();
  h3_codec_set_buffer_length(buf_len);

  audio_i2s2_init();

  audio_i2s2_on();
  h3_codec_start();
#elif defined(AWBM_PLATFORM_h616)
  audio_ahub_init();
#else
#warning unimplemented
#endif
}
