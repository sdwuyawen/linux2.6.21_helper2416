
/* linux/arch/arm/mach-s3c64xx/s3c6400.c
 *
 * Copyright (c) 2006, Samsung Electronics
 * All rights reserved.
 *
 * Samsung S3C6400 Mobile CPU support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * derived from linux/arch/arm/mach-s3c2410/devs.c, written by
 * Ben Dooks <ben@simtec.co.uk>
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/clk.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/proc-fns.h>
#include <asm/arch/idle.h>

#include <asm/arch/regs-s3c6400-clock.h>
#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/map.h>
#include <asm/arch/nand.h>

//#include <asm/plat-s3c24xx/clock.h>
#include <asm/plat-s3c24xx/s3c6400.h>
#include <asm/plat-s3c24xx/devs.h>
#include <asm/plat-s3c24xx/cpu.h>
#include "pm-s3c6400.h"

/* Serial port registrations */

static struct map_desc s3c6400_iodesc[] __initdata = {
	IODESC_ENT(TIMER),
	IODESC_ENT(LCD),
	IODESC_ENT(HOSTIFB),
	IODESC_ENT(IIS),
	IODESC_ENT(AC97),
	IODESC_ENT(OTG),
	IODESC_ENT(OTGSFR),
	IODESC_ENT(CAMIF),
};

/* s3c6400_idle
 *
 * use the standard idle call by ensuring the idle mode
 * in power config, then issuing the idle co-processor
 * instruction
*/

static void s3c6400_idle(void)
{
	unsigned long tmp;

	/* ensure our idle mode is to go to idle */

	/* Set WFI instruction to SLEEP mode */

	tmp = __raw_readl(S3C_PWR_CFG);
	tmp &= ~(0x3<<5);
	tmp |= (0x1<<5);
	__raw_writel(tmp, S3C_PWR_CFG);

	cpu_do_idle();
}


void __init s3c6400_init_uarts(struct s3c2410_uartcfg *cfg, int no)
{

	s3c24xx_init_uartdevs("s3c-uart", s3c2410_uart_resources, cfg, no);


#if defined (CONFIG_S3C_SIR)
	s3c24xx_uart_src[3]->name = "s3c-irda";
#endif

	/* rename devices that are s3c2413/s3c2443/s3c6400 specific */
        s3c_device_lcd.name  = "s3c-lcd";
	s3c_device_nand.name  = "s3c-nand";
	s3c_device_tvenc.name = "s3c-tvenc";
	s3c_device_tvscaler.name = "s3c-tvscaler";
}

struct sysdev_class s3c6400_sysclass = {
	set_kset_name("s3c6400-core"),
};

static struct sys_device s3c6400_sysdev = {
	.cls		= &s3c6400_sysclass,
};


static int __init s3c6400_core_init(void)
{
	return sysdev_class_register(&s3c6400_sysclass);
}

core_initcall(s3c6400_core_init);

void __init s3c6400_map_io(struct map_desc *mach_desc, int size)
{
	/* register our io-tables */
	iotable_init(s3c6400_iodesc, ARRAY_SIZE(s3c6400_iodesc));
	iotable_init(mach_desc, size);
	/* rename any peripherals used differing from the s3c2412 */

	/* set our idle function */

	s3c24xx_idle = s3c6400_idle;
}

int __init s3c6400_init(void)
{
	int ret;

	printk("s3c6400: Initialising architecture\n");

	ret = sysdev_register(&s3c6400_sysdev);

	if (ret != 0)
		printk(KERN_ERR "failed to register sysdev for s3c6400\n");

	return ret;
}



#define CAMDIV_val     20

int s3c_camif_set_clock (unsigned int camclk)
{
	unsigned int camclk_div, val, hclkcon;
	struct clk *src_clk = clk_get(NULL, "hclkx2");

	printk(KERN_INFO "External camera clock is set to %dHz\n", camclk);

	camclk_div = clk_get_rate(src_clk) / camclk;
	printk("Parent clk = %ld, CAMDIV = %d\n", clk_get_rate(src_clk), camclk_div);

	// CAMIF HCLK Enable
	hclkcon = __raw_readl(S3C_HCLK_GATE);
	hclkcon |= S3C_CLKCON_HCLK_CAMIF;
	__raw_writel(hclkcon, S3C_HCLK_GATE);

	/* CAMCLK Enable */
	val = readl(S3C_SCLK_GATE);
	val |= S3C_CLKCON_SCLK_CAM;
	writel(val, S3C_SCLK_GATE);

	val = readl(S3C_CLK_DIV0);
	val &= ~(0xf<<CAMDIV_val);
	writel(val, S3C_CLK_DIV0);

	/* CAM CLK DIVider Ratio = (EPLL clk)/(camclk_div) */
	val |= ((camclk_div - 1) << CAMDIV_val);
	writel(val, S3C_CLK_DIV0);
	val = readl(S3C_CLK_DIV0);

	return 0;
}

void s3c_camif_disable_clock (void)
{
	unsigned int val;

	val = readl(S3C_SCLK_GATE);
	val &= ~S3C_CLKCON_SCLK_CAM;
	writel(val, S3C_SCLK_GATE);
}


static int mem_proc_read (
 char *buffer,
 char **buffer_location,
 off_t offset,
 int buffer_length,
 int *zero,
 void *ptr
)
{
    return 0;
}

static int mem_proc_write (
 struct file *file,
 const char *buffer,
 unsigned long count,
 void *data
)
{
    ulong address = 0, size = 0x100;
    ulong i = 0, j = 0;
    volatile uint *addr;
    char flag = 'b';
    uint temp=0;

    printk("buffers: %s\n", buffer);
    sscanf(buffer, "%lx %lx %c", &address, &size, &flag);

    switch (flag) {
    case 'b':
	for (i = address; i < address + size; i += 4 * 4) {
	    printk("0x%08lx: %08x %08x %08x %08x\n",
		    i,
		    *(uint *) i,
		    *(uint *) (i + 0x04),
		    *(uint *) (i + 0x08),
		    *(uint *) (i + 0x0c));
	}
	break;

    case 'l':
	for (i = address; i < address + size; i += 4 * 4) {
	    printk("0x%08lx: %08x %08x %08x %08x\n",
		    i,
		    swab32(*(uint *) i),
		    swab32(*(uint *) (i + 0x04)),
		    swab32(*(uint *) (i + 0x08)),
		    swab32(*(uint *) (i + 0x0c)));
	}
	break;

    case 'w':
	for (j = 0; j < 0x20000; j++) {
	    temp = 0;
	    for (i = address; i < address + size; i += 4) {
		addr = (volatile uint *)i;
		temp += *addr;
//		*(volatile uint *)i = temp+1;
	    }
	    if (!(j%0x1000)) {
		printk("%08x\n", temp);
	    }
	}
	break;
    }

    return count;
}

static struct proc_dir_entry *evb_resource_dump;

int __init mem_rw_proc (void)
{
    evb_resource_dump = create_proc_entry("memory", 0666, &proc_root);
    evb_resource_dump->read_proc = mem_proc_read;
    evb_resource_dump->write_proc = mem_proc_write;
    evb_resource_dump->nlink = 1;
    return 0;
}
module_init(mem_rw_proc);

