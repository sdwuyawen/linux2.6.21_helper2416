/* linux/include/asm-arm/arch-s3c2410/debug-macro.S
 *
 * Debugging macro include header
 *
 *  Copyright (C) 1994-1999 Russell King
 *  Copyright (C) 2005 Simtec Electronics
 *
 *  Moved from linux/arch/arm/kernel/debug.S by Ben Dooks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/arch/map.h>
#include <asm/arch/regs-serial.h>
#include <asm/arch/regs-gpio.h>

#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410) 

#define S3C2410_UART1_OFF (0x4000)
#define SHIFT_2440TXF (14-9)

		.macro addruart, rx
		mrc	p15, 0, \rx, c1, c0
		tst	\rx, #1
		ldreq	\rx, = S3C24XX_PA_UART
		ldrne	\rx, = S3C24XX_VA_UART
#if CONFIG_DEBUG_S3C2410_UART != 0
		add	\rx, \rx, #(S3C2410_UART1_OFF * CONFIG_DEBUG_S3C2410_UART)
#endif
		.endm

		.macro	senduart,rd,rx
		strb 	\rd, [\rx, # S3C2410_UTXH ]
		.endm

		.macro	busyuart, rd, rx
		ldr	\rd, [ \rx, # S3C2410_UFCON ]
		tst	\rd, #S3C2410_UFCON_FIFOMODE	@ fifo enabled?
		beq	1001f				@
		@ FIFO enabled...
1003:
		@ check for arm920 vs arm926. currently assume all arm926
		@ devices have an 64 byte FIFO identical to the s3c2440
		mrc	p15, 0, \rd, c0, c0
		and	\rd, \rd, #0xff0
		teq	\rd, #0x260
		beq	1004f
		mrc	p15, 0, \rd, c1, c0
		tst	\rd, #1
		addeq	\rd, \rx, #(S3C24XX_PA_GPIO - S3C24XX_PA_UART)
		addne	\rd, \rx, #(S3C24XX_VA_GPIO - S3C24XX_VA_UART)
		bic	\rd, \rd, #0xff000
		ldr	\rd, [ \rd, # S3C2410_GSTATUS1 - S3C2410_GPIOREG(0) ]
		and	\rd, \rd, #0x00ff0000
		teq	\rd, #0x00440000		@ is it 2440?
1004:
		ldr	\rd, [ \rx, # S3C2410_UFSTAT ]
		moveq	\rd, \rd, lsr #SHIFT_2440TXF
		tst	\rd, #S3C2410_UFSTAT_TXFULL
		bne	1003b
		b	1002f

1001:
		@ busy waiting for non fifo
		ldr	\rd, [ \rx, # S3C2410_UTRSTAT ]
		tst	\rd, #S3C2410_UTRSTAT_TXFE
		beq	1001b

1002:		@ exit busyuart
		.endm

		.macro	waituart,rd,rx

		ldr	\rd, [ \rx, # S3C2410_UFCON ]
		tst	\rd, #S3C2410_UFCON_FIFOMODE	@ fifo enabled?
		beq	1001f				@
		@ FIFO enabled...
1003:
		mrc	p15, 0, \rd, c1, c0
		tst	\rd, #1
		addeq	\rd, \rx, #(S3C24XX_PA_GPIO - S3C24XX_PA_UART)
		addne	\rd, \rx, #(S3C24XX_VA_GPIO - S3C24XX_VA_UART)
		bic	\rd, \rd, #0xff000
		ldr	\rd, [ \rd, # S3C2410_GSTATUS1 - S3C2410_GPIOREG(0) ]
		and	\rd, \rd, #0x00ff0000
		teq	\rd, #0x00440000		@ is it 2440?

		ldr	\rd, [ \rx, # S3C2410_UFSTAT ]
		andne	\rd, \rd, #S3C2410_UFSTAT_TXMASK
		andeq	\rd, \rd, #S3C2440_UFSTAT_TXMASK
		teq	\rd, #0
		bne	1003b
		b	1002f

1001:
		@ idle waiting for non fifo
		ldr	\rd, [ \rx, # S3C2410_UTRSTAT ]
		tst	\rd, #S3C2410_UTRSTAT_TXFE
		beq	1001b

1002:		@ exit busyuart
		.endm

#else


		.macro  addruart,rx
		mrc     p15, 0, \rx, c1, c0
		tst     \rx, #1                 @ MMU enabled ?
		ldreq	\rx, = S3C24XX_PA_UART
		ldrne	\rx, = S3C24XX_VA_UART
		.endm

		.macro  senduart,rd,rx
		str     \rd, [\rx, #0x20]       @ UTXH
		.endm

		.macro  waituart,rd,rx
		.endm

		.macro  busyuart,rd,rx
1001:   ldr     \rd, [\rx, #0x10]       @ read UTRSTAT
		tst     \rd, #1 << 2            @ TX_EMPTY ?
		beq     1001b
		.endm

#endif
