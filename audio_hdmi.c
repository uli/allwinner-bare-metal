#include "audio.h"
#include "system.h"

void audio_hdmi_init(void)
{
	// Audio setup borrowed from Allwinner vendor HDMI driver for Linux.
	// Register addresses have been "descrambled" (see
	// https://linux-sunxi.org/DWC_HDMI_Controller for details).

	HDMI_FC_AUDSCONF = 0xF0;
	HDMI_FC_AUDSV = 0xEE;
	HDMI_FC_AUDSU = 0x00;
	HDMI_FC_AUDSCHNL0 = 0x30;
	HDMI_FC_AUDSCHNL1 = 0x00;
	HDMI_FC_AUDSCHNL2 = 0x01;
	HDMI_FC_AUDSCHNL3 = 0x42;
	HDMI_FC_AUDSCHNL4 = 0x86;
	HDMI_FC_AUDSCHNL5 = 0x31;
	HDMI_FC_AUDSCHNL6 = 0x75;
	// Sampling frequency (unit?)
	HDMI_FC_AUDSCHNL7 = 0x01;
	HDMI_FC_AUDSCHNL7 = 0x02;	// yes, twice
	HDMI_FC_AUDSCHNL8 = 0x02;

	HDMI_AUD_CONF1 = 0x10;

	/* Table of "n" values for various sample rates:
	   32000,		3072,		4096,
	   44100,		4704,		6272,
	   88200,		4704*2,		6272*2,
	   176400,		4704*4,		6272*4,
	   48000,		5120,		6144,
	   96000,		5120*2,		6144*2,
	   192000,		5120*4,		6144*4,
	 */
	// 48 kHz -> n = 6144
	HDMI_AUD_N1 = 0x00;
	HDMI_AUD_N2 = 0x18;
	HDMI_AUD_N3 = 0x00;
	HDMI_AUD_CTS3 = 0x00;
	HDMI_AUD_INPUTCLKFS = 0x04;

	HDMI_FC_AUDSCONF = 0x00;
	HDMI_FC_AUDIOCONF0 = 0x10;
	HDMI_FC_AUDIOCONF1 = 0x00;
	HDMI_FC_AUDIOCONF2 = 0x00;
	HDMI_FC_AUDIOCONF3 = 0x00;

	HDMI_AUD_CONF2 = 0x00;
	HDMI_AUD_CONF0 = 0x00;

	HDMI_MC_CLKDIS = 0x74 | 0x08;
	HDMI_MC_SWRSTZREQ = 0xF7;
	udelay(200);
	HDMI_AUD_CONF0 = 0xAF;
	udelay(200);
	HDMI_MC_CLKDIS = 0x74 | 0x00;
}
