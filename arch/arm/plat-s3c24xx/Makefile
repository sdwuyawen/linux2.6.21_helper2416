# arch/arm/plat-s3c24xx/Makefile
#
# Copyright 2007 Simtec Electronics
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=


# Core files

obj-y				+= cpu.o
obj-$(CONFIG_PLAT_S3C24XX)	+= irq.o
obj-$(CONFIG_PLAT_S3C64XX)	+= irq-pl192.o
obj-y				+= devs.o
obj-y				+= gpio.o
obj-$(CONFIG_PLAT_S3C24XX)	+= time.o
obj-$(CONFIG_PLAT_S3C64XX)	+= time-s3c64xx.o
obj-y				+= clock.o

# Architecture dependant builds

obj-$(CONFIG_CPU_S3C244X)	+= s3c244x.o
obj-$(CONFIG_CPU_S3C244X)	+= s3c244x-irq.o
obj-$(CONFIG_PM_SIMTEC)		+= pm-simtec.o
obj-$(CONFIG_PM)		+= pm.o
obj-$(CONFIG_PM)		+= sleep.o
obj-$(CONFIG_S3C2410_DMA)	+= dma.o
obj-$(CONFIG_S3C64XX_DMA)	+= dma-pl080.o
obj-$(CONFIG_MACH_SMDK)		+= common-smdk.o
