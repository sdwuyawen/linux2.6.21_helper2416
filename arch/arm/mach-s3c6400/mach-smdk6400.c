/***********************************************************************
 *
 * linux/arch/arm/mach-s3c64xx/smdk6400.c
 *
 * $Id: mach-smdk6400.c,v 1.53 2008/08/05 09:31:45 dasan Exp $
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

static struct map_desc smdk6400_iodesc[] __initdata = {
	IODESC_ENT(CS8900),
};

#define DEF_UCON S3C_UCON_DEFAULT
#define DEF_ULCON S3C_LCON_CS8 | S3C_LCON_PNONE
#define DEF_UFCON S3C_UFCON_RXTRIG8 | S3C_UFCON_FIFOMODE


static struct s3c24xx_uart_clksrc smdk6400_serial_clocks[] = {

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



static struct s3c2410_uartcfg smdk6400_uartcfgs[] = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6400_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6400_serial_clocks),
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6400_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6400_serial_clocks),
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6400_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6400_serial_clocks),
	},
	[3] = {
		.hwport	     = 3,
		.flags	     = 0,
		.ucon	     = DEF_UCON,
		.ulcon	     = DEF_ULCON,
		.ufcon	     = DEF_UFCON,
		.clocks	     = smdk6400_serial_clocks,
		.clocks_size = ARRAY_SIZE(smdk6400_serial_clocks),
	}

};



/*add devices as drivers are integrated*/
static struct platform_device *smdk6400_devices[] __initdata = {
	&s3c_device_lcd,
	&s3c_device_rtc,
	&s3c_device_ac97,
	&s3c_device_iis,
	&s3c_device_adc,
 	&s3c_device_i2c,
	&s3c_device_usb,
	&s3c_device_usbgadget,
	&s3c_device_usb_otghcd,
	&s3c_device_tvenc,
	&s3c_device_tvscaler,
	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,
	&s3c_device_hsmmc2,
	&s3c_device_wdt,
	&s3c_device_jpeg,
	&s3c_device_vpp,
	&s3c_device_ide,
	&s3c_device_mfc,
	&s3c_device_spi0,
	&s3c_device_spi1,
	&s3c_device_2d,
	&s3c_device_keypad,
	&s3c_device_camif,
};


static struct s3c24xx_board smdk6400_board __initdata = {
	.devices       = smdk6400_devices,
	.devices_count = ARRAY_SIZE(smdk6400_devices)
};


static void __init smdk6400_map_io(void)
{
	s3c24xx_init_io(smdk6400_iodesc, ARRAY_SIZE(smdk6400_iodesc));
	s3c24xx_init_clocks(0);
	s3c24xx_init_uarts(smdk6400_uartcfgs, ARRAY_SIZE(smdk6400_uartcfgs));
	s3c24xx_set_board(&smdk6400_board);
}


void smdk6400_cs89x0_set(void)
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


static void __init smdk6400_fixup (struct machine_desc *desc, struct tag *tags,
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
#elif defined(CONFIG_RESERVED_MEM_TV_MFC_POST_CAMERA)
	mi->bank[0].size = 91*1024*1024;
#elif defined(CONFIG_VIDEO_SAMSUNG)
	mi->bank[0].size = 113*1024*1024;
#else
 	mi->bank[0].size = 128*1024*1024;
#endif
	mi->bank[0].node = 0;

	mi->nr_banks = 1;
}

static void smdk6400_hsmmc_init (void)
{
	/* jsgood: hsmmc0/1 card detect pin should be high before setup gpio. (GPG6 to Input) */
	writel(readl(S3C_GPGCON) & 0xf0ffffff, S3C_GPGCON);

#if defined(CONFIG_MACH_SANJOSE2)
	/* for hsmmc ch2 */
	writel(0, S3C_GPNPU);
	writel((readl(S3C_GPNCON) & ~(0x3 << 24)) | (0x2 << 24), S3C_GPNCON);	/* GPN12 to EINT */
	writel(readl(S3C_EINT0CON0) | (0x3 << 25), S3C_EINT0CON0);		/* EINT12 to both edge triggered */
	writel(readl(S3C_EINT0MASK) & ~(0x1 << 12), S3C_EINT0MASK);		/* EINT12 unmask */
	writel(readl(S3C_VIC1INTENABLE) | (0x1 << 0), S3C_VIC1INTENABLE);	/* EINT12 enable */

#if 0
	/* smdk6400 */
	writel((readl(S3C_GPNCON) & ~(0x3 << 30)) | (0x2 << 30), S3C_GPNCON);	/* GPN15 to EINT */
	writel(readl(S3C_EINT0CON0) | (0x3 << 29), S3C_EINT0CON0);		/* EINT15 to both edge triggered */
	writel(readl(S3C_EINT0MASK) & ~(0x1 << 15), S3C_EINT0MASK);		/* EINT15 unmask */
	writel(readl(S3C_VIC1INTENABLE) | (0x1 << 4), S3C_VIC1INTENABLE);	/* EINT15 enable */
#endif

	/* GPC7 to high for usb power up */
	s3c_gpio_cfgpin(S3C_GPC7, S3C_GPC7_OUTP);
	s3c_gpio_setpin(S3C_GPC7, 1);
#endif
}

static void __init smdk6400_machine_init (void)
{
	smdk6400_cs89x0_set();
	smdk_machine_init();
	smdk6400_hsmmc_init();
}

MACHINE_START(SMDK6400, "SMDK6400")
	/* Maintainer: Samsung Electronics */
	.phys_io    = S3C24XX_PA_UART,
	.io_pg_offst    = (((u32)S3C24XX_VA_UART) >> 18) & 0xfffc,
	.boot_params    = S3C_SDRAM_PA + 0x100,

	.init_irq   = s3c_init_irq,
	.map_io     = smdk6400_map_io,
	.fixup      = smdk6400_fixup,
	.timer      = &s3c_timer,
	.init_machine   = smdk6400_machine_init,
MACHINE_END


/*--------------------------------------------------------------
 * HS-MMC GPIO Set function
 * the location of this function must be re-considered.
 * by scsuh
 *--------------------------------------------------------------*/
void hsmmc_set_gpio (uint channel, uint width)
{
	u32 reg;

	switch (channel) {
	/* can supports 1 and 4 bit bus */
	case 0:
		if (width == 1) {
			/* MMC CLK0, MMC CMD0, MMC DATA0[0], MMC CDn0 0~2,6 */
			reg = readl(S3C_GPGCON) & 0xf0fff000;
			writel(reg | 0x02000222, S3C_GPGCON);
			reg = readl(S3C_GPGPU) & 0xffffcfc0;
			writel(reg, S3C_GPGPU);
		}
		else if (width == 4) {
			/* MMC CLK0, MMC CMD0, MMC DATA0[0-3], MMC CDn0 0~6 */
			reg = readl(S3C_GPGCON) & 0xf0000000;
			writel(reg | 0x02222222, S3C_GPGCON);
			reg = readl(S3C_GPGPU) & 0xfffff000;
			writel(reg, S3C_GPGPU);
		}
		break;

	/* can supports 1, 4, and 8 bit bus */
	case 1:
		/* MMC CDn1 - 6 */
		reg = readl(S3C_GPGCON) & 0xf0ffffff;
		writel(reg | 0x03000000, S3C_GPGCON);
		reg = readl(S3C_GPGPU) & 0xffffcfff;
		writel(reg, S3C_GPGPU);

		if (width == 1) {
			/* MMC CLK1, MMC CMD1, MMC DATA1[0]  - 0~2 */
			reg = readl(S3C_GPH0CON) & 0xfffff000;
			writel(reg | 0x222, S3C_GPH0CON);
			reg = readl(S3C_GPHPU) & 0xffffffc0;
			writel(reg, S3C_GPHPU);
		}
		else if (width == 4) {
			/* MMC CLK1, MMC CMD1, MMC DATA1[0-3] - 0~5 */
			reg = readl(S3C_GPH0CON) & 0xff000000;
			writel(reg | 0x00222222, S3C_GPH0CON);
			reg = readl(S3C_GPHPU) & 0xfffff000;
			writel(reg, S3C_GPHPU);
		}
		else if (width == 8) {
			/* MMC CLK1, MMC CMD1, MMC DATA1[0-5] - 0~7 */
			writel(0x22222222, S3C_GPH0CON);

			/* MMC DATA1[6-7] 8~9 */
			writel(0x00000022, S3C_GPH1CON);

			reg = readl(S3C_GPHPU) & 0xfff00000;
			writel(reg, S3C_GPHPU);
		}
		break;

	/* can supports 1 and 4 bit bus, no irq_cd */
	case 2:
		/* MMC CLK2, MMC CMD2 */
		reg = readl(S3C_GPCCON) & 0xff00ffff;
		writel(reg | 0x00330000, S3C_GPCCON);
		reg = readl(S3C_GPCPU) & 0xfffff0ff;
		writel(reg, S3C_GPCPU);

		if (width == 1) {
			/* MMC DATA2[0] */
			reg = readl(S3C_GPH0CON) & 0xf0ffffff;
			writel(reg | 0x03000000, S3C_GPH0CON);
			reg = readl(S3C_GPHPU) & 0xfffff3ff;
			writel(reg, S3C_GPHPU);
		}
		else if (width == 4) {
			/* MMC DATA2[1] */
			reg = readl(S3C_GPH0CON) & 0x00ffffff;
			writel(reg | 0x33000000, S3C_GPH0CON);

			/* MMC DATA2[2-3] */
			writel(0x00000033, S3C_GPH1CON);

			reg = readl(S3C_GPHPU) & 0xfff00fff;
			writel(reg, S3C_GPHPU);
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
		.ctrl4 = 0,
	},
	
	/* ctrl for sd */
	.fd_ctrl[MMC_MODE_SD] = {
		.ctrl2 = 0xC0000100,			/* ctrl2 for sd */
		.ctrl3[SPEED_NORMAL] = 0,		/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0,		/* ctrl3 for high speed */
		.ctrl4 = 0,
	},
	
	.clk_name[0] = "hsmmc0",	/* 1st clock source */
	.clk_name[1] = "",		/* 2nd clock source */
	.clk_name[2] = "sclk_48m_mmc0",	/* 3rd clock source */
};

struct s3c_hsmmc_cfg s3c_hsmmc1_platform = {
	.hwport = 1,
	.enabled = 1,
	.host_caps = HOST_CAPS | MMC_CAP_8_BIT_DATA,
	.bus_width = 8,
	.highspeed = 0,

	/* ctrl for mmc */
	.fd_ctrl[MMC_MODE_MMC] = {
		.ctrl2 = 0xC0004100,			/* ctrl2 for mmc */
		.ctrl3[SPEED_NORMAL] = 0x80808080,	/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0x00000080,	/* ctrl3 for high speed */
		.ctrl4 = 0,
	},
	
	/* ctrl for sd */
	.fd_ctrl[MMC_MODE_SD] = {
		.ctrl2 = 0xC0000100,			/* ctrl2 for sd */
		.ctrl3[SPEED_NORMAL] = 0,		/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0,		/* ctrl3 for high speed */
		.ctrl4 = 0,
	},

	.clk_name[0] = "hsmmc1",	/* 1st clock source */
	.clk_name[1] = "",		/* 2nd clock source */
	.clk_name[2] = "sclk_48m_mmc1",	/* 3rd clock source */
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
		.ctrl4 = 0,
	},
	
	/* ctrl for sd */
	.fd_ctrl[MMC_MODE_SD] = {
		.ctrl2 = 0xC0000100,			/* ctrl2 for sd */
		.ctrl3[SPEED_NORMAL] = 0,		/* ctrl3 for low speed */
		.ctrl3[SPEED_HIGH]   = 0,		/* ctrl3 for high speed */
		.ctrl4 = 0,
	},
	
	.clk_name[0] = "hsmmc2",	/* 1st clock source */
	.clk_name[1] = "",		/* 2nd clock source */
	.clk_name[2] = "sclk_48m_mmc2",	/* 3rd clock source */
};

