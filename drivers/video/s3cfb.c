/*
 * linux/drivers/video/s3cfb.c
 *
 * $Id: s3cfb.c,v 1.23 2008/07/08 01:00:44 jsgood Exp $
 *
 * Revision 1.16  2006/09/14 04:45:15  ihlee215
 * OSD support added
 *
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    S3C LCD Controller Frame Buffer Driver
 *	    based on skeletonfb.c, sa1100fb.c
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

//#include <asm/mach/map.h>
#include <asm/arch/regs-lcd.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/idle.h>

#include "s3cfb.h"

#ifdef CONFIG_PM
#include <linux/pm.h>
#include <asm/plat-s3c24xx/pm.h>
#endif

#define DEFAULT_BACKLIGHT_LEVEL		2

struct s3c_fb_info info[CONFIG_FB_NUM];

s3cfb_vSyncInfoType s3cfb_vSyncInfo; 

/* backlight and power control functions */
static int backlight_level = DEFAULT_BACKLIGHT_LEVEL;
static int backlight_power = 1;
static int lcd_power	   = 1;


static  void s3c_fb_lcd_power(int to)
{
	lcd_power = to;

	if (mach_info.lcd_power)
		(mach_info.lcd_power)(to);
}

static inline void s3c_fb_backlight_power(int to)
{
	backlight_power = to;

	if (mach_info.backlight_power)
		(mach_info.backlight_power)(to);
}

 void s3c_fb_backlight_level(int to)
{
	backlight_level = to;

	if (mach_info.set_brightness)
		(mach_info.set_brightness)(to);
}

/*
 *	s3c_fb_check_var():
 *	Get the video params out of 'var'. If a value doesn't fit, round it up,
 *	if it's too big, return -EINVAL.
 *
 */
static int s3c_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	//struct s3c_fb_info *fbi = (struct s3c_fb_info *)info;

	dprintk("check_var(var=%p, info=%p)\n", var, info);

	switch (var->bits_per_pixel) {
		case 8:
			var->red	= s3c_fb_rgb_8.red;
			var->green	= s3c_fb_rgb_8.green;
			var->blue	= s3c_fb_rgb_8.blue;
			var->transp  	= s3c_fb_rgb_8.transp;
			break;

		case 16:
			var->red	= s3c_fb_rgb_16.red;
			var->green	= s3c_fb_rgb_16.green;
			var->blue	= s3c_fb_rgb_16.blue;
			var->transp  	= s3c_fb_rgb_16.transp;
			break;

		case 24:
			var->red	= s3c_fb_rgb_24.red;
			var->green	= s3c_fb_rgb_24.green;
			var->blue	= s3c_fb_rgb_24.blue;
			var->transp  	= s3c_fb_rgb_24.transp;
			break;

		case 32:
			var->red	= s3c_fb_rgb_32.red;
			var->green   	= s3c_fb_rgb_32.green;
			var->blue     	= s3c_fb_rgb_32.blue;
			var->transp  	= s3c_fb_rgb_32.transp;
			break;
	}

	return 0;
}


static inline void s3c_modify_gpio(void __iomem *reg,
			       unsigned long set, unsigned long mask)
{
	unsigned long tmp;

	tmp = __raw_readl(reg) & ~mask;
	__raw_writel(tmp | set, reg);
}


/*
 *      s3c_fb_set_par - Optional function. Alters the hardware state.
 *      @info: frame buffer structure that represents a single frame buffer
 *
 */
static int s3c_fb_set_par(struct fb_info *info)
{
	struct s3c_fb_info *fbi = (struct s3c_fb_info *)info;
	struct fb_var_screeninfo *var = &info->var;

        if (var->bits_per_pixel == 16)
		fbi->fb.fix.visual = FB_VISUAL_TRUECOLOR;
	else if (var->bits_per_pixel == 32)
		fbi->fb.fix.visual = FB_VISUAL_TRUECOLOR;
	else
                fbi->fb.fix.visual = FB_VISUAL_PSEUDOCOLOR;

	fbi->fb.fix.line_length     = var->width * mach_info.bytes_per_pixel;

	/* activate this new configuration */
	s3c_fb_activate_var(fbi, var);

	return 0;
}

static int palette_win;

static void schedule_palette_update(struct s3c_fb_info *fbi,
				    unsigned int regno, unsigned int val)
{
	unsigned long flags;

	local_irq_save(flags);

	fbi->palette_buffer[regno] = val;


	if (!fbi->palette_ready) {
		fbi->palette_ready = 1;
		palette_win=fbi->win_id;
	}

	local_irq_restore(flags);
}


static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;
	return chan << bf.offset;
}


static int s3c_fb_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	struct s3c_fb_info *fbi = (struct s3c_fb_info *)info;
	unsigned int val;

	switch (fbi->fb.fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/* true-colour, use pseuo-palette */
		if (regno < 16) {
			u32 *pal = fbi->fb.pseudo_palette;

			val  = chan_to_field(red,   fbi->fb.var.red);
			val |= chan_to_field(green, fbi->fb.var.green);
			val |= chan_to_field(blue,  fbi->fb.var.blue);

			pal[regno] = val;
		}
		break;

	case FB_VISUAL_PSEUDOCOLOR:
		if (regno < 256) {
#if defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)
			/* currently assume RGB 8-8-8 mode -- for SONY */
			val  = ((red   <<  8) & 0xff0000);
			val |= ((green >>  0) & 0xff00);
			val |= ((blue  >> 8) & 0xff);
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
			/* currently assume RGB 5-6-5 mode */
			val  = ((red   >>  0) & 0xf800);
			val |= ((green >>  5) & 0x07e0);
			val |= ((blue  >> 11) & 0x001f);
#endif

			dprintk("index = %d, val = 0x%8X\n", regno, val);
			schedule_palette_update(fbi, regno, val);
		}

		break;

	default:
		return 1;   /* unknown type */
	}

	return 0;
}


#if defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) || defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
static void s3c_fb_write_palette(struct s3c_fb_info *fbi)
{
	unsigned int i;
	unsigned long ent;
#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	unsigned int win_num=fbi->win_id;
#endif

	fbi->palette_ready = 0;

	//__raw_writel((mach_info.wincon0 | S3C_WINCONx_ENWIN_F_DISABLE), S3C_WINCON0);
	__raw_writel((mach_info.wpalcon | S3C_WPALCON_PALUPDATEEN), S3C_WPALCON);

	for (i = 0; i < 256; i++) {
		if ((ent = fbi->palette_buffer[i]) == PALETTE_BUFF_CLEAR)
			continue;

#if defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)
		__raw_writel(ent, S3C_TFTPAL0(i));
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
		__raw_writel(ent, S3C_TFTPAL0(i)+0x400*win_num);
#endif

		/* it seems the only way to know exactly
		 * if the palette wrote ok, is to check
		 * to see if the value verifies ok
		 */

#if defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)
		if (__raw_readl(S3C_TFTPAL0(i)) == ent) {
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
		if (__raw_readl(S3C_TFTPAL0(i)+0x400*win_num) == ent) {
#endif			
			fbi->palette_buffer[i] = PALETTE_BUFF_CLEAR;
		}
		else {

			fbi->palette_ready = 1;   /* retry */
			printk("Retry writing into the palette\n");
		}

	}

	__raw_writel(mach_info.wpalcon, S3C_WPALCON);
	__raw_writel((mach_info.wincon0 | S3C_WINCONx_ENWIN_F_ENABLE), S3C_WINCON0);
}
#endif


/**
 *	s3c_fb_pan_display
 *	@var: frame buffer variable screen structure
 *	@info: frame buffer structure that represents a single frame buffer
 *
 *	Pan (or wrap, depending on the `vmode' field) the display using the
 *	`xoffset' and `yoffset' fields of the `var' structure.
 *	If the values don't fit, return -EINVAL.
 *
 *	Returns negative errno on error, or zero on success.
 */
static int s3c_fb_pan_display(struct fb_var_screeninfo *var,
			     struct fb_info *info)
{
	struct s3c_fb_info *fbi = (struct s3c_fb_info *)info;
	
	dprintk("s3c_fb_pan_display(var=%p, info=%p)\n", var, info);

	if (var->xoffset != 0)	/* not yet ... */
		return -EINVAL;

	if (var->yoffset + info->var.yres > info->var.yres_virtual)
		return -EINVAL;
	
	fbi->fb.var.xoffset = var->xoffset;
	fbi->fb.var.yoffset = var->yoffset;

	s3c_fb_set_lcdaddr(fbi);

	return 0;
}


/**
 *      s3c_fb_blank
 *	@blank_mode: the blank mode we want.
 *	@info: frame buffer structure that represents a single frame buffer
 *
 *	Blank the screen if blank_mode != 0, else unblank. Return 0 if
 *	blanking succeeded, != 0 if un-/blanking failed due to e.g. a
 *	video mode which doesn't support it. Implements VESA suspend
 *	and powerdown modes on hardware that supports disabling hsync/vsync:
 *	blank_mode == 2: suspend vsync
 *	blank_mode == 3: suspend hsync
 *	blank_mode == 4: powerdown
 *
 *	Returns negative errno on error, or zero on success.
 *
 */
static int s3c_fb_blank(int blank_mode, struct fb_info *info)
{
	dprintk("blank(mode=%d, info=%p)\n", blank_mode, info);

	switch (blank_mode) {
	case VESA_NO_BLANKING:	/* lcd on, backlight on */
		s3c_fb_lcd_power(1);
		s3c_fb_backlight_power(1);
		break;

	case VESA_VSYNC_SUSPEND: /* lcd on, backlight off */
	case VESA_HSYNC_SUSPEND:
		s3c_fb_lcd_power(1);
		s3c_fb_backlight_power(0);
		break;

	case VESA_POWERDOWN: /* lcd and backlight off */
		s3c_fb_lcd_power(0);
		s3c_fb_backlight_power(0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


/* sysfs export of baclight control */
static int s3c_fb_lcd_power_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", lcd_power);
}


static int s3c_fb_lcd_power_store(struct device *dev, struct device_attribute *attr,
							   const char *buf, size_t len)
{

	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 ||
	    strnicmp(buf, "1", 1) == 0) {
		s3c_fb_lcd_power(1);
	} else if (strnicmp(buf, "off", 3) == 0 ||
		   strnicmp(buf, "0", 1) == 0) {
		s3c_fb_lcd_power(0);
	} else {
		return -EINVAL;
	}

	return len;
}

static int s3c_fb_backlight_level_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", backlight_level);
}
static int s3c_fb_backlight_power_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", backlight_power);
}
static int s3c_fb_backlight_level_store(struct device *dev, struct device_attribute *attr,
					   const char *buf, size_t len)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	if (value < mach_info.backlight_min ||
	    value > mach_info.backlight_max)
		return -ERANGE;

	s3c_fb_backlight_level(value);
	return len;
}
static int s3c_fb_backlight_power_store(struct device *dev, struct device_attribute *attr,
					   const char *buf, size_t len)
{
	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 ||
	    strnicmp(buf, "1", 1) == 0) {
		s3c_fb_backlight_power(1);
	} else if (strnicmp(buf, "off", 3) == 0 ||
		   strnicmp(buf, "0", 1) == 0) {
		s3c_fb_backlight_power(0);
	} else {
		return -EINVAL;
	}

	return len;
}


static DEVICE_ATTR(lcd_power, 0644,
		   s3c_fb_lcd_power_show,
		   s3c_fb_lcd_power_store);

static DEVICE_ATTR(backlight_level, 0644,
		   s3c_fb_backlight_level_show,
		   s3c_fb_backlight_level_store);

static DEVICE_ATTR(backlight_power, 0644,
		   s3c_fb_backlight_power_show,
		   s3c_fb_backlight_power_store);



struct fb_ops s3c_fb_ops = {
	.owner		= THIS_MODULE,
	.fb_check_var	= s3c_fb_check_var,
	.fb_set_par	= s3c_fb_set_par,
	.fb_blank	= s3c_fb_blank,
	.fb_pan_display	= s3c_fb_pan_display,
	.fb_setcolreg	= s3c_fb_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
	.fb_cursor	= soft_cursor,
	.fb_ioctl	= s3c_fb_ioctl,
};

/* Fake monspecs to fill in fbinfo structure */
/* Don't know if the values are important    */
struct fb_monspecs monspecs __initdata = {
	.hfmin	= 30000,
	.hfmax	= 70000,
	.vfmin	= 50,
	.vfmax	= 65,
};

static struct clk      *lcd_clock;


void __init s3c_fb_init_fbinfo(struct s3c_fb_info *finfo, char *drv_name, int index) 
{
	int i = 0;

	if(index==0) Init_LDI();
	strcpy(finfo->fb.fix.id, drv_name);

	finfo->win_id = index;
	finfo->fb.fix.type	    = FB_TYPE_PACKED_PIXELS;
	finfo->fb.fix.type_aux	    = 0;
	finfo->fb.fix.xpanstep	    = 0;
	finfo->fb.fix.ypanstep	    = 1;
	finfo->fb.fix.ywrapstep	    = 0;
	finfo->fb.fix.accel	    = FB_ACCEL_NONE;

	finfo->fb.fbops		    = &s3c_fb_ops;
	finfo->fb.flags		    = FBINFO_FLAG_DEFAULT;
	finfo->fb.monspecs	    = monspecs;
	finfo->fb.pseudo_palette      = &finfo->pseudo_pal;

	finfo->fb.var.nonstd	    = 0;
	finfo->fb.var.activate	    = FB_ACTIVATE_NOW;
	finfo->fb.var.accel_flags     = 0;
	finfo->fb.var.vmode	    = FB_VMODE_NONINTERLACED;

	finfo->fb.var.xoffset 	      = mach_info.xoffset;
	finfo->fb.var.yoffset 	      = mach_info.yoffset;

	if(index==0){
		finfo->fb.var.height	    = mach_info.height;
		finfo->fb.var.width	    = mach_info.width;
		
		finfo->fb.var.xres	    = mach_info.xres;
		finfo->fb.var.xres_virtual    = mach_info.xres_virtual;

		finfo->fb.var.yres	    = mach_info.yres;
		finfo->fb.var.yres_virtual    = mach_info.yres_virtual;
	}
	else{
		finfo->fb.var.height	    = mach_info.osd_height;
		finfo->fb.var.width	    = mach_info.osd_width;
		
		finfo->fb.var.xres	    = mach_info.osd_xres;
		finfo->fb.var.xres_virtual    = mach_info.osd_xres_virtual;

		finfo->fb.var.yres	    = mach_info.osd_yres;
		finfo->fb.var.yres_virtual    = mach_info.osd_yres_virtual;
	}
	
	finfo->fb.var.bits_per_pixel  = mach_info.bpp;
        finfo->fb.var.pixclock = mach_info.pixclock;
	finfo->fb.var.hsync_len = mach_info.hsync_len;
	finfo->fb.var.left_margin = mach_info.left_margin;
	finfo->fb.var.right_margin = mach_info.right_margin;
	finfo->fb.var.vsync_len = mach_info.vsync_len;
	finfo->fb.var.upper_margin = mach_info.upper_margin;
	finfo->fb.var.lower_margin = mach_info.lower_margin;
	finfo->fb.var.sync = mach_info.sync;
	finfo->fb.var.grayscale = mach_info.cmap_grayscale;

#if defined(CONFIG_G3D)
	finfo->fb.fix.smem_len	    = finfo->fb.var.xres_virtual * finfo->fb.var.yres_virtual *
				      mach_info.bytes_per_pixel * 3;
#else
	//finfo->fb.fix.smem_len	    = finfo->fb.var.xres_virtual * finfo->fb.var.yres_virtual 							*mach_info.bytes_per_pixel;
	finfo->fb.fix.smem_len	    = finfo->fb.var.xres_virtual * finfo->fb.var.yres_virtual 							* mach_info.bytes_per_pixel * 2;
#endif

	finfo->fb.fix.line_length   = finfo->fb.var.width * mach_info.bytes_per_pixel;

	for (i = 0; i < 256; i++)
		finfo->palette_buffer[i] = PALETTE_BUFF_CLEAR;

}



/* =================== Double buffering ====================================== */
static int fbNum = 0;
void s3c_fb_change_req(int req_fbNum) {
	fbNum = req_fbNum;
	info[fbNum].fb_change_ready = 1;
}


static irqreturn_t s3c_fb_irq(int irqno, void *param)
{
	
	// VSYNC interrupt
	// 1. For changing a palette
	if (info[palette_win].palette_ready) {
		s3c_fb_write_palette(&info[palette_win]);
	}	

/* Application want to change FB by WaitForVsync.
	// 2. For changing the working frame buffer
	if (info[fbNum].fb_change_ready) {
		s3c_fb_set_fb_change(fbNum);
	}
*/

#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	// For clearing the interrupt source
	__raw_writel(__raw_readl(S3C_VIDINTCON1), S3C_VIDINTCON1); 
#endif

  	//(WAITFORVSYNC)
  	s3cfb_vSyncInfo.count++;
  	wake_up_interruptible( &s3cfb_vSyncInfo.waitQueue );
 	 //(WAITFORVSYNC)

	return IRQ_HANDLED;
}


//(WAITFORVSYNC)
int s3cfb_wait_for_sync(u_int32_t crtc) 
{
	int cnt;

  	cnt = s3cfb_vSyncInfo.count;
	wait_event_interruptible_timeout(s3cfb_vSyncInfo.waitQueue, cnt != s3cfb_vSyncInfo.count, HZ/10);

  	return cnt;
}
//(WAITFORVSYNC)


void s3c_fb_change_buff(int req_winNum, int req_fb) {
#if (defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)) && defined(CONFIG_FB_DOUBLE_BUFFERING)	
		// Software-based trigger
		__raw_writel((1<<0), S3C_CPUTRIGCON2);
#elif (defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)) && defined(CONFIG_FB_DOUBLE_BUFFERING)
		switch (req_winNum) 
		{
			case 0 : 	// In case of windows 0
             			if (0 == req_fb)  mach_info.wincon0 &= ~BIT20;
	      			else mach_info.wincon0 |= BIT20;
                             __raw_writel(mach_info.wincon0 | S3C_WINCONx_ENWIN_F_ENABLE, S3C_WINCON0);	
				break;
				
			case 1 : // In case of windows 1
		     		if (0 == req_fb)  mach_info.wincon1 &= ~BIT20;
	      			else mach_info.wincon1 |= BIT20;
			       __raw_writel(mach_info.wincon1 | S3C_WINCONx_ENWIN_F_ENABLE, S3C_WINCON1);
				break;
				
			default :
				break;
		}
#endif // #if defined(CONFIG_CPU_S3C2443) 
}


int s3c_fb_get_win_buff_status(int winNum)
{
	int retVal = 0;
	int buffStatus, buffSelect;
	
	switch (winNum)
	{
		case 0 :
			buffStatus = (readl(S3C_WINCON0) & BIT21) >> 21;
			buffSelect = (readl(S3C_WINCON0) & BIT20) >> 20;
			if (buffStatus == buffSelect)
			{
				if (0 == buffSelect) retVal = 0;
				else retVal = 1;
			}
			else 
			{
				if (0 == buffSelect) retVal = -1;
				else retVal = -2;
			}
			break;
		case 1:
			buffStatus = (readl(S3C_WINCON1) & BIT21) >> 21;
			buffSelect = (readl(S3C_WINCON1) & BIT20) >> 20;
			if (buffStatus == buffSelect)
			{
				if (0 == buffSelect) retVal = 0;
				else retVal = 1;
			}
			else 
			{
				if (0 == buffSelect) retVal = -1;
				else retVal = -2;
			}
			break;
		default :
			break;
	}
	return (retVal);
}

#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
void s3c_fb_enable_local_post(int in_yuv)
{
	u32 value;

	mach_info.wincon0 &= ~(BIT22 | BIT13);
	value = S3C_WINCONx_ENLOCAL_POST | S3C_WINCONx_ENWIN_F_ENABLE;

	if (in_yuv)
		value |= S3C_WINCONx_INRGB_YUV;
	
	__raw_writel(mach_info.wincon0 | value, S3C_WINCON0);

	return 0;
}

void s3c_fb_enable_dma(void)
{
	u32 value;

	mach_info.wincon0 &= ~(BIT22 | BIT13);
	value = S3C_WINCONx_ENLOCAL_DMA | S3C_WINCONx_ENWIN_F_ENABLE;

	__raw_writel(mach_info.wincon0 | value, S3C_WINCON0);

	return 0;
}

EXPORT_SYMBOL(s3c_fb_enable_dma);
EXPORT_SYMBOL(s3c_fb_enable_local_post);
#endif

/* =================== DVS ======================================= */

#if defined (CONFIG_S3C_DVS) && defined (CONFIG_CPU_S3C2412)
static irqreturn_t s3c_dvs_irq(int irqno, void *param)
{
	s3c_dvs_action();

	return IRQ_HANDLED;
}
#endif



static int __devinit s3c_fb_probe(struct platform_device *pdev)
{
	char driver_name[]="s3c_fb";
	int ret;
	int index=0;
	printk("%d\n",S3C_FB_NUM);
	for(index=0; index<S3C_FB_NUM; index++){ 
		s3c_fb_init_fbinfo(&info[index], driver_name, index);

		if(index==0){
			s3c_fb_backlight_power(1);
			s3c_fb_lcd_power(1);
			s3c_fb_backlight_level(DEFAULT_BACKLIGHT_LEVEL);

			dprintk("dev FB init\n");

			if (!request_mem_region((unsigned long)S3C24XX_VA_LCD, SZ_1M, "s3c-lcd")) {
		                ret = -EBUSY;
				goto dealloc_fb;
		        }

			dprintk("got LCD region\n");

			lcd_clock = clk_get(NULL, "lcd");
			if (!lcd_clock) {
				printk(KERN_INFO "failed to get lcd clock source\n");
				ret =  -ENOENT;
		                goto release_irq;
			}

			clk_enable(lcd_clock);
			printk("S3C_LCD clock got enabled :: %ld.%03ld Mhz\n", print_mhz(clk_get_rate(lcd_clock)));

			msleep(5);
		}

		/* Initialize video memory */
		ret = s3c_fb_map_video_memory(&info[index]);
		if (ret) {
			printk("Failed to allocate video RAM: %d\n", ret);
			ret = -ENOMEM;
			goto release_clock;
		}
		dprintk("got video memory\n");

		ret = s3c_fb_init_registers(&info[index]);
		ret = s3c_fb_check_var(&info[index].fb.var, &info[index].fb);

		if(index<2){
			/* 2007-01-09-Tue. for RGB 8-8-8 palette*/
			if(fb_alloc_cmap(&info[index].fb.cmap, 256, 0)<0){
				goto dealloc_fb;
			}
		}
		else{
			/* 2007-01-09-Tue. for RGB 8-8-8 palette*/
			if(fb_alloc_cmap(&info[index].fb.cmap, 16, 0)<0){
				goto dealloc_fb;
			}
		}
		
		ret = register_framebuffer(&info[index].fb);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register framebuffer device: %d\n", ret);
			goto free_video_memory;
		}
	}// 	for(index=0; index<CONFIG_FB_NUM; index++)

//(WAITFORVSYNC)
  //    - initialize the struct for Waitforvsync
  s3cfb_vSyncInfo.count = 0;
  init_waitqueue_head(&s3cfb_vSyncInfo.waitQueue);
  //(WAITFORVSYNC)

#if defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)
	ret = request_irq(IRQ_S3C2443_LCD3, s3c_fb_irq, 0, "s3c-lcd", pdev);
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	ret = request_irq(IRQ_LCD_VSYNC, s3c_fb_irq, 0, "s3c-lcd", pdev);
#elif defined (CONFIG_S3C_DVS) && defined (CONFIG_CPU_S3C2412)
	ret = request_irq(IRQ_LCD, s3c_dvs_irq, 0, "s3c-lcd", pdev);
#endif
	if (ret != 0) {
		printk("Failed to install irq (%d)\n", ret);
		goto release_irq;
	}

#if defined (CONFIG_S3C_DVS) & defined (CONFIG_CPU_S3C2412)
	disable_irq(IRQ_LCD);
#endif

	/* create device files */
	ret = device_create_file(&(pdev->dev), &dev_attr_backlight_power);
	if (ret < 0)
		printk(KERN_WARNING "s3cfb: failed to add entries\n");
	
	ret = device_create_file(&(pdev->dev), &dev_attr_backlight_level);
	if (ret < 0)
		printk(KERN_WARNING "s3cfb: failed to add entries\n");
	
	ret = device_create_file(&(pdev->dev), &dev_attr_lcd_power);

	if (ret < 0)
		printk(KERN_WARNING "s3cfb: failed to add entries\n");

	printk(KERN_INFO "fb%d: %s frame buffer device\n",
		info[index].fb.node, info[index].fb.fix.id);
	return 0;

free_video_memory:
	s3c_fb_unmap_video_memory(&info[index]);

release_clock:
	clk_disable(lcd_clock);
	clk_put(lcd_clock);

release_irq:
#if defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416)
	free_irq(IRQ_S3C2443_LCD3, &info);
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	free_irq(IRQ_LCD_VSYNC, &info);
#endif
//release_mem:
	release_mem_region((unsigned long)S3C24XX_VA_LCD, S3C24XX_SZ_LCD);
dealloc_fb:
	framebuffer_release(&info[index].fb);
	return ret;
}

/* -----------------------------------------
 * s3c_fb_stop_lcd & s3c_fb_start_lcd
 *
 * shutdown/ start the lcd controller
 */
static void s3c_fb_stop_lcd(void)
{
	unsigned long flags;
	unsigned long tmp;

	local_irq_save(flags);

#if defined(CONFIG_CPU_S3C2443) || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) || defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	tmp = __raw_readl(S3C_VIDCON0);
	__raw_writel(tmp & ~(S3C_VIDCON0_ENVID_ENABLE | S3C_VIDCON0_ENVID_F_ENABLE), S3C_VIDCON0);
#else
	tmp = __raw_readl(S3C_LCDCON1);
	__raw_writel(tmp & ~S3C2410_LCDCON1_ENVID, S3C_LCDCON1);
#endif
	local_irq_restore(flags);
}

void s3c_fb_start_lcd(void) {
	unsigned long flags;
	unsigned long tmp;

	local_irq_save(flags);

#if defined(CONFIG_FB_LTV350QV)  || defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) || defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	tmp = __raw_readl(S3C_VIDCON0);
	__raw_writel(tmp | S3C_VIDCON0_ENVID_ENABLE | S3C_VIDCON0_ENVID_F_ENABLE, S3C_VIDCON0);
#else
	tmp = __raw_readl(S3C_LCDCON1);
	__raw_writel(tmp | S3C_WINCONx_ENWIN_F_ENABLE, S3C_LCDCON1);
#endif
	local_irq_restore(flags);	
}


/*
 *  Cleanup
 */
static int s3c_fb_remove(struct platform_device *pdev)
{
	struct fb_info	   *fbinfo = platform_get_drvdata(pdev);
	struct s3c_fb_info *info = fbinfo->par;
	int irq;
	int index=0;

	s3c_fb_stop_lcd();
	msleep(1);

	for(index=0; index<S3C_FB_NUM; index++){ 
		s3c_fb_unmap_video_memory((struct s3c_fb_info *)&info[index]);

	 	if (lcd_clock) {
	 		clk_disable(lcd_clock);
	 		clk_put(lcd_clock);
	 		lcd_clock = NULL;
		}

		irq = platform_get_irq(pdev, 0);
		free_irq(irq,&info[index]);
		release_mem_region((unsigned long)S3C24XX_VA_LCD, S3C24XX_SZ_LCD);
		unregister_framebuffer(&(info[index].fb));
	} // for(index=0; index<CONFIG_FB_NUM; index++)
	return 0;
}

#ifdef CONFIG_PM

static struct sleep_save lcd_save[] = {
#if defined(CONFIG_CPU_S3C2450) || defined(CONFIG_CPU_S3C2416) 
        SAVE_ITEM(S3C_VIDCON0),SAVE_ITEM(S3C_VIDCON1),
        SAVE_ITEM(S3C_VIDTCON0),SAVE_ITEM(S3C_VIDTCON1),
        SAVE_ITEM(S3C_VIDTCON2),SAVE_ITEM(S3C_WINCON0),
        SAVE_ITEM(S3C_WINCON1),
        SAVE_ITEM(S3C_VIDINTCON),SAVE_ITEM(S3C_SYSIFCON0),
        SAVE_ITEM(S3C_SIFCCON0),

        SAVE_ITEM(S3C_CPUTRIGCON2),SAVE_ITEM(S3C_VIDOSD0A),
        SAVE_ITEM(S3C_VIDOSD0B),SAVE_ITEM(S3C_VIDOSD0C),
        SAVE_ITEM(S3C_VIDOSD1A),SAVE_ITEM(S3C_VIDOSD1B),
        SAVE_ITEM(S3C_VIDOSD1C),SAVE_ITEM(S3C_VIDW00ADD0B0),
        SAVE_ITEM(S3C_VIDW00ADD0B1),SAVE_ITEM(S3C_VIDW01ADD0),
        SAVE_ITEM(S3C_VIDW00ADD1B0),SAVE_ITEM(S3C_VIDW00ADD1B1),
        SAVE_ITEM(S3C_VIDW01ADD1),SAVE_ITEM(S3C_VIDW00ADD2B0),
        SAVE_ITEM(S3C_VIDW00ADD2B1),SAVE_ITEM(S3C_VIDW01ADD2),
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	SAVE_ITEM(S3C_VIDCON0),
	SAVE_ITEM(S3C_VIDCON1),	

	SAVE_ITEM(S3C_VIDTCON0),	
	SAVE_ITEM(S3C_VIDTCON1),	
	SAVE_ITEM(S3C_VIDTCON2),	
	SAVE_ITEM(S3C_VIDTCON3),	

	SAVE_ITEM(S3C_WINCON0),	
	SAVE_ITEM(S3C_WINCON1),	
	SAVE_ITEM(S3C_WINCON2),	
	SAVE_ITEM(S3C_WINCON3),	
	SAVE_ITEM(S3C_WINCON4),	


	SAVE_ITEM(S3C_VIDOSD0A),	
	SAVE_ITEM(S3C_VIDOSD0B),	
	SAVE_ITEM(S3C_VIDOSD0C),	

	SAVE_ITEM(S3C_VIDOSD1A),	
	SAVE_ITEM(S3C_VIDOSD1B),	
	SAVE_ITEM(S3C_VIDOSD1C),	
	SAVE_ITEM(S3C_VIDOSD1D),	

	SAVE_ITEM(S3C_VIDOSD2A),	
	SAVE_ITEM(S3C_VIDOSD2B),	
	SAVE_ITEM(S3C_VIDOSD2C),	
	SAVE_ITEM(S3C_VIDOSD2D),	

	SAVE_ITEM(S3C_VIDOSD3A),	
	SAVE_ITEM(S3C_VIDOSD3B),	
	SAVE_ITEM(S3C_VIDOSD3C),	

	SAVE_ITEM(S3C_VIDOSD4A),	
	SAVE_ITEM(S3C_VIDOSD4B),	
	SAVE_ITEM(S3C_VIDOSD4C),	

	SAVE_ITEM(S3C_VIDW00ADD0B0),	
	SAVE_ITEM(S3C_VIDW00ADD0B1),	
	SAVE_ITEM(S3C_VIDW01ADD0B0),	
	SAVE_ITEM(S3C_VIDW01ADD0B1),	
	SAVE_ITEM(S3C_VIDW02ADD0),
	SAVE_ITEM(S3C_VIDW03ADD0),
	SAVE_ITEM(S3C_VIDW04ADD0),
	SAVE_ITEM(S3C_VIDW00ADD1B0),	
	SAVE_ITEM(S3C_VIDW00ADD1B1),	
	SAVE_ITEM(S3C_VIDW01ADD1B0),	
	SAVE_ITEM(S3C_VIDW01ADD1B1),	
	SAVE_ITEM(S3C_VIDW02ADD1),
	SAVE_ITEM(S3C_VIDW03ADD1),
	SAVE_ITEM(S3C_VIDW04ADD1),
	SAVE_ITEM(S3C_VIDW00ADD2),
	SAVE_ITEM(S3C_VIDW01ADD2),
	SAVE_ITEM(S3C_VIDW02ADD2),
	SAVE_ITEM(S3C_VIDW03ADD2),
	SAVE_ITEM(S3C_VIDW04ADD2),

	SAVE_ITEM(S3C_VIDINTCON0),	
	SAVE_ITEM(S3C_VIDINTCON1),	
	SAVE_ITEM(S3C_W1KEYCON0),	
	SAVE_ITEM(S3C_W1KEYCON1),	
	SAVE_ITEM(S3C_W2KEYCON0),	
	SAVE_ITEM(S3C_W2KEYCON1),	

	SAVE_ITEM(S3C_W3KEYCON0),	
	SAVE_ITEM(S3C_W3KEYCON1),	
	SAVE_ITEM(S3C_W4KEYCON0),	
	SAVE_ITEM(S3C_W4KEYCON1),	
	SAVE_ITEM(S3C_DITHMODE),	

	SAVE_ITEM(S3C_WIN0MAP),		
	SAVE_ITEM(S3C_WIN1MAP),		
	SAVE_ITEM(S3C_WIN2MAP),		
	SAVE_ITEM(S3C_WIN3MAP),		
	SAVE_ITEM(S3C_WIN4MAP),		
	SAVE_ITEM(S3C_WPALCON),		

	SAVE_ITEM(S3C_TRIGCON),	
	SAVE_ITEM(S3C_I80IFCONA0),	
	SAVE_ITEM(S3C_I80IFCONA1),	
	SAVE_ITEM(S3C_I80IFCONB0),	
	SAVE_ITEM(S3C_I80IFCONB1),	
	SAVE_ITEM(S3C_LDI_CMDCON0),	
	SAVE_ITEM(S3C_LDI_CMDCON1),	
	SAVE_ITEM(S3C_SIFCCON0),	
	SAVE_ITEM(S3C_SIFCCON1),	
	SAVE_ITEM(S3C_SIFCCON2),	

	SAVE_ITEM(S3C_LDI_CMD0),	
	SAVE_ITEM(S3C_LDI_CMD1),	
	SAVE_ITEM(S3C_LDI_CMD2),	
	SAVE_ITEM(S3C_LDI_CMD3),	
	SAVE_ITEM(S3C_LDI_CMD4),	
	SAVE_ITEM(S3C_LDI_CMD5),	
	SAVE_ITEM(S3C_LDI_CMD6),	
	SAVE_ITEM(S3C_LDI_CMD7),	
	SAVE_ITEM(S3C_LDI_CMD8),	
	SAVE_ITEM(S3C_LDI_CMD9),	
	SAVE_ITEM(S3C_LDI_CMD10),	
	SAVE_ITEM(S3C_LDI_CMD11),	

	SAVE_ITEM(S3C_W2PDATA01),	
	SAVE_ITEM(S3C_W2PDATA23),	
	SAVE_ITEM(S3C_W2PDATA45),	
	SAVE_ITEM(S3C_W2PDATA67),	
	SAVE_ITEM(S3C_W2PDATA89),	
	SAVE_ITEM(S3C_W2PDATAAB),	
	SAVE_ITEM(S3C_W2PDATACD),	
	SAVE_ITEM(S3C_W2PDATAEF),	
	SAVE_ITEM(S3C_W3PDATA01),	
	SAVE_ITEM(S3C_W3PDATA23),	
	SAVE_ITEM(S3C_W3PDATA45),	
	SAVE_ITEM(S3C_W3PDATA67),	
	SAVE_ITEM(S3C_W3PDATA89), 	
	SAVE_ITEM(S3C_W3PDATAAB),	
	SAVE_ITEM(S3C_W3PDATACD),	
	SAVE_ITEM(S3C_W3PDATAEF),	
	SAVE_ITEM(S3C_W4PDATA01),	
	SAVE_ITEM(S3C_W4PDATA23),
#endif
};

/* suspend and resume support for the lcd controller */
static int s3c_fb_suspend(struct platform_device *dev, pm_message_t state)
{
	s3c_fb_stop_lcd();

	s3c2410_pm_do_save(lcd_save, ARRAY_SIZE(lcd_save));
	/* sleep before disabling the clock, we need to ensure
	 * the LCD DMA engine is not going to get back on the bus
	 * before the clock goes off again (bjd) */

	msleep(1);
	clk_disable(lcd_clock);

	return 0;
}

static int s3c_fb_resume(struct platform_device *dev)
{
	clk_enable(lcd_clock);
	msleep(1);
	s3c2410_pm_do_restore(lcd_save, ARRAY_SIZE(lcd_save));
        Init_LDI();

	s3c_fb_start_lcd();
	return 0;
}

#else
#define s3c_fb_suspend NULL
#define s3c_fb_resume  NULL
#endif

static struct platform_driver s3c_fb_driver = {
	.probe		= s3c_fb_probe,
        .remove		= s3c_fb_remove,
	.suspend	= s3c_fb_suspend,
	.resume		= s3c_fb_resume,
        .driver		= {
		.name	= "s3c-lcd",
		.owner	= THIS_MODULE,
	},
};

int __devinit s3c_fb_init(void)
{
	return platform_driver_register(&s3c_fb_driver);
}
static void __exit s3c_fb_cleanup(void)
{
	platform_driver_unregister(&s3c_fb_driver);
}

EXPORT_SYMBOL(s3c_fb_change_req);
EXPORT_SYMBOL(s3c_fb_stop_lcd);
EXPORT_SYMBOL(s3c_fb_start_lcd);
EXPORT_SYMBOL(s3c_fb_get_win_buff_status);
EXPORT_SYMBOL(s3c_fb_change_buff);


module_init(s3c_fb_init);
module_exit(s3c_fb_cleanup);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("Framebuffer driver for the S3C");
MODULE_LICENSE("GPL");
