/*
 * Glue audio driver for the SA1111 compagnon chip & Philips UDA1341 codec.
 *
 * Copyright (C) 2007, Ryu Euiyoul <ryu.real@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2000-09-04	John Dorsey	SA-1111 Serial Audio Controller support
 * 				was initially added to the sa1100-uda1341.c
 * 				driver.
 *
 * 2001-06-03	Nicolas Pitre	Made this file a separate module, based on
 * 				the former sa1100-uda1341.c driver.
 *
 * 2001-09-23	Russell King	Remove old L3 bus driver.
 *
 * 2001-12-19	Nicolas Pitre	Moved SA1111 SAC support to this file where
 * 				it actually belongs (formerly in dma-sa1111.c
 * 				from John Dorsey).
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/l3/l3.h>
#include <linux/l3/uda1341.h>

#include <asm/semaphore.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/dma.h>
#include <asm/io.h>

#include <asm/arch/dma.h>
#include <asm/arch/regs-iis.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-clock.h>
#include <asm/arch/regs-s3c2443-clock.h>
#include "s3c24xx_pcm.h"

//#define DPRINTK  printk
#define DPRINTK( x... )

#define S_CLOCK_FREQ    384

static long 	audio_rate=44100;
static struct 	uda1341 *uda1341;
static struct 	clk	*iis_clock;
static DECLARE_MUTEX(s3c_lock);
static struct l3_adapter l3_s3c_adapter = {
	.owner	= THIS_MODULE,
	.name	= "l3-s3c",
	.lock	= &s3c_lock,
};

struct s3c24xx_i2s_info {
	void __iomem	*regs;
};
static struct s3c24xx_i2s_info s3c24xx_i2s;

static int
mixer_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	return uda1341_mixer_ctl(uda1341, cmd, (void *)arg);
}

static struct file_operations uda1341_mixer_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= mixer_ioctl,
};

static int iispsr_value(int sample_rate)
{
	int i, prescaler = 0;
	
	unsigned long fact0 = clk_get_rate(iis_clock)/(S_CLOCK_FREQ);
	unsigned long r0_sample_rate, r1_sample_rate = 0, r2_sample_rate;

	DPRINTK("requested sample_rate = %d\n", sample_rate);

	for(i = 1; i < 32; i++) {
		r1_sample_rate = fact0 / i;
		if(r1_sample_rate < sample_rate)
			break;
	}

	r0_sample_rate = fact0 / (i + 1);
	r2_sample_rate = fact0 / (i - 1);

	DPRINTK("calculated (%d-1) freq = %ld, error = %d\n",
		i + 1, r0_sample_rate, abs(r0_sample_rate - sample_rate));
	DPRINTK("calculated (%d-1) freq = %ld, error = %d\n",
		i, r1_sample_rate, abs(r1_sample_rate - sample_rate));
	DPRINTK("calculated (%d-1) freq = %ld, error = %d\n",
		i - 1, r2_sample_rate, abs(r2_sample_rate - sample_rate));

	prescaler = i;
	if(abs(r0_sample_rate - sample_rate) <
	   abs(r1_sample_rate - sample_rate))
		prescaler = i + 1;
	if(abs(r2_sample_rate - sample_rate) <
	   abs(r1_sample_rate - sample_rate))
		prescaler = i - 1;

	prescaler = max_t(int, 0, (prescaler - 1));

	DPRINTK("selected prescale value = %d, freq = %ld, error = %d\n",
	       prescaler, fact0 / (prescaler + 1),
	       abs((fact0 / (prescaler + 1)) - sample_rate));

	return prescaler;
}

static int s3c_init_iis(void)
{
	u32 val =0;

	val =  S3C2443_IISMOD_IMS_INTERNAL_MASTER | S3C2443_IISMOD_TXRXMODE | S3C2443_IISMOD_IIS
		 | S3C2443_IISMOD_384FS | S3C2443_IISMOD_32FS | S3C2443_IISMOD_16BIT;
 	writel(val,s3c24xx_i2s.regs + S3C2410_IISMOD);

	val =0;
	val = S3C2443_IISCON_TXDMAEN | S3C2443_IISCON_RXDMAEN | S3C2410_IISCON_IISEN;
	writel(val,s3c24xx_i2s.regs + S3C2410_IISCON);  

	val = 0 ;
	val =  (1<<15) ;	  
	writel(val,s3c24xx_i2s.regs + S3C2443_IISFIC);
	val =  (0<<15) ;	  
	writel(val,s3c24xx_i2s.regs + S3C2443_IISFIC);
        DPRINTK("IISMOD : 0x%08x, IISCON : 0x%08x, IISFIC : 0x%08x\n",
		readl(s3c24xx_i2s.regs + S3C2410_IISMOD),
		readl(s3c24xx_i2s.regs + S3C2410_IISCON),
		readl(s3c24xx_i2s.regs + S3C2443_IISFIC));

	return 0;
}

static long audio_set_dsp_speed(long val)
{
	unsigned long tmp;
	int prescaler = 0;

	DPRINTK("requested = %ld\n", val);
	tmp =  clk_get_rate(iis_clock)/S_CLOCK_FREQ; 
	 
	DPRINTK("requested = %ld, limit = %ld\n", val, tmp);
	if(val > (tmp >> 1))
		return -1;

	prescaler = iispsr_value(val);

	tmp  = (prescaler) | 1 << 15 ; /*prescale enable*/

	writel(tmp,s3c24xx_i2s.regs + S3C2443_IISPSR);
	audio_rate = val;
	DPRINTK("return audio_rate = %ld\n", (unsigned long)audio_rate);

	return audio_rate;
}

static int
s3c_audio_ioctl(struct inode *inode, struct file *file, uint cmd, ulong arg)
{
	long val = 0;
	int ret = 0;
	
	audio_state_t *state = file->private_data;

	s3c_init_iis();	

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
			ret = mixer_ioctl(inode, file, cmd, arg);
			break;
	}
	return ret;
}

static void s3c_audio_shutdown(void *dummy)
{
	uda1341_close(uda1341);
	/* Disable the I2S clock and L3 bus clock */
	clk_disable(iis_clock);
}

static void s3c_select_clkdiv(void)
{
#if defined CONFIG_CPU_S3C2443
	writel((readl(S3C2443_PCLKCON)|(1<<9)), S3C2443_PCLKCON);
	writel((readl(S3C2443_SCLKCON)|(1<<9)), S3C2443_SCLKCON);
	writel((readl(S3C2443_CLKDIV1) & ~(0xf << 12)),S3C2443_CLKDIV1);

	writel((readl(s3c24xx_i2s.regs + S3C2443_IISPSR)|(1<<15) |(5<<0)) ,s3c24xx_i2s.regs + S3C2443_IISPSR);
#elif defined CONFIG_CPU_S3C2412
	writel((readl(S3C_CLKCON)| (1 << 26)| (1<<13)),S3C_CLKCON);
	writel((readl(S3C_CLKDIVN) & ~(0xf << 12)),S3C_CLKDIVN);
	writel((readl(S3C_IISPSR)&0),S3C_IISPSR);
#endif 
}

static void s3c_iis_config_pins(void)
{
	s3c2410_gpio_cfgpin(S3C2410_GPE0, S3C2410_GPE0_I2SLRCK);
	s3c2410_gpio_cfgpin(S3C2410_GPE1, S3C2410_GPE1_I2SSCLK);
	s3c2410_gpio_cfgpin(S3C2410_GPE2, S3C2410_GPE2_CDCLK);
	s3c2410_gpio_cfgpin(S3C2410_GPE3, S3C2410_GPE3_I2SSDI);
	s3c2410_gpio_cfgpin(S3C2410_GPE4, S3C2410_GPE4_I2SSDO);
}

static int s3c_select_clk(void)
{
	iis_clock = clk_get(NULL, "iis");
	if (!iis_clock) {
		printk("failed to get iis clock source\n");
		return -ENOENT;
	}
	clk_enable(iis_clock);
	s3c_select_clkdiv();
	return 0;
}

static audio_stream_t output_stream = {
	id:	"s3c audio out ",
	dma:	DMACH_I2S_OUT,
};
static audio_stream_t input_stream = {
	id:	"s3c audio in ",
	dma:    DMACH_I2S_IN,
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
	struct uda1341_cfg cfg;
	int ret;

	s3c24xx_i2s.regs = ioremap(S3C24XX_PA_IIS, 0x100);
	if (s3c24xx_i2s.regs == NULL)
		return -ENXIO;

	ret = l3_add_adapter(&l3_s3c_adapter);
	if (ret)
		goto out;

	uda1341 = uda1341_attach("l3-bit-s3c-gpio");
	if (IS_ERR(uda1341)) {
		ret = PTR_ERR(uda1341);
		goto remove_l3;
	}
	cfg.fs     = 384;
	cfg.format = FMT_I2S;

	uda1341_open(uda1341);

	audio_dev_id = register_sound_dsp(&s3c_audio_fops, -1);
	mixer_dev_id = register_sound_mixer(&uda1341_mixer_fops, -1);

	printk(KERN_INFO "Sound uda1341: dsp id %d mixer id %d maped %p\n",
		audio_dev_id, mixer_dev_id, s3c24xx_i2s.regs);

	flush_scheduled_work();

	return 0;
remove_l3:
	uda1341_detach(uda1341);
	l3_del_adapter(&l3_s3c_adapter);
out:
	return ret;
}

static int s3c_audio_remove(struct device *_dev)
{
	unregister_sound_dsp(audio_dev_id);
	unregister_sound_mixer(mixer_dev_id);
	uda1341_detach(uda1341);
	l3_del_adapter(&l3_s3c_adapter);
	return 0;
}

#ifdef CONFIG_S3C2443_PM
static int smdk_audio_suspend( u32 state)
{
	return s3c_audio_suspend(&audio_state, state, SUSPEND_POWER_DOWN);
}

static int smdk_audio_resume()
{
	return s3c_audio_resume(&audio_state, RESUME_ENABLE);
}
#endif

static struct device_driver s3c_iis_driver = {
    .name       = "s3c2410-iis",
    .bus        = &platform_bus_type,
    .probe      = s3c_audio_probe,
    .remove     = s3c_audio_remove,
#ifdef CONFIG_S3C2443_PM
    .suspend    = smdk_audio_suspend,
    .resume     = smdk_audio_resume,
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

