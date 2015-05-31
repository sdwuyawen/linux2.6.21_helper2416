/* linux/drivers/input/touchscreen/s3c-ts.c
 *
 * $Id: s3c-ts.c,v 1.9 2008/05/21 04:43:51 ihlee215 Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (c) 2004 Arnaud Patard <arnaud.patard@rtp-net.org>
 * iPAQ H1940 touchscreen support
 *
 * ChangeLog
 *
 * 2004-09-05: Herbert Potzl <herbert@13thfloor.at>
 *	- added clock (de-)allocation code
 *
 * 2005-03-06: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - h1940_ -> s3c24xx (this driver is now also used on the n30
 *        machines :P)
 *      - Debug messages are now enabled with the config option
 *        TOUCHSCREEN_S3C_DEBUG
 *      - Changed the way the value are read
 *      - Input subsystem should now work
 *      - Use ioremap and readl/writel
 *
 * 2005-03-23: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - Make use of some undocumented features of the touchscreen
 *        controller
 *
 * 2006-09-05: Ryu Euiyoul <ryu.real@gmail.com>
 *      - added power management suspend and resume code
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
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>

#include <asm/arch/regs-adc.h>
#include <asm/arch/irqs.h>

//#define CONFIG_TOUCHSCREEN_S3C_DEBUG
#undef CONFIG_TOUCHSCREEN_S3C_DEBUG

/* For ts->dev.id.version */
#define S3C_TSVERSION	0x0101

#define WAIT4INT(x)  (((x)<<8) | \
		     S3C2410_ADCTSC_YM_SEN | S3C2410_ADCTSC_YP_SEN | S3C2410_ADCTSC_XP_SEN | \
		     S3C2410_ADCTSC_XY_PST(3))

#define AUTOPST	     (S3C2410_ADCTSC_YM_SEN | S3C2410_ADCTSC_YP_SEN | S3C2410_ADCTSC_XP_SEN | \
		     S3C2410_ADCTSC_AUTO_PST | S3C2410_ADCTSC_XY_PST(0))


#define DEBUG_LVL    KERN_DEBUG

struct s3c_ts_mach_info {
       int             delay;
       int             presc;
       int             oversampling_shift;
       int	       resol_bit;
};

#define	OVER_SAMPLE_SHIFT	3
/* Touchscreen default configuration */
struct s3c_ts_mach_info s3c_ts_cfg __initdata = {
                .delay = 65535,
                .presc = 255,
                .oversampling_shift = OVER_SAMPLE_SHIFT,
#if defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) || defined(CONFIG_CPU_S3C6410)
		.resol_bit = 12,
#else
		.resol_bit = 10,
#endif
};


/*
 * Definitions & global arrays.
 */
static char *s3c_ts_name = "s3c TouchScreen";


/*
 * Per-touchscreen data.
 */
struct s3c_ts {
	struct input_dev *dev;
	long xp;
	long yp;
	int count;
	int shift;
	char phys[32];
};

static struct s3c_ts *ts;
static void __iomem *base_addr;
static struct clk	*adc_clock;
static struct point {
	int	xp;
	int	yp;
} point_array[1<<OVER_SAMPLE_SHIFT];

void filter_abnormal(void)
{
	int	i;
	int	min_x,max_x;
	int	min_y,max_y;
	struct point *apt = point_array;
	int	*S;	// variance
	int	valid_cnt = 0;

	min_x = max_x = apt[0].xp;
	min_y = max_y = apt[0].yp;

#if 1
	S = kmalloc(ts->count * sizeof(*S), GFP_KERNEL);
	S[0] = abs(apt[0].xp - apt[1].xp)*2;
	for(i=1; i<ts->count-1; i++)
	{
		S[i] = abs(apt[i-1].xp - apt[i].xp) + abs(apt[i+1].xp - apt[i].xp);
	}
	S[ts->count-1] = abs(apt[ts->count-1].xp - apt[ts->count-2].xp)*2;

	ts->xp = 0;
	
	for(i=0; i<ts->count; i++)
	{
		if(S[i] < 30)
		{
			ts->xp += apt[i].xp;
			valid_cnt++;
		}
	}
	if(valid_cnt)
	{
		ts->xp /= valid_cnt;
	}
	else
	{
		int	tmp;
		tmp = S[0];
		for(i=1; i<ts->count; i++)
		{
			if(tmp > S[i])
			{
				tmp = S[i];
				ts->xp= apt[i].xp;
			}
		}
	}

	
	S[0] = abs(apt[0].yp - apt[1].yp)*2;
	for(i=1; i<ts->count-1; i++)
	{
		S[i] = abs(apt[i-1].yp - apt[i].yp) + abs(apt[i+1].yp - apt[i].yp);
	}
	S[ts->count-1] = abs(apt[ts->count-1].yp - apt[ts->count-2].yp)*2;

	ts->yp = 0;
	valid_cnt = 0;
	for(i=0; i<ts->count; i++)
	{
		if(S[i] < 160)
		{
			ts->yp += apt[i].yp;
			valid_cnt++;
		}
	}
	if(valid_cnt)
	{
		ts->yp /= valid_cnt;
	}
	else
	{
		int	tmp;

		tmp = S[0];
		for(i=1; i<ts->count; i++)
		{
			if(tmp > S[i])
			{
				tmp = S[i];
				ts->yp = apt[i].yp;
			}
		}
	}

	kfree(S);

#else
	for(i=1; i<ts->count; i++)
	{
		ts->xp += apt[i].xp;
		ts->yp += apt[i].yp;
		min_x = min(min_x, apt[i].xp);
		min_y = min(min_y, apt[i].yp);
		max_x = max(min_x, apt[i].xp);
		max_y = max(min_y, apt[i].yp);
//		printk(KERN_INFO "X: %03ld, Y: %03ld\n", apt[i].xp, apt[i].yp);
	}
	ts->xp -= (min_x + max_x);
	ts->yp -= (min_y + max_y);
//	printk(KERN_INFO "iX: %03ld, Y: %03ld, aX: %03ld, Y: %03ld\n", min_x, min_y, max_x, max_y);
#endif
}

static void touch_timer_fire(unsigned long data)
{
	unsigned long data0;
	unsigned long data1;
	int updown;

	data0 = readl(base_addr+S3C2410_ADCDAT0);
	data1 = readl(base_addr+S3C2410_ADCDAT1);

	updown = (!(data0 & S3C2410_ADCDAT0_UPDOWN)) && (!(data1 & S3C2410_ADCDAT1_UPDOWN));

	if (updown) {
		if (ts->count) {

			filter_abnormal();
#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
			{
				struct timeval tv;
				do_gettimeofday(&tv);
//				printk(KERN_INFO "T: %06d, X: %03ld, Y: %03ld\n", (int)tv.tv_usec, ts->xp, ts->yp);
			}
#endif

			input_report_abs(ts->dev, ABS_X, ts->xp);
			input_report_abs(ts->dev, ABS_Y, ts->yp);

			input_report_key(ts->dev, BTN_TOUCH, 1);
			input_report_abs(ts->dev, ABS_PRESSURE, 1);
			input_sync(ts->dev);
		}

		ts->xp = 0;
		ts->yp = 0;
		ts->count = 0;

		writel(S3C2410_ADCTSC_PULL_UP_DISABLE | AUTOPST, base_addr+S3C2410_ADCTSC);
		writel(readl(base_addr+S3C2410_ADCCON) | S3C2410_ADCCON_ENABLE_START, base_addr+S3C2410_ADCCON);
	}
	else {

		ts->count = 0;

//		input_report_key(ts->dev, BTN_TOUCH, 0);
		input_report_abs(ts->dev, ABS_PRESSURE, 0);
		input_sync(ts->dev);

		writel(WAIT4INT(0), base_addr+S3C2410_ADCTSC);
	}
}

static struct timer_list touch_timer =
		TIMER_INITIALIZER(touch_timer_fire, 0, 0);

static irqreturn_t stylus_updown(int irqno, void *param)
{
	unsigned long data0;
	unsigned long data1;
	int updown;

	
	data0 = readl(base_addr+S3C2410_ADCDAT0);
	data1 = readl(base_addr+S3C2410_ADCDAT1);

	updown = (!(data0 & S3C2410_ADCDAT0_UPDOWN)) && (!(data1 & S3C2410_ADCDAT1_UPDOWN));

#ifdef CONFIG_TOUCHSCREEN_S3C_DEBUG
       printk(KERN_INFO "   %c\n",	updown ? 'D' : 'U');
#endif

	/* TODO we should never get an interrupt with updown set while
	 * the timer is running, but maybe we ought to verify that the
	 * timer isn't running anyways. */

	if (updown)
		touch_timer_fire(0);

#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
        __raw_writel(0x0, base_addr+S3C6400_ADCCLRWK);
        __raw_writel(0x0, base_addr+S3C6400_ADCCLRINT);
#endif
        
	return IRQ_HANDLED;
}

static irqreturn_t stylus_action(int irqno, void *param)
{
	unsigned long data0;
	unsigned long data1;

	data0 = readl(base_addr+S3C2410_ADCDAT0);
	data1 = readl(base_addr+S3C2410_ADCDAT1);

#if defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) || defined(CONFIG_CPU_S3C6410)
//	ts->xp += data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
//	ts->yp += data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;
	point_array[ts->count].xp = data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT;
	point_array[ts->count].yp = data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT;
#else
	ts->xp += data0 & S3C2410_ADCDAT0_XPDATA_MASK;
	ts->yp += data1 & S3C2410_ADCDAT1_YPDATA_MASK;
#endif

	ts->count++;

	if (ts->count < (1<<ts->shift)) {
		writel(S3C2410_ADCTSC_PULL_UP_DISABLE | AUTOPST, base_addr+S3C2410_ADCTSC);
		writel(readl(base_addr+S3C2410_ADCCON) | S3C2410_ADCCON_ENABLE_START, base_addr+S3C2410_ADCCON);
	} else {
		mod_timer(&touch_timer, jiffies+2);
		writel(WAIT4INT(1), base_addr+S3C2410_ADCTSC);
	}

#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
        __raw_writel(0x0, base_addr+S3C6400_ADCCLRWK);
        __raw_writel(0x0, base_addr+S3C6400_ADCCLRINT);
#endif
	
	return IRQ_HANDLED;
}

/*
 * The functions for inserting/removing us as a module.
 */
static int __init s3c_ts_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	int err;

	adc_clock = clk_get(NULL, "adc");
	if (!adc_clock) {
		printk(KERN_ERR "failed to get adc clock source\n");
		return -ENOENT;
	}
	clk_enable(adc_clock);

	base_addr=ioremap(S3C24XX_PA_ADC, 0x20);

	if (base_addr == NULL) {
		printk(KERN_ERR "Failed to remap register block\n");
		return -ENOMEM;
	}

	if ((s3c_ts_cfg.presc&0xff) > 0)
		writel(S3C2410_ADCCON_PRSCEN | S3C2410_ADCCON_PRSCVL(s3c_ts_cfg.presc&0xFF),\
				base_addr+S3C2410_ADCCON);
	else
		writel(0, base_addr+S3C2410_ADCCON);


	/* Initialise registers */
	if ((s3c_ts_cfg.delay&0xffff) > 0)
		writel(s3c_ts_cfg.delay & 0xffff, base_addr+S3C2410_ADCDLY);

	if (s3c_ts_cfg.resol_bit == 12) {
#if defined(CONFIG_CPU_S3C6410)
		writel(readl(base_addr+S3C2410_ADCCON)|S3C6410_ADCCON_RESSEL_12BIT, base_addr+S3C2410_ADCCON);
#elif defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) 
		writel(readl(base_addr+S3C2410_ADCCON)|S3C2450_ADCCON_RESSEL_12BIT, base_addr+S3C2410_ADCCON);
#endif
	}

	writel(WAIT4INT(0), base_addr+S3C2410_ADCTSC);

	ts = kzalloc(sizeof(struct s3c_ts), GFP_KERNEL);
	
	input_dev = input_allocate_device();

	if (!input_dev) {
		err = -ENOMEM;
		goto fail;
	}
	ts->dev = input_dev;

	ts->dev->evbit[0] = ts->dev->evbit[0] = BIT(EV_SYN) | BIT(EV_KEY) | BIT(EV_ABS);
	ts->dev->keybit[LONG(BTN_TOUCH)] = BIT(BTN_TOUCH);

#if defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) || defined(CONFIG_CPU_S3C6410)
	input_set_abs_params(ts->dev, ABS_X, 0, 0xFFF, 0, 0);
	input_set_abs_params(ts->dev, ABS_Y, 0, 0xFFF, 0, 0);
#else
	input_set_abs_params(ts->dev, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(ts->dev, ABS_Y, 0, 0x3FF, 0, 0);
#endif
	input_set_abs_params(ts->dev, ABS_PRESSURE, 0, 1, 0, 0);

	sprintf(ts->phys, "input(ts)");

	ts->dev->private = ts;
	ts->dev->name = s3c_ts_name;
	ts->dev->phys = ts->phys;
	ts->dev->id.bustype = BUS_RS232;
	ts->dev->id.vendor = 0xDEAD;
	ts->dev->id.product = 0xBEEF;
	ts->dev->id.version = S3C_TSVERSION;

	ts->shift = s3c_ts_cfg.oversampling_shift;

	/* Get irqs */
	if (request_irq(IRQ_ADC, stylus_action, SA_SAMPLE_RANDOM, "s3c_action", ts->dev)) {
		printk(KERN_ERR "s3c_ts.c: Could not allocate ts IRQ_ADC !\n");
		iounmap(base_addr);
		return -EIO;
	}

	if (request_irq(IRQ_TC, stylus_updown, SA_SAMPLE_RANDOM, "s3c_updown", ts->dev)) {
		printk(KERN_ERR "s3c_ts.c: Could not allocate ts IRQ_TC !\n");
		iounmap(base_addr);
		return -EIO;
	}

	printk(KERN_INFO "%s got loaded successfully : %d bits\n", s3c_ts_name, s3c_ts_cfg.resol_bit);

	/* All went ok, so register to the input system */
	err = input_register_device(ts->dev);
	
	if(err) {
		free_irq(IRQ_TC, ts->dev);
		free_irq(IRQ_ADC, ts->dev);
		clk_disable(adc_clock);
		iounmap(base_addr);
		return -EIO;
	}

	return 0;

fail:	input_free_device(input_dev);
	kfree(ts);
	return err;
}

static int s3c_ts_remove(struct platform_device *dev)
{
	printk(KERN_INFO "s3c_ts_remove() of TS called !\n");

	disable_irq(IRQ_ADC);
	disable_irq(IRQ_TC);
	free_irq(IRQ_TC, ts->dev);
	free_irq(IRQ_ADC, ts->dev);

	if (adc_clock) {
		clk_disable(adc_clock);
		clk_put(adc_clock);
		adc_clock = NULL;
	}

	input_unregister_device(ts->dev);
	iounmap(base_addr);

	return 0;
}

#ifdef CONFIG_PM
static unsigned int adccon, adctsc, adcdly;

static int s3c_ts_suspend(struct platform_device *dev, pm_message_t state)
{
	adccon = readl(base_addr+S3C2410_ADCCON);
	adctsc = readl(base_addr+S3C2410_ADCTSC);
	adcdly = readl(base_addr+S3C2410_ADCDLY);

	disable_irq(IRQ_ADC);
	disable_irq(IRQ_TC);

	clk_disable(adc_clock);

	return 0;
}

static int s3c_ts_resume(struct platform_device *pdev)
{
	clk_enable(adc_clock);

	writel(adccon, base_addr+S3C2410_ADCCON);
	writel(adctsc, base_addr+S3C2410_ADCTSC);
	writel(adcdly, base_addr+S3C2410_ADCDLY);
	writel(WAIT4INT(0), base_addr+S3C2410_ADCTSC);

	enable_irq(IRQ_ADC);
	enable_irq(IRQ_TC);

	return 0;
}
#else
#define s3c_ts_suspend NULL
#define s3c_ts_resume  NULL
#endif

static struct platform_driver s3c_ts_driver = {
       .probe          = s3c_ts_probe,
       .remove         = s3c_ts_remove,
       .suspend        = s3c_ts_suspend,
       .resume         = s3c_ts_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c2410-adc",
	},
};

static char banner[] __initdata = KERN_INFO "S3C Touchscreen driver, (c) 2007 Samsung Electronics\n";

static int __init s3c_ts_init(void)
{
	printk("%s", banner);
	return platform_driver_register(&s3c_ts_driver);
}

static void __exit s3c_ts_exit(void)
{
	platform_driver_unregister(&s3c_ts_driver);
}

//module_init(s3c_ts_init);
subsys_initcall(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_AUTHOR("Samsung AP");
MODULE_DESCRIPTION("s3c touchscreen driver");
MODULE_LICENSE("GPL");
