/* linux/arch/arm/mach-s3c64xx/clock_s3c6400.c
 *
 * Copyright (c) 2004 Simtec Electronics
 * Ben Dooks <ben@simtec.co.uk>
 *
 * S3C2412 Clock control support
 *
 * Based on, and code from linux/arch/arm/mach-versatile/clock.c
 **
 **  Copyright (C) 2004 ARM Limited.
 **  Written by Deep Blue Solutions Limited.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/sysdev.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/clk.h>
#include <linux/mutex.h>

#include <asm/mach/map.h>

#include <asm/hardware.h>
#include <asm/atomic.h>
#include <asm/irq.h>
#include <asm/io.h>

#include <asm/plat-s3c24xx/clock.h>
#include <asm/plat-s3c24xx/cpu.h>
#include <asm/arch/regs-s3c6410-clock.h>

#ifdef CONFIG_USB_OHCI_HCD
#include <asm/arch/regs-usb-otg-hs.h>
#endif

#define HCLK_GATING_ON_LIST	S3C_CLKCON_HCLK_BUS | S3C_CLKCON_HCLK_DDR1 | S3C_CLKCON_HCLK_DDR0 |\
				S3C_CLKCON_HCLK_MEM1 | S3C_CLKCON_HCLK_MEM0 | S3C_CLKCON_HCLK_DMA0 |\
				S3C_CLKCON_HCLK_DMA1 | S3C_CLKCON_HCLK_INTC | S3C_CLKCON_HCLK_LCD |\
				S3C_CLKCON_HCLK_DHOST | S3C_CLKCON_HCLK_POST0 | S3C_CLKCON_HCLK_MFC

#define PCLK_GATING_ON_LIST	S3C_CLKCON_PCLK_GPIO | S3C_CLKCON_PCLK_UART0 | S3C_CLKCON_PCLK_UART1 | S3C_CLKCON_PCLK_MFC

#define SCLK_GATING_ON_LIST	S3C_CLKCON_SCLK_UART | S3C_CLKCON_SCLK_LCD27 | S3C_CLKCON_SCLK_LCD | S3C_CLKCON_SCLK_POST1_27 |	\
				S3C_CLKCON_SCLK_POST0_27 | S3C_CLKCON_SCLK_POST1 | S3C_CLKCON_SCLK_POST0 | S3C_CLKCON_SCLK_MFC

/* Select the source clock of USB Host. 0:48MHz, 1:FoutEpll */
#define S3C_USB_CLKSRC_EPLL 0

/* old functions */
void inline s3c_clk_enable (uint clocks, uint enable, ulong gate_reg)
{
	unsigned long clkcon;
	unsigned long flags;

	local_irq_save(flags);

	clkcon = __raw_readl(gate_reg);
	clkcon &= ~clocks;

	if (enable)
		clkcon |= clocks;

	__raw_writel(clkcon, gate_reg);

	local_irq_restore(flags);
}


/*This function returns the virtual address of gating register*/
ulong clk_get_gate_reg (struct clk *clk)
{
	struct clk *parent_clk = clk_get_parent(clk) ;

	if (strcmp(parent_clk->name, "hclk") == 0)
		return (unsigned long) S3C_HCLK_GATE;
	else if (strcmp(parent_clk->name, "pclk") == 0)
		return (unsigned long) S3C_PCLK_GATE;
	else if (strcmp(parent_clk->name, "clk48m") == 0)
		return (unsigned long) S3C_OTHERS;
	else
		return (unsigned long) S3C_SCLK_GATE;
}


int s3c_clkcon_enable (struct clk *clk, int enable)
{
	unsigned long gate_reg;

	gate_reg = clk_get_gate_reg(clk);
	s3c_clk_enable(clk->ctrlbit, enable, gate_reg);
	return 0;
}

static unsigned long s3c6400_clk_getrate(struct clk *clk)
{
	/* set clock rate to use in drivers */
	if (!clk->rate) {
		if (clk->parent) {
			clk->rate = clk->parent->rate;
		}
	}
	return clk->rate;
}

#define CAMDIV_val     20

static int s3c6410_setrate_camera(struct clk *clk, unsigned long rate)
{
	unsigned int camclk_div, val;
	unsigned long src_clk = clk_get_rate(clk->parent);

	if (rate == 4800000) {
		printk(KERN_INFO "External camera clock is set to 48MHz\n");
	}
	else if (rate > 48000000) {
		printk(KERN_ERR "Invalid camera clock\n");
	}

	camclk_div = src_clk / rate;
	printk("Parent clock for Camera = %ld, CAMDIV = %d\n", src_clk, camclk_div);

	val = readl(S3C_CLK_DIV0);
	val &= ~(0xf<<CAMDIV_val);
	writel(val, S3C_CLK_DIV0);

	/* CAM CLK DIVider Ratio = (EPLL clk)/(camclk_div) */
	val |= ((camclk_div - 1) << CAMDIV_val);
	writel(val, S3C_CLK_DIV0);
	val = readl(S3C_CLK_DIV0);

	return 0;
}

static struct clk init_clocks_disable[] = {
	{
		.name		= "nand",
		.id		= -1,
		.parent		= &clk_h,
		.enable		= s3c_clkcon_enable,
		//.ctrlbit	= S3C_CLKCON_HCLK_NAND,
	}, {
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s3c_clkcon_enable,
		.ctrlbit	= S3C_CLKCON_PCLK_TSADC,
	}, {
		.name		= "i2c",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s3c_clkcon_enable,
		.ctrlbit	= S3C_CLKCON_PCLK_IIC,
	}, {
		.name		= "iis",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s3c_clkcon_enable,
		.ctrlbit	= S3C_CLKCON_PCLK_IIS0,
	}, {
		.name		= "iis",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s3c_clkcon_enable,
		.ctrlbit	= S3C_CLKCON_PCLK_IIS1,
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s3c_clkcon_enable,
		.ctrlbit	= S3C_CLKCON_PCLK_SPI0,
	}, {
		.name		= "spi",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s3c_clkcon_enable,
		.ctrlbit	= S3C_CLKCON_PCLK_SPI1,
	}
};


/* standard clock definitions */
static struct clk init_clocks[] = {
    	/* AHB devices */
	{ .name    = "dma 1",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_DMA1,
	},
	{ .name    = "dma 0",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_DMA0,
	},
	{ .name    = "lcd",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_LCD,
	},
	{ .name    = "TZIC",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_TZIC,
	},
	{ .name    = "INTC",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_INTC,
	},
	{ .name    = "camif",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_CAMIF
	},
	{ .name    = "otg",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_USB
	},
	{ .name    = "hsmmc0",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .get_rate = s3c6400_clk_getrate,
	  .ctrlbit = S3C_CLKCON_HCLK_HSMMC0
	},
	{ .name    = "hsmmc1",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .get_rate = s3c6400_clk_getrate,
	  .ctrlbit = S3C_CLKCON_HCLK_HSMMC1
	},
	{ .name    = "hsmmc2",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .get_rate = s3c6400_clk_getrate,
	  .ctrlbit = S3C_CLKCON_HCLK_HSMMC2
	},
	{ .name    = "hclk_mfc",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_MFC
	},
	{ .name    = "hclk_post0",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_POST0
	},
	{ .name    = "hclk_jpeg",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_JPEG
	},
	{ .name    = "tv_encoder",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_TV
	},
	{ .name    = "tv_scaler",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_SCALER
	},
	/* register to use HS-MMC clock */
	{ .name    = "sclk_48m",
	  .id	   = -1,
	  .parent  = &clk_48m,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = 1<<16,	/* USB SIG Mask */
	  .usage   = 0,
	  .rate    = 48*1000*1000,
	},
	{ .name    = "sclk_48m_mmc0",
	  .id	   = -1,
	  .parent  = &clk_s,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_SCLK_MMC0_48,
	  .usage   = 0,
	  .rate    = 48*1000*1000,
	},
	{ .name    = "sclk_48m_mmc1",
	  .id	   = -1,
	  .parent  = &clk_s,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_SCLK_MMC1_48,
	  .usage   = 0,
	  .rate    = 48*1000*1000,
	},
	{ .name    = "sclk_48m_mmc2",
	  .id	   = -1,
	  .parent  = &clk_s,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_SCLK_MMC2_48,
	  .usage   = 0,
	  .rate    = 48*1000*1000,
	},
	{ .name    = "sclk_mfc",
	  .id	   = -1,
	  .parent  = &clk_s,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_SCLK_MFC,
	  .usage   = 0,
	  .rate    = 48*1000*1000,
	},
	{ .name    = "sclk_jpeg",
	  .id	   = -1,
	  .parent  = &clk_s,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_SCLK_JPEG,
	  .usage   = 0,
	  .rate    = 48*1000*1000,
	},
	{ .name    = "cfata",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_IHOST
	},

	/* APB Devices */
	{ .name    = "RTC",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_RTC,
	},
	{ .name    = "GPIO",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_GPIO,
	},
	{ .name    = "UART2",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_UART2,
	},
	{ .name    = "UART1",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_UART1,
	},
	{ .name    = "UART0",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_UART0,
	},
	{ .name    = "timers",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_PWM,
	},
	{ .name    = "watchdog",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_WDT,
	},
	{ .name    = "i2c",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_IIC,
	},
	{ .name    = "spi0",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_SPI0,
	},
	{ .name    = "spi1",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_SPI1,
	},
	{ .name    = "usb-host",
	  .id	   = -1,
	  .parent  = &clk_h,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_HCLK_UHOST,
	},
	{ .name    = "adc",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_TSADC,
	},
	{ .name    = "pclk_mfc",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_MFC,
	},
	{
	  .name     = "camera",
	  .id	    = -1,
	  .parent   = &clk_hx2,
	  .enable   = s3c_clkcon_enable,
	  .ctrlbit  = S3C_CLKCON_SCLK_CAM,
	  .set_rate = s3c6410_setrate_camera,
	},
	{ .name    = "pcmif_0",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_PCM0,
	},
	{ .name    = "pcmif_1",
	  .id	   = -1,
	  .parent  = &clk_p,
	  .enable  = s3c_clkcon_enable,
	  .ctrlbit = S3C_CLKCON_PCLK_PCM1,
	},

};


int s3c6410_epll_clk_uart_enable (struct clk *clk, int enable)
{
	unsigned long gate_reg;

	gate_reg = clk_get_gate_reg(clk);
	s3c_clk_enable(clk->ctrlbit, enable, gate_reg);

	/* MUXepll : FOUTepll */
	writel(readl(S3C_CLK_SRC)|S3C_CLKSRC_EPLL_CLKSEL, S3C_CLK_SRC);

	/* UARTsel : MOUTepll */
	writel((readl(S3C_CLK_SRC) & ~(0x1 << 13)) | (0 << 13), S3C_CLK_SRC);

	return 0;
}


static int s3c6410_setrate_epll_clk_192m(struct clk *clk, unsigned long rate)
{
	writel(S3C_EPLL_EN|S3C_EPLLVAL(32,1,1), S3C_EPLL_CON0);
	writel(0, S3C_EPLL_CON1);
	mdelay(5);

	return 0;
}

static unsigned long s3c6410_getrate_epll_clk_192m(struct clk *clk)
{
	s3c6410_setrate_epll_clk_192m(clk, 192000000);
	clk->rate = 192000000;
	return clk->rate;
}


static struct clk clk_epll_uart_192m = {
	  .name    = "epll_clk_uart_192m",
	  .id	   = -1,
	  .parent  = &clk_epll,
	  .enable  = s3c6410_epll_clk_uart_enable,
	  .ctrlbit = S3C_CLKCON_SCLK_UART,
	  .set_rate = s3c6410_setrate_epll_clk_192m,
	  .get_rate = s3c6410_getrate_epll_clk_192m,
};

/* function for UART Clock with MPLL by boyko */

static unsigned long s3c6410_mpll_get_clk(struct clk *clk)
{
	unsigned long mpll_con;
	unsigned long m = 0;
	unsigned long p = 0;
	unsigned long s = 0;
	unsigned long ret;

	mpll_con = (readl(S3C_MPLL_CON));

	m = (mpll_con >> 16) & 0x3ff;
	p = (mpll_con >> 8) & 0x3f;
	s = mpll_con & 0x3;

	ret = (m * (12000000 / (p * (1 << s))));

	return (((readl(S3C_CLK_DIV0) >> 13 ) & 0x1) ? (ret / 2) : ret );

}

static int s3c6410_mpll_clk_uart_enable (struct clk *clk, int enable)
{
	unsigned long gate_reg;

	gate_reg = clk_get_gate_reg(clk);
	s3c_clk_enable(clk->ctrlbit, enable, gate_reg);

	writel(readl(S3C_CLK_SRC)|S3C_CLKSRC_EPLL_CLKSEL, S3C_CLK_SRC);

	writel((readl(S3C_CLK_SRC) & ~(0x1 << 13)) | (1 << 13), S3C_CLK_SRC);

	return 0;
}


static int s3c6410_setrate_mpll_clk(struct clk *clk, unsigned long rate)
{
	writel(readl(S3C_CLK_DIV2) & ~(0xf << 16) , S3C_CLK_DIV2);
	return 0;
}

static unsigned long s3c6410_getrate_mpll_clk(struct clk *clk)
{
	s3c6410_setrate_mpll_clk(clk,0);

	return s3c6410_mpll_get_clk(clk);
}

static struct clk clk_mpll_uart = {
	  .name    = "mpll_clk_uart",
	  .id	   = -1,
	  .parent  = &clk_mpll,
	  .enable  = s3c6410_mpll_clk_uart_enable,
	  .ctrlbit = S3C_CLKCON_SCLK_UART,
	  .set_rate = s3c6410_setrate_mpll_clk,
	  .get_rate = s3c6410_getrate_mpll_clk,
};


/* For  HS-MMC controller */
static unsigned long s3c6410_getrate_DOUTmpll_hsmmc_clk(struct clk *clk)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	unsigned long div = readl(S3C_CLK_DIV1);
	div &= ~S3C_CLKDIV1_HSMMCDIV_MASK;
	
	/* MMC_RATIO = 2+1 */
	div |= 0x2;
	
	writel(div, S3C_CLK_DIV1);

	return parent_rate / (0x2 + 1);
}

static unsigned long s3c6410_getrate_DOUTmpll_hsmmc1_clk(struct clk *clk)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	unsigned long div = readl(S3C_CLK_DIV1);
	div &= ~S3C_CLKDIV1_HSMMCDIV1_MASK;
	
	/* MMC1_RATIO = 2+1 */
	div |= (0x2<<S3C_CLKDIV1_HSMMCDIV1_SHIFT);
	
	writel(div, S3C_CLK_DIV1);
	
	return parent_rate / (0x2 + 1);
}

static struct clk clk_hsmmc_DOUTmpll_mmc0 = {
	.name    	= "sclk_DOUTmpll_mmc0",
	.id	   	= -1,
	.parent  	= &clk_mpll_uart,
	.enable  	= s3c_clkcon_enable,
	.ctrlbit 	= S3C_CLKCON_SCLK_MMC0,
	.get_rate 	= s3c6410_getrate_DOUTmpll_hsmmc_clk,
	.usage   	= 0,
};

static struct clk clk_hsmmc_DOUTmpll_mmc1 = {
	.name    	= "sclk_DOUTmpll_mmc1",
	.id	   	= -1,
	.parent  	= &clk_mpll_uart,
	.enable  	= s3c_clkcon_enable,
	.ctrlbit 	= S3C_CLKCON_SCLK_MMC1,
	.get_rate 	= s3c6410_getrate_DOUTmpll_hsmmc1_clk,
	.usage   	= 0,
};

static struct clk clk_hsmmc_DOUTmpll_mmc2 = {
	.name    	= "sclk_DOUTmpll_mmc2",
	.id	   	= -1,
	.parent  	= &clk_mpll_uart,
	.enable  	= s3c_clkcon_enable,
	.ctrlbit 	= S3C_CLKCON_SCLK_MMC2,
	.get_rate 	= s3c6410_getrate_DOUTmpll_hsmmc_clk,
	.usage   	= 0,
};


/* clocks to add straight away */
static struct clk *clks[] __initdata = {
	&clk_epll_uart_192m,
	&clk_mpll_uart,
	&clk_hsmmc_DOUTmpll_mmc0,
	&clk_hsmmc_DOUTmpll_mmc1,
	&clk_hsmmc_DOUTmpll_mmc2,
};

void __init s3c6400_init_clocks(int xtal)
{
	unsigned long clkdiv0;
	unsigned long fclk, arm_clk;
	unsigned long hclkx2;
	unsigned long hclk;
	unsigned long pclk;
	unsigned long epll_clk;

	struct clk *clkp;
	int ret;
	int ptr;

	clk_xtal.rate = xtal;

	/* now we've got our machine bits initialised, work out what
	 * clocks we've got */

	/* Changed below lines to support s3c6400 - JaeCheol Lee */
	fclk = s3c6400_get_pll(__raw_readl(S3C_APLL_CON), xtal);
	clkdiv0 = __raw_readl(S3C_CLK_DIV0);

        arm_clk = fclk/((clkdiv0 & S3C_CLKDIVN_APLL_MASK)+1);

	if(!(__raw_readl(S3C_OTHERS) & 0x80)) {
		fclk = s3c6400_get_pll(__raw_readl(S3C_MPLL_CON), xtal);
	}

        hclkx2 = fclk / (((clkdiv0 & S3C_CLKDIVN_HCLKX2_MASK)>>9)+1);

	hclk = hclkx2 / (((clkdiv0 & S3C_CLKDIVN_HCLK_MASK)>>8)+1);

        //it should get the divisor from register and then sets the pclk properly
        pclk = hclkx2 / (((clkdiv0 & S3C_CLKDIVN_PCLK_MASK)>>12)+1);
        /* print brief summary of clocks, etc */

        printk("S3C6400: core %ld.%03ld MHz, memory %ld.%03ld MHz, peripheral %ld.%03ld MHz\n",
               print_mhz(arm_clk), print_mhz(hclk),
               print_mhz(pclk));

	/* Default EPLL frequency : 192Mhz */
	writel(S3C_EPLL_EN|S3C_EPLLVAL(32,1,1), S3C_EPLL_CON0);
	writel(0, S3C_EPLL_CON1);
	mdelay(50);

	epll_clk = s3c6400_get_epll(xtal);
	clk_epll.rate = epll_clk;
	printk("S3C6400: EPLL %ld.%03ld MHz\n", print_mhz(epll_clk));

	/* initialize hclkx2 by jsgood */
	clk_hx2.rate = hclkx2;

        /* initialise the clocks here, to allow other things like the
         * console to use them, and to add new ones after the initialisation
         */

        s3c24xx_setup_clocks(xtal, arm_clk, hclk, pclk);

	for (ptr = 0; ptr < ARRAY_SIZE(clks); ptr++) {
		clkp = clks[ptr];

		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

	/* register clocks from clock array */
	clkp = init_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}

		/* set clock rate to use in drivers */
		if (!clkp->rate) {
			if (clkp->parent) {
				clkp->rate = clkp->parent->rate;
			}
		}

	}

#ifdef CONFIG_USB_OHCI_HCD
	if (S3C_USB_CLKSRC_EPLL == 1) {
		writel((readl(S3C_CLK_SRC)& ~S3C_CLKSRC_UHOST_MASK)
			|S3C_CLKSRC_EPLL_CLKSEL|S3C_CLKSRC_UHOST_EPLL,
			S3C_CLK_SRC);

		/* USB host colock divider ratio is 2 */
		writel((readl(S3C_CLK_DIV1)& ~S3C_CLKDIVN_UHOST_MASK)
			|S3C_CLKDIV1_USBDIV2, S3C_CLK_DIV1);
	} else {  /* 48MHz */
		writel(readl(S3C_CLK_SRC)& ~S3C_CLKSRC_UHOST_MASK, S3C_CLK_SRC);
		writel(readl(S3C_OTHERS)|S3C_OTHERS_USB_SIG_MASK, S3C_OTHERS);

		writel(0x0, S3C_USBOTG_PHYPWR);
		writel(0x20, S3C_USBOTG_PHYCLK);
		writel(0x1, S3C_USBOTG_RSTCON);
		udelay(50);
		writel(0x0, S3C_USBOTG_RSTCON);
		udelay(50);

		/* USB host colock divider ratio is 1 */
		writel(readl(S3C_CLK_DIV1)& ~S3C_CLKDIVN_UHOST_MASK, S3C_CLK_DIV1);
	}

	writel(readl(S3C_HCLK_GATE)|S3C_CLKCON_HCLK_UHOST|S3C_CLKCON_HCLK_SECUR,
		S3C_HCLK_GATE);
	writel(readl(S3C_SCLK_GATE)|S3C_CLKCON_SCLK_UHOST, S3C_SCLK_GATE);

#endif


	/* We must be careful disabling the clocks we are not intending to
	 * be using at boot time, as subsytems such as the LCD which do
	 * their own DMA requests to the bus can cause the system to lockup
	 * if they where in the middle of requesting bus access.
	 *
	 * Disabling the LCD clock if the LCD is active is very dangerous,
	 * and therefore the bootloader should be careful to not enable
	 * the LCD clock if it is not needed.
	*/

	/* install (and disable) the clocks we do not need immediately */

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {

		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}

		(clkp->enable)(clkp, 0);
	}
}


/* clocks that could be registered by external code */
struct clk s3c_dclk0 = {
	.name		= "dclk0",
	.id		= -1,
};

struct clk s3c_dclk1 = {
	.name		= "dclk1",
	.id		= -1,
};

struct clk s3c_clkout0 = {
	.name		= "clkout0",
	.id		= -1,
};

struct clk s3c_clkout1 = {
	.name		= "clkout1",
	.id		= -1,
};

