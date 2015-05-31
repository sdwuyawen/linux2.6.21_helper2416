/*
 *  linux/sound/oss/s3c64xx-ac97.c -- AC97 interface for the S3C
 *
 *  $Id: s3c6400_ac97.c,v 1.5 2008/01/31 07:22:14 eyryu Exp $
 *  Copyright (C) 2007, Ryu Euiyoul <ryu.real@gmail.com>
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/ac97_codec.h>
#include <linux/major.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/dma.h>
#include <asm/io.h>

#include <asm/arch/regs-s3c6400-clock.h>
#include <asm/arch/regs-ac97.h>
#include <asm/arch/regs-gpio.h>

#include "s3c6400_pcm.h"

extern struct class_simple *sound_class;

static int waitingForMask = AC_GLBSTAT_RI;
static int s3c64xx_ac97_refcount;

static int	audio_channels;
static struct clk	*ac97_clock;

extern struct clk *clk_get(struct device *dev, const char *id);
extern int clk_enable(struct clk *clk);
extern void clk_disable(struct clk *clk);

static DEFINE_MUTEX(ac97_mutex);
static DECLARE_WAIT_QUEUE_HEAD(gsr_wq);

static unsigned short s3c64xx_ac97_read(struct ac97_codec *codec,
				      unsigned char reg)
{
	unsigned int ___stat;
	unsigned char ___addr;
	unsigned short ___data;
	unsigned long ac_glbctrl;
	unsigned long ac_codec_cmd;

	mutex_lock(&ac97_mutex);

	waitingForMask = AC_GLBSTAT_RI;
	//ac_codec_cmd = __raw_readl(AC_CODEC_CMD);
	ac_codec_cmd = AC_CMD_R | AC_CMD_ADDR(reg);
	__raw_writel(ac_codec_cmd, AC_CODEC_CMD);

	udelay(1000);

	ac_glbctrl = __raw_readl(AC_GLBCTRL);
	ac_glbctrl |= AC_GLBCTRL_RIE;
	__raw_writel(ac_glbctrl,AC_GLBCTRL);

	___stat = __raw_readl(AC_CODEC_STAT);
	___addr = (___stat >> 16) & 0x7f;
	___data = (___stat & 0xffff);

	wait_event_timeout(gsr_wq,___addr==reg,1);
	if (___addr != reg) {
		printk(KERN_ERR"AC97: read error (ac97_reg=%x addr=%x)\n", reg, ___addr);
		printk(KERN_ERR"Check audio codec jumpper settings\n\n");
		goto out;
	}

out:	mutex_unlock(&ac97_mutex);
	return ___data;
}

static void s3c64xx_ac97_write(struct ac97_codec *codec, u8 reg, u16 val)
{
	unsigned int ___stat;
	unsigned short ___data;
	unsigned long ac_glbctrl;
	unsigned long ac_codec_cmd;

	mutex_lock(&ac97_mutex);

	waitingForMask = AC_GLBSTAT_RI;
	ac_codec_cmd = AC_CMD_ADDR(reg) | AC_CMD_DATA(val);
	__raw_writel(ac_codec_cmd, AC_CODEC_CMD);

	udelay(50);

	ac_glbctrl = __raw_readl(AC_GLBCTRL);
	ac_glbctrl |= AC_GLBCTRL_RIE;
	__raw_writel(ac_glbctrl,AC_GLBCTRL);

	ac_codec_cmd = __raw_readl(AC_CODEC_CMD);
	ac_codec_cmd |= AC_CMD_R;	/* By default it shud be read enabled. No? */
	__raw_writel(ac_codec_cmd, AC_CODEC_CMD);

	___stat = __raw_readl(AC_CODEC_CMD);
	___data = (___stat & 0xffff);

	wait_event_timeout(gsr_wq,___data==val,1);
	if(___data!=val){
		printk("%s: write error (ac97_val=%x data=%x)\n",
				__FUNCTION__, val, ___data);
	}

	mutex_unlock(&ac97_mutex);
}

static irqreturn_t s3c64xx_ac97_isr(int irq, void *dev_id)
{
	int gsr;
	unsigned long ac_glbctrl;

	gsr = __raw_readl(AC_GLBSTAT) & waitingForMask;

	if (gsr) {
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl &= ~AC_GLBCTRL_RIE;
		__raw_writel(ac_glbctrl,AC_GLBCTRL);
		wake_up(&gsr_wq);
	}

	return IRQ_HANDLED;
}

static struct ac97_codec s3c64xx_ac97_codec = {
	.codec_read = s3c64xx_ac97_read,
	.codec_write = s3c64xx_ac97_write,
	.codec_wait = NULL,
};

void codec_reset_settings(int mode)
{
	if(mode	== MODE_PLAY) {
		s3c64xx_ac97_write(0,0x2A,0x0001);
		s3c64xx_ac97_write(0,0x2C,0xbb80);
		s3c64xx_ac97_write(0,0x02,0x8080);	// Mute SPK volume
		s3c64xx_ac97_write(0,0x04,0x0606);	// Set Headphone volume
		s3c64xx_ac97_write(0,0x06,0x8080);	// Mute OUT3,4
		s3c64xx_ac97_write(0,0x08,0xc880);	// Mute MONOIN to hp,spk mixer
		s3c64xx_ac97_write(0,0x0a,0xff1f);	// Mute LINE to headphone,spk,mono mixer path
		s3c64xx_ac97_write(0,0x0c,0x6808);	// Mute DAC to spk,mono mixer
		s3c64xx_ac97_write(0,0x26,1<<8);	// Disables stereo ADCs and record mux PGA	
		s3c64xx_ac97_write(0,0x1c,0x00aa);
		s3c64xx_ac97_write(0,0x3c,0xf933);
		s3c64xx_ac97_write(0,0x3e,0xf8ff);
		s3c64xx_ac97_write(0,0x42,0x00ff);
	}
	else {
		s3c64xx_ac97_write(0,0x2A,0x0001);
		s3c64xx_ac97_write(0,0x5C,0x0000);	// Select ADC slot 6	 ( MIC case )
		s3c64xx_ac97_write(0,0x0c,0xe808);	// Mute DAC to hp,spk,mono mixer
		s3c64xx_ac97_write(0,0x26,1<<9);	// Disables stereo DAC
		s3c64xx_ac97_write(0,0x22,0x4040);	// Mic setting
		s3c64xx_ac97_write(0,0x12,0x0f0f);	// Recording Volume
		s3c64xx_ac97_write(0,0x0a,0xe808);	// Mute LINE to headphone,spk,mono mixer path and set input gain
		s3c64xx_ac97_write(0,0x14,0xd612);	// Recording source (LINEL,R)
		s3c64xx_ac97_write(0,0x3c,0xf8cf);	// Power on left,right ADC 
		s3c64xx_ac97_write(0,0x3e,0xff9f);	// Enable LINEL,R PGA
	}
}

int s3c64xx_ac97_get(struct ac97_codec *codec)
{
	int ret = 0;
	unsigned long clkcon;
	unsigned long ac_glbctrl;

	if (!s3c64xx_ac97_refcount) {
		clkcon = __raw_readl( S3C_PCLK_GATE);
		clkcon |= S3C_CLKCON_PCLK_AC97;
		__raw_writel(clkcon,S3C_PCLK_GATE);

		clkcon = __raw_readl( S3C_PCLK_GATE);

		// Cold reset
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl = AC_GLBCTRL_COLD;
		__raw_writel(ac_glbctrl, AC_GLBCTRL);
		udelay(1000);
		// Controller and Codec normal mode
		__raw_writel(0, AC_GLBCTRL);
		udelay(1000);

		//AC-link on
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl = AC_GLBCTRL_AE;
		__raw_writel(ac_glbctrl, AC_GLBCTRL);
		udelay(1000);
		// Controller and Codec normal mode
		__raw_writel(0, AC_GLBCTRL);
		udelay(1000);

		// Warm reset
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl = AC_GLBCTRL_WARM;
		__raw_writel(ac_glbctrl, AC_GLBCTRL);
		udelay(1000);
		// Controller and Codec normal mode
		__raw_writel(0, AC_GLBCTRL);
		udelay(1000);

		//AC-link on
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl = AC_GLBCTRL_AE;
		__raw_writel(ac_glbctrl, AC_GLBCTRL);
		udelay(1000);
		// Controller and Codec normal mode
		__raw_writel(0, AC_GLBCTRL);
		udelay(1000);

		//Transfer data enable using AC-link
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl |= AC_GLBCTRL_TE;
		__raw_writel(ac_glbctrl, AC_GLBCTRL);
		udelay(1000);


		/* Enable both always */
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl |= AC_GLBCTRL_POMODE_DMA | AC_GLBCTRL_PIMODE_DMA | AC_GLBCTRL_MIMODE_DMA;
		__raw_writel(ac_glbctrl, AC_GLBCTRL);
		ret = request_irq(IRQ_AC97, s3c64xx_ac97_isr, SA_INTERRUPT,"AC97", NULL);
		if (ret)
			goto out;

		s3c64xx_ac97_write(0,0x26,(1<<8));
		ac_glbctrl = __raw_readl(AC_GLBCTRL);
		ac_glbctrl |= (1<<2);
		__raw_writel(ac_glbctrl,AC_GLBCTRL);
		udelay(1000);

		ret = ac97_probe_codec(codec);
		if (ret != 1) {
			free_irq(IRQ_AC97, NULL);
			__raw_writel(0, AC_GLBCTRL);
			clkcon &= ~ S3C_CLKCON_PCLK_AC97;
			__raw_writel(clkcon,S3C_PCLK_GATE);
			goto out;
		}
		ret = 0;
	}

	s3c64xx_ac97_refcount++;

      out:
	codec = &s3c64xx_ac97_codec;

	return ret;
}

void s3c64xx_ac97_put(void)
{
	unsigned long clkcon;
	unsigned long ac_glbctrl;

	s3c64xx_ac97_refcount--;
	if (!s3c64xx_ac97_refcount) {
		ac_glbctrl = 0x0;
		__raw_writel(ac_glbctrl, AC_GLBCTRL);
		clkcon = __raw_readl( S3C_PCLK_GATE);
		clkcon &= ~S3C_CLKCON_PCLK_AC97;
		__raw_writel(clkcon,S3C_PCLK_GATE);
		free_irq(IRQ_AC97, NULL);
	}
}

EXPORT_SYMBOL(s3c64xx_ac97_get);
EXPORT_SYMBOL(s3c64xx_ac97_put);

static int mixer_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	int ret;
	printk(KERN_DEBUG "??? Mixer_ioctl : cmd %x\n",cmd);
#if 0
	int val, mask;

	if (_SIOC_DIR(cmd) == (_SIOC_WRITE | _SIOC_READ)) {	/* trap only the write command */
		switch (_IOC_NR(cmd)) {
		case SOUND_MIXER_RECSRC:	/* See if its an ioctl to set the _other_ record-source */

			val = s3c64xx_ac97_codec.recmask_io(&s3c64xx_ac97_codec, 1, 0);
			get_user(mask, (int *) arg);

			/* Req=LINE or Stereo and Curr=MIC */
			if (((mask & (1 << SOUND_MIXER_LINE)) || (mask & (1 << SOUND_MIXER_IGAIN)))
										    && (val & (1 << SOUND_MIXER_MIC))) {
				if (audio_state.read_busy == FLAG_UP) {
					audio_state.read_busy = FLAG_REC_CHG;
					/* Wait until current 'read' is through. */
					sleep_on(&audio_state.rbq);
				}

				mask = 1 << SOUND_MIXER_LINE;	/* No stereo */
				/* All Analog Mode, ADC Input select=>left slot3, right slot4 */
				s3c64xx_ac97_write(NULL, 0x6E, 0x0000);
				printk("Input source is now LineIn \n");

			} else {
				/* Req=MIC and Curr=LINE or Stereo */
				if (((val & (1 << SOUND_MIXER_LINE)) || (val & (1 << SOUND_MIXER_IGAIN)))
				    						&& (mask & (1 << SOUND_MIXER_MIC))) {

					if (audio_state.read_busy == FLAG_UP) {
						audio_state.read_busy = FLAG_REC_CHG;
						/* Wait until current 'read' is through. */
						sleep_on(&ac97_audio_state.rbq);
					}

					mask = 1 << SOUND_MIXER_MIC;

					/* ADC Input Slot => left slot6, right slot9, MIC GAIN = 20 */
					s3c64xx_ac97_write(NULL, 0x6E,0x0020);

					printk("Input source is now MIC \n");
				} else if (val == mask) {
					printk("Input source is the same.\n");
					return 0;
				} else {
					printk("Requested input source is not present.\n");
					printk("Req=0x08%X and Curr=0x08%X\n", mask, val);
					return -EINVAL;	/* Can't do anything. */
				}
			}

			put_user(mask, (int *) arg);
			ret = s3c64xx_ac97_codec.mixer_ioctl(&s3c64xx_ac97_codec, cmd, arg);
			switch_stream(audio_state.input_stream);

			if (audio_state.read_busy == FLAG_REC_CHG) {
				wake_up(&audio_state.rbq);
				audio_state.read_busy = FLAG_DOWN;	/* audio_read will set it. */
			}
			return ret;

		default:
			break;
		}
	}
#endif
	ret = s3c64xx_ac97_codec.mixer_ioctl(&s3c64xx_ac97_codec, cmd, arg);
	return ret;
}

static struct file_operations mixer_fops = {
      ioctl:mixer_ioctl,
      llseek:no_llseek,
      owner:THIS_MODULE
};

static int codec_adc_rate;
static int codec_dac_rate;

static int ac97_ioctl(struct inode *inode, struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int ret;
	long val = 0;
	audio_state_t *state = file->private_data;

	switch (cmd) {
	case SNDCTL_DSP_STEREO:
		if (get_user(val, (int *) arg))
			return -EINVAL;
		audio_channels = (val) ? 2 : 1;
		state->sound_mode = audio_channels;
		return 0;

	case SNDCTL_DSP_CHANNELS:
		if (get_user(val, (int *) arg))
			return -EINVAL;
		if (val != 1 && val != 2)
			return -EINVAL;
		audio_channels = val;
		state->sound_mode = audio_channels;
		return put_user(val, (int *) arg);

	case SOUND_PCM_READ_CHANNELS:
		return put_user(audio_channels, (long *) arg);

	case SNDCTL_DSP_SPEED:
		ret = get_user(val, (long *) arg);
		if (ret)
			return ret;
		if (file->f_mode & FMODE_READ)
			codec_adc_rate = ac97_set_adc_rate(&s3c64xx_ac97_codec, val);
		if (file->f_mode & FMODE_WRITE)
			codec_dac_rate = ac97_set_dac_rate(&s3c64xx_ac97_codec, val);
		/* fall through */

	case SOUND_PCM_READ_RATE:
		if (file->f_mode & FMODE_READ)
			val = codec_adc_rate;
		if (file->f_mode & FMODE_WRITE)
			val = codec_dac_rate;
		return put_user(val, (long *) arg);

	case SNDCTL_DSP_SETFMT:
	case SNDCTL_DSP_GETFMTS:
		/* FIXME: can we do other fmts? */
		return put_user(AFMT_S16_LE, (long *) arg);

	default:
		/* Maybe this is meant for the mixer (As per OSS Docs) */
		return mixer_ioctl(inode, file, cmd, arg);
	}
	return 0;
}

static void s3c64xx_ac97_config_pins(void)
{
        s3c_gpio_cfgpin(S3C_GPD0,S3C_GPD0_AC97_BITCLK);
        s3c_gpio_cfgpin(S3C_GPD1,S3C_GPD1_AC97_RESET);
        s3c_gpio_cfgpin(S3C_GPD2,S3C_GPD2_AC97_SYNC);
        s3c_gpio_cfgpin(S3C_GPD3,S3C_GPD3_AC97_SDI);
        s3c_gpio_cfgpin(S3C_GPD4,S3C_GPD4_AC97_SDO);

        s3c_gpio_pullup(S3C_GPD0,0);
        s3c_gpio_pullup(S3C_GPD1,0);
        s3c_gpio_pullup(S3C_GPD2,0);
        s3c_gpio_pullup(S3C_GPD3,0);
        s3c_gpio_pullup(S3C_GPD4,0);
}

static int s3c64xx_select_clk(void)
{
	ac97_clock = NULL;
	ac97_clock = clk_get(NULL, "ac97");
	if (!ac97_clock) {
		printk(KERN_DEBUG "failed to get ac97 clock source\n");
		return -ENOENT;
	}
	clk_enable(ac97_clock);
	return 0;
}

static int s3c64xx_init_ac97(void)
{
	int ret;
	struct ac97_codec *codec = &s3c64xx_ac97_codec;

	ret = s3c64xx_ac97_get(codec);
	if (ret)
		return ret;
	return 0;

}

/* AC97 GPIO configuration*/
static int s3c64xx_audio_init(int *dummy)
{

	s3c64xx_ac97_config_pins();
	udelay(1000);
	s3c64xx_select_clk();
	if(s3c64xx_init_ac97())
	{
		printk("Audio init failed \n");
		return -EBUSY;
	}
	return 0;
}

static void s3c64xx_audio_shutdown(void *dummy)
{
	/* Disable the AC97 clock  */
	clk_disable(ac97_clock);
}

static audio_stream_t output_stream = {
	id: "s3c64xx audio out ",
	subchannel: DMACH_AC97_PCM_OUT,
};
static audio_stream_t input_stream_line = {
	id: "s3c64xx audio line in ",
	subchannel: DMACH_AC97_PCM_IN,
};
static audio_stream_t input_stream_mic = {
	id: "s3c64xx audio mic in ",
	subchannel: DMACH_AC97_MIC_IN,
};
static audio_state_t audio_state = {
	.output_stream	= &output_stream,
	.input_stream_line	= &input_stream_line,
	.input_stream_mic	= &input_stream_mic,
	.hw_shutdown	= s3c64xx_audio_shutdown,
	.client_ioctl	= ac97_ioctl,
};


extern int s3c_audio_attach(struct inode *inode, struct file *file,
			audio_state_t *state);

static int ac97_audio_open(struct inode *inode, struct file *file)
{
	init_MUTEX(&audio_state.sem);
	return s3c_audio_attach(inode, file, &audio_state);
}

static struct file_operations ac97_audio_fops = {
      open:ac97_audio_open,
      owner:THIS_MODULE
};

#ifdef CONFIG_PM
int s3c64xx_ac97_suspend(void)
{
	s3c64xx_ac97_put();
	return 0;
}

int s3c64xx_ac97_resume(void)
{
	int dummy;
	struct ac97_codec *codec = &s3c64xx_ac97_codec;
	printk(KERN_DEBUG "audio resume \n");
	s3c64xx_audio_init(dummy);
	return 0;
}

static int s3c64xx_pm_callback(struct pm_dev *dev, pm_request_t rqst, void *data)
{
	printk(KERN_DEBUG "audio pm callback \n");
	switch(rqst) {
	case PM_SUSPEND:
		s3c64xx_ac97_suspend();
		break;
	case PM_RESUME:
		s3c64xx_ac97_resume();
		break;
	}
	return 0;

}
#else
#define s3c64xx_ac97_suspend NULL
#define s3c64xx_ac97_resume NULL
#endif

static int audio_dev_id, mixer_dev_id;

static int  s3c64xx_audio_probe(struct device * dev)
{
	unsigned int ret,dummy;
	ret =  s3c64xx_audio_init(&dummy);
	if (ret)
		return ret;

	audio_dev_id = register_sound_dsp(&ac97_audio_fops, -1);
	mixer_dev_id = register_sound_mixer(&mixer_fops, -1);
	printk(KERN_INFO "Sound: S3C-AC97: dsp id %d mixer id %d\n",
		audio_dev_id, mixer_dev_id);

	flush_scheduled_work();
#ifdef CONFIG_PM
	pm_register(PM_SYS_DEV, PM_SYS_UNKNOWN, s3c64xx_pm_callback);
#endif
	return 0;
}

static int  s3c64xx_audio_remove(struct device * dev)
{
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	s3c64xx_ac97_put();
	return 0;
}

static struct device_driver s3c64xx_ac97_driver = {
    .name       = "s3c-ac97",
    .bus        = &platform_bus_type,
    .probe      = s3c64xx_audio_probe,
    .remove     = s3c64xx_audio_remove,
#ifdef CONFIG_PM
    .suspend    = s3c64xx_ac97_suspend,
    .resume     = s3c64xx_ac97_resume,
#endif
};

static int __init  s3c64xx_ac97_init(void)
{
	return driver_register(&s3c64xx_ac97_driver);
}

static void __exit  s3c64xx_ac97_exit(void)
{
	driver_unregister(&s3c64xx_ac97_driver);

}

module_init(s3c64xx_ac97_init);
module_exit(s3c64xx_ac97_exit);




