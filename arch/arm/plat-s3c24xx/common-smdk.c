/* linux/arch/arm/plat-s3c24xx/common-smdk.c
 *
 * Copyright (c) 2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Common code for SMDK2410 and SMDK2440 boards
 *
 * http://www.fluff.org/ben/smdk2440/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/mach/flash.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/arch/regs-gpio.h>
#include <asm/arch/leds-gpio.h>

#include <asm/arch/nand.h>

#include <asm/plat-s3c24xx/common-smdk.h>
#include <asm/plat-s3c24xx/devs.h>
#include <asm/plat-s3c24xx/pm.h>

/* LED devices */

static struct s3c24xx_led_platdata smdk_pdata_led4 = {
	.gpio		= S3C2410_GPK5,
	.flags		= S3C24XX_LEDF_ACTLOW,
	.name		= "led4",
	.def_trigger	= "timer",
};

static struct s3c24xx_led_platdata smdk_pdata_led5 = {
	.gpio		= S3C2410_GPK6,
	.flags		= S3C24XX_LEDF_ACTLOW,
	.name		= "led5",
	.def_trigger	= "nand-disk",
};

static struct s3c24xx_led_platdata smdk_pdata_led6 = {
	.gpio		= S3C2410_GPK7,
	.flags		= S3C24XX_LEDF_ACTLOW,
	.name		= "led6",
};

static struct s3c24xx_led_platdata smdk_pdata_led7 = {
	.gpio		= S3C2410_GPK8,
	.flags		= S3C24XX_LEDF_ACTLOW,
	.name		= "led7",
};

static struct gpio_keys_button helper2416_button[] = {
	{
		.keycode	= KEY_UP,
		.gpio		= S3C2410_GPK0,
		.active_low	= 1,
		.mode		= S3C2410_GPK0_INP,
		.desc		= "KeyUp",
	},
	{
		.keycode	= KEY_RIGHT,
		.gpio		= S3C2410_GPK1,
		.active_low	= 1,
		.mode		= S3C2410_GPK1_INP,
		.desc		= "KeyRight",
	},
	{
		.keycode	= KEY_ENTER,
		.gpio		= S3C2410_GPK2,
		.active_low	= 1,
		.mode		= S3C2410_GPK2_INP,
		.desc		= "KeyEnter",
	},
	{
		.keycode	= KEY_DOWN,
		.gpio		= S3C2410_GPK3,
		.active_low	= 1,
		.mode		= S3C2410_GPK3_INP,
		.desc		= "KeyDown",
	},
	{
		.keycode	= KEY_LEFT,
		.gpio		= S3C2410_GPK4,
		.active_low	= 1,
		.mode		= S3C2410_GPK4_INP,
		.desc		= "KeyLeft",
	},
};

static struct gpio_keys_platform_data helper2416_pdata_button = {
	.buttons	= helper2416_button,
	.nbuttons	= ARRAY_SIZE(helper2416_button),
};

static struct platform_device helper2416_device_button = 
{
	.name		= "helper2416_keypad",
	.id		= -1,
	.dev		= {
		.platform_data = &helper2416_pdata_button,
	}
};

static struct platform_device smdk_led4 = {
	.name		= "s3c24xx_led",
	.id		= 0,
	.dev		= {
		.platform_data = &smdk_pdata_led4,
	},
};

static struct platform_device smdk_led5 = {
	.name		= "s3c24xx_led",
	.id		= 1,
	.dev		= {
		.platform_data = &smdk_pdata_led5,
	},
};

static struct platform_device smdk_led6 = {
	.name		= "s3c24xx_led",
	.id		= 2,
	.dev		= {
		.platform_data = &smdk_pdata_led6,
	},
};

static struct platform_device smdk_led7 = {
	.name		= "s3c24xx_led",
	.id		= 3,
	.dev		= {
		.platform_data = &smdk_pdata_led7,
	},
};

/* NAND parititon from 2.4.18-swl5 */

static struct mtd_partition smdk_default_nand_part[] = {
	[0] = {
		.name	= "Boot Agent",
		.size	= SZ_16K,
		.offset	= 0,
	},
	[1] = {
		.name	= "S3C2410 flash partition 1",
		.offset = 0,
		.size	= SZ_2M,
	},
	[2] = {
		.name	= "S3C2410 flash partition 2",
		.offset = SZ_4M,
		.size	= SZ_4M,
	},
	[3] = {
		.name	= "S3C2410 flash partition 3",
		.offset	= SZ_8M,
		.size	= SZ_2M,
	},
	[4] = {
		.name	= "S3C2410 flash partition 4",
		.offset = SZ_1M * 10,
		.size	= SZ_4M,
	},
	[5] = {
		.name	= "S3C2410 flash partition 5",
		.offset	= SZ_1M * 14,
		.size	= SZ_1M * 10,
	},
	[6] = {
		.name	= "S3C2410 flash partition 6",
		.offset	= SZ_1M * 24,
		.size	= SZ_1M * 24,
	},
	[7] = {
		.name	= "S3C2410 flash partition 7",
		.offset = SZ_1M * 48,
		.size	= SZ_16M,
	}
};

/* ----------------S3C NAND partition information ---------------------*/
struct mtd_partition s3c_partition_info[] = {
        {
                .name		= "Bootloader",
                .offset		= 0,
                .size		= (256*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
        {
                .name		= "Kernel",
                .offset		= (256*SZ_1K),    /* Block number is 0x10 */
                .size		= (4*SZ_1M) - (256*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
#ifdef CONFIG_SPLIT_ROOT_FILESYSTEM
        {
                .name		= "Root - Cramfs",
                .offset		= (4*SZ_1M),    /* Block number is 0x80 */
                .size		= (48*SZ_1M),
        },
#endif
        {
                .name		= "File System",
                .offset		= MTDPART_OFS_APPEND,
                .size		= MTDPART_SIZ_FULL,
        }
};

struct s3c_nand_mtd_info nand_mtd_info = {
	.chip_nr = 1,
	.mtd_part_nr = ARRAY_SIZE(s3c_partition_info),
	.partition = s3c_partition_info,
};

struct s3c_nand_mtd_info * get_board_nand_mtd_info (void)
{
	return &nand_mtd_info;
}

struct flash_platform_data s3c_onenand_data = {
	.parts		= s3c_partition_info,
	.nr_parts	= ARRAY_SIZE(s3c_partition_info),
};


/* ---------------------------------------------------------------------*/


static struct s3c2410_nand_set smdk_nand_sets[] = {
	[0] = {
		.name		= "NAND",
		.nr_chips	= 1,
		.nr_partitions	= ARRAY_SIZE(smdk_default_nand_part),
		.partitions	= smdk_default_nand_part,
	},
};

/* choose a set of timings which should suit most 512Mbit
 * chips and beyond.
*/

static struct s3c2410_platform_nand smdk_nand_info = {
	.tacls		= 20,
	.twrph0		= 60,
	.twrph1		= 20,
	.nr_sets	= ARRAY_SIZE(smdk_nand_sets),
	.sets		= smdk_nand_sets,
};

/* devices we initialise */

static struct platform_device __initdata *smdk_devs[] = {
	&s3c_device_nand,
	&s3c_device_onenand,
	&smdk_led4,
	&smdk_led5,
	&smdk_led6,
	&smdk_led7,
	&helper2416_device_button,
};

void __init smdk_machine_init(void)
{
	/* Configure the LEDs (even if we have no LED support)*/

	s3c2410_gpio_cfgpin(S3C2410_GPK5, S3C2410_GPK5_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPK6, S3C2410_GPK6_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPK7, S3C2410_GPK7_OUTP);
	s3c2410_gpio_cfgpin(S3C2410_GPK8, S3C2410_GPK8_OUTP);

	s3c2410_gpio_setpin(S3C2410_GPK5, 1);
	s3c2410_gpio_setpin(S3C2410_GPK6, 1);
	s3c2410_gpio_setpin(S3C2410_GPK7, 1);
	s3c2410_gpio_setpin(S3C2410_GPK8, 1);

	/* ¹Ø±Õ·äÃùÆ÷ */
	s3c2410_gpio_cfgpin(S3C2410_GPB0, S3C2410_GPB0_OUTP);
	s3c2410_gpio_setpin(S3C2410_GPB0, 1);

	
	s3c_device_nand.dev.platform_data = &smdk_nand_info;
	
	//For s3c nand partition
	s3c_device_nand.dev.platform_data = &nand_mtd_info;

	platform_add_devices(smdk_devs, ARRAY_SIZE(smdk_devs));

	s3c2410_pm_init();
}
