 /*
  * s3c64xx-wm8753.c  --  Samsung audio driver for WM8753
  * "$Id: s3c6400_i2s.c,v 1.6 2008/01/31 07:22:14 eyryu Exp $" 
  *
  * Copyright 2003 Wolfson Microelectronics PLC.
  * Copyright (C) 2007, Ryu Euiyoul <ryu.real@gmail.com>
  * Author: Liam Girdwood
  *         liam.girdwood@wolfsonmicro.com or linux@wolfsonmicro.com
  *  This program is free software; you can redistribute  it and/or modify it
  *  under  the terms of  the GNU General  Public License as published by the
  *  Free Software Foundation;  either version 2 of the  License, or (at your
  *  option) any later version.
  *
  * Notes:
  *  The WM8753 is a low power, high quality stereo codec with integrated PCM
  *  codec designed for portable digital telephony applications.
  *  
  *  Features:
  *       - supports 16 bit mono voice DAC and left ADC in PCM mode
  *       - support VDAC sample rates of 8k, 12k, 16k, 24k, 48k.
  *       - support 16 bit stereo playback on HiFi DAC.
  *       - support HiFi DAC sample rates of 8k, 12k, 16k, 48k.
  *       - low power consumption 
  *       - support 8753 stand alone and 8753/9712 combi boards
  *       - workaround for bug#1190412 on Bulverde B0
  *
  *  TODO:
  *       - Test PM, IOCTL's, recording
  *       - TEST, TEST and more TESTING and then fix the bugs !
  *
  *  Bugs:
  *       - SSP2 RX is held high and not working
  *
  *  Revision history
  *    26th Aug 2003   Initial version.
  *     1st Dec 2003   Version 0.5. Added new sample rates and cleaned up.
  *    21st Jan 2004   Audio distortion workaround added, power settings
  *                    have been refined to minimize consumption.
  *     2nd Feb 2004   HiFi DAC added. 
  *     9th Dec 2004   Ported to 2.6.
  *    10th Jan 2005   Modified for s3c2442
  *    28th Mar 2007   Support at smdk6400 by ryu
  */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/pm.h>
#include <linux/platform_device.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/io.h>
#include <asm/dma.h>

#include <asm/arch/regs-iis.h>
#include <asm/arch/regs-s3c6400-clock.h>
#include <asm/arch/regs-gpio.h>
#include "wm8753.h"
#include "s3c6400_pcm.h"

#define AUDIO_NAME "s3c64xx-wm8753"

/*
 * HiFi DAC.
 * 
 * Set hifi to 1 to enable the HiFi DAC over the I2S link.
 */
static int hifi = 1;
module_param(hifi, int, 0);
MODULE_PARM_DESC(hifi, "Enable the HiFi DAC.");

/*
 * Voice ADC/DAC
 * 
 * Set voice to 1 to enable the voice ADC/DAC over the SSP link.
 */
static int voice = 1;
module_param(voice, int, 0);
MODULE_PARM_DESC(voice, "Enable the Voice ADC/DAC.");

/* OSS interface to WM8753 */
#define WM8753_STEREO_MASK (SOUND_MASK_VOLUME | SOUND_MASK_PCM | SOUND_MASK_IGAIN)

#define WM8753_SUPPORTED_MASK (WM8753_STEREO_MASK | \
    SOUND_MASK_BASS | SOUND_MASK_TREBLE | SOUND_MASK_MIC)
#define WM8753_RECORD_MASK (SOUND_MASK_MIC | SOUND_MASK_IGAIN)

/* taken from ac97_codec.h */
/* original check is not good enough in case FOO is greater than
 * SOUND_MIXER_NRDEVICES because the supported_mixers has exactly
 * SOUND_MIXER_NRDEVICES elements.
 * before matching the given mixer against the bitmask in supported_mixers we
 * check if mixer number exceeds maximum allowed size which is as mentioned
 * above SOUND_MIXER_NRDEVICES */
#define supported_mixer(CODEC,FOO) ((FOO >= 0) && \
    (FOO < SOUND_MIXER_NRDEVICES) && \
    (CODEC)->supported_mixers & (1 << FOO))
/* PLL divisors */
#define PLL_N 0x7
#define PLL_K 0x23F548

//#define DBG   printk
#define DBG(...)

//static struct clk     *iis_clock;
extern struct clk *clk_get(struct device *dev, const char *id);
extern int clk_enable(struct clk *clk);
extern void clk_disable(struct clk *clk);
extern unsigned long clk_get_rate(struct clk *clk);
extern int s3c_audio_attach(struct inode *inode, struct file *file,
			    audio_state_t * state);

/* default ADC, HiFi and Voice DAC sample rates */
#define WM8753_DEFAULT_HSRATE	WM8753_A8D8
#define WM8753_DEFAULT_VSRATE	WM8753_VD48K

/* 
 * WM8753 2 wire address is determined by GPIO5
 * state during powerup.
 *    low  = WM8753_2W_ADDR1
 *    high = WM8753_2W_ADDR2
 */
#define WM8753_2W_ADDR1	0x1a
#define WM8753_2W_ADDR2	0x1b
static unsigned short normal_i2c[] = {
	WM8753_2W_ADDR1, I2C_CLIENT_END
};

/* Magic definition of all other i2c variables and things */
I2C_CLIENT_INSMOD;
struct codec_t {
	char *name;
	int modcnt;
	int supported_mixers;
	int stereo_mixers;
	int record_sources;
	int codec_rate;
	unsigned int mixer_state[SOUND_MIXER_NRDEVICES];

#ifdef CONFIG_PM
	struct pm_dev *pm;

#endif				/*  */
	struct i2c_client wm_client;
	struct completion i2c_init;
	int wm8753_valid;
};
static struct codec_t codec = {
	.name 		= AUDIO_NAME,
	.modcnt 	= 0,
	.codec_rate 	= WM8753_DEFAULT_HSRATE,
	.supported_mixers = WM8753_SUPPORTED_MASK,
	.stereo_mixers 	= WM8753_STEREO_MASK,
	.record_sources = WM8753_RECORD_MASK,
	.wm8753_valid 	= 0,
};

/*
 * wm8753 register cache
 * We can't read the WM8753 register space when we 
 * are using 2 wire for device control, so we cache them instead. 
 */
static u16 reg_cache[64] = {
	0x0008, 0x0000, 0x000a, 0x000a, 0x0003, 0x0000, 0x0007, 0x01ff,
	0x01ff, 0x000f, 0x000f, 0x007b, 0x0000, 0x0032,
	0x0000, 0x00c3, 0x00c3, 0x00c0, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0040, 0x0010,
	0x018f, 0x0000, 0x0000, 0x0000, 0x0000, 0x0055,
	0x0005, 0x0050, 0x0055, 0x0050, 0x0055, 0x0050,
	0x0055, 0x0179, 0x0179, 0x0179, 0x0179, 0x0079,
	0x0064, 0x0000, 0x0000, 0x0000, 0x0097, 0x0097,
	0x0000, 0x0004, 0x0000, 0x0083, 0x0024, 0x01ba,
	0x0000, 0x0083, 0x0024, 0x01ba
};
static int wm8753_reset(struct i2c_client *client);
static int wm8753_2w_write(struct i2c_client *client, u8 reg, u16 value);

/*
 * read wm8753 register cache
 */
static inline u16 wm8753_read_cache(u8 reg)
{
	return reg_cache[reg - 1];
}


/*
 * write wm8753 register cache
 */
static inline void wm8753_write_cache(u8 reg, u16 value)
{
	reg_cache[reg - 1] = value;
}

/*
 * write to the WM8753 register space
 */
static int wm8753_2w_write(struct i2c_client *client, u8 reg, u16 value)
{
	u8 data[2];

	/* data is 
	 *   D15..D9 WM8753 register offset
	 *   D8...D0 register data
	 */
	data[0] = (reg << 1) | ((value >> 8) & 0x0001);
	data[1] = value & 0x00ff;
	if (i2c_master_send(client, data, 2) == 2) {
		wm8753_write_cache(reg, value);
		return 0;
	} else {
		printk(KERN_ERR
		       "%s error: failed to write 0x%x to register 0x%x\n",
		       AUDIO_NAME, value, reg);
		return -EIO;
	}
}
static long audio_set_dsp_speed(long val)
{
	u32 iispsr = 0, prescaler = 0;
	u32 iiscon = 0, iismod = 0, iisfic = 0;
	iiscon = S3C_IIS0CON_TXDMACTIVE | S3C_IIS0CON_RXDMACTIVE;
	writel(iiscon, S3C_IIS0CON);

	/* Setup the EPLL clock source */
	writel(0, S3C_EPLL_CON1);
	switch (val) {
	case 8000:
	case 16000:
	case 32000:
		writel((1 << 31) | (128 << 16) | (25 << 8) | (0 << 0),
		       S3C_EPLL_CON0);
		break;
	case 48000:
		writel((1 << 31) | (192 << 16) | (25 << 8) | (0 << 0),
		       S3C_EPLL_CON0);
		break;
	case 11025:
	case 22050:
	case 44100:
	default:
		writel((1 << 31) | (254 << 16) | (9 << 8) | (2 << 0),
		       S3C_EPLL_CON0);
		break;
	}
	while (!(readl(S3C_EPLL_CON0) & (1 << 30)));

	//MUXepll : FOUTepll
	writel(readl(S3C_CLK_SRC) | S3C_CLKSRC_EPLL_CLKSEL, S3C_CLK_SRC);

	//AUDIO0 sel : FOUTepll
	writel((readl(S3C_CLK_SRC) & ~(0x7 << 7)) | (0 << 7), S3C_CLK_SRC);

	//CLK_DIV2 setting
	writel(0x0, S3C_CLK_DIV2);

	/* FIFO is flushed before operation */
	iisfic = S3C_IIS_TX_FLUSH | S3C_IIS_RX_FLUSH;
	writel(iisfic, S3C_IIS0FIC);
	writel(0, S3C_IIS0FIC);

	/* Clear I2S prescaler value [13:8] and disable prescaler */
	iispsr = readl(S3C_IIS0PSR);
	iispsr &= ~((0x3f << 8) | (1 << 15));
	writel(iispsr, S3C_IIS0PSR);

	/* Configure I2SMOD */
	iismod =
	    S3C_IIS0MOD_IMS_EXTERNAL_MASTER | S3C_IIS0MOD_TXRXMODE
	    | S3C_IIS0MOD_IIS | S3C_IIS0MOD_32FS | S3C_IIS0MOD_16BIT;
	iismod |= S3C_IIS0MOD_384FS;
	writel(iismod, S3C_IIS0MOD);
	switch (val) {
	case 8000:
	case 11025:
		prescaler = 19;
		break;
	case 16000:
	case 22050:
		prescaler = 9;
		break;
	case 32000:
	case 44100:
	case 48000:
	default:
		prescaler = 4;
		break;
	}
	writel(readl(S3C_IIS0PSR) | (prescaler << 8) | (1 << 15),
	       S3C_IIS0PSR);
	DBG("#### IISCON: 0x%08x IISMOD: 0x%08x IISPSR: 0x%08x\n",
	    readl(S3C_IIS0CON), readl(S3C_IIS0MOD), readl(S3C_IIS0PSR));
	iiscon |= S3C_IIS0CON_I2SACTIVE;
	writel(iiscon, S3C_IIS0CON);
	return val;
}
static int wm8753_get_mixer(int cmd)
{
	int val = 0;
	u16 r = 0, l = 0;
	switch (cmd) {
	case SOUND_MIXER_VOLUME:	/* OUT1 Volume */
		l = wm8753_read_cache(WM8753_LOUT1V) & 0x7f;
		r = wm8753_read_cache(WM8753_ROUT1V) & 0x7f;
		break;
	case SOUND_MIXER_BASS:	/* bass */
		l = wm8753_read_cache(WM8753_BASS) & 0x0f;
		break;
	case SOUND_MIXER_TREBLE:	/* treble */
		l = wm8753_read_cache(WM8753_TREBLE) & 0x0f;
		break;
	case SOUND_MIXER_SYNTH:
		break;
	case SOUND_MIXER_PCM:
		break;
	case SOUND_MIXER_SPEAKER:	/* OUT2 Volume */
		l = wm8753_read_cache(WM8753_LOUT2V) & 0x7f;
		r = wm8753_read_cache(WM8753_ROUT2V) & 0x7f;
		break;
	case SOUND_MIXER_LINE:
		break;
	case SOUND_MIXER_MIC:
		l = wm8753_read_cache(WM8753_INCTL1) & 0x060;
		r = wm8753_read_cache(WM8753_INCTL1) & 0x180;
		break;
	case SOUND_MIXER_CD:
		break;
	case SOUND_MIXER_IMIX:	/* Recording monitor */
		break;
	case SOUND_MIXER_ALTPCM:
		break;
	case SOUND_MIXER_RECLEV:	/* Recording level */
		break;
	case SOUND_MIXER_IGAIN:	/* Input gain */
		l = wm8753_read_cache(WM8753_LADC) & 0xff;
		r = wm8753_read_cache(WM8753_RADC) & 0xff;
		break;
	case SOUND_MIXER_OGAIN:	/* Output gain */
		l = wm8753_read_cache(WM8753_LDAC) & 0xff;
		r = wm8753_read_cache(WM8753_RDAC) & 0xff;
		break;
	default:
		DBG(KERN_WARNING "unknown mixer IOCTL\n");
		return -EINVAL;
	}

	/* mix left and right */
	val = ((l << 8) & 0xff00) + (r & 0xff);
	return val;
}


/* ** NOT TESTED ** */
static int wm8753_set_mixer(int cmd, int val)
{
	int ret = 0;
	u16 reg;
	unsigned int left, right;

	/* separate left and right settings */
	right = ((val >> 8) & 0xff);
	right = right + 0x32;
	left = (val & 0xff);
	left = left + 0x32;

	/* 1111111 = +6dB, 0110000 = -73dB */
	if (right > 0x7f)
		right = 0x7f;
	if (right < 0x30)
		right = 0x30;
	if (left > 0x7f)
		left = 0x7f;
	if (left < 0x30)
		left = 0x30;
	switch (cmd) {
	case SOUND_MIXER_VOLUME:	/* volume OUT1 */
		reg = wm8753_read_cache(WM8753_LOUT1V) & 0x180;
		wm8753_2w_write(&codec.wm_client, WM8753_LOUT1V,
				reg | left);
		reg = wm8753_read_cache(WM8753_ROUT1V) & 0x180;
		wm8753_2w_write(&codec.wm_client, WM8753_ROUT1V,
				reg | right);

		/* volume OUT2 */
		reg = wm8753_read_cache(WM8753_LOUT2V) & 0x180;
		wm8753_2w_write(&codec.wm_client, WM8753_LOUT2V,
				reg | left);
		reg = wm8753_read_cache(WM8753_ROUT2V) & 0x180;
		wm8753_2w_write(&codec.wm_client, WM8753_ROUT2V,
				reg | right);
		break;
	case SOUND_MIXER_BASS:	/* bass */
		reg = wm8753_read_cache(WM8753_BASS) & 0x1f0;
		wm8753_2w_write(&codec.wm_client, WM8753_BASS,
				reg | (left & 0x0f));
		break;
	case SOUND_MIXER_TREBLE:	/* treble */
		reg = wm8753_read_cache(WM8753_TREBLE) & 0x1f0;
		wm8753_2w_write(&codec.wm_client, WM8753_TREBLE,
				reg | (left & 0x0f));
		break;
	case SOUND_MIXER_SYNTH:
		break;
	case SOUND_MIXER_PCM:
		break;
	case SOUND_MIXER_SPEAKER:	/* volume OUT2 */
		reg = wm8753_read_cache(WM8753_LOUT2V) & 0x180;
		wm8753_2w_write(&codec.wm_client, WM8753_LOUT2V,
				reg | left);
		reg = wm8753_read_cache(WM8753_ROUT2V) & 0x180;
		wm8753_2w_write(&codec.wm_client, WM8753_ROUT2V,
				reg | right);
		break;
	case SOUND_MIXER_LINE:
		break;
	case SOUND_MIXER_MIC:
		break;
	case SOUND_MIXER_CD:
		break;
	case SOUND_MIXER_IMIX:	/*  Recording monitor  */
		break;
	case SOUND_MIXER_ALTPCM:
		break;
	case SOUND_MIXER_RECLEV:	/* Recording level */
		break;
	case SOUND_MIXER_IGAIN:	/* Input gain LR ADC */
		reg = wm8753_read_cache(WM8753_LADC) & 0x100;
		wm8753_2w_write(&codec.wm_client, WM8753_LADC, reg | left);
		reg = wm8753_read_cache(WM8753_RADC) & 0x100;
		wm8753_2w_write(&codec.wm_client, WM8753_RADC,
				reg | right);
		break;
	case SOUND_MIXER_OGAIN:	/* Output gain LR DAC */
		reg = wm8753_read_cache(WM8753_LDAC) & 0x100;
		wm8753_2w_write(&codec.wm_client, WM8753_LDAC, reg | left);
		reg = wm8753_read_cache(WM8753_RDAC) & 0x100;
		wm8753_2w_write(&codec.wm_client, WM8753_RDAC,
				reg | right);
		break;
	default:
		DBG(KERN_WARNING "unknown mixer IOCTL");
		ret = -EINVAL;
		break;
	}
	return ret;
}
static int wm8753_mixer_ioctl(struct inode *inode, struct file *file,
			      unsigned int cmd, unsigned long arg)
{
	int i, val = 0;
	if (cmd == SOUND_MIXER_INFO) {
		mixer_info info;
		strncpy(info.id, codec.name, sizeof(info.id));
		strncpy(info.name, codec.name, sizeof(info.name));
		info.modify_counter = codec.modcnt;
		if (copy_to_user((void *) arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}
	if (cmd == SOUND_OLD_MIXER_INFO) {
		_old_mixer_info info;
		strncpy(info.id, codec.name, sizeof(info.id));
		strncpy(info.name, codec.name, sizeof(info.name));
		if (copy_to_user((void *) arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}
	if (_IOC_TYPE(cmd) != 'M' || _SIOC_SIZE(cmd) != sizeof(int))
		return -EINVAL;
	if (cmd == OSS_GETVERSION)
		return put_user(SOUND_VERSION, (int *) arg);
	if (_SIOC_DIR(cmd) == _SIOC_READ) {
		switch (_IOC_NR(cmd)) {
		case SOUND_MIXER_DEVMASK:	/* give them the supported mixers */
			val = codec.supported_mixers;
			break;
		case SOUND_MIXER_RECMASK:	/* Arg contains a bit for each supported recording source */
			val = codec.record_sources;
			break;
		case SOUND_MIXER_STEREODEVS:	/* Mixer channels supporting stereo */
			val = codec.stereo_mixers;
			break;
		case SOUND_MIXER_CAPS:
			val = SOUND_CAP_EXCL_INPUT;
			break;
		default:	/* read a specific mixer */
			i = _IOC_NR(cmd);
			if (!supported_mixer(&codec, i))
				return -EINVAL;
			val = wm8753_get_mixer(i);
			break;
		}
		return put_user(val, (int *) arg);
	}
	if (_SIOC_DIR(cmd) == (_SIOC_WRITE | _SIOC_READ)) {
		codec.modcnt++;
		if (get_user(val, (int *) arg))
			return -EFAULT;
		switch (_IOC_NR(cmd)) {
		default:	/* write a specific mixer */
			i = _IOC_NR(cmd);
			if (!supported_mixer(&codec, i))
				return -EINVAL;
			return wm8753_set_mixer(i, val);
		}
	}
	return -EINVAL;
}
static struct file_operations wm8753_mixer_fops = {
	ioctl : wm8753_mixer_ioctl, 
	llseek: no_llseek, 
	owner : THIS_MODULE
};

static int wm8753_power_down(void)
{
	if ((wm8753_2w_write(&codec.wm_client, WM8753_PWR1, 0x0100) != 0)
	    && (wm8753_2w_write(&codec.wm_client, WM8753_PWR2, 0x0000) !=
		0)
	    && (wm8753_2w_write(&codec.wm_client, WM8753_PWR3, 0x0000) !=
		0)
	    && (wm8753_2w_write(&codec.wm_client, WM8753_PWR4, 0x0000) !=
		0)) {
		DBG(KERN_ERR "could not powerdown WM8753");
		return -EIO;
	}
	return 0;
}
int wm8753_set_mic1_path(void)
{

	/* ALCSEL : Left ch only, Set ALC target gain */
	if (wm8753_2w_write(&codec.wm_client, WM8753_ALC1, 0x017b) != 0)
		return -EIO;

	/* MICSEL : Mic1 selected */
	if (wm8753_2w_write(&codec.wm_client, WM8753_MICBIAS, 0x0000)
	    != 0)
		return -EIO;

	/* MIC1ALC : ALC mix input select mic1 */
	if (wm8753_2w_write(&codec.wm_client, WM8753_INCTL2, 0x0002) != 0)
		return -EIO;

	/* RADCSEL : PGA, LADCSEL : PGA */
	if (wm8753_2w_write(&codec.wm_client, WM8753_ADCIN, 0x0000) != 0)
		return -EIO;

	/* 50k, VREF : on, MICB,VDAC : off, DACL,DACR : off, DIGENB : MCLK */
	if (wm8753_2w_write(&codec.wm_client, WM8753_PWR1, 0x00c0) != 0)
		return -EIO;

	/* MICAMP1EN,MICAMP2EN,ALCMIX,PGAL,PGAR,ADCL,ADCR : on, RXMIX,LINEMIX : off */
	if (wm8753_2w_write(&codec.wm_client, WM8753_PWR2, 0x01fc) != 0)
		return -EIO;
	return 0;
}
int wm8753_set_linein_path(void)
{

	/* RADCSEL : LINE2, LADCSEL : LINE1 */
	if (wm8753_2w_write(&codec.wm_client, WM8753_ADCIN, 0x0015) != 0)
		return -EIO;

	/* ALCSEL : off */
	if (wm8753_2w_write(&codec.wm_client, WM8753_ALC1, 0x0000) != 0)
		return -EIO;

	/* RM : LINE2, LM : LINE1 */
	if (wm8753_2w_write(&codec.wm_client, WM8753_INCTL1, 0x0000) != 0)
		return -EIO;

	/* 50k, VREF : on, MICB,VDAC : off, DACL,DACR : off, DIGENB : MCLK */
	if (wm8753_2w_write(&codec.wm_client, WM8753_PWR1, 0x00c0) != 0)
		return -EIO;

	/* MICAMP1EN,MICAMP2EN,RXMIX,LINEMIX,ALCMIX,PGAL,PGAR  : off, ADCL,ADCR : on */
	if (wm8753_2w_write(&codec.wm_client, WM8753_PWR2, 0x000c) != 0)
		return -EIO;
	return 0;
}
int wm8753_set_hpout_path(void)
{

	/* LOUT1,ROUT1,LOUT2,ROUT2,OUT3,OUT4 : on, MONO1,2 : off */
	if (wm8753_2w_write(&codec.wm_client, WM8753_PWR3, 0x01f8) != 0)
		return -EIO;

	/* 50k, VREF : on, MICB,VDAC : off, DACL,DACR : on, DIGENB : MCLK */
	if (wm8753_2w_write(&codec.wm_client, WM8753_PWR1, 0x00cc) != 0)
		return -EIO;
	return 0;
}


/* 
 * initiliase the WM8753
 */
static int wm8753_init(void)
{
	int ret = 0;

	/* power up the device */

	/* Reset all registers to their default value */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_RESET, 0x0000);

	/* 5k divider en, VREF on, MICB off, VDAC off, DACL off, DACR off, MCLK dis */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_PWR1, 0x01c1);

	/* Set Left DAC volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_LDAC, 0x01ff);

	/* Set Right DAC volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_RDAC, 0x01ff);

	/* Set Left OUT1 volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_LOUT1V, 0x0179);

	/* Set Right OUT1 volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_ROUT1V, 0x0179);

	/* Set Left OUT2 volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_LOUT2V, 0x0179);

	/* Set Right OUT2 volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_ROUT2V, 0x0179);

	/* Set Left input volume, Mute en */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_LINVOL, 0x0197);

	/* Set Right input volume, Mute en */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_RINVOL, 0x0197);

	/* Set Left ADC volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_LADC, 0x01e8);

	/* Set Right ADC volume */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_RADC, 0x01e8);

	/* LOUT1,ROUT1,LOUT2,ROUT2,MONO1,2 : off, OUT3,OUT4 : on */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_PWR3, 0x0018);

	/* IFMODE : HiFi over HiFi, VXFS : out, LRCOE : out */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_IOCTL, 0x000b);

	/* RADC input : LINE2, LADC input : LINE1 */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_ADCIN, 0x0015);

	/* RDAC -> Right Mixer */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_ROUTM1, 0x150);

	/* LDAC -> Left Mixer */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_LOUTM1, 0x150);

	/* MCLKSEL : from MCLK pin, PCMCLKSEL : PCMCLK pin, CLKEQ : HiFi DAC */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_CLOCK, 0x0004);

	/* HiFi audio interface : I2S format */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_HIFI, 0x0002);

	/* PCM audio interface : I2S format */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_PCM, 0x0002);

	/* VXDACOSR : 1, ADCOSR : 1, DACOSR : 1  */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_SRATE2, 0x0007);

	/* IFMODE : HiFi over HiFi, VXFS : out, LRCOE : input */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_IOCTL, 0x000a);

	/* 50k, VREF : on, MICB,VDAC : off, DACL,DACR : on, DIGENB : MCLK */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_PWR1, 0x00cc);

	/* ADCL,ADCR : on, MICAPM1,2, ALCMIX,PGAL,PGAR,RXMIX,LINEMIX : off */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_PWR2, 0x000c);

	/* LOUT1,ROUT1,LOUT2,ROUT2,OUT3,OUT4 : on, MONO1,2 : off */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_PWR3, 0x01f8);

	/* RECMIX,MONOMIX : off, RIGHT MIX, LEFT MIX : on */
	ret += wm8753_2w_write(&codec.wm_client, WM8753_PWR4, 0x0003);
	ret += wm8753_2w_write(&codec.wm_client, WM8753_DAC, 0x0000);
	return ret != 0 ? -EIO : 0;
}
static int s3c_select_clk(void)
{

	/* PCLK & SCLK gating enable */
	writel(readl(S3C_PCLK_GATE) | S3C_CLKCON_PCLK_IIS0, S3C_PCLK_GATE);
	writel(readl(S3C_SCLK_GATE) | S3C_CLKCON_SCLK_AUDIO0,
	       S3C_SCLK_GATE);
	return 0;
}


/* open the WM8753 audio driver */
static int s3c_audio_ioctl(struct inode *inode, struct file *file,
			   uint cmd, ulong arg)
{
	long val = 0;
	int ret = 0;
	audio_state_t *state = file->private_data;
	switch (cmd) {
	case SNDCTL_DSP_STEREO:
		if (get_user(val, (int *) arg))
			return -EINVAL;
		state->sound_mode = (val) ? 2 : 1;
		return 0;
	case SNDCTL_DSP_CHANNELS:
		if (get_user(val, (int *) arg))
			return -EINVAL;
		if (val != 1 && val != 2)
			return -EINVAL;
		state->sound_mode = val;
		return put_user(val, (int *) arg);
	case SOUND_PCM_READ_CHANNELS:
		return put_user(state->sound_mode, (long *) arg);
	case SNDCTL_DSP_SPEED:
		ret = get_user(val, (long *) arg);
		if (ret)
			break;
		audio_set_dsp_speed(val);
		switch (val) {
		case 8000:
		case 16000:
		case 32000:
			wm8753_2w_write(&codec.wm_client, WM8753_SRATE1,
					0x0018);
			break;
		case 11025:
		case 22050:
		case 44100:
			wm8753_2w_write(&codec.wm_client, WM8753_SRATE1,
					0x0022);
			break;
		case 48000:
			wm8753_2w_write(&codec.wm_client, WM8753_SRATE1,
					0x000a);
			break;
		}
		break;
	case SOUND_PCM_READ_RATE:
		ret = put_user(val, (long *) arg);
		break;
	case SNDCTL_DSP_SETFMT:
	case SNDCTL_DSP_GETFMTS:

		/* we can do 16-bit only */
		ret = put_user(AFMT_S16_LE, (long *) arg);
		break;
	default:

		/* Maybe this is meant for the mixer (as per OSS Docs) */
		ret = wm8753_mixer_ioctl(inode, file, cmd, arg);
		break;
	}
	return ret;
}

static void s3c_audio_shutdown(void *dummy)
{
	unsigned long iismod, iiscon;
	iismod = readl(S3C_IIS0MOD);
	iismod &= ~S3C_IIS0MOD_TXMODE;
	iismod &= ~S3C_IIS0MOD_RXMODE;
	writel(iismod, S3C_IIS0MOD);
	iiscon = readl(S3C_IIS0CON);
	if (iismod & (S3C_IIS0MOD_TXMODE | S3C_IIS0MOD_RXMODE)) {
		iiscon &= !S3C_IIS0CON_I2SACTIVE;
		writel(iiscon, S3C_IIS0CON);
	}

	/* Disable the I2S clock */
	writel(readl(S3C_PCLK_GATE) & ~(S3C_CLKCON_PCLK_IIS0),
	       S3C_PCLK_GATE);
	writel(readl(S3C_SCLK_GATE) & ~(S3C_CLKCON_SCLK_AUDIO0),
	       S3C_SCLK_GATE);
	writel(readl(S3C_EPLL_CON0) & ~(1 << 31), S3C_EPLL_CON0);
}

static int s3c_init_iis(void)
{
	/*Set I2C port to controll WM8753 codec */
	s3c_gpio_pullup(S3C_GPB5, 0);
	s3c_gpio_pullup(S3C_GPB6, 0);
	s3c_gpio_cfgpin(S3C_GPB5, S3C_GPB5_I2C_SCL);
	s3c_gpio_cfgpin(S3C_GPB6, S3C_GPB6_I2C_SDA);
	s3c_gpio_cfgpin(S3C_GPD0, S3C_GPD0_I2S_CLK0);
	s3c_gpio_cfgpin(S3C_GPD1, S3C_GPD1_I2S_CDCLK0);
	s3c_gpio_cfgpin(S3C_GPD2, S3C_GPD2_I2S_LRCLK0);
	s3c_gpio_cfgpin(S3C_GPD3, S3C_GPD3_I2S_DI0);
	s3c_gpio_cfgpin(S3C_GPD4, S3C_GPD4_I2S_DO0);
	s3c_gpio_pullup(S3C_GPD0, 2);
	s3c_gpio_pullup(S3C_GPD1, 2);
	s3c_gpio_pullup(S3C_GPD2, 2);
	s3c_gpio_pullup(S3C_GPD3, 2);
	s3c_gpio_pullup(S3C_GPD4, 2);
	return 0;
}

static audio_stream_t output_stream = {
      	id: "s3c64xx audio out ", 
	subchannel: DMACH_I2S_OUT,
};

static audio_stream_t input_stream = {
      	id: "s3c64xx audio in ", 
	subchannel: DMACH_I2S_IN,
};

static audio_state_t audio_state = {
	.output_stream = &output_stream,
	.input_stream  = &input_stream,
	.hw_shutdown = s3c_audio_shutdown,
	.client_ioctl = s3c_audio_ioctl,
};

static int wm8753_hifi_open(struct inode *inode, struct file *file)
{
	s3c_init_iis();
	s3c_select_clk();
	init_MUTEX(&audio_state.sem);
	return s3c_audio_attach(inode, file, &audio_state);
}

static struct file_operations wm8753_hifi_fops = {
	open : wm8753_hifi_open, 
	owner: THIS_MODULE
};


/* 
 * reset the WM8753
 */
static inline int wm8753_reset(struct i2c_client *client)
{
	return wm8753_2w_write(client, WM8753_RESET, 0);
}


/* WM8753 2 Wire layer */
static int wm8753_attach_adapter(struct i2c_adapter *adapter);
static int wm8753_detach_client(struct i2c_client *client);
static struct i2c_driver wm_driver = {
	.driver = {
		.name = "wm8753",
		.owner = THIS_MODULE,},
	.id = I2C_DRIVERID_WM8753,
	.attach_adapter = wm8753_attach_adapter,
	.detach_client = wm8753_detach_client,
};

static int wm8753_detect(struct i2c_adapter *adap, int addr, int kind)
{
	int ret;
	i2c_set_clientdata(&codec.wm_client, &codec);
	codec.wm_client.addr = addr;
	codec.wm_client.adapter = adap;
	codec.wm_client.driver = &wm_driver;
	codec.wm_client.flags = 0;

	/* since the codec is read only, we have to assume that a successful
	 * write (via a codec reset) is proof that the codec is responding
	 */
	if (wm8753_reset(&codec.wm_client) != 0) {
		printk(KERN_ERR "cannot reset the WM8753");
		complete(&codec.i2c_init);
		return -EIO;
	}
	strlcpy(codec.wm_client.name, "wm8753", I2C_NAME_SIZE);
	if ((ret = i2c_attach_client(&codec.wm_client)) == 0)
		codec.wm8753_valid = 1;
	complete(&codec.i2c_init);
	return ret;
}


//static int wm8753_attach_adapter(struct i2c_adapter *adapter)
static int wm8753_attach_adapter(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, wm8753_detect);

	//return i2c_probe(adapter, &addr_data, wm8753_detect);
}


/*
 * Detach WM8753 2 wire client 
 */
static int wm8753_detach_client(struct i2c_client *client)
{
	i2c_detach_client(client);
	return 0;
}

static int s3c_i2s_init(void)
{
	init_completion(&codec.i2c_init);
	if (i2c_add_driver(&wm_driver)) {
		printk(KERN_ERR "can't add i2c driver\n");
		goto err_dev;
	}

	/* wait for i2c init to complete */
	wait_for_completion(&codec.i2c_init);
	if (!codec.wm8753_valid) {
		printk(KERN_ERR "can't find codec\n");
		goto err_dev;
	}

	/* initialise WM8753 */
	if (wm8753_init() != 0) {
		printk(KERN_ERR "can't initialise WM8753\n");
		goto err_io;
	}
	return 0;

err_dev:
	return -ENODEV;

err_io:
	i2c_del_driver(&wm_driver);
	return -EIO;
}


#ifdef CONFIG_PM
/* TODO : Power management */
int s3c64xx_i2s_suspend(void)
{
	return 0;
}

int s3c64xx_i2s_resume(void)
{
	return 0;
}

static int s3c64xx_pm_callback(struct pm_dev *dev, pm_request_t rqst,
			       void *data)
{
	switch (rqst) {
	case PM_SUSPEND:
		s3c64xx_i2s_suspend();
		break;
	case PM_RESUME:
		s3c64xx_i2s_resume();
		break;
	}
	return 0;
}


#else				/*  */
#define s3c64xx_i2s_suspend NULL
#define s3c64xx_i2s_resume NULL
#endif				/*  */

static int audio_dev_id, mixer_dev_id;

static int s3c64xx_audio_probe(struct device *dev)
{
	s3c_i2s_init();
	audio_dev_id = register_sound_dsp(&wm8753_hifi_fops, -1);
	mixer_dev_id = register_sound_mixer(&wm8753_mixer_fops, -1);
	printk(KERN_INFO "Sound: S3C-I2S: dsp id %d mixer id %d\n",
	       audio_dev_id, mixer_dev_id);
	flush_scheduled_work();

#ifdef CONFIG_PM
	pm_register(PM_SYS_DEV, PM_SYS_UNKNOWN, s3c64xx_pm_callback);

#endif				/*  */
	return 0;
}

static int s3c64xx_audio_remove(struct device *dev)
{
	wm8753_power_down();
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	i2c_del_driver(&wm_driver);
	return 0;
}

static struct device_driver s3c64xx_i2s_driver = {
	.name = "s3c2410-iis",
	.bus = &platform_bus_type,
	.probe = s3c64xx_audio_probe,
	.remove = s3c64xx_audio_remove,
#ifdef CONFIG_PM
	.suspend = s3c64xx_audio_suspend,
	.resume = s3c64xx_audio_resume,
#endif
};

/*
 * initialise the WM8753 driver
 * register the mixer and dsp interfaces with the kernel 
 */
static int __init s3c_wm8753_init(void)
{
	return driver_register(&s3c64xx_i2s_driver);
}

module_init(s3c_wm8753_init);

/* 
 * unregister interfaces and clean up
 */
static void __exit s3c_wm8753_exit(void)
{
	driver_unregister(&s3c64xx_i2s_driver);
} 

module_exit(s3c_wm8753_exit);

MODULE_AUTHOR("Ryu Euiyoul");
MODULE_DESCRIPTION("Samsung S3C WM8753 driver");
MODULE_LICENSE("GPL");
