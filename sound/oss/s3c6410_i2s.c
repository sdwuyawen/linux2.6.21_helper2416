/*
 * Glue audio driver for the SA1111 compagnon chip & Philips UDA1341 codec.
 *
 * Copyright (C) 2007, Ryu Euiyoul <ryu.real@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/l3/l3.h>
#include <linux/l3/uda1341.h>
#include <linux/platform_device.h>

#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/dma.h>
#include <asm/io.h>

#include <asm/arch/dma.h>
#include <asm/arch/regs-iis.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-s3c6400-clock.h>
#include "s3c6400_pcm.h"

#define DPRINTK( x... )
//#define DPRINTK  printk

#define S_CLOCK_FREQ    384
#define CONFIG_IIS_24BIT

static DECLARE_MUTEX(s3c_lock);

static int
mixer_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	printk("Warning: Set the mixer at %s, %d\n",__FUNCTION__,__LINE__);
	//return uda1341_mixer_ctl(uda1341, cmd, (void *)arg);
	return 0;
}

static struct file_operations uda1341_mixer_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= mixer_ioctl,
};

static int l3_gpio_set(void)
{
	s3c_gpio_cfgpin(S3C_GPN15, S3C_GPN15_OUTP); /* DAT */
	s3c_gpio_cfgpin(S3C_GPN14, S3C_GPN14_OUTP); /* MOD */
	s3c_gpio_cfgpin(S3C_GPN13, S3C_GPN13_OUTP); /* CLK */

	s3c_gpio_setpin(S3C_GPN15, 0);
	s3c_gpio_setpin(S3C_GPN14, 1);
	s3c_gpio_setpin(S3C_GPN13, 1);

	return 0;
}

static int l3_uda1342_set_addr(u8 dat)
{
	u32 i, j;

	s3c_gpio_setpin(S3C_GPN13, 1);
	s3c_gpio_setpin(S3C_GPN15, 0);
	s3c_gpio_setpin(S3C_GPN14, 0);
	
	for(j=0; j<4; j++);

	for(i=0; i<8; i++){
		if(dat & 0x1){
			s3c_gpio_setpin(S3C_GPN13, 0);
			s3c_gpio_setpin(S3C_GPN15, 1);

			for(j=0; j<4; j++);

			s3c_gpio_setpin(S3C_GPN13, 1);
			s3c_gpio_setpin(S3C_GPN15, 1);

			for(j=0; j<4; j++);
		} else{

			s3c_gpio_setpin(S3C_GPN13, 0);
			s3c_gpio_setpin(S3C_GPN15, 0);

			for(j=0; j<4; j++);

			s3c_gpio_setpin(S3C_GPN13, 1);
			s3c_gpio_setpin(S3C_GPN15, 0);

			for(j=0; j<4; j++);
		}	
		dat >>= 1;
	}

	s3c_gpio_setpin(S3C_GPN15, 0);
	s3c_gpio_setpin(S3C_GPN13, 1);
	s3c_gpio_setpin(S3C_GPN14, 1);

	return 0;
}

static int l3_uda1342_set_data(u8 dat)
{
	u32 i, j;

	s3c_gpio_setpin(S3C_GPN15, 0);
	s3c_gpio_setpin(S3C_GPN13, 1);
	s3c_gpio_setpin(S3C_GPN14, 1);
	
	for(j=0; j<4; j++);

	for(i=0; i<8; i++){
		if(dat & 0x1){
			s3c_gpio_setpin(S3C_GPN13, 0);
			s3c_gpio_setpin(S3C_GPN15, 1);

			for(j=0; j<4; j++);

			s3c_gpio_setpin(S3C_GPN13, 1);
			s3c_gpio_setpin(S3C_GPN15, 1);

			for(j=0; j<4; j++);
		} else{

			s3c_gpio_setpin(S3C_GPN13, 0);
			s3c_gpio_setpin(S3C_GPN15, 0);

			for(j=0; j<4; j++);

			s3c_gpio_setpin(S3C_GPN13, 1);
			s3c_gpio_setpin(S3C_GPN15, 0);

			for(j=0; j<4; j++);
		}	
		dat >>= 1;
	}

	s3c_gpio_setpin(S3C_GPN13, 1);
	s3c_gpio_setpin(S3C_GPN14, 1);
	s3c_gpio_setpin(S3C_GPN15, 0);

	return 0;
}

static int l3_uda1342_init(void)
{
	l3_uda1342_set_addr(0x12);
	l3_uda1342_set_data(0x00);
	l3_uda1342_set_data(0x45);
	l3_uda1342_set_data(0x44);

	l3_uda1342_set_addr(0x12);
	l3_uda1342_set_data(0x00);
	l3_uda1342_set_data(0x44);
	l3_uda1342_set_data(0x44);

	return 0;
}

static int l3_uda1342_rec_init(void)
{
	l3_uda1342_set_addr(0x12);
	l3_uda1342_set_data(0x80);
	l3_uda1342_set_data(0x0);
	l3_uda1342_set_data(0x14);

	l3_uda1342_set_addr(0x12);
	l3_uda1342_set_data(0x04);
	l3_uda1342_set_data(0xC0); /* 0xC0: ADC input amplifire gain 9dB */
	l3_uda1342_set_data(0x00); /* 0x0 : ADC mixer gain 0dB */

	return 0;
}

static long audio_set_dsp_speed(long val, int f_mode)
{
	//u32 iispsr = 0,  prescaler = 0;
	u32 iiscon = 0, iismod = 0, iisfic = 0;

	DPRINTK("%s : requested = %ld, f_mode = 0x%x\n",
		       __FUNCTION__, val, f_mode);

	l3_gpio_set();

	l3_uda1342_init();

	if ( f_mode & FMODE_READ)
		l3_uda1342_rec_init();

#if 0
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
#endif

	/* FIFO is flushed before operation */
	iisfic = S3C_IIS_TX_FLUSH | S3C_IIS_RX_FLUSH;
	writel(iisfic, S3C_IIS0FIC);
	writel(0, S3C_IIS0FIC);
#if 0
	/* Clear I2S prescaler value [13:8] and disable prescaler */
	iispsr = readl(S3C_IIS0PSR);
	iispsr &= ~((0x3f << 8) | (1 << 15));
	writel(iispsr, S3C_IIS0PSR);
#endif

	/* Configure I2SMOD */
	iismod = S3C_IIS0MOD_IMS_EXTERNAL_MASTER | S3C_IIS0MOD_TXRXMODE
	    | S3C_IIS0MOD_IIS;
	iismod |= S3C_IIS0MOD_384FS;

#ifdef CONFIG_IIS_24BIT
        iismod |= S3C_IIS0MOD_48FS;
	iismod |= S3C_IIS0MOD_24BIT;
#else
        iismod |= S3C_IIS0MOD_32FS;
	iismod |= S3C_IIS0MOD_16BIT;
#endif
	writel(iismod, S3C_IIS0MOD);
#if 0
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
#endif
//	writel(readl(S3C_IIS0PSR) | (prescaler << 8) | (1 << 15),
	//writel(0x8020, S3C_IIS0PSR);

	iiscon = S3C_IIS0CON_TXDMACTIVE	| S3C_IIS0CON_RXDMACTIVE;
	iiscon |= S3C_IIS0CON_I2SACTIVE;
//	iiscon |= (1<<16); /* underrun interrupt enable */

#if 0
	DPRINTK("%s, IISCON: 0x%08x IISMOD: 0x%08x IISFIC : 0x%08x IISPSR: 0x%08x\n",
		__FUNCTION__, readl(S3C_IIS0CON), 
		readl(S3C_IIS0MOD), readl(S3C_IIS0FIC), readl(S3C_IIS0PSR));
#endif

	writel(iiscon, S3C_IIS0CON);
	return val;
}

static int s3c_audio_ioctl(struct inode *inode, 
		struct file *file, uint cmd, ulong arg)
{
	long val = 0;
	int ret = 0;
	
	audio_state_t *state = file->private_data;

	DPRINTK("%s : #### sound_mode = 0x%x\n", __FUNCTION__, state->sound_mode);
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
			audio_set_dsp_speed(val, file->f_mode);
			break;

		case SOUND_PCM_READ_RATE:
			ret = put_user(val, (long *) arg);
			break;
#if 0
		case SNDCTL_DSP_SETFMT:
		case SNDCTL_DSP_GETFMTS:
			/* we can do 16-bit only */
			ret = put_user(AFMT_S16_LE, (long *) arg);
			break;
#endif
		default:
			/* Maybe this is meant for the mixer (as per OSS Docs) */
//			ret = mixer_ioctl(inode, file, cmd, arg);
			break;
	}
	return ret;
}

static void s3c_audio_shutdown(void *dummy)
{
	unsigned long iiscon;

	/* close I2S port */
	s3c_gpio_cfgpin(S3C_GPD0, S3C_GPD0_INP);
	s3c_gpio_cfgpin(S3C_GPD1, S3C_GPD1_INP);
	s3c_gpio_cfgpin(S3C_GPD2, S3C_GPD2_INP);
	s3c_gpio_cfgpin(S3C_GPD3, S3C_GPD3_INP);
	s3c_gpio_cfgpin(S3C_GPD4, S3C_GPD4_INP);

	s3c_gpio_pullup(S3C_GPD0, 0);
	s3c_gpio_pullup(S3C_GPD1, 0);
	s3c_gpio_pullup(S3C_GPD2, 0);
	s3c_gpio_pullup(S3C_GPD3, 0);
	s3c_gpio_pullup(S3C_GPD4, 0);

	/* deactivate I2S */
	iiscon = readl(S3C_IIS0CON);
	iiscon &= !S3C_IIS0CON_I2SACTIVE;
	writel(iiscon, S3C_IIS0CON);

	DPRINTK("#### IISCON: 0x%08x IISMOD: 0x%08x IISPSR: 0x%08x\n",
	    readl(S3C_IIS0CON), readl(S3C_IIS0MOD), readl(S3C_IIS0PSR));
}

static int s3c_select_clk(void)
{
	/* PCLK & SCLK gating enable */
	writel(readl(S3C_PCLK_GATE) | S3C_CLKCON_PCLK_IIS0, S3C_PCLK_GATE);
	writel(readl(S3C_SCLK_GATE) | S3C_CLKCON_SCLK_AUDIO0,
		       	S3C_SCLK_GATE);

	return 0;
}

static int s3c_iis_config_pins(void)
{
	/*Set I2S port to controll UDA1342 codec */
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
	subchannel:	DMACH_I2S_OUT,
};

static audio_stream_t input_stream = {
      	id: "s3c64xx audio in ", 
	subchannel:    DMACH_I2S_IN,
};

static audio_state_t audio_state = {
	.output_stream	= &output_stream,
	.input_stream	= &input_stream,
	.hw_shutdown	= s3c_audio_shutdown,
	.client_ioctl	= s3c_audio_ioctl,
};

static int s3c_audio_open(struct inode *inode, struct file *file)
{
	s3c_iis_config_pins();

	s3c_select_clk();

	init_MUTEX(&audio_state.sem);

        return s3c_audio_attach(inode, file, &audio_state);
}

static irqreturn_t s3c_iis_irq(int irqno, void *dev_id)
{
	u32 iiscon;
	
	iiscon = readl(S3C_IIS0CON);
	if((1<<17) & iiscon) {
		writel((1<<17)| iiscon, S3C_IIS0CON);
	}
	printk("%s: %d IISCON = 0x%08x\n",__FUNCTION__, __LINE__,readl(S3C_IIS0CON));

	return IRQ_HANDLED;
}

/*
 * Missing fields of this structure will be patched with the call
 * to s3c_audio_attach().
 */
static struct file_operations s3c_audio_fops = {
	.owner	= THIS_MODULE,
	.open	= s3c_audio_open,
};

static int audio_dev_id, mixer_dev_id;

static int s3c_audio_probe(struct device *_dev)
{
	int ret;

	ret = request_irq(IRQ_IIS, s3c_iis_irq, SA_INTERRUPT,
			 "s3c2410-iis", NULL);

	if (ret != 0) {
		printk("IIS: cannot claim IRQ\n");
		return 0;
	}

	audio_dev_id = register_sound_dsp(&s3c_audio_fops, -1);
	mixer_dev_id = register_sound_mixer(&uda1341_mixer_fops, -1);

	printk(KERN_INFO "Sound uda1342: dsp id %d mixer id %d\n",
		audio_dev_id, mixer_dev_id);

	flush_scheduled_work();

	return 0;
}

static int s3c_audio_remove(struct device *_dev)
{
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	return 0;
}

#ifdef CONFIG_S3C64XX_PM
static int s3c64xx_audio_suspend( u32 state)
{
	return s3c_audio_suspend(&audio_state, state, SUSPEND_POWER_DOWN);
}

static int s3c64xx_audio_resume()
{
	return s3c64xx_audio_resume(&audio_state, RESUME_ENABLE);
}
#endif

static struct device_driver s3c_iis_driver = {
    .name       = "s3c2410-iis",
    .bus        = &platform_bus_type,
    .probe      = s3c_audio_probe,
    .remove     = s3c_audio_remove,
#ifdef CONFIG_S3C64XX_PM
    .suspend    = s3c64xx_audio_suspend,
    .resume     = s3c64xx_audio_resume,
#endif
};

static int __init s3c_uda1341_init(void)
{
        DPRINTK("Sound: %s installed\n", __FUNCTION__);

	return driver_register(&s3c_iis_driver);
}

static void s3c_uda1341_exit(void)
{
	driver_unregister(&s3c_iis_driver);

}

module_init(s3c_uda1341_init);
module_exit(s3c_uda1341_exit);

