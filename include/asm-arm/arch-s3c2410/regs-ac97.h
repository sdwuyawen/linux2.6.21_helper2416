/* linux/include/asm-arm/arch-s3c2410/regs-ac97.h
 *
 * Copyright (c) 2006 Simtec Electronics <linux@simtec.co.uk>
 *		http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * S3C2440 AC97 Controller
*/

#ifndef __ASM_ARCH_REGS_AC97_H
#define __ASM_ARCH_REGS_AC97_H __FILE__

#define S3C_AC97_GLBCTRL				(0x00)

#define S3C_AC97_GLBCTRL_CODECREADYIE			(1<<22)
#define S3C_AC97_GLBCTRL_PCMOUTURIE			(1<<21)
#define S3C_AC97_GLBCTRL_PCMINORIE			(1<<20)
#define S3C_AC97_GLBCTRL_MICINORIE			(1<<19)
#define S3C_AC97_GLBCTRL_PCMOUTTIE			(1<<18)
#define S3C_AC97_GLBCTRL_PCMINTIE			(1<<17)
#define S3C_AC97_GLBCTRL_MICINTIE			(1<<16)
#define S3C_AC97_GLBCTRL_PCMOUTTM_OFF			(0<<12)
#define S3C_AC97_GLBCTRL_PCMOUTTM_PIO			(1<<12)
#define S3C_AC97_GLBCTRL_PCMOUTTM_DMA			(2<<12)
#define S3C_AC97_GLBCTRL_PCMOUTTM_MASK			(3<<12)
#define S3C_AC97_GLBCTRL_PCMINTM_OFF			(0<<10)
#define S3C_AC97_GLBCTRL_PCMINTM_PIO			(1<<10)
#define S3C_AC97_GLBCTRL_PCMINTM_DMA			(2<<10)
#define S3C_AC97_GLBCTRL_PCMINTM_MASK			(3<<10)
#define S3C_AC97_GLBCTRL_MICINTM_OFF			(0<<8)
#define S3C_AC97_GLBCTRL_MICINTM_PIO			(1<<8)
#define S3C_AC97_GLBCTRL_MICINTM_DMA			(2<<8)
#define S3C_AC97_GLBCTRL_MICINTM_MASK			(3<<8)
#define S3C_AC97_GLBCTRL_TRANSFERDATAENABLE		(1<<3)
#define S3C_AC97_GLBCTRL_ACLINKON			(1<<2)
#define S3C_AC97_GLBCTRL_WARMRESET			(1<<1)
#define S3C_AC97_GLBCTRL_COLDRESET			(1<<0)

#define S3C_AC97_GLBSTAT				(0x04)

#define S3C_AC97_GLBSTAT_CODECREADY			(1<<22)
#define S3C_AC97_GLBSTAT_PCMOUTUR			(1<<21)
#define S3C_AC97_GLBSTAT_PCMINORI			(1<<20)
#define S3C_AC97_GLBSTAT_MICINORI			(1<<19)
#define S3C_AC97_GLBSTAT_PCMOUTTI			(1<<18)
#define S3C_AC97_GLBSTAT_PCMINTI			(1<<17)
#define S3C_AC97_GLBSTAT_MICINTI			(1<<16)
#define S3C_AC97_GLBSTAT_MAINSTATE_IDLE			(0<<0)
#define S3C_AC97_GLBSTAT_MAINSTATE_INIT			(1<<0)
#define S3C_AC97_GLBSTAT_MAINSTATE_READY		(2<<0)
#define S3C_AC97_GLBSTAT_MAINSTATE_ACTIVE		(3<<0)
#define S3C_AC97_GLBSTAT_MAINSTATE_LP			(4<<0)
#define S3C_AC97_GLBSTAT_MAINSTATE_WARM			(5<<0)

#define S3C_AC97_CODEC_CMD				(0x08)

#define S3C_AC97_CODEC_CMD_READ				(1<<23)

#define S3C_AC97_STAT					(0x0c)
#define S3C_AC97_PCM_ADDR				(0x10)
#define S3C_AC97_PCM_DATA				(0x18)
#define S3C_AC97_MIC_DATA				(0x1C)



#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410) 

#include <asm/arch/bitfield.h>

/*************************************************************
			AC-97 interface registers	     */

#define AC97_CTL_BASE		S3C24XX_VA_AC97

#define S3C_AC97REG(x)		((x) + AC97_CTL_BASE)
#define S3C_AC97REG_PHYS(x)	((x) + S3C24XX_PA_AC97)

#define AC_GLBCTRL	S3C_AC97REG(0x00) /* global control */
#define AC_GLBSTAT	S3C_AC97REG(0x04) /* global status */
#define AC_CODEC_CMD	S3C_AC97REG(0x08) /* codec command */
#define AC_CODEC_STAT	S3C_AC97REG(0x0c) /* codec status */
#define AC_PCMADDR	S3C_AC97REG(0x10) /* PCM out/in FIFO address */
#define AC_MICADDR	S3C_AC97REG(0x14) /* MIC in FIFO address */
#define AC_PCMDATA	S3C_AC97REG(0x18) /* PCM out/in FIFO data */
#define AC_MICDATA	S3C_AC97REG(0x1c) /* MIC in FIFO data */
#define AC_CLRINT	S3C_AC97REG(0x20) /* MIC in FIFO data */

#define AC_PCMDATA_PHYS   S3C_AC97REG_PHYS(0x18) /* PCM out/in FIFO data */
#define AC_MICDATA_PHYS   S3C_AC97REG_PHYS(0x1c) /* MIC in FIFO data */

#define AC_GLBCTRL_RIE		(1<<22) /* codec ready interrupt enable */
#define AC_GLBCTRL_POUIE	(1<<21) /* PCM out underrun interrupt enable */
#define AC_GLBCTRL_PIOIE	(1<<20) /* PCM in overrun interrupt enable */
#define AC_GLBCTRL_MIOIE	(1<<19) /* MIC in underrun interrupt enable */
#define AC_GLBCTRL_POTIE	(1<<18) /* PCM out threshold interrupt enable */
#define AC_GLBCTRL_PITIE	(1<<17) /* PCM in threshold interrupt enable */
#define AC_GLBCTRL_MITIE	(1<<16) /* MIC in threshold interrupt enable */
#define  fAC_GLBCTRL_POMODE	Fld(2,12) /* PCM out transfer mode */
#define AC_GLBCTRL_POMODE	FMsk(fAC_GLBCTRL_POMODE)
#define AC_GLBCTRL_POMODE_OFF	FInsrt(0, fAC_GLBCTRL_POMODE) /* off */
#define AC_GLBCTRL_POMODE_PIO	FInsrt(1, fAC_GLBCTRL_POMODE) /* PIO */
#define AC_GLBCTRL_POMODE_DMA	FInsrt(2, fAC_GLBCTRL_POMODE) /* DMA */
#define  fAC_GLBCTRL_PIMODE	Fld(2,10) /* PCM in transfer mode */
#define AC_GLBCTRL_PIMODE	FMsk(fAC_GLBCTRL_PIMODE)
#define AC_GLBCTRL_PIMODE_OFF	FInsrt(0, fAC_GLBCTRL_PIMODE) /* off */
#define AC_GLBCTRL_PIMODE_PIO	FInsrt(1, fAC_GLBCTRL_PIMODE) /* PIO */
#define AC_GLBCTRL_PIMODE_DMA	FInsrt(2, fAC_GLBCTRL_PIMODE) /* DMA */
#define  fAC_GLBCTRL_MIMODE	Fld(2,8) /* MIC in transfer mode */
#define AC_GLBCTRL_MIMODE	FMsk(fAC_GLBCTRL_MIMODE)
#define AC_GLBCTRL_MIMODE_OFF	FInsrt(0, fAC_GLBCTRL_MIMODE) /* off */
#define AC_GLBCTRL_MIMODE_PIO	FInsrt(1, fAC_GLBCTRL_MIMODE) /* PIO */
#define AC_GLBCTRL_MIMODE_DMA	FInsrt(2, fAC_GLBCTRL_MIMODE) /* DMA */
#define AC_GLBCTRL_TE	       (1<<3) /* Transfer data enable using AC-link */
#define AC_GLBCTRL_AE	       (1<<2) /* AC-link on */
#define AC_GLBCTRL_WARM        (1<<1) /* warm reset */
#define AC_GLBCTRL_COLD        (1<<0) /* cold reset */

#define AC_GLBSTAT_RI	       (1<<22) /* codec ready interrupt */
#define AC_GLBSTAT_POUI        (1<<21) /* PCM out underrun interrupt */
#define AC_GLBSTAT_PIOI        (1<<20) /* PCM in overrun interrupt */
#define AC_GLBSTAT_MIOI        (1<<19) /* MIC in overrun interrupt */
#define AC_GLBSTAT_POTI        (1<<18) /* PCM out threshold interrupt */
#define AC_GLBSTAT_PITI        (1<<17) /* PCM in threshold interrupt */
#define AC_GLBSTAT_MITI        (1<<16) /* MIC in threshold interrupt */
#define fAC_GLBSTAT_CSTAT	Fld(3,0) /* Controller main state */
#define AC_GLBSTAT_CSTAT()     FExtr(__raw_readl(AC_GLBSTAT), fAC_GLBSTAT_CSTAT)
#define AC_CSTAT_IDLE	(0)
#define AC_CSTAT_INIT	(1)
#define AC_CSTAT_READY	(2)
#define AC_CSTAT_ACTIVE (3)
#define AC_CSTAT_LP	(4)
#define AC_CSTAT_WARM	(5)

#define AC_CMD_R (1<<23) /* read enable - 0: command write, 1: status read */

#define fAC_CMD_ADDR Fld(7, 16) /* codec command address */
#define AC_CMD_ADDR(x) FInsrt((x), fAC_CMD_ADDR)
#define fAC_CMD_DATA Fld(16, 0) /* codec command data */
#define AC_CMD_DATA(x) FInsrt((x), fAC_CMD_DATA)

#define fAC_STAT_ADDR Fld(7, 16) /* codec status address */
#define AC_STAT_ADDR(x) FExtr((x), fAC_STAT_ADDR)
#define fAC_STAT_DATA Fld(16, 0) /* codec status data */
#define AC_STAT_DATA(x) FExtr((x), fAC_STAT_DATA)

#define DMA_CH0 	0
#define DMA_CH1 	1
#define DMA_CH2 	2
#define DMA_CH3 	3
#define MODE_PLAY	1
#define MODE_REC	0

#define DMA_BUF_WR	1
#define DMA_BUF_RD	0

#endif

#endif /* __ASM_ARCH_REGS_AC97_H */
