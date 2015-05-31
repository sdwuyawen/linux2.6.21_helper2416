/* linux/include/asm-arm/arch-s3c2410/regs-serial.h
 *
 *  From linux/include/asm-arm/hardware/serial_s3c2410.h
 *
 *  Internal header file for Samsung S3C2410 serial ports (UART0-2)
 *
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *
 *  Additional defines, (c) 2003 Simtec Electronics (linux@simtec.co.uk)
 *
 *  Adapted from:
 *
 *  Internal header file for MX1ADS serial ports (UART1 & 2)
 *
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
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

#ifndef __ASM_ARM_REGS_SERIAL_H
#define __ASM_ARM_REGS_SERIAL_H

#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410)
#define S3C24XX_VA_UART0      (S3C24XX_VA_UART)
#define S3C24XX_VA_UART1      (S3C24XX_VA_UART + 0x400 )
#define S3C24XX_VA_UART2      (S3C24XX_VA_UART + 0x800 )
#define S3C24XX_VA_UART3      (S3C24XX_VA_UART + 0xC00 )

#define S3C2410_PA_UART0      (S3C24XX_PA_UART)
#define S3C2410_PA_UART1      (S3C24XX_PA_UART + 0x400 )
#define S3C2410_PA_UART2      (S3C24XX_PA_UART + 0x800 )
#define S3C2443_PA_UART3      (S3C24XX_PA_UART + 0xC00 )
#else
#define S3C24XX_VA_UART0      (S3C24XX_VA_UART)
#define S3C24XX_VA_UART1      (S3C24XX_VA_UART + 0x4000 )
#define S3C24XX_VA_UART2      (S3C24XX_VA_UART + 0x8000 )
#define S3C24XX_VA_UART3      (S3C24XX_VA_UART + 0xC000 )

#define S3C2410_PA_UART0      (S3C24XX_PA_UART)
#define S3C2410_PA_UART1      (S3C24XX_PA_UART + 0x4000 )
#define S3C2410_PA_UART2      (S3C24XX_PA_UART + 0x8000 )
#define S3C2443_PA_UART3      (S3C24XX_PA_UART + 0xC000 )
#endif

#define S3C2410_URXH	  (0x24)
#define S3C2410_UTXH	  (0x20)
#define S3C2410_ULCON	  (0x00)
#define S3C2410_UCON	  (0x04)
#define S3C2410_UFCON	  (0x08)
#define S3C2410_UMCON	  (0x0C)
#define S3C2410_UBRDIV	  (0x28)
#define S3C2410_UTRSTAT	  (0x10)
#define S3C2410_UERSTAT	  (0x14)
#define S3C2410_UFSTAT	  (0x18)
#define S3C2410_UMSTAT	  (0x1C)

#define S3C2410_LCON_CFGMASK	  ((0xF<<3)|(0x3))

#define S3C2410_LCON_CS5	  (0x0)
#define S3C2410_LCON_CS6	  (0x1)
#define S3C2410_LCON_CS7	  (0x2)
#define S3C2410_LCON_CS8	  (0x3)
#define S3C2410_LCON_CSMASK	  (0x3)

#define S3C2410_LCON_PNONE	  (0x0)
#define S3C2410_LCON_PEVEN	  (0x5 << 3)
#define S3C2410_LCON_PODD	  (0x4 << 3)
#define S3C2410_LCON_PMASK	  (0x7 << 3)

#define S3C2410_LCON_STOPB	  (1<<2)
#define S3C2410_LCON_IRM          (1<<6)

#define S3C2440_UCON_CLKMASK	  (3<<10)
#define S3C2440_UCON_PCLK	  (0<<10)
#define S3C2440_UCON_UCLK	  (1<<10)
#define S3C2440_UCON_PCLK2	  (2<<10)
#define S3C2440_UCON_FCLK	  (3<<10)
#define S3C2443_UCON_EPLL	  (3<<10)

#define S3C2440_UCON2_FCLK_EN	  (1<<15)
#define S3C2440_UCON0_DIVMASK	  (15 << 12)
#define S3C2440_UCON1_DIVMASK	  (15 << 12)
#define S3C2440_UCON2_DIVMASK	  (7 << 12)
#define S3C2440_UCON_DIVSHIFT	  (12)

#define S3C2412_UCON_CLKMASK	(3<<10)
#define S3C2412_UCON_UCLK	(1<<10)
#define S3C2412_UCON_USYSCLK	(3<<10)
#define S3C2412_UCON_PCLK	(0<<10)
#define S3C2412_UCON_PCLK2	(2<<10)

#define S3C2410_UCON_UCLK	  (1<<10)
#define S3C2410_UCON_SBREAK	  (1<<4)

#define S3C2410_UCON_TXILEVEL	  (1<<9)
#define S3C2410_UCON_RXILEVEL	  (1<<8)
#define S3C2410_UCON_TXIRQMODE	  (1<<2)
#define S3C2410_UCON_RXIRQMODE	  (1<<0)
#define S3C2410_UCON_RXFIFO_TOI	  (1<<7)
#define S3C2443_UCON_RXERR_IRQEN  (1<<6)
#define S3C2443_UCON_LOOPBACK	  (1<<5)

#define S3C2410_UCON_DEFAULT	  (S3C2410_UCON_TXILEVEL  | \
				   S3C2410_UCON_RXILEVEL  | \
				   S3C2410_UCON_TXIRQMODE | \
				   S3C2410_UCON_RXIRQMODE | \
				   S3C2410_UCON_RXFIFO_TOI)

#define S3C2410_UFCON_FIFOMODE	  (1<<0)
#define S3C2410_UFCON_TXTRIG0	  (0<<6)
#define S3C2410_UFCON_RXTRIG8	  (1<<4)
#define S3C2410_UFCON_RXTRIG12	  (2<<4)

/* S3C2440 FIFO trigger levels */
#define S3C2440_UFCON_RXTRIG1	  (0<<4)
#define S3C2440_UFCON_RXTRIG8	  (1<<4)
#define S3C2440_UFCON_RXTRIG16	  (2<<4)
#define S3C2440_UFCON_RXTRIG32	  (3<<4)

#define S3C2440_UFCON_TXTRIG0	  (0<<6)
#define S3C2440_UFCON_TXTRIG16	  (1<<6)
#define S3C2440_UFCON_TXTRIG32	  (2<<6)
#define S3C2440_UFCON_TXTRIG48	  (3<<6)

#define S3C2410_UFCON_RESETBOTH	  (3<<1)
#define S3C2410_UFCON_RESETTX	  (1<<2)
#define S3C2410_UFCON_RESETRX	  (1<<1)

#define S3C2410_UFCON_DEFAULT	  (S3C2410_UFCON_FIFOMODE | \
				   S3C2410_UFCON_TXTRIG0  | \
				   S3C2410_UFCON_RXTRIG8 )

#define	S3C2410_UMCOM_AFC	  (1<<4)
#define	S3C2410_UMCOM_RTS_LOW	  (1<<0)

#define S3C2412_UMCON_AFC_63	(0<<5)		/* same as s3c2443 */
#define S3C2412_UMCON_AFC_56	(1<<5)
#define S3C2412_UMCON_AFC_48	(2<<5)
#define S3C2412_UMCON_AFC_40	(3<<5)
#define S3C2412_UMCON_AFC_32	(4<<5)
#define S3C2412_UMCON_AFC_24	(5<<5)
#define S3C2412_UMCON_AFC_16	(6<<5)
#define S3C2412_UMCON_AFC_8	(7<<5)

#define S3C2410_UFSTAT_TXFULL	  (1<<9)
#define S3C2410_UFSTAT_RXFULL	  (1<<8)
#define S3C2410_UFSTAT_TXMASK	  (15<<4)
#define S3C2410_UFSTAT_TXSHIFT	  (4)
#define S3C2410_UFSTAT_RXMASK	  (15<<0)
#define S3C2410_UFSTAT_RXSHIFT	  (0)

/* UFSTAT S3C2443 same as S3C2440 */
#define S3C2440_UFSTAT_TXFULL	  (1<<14)
#define S3C2440_UFSTAT_RXFULL	  (1<<6)
#define S3C2440_UFSTAT_TXSHIFT	  (8)
#define S3C2440_UFSTAT_RXSHIFT	  (0)
#define S3C2440_UFSTAT_TXMASK	  (63<<8)
#define S3C2440_UFSTAT_RXMASK	  (63)

#define S3C2410_UTRSTAT_TXE	  (1<<2)
#define S3C2410_UTRSTAT_TXFE	  (1<<1)
#define S3C2410_UTRSTAT_RXDR	  (1<<0)

#define S3C2410_UERSTAT_OVERRUN	  (1<<0)
#define S3C2410_UERSTAT_FRAME	  (1<<2)
#define S3C2410_UERSTAT_BREAK	  (1<<3)
#define S3C2443_UERSTAT_PARITY	  (1<<1)

#define S3C2410_UERSTAT_ANY	  (S3C2410_UERSTAT_OVERRUN | \
				   S3C2410_UERSTAT_FRAME | \
				   S3C2410_UERSTAT_BREAK)

#define S3C2410_UMSTAT_CTS	  (1<<0)
#define S3C2410_UMSTAT_DeltaCTS	  (1<<2)

#define S3C2443_DIVSLOT		  (0x2C)


#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)


/* UART */
#if 0
#define S3C_VA_UART0	(S3C24XX_VA_UART)
#define S3C_VA_UART1	(S3C24XX_VA_UART + 0x400 )
#define S3C_VA_UART2	(S3C24XX_VA_UART + 0x800 )
#define S3C_VA_UART3	(S3C24XX_VA_UART + 0xC00 )

#define S3C_PA_UART0	(S3C24XX_PA_UART)
#define S3C_PA_UART1	(S3C24XX_PA_UART + 0x400 )
#define S3C_PA_UART2	(S3C24XX_PA_UART + 0x800 )
#define S3C_PA_UART3	(S3C24XX_PA_UART + 0xC00 )
#endif

#define S3C_IRQ_UART0	IRQ_UART0
#define S3C_IRQ_UART1	IRQ_UART1
#define S3C_IRQ_UART2	IRQ_UART2
#define S3C_IRQ_UART3	IRQ_UART3

#define S3C_ULCON	  (0x00)
#define S3C_UCON	  (0x04)
#define S3C_UFCON	  (0x08)
#define S3C_UMCON	  (0x0C)
#define S3C_UTRSTAT	  (0x10)
#define S3C_UERSTAT	  (0x14)
#define S3C_UFSTAT	  (0x18)
#define S3C_UMSTAT	  (0x1C)
#define S3C_UTXH	  (0x20)
#define S3C_URXH	  (0x24)
#define S3C_UBRDIV	  (0x28)
#define S3C_UDIVSLOT (0x2C)
#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
#define S3C_UINTPND	  (0x30)
#define S3C_UINTSP	  (0x34)
#define S3C_UINTMSK	  (0x38)
#else
#define S3C_UINTPND	  (S3C24XX_VA_IRQ + 0x18 )
#define S3C_UINTMSK	  (S3C24XX_VA_IRQ + 0x1c )
#endif

/* base definitions for UART Line Control Register */
#define S3C_LCON_CFGMASK	(0x7f)

#define S3C_LCON_CS5	(0x0)
#define S3C_LCON_CS6	(0x1)
#define S3C_LCON_CS7	(0x2)
#define S3C_LCON_CS8	(0x3)
#define S3C_LCON_CSMASK	(0x3)

#define S3C_LCON_PNONE	(0x0)
#define S3C_LCON_PEVEN	(0x5 << 3)
#define S3C_LCON_PODD	(0x4 << 3)
#define S3C_LCON_PMASK	(0x7 << 3)

#define S3C_LCON_STOPB	(1<<2)
#define S3C_LCON_IRM	    (1<<6)

#define S3C_UCON_CLKMASK	(3<<10)
#define S3C_UCON_PCLK	(0<<10)
#define S3C_UCON_UCLK	(1<<10)
#define S3C_UCON_PCLK2	(2<<10)
#define S3C_UCON_FCLK	(3<<10)
#define S3C_UCON_SBREAK	(1<<4)

#define S3C_UCON_TXILEVEL	(1<<9)
#define S3C_UCON_RXILEVEL	(1<<8)
#define S3C_UCON_TXIRQMODE	(1<<2)
#define S3C_UCON_RXIRQMODE	(1<<0)
#define S3C_UCON_RXFIFO_TOI	(1<<7)
#define S3C_UCON_RX_ESIE	(1<<6)
#define S3C_UCON_LOOP_OPERATION (0<<5)
#define S3C_UCON_NO_SBS 	(0<<4)


#define S3C_UCON_DEFAULT	  (S3C_UCON_TXILEVEL  | \
				   S3C_UCON_RXILEVEL  | \
				   S3C_UCON_TXIRQMODE | \
				   S3C_UCON_RXIRQMODE | \
				   S3C_UCON_RXFIFO_TOI)

/* base definitions for UART FIFO Control Register */
#define S3C_UFCON_FIFOMODE	  (1<<0)
#define S3C2410_UFCON_TXTRIG0	  (0<<6)
#define S3C2410_UFCON_RXTRIG8	  (1<<4)
#define S3C_UFCON_RXTRIG12	  (2<<4)

/* S3C2413 FIFO trigger levels */
#define S3C_UFCON_RXTRIG1	  (0<<4)
#define S3C_UFCON_RXTRIG8	  (1<<4)
#define S3C_UFCON_RXTRIG16	  (2<<4)
#define S3C_UFCON_RXTRIG32	  (3<<4)

#define S3C_UFCON_TXTRIG0	  (0<<6)
#define S3C_UFCON_TXTRIG16	  (1<<6)
#define S3C_UFCON_TXTRIG32	  (2<<6)
#define S3C_UFCON_TXTRIG48	  (3<<6)

#define S3C_UFCON_RESETBOTH	  (3<<1)
#define S3C_UFCON_RESETTX	  (1<<2)
#define S3C_UFCON_RESETRX	  (1<<1)
#define S3C_UFCON_FIFO_ENABLE	  (1<<0)

#define S3C_UFCON_DEFAULT	  (S3C_UFCON_FIFOMODE | \
				   S3C_UFCON_TXTRIG0  | \
				   S3C_UFCON_RXTRIG8 )

#define	S3C_UMCOM_AFC	  (1<<4)
#define	S3C_UMCOM_RTS_LOW	  (1<<0)

#define S3C2410_UFSTAT_TXFULL	  (1<<9)
#define S3C2410_UFSTAT_RXFULL	  (1<<8)
#define S3C2410_UFSTAT_TXMASK	  (15<<4)
#define S3C2410_UFSTAT_TXSHIFT	  (4)
#define S3C2410_UFSTAT_RXMASK	  (15<<0)
#define S3C2410_UFSTAT_RXSHIFT	  (0)

#define S3C_UFSTAT_TXFULL	  (1<<14)
#define S3C_UFSTAT_RXFULL	  (1<<6)
#define S3C_UFSTAT_TXSHIFT	  (8)
#define S3C_UFSTAT_RXSHIFT	  (0)
#define S3C_UFSTAT_TXMASK	  (63<<8)
#define S3C_UFSTAT_RXMASK	  (63)

#define S3C_UTRSTAT_TXE	  (1<<2)
#define S3C_UTRSTAT_TXFE	  (1<<1)
#define S3C_UTRSTAT_RXDR	  (1<<0)

#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
#define UART_RX_INT		(1<<0)
#define UART_TX_INT		(1<<2)
#define UART_ERR_INT		(1<<1)
#define UART_MODEM_INT		(1<<3)
#else
#define UART_RX_INT		(1<<6)
#define UART_TX_INT		(1<<7)
#define UART_ERR_INT		(1<<8)
#endif

#define S3C_UERSTAT_OVERRUN	  (1<<0)
#define S3C_UERSTAT_FRAME	  (1<<2)
#define S3C_UERSTAT_BREAK	  (1<<3)
#define S3C_UERSTAT_ANY	  (S3C_UERSTAT_OVERRUN | \
				   S3C_UERSTAT_FRAME | \
				   S3C_UERSTAT_BREAK)

#define S3C_UMSTAT_CTS	  (1<<0)
#define S3C_UMSTAT_DeltaCTS	  (1<<2)

#endif


#ifndef __ASSEMBLY__

/* struct s3c24xx_uart_clksrc
 *
 * this structure defines a named clock source that can be used for the
 * uart, so that the best clock can be selected for the requested baud
 * rate.
 *
 * min_baud and max_baud define the range of baud-rates this clock is
 * acceptable for, if they are both zero, it is assumed any baud rate that
 * can be generated from this clock will be used.
 *
 * divisor gives the divisor from the clock to the one seen by the uart
*/
#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410)
#define NR_PORTS 	(4)
#define UART_FIFO_SIZE	64
#define UART_HAS_INTMSK
#define UART_C_CFLAG
#define UART_UMCON
#define UART_CLK	115200
#elif defined(CONFIG_CPU_S3C2416)
#define NR_PORTS 	(4)
#else
#define NR_PORTS 	(3)
#endif


struct s3c24xx_uart_clksrc {
	const char	*name;
#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410)
	unsigned int	 clock_src;
#endif
	unsigned int	 divisor;
	unsigned int	 min_baud;
	unsigned int	 max_baud;
};

/* configuration structure for per-machine configurations for the
 * serial port
 *
 * the pointer is setup by the machine specific initialisation from the
 * arch/arm/mach-s3c2410/ directory.
*/

struct s3c2410_uartcfg {
	unsigned char	   hwport;	 /* hardware port number */
	unsigned char	   unused;
	unsigned short	   flags;
#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410)
	unsigned int	   uart_flags;	 /* default uart flags */
#else
	unsigned long	uart_flags;	 /* default uart flags */
#endif
	unsigned long	   ucon;	 /* value of ucon for port */
	unsigned long	   ulcon;	 /* value of ulcon for port */
	unsigned long	   ufcon;	 /* value of ufcon for port */

	struct s3c24xx_uart_clksrc *clocks;
	unsigned int		    clocks_size;
};

/* s3c24xx_uart_devs
 *
 * this is exported from the core as we cannot use driver_register(),
 * or platform_add_device() before the console_initcall()
*/

extern struct platform_device *s3c24xx_uart_devs[NR_PORTS];

#endif /* __ASSEMBLY__ */

#endif /* __ASM_ARM_REGS_SERIAL_H */

