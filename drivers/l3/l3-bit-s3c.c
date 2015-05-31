/*
 *  linux/drivers/l3/l3-bit-sa1100.c
 *
 *  Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  This is a combined I2C and L3 bus driver.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <linux/l3/algo-bit.h>

#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/arch/regs-gpio.h>

#define NAME "l3-bit-s3c-gpio"

struct bit_data {
	unsigned int	sda;
	unsigned int	scl;
	unsigned int	l3_mode;
};

static int getsda(void *data)
{
	struct bit_data *bits = data;
	return s3c2410_gpio_getpin(bits->sda);
}

static DECLARE_MUTEX(l3_lock);
#define LOCK        &l3_lock


/*
 * iPAQs need the clock line driven hard high and low.
 */

static void l3_setscl(void *data, int state)
{
	struct bit_data *bits = data;
	unsigned long flags;

	local_irq_save(flags);
	if (state)
		s3c2410_gpio_setpin(bits->scl, 1);
	else
		s3c2410_gpio_setpin(bits->scl, 0);

#if defined CONFIG_MACH_SMDK2443
	s3c2410_gpio_cfgpin(bits->scl, S3C2410_GPG2_OUTP); 
#elif defined CONFIG_MACH_SMDK2412
	s3c2410_gpio_cfgpin(bits->scl, S3C_GPB4_OUTP); 
#endif

	local_irq_restore(flags);
}



static void l3_setsda(void *data, int state)
{
	struct bit_data *bits = data;

	if (state)
		s3c2410_gpio_setpin(bits->sda, 1);
	else
		s3c2410_gpio_setpin(bits->sda, 0);
}
static void l3_setdir(void *data, int in)
{
	struct bit_data *bits = data;
	unsigned long flags;

	local_irq_save(flags);

#if defined CONFIG_MACH_SMDK2443
#if 0 //ryu
	if (in)
		s3c2410_gpio_cfgpin(bits->sda, S3C_GPG1_INP); 
	else
#endif
		s3c2410_gpio_cfgpin(bits->sda, S3C2410_GPG1_OUTP); 
#elif defined CONFIG_MACH_SMDK2412
	if (in)
		s3c2410_gpio_cfgpin(bits->sda, S3C_GPB3_INP); 
	else
		s3c2410_gpio_cfgpin(bits->sda, S3C_GPB3_OUTP); 
#endif


	local_irq_restore(flags);
}

static void l3_setmode(void *data, int state)
{
	struct bit_data *bits = data;

	if (state)
		s3c2410_gpio_setpin(bits->l3_mode, 1);
	else
		s3c2410_gpio_setpin(bits->l3_mode, 0);
}

static struct l3_algo_bit_data l3_bit_data = {
	.data		= NULL,
	.setdat		= l3_setsda,
	.setclk		= l3_setscl,
	.setmode	= l3_setmode,
	.setdir		= l3_setdir,
	.getdat		= getsda,
	.data_hold	= 1,
	.data_setup	= 1,
	.clock_high	= 1,
	.mode_hold	= 1,
	.mode_setup	= 1,
};

static struct l3_adapter l3_adapter_s3c= {
	.owner		= THIS_MODULE,
	.name		= NAME,
	.algo_data	= &l3_bit_data,
	.lock		= LOCK,
};

static int __init l3_init(struct bit_data *bits)
{
	l3_bit_data.data = bits;
	return l3_bit_add_bus(&l3_adapter_s3c);
}

static void  l3_exit(void)
{
	l3_bit_del_bus(&l3_adapter_s3c);
}

static struct bit_data bit_data;

static int __init bus_init(void)
{

	struct bit_data *bit = &bit_data;
	unsigned long flags;
	int ret;

#if defined CONFIG_MACH_SMDK2443
	bit->sda     = S3C2410_GPG1;
	bit->scl     = S3C2410_GPG2;
	bit->l3_mode = S3C2410_GPG0;
#elif defined CONFIG_MACH_SMDK2412
	bit->sda     = S3C_GPB3;
	bit->scl     = S3C_GPB4;
	bit->l3_mode = S3C_GPB2;
#endif

	if (!bit->sda)
		return -ENODEV;

	/*
	 * Default level for L3 mode is low.
	 */

	local_irq_save(flags);

	s3c2410_gpio_setpin(bit->l3_mode, 1);

	s3c2410_gpio_setpin(bit->scl, 1);

	s3c2410_gpio_setpin(bit->sda, 0);

#if defined CONFIG_MACH_SMDK2443
	s3c2410_gpio_cfgpin(bit->l3_mode, S3C2410_GPG0_OUTP); 
	s3c2410_gpio_cfgpin(bit->sda, S3C2410_GPG1_OUTP); 
	s3c2410_gpio_cfgpin(bit->scl, S3C2410_GPG2_OUTP); 
#elif defined CONFIG_MACH_SMDK2412
	s3c2410_gpio_cfgpin(bit->l3_mode, S3C_GPB2_OUTP); 
	s3c2410_gpio_cfgpin(bit->sda, S3C_GPB3_OUTP); 
	s3c2410_gpio_cfgpin(bit->scl, S3C_GPB4_OUTP); 
#endif

	/* L3 gpio interface set */
	local_irq_restore(flags);

	ret = l3_init(bit);
	if (ret)
		{

		printk("l3 init failed \n");
		l3_exit();
		}
	printk("GPIO L3 bus interface for s3c, installed\n");

	return ret;

}

static void __exit bus_exit(void)
{
	l3_exit();
}

module_init(bus_init);
module_exit(bus_exit);
