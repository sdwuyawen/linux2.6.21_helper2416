/***********************************************************************
 *
 * linux/arch/arm/mach-s3c6410/mach-smdk6410.c
 *
 * $Id: mach-smdk6430.c,v 1.2 2008/07/14 00:19:02 wizardsj Exp $
 *
 * Copyright (C) 2005, Sean Choi <sh428.choi@samsung.com>
 * All rights reserved
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * derived from linux/arch/arm/mach-s3c2410/devs.c, written by
 * Ben Dooks <ben@simtec.co.uk>
 *
 ***********************************************************************/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <asm/mach/flash.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-gpio.h>

#include <asm/arch/regs-mem.h>
#include <asm/arch/regs-s3c6410-clock.h>
#include <asm/arch/nand.h>

#include <asm/plat-s3c24xx/s3c6400.h>
#include <asm/plat-s3c24xx/devs.h>
#include <asm/plat-s3c24xx/cpu.h>
#include <asm/arch/hsmmc.h>

#include <asm/plat-s3c24xx/common-smdk.h>
#include <asm/arch-s3c2410/reserved_mem.h>

#if defined(CONFIG_MACH_SANJOSE2)
#include <asm/arch/regs-irq.h>
#endif

extern struct sys_timer s3c_timer;

static struct map_desc smdk6410_iodesc[] __initdata = {
	IODESC_ENT(CS8900),
};

#define DEF_UCON S3C_UCON_DEFAULT
#define DEF_ULCON S3C_LCON_CS8 | S3C_LCON_PNONE
#define DEF_UFCON S3C_UFCON_RXTRIG8 | S3C_UFCON_FIFOMODE


static struct s3c24xx_uart_clksrc smdk6410_serial_clocks[] = {

/* UART Clock with MPLL by Boyko */

	[0] = {
		.name		= "mpll_clk_uart",
		.divisor	= 1,
		.min_baud	= 0,
		.max_baud	= 0,
	},

#if 0 /* HS UART Source is changed from epll to mpll */

	[1] = {
		.name		= "pclk",
		.divisor	= 1,
		.min_baud	= 0,
		.max_baud	= 0,
	},

#if defined (CONFIG_SERIAL_S3C64XX_HS_UART)

	[2] = {
		.name		= "epll_clk_uart_192m",
		.divisor	= 1,
		.min_baud	= 0,
		.max_baud	= 4000000,
	}
#endif

#endif

};



static struct s3c2410_uartcfg smdk6410_uartcfgs[] = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6410_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6410_serial_clocks),
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6410_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6410_serial_clocks),
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6410_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6410_serial_clocks),
	},
	[3] = {
		.hwport	     = 3,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6410_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6410_serial_clocks),
	}

};



/*add devices as drivers are integrated*/
static struct platform_device *smdk6410_devices[] __initdata = {
	&s3c_device_lcd,
	&s3c_device_rtc,
	&s3c_device_ac97,
	&s3c_device_iis,
	&s3c_device_adc,
 	&s3c_device_i2c,
	&s3c_device_usb,
	&s3c_device_usbgadget,
	&s3c_device_tvenc,
	&s3c_device_tvscaler,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
	&s3c_device_hsmmc2,
	&s3c_device_wdt,
	&s3c_device_jpeg,
	&s3c_device_vpp,
	&s3c_device_ide,
//	&s3c_device_mfc,
	&s3c_device_spi0,
	&s3c_device_spi1,
	&s3c_device_camif,
	&s3c_device_2d,
	&s3c_device_keypad,
//	&s3c_device_g3d,
	&s3c_device_smc911x,
	&s3c_device_rotator,
	&s3c_device_iis_v40,
};


static struct s3c24xx_board smdk6410_board __initdata = {
	.devices       = smdk6410_devices,
	.devices_count = ARRAY_SIZE(smdk6410_devices)
};


static void __init smdk6410_map_io(void)
{
	s3c24xx_init_io(smdk6410_iodesc, ARRAY_SIZE(smdk6410_iodesc));
	s3c24xx_init_clocks(0);
	s3c24xx_init_uarts(smdk6410_uartcfgs, ARRAY_SIZE(smdk6410_uartcfgs));
	s3c24xx_set_board(&smdk6410_board);
}


void smdk6410_cs89x0_set(void)
{
	unsigned int tmp;

	tmp = __raw_readl(S3C_SROM_BW);
	tmp &=~(0xF<<4);
	tmp |= (1<<7) | (1<<6) | (1<<4);
	__raw_writel(tmp, S3C_SROM_BW);

	__raw_writel((0x0<<28)|(0x4<<24)|(0xd<<16)|(0x1<<12)|(0x4<<8)|(0x6<<4)|(0x0<<0), S3C_SROM_BC1);

#if defined(CONFIG_MACH_SANJOSE2)
	__raw_writel(0, S3C_GPPPU);
	__raw_writel((__raw_readl(S3C_GPPCON) & ~(0x3 << 14)) | (0x1 << 14), S3C_GPPCON);
#endif
}


static void __init smdk6410_fixup (struct machine_desc *desc, struct tag *tags,
	      char **cmdline, struct meminfo *mi)
{
	/*
	 * Bank start addresses are not present in the information
	 * passed in from the boot loader.  We could potentially
	 * detect them, but instead we hard-code them.
	 */
	mi->bank[0].start = 0x50000000;

#if defined(CONFIG_RESERVED_MEM_JPEG)
	mi->bank[0].size = 120*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_JPEG_POST)
	mi->bank[0].size = 112*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_MFC)
	mi->bank[0].size = 122*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_MFC_POST)
	mi->bank[0].size = 114*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_JPEG_MFC_POST)
	mi->bank[0].size = 106*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_JPEG_CAMERA)
	mi->bank[0].size = 105*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_JPEG_POST_CAMERA)
	mi->bank[0].size = 97*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_MFC_CAMERA)
	mi->bank[0].size = 107*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_MFC_POST_CAMERA)
	mi->bank[0].size = 99*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_JPEG_MFC_POST_CAMERA)
	mi->bank[0].size = 91*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_CMM_MFC_POST)
	mi->bank[0].size = 106*1024*1024;
#elif defined(CONFIG_RESERVED_MEM_CMM_JPEG_MFC_POST_CAMERA)
	mi->bank[0].size = 83*1024*1024;
#elif defined(CONFIG_VIDEO_SAMSUNG)
	mi->bank[0].size = 113*1024*1024;
#else
 	mi->bank[0].size = 128*1024*1024;
#endif
	mi->bank[0].node = 0;

	mi->nr_banks = 1;
}

void smdk6410_hsmmc_init (void)
{
	/* hsmmc data strength */
	writel(readl(S3C_SPCON) | (0x3 << 26), S3C_SPCON);

	/* jsgood: hsmmc0/1 card detect pin should be high before setup gpio. (GPG6 to Input) */
	writel(readl(S3C_GPGCON) & 0xf0ffffff, S3C_GPGCON);

	/* GPIO N 13 (external interrupt) : Chip detect */
	s3c_gpio_cfgpin(S3C_GPN13, S3C_GPN13_EXTINT13);		/* GPN13 to EINT13 */
	s3c_gpio_pullup(S3C_GPN13, 0x2);	 	 	/* Pull-up Enable */

	/* jsgood: MUXmmc# to DOUTmpll for MPLL Clock Source */
	writel((readl(S3C_CLK_SRC) & ~(0x3f << 18)) | (0x15 << 18), S3C_CLK_SRC);

#if defined(CONFIG_MACH_SANJOSE2)
	/* for hsmmc ch2 */
	writel(0, S3C_GPNPU);
	writel((readl(S3C_GPNCON) & ~(0x3 << 24)) | (0x2 << 24), S3C_GPNCON);	/* GPN12 to EINT */
	writel(readl(S3C_EINT0CON0) | (0x3 << 25), S3C_EINT0CON0);		/* EINT12 to both edge triggered */
	writel(readl(S3C_EINT0MASK) & ~(0x1 << 12), S3C_EINT0MASK);		/* EINT12 unmask */
	writel(readl(S3C_VIC1INTENABLE) | (0x1 << 0), S3C_VIC1INTENABLE);	/* EINT12 enable */
#endif
}

#if 0 /* removed otg reset due to change mmc clock source USB clock to MPLL */
static void smdk6410_usb_otg_reset (void)
{
#if defined(CONFIG_MACH_SANJOSE2)
	/* GPC7 to high for usb power up */
	s3c_gpio_cfgpin(S3C_GPC7, S3C_GPC7_OUTP);
	s3c_gpio_setpin(S3C_GPC7, 1);
#endif

	/* initialize for usb 48m ext clock */
	writel(readl(S3C_OTHERS) | S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);

	writel(0x0, S3C_USBOTG_PHYPWR);
	writel(0x20, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);
	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
#endif

static void __init smdk6410_machine_init (void)
{
	smdk6410_cs89x0_set();
	smdk_machine_init();
#if 0 /* removed otg reset due to change mmc clock source USB clock to MPLL */
	smdk6410_usb_otg_reset();
#endif
	smdk6410_hsmmc_init();
}

MACHINE_START(SMDK6430, "SMDK6430")
	/* Maintainer: Samsung Electronics */
	.phys_io    = S3C24XX_PA_UART,
	.io_pg_offst    = (((u32)S3C24XX_VA_UART) >> 18) & 0xfffc,
	.boot_params    = S3C_SDRAM_PA + 0x100,

	.init_irq   = s3c_init_irq,
	.map_io     = smdk6410_map_io,
	.fixup      = smdk6410_fixup,
	.timer      = &s3c_timer,
	.init_machine   = smdk6410_machine_init,
MACHINE_END


/*--------------------------------------------------------------
 * HS-MMC GPIO Set function
 * the location of this function must be re-considered.
 * by scsuh
 *--------------------------------------------------------------*/
void hsmmc_set_gpio (uint channel, uint width)
{
	switch (channel) {
	/* can supports 1 and 4 bit bus */
	case 0:
		/* GPIO G : Command, Clock */
		s3c_gpio_cfgpin(S3C_GPG0, S3C_GPG0_MMC_CLK0);
		s3c_gpio_cfgpin(S3C_GPG1, S3C_GPG1_MMC_CMD0);

		s3c_gpio_pullup(S3C_GPG0, 0x0);	  /* Pull-up/down disable */
		s3c_gpio_pullup(S3C_GPG1, 0x0);	  /* Pull-up/down disable */

		/* GPIO G : Chip detect + LED */
		s3c_gpio_cfgpin(S3C_GPG6, S3C_GPG6_MMC_CD1);
		s3c_gpio_pullup(S3C_GPG6, 0x2);	  /* Pull-up Enable */
		
		if (width == 1) {
			/* GPIO G : MMC DATA1[0] */
			s3c_gpio_cfgpin(S3C_GPG2, S3C_GPG2_MMC_DATA0_0);

			s3c_gpio_pullup(S3C_GPG2, 0x0);	  /* Pull-up/down disable */
		}
		else if (width == 4) {
			/* GPIO G : MMC DATA1[0:3] */
			s3c_gpio_cfgpin(S3C_GPG2, S3C_GPG2_MMC_DATA0_0);
			s3c_gpio_cfgpin(S3C_GPG3, S3C_GPG3_MMC_DATA0_1);
			s3c_gpio_cfgpin(S3C_GPG4, S3C_GPG4_MMC_DATA0_2);
			s3c_gpio_cfgpin(S3C_GPG5, S3C_GPG5_MMC_DATA0_3);

			s3c_gpio_pullup(S3C_GPG2, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPG3, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPG4, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPG5, 0x0);	  /* Pull-up/down disable */
		}
		break;

	/* can supports 1, 4, and 8 bit bus */
	case 1:
		/* GPIO H : Command, Clock */
		s3c_gpio_cfgpin(S3C_GPH0, S3C_GPH0_MMC_CLK1);
		s3c_gpio_cfgpin(S3C_GPH1, S3C_GPH1_MMC_CMD1);

		s3c_gpio_pullup(S3C_GPH0, 0x0);	  /* Pull-up/down disable */
		s3c_gpio_pullup(S3C_GPH1, 0x0);	  /* Pull-up/down disable */

		/* GPIO G : Chip detect + LED */
		s3c_gpio_cfgpin(S3C_GPG6, S3C_GPG6_MMC_CD1);
		s3c_gpio_pullup(S3C_GPG6, 0x2);	  /* Pull-up Enable */
		
		if (width == 1) {
			/* GPIO H : MMC DATA1[0] */
			s3c_gpio_cfgpin(S3C_GPH2, S3C_GPH2_MMC_DATA1_0);

			s3c_gpio_pullup(S3C_GPH2, 0x0);	  /* Pull-up/down disable */
		}
		else if (width == 4) {
			/* GPIO H : MMC DATA1[0:3] */
			s3c_gpio_cfgpin(S3C_GPH2, S3C_GPH2_MMC_DATA1_0);
			s3c_gpio_cfgpin(S3C_GPH3, S3C_GPH3_MMC_DATA1_1);
			s3c_gpio_cfgpin(S3C_GPH4, S3C_GPH4_MMC_DATA1_2);
			s3c_gpio_cfgpin(S3C_GPH5, S3C_GPH5_MMC_DATA1_3);

			s3c_gpio_pullup(S3C_GPH2, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH3, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH4, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH5, 0x0);	  /* Pull-up/down disable */
		}
		else if (width == 8) {
			/* GPIO H : MMC DATA1[0:7] */
			s3c_gpio_cfgpin(S3C_GPH2, S3C_GPH2_MMC_DATA1_0);
			s3c_gpio_cfgpin(S3C_GPH3, S3C_GPH3_MMC_DATA1_1);
			s3c_gpio_cfgpin(S3C_GPH4, S3C_GPH4_MMC_DATA1_2);
			s3c_gpio_cfgpin(S3C_GPH5, S3C_GPH5_MMC_DATA1_3);
			
			s3c_gpio_cfgpin(S3C_GPH6, S3C_GPH6_MMC_DATA1_4);
			s3c_gpio_cfgpin(S3C_GPH7, S3C_GPH7_MMC_DATA1_5);
			s3c_gpio_cfgpin(S3C_GPH8, S3C_GPH8_MMC_DATA1_6);
			s3c_gpio_cfgpin(S3C_GPH9, S3C_GPH9_MMC_DATA1_7);

			s3c_gpio_pullup(S3C_GPH2, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH3, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH4, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH5, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH6, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH7, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH8, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH9, 0x0);	  /* Pull-up/down disable */
		}
		break;

	/* can supports 1 and 4 bit bus, no irq_cd */
	case 2:
		/* GPIO H : Command, Clock */
		s3c_gpio_cfgpin(S3C_GPH0, S3C_GPH0_MMC_CLK1);
		s3c_gpio_cfgpin(S3C_GPH1, S3C_GPH1_MMC_CMD1);

		s3c_gpio_pullup(S3C_GPH0, 0x0);	  /* Pull-up/down disable */
		s3c_gpio_pullup(S3C_GPH1, 0x0);	  /* Pull-up/down disable */

		/* GPIO G : Chip detect + LED */
		s3c_gpio_cfgpin(S3C_GPG6, S3C_GPG6_MMC_CD1);
		s3c_gpio_pullup(S3C_GPG6, 0x2);	  /* Pull-up Enable */

		if (width == 1) {
			/* GPIO H : MMC DATA1[0] */
			s3c_gpio_cfgpin(S3C_GPH6, S3C_GPH6_MMC_DATA2_0);

			s3c_gpio_pullup(S3C_GPH6, 0x0);	  /* Pull-up/down disable */
		}
		else if (width == 4) {
			/* GPIO H : MMC DATA1[0:3] */
			s3c_gpio_cfgpin(S3C_GPH6, S3C_GPH6_MMC_DATA2_0);
			s3c_gpio_cfgpin(S3C_GPH7, S3C_GPH7_MMC_DATA2_1);
			s3c_gpio_cfgpin(S3C_GPH8, S3C_GPH8_MMC_DATA2_2);
			s3c_gpio_cfgpin(S3C_GPH9, S3C_GPH9_MMC_DATA2_3);

			s3c_gpio_pullup(S3C_GPH6, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH7, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH8, 0x0);	  /* Pull-up/down disable */
			s3c_gpio_pullup(S3C_GPH9, 0x0);	  /* Pull-up/down disable */
		}
		break;

	default:
		break;
	}
}


/* For host controller's capabilities */
#define HOST_CAPS (MMC_CAP_4_BIT_DATA | MMC_CAP_MULTIWRITE | \
			MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED)

struct s3c_hsmmc_cfg s3c_hsmmc0_platform = {
	.hwport = 0,
	.enabled = 0,
	.host_caps = HOST_CAPS,
	.bus_width = 4,
	.highspeed = 0,

	/* ctrl for mmc */
	.fd_ctrl[MMC_MODE_MMC] = {
		.ctrl2 = 0xC0004100,			/* ctrl2 for mmc */
		.ctrl3[SPEED_NORMAL] = 0x80808080,	/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0x00000080,	/* ctrl3 for high speed */
		.ctrl4 = 0x3,
	},
	
	/* ctrl for sd */
	.fd_ctrl[MMC_MODE_SD] = {
		.ctrl2 = 0xC0004100,			/* ctrl2 for sd */
		.ctrl3[SPEED_NORMAL] = 0x80808080,	/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0x00000080,	/* ctrl3 for high speed */
		.ctrl4 = 0x3,
	},

	/*
	 *  Warning: Do not change the order of clk_name[]
	 *  The index number + 1 is used to value for SELBASECLK in CONTROL2 register for Base Clock Source Select
	 */
	.clk_name[0] = "",
        .clk_name[1] = "sclk_DOUTmpll_mmc0",
};

struct s3c_hsmmc_cfg s3c_hsmmc1_platform = {
	.hwport = 1,
	.enabled = 1,
	.host_caps = HOST_CAPS,
	.bus_width = 4,
	.highspeed = 0,

	/* ctrl for mmc */
	.fd_ctrl[MMC_MODE_MMC] = {
		.ctrl2 = 0xC0004100,			/* ctrl2 for mmc */
		.ctrl3[SPEED_NORMAL] = 0x80808080,	/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0x00000080,	/* ctrl3 for high speed */
		.ctrl4 = 0x3,
	},
	
	/* ctrl for sd */
	.fd_ctrl[MMC_MODE_SD] = {
		.ctrl2 = 0xC0004100,			/* ctrl2 for sd */
		.ctrl3[SPEED_NORMAL] = 0x80808080,	/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0x00000080,	/* ctrl3 for high speed */
		.ctrl4 = 0x3,
	},

	/*
	 *  Warning: Do not change the order of clk_name[]
	 *  The index number + 1 is used to value for SELBASECLK in CONTROL2 register for Base Clock Source Select
	 */
	.clk_name[0] = "",
        .clk_name[1] = "sclk_DOUTmpll_mmc1",
};

struct s3c_hsmmc_cfg s3c_hsmmc2_platform = {
	.hwport = 2,
	.enabled = 0,
	.host_caps = HOST_CAPS,
	.bus_width = 4,
	.highspeed = 0,

	/* ctrl for mmc */
	.fd_ctrl[MMC_MODE_MMC] = {
		.ctrl2 = 0xC0004100,			/* ctrl2 for mmc */
		.ctrl3[SPEED_NORMAL] = 0x80808080,	/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0x00000080,	/* ctrl3 for high speed */
		.ctrl4 = 0x3,
	},
	
	/* ctrl for sd */
	.fd_ctrl[MMC_MODE_SD] = {
		.ctrl2 = 0xC0004100,			/* ctrl2 for sd */
		.ctrl3[SPEED_NORMAL] = 0x80808080,	/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0x00000080,	/* ctrl3 for high speed */
		.ctrl4 = 0x3,
	},

	/*
	 *  Warning: Do not change the order of clk_name[]
	 *  The index number + 1 is used to value for SELBASECLK in CONTROL2 register for Base Clock Source Select
	 */
	.clk_name[0] = "",
        .clk_name[1] = "sclk_DOUTmpll_mmc2",
};

