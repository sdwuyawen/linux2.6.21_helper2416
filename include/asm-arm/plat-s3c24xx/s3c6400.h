/* arch/arm/mach-s3c24xx/s3c6400.h
 *
 * Copyright (c) 2004-2005 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * Header file for s3c6400 cpu support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modifications:
 *	24-Aug-2004 BJD  Start of S3C2412 CPU support
 *	04-Nov-2004 BJD  Added s3c6400_init_uarts()
 *	04-Jan-2005 BJD  Moved uart init to cpu code
 *	10-Jan-2005 BJD  Moved 2412 specific init here
 *	14-Jan-2005 BJD  Split the clock initialisation code
*/

#if defined (CONFIG_CPU_S3C6400) || defined (CONFIG_CPU_S3C6410) 

extern  int s3c6400_init(void);

extern void s3c6400_map_io(struct map_desc *mach_desc, int size);

extern void s3c6400_init_uarts(struct s3c2410_uartcfg *cfg, int no);

extern void s3c6400_init_clocks(int xtal);

#else
#define s3c6400_init NULL
#define s3c6400_map_io NULL
#define s3c6400_init_uarts NULL
#define s3c6400_init_clocks NULL
#endif

