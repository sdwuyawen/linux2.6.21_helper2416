/* linux/arch/arm/mach-s3c64xx/pwm.c
 *
 * (c) 2003-2005 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S3C64XX PWM core
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Changelog:
 * This file is based on the Sangwook Lee/Samsung patches, re-written due
 * to various ommisions from the code (such as flexible pwm configuration)
 * for use with the BAST system board.
 *
 *
 */
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>

#include <asm/arch/regs-timer.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/irqs.h>
#include "pwm-s3c2450.h"


s3c2450_pwm_chan_t s3c_chans[S3C_PWM_CHANNELS];


static inline void
s3c2450_pwm_buffdone(s3c2450_pwm_chan_t *chan, void *dev)
{

	if (chan->callback_fn != NULL) {
		(chan->callback_fn)( dev);
	}
}


static int s3c2450_pwm_start (int channel)
{
	unsigned long tcon;
	tcon = __raw_readl(S3C2410_TCON);
	switch(channel)
	{
	case 0:
		tcon |= S3C2410_TCON_T0START;
		tcon &= ~S3C2410_TCON_T0MANUALUPD;
	break;
	case 1:
		tcon |= S3C2410_TCON_T1START;
		tcon &= ~S3C2410_TCON_T1MANUALUPD;
	break;
	case 2:
		tcon |= S3C2410_TCON_T2START;
		tcon &= ~S3C2410_TCON_T2MANUALUPD;
	break;
	case 3:
		tcon |= S3C2410_TCON_T3START;
		tcon &= ~S3C2410_TCON_T3MANUALUPD;
	break;

	}
	__raw_writel(tcon, S3C2410_TCON);

	return 0;
}


int s3c2450_timer_setup (int channel, int usec, unsigned long g_tcnt, unsigned long g_tcmp)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcmp;
	unsigned long tcfg1;
	unsigned long tcfg0;
	unsigned long pclk;
	struct clk *clk;

	printk("\n\nPWM channel %d set g_tcnt = %ld, g_tcmp = %ld \n\n", channel, g_tcnt, g_tcmp);

	tcnt = 0xffffffff;  /* default value for tcnt */
	tcmp = 0;

	/* read the current timer configuration bits */
	tcon = __raw_readl(S3C2410_TCON);
	tcfg1 = __raw_readl(S3C2410_TCFG1);
	tcfg0 = __raw_readl(S3C2410_TCFG0);

	clk = clk_get(NULL, "timers");
	if (IS_ERR(clk))
		panic("failed to get clock for pwm timer");

	clk_enable(clk);

	pclk = clk_get_rate(clk);

	/* configure clock tick */

	switch(channel)
	{
		case 0:
			/* set gpio as PWM TIMER0 to signal output*/
			s3c2410_gpio_cfgpin(S3C2410_GPB0, S3C2410_GPB0_TOUT0);
			s3c2410_gpio_setpin(S3C2410_GPB0, 0);
			tcfg1 &= ~S3C2410_TCFG1_MUX0_MASK;
			tcfg1 |= S3C2410_TCFG1_MUX0_DIV2;

			tcfg0 &= ~S3C2410_TCFG_PRESCALER0_MASK;
			tcfg0 |= (4) << 0;
			tcon &= ~(7<<0);
			tcon |= S3C2410_TCON_T0RELOAD;
			break;

		case 1:
			/* set gpio as PWM TIMER1 to signal output*/
			s3c2410_gpio_cfgpin(S3C2410_GPB1, S3C2410_GPB1_TOUT1);
			s3c2410_gpio_setpin(S3C2410_GPB1, 0);
			tcfg1 &= ~S3C2410_TCFG1_MUX1_MASK;
			tcfg1 |= S3C2410_TCFG1_MUX1_DIV2;

			tcfg0 &= ~S3C2410_TCFG_PRESCALER0_MASK;
			tcfg0 |= (4) << 0;

			tcon &= ~(7<<8);
			tcon |= S3C2410_TCON_T1RELOAD;
			break;
		case 2:
			s3c2410_gpio_cfgpin(S3C2410_GPB2, S3C2410_GPB2_TOUT2);
			s3c2410_gpio_setpin(S3C2410_GPB2, 0);
			tcfg1 &= ~S3C2410_TCFG1_MUX2_MASK;
			tcfg1 |= S3C2410_TCFG1_MUX2_DIV2;

			tcfg0 &= ~S3C2410_TCFG_PRESCALER1_MASK;
			tcfg0 |= (4) << S3C2410_TCFG_PRESCALER1_SHIFT;

			tcon &= ~(7<<12);
			tcon |= S3C2410_TCON_T2RELOAD;
			break;
		case 3:
			s3c2410_gpio_cfgpin(S3C2410_GPB3, S3C2410_GPB3_TOUT3);
			s3c2410_gpio_setpin(S3C2410_GPB3, 0);
			tcfg1 &= ~S3C2410_TCFG1_MUX3_MASK;
			tcfg1 |= S3C2410_TCFG1_MUX3_DIV2;

			tcfg0 &= ~S3C2410_TCFG_PRESCALER1_MASK;
			tcfg0 |= (4) << S3C2410_TCFG_PRESCALER1_SHIFT;
			tcon &= ~(7<<16);
			tcon |= S3C2410_TCON_T3RELOAD;
			break;
	}

#if 0

	tcnt = (pclk / ((PRESCALER)*DIVIDER)) / usec;

	printk("s3c2450 pwm timer tcnt=0x%08lx, pclk=0x%08lx, PRESCALER=%d, DIVIDER=%d, usec=%d\n",
	       tcnt, pclk, PRESCALER, DIVIDER, usec);

	/* timers reload after counting zero, so reduce the count by 1 */

	tcnt--;
#endif

	__raw_writel(tcfg1, S3C2410_TCFG1);
	__raw_writel(tcfg0, S3C2410_TCFG0);

	__raw_writel(tcon, S3C2410_TCON);
#if 0
	if (tcnt > 0xffffffff) {
		panic("setup_timer: HZ is too small, cannot configure timer!");
		return 0;
	}
#endif
	__raw_writel(tcnt, S3C2410_TCNTB(channel));
	__raw_writel(tcmp, S3C2410_TCMPB(channel));

	switch(channel)
	{
		case 0:
			tcon |= S3C2410_TCON_T0MANUALUPD;
			break;
		case 1:
			tcon |= S3C2410_TCON_T1MANUALUPD;
			break;
		case 2:
			tcon |= S3C2410_TCON_T2MANUALUPD;
			break;
		case 3:
			tcon |= S3C2410_TCON_T3MANUALUPD;
			break;
	}
	__raw_writel(tcon, S3C2410_TCON);

	tcnt = g_tcnt;
	__raw_writel(tcnt, S3C2410_TCNTB(channel));

	tcmp = g_tcmp;
	__raw_writel(tcmp, S3C2410_TCMPB(channel));

	/* start the timer running */

	s3c2450_pwm_start (channel);

	return 0;
}


static irqreturn_t s3c2450_pwm_irq(int irq, void *devpw)
{
	s3c2450_pwm_chan_t *chan = (s3c2450_pwm_chan_t *)devpw;
	void *dev=chan->dev;

	/* modify the channel state */
	s3c2450_pwm_buffdone(chan, dev);

	return IRQ_HANDLED;
}


int s3c2450_pwm_request(pwmch_t  channel, s3c_pwm_client_t *client, void *dev)
{
	s3c2450_pwm_chan_t *chan = &s3c_chans[channel];
	unsigned long flags;
	int err;

	pr_debug("pwm%d: s3c_request_pwm: client=%s, dev=%p\n",
		 channel, client->name, dev);


	local_irq_save(flags);


	if (chan->in_use) {
		if (client != chan->client) {
			printk(KERN_ERR "pwm%d: already in use\n", channel);
			local_irq_restore(flags);
			return -EBUSY;
		} else {
			printk(KERN_ERR "pwm%d: client already has channel\n", channel);
		}
	}

	chan->client = client;
	chan->in_use = 1;
	chan->dev = dev;

	if (!chan->irq_claimed) {
		pr_debug("pwm%d: %s : requesting irq %d\n",
			 channel, __FUNCTION__, chan->irq);

		err = request_irq(chan->irq, s3c2450_pwm_irq, SA_INTERRUPT,
				  client->name, (void *)chan);

		if (err) {
			chan->in_use = 0;
			local_irq_restore(flags);

			printk(KERN_ERR "%s: cannot get IRQ %d for PWM %d\n",
			       client->name, chan->irq, chan->number);
			return err;
		}

		chan->irq_claimed = 1;
		chan->irq_enabled = 1;
	}

	local_irq_restore(flags);

	/* need to setup */

	pr_debug("%s: channel initialised, %p\n", __FUNCTION__, chan);

	return 0;
}

int s3c2450_pwm_free (pwmch_t channel, s3c_pwm_client_t *client)
{
	s3c2450_pwm_chan_t *chan = &s3c_chans[channel];
	unsigned long flags;


	local_irq_save(flags);

	if (chan->client != client) {
		printk(KERN_WARNING "pwm%d: possible free from different client (channel %p, passed %p)\n",
		       channel, chan->client, client);
	}

	/* sort out stopping and freeing the channel */


	chan->client = NULL;
	chan->in_use = 0;

	if (chan->irq_claimed)
		free_irq(chan->irq, (void *)chan);
	chan->irq_claimed = 0;

	local_irq_restore(flags);

	return 0;
}


int s3c2450_pwm_set_buffdone_fn(pwmch_t channel, s3c_pwm_cbfn_t rtn)
{
	s3c2450_pwm_chan_t *chan = &s3c_chans[channel];


	pr_debug("%s: chan=%d, callback rtn=%p\n", __FUNCTION__, channel, rtn);

	chan->callback_fn = rtn;

	return 0;
}


#define s3c2450_pwm_suspend NULL
#define s3c2450_pwm_resume  NULL

struct sysdev_class pwm_sysclass = {
	set_kset_name("s3c-pwm"),
	.suspend	= s3c2450_pwm_suspend,
	.resume		= s3c2450_pwm_resume,
};


/* initialisation code */

static int __init s3c2450_init_pwm(void)
{
	s3c2450_pwm_chan_t *cp;
	int channel;
	int ret;

	printk("S3C PWM Driver, (c) 2006-2007 Samsung Electronics\n");

	ret = sysdev_class_register(&pwm_sysclass);
	if (ret != 0) {
		printk(KERN_ERR "pwm sysclass registration failed\n");
		return -ENODEV;
	}

	for (channel = 0; channel < S3C_PWM_CHANNELS; channel++) {
		cp = &s3c_chans[channel];

		memset(cp, 0, sizeof(s3c2450_pwm_chan_t));

		cp->number = channel;
		/* pwm channel irqs are in order.. */
		cp->irq    = channel + IRQ_TIMER0;

		/* register system device */

		ret = sysdev_register(&cp->sysdev);

		pr_debug("PWM channel %d , irq %d\n",
		       cp->number,  cp->irq);
	}

//	s3c2450_timer_setup(3,10,1000,200);
	return ret;
}
__initcall(s3c2450_init_pwm);
