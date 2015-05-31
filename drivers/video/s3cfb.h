#ifndef __S3CFB_H
#define __S3CFB_H
/*
 * linux/drivers/video/s3cfb.h
 *
 * $Id: s3cfb.h,v 1.13 2008/05/26 07:58:22 jsgood Exp $
 *
 * Copyright (c)2005 rahul tanwar <rahul.tanwar@samsung.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    S3C LCD Controller Frame Buffer Driver
 *	    based on skeletonfb.c, sa1100fb.h
 *
 */


#define BIT0                            0x00000001
#define BIT1                            0x00000002
#define BIT2                            0x00000004
#define BIT3                            0x00000008
#define BIT4                            0x00000010
#define BIT5                            0x00000020
#define BIT6                            0x00000040
#define BIT7                            0x00000080
#define BIT8                            0x00000100
#define BIT9                            0x00000200
#define BIT10                           0x00000400
#define BIT11                           0x00000800
#define BIT12                           0x00001000
#define BIT13                           0x00002000
#define BIT14                           0x00004000
#define BIT15                           0x00008000
#define BIT16                           0x00010000
#define BIT17                           0x00020000
#define BIT18                           0x00040000
#define BIT19                           0x00080000
#define BIT20                           0x00100000
#define BIT21                           0x00200000
#define BIT22                           0x00400000
#define BIT23                           0x00800000
#define BIT24                           0x01000000
#define BIT25                           0x02000000
#define BIT26                           0x04000000
#define BIT27                           0x08000000
#define BIT28                           0x10000000
#define BIT29                           0x20000000
#define BIT30                           0x40000000
#define BIT31                           0x80000000


#define MIN_XRES	64
#define MIN_YRES	64

struct s3c_fb_rgb {
    struct fb_bitfield		red;
    struct fb_bitfield		green;
    struct fb_bitfield		blue;
    struct fb_bitfield		transp;
};

const static struct s3c_fb_rgb s3c_fb_rgb_8 = {
      .red = {.offset = 0, .length = 8,},
      .green = {.offset = 0, .length = 8,},
      .blue = {.offset = 0, .length = 8,},
      .transp = {.offset = 0, .length = 0,},
};

const static struct s3c_fb_rgb s3c_fb_rgb_16 = {
      .red = {.offset = 11, .length = 5,},
      .green = {.offset = 5, .length = 6,},
      .blue = {.offset = 0, .length = 5,},
      .transp = {.offset = 0, .length = 0,},
};

const static struct s3c_fb_rgb s3c_fb_rgb_24 = {
      .red = {.offset = 16, .length = 8,},
      .green = {.offset = 8, .length = 8,},
      .blue = {.offset = 0, .length = 8,},
      .transp = {.offset = 0, .length = 0,},
};

const static struct s3c_fb_rgb s3c_fb_rgb_32 = {
      .red={.offset=16, .length=8,},
      .green={.offset=8, .length=8,},
      .blue={.offset=0, .length=8,},
      .transp={.offset= 24, .length=8,},
};

struct s3c_fb_info {
	struct fb_info		fb;
	struct device		*dev;

// YREOM 2007 add for supporting many OSD
	u_int			win_id;

	u_int			max_bpp;
	u_int			max_xres;
	u_int			max_yres;

	/* raw memory addresses */
	dma_addr_t		map_dma_f1;	/* physical */
	u_char *		map_cpu_f1;	/* virtual */
	u_int			map_size_f1;

	/* addresses of pieces placed in raw buffer */
	u_char *		screen_cpu_f1;	/* virtual address of frame buffer */
	dma_addr_t		screen_dma_f1;	/* physical address of frame buffer */

	/* raw memory addresses */
	dma_addr_t		map_dma_f2;	/* physical */
	u_char *		map_cpu_f2;	/* virtual */
	u_int			map_size_f2;

	/* addresses of pieces placed in raw buffer */
	u_char *		screen_cpu_f2;	/* virtual address of frame buffer */
	dma_addr_t		screen_dma_f2;	/* physical address of frame buffer */

	unsigned int		palette_ready;
	unsigned int		fb_change_ready;

	/* keep these registers in case we need to re-write palette */
	u32			palette_buffer[256];
	u32			pseudo_pal[16];
};


#define PALETTE_BUFF_CLEAR (0x80000000)	/* entry is clear/invalid */

int s3c_fb_init(void);

#if 0
#define dprintk(msg...)	printk("s3c_fb: " msg)
#else
#define dprintk(msg...) while (0) { }
#endif

struct s3c_fb_mach_info {

	/* Screen size */
	int		width;
	int		height;

	/* Screen info */
	int		xres;
	int		yres;

	/* Virtual Screen info */
	int		xres_virtual;
	int		yres_virtual;
	int		xoffset;
	int		yoffset;

	/* OSD Screen size */
	int		osd_width;
	int		osd_height;

	/* OSD Screen info */
	int		osd_xres;
	int		osd_yres;

	/* OSD Screen info */
	int		osd_xres_virtual;
	int		osd_yres_virtual;

	int		bpp;
	int		bytes_per_pixel;
	unsigned long	pixclock;

	int hsync_len;
	int left_margin;
	int right_margin;
	int vsync_len;
	int upper_margin;
	int lower_margin;
	int sync;
	int cmap_grayscale:1, cmap_inverse:1, cmap_static:1, unused:29;

	/* lcd configuration registers */
	unsigned long lcdcon1;
	unsigned long lcdcon2;

        unsigned long	lcdcon3;
	unsigned long	lcdcon4;
	unsigned long	lcdcon5;

	/* GPIOs */
	unsigned long	gpcup;
	unsigned long	gpcup_mask;
	unsigned long	gpccon;
	unsigned long	gpccon_mask;
	unsigned long	gpdup;
	unsigned long	gpdup_mask;
	unsigned long	gpdcon;
	unsigned long	gpdcon_mask;

	/* lpc3600 control register */
	unsigned long	lpcsel;
	unsigned long lcdtcon1;
	unsigned long lcdtcon2;
	unsigned long lcdtcon3;
	unsigned long lcdosd1;
	unsigned long lcdosd2;
	unsigned long lcdosd3;
	unsigned long lcdsaddrb1;
	unsigned long lcdsaddrb2;
	unsigned long lcdsaddrf1;
	unsigned long lcdsaddrf2;
	unsigned long lcdeaddrb1;
	unsigned long lcdeaddrb2;
	unsigned long lcdeaddrf1;
	unsigned long lcdeaddrf2;
	unsigned long lcdvscrb1;
	unsigned long lcdvscrb2;
	unsigned long lcdvscrf1;
	unsigned long lcdvscrf2;
	unsigned long lcdintcon;
	unsigned long lcdkeycon;
	unsigned long lcdkeyval;
	unsigned long lcdbgcon;
	unsigned long lcdfgcon;
	unsigned long lcddithcon;


	/* For s3c2443, s3c6400 */
	unsigned long vidcon0;
	unsigned long vidcon1;
	unsigned long vidtcon0;
	unsigned long vidtcon1;
	unsigned long vidtcon2;
	unsigned long vidtcon3;
	unsigned long wincon0;
	unsigned long wincon2;
	unsigned long wincon1;
	unsigned long wincon3;
	unsigned long wincon4;

	unsigned long vidosd0a;
	unsigned long vidosd0b;
	unsigned long vidosd0c;
	unsigned long vidosd1a;
	unsigned long vidosd1b;
	unsigned long vidosd1c;
	unsigned long vidosd1d;
	unsigned long vidosd2a;
	unsigned long vidosd2b;
	unsigned long vidosd2c;
	unsigned long vidosd2d;
	unsigned long vidosd3a;
	unsigned long vidosd3b;
	unsigned long vidosd3c;
	unsigned long vidosd4a;
	unsigned long vidosd4b;
	unsigned long vidosd4c;

	unsigned long vidw00add0b0;
	unsigned long vidw00add0b1;
	unsigned long vidw01add0;
	unsigned long vidw01add0b0;
	unsigned long vidw01add0b1;

	unsigned long vidw00add1b0;
	unsigned long vidw00add1b1;
	unsigned long vidw01add1;
	unsigned long vidw01add1b0;
	unsigned long vidw01add1b1;

	unsigned long vidw00add2b0;
	unsigned long vidw00add2b1;

	unsigned long vidw02add0;
	unsigned long vidw03add0;
	unsigned long vidw04add0;

	unsigned long vidw02add1;
	unsigned long vidw03add1;
	unsigned long vidw04add1;
	unsigned long vidw00add2;
	unsigned long vidw01add2;
	unsigned long vidw02add2;
	unsigned long vidw03add2;
	unsigned long vidw04add2;

	unsigned long vidintcon;
	unsigned long vidintcon0;
	unsigned long vidintcon1;
	unsigned long w1keycon0;
	unsigned long w1keycon1;
	unsigned long w2keycon0;
	unsigned long w2keycon1;
	unsigned long w3keycon0;
	unsigned long w3keycon1;
	unsigned long w4keycon0;
	unsigned long w4keycon1;

	unsigned long win0map;
	unsigned long win1map;
	unsigned long win2map;
	unsigned long win3map;
	unsigned long win4map;

	unsigned long wpalcon;
	unsigned long dithmode;
	unsigned long intclr0;
	unsigned long intclr1;
	unsigned long intclr2;

	unsigned long win0pal;
	unsigned long win1pal;

       /* backlight info */
	int		backlight_min;
	int		backlight_max;
	int		backlight_default;

	/* Utility fonctions */
	void		(*backlight_power)(int);
	void		(*lcd_power)(int);
	void		(*set_brightness)(int);

};


typedef struct {
	int Bpp;
	int LeftTop_x;
	int LeftTop_y;
	int Width;
	int Height;
}  s3c_win_info_t;


typedef struct {
	int width;
	int height;
	int bpp;
	int offset;
	int v_width;
	int v_height;
}  vs_info_t;

typedef struct {
	int direction;
	unsigned int compkey_red;
	unsigned int compkey_green;
	unsigned int compkey_blue;
}  s3c_color_key_info_t;

typedef struct {
	unsigned int colval_red;
	unsigned int colval_green;
	unsigned int colval_blue;
}  s3c_color_val_info_t;

#ifndef MHZ
#define MHZ (1000*1000)
#endif

#define print_mhz(m) ((m) / MHZ), ((m / 1000) % 1000)

extern int  soft_cursor(struct fb_info *info, struct fb_cursor *cursor);
extern  struct s3c_fb_mach_info mach_info;

extern void set_brightness(int);
extern int s3c_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg);

extern int __init s3c_fb_map_video_memory(struct s3c_fb_info *fbi);
extern void s3c_fb_unmap_video_memory(struct s3c_fb_info *fbi);
extern void Init_LDI(void);
extern int s3c_fb_init_registers(struct s3c_fb_info *fbi);
extern void s3c_fb_set_lcdaddr(struct s3c_fb_info *fbi);
extern void s3c_fb_activate_var(struct s3c_fb_info *fbi, struct fb_var_screeninfo *var);
extern void s3c_fb_set_fb_change(int req_fb);


extern int s3c_fb_init_win (struct s3c_fb_info *fbi, int Bpp, int LeftTop_x, int LeftTop_y, int Width, int Height, int OnOff);
extern int s3c_fb_win_onoff(struct s3c_fb_info *fbi, int On);
extern int s3c_fb_set_alpha_level(struct s3c_fb_info *fbi);
extern int s3c_fb_set_alpha_mode(struct s3c_fb_info *fbi, int Alpha_mode, int Alpha_level);
extern int s3c_fb_set_position_win(struct s3c_fb_info *fbi, int LeftTop_x, int LeftTop_y, int Width, int Height);
extern int s3c_fb_set_size_win(struct s3c_fb_info *fbi, int Width, int Height);
extern int s3c_fb_set_bpp(struct s3c_fb_info *fbi, int Bpp);
extern int s3c_fb_set_memory_size_win(struct s3c_fb_info *fbi);
extern int s3c_fb_set_out_path(struct s3c_fb_info *fbi, int Path);

/* For color key support */
#define COLOR_KEY_DIR_BG_DISPLAY 0
#define COLOR_KEY_DIR_FG_DISPLAY 1

int s3c_fb_color_key_alpha_onoff(struct s3c_fb_info *fbi, int On);
int s3c_fb_color_key_onoff(struct s3c_fb_info *fbi, int On);
int s3c_fb_setup_color_key_register(struct s3c_fb_info *fbi, s3c_color_key_info_t colkey_info);
int s3c_fb_set_color_value(struct s3c_fb_info *fbi, s3c_color_val_info_t colval_info);


#if defined(CONFIG_CPU_S3C2412)
#define S3C_FB_MAX_NUM 1
#elif defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)
#define S3C_FB_MAX_NUM 2
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
#define S3C_FB_MAX_NUM 5
#else
#define S3C_FB_MAX_NUM 1
#endif

// (WAITFORVSYNC)
#define FBIO_WAITFORVSYNC	_IOW('F', 0x20, u_int32_t)
typedef struct
{
  wait_queue_head_t waitQueue;
  int               count;
} s3cfb_vSyncInfoType;

extern int s3cfb_wait_for_sync(u_int32_t crtc);
//(WAITFORVSYNC)


#define FB_MAX_NUM(x,y)		((x)>(y) ? (y) : (x))
#define S3C_FB_NUM		FB_MAX_NUM(S3C_FB_MAX_NUM, CONFIG_FB_NUM)

struct s3c_fb_dma_info
{
	dma_addr_t map_dma_f1;
	dma_addr_t map_dma_f2;
};

#endif
