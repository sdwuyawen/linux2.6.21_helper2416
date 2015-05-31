/* linux/include/asm-arm/arch-s3c2410/regs-pcm.h
 *
 * Copyright (c) 2008 Samsung Electronics  
 *	Ryu Euiyoul <ryu.real@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C PCM register definition
*/

#ifndef __ASM_ARCH_REGS_PCM_H
#define __ASM_ARCH_REGS_PCM_H

#define S3C_PCM_CTL		(0x00)

#define S3C_PCM_CTL_PCM_ENABLE	(1<<0)
#define S3C_PCM_CTL_RXFIFO_EN	(1<<1)
#define S3C_PCM_CTL_TXFIFO_EN	(1<<2)
#define S3C_PCM_CTL_RX_MSB_POS	(1<<3)
#define S3C_PCM_CTL_TX_MSB_POS	(1<<4)
#define S3C_PCM_CTL_RX_DMA_EN	(1<<5)
#define S3C_PCM_CTL_TX_DMA_EN	(1<<6)
#define S3C_PCM_CTL_RX_FIFO_DIPSTICK(x)		(((x)&0x3F)<<7)
#define S3C_PCM_CTL_TX_FIFO_DIPSTICK(x)		(((x)&0x3F)<<13)

#define S3C_PCM_CLKCTL		(0x04)

#define S3C_PCM_CLKCTL_SYNC_DIV(x)	(((x)&0x1FF)<<0)
#define S3C_PCM_CLKCTL_SCLK_DIV(x)	(((x)&0x1FF)<<9)
#define S3C_PCM_CTLCTL_SERCLK_SEL	(1<<18)
#define S3C_PCM_CTLCTL_SERCLK_EN	(1<<19)

#define S3C_PCM_TX_FIFO		(0x08)
#define S3C_PCM_TX_FIFO_DVALID	(1<<16)

#define S3C_PCM_RX_FIFO		(0x0C)
#define S3C_PCM_RX_FIFO_DVALID	(1<<16)

#define S3C_PCM_IRQ_CTL		(0x10)

#define S3C_PCM_IRQ_STAT	(0x14)

#define S3C_PCM_FIFO_STAT	(0x18)

#define S3C_PCM_FIFO_CLRINT	(0x20)


#endif /* __ASM_ARCH_REGS_PCM_H */
