#
# Makefile for kernel SPI drivers.
#

ifeq ($(CONFIG_SPI_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

# small core, mostly translating board-specific
# config declarations into driver model code
obj-$(CONFIG_SPI_MASTER)		+= spi.o

# SPI master controller drivers (bus)
obj-$(CONFIG_SPI_BITBANG)		+= spi_bitbang.o
obj-$(CONFIG_SPI_ATMEL)			+= atmel_spi.o
obj-$(CONFIG_SPI_BUTTERFLY)		+= spi_butterfly.o
obj-$(CONFIG_SPI_IMX)			+= spi_imx.o
obj-$(CONFIG_SPI_PXA2XX)		+= pxa2xx_spi.o
obj-$(CONFIG_SPI_OMAP_UWIRE)		+= omap_uwire.o
obj-$(CONFIG_SPI_MPC83xx)		+= spi_mpc83xx.o
obj-$(CONFIG_SPI_S3C24XX_GPIO)		+= spi_s3c24xx_gpio.o
obj-$(CONFIG_SPI_S3C24XX)		+= spi_s3c24xx.o
# 	... add above this line ...

# SPI protocol drivers (device/link on bus)
obj-$(CONFIG_SPI_AT25)		+= at25.o
# 	... add above this line ...

# SPI slave controller drivers (upstream link)
# 	... add above this line ...

# SPI slave drivers (protocol for that link)
# 	... add above this line ...

ifeq ($(CONFIG_MACH_SMDK2443),y)
obj-$(CONFIG_SPI)			+= spi-dev.o hspi-s3c2443.o
endif

ifeq ($(CONFIG_MACH_SMDK2450),y)
obj-$(CONFIG_SPI)                       += spi-dev.o hspi-s3c2443.o
endif

ifeq ($(CONFIG_MACH_SMDK2416),y)
obj-$(CONFIG_SPI)                       += spi-dev.o hspi-s3c2443.o
endif

ifeq ($(CONFIG_MACH_SMDK6400),y)
obj-$(CONFIG_SPI)			+= spi-dev.o hspi-s3c6400.o
endif

ifeq ($(CONFIG_MACH_SMDK6410),y)
obj-$(CONFIG_SPI)			+= spi-dev.o hspi-s3c6400.o
endif

