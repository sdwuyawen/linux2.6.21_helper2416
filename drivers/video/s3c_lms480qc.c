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
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <asm/mach/map.h>
#include <asm/arch/regs-lcd.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-spi.h>
#include <asm/arch/regs-s3c6400-clock.h>

#include "s3cfb.h"

#define ON 		1
#define OFF		0

#define DEFAULT_BACKLIGHT_LEVEL		2

#define H_FP		10		/* front porch */
#define H_SW		1		/* Hsync width */
#define H_BP		104		/* Back porch */

#define V_FP		8		/* front porch */
#define V_SW		1		/* Vsync width */
#define V_BP		21		/* Back porch */

extern struct s3c_fb_info info[S3C_FB_NUM];
s3c_win_info_t window_info;

/* for spi controller */
static void __iomem *spi_base;

//------------------ Virtual Screen -----------------------
#if defined(CONFIG_FB_VIRTUAL_SCREEN)
vs_info_t vs_info;

#define START_VIRTUAL_LCD 		11
#define STOP_VIRTUAL_LCD 		10
#define SET_VIRTUAL_LCD 		12

#define VS_MOVE_LEFT			15
#define VS_MOVE_RIGHT			16
#define VS_MOVE_UP			17
#define VS_MOVE_DOWN			18

#define MAX_DISPLAY_OFFSET		200
#define DEF_DISPLAY_OFFSET		100

int virtual_display_offset = DEF_DISPLAY_OFFSET;
#endif

//------------------ OSD (On Screen Display) -----------------------
#define START_OSD		1
#define STOP_OSD		0

// QCIF OSD image
#if defined(CONFIG_CPU_S3C2443)
#define H_RESOLUTION_OSD	100	/* horizon pixel  x resolition */
#define V_RESOLUTION_OSD	30	/* line cnt       y resolution */
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
#define H_RESOLUTION_OSD	800	/* horizon pixel  x resolition */
#define V_RESOLUTION_OSD	480	/* line cnt       y resolution */
#endif

#define ALPHA_UP		3
#define ALPHA_DOWN		4
#define MOVE_LEFT		5
#define MOVE_RIGHT		6
#define MOVE_UP			7
#define MOVE_DOWN		8

#define MAX_ALPHA_LEVEL		0x0f

int osd_alpha_level = MAX_ALPHA_LEVEL;
int osd_left_top_x = 0;
int osd_left_top_y = 0;
int osd_right_bottom_x = H_RESOLUTION_OSD-1;
int osd_left_bottom_y = V_RESOLUTION_OSD -1;
//------------------------------------------------------------------------

#define H_RESOLUTION	800	/* horizon pixel  x resolition */
#define V_RESOLUTION	480	/* line cnt       y resolution */

#define H_RESOLUTION_VIRTUAL	800	/* horizon pixel  x resolition */
#define V_RESOLUTION_VIRTUAL	600	/* line cnt       y resolution */


#if defined(CONFIG_CPU_S3C2443)
#define VFRAME_FREQ     75	/* frame rate freq */
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
#define VFRAME_FREQ     60	/* frame rate freq */
#endif
#define PIXEL_BPP8	8
#define PIXEL_BPP16	16	/*  RGB 5-6-5 format for SMDK EVAL BOARD */
#define PIXEL_BPP24	24	/*  RGB 8-8-8 format for SMDK EVAL BOARD */

#define LCD_PIXEL_CLOCK (VFRAME_FREQ *(H_FP+H_SW+H_BP+H_RESOLUTION) * (V_FP+V_SW+V_BP+V_RESOLUTION))
#define PIXEL_CLOCK	VFRAME_FREQ * LCD_PIXEL_CLOCK	/*  vclk = frame * pixel_count */

#define MAX_DISPLAY_BRIGHTNESS		9
#define DEF_DISPLAY_BRIGHTNESS		4

int display_brightness = DEF_DISPLAY_BRIGHTNESS;

void set_brightness(int);

struct s3c_fb_mach_info mach_info = {

#if defined(CONFIG_CPU_S3C2443)
	.vidcon0= S3C_VIDCON0_VIDOUT_RGB_IF | S3C_VIDCON0_PNRMODE_RGB_P | S3C_VIDCON0_CLKDIR_DIVIDED | S3C_VIDCON0_CLKSEL_F_HCLK,
	.vidcon1= S3C_VIDCON1_IHSYNC_INVERT | S3C_VIDCON1_IVSYNC_INVERT,
	.vidtcon0= S3C_VIDTCON0_VBPDE(V_BP-1) | S3C_VIDTCON0_VFPD(V_FP-1) | S3C_VIDTCON0_VSPW(V_SW-1),
	.vidtcon1= S3C_VIDTCON1_HBPD(H_BP-1) | S3C_VIDTCON1_HFPD(H_FP-1 ) | S3C_VIDTCON1_HSPW(H_SW-1),

#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	.vidcon0 = S3C_VIDCON0_INTERLACE_F_PROGRESSIVE | S3C_VIDCON0_VIDOUT_RGB_IF | S3C_VIDCON0_L1_DATA16_SUB_16_MODE 
		| S3C_VIDCON0_L0_DATA16_MAIN_16_MODE | S3C_VIDCON0_PNRMODE_RGB_P 
		| S3C_VIDCON0_CLKVALUP_ALWAYS | S3C_VIDCON0_CLKDIR_DIVIDED | S3C_VIDCON0_CLKSEL_F_HCLK |
		S3C_VIDCON0_ENVID_DISABLE | S3C_VIDCON0_ENVID_F_DISABLE,
	.vidcon1 = S3C_VIDCON1_IHSYNC_NORMAL | S3C_VIDCON1_IVSYNC_NORMAL |S3C_VIDCON1_IVDEN_NORMAL | S3C_VIDCON1_IVCLK_FALL_EDGE,
	.vidtcon0 = S3C_VIDTCON0_VBPDE(0) | S3C_VIDTCON0_VBPD(V_BP-1) | S3C_VIDTCON0_VFPD(V_FP-1) | S3C_VIDTCON0_VSPW(V_SW-1),
	.vidtcon1 = S3C_VIDTCON1_VFPDE(0) | S3C_VIDTCON1_HBPD(H_BP-1) | S3C_VIDTCON1_HFPD(H_FP-1) | S3C_VIDTCON1_HSPW(H_SW-1),
#endif
	.vidtcon2= S3C_VIDTCON2_LINEVAL(V_RESOLUTION-1) | S3C_VIDTCON2_HOZVAL(H_RESOLUTION-1),

#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	.dithmode = (S3C_DITHMODE_RDITHPOS_5BIT | S3C_DITHMODE_GDITHPOS_6BIT | S3C_DITHMODE_BDITHPOS_5BIT) & S3C_DITHMODE_DITHERING_DISABLE,
	//24 bit: .dithmode = (S3C_DITHMODE_RDITHPOS_8BIT | S3C_DITHMODE_GDITHPOS_8BIT | S3C_DITHMODE_BDITHPOS_8BIT) & S3C_DITHMODE_DITHERING_DISABLE,
#endif

#if defined (CONFIG_FB_BPP_8)
	.wincon0=  S3C_WINCONx_BYTSWP_ENABLE |S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_8BPP_PAL,  // 4word burst, 8bpp-palletized,
	.wincon1=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1,  // 4word burst, 16bpp for OSD
	//.wincon1=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_A555 | S3C_WINCONx_BLD_PIX_PIXEL| S3C_WINCONx_ALPHA_SEL_1,  // 4word burst, 16bpp for OSD

#elif defined (CONFIG_FB_BPP_16)
	#if defined(CONFIG_CPU_S3C2443)
	.wincon0=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565,  // 4word burst, 16bpp,
	.wincon1=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1,  // 4word burst, 16bpp for OSD
	//.wincon1=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_A555 | S3C_WINCONx_BLD_PIX_PIXEL| S3C_WINCONx_ALPHA_SEL_1,  // 4word burst, 16bpp for OSD
	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
//	.wincon0 =  S3C_WINCONx_ENLOCAL_DMA | S3C_WINCONx_BUFSEL_0 | S3C_WINCONx_BUFAUTOEN_DISABLE | S3C_WINCONx_BITSWP_DISABLE |
	.wincon0 =  S3C_WINCONx_ENLOCAL_DMA | S3C_WINCONx_BUFSEL_1 | S3C_WINCONx_BUFAUTOEN_DISABLE | S3C_WINCONx_BITSWP_DISABLE |
		S3C_WINCONx_BYTSWP_DISABLE | S3C_WINCONx_HAWSWP_ENABLE|
		S3C_WINCONx_BURSTLEN_16WORD | S3C_WINCONx_BPPMODE_F_16BPP_565 |
#if defined(CONFIG_FB_DOUBLE_BUFFERING)
		//S3C_WINCONx_BUFAUTOEN_ENABLE |
#endif
		S3C_WINCONx_ENWIN_F_DISABLE,  // 4word burst, 16bpp,

	.wincon1 =  S3C_WINCONx_ENLOCAL_DMA | S3C_WINCONx_BUFSEL_0 | S3C_WINCONx_BUFAUTOEN_DISABLE | S3C_WINCONx_BITSWP_DISABLE |
		S3C_WINCONx_BYTSWP_DISABLE | S3C_WINCONx_HAWSWP_ENABLE |
		S3C_WINCONx_BURSTLEN_16WORD | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_BPPMODE_F_16BPP_565 |
		
#if defined(CONFIG_FB_DOUBLE_BUFFERING)
		//S3C_WINCONx_BUFAUTOEN_ENABLE |
#endif
		S3C_WINCONx_ALPHA_SEL_1 | S3C_WINCONx_ENWIN_F_DISABLE,  // 4word burst, 16bpp,
		
	.wincon2 = S3C_WINCONx_ENLOCAL_DMA | S3C_WINCONx_BITSWP_DISABLE |
		S3C_WINCONx_BYTSWP_DISABLE | S3C_WINCONx_HAWSWP_ENABLE|
		S3C_WINCONx_BURSTLEN_4WORD | S3C_WINCONx_BURSTLEN_16WORD | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_BPPMODE_F_16BPP_565 |
		S3C_WINCONx_ALPHA_SEL_1 | S3C_WINCONx_ENWIN_F_DISABLE,
		
	.wincon3 = S3C_WINCONx_BITSWP_DISABLE | S3C_WINCONx_BYTSWP_DISABLE | S3C_WINCONx_HAWSWP_ENABLE |
		S3C_WINCONx_BURSTLEN_4WORD | S3C_WINCONx_BURSTLEN_16WORD | S3C_WINCONx_BLD_PIX_PLANE |
		S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_ALPHA_SEL_1 | S3C_WINCONx_ENWIN_F_DISABLE,
		
	.wincon4 = S3C_WINCONx_BITSWP_DISABLE | S3C_WINCONx_BYTSWP_DISABLE | S3C_WINCONx_HAWSWP_ENABLE|
		S3C_WINCONx_BURSTLEN_4WORD | S3C_WINCONx_BURSTLEN_16WORD | S3C_WINCONx_BLD_PIX_PLANE |
		S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_ALPHA_SEL_1 | S3C_WINCONx_ENWIN_F_DISABLE,
		
	#endif
#elif defined (CONFIG_FB_BPP_24)
	.wincon0=  S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888,  // 4word burst, 24bpp,
	.wincon1=  S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1,  // 4word burst, 24bpp for OSD
	#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	.wincon2 = S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1, 
	.wincon3 = S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1,
	.wincon4 = S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1, 
	#endif
#endif

#if defined(CONFIG_CPU_S3C2443)
	.vidosd0a= S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0),
	.vidosd0b= S3C_VIDOSDxB_OSD_RBX_F(H_RESOLUTION-1) | S3C_VIDOSDxB_OSD_RBY_F(V_RESOLUTION-1),

	.vidosd1a= S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0),
	.vidosd1b= S3C_VIDOSDxB_OSD_RBX_F(H_RESOLUTION_OSD-1) | S3C_VIDOSDxB_OSD_RBY_F(V_RESOLUTION_OSD-1),
	.vidosd1c= S3C_VIDOSDxC_ALPHA1_B(MAX_ALPHA_LEVEL) | S3C_VIDOSDxC_ALPHA1_G(MAX_ALPHA_LEVEL) |S3C_VIDOSDxC_ALPHA1_R(MAX_ALPHA_LEVEL),
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	.vidosd0a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0),
	.vidosd0b = S3C_VIDOSDxB_OSD_RBX_F(H_RESOLUTION-1) | S3C_VIDOSDxB_OSD_RBY_F(V_RESOLUTION-1),
	.vidosd0c = S3C_VIDOSDxD_OSDSIZE(H_RESOLUTION*V_RESOLUTION),

	.vidosd1a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0),
	.vidosd1b = S3C_VIDOSDxB_OSD_RBX_F(H_RESOLUTION_OSD-1) | S3C_VIDOSDxB_OSD_RBY_F(V_RESOLUTION_OSD-1),
	.vidosd1c= S3C_VIDOSDxC_ALPHA1_B(MAX_ALPHA_LEVEL) | S3C_VIDOSDxC_ALPHA1_G(MAX_ALPHA_LEVEL) |S3C_VIDOSDxC_ALPHA1_R(MAX_ALPHA_LEVEL),
	.vidosd1d = S3C_VIDOSDxD_OSDSIZE(H_RESOLUTION*V_RESOLUTION),

	.vidosd2a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0),
	.vidosd2b = S3C_VIDOSDxB_OSD_RBX_F(H_RESOLUTION_OSD-1) | S3C_VIDOSDxB_OSD_RBY_F(V_RESOLUTION_OSD-1),
	.vidosd2c = S3C_VIDOSDxC_ALPHA1_B(MAX_ALPHA_LEVEL) | S3C_VIDOSDxC_ALPHA1_G(MAX_ALPHA_LEVEL) |S3C_VIDOSDxC_ALPHA1_R(MAX_ALPHA_LEVEL),
	.vidosd2d = S3C_VIDOSDxD_OSDSIZE(H_RESOLUTION*V_RESOLUTION),

	.vidosd3a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0),
	.vidosd3b = S3C_VIDOSDxB_OSD_RBX_F(H_RESOLUTION_OSD-1) | S3C_VIDOSDxB_OSD_RBY_F(V_RESOLUTION_OSD-1),
	.vidosd3c = S3C_VIDOSDxC_ALPHA1_B(MAX_ALPHA_LEVEL) | S3C_VIDOSDxC_ALPHA1_G(MAX_ALPHA_LEVEL) |S3C_VIDOSDxC_ALPHA1_R(MAX_ALPHA_LEVEL),

	.vidosd4a = S3C_VIDOSDxA_OSD_LTX_F(0) | S3C_VIDOSDxA_OSD_LTY_F(0),
	.vidosd4b = S3C_VIDOSDxB_OSD_RBX_F(H_RESOLUTION_OSD-1) | S3C_VIDOSDxB_OSD_RBY_F(V_RESOLUTION_OSD-1),
	.vidosd4c = S3C_VIDOSDxC_ALPHA1_B(MAX_ALPHA_LEVEL) | S3C_VIDOSDxC_ALPHA1_G(MAX_ALPHA_LEVEL) |S3C_VIDOSDxC_ALPHA1_R(MAX_ALPHA_LEVEL),
#endif

#if defined(CONFIG_CPU_S3C2443)
	.vidintcon= S3C_VIDINTCON0_FRAMESEL0_VSYNC | S3C_VIDINTCON0_FRAMESEL1_NONE | S3C_VIDINTCON0_INTFRMEN_ENABLE | S3C_VIDINTCON0_INTEN_ENABLE,
#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	.vidintcon0 = S3C_VIDINTCON0_FRAMESEL0_BACK | S3C_VIDINTCON0_FRAMESEL1_NONE | S3C_VIDINTCON0_INTFRMEN_DISABLE | S3C_VIDINTCON0_FIFOSEL_WIN0 | S3C_VIDINTCON0_FIFOLEVEL_25 | S3C_VIDINTCON0_INTFIFOEN_DISABLE | S3C_VIDINTCON0_INTEN_DISABLE,
	.vidintcon1 = 0,
#endif

	.width=	H_RESOLUTION,
	.height= V_RESOLUTION,
	.xres=	H_RESOLUTION,
	.yres=	V_RESOLUTION,

	.xoffset=	0,
	.yoffset=	0,

#if defined(CONFIG_FB_VIRTUAL_SCREEN)
	.xres_virtual =	H_RESOLUTION_VIRTUAL,
	.yres_virtual =	V_RESOLUTION_VIRTUAL,
#else
	.xres_virtual =	H_RESOLUTION,
	.yres_virtual =	V_RESOLUTION,
#endif

	.osd_width=	H_RESOLUTION_OSD,
	.osd_height=	V_RESOLUTION_OSD,
	.osd_xres=	H_RESOLUTION_OSD,
	.osd_yres=	V_RESOLUTION_OSD,

	.osd_xres_virtual=	H_RESOLUTION_OSD,
	.osd_yres_virtual=	V_RESOLUTION_OSD,

#if defined (CONFIG_FB_BPP_8)
	.bpp=		PIXEL_BPP8,
	.bytes_per_pixel= 1,
	#if defined(CONFIG_CPU_S3C2443)
	.wpalcon=	W0PAL_24BIT,
	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	.wpalcon=	W0PAL_16BIT,
	#endif
#elif defined (CONFIG_FB_BPP_16)
	.bpp=		PIXEL_BPP16,
	.bytes_per_pixel= 2,

#elif defined (CONFIG_FB_BPP_24)
        .bpp=		PIXEL_BPP24,
        .bytes_per_pixel= 4,

#endif
      	.pixclock=	PIXEL_CLOCK,

	.w1keycon0=	S3C_WxKEYCON0_KEYBLEN_DISABLE | S3C_WxKEYCON0_KEYEN_F_DISABLE | S3C_WxKEYCON0_DIRCON_MATCH_FG_IMAGE |
			S3C_WxKEYCON0_COMPKEY(0x0),
	.w1keycon1=	S3C_WxKEYCON1_COLVAL(0xffffff),
	.w2keycon0=	S3C_WxKEYCON0_KEYBLEN_DISABLE | S3C_WxKEYCON0_KEYEN_F_DISABLE | S3C_WxKEYCON0_DIRCON_MATCH_FG_IMAGE |
			S3C_WxKEYCON0_COMPKEY(0x0),
	.w2keycon1=	S3C_WxKEYCON1_COLVAL(0xffffff),
	.w3keycon0=	S3C_WxKEYCON0_KEYBLEN_DISABLE | S3C_WxKEYCON0_KEYEN_F_DISABLE | S3C_WxKEYCON0_DIRCON_MATCH_FG_IMAGE |
			S3C_WxKEYCON0_COMPKEY(0x0),
	.w3keycon1=	S3C_WxKEYCON1_COLVAL(0xffffff),
	.w4keycon0=	S3C_WxKEYCON0_KEYBLEN_DISABLE | S3C_WxKEYCON0_KEYEN_F_DISABLE | S3C_WxKEYCON0_DIRCON_MATCH_FG_IMAGE |
			S3C_WxKEYCON0_COMPKEY(0x0),
	.w4keycon1=	S3C_WxKEYCON1_COLVAL(0xffffff),


      	.hsync_len= 	H_SW,
	.vsync_len=	V_SW,

      	.left_margin= 	H_FP,
	.upper_margin=	V_FP,
      	.right_margin= 	H_BP,
	.lower_margin=	V_BP,

      	.sync= 		0,
	.cmap_static=	1,
};

#if defined(CONFIG_S3C6400_PWM)
void set_brightness(int val)
{
	int channel = 1;  // must use channel-1
	int usec = 0;       // don't care value 
	unsigned long tcnt=1000;
	unsigned long tcmp=0;

	if(val < 0) val=0;
	if(val > MAX_DISPLAY_BRIGHTNESS) val=MAX_DISPLAY_BRIGHTNESS;

	display_brightness = val;
	
	switch (val) {
		case 0:
			tcmp= 0;
			break;
		case 1:
			tcmp= 50;
			break;
		case 2:
			tcmp= 100;
			break;
		case 3:
			tcmp= 150;
			break;
		case 4:
			tcmp= 200;
			break;
		case 5:
			tcmp= 250;	
			break;
		case 6:
			tcmp= 300;			
			break;
		case 7:
			tcmp= 350;			
			break;			
		case 8:
			tcmp= 400;
			break;
		case 9:
			tcmp= 450;
			break;	
	}  // end of switch (level) 

	s3c6400_timer_setup (channel, usec, tcnt, tcmp); 

}
#endif


#if defined(CONFIG_FB_VIRTUAL_SCREEN)
void set_virtual_display_offset(int val)
{
	if(val < 1)
	   val = 1;
	if(val > MAX_DISPLAY_OFFSET)
	   val = MAX_DISPLAY_OFFSET;

	virtual_display_offset = val;
}

int set_vs_info(vs_info_t vs_info_from_app )
{

	/* check invalid value */
	if(vs_info_from_app.width != H_RESOLUTION || vs_info_from_app.height != V_RESOLUTION ){
		return 1;
	}
	if(!(vs_info_from_app.bpp==8 ||vs_info_from_app.bpp==16 ||vs_info_from_app.bpp==24 || vs_info_from_app.bpp==32) ){
		return 1;
	}
	if(vs_info_from_app.offset<0){
		return 1;
	}
	if(vs_info_from_app.v_width != H_RESOLUTION_VIRTUAL  || vs_info_from_app.v_height != V_RESOLUTION_VIRTUAL){
		return 1;
	}

	/* save virtual screen information */
	vs_info = vs_info_from_app;
	set_virtual_display_offset(vs_info.offset);
	return 0;
}


int set_virtual_display_register(int vs_cmd)
{
	int PageWidth, Offset;
	int ShiftValue;

	PageWidth = mach_info.xres * mach_info.bytes_per_pixel;
	Offset = (mach_info.xres_virtual - mach_info.xres) * mach_info.bytes_per_pixel;

	switch(vs_cmd){
		case SET_VIRTUAL_LCD:
			/* size of buffer */
			#if defined(CONFIG_CPU_S3C2443)
			mach_info.vidw00add2b0 = S3C_VIDWxxADD2_OFFSIZE_F(Offset) | (S3C_VIDWxxADD2_PAGEWIDTH_F(PageWidth));
			mach_info.vidw00add2b1 = S3C_VIDWxxADD2_OFFSIZE_F(Offset) | (S3C_VIDWxxADD2_PAGEWIDTH_F(PageWidth));

			__raw_writel(mach_info.vidw00add2b0, S3C_VIDW00ADD2B0);
			__raw_writel(mach_info.vidw00add2b1, S3C_VIDW00ADD2B1);
			#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
			mach_info.vidw00add2 = S3C_VIDWxxADD2_OFFSIZE_F(Offset) | (S3C_VIDWxxADD2_PAGEWIDTH_F(PageWidth));
			__raw_writel(mach_info.vidw00add2, S3C_VIDW00ADD2);
			#endif

			break;

		case VS_MOVE_LEFT:
			if(mach_info.xoffset < virtual_display_offset){
				ShiftValue = mach_info.xoffset;
			}
			else ShiftValue = virtual_display_offset;
			mach_info.xoffset -= ShiftValue;

			/* For buffer start address */
			mach_info.vidw00add0b0 = mach_info.vidw00add0b0 - ShiftValue*mach_info.bytes_per_pixel;
			mach_info.vidw00add0b1 = mach_info.vidw00add0b1 - ShiftValue*mach_info.bytes_per_pixel;
			break;

		case VS_MOVE_RIGHT:
			if((vs_info.v_width - (mach_info.xoffset + vs_info.width) )< (virtual_display_offset)){
				ShiftValue = vs_info.v_width - (mach_info.xoffset + vs_info.width);
			}
			else ShiftValue = virtual_display_offset;
			mach_info.xoffset += ShiftValue;

			/* For buffer start address */
			mach_info.vidw00add0b0 = mach_info.vidw00add0b0 + ShiftValue*mach_info.bytes_per_pixel;
			mach_info.vidw00add0b1 = mach_info.vidw00add0b1 + ShiftValue*mach_info.bytes_per_pixel;
			break;

		case VS_MOVE_UP:
			if(mach_info.yoffset < virtual_display_offset){
				ShiftValue = mach_info.yoffset;
			}
			else ShiftValue = virtual_display_offset;
			mach_info.yoffset -= ShiftValue;

			/* For buffer start address */
			mach_info.vidw00add0b0 = mach_info.vidw00add0b0 - ShiftValue*mach_info.xres_virtual*mach_info.bytes_per_pixel;
			mach_info.vidw00add0b1 = mach_info.vidw00add0b1 - ShiftValue*mach_info.xres_virtual*mach_info.bytes_per_pixel;
			break;

		case VS_MOVE_DOWN:
			if((vs_info.v_height - (mach_info.yoffset + vs_info.height) )< (virtual_display_offset)){
				ShiftValue = vs_info.v_height - (mach_info.yoffset + vs_info.height);
			}
			else ShiftValue = virtual_display_offset;
			mach_info.yoffset += ShiftValue;

			/* For buffer start address */
			mach_info.vidw00add0b0 = mach_info.vidw00add0b0 + ShiftValue*mach_info.xres_virtual*mach_info.bytes_per_pixel;
			mach_info.vidw00add0b1 = mach_info.vidw00add0b1 + ShiftValue*mach_info.xres_virtual*mach_info.bytes_per_pixel;
			break;

		default:
			return -EINVAL;
	}

	/* End address */
	mach_info.vidw00add1b0 = S3C_VIDWxxADD1_VBASEL_F(mach_info.vidw00add0b0 + (PageWidth + Offset) * (mach_info.yres));
	mach_info.vidw00add1b1 = S3C_VIDWxxADD1_VBASEL_F(mach_info.vidw00add0b1 + (PageWidth + Offset) * (mach_info.yres));

	__raw_writel(mach_info.vidw00add0b0, S3C_VIDW00ADD0B0);
	__raw_writel(mach_info.vidw00add0b1, S3C_VIDW00ADD0B1);

	__raw_writel(mach_info.vidw00add1b0, S3C_VIDW00ADD1B0);
	__raw_writel(mach_info.vidw00add1b1, S3C_VIDW00ADD1B1);

	return 0;
}
#endif


/*
 * As LCD-Brightness is best related to Display and hence FrameBuffer,
 * so we put the brightness control in /dev/fb.
 */
#define GET_DISPLAY_BRIGHTNESS   	 _IOR('F', 1, u_int)             /* get brightness */
#define SET_DISPLAY_BRIGHTNESS    	 _IOW('F', 2, u_int)             /* set brightness */


#if defined(CONFIG_FB_VIRTUAL_SCREEN)
#define SET_VS_START 			_IO('F', 103)
#define SET_VS_STOP 			_IO('F', 104)
#define SET_VS_INFO 			_IOW('F', 105, vs_info_t)
#define SET_VS_MOVE 			_IOW('F', 106, u_int)
#endif

#define SET_OSD_START 			_IO('F', 201)
#define SET_OSD_STOP 			_IO('F', 202)
#define SET_OSD_ALPHA_UP 		_IO('F', 203)
#define SET_OSD_ALPHA_DOWN 		_IO('F', 204)
#define SET_OSD_MOVE_LEFT 		_IO('F', 205)
#define SET_OSD_MOVE_RIGHT		_IO('F', 206)
#define SET_OSD_MOVE_UP 		_IO('F', 207)
#define SET_OSD_MOVE_DOWN		_IO('F', 208)
#define SET_OSD_INFO			_IOW('F', 209, s3c_win_info_t)
#if 0
#define SET_OSD_BRIGHT			_IOW('F', 210, int) 
#endif

#define SET_COLOR_KEY_START 		_IO('F', 300)
#define SET_COLOR_KEY_STOP 		_IO('F', 301)
#define SET_COLOR_KEY_ALPHA_START 	_IO('F', 302)
#define SET_COLOR_KEY_ALPHA_STOP 	_IO('F', 303)
#define SET_COLOR_KEY_INFO		_IOW('F', 304, s3c_color_key_info_t)
#define SET_COLOR_KEY_COLVAL		_IOW('F', 305, s3c_color_val_info_t)

#if defined(CONFIG_FB_DOUBLE_BUFFERING)
#define GET_FB_NUM			_IOWR('F', 306, u_int)
#endif


int s3c_fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	struct s3c_fb_info *fbi = container_of(info, struct s3c_fb_info, fb);
	struct fb_var_screeninfo *var= &fbi->fb.var;

	int brightness;
	s3c_win_info_t win_info_from_app;
	s3c_color_key_info_t colkey_info;
	s3c_color_val_info_t colval_info;
	
#if defined(CONFIG_FB_DOUBLE_BUFFERING)
	u_int f_num_val;
#endif

#if defined(CONFIG_FB_VIRTUAL_SCREEN)
	vs_info_t vs_info_from_app;
#endif
	
	switch(cmd){
#if defined(CONFIG_FB_VIRTUAL_SCREEN)
		case SET_VS_START:
				mach_info.wincon0 &= ~(S3C_WINCONx_ENWIN_F_ENABLE);
				__raw_writel(mach_info.wincon0|S3C_WINCONx_ENWIN_F_ENABLE, S3C_WINCON0);  

				fbi->fb.var.xoffset = mach_info.xoffset;
				fbi->fb.var.yoffset = mach_info.yoffset;
				break;

		case SET_VS_STOP:
				break;

		case SET_VS_INFO:
				if(copy_from_user(&vs_info_from_app, (vs_info_t *) arg, sizeof(vs_info_t)))
					return -EFAULT;
				
				if(set_vs_info(vs_info_from_app)){
					printk("Error SET_VS_INFO\n");
					return -EINVAL;
				}
				
				set_virtual_display_register(SET_VIRTUAL_LCD);

				fbi->fb.var.xoffset = mach_info.xoffset;
				fbi->fb.var.yoffset = mach_info.yoffset;
				break;

		case SET_VS_MOVE:
				set_virtual_display_register(arg);

				fbi->fb.var.xoffset = mach_info.xoffset;
				fbi->fb.var.yoffset = mach_info.yoffset;
				break;
#endif

		case SET_OSD_INFO :
				if(copy_from_user
				 (&win_info_from_app, (s3c_win_info_t *) arg, 
				   sizeof(s3c_win_info_t)))
					return -EFAULT;
				
				s3c_fb_init_win(fbi, win_info_from_app.Bpp, 
				win_info_from_app.LeftTop_x, win_info_from_app.LeftTop_y, win_info_from_app.Width, win_info_from_app.Height, OFF);				
				break; 
			
		case SET_OSD_START :
				s3c_fb_win_onoff(fbi, ON); // on
				break;

		case SET_OSD_STOP:
				s3c_fb_win_onoff(fbi, OFF); // off
				break;

		case SET_OSD_ALPHA_UP:
				if(osd_alpha_level < MAX_ALPHA_LEVEL) osd_alpha_level ++;
				s3c_fb_set_alpha_level(fbi);
				break;

		case SET_OSD_ALPHA_DOWN:
				if(osd_alpha_level > 0) osd_alpha_level --;
				s3c_fb_set_alpha_level(fbi);
				break;

		case SET_OSD_MOVE_LEFT:
				if(var->xoffset>0) var->xoffset--;
				s3c_fb_set_position_win(fbi, var->xoffset, var->yoffset, var->xres, var->yres);
				break;

		case SET_OSD_MOVE_RIGHT:
				if(var->xoffset < (H_RESOLUTION - var->xres)) var->xoffset ++;
				s3c_fb_set_position_win(fbi, var->xoffset, var->yoffset, var->xres, var->yres);
				break;

		case SET_OSD_MOVE_UP:
				if(var->yoffset>0) var->yoffset--;
				s3c_fb_set_position_win(fbi, var->xoffset, var->yoffset, var->xres, var->yres);
				break;

		case SET_OSD_MOVE_DOWN:
				if(var->yoffset < (V_RESOLUTION - var->yres)) var->yoffset ++;
				s3c_fb_set_position_win(fbi, var->xoffset, var->yoffset, var->xres, var->yres);
				break;

#if defined(CONFIG_FB_DOUBLE_BUFFERING)
		case GET_FB_NUM:
				if(copy_from_user((void *)&f_num_val, (const void *)arg, sizeof(u_int)))
					return -EFAULT;
				
				if(copy_to_user((void *)arg, (const void *) &f_num_val, sizeof(u_int)))
					return -EFAULT;
				
				break;
#endif

		case SET_DISPLAY_BRIGHTNESS:
				if(copy_from_user(&brightness, (int *) arg, sizeof(int)))
					return -EFAULT;
				#if defined(CONFIG_S3C6400_PWM)
				set_brightness(brightness);
				#endif
				break;
				
		case GET_DISPLAY_BRIGHTNESS:
				if(copy_to_user((void *)arg, (const void *) &display_brightness, sizeof(int)))
					return -EFAULT;				
				break;
				
		//(WAITFORVSYNC)
    		case FBIO_WAITFORVSYNC:
  			{
  				u_int32_t crt;

  				if (get_user(crt, (u_int32_t __user *)arg))
  					return -EFAULT;

  				return s3cfb_wait_for_sync(crt);
  			}
    		//(WAITFORVSYNC)	

		case SET_COLOR_KEY_START:
				s3c_fb_color_key_onoff(fbi, ON);
				break;
				
		case SET_COLOR_KEY_STOP:
				s3c_fb_color_key_onoff(fbi, OFF);
				break;
				
		case SET_COLOR_KEY_ALPHA_START:
				s3c_fb_color_key_alpha_onoff(fbi, ON);
				break;
				
		case SET_COLOR_KEY_ALPHA_STOP:
				s3c_fb_color_key_alpha_onoff(fbi, OFF);
				break;
				
		case SET_COLOR_KEY_INFO:
				if(copy_from_user(&colkey_info, (s3c_color_key_info_t *) arg, 
				   sizeof(s3c_color_key_info_t)))
					return -EFAULT;
				
				s3c_fb_setup_color_key_register(fbi, colkey_info);
				break;

		case SET_COLOR_KEY_COLVAL:
				if(copy_from_user(&colval_info, (s3c_color_val_info_t *) arg, 
				   sizeof(s3c_color_val_info_t)))
					return -EFAULT;
				
				s3c_fb_set_color_value(fbi, colval_info);
				break;

		default:
			return -EINVAL;
	}
	
	return 0;
}


/*
 * s3c_fb_map_video_memory():
 *	Allocates the DRAM memory for the frame buffer.  This buffer is
 *	remapped into a non-cached, non-buffered, memory region to
 *	allow palette and pixel writes to occur without flushing the
 *	cache.  Once this area is remapped, all virtual memory
 *	access to the video memory should occur at the new region.
 */
 int __init s3c_fb_map_video_memory(struct s3c_fb_info *fbi)
{
	dprintk("map_video_memory(fbi=%p)\n", fbi);

	fbi->map_size_f1 = PAGE_ALIGN(fbi->fb.fix.smem_len + PAGE_SIZE);
	fbi->map_cpu_f1 = dma_alloc_writecombine(fbi->dev, fbi->map_size_f1,
					      &fbi->map_dma_f1, GFP_KERNEL);

	fbi->map_size_f1 = fbi->fb.fix.smem_len;

	if (fbi->map_cpu_f1) {
		/* prevent initial garbage on screen */
		printk("Window[%d]- FB1 : map_video_memory: clear %p:%08x\n",
			fbi->win_id, fbi->map_cpu_f1, fbi->map_size_f1);
		memset(fbi->map_cpu_f1, 0xf0, fbi->map_size_f1);

		fbi->screen_dma_f1 = fbi->map_dma_f1;
		fbi->fb.screen_base = fbi->map_cpu_f1;
		fbi->fb.fix.smem_start = fbi->screen_dma_f1;

		printk("           FB1 : map_video_memory: dma=%08x cpu=%p size=%08x\n",
			fbi->map_dma_f1, fbi->map_cpu_f1, fbi->fb.fix.smem_len);
	}
	if( !fbi->map_cpu_f1)
		return -ENOMEM;


//#if !defined(CONFIG_FB_VIRTUAL_SCREEN) && defined(CONFIG_FB_DOUBLE_BUFFERING)	
#if  defined(CONFIG_FB_DOUBLE_BUFFERING)
	#if defined(CONFIG_CPU_S3C2443)	
	if(fbi->win_id<1){  // WIN0 support double-buffer 
	
	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	if(fbi->win_id<2){  // WIN0, WIN1 support double-buffer 
	#endif
	
	fbi->map_size_f2 = PAGE_ALIGN(fbi->fb.fix.smem_len + PAGE_SIZE);
	fbi->map_cpu_f2 = dma_alloc_writecombine(fbi->dev, fbi->map_size_f2,
					      &fbi->map_dma_f2, GFP_KERNEL);

	fbi->map_size_f2 = fbi->fb.fix.smem_len;

	if (fbi->map_cpu_f2) {
		/* prevent initial garbage on screen */
		printk("Window[%d] - FB2 : map_video_memory: clear %p:%08x\n",
			fbi->win_id, fbi->map_cpu_f2, fbi->map_size_f2);
		memset(fbi->map_cpu_f2, 0xf0, fbi->map_size_f2);

		fbi->screen_dma_f2 = fbi->map_dma_f2;

		printk("            FB2 : map_video_memory: dma=%08x cpu=%p size=%08x\n",
			fbi->map_dma_f2, fbi->map_cpu_f2, fbi->fb.fix.smem_len);
	}
	if( !fbi->map_cpu_f2)
		return -ENOMEM;
	}
#endif

	return 0;
}



void s3c_fb_unmap_video_memory(struct s3c_fb_info *fbi)
{
	dma_free_writecombine(fbi->dev, fbi->map_size_f1, fbi->map_cpu_f1,  fbi->map_dma_f1);

//#if !defined(CONFIG_FB_VIRTUAL_SCREEN) && defined(CONFIG_FB_DOUBLE_BUFFERING)
#if  defined(CONFIG_FB_DOUBLE_BUFFERING)
	dma_free_writecombine(fbi->dev, fbi->map_size_f2, fbi->map_cpu_f2,  fbi->map_dma_f2);
#endif
}



/*
 * s3c_fb_init_registers - Initialise all LCD-related registers
 */
int s3c_fb_init_registers(struct s3c_fb_info *fbi)
{
	u_long flags = 0;
	u_long PageWidth = 0, Offset = 0;
	int win_num =  fbi->win_id;
		
	struct clk *lcd_clock;
	struct fb_var_screeninfo *var= &fbi->fb.var;

	unsigned long VideoPhysicalTemp_f1 = fbi->screen_dma_f1;
	unsigned long VideoPhysicalTemp_f2 = fbi->screen_dma_f2;

	/* Initialise LCD with values from hare */

	local_irq_save(flags);

	if(win_num==0){ 	
		mach_info.vidcon0 = mach_info.vidcon0 & ~(S3C_VIDCON0_ENVID_ENABLE | S3C_VIDCON0_ENVID_F_ENABLE);
		__raw_writel(mach_info.vidcon0, S3C_VIDCON0);

		lcd_clock = clk_get(NULL, "lcd");
		mach_info.vidcon0 |=  S3C_VIDCON0_CLKVAL_F((int)((clk_get_rate(lcd_clock) / LCD_PIXEL_CLOCK) - 1));
 	}

        /* For buffer start address */
	__raw_writel(VideoPhysicalTemp_f1, S3C_VIDW00ADD0B0+(0x08*win_num));
	#if defined(CONFIG_CPU_S3C2443)
	if(win_num==0) __raw_writel(VideoPhysicalTemp_f2, S3C_VIDW00ADD0B1);
	
	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	if(win_num<2) __raw_writel(VideoPhysicalTemp_f2, S3C_VIDW00ADD0B1+(0x08*win_num));	
	
	#endif
	
	#if defined(CONFIG_FB_VIRTUAL_SCREEN)
	if(win_num==0){
		mach_info.vidw00add0b0=VideoPhysicalTemp_f1;
		mach_info.vidw00add0b1=VideoPhysicalTemp_f2;
	}
	#endif
	
	PageWidth = var->xres * mach_info.bytes_per_pixel;
	Offset = (var->xres_virtual - var->xres) * mach_info.bytes_per_pixel;
	#if defined(CONFIG_FB_VIRTUAL_SCREEN)
	if(win_num==0) Offset=0;
	#endif
	
	/* End address */
	__raw_writel(S3C_VIDWxxADD1_VBASEL_F((unsigned long) VideoPhysicalTemp_f1 + (PageWidth + Offset) * (var->yres)), 
				S3C_VIDW00ADD1B0+(0x08*win_num));
	#if defined(CONFIG_CPU_S3C2443)
	if(win_num==0) 
		__raw_writel(S3C_VIDWxxADD1_VBASEL_F((unsigned long) VideoPhysicalTemp_f2 + (PageWidth + Offset) * (var->yres)), 
				S3C_VIDW00ADD1B1);	

	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	if(win_num<2) 
		__raw_writel(S3C_VIDWxxADD1_VBASEL_F((unsigned long) VideoPhysicalTemp_f2 + (PageWidth + Offset) * (var->yres)), 
				S3C_VIDW00ADD1B1+(0x08*win_num));	
	#endif
	
	#if defined(CONFIG_FB_VIRTUAL_SCREEN)
	if(win_num==0){
		mach_info.vidw00add1b0=S3C_VIDWxxADD1_VBASEL_F((unsigned long) VideoPhysicalTemp_f1 + (PageWidth + Offset) * (var->yres));
		mach_info.vidw00add1b1=S3C_VIDWxxADD1_VBASEL_F((unsigned long) VideoPhysicalTemp_f2 + (PageWidth + Offset) * (var->yres));
	}
	#endif
	
	/* size of buffer */
	#if defined(CONFIG_CPU_S3C2443)
	__raw_writel(S3C_VIDWxxADD2_OFFSIZE_F(Offset) | (S3C_VIDWxxADD2_PAGEWIDTH_F(PageWidth)), S3C_VIDW00ADD2B0+(0x08*win_num));
	if(win_num==0) __raw_writel(S3C_VIDWxxADD2_OFFSIZE_F(Offset) | (S3C_VIDWxxADD2_PAGEWIDTH_F(PageWidth)), S3C_VIDW00ADD2B1);

	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	__raw_writel(S3C_VIDWxxADD2_OFFSIZE_F(Offset) | (S3C_VIDWxxADD2_PAGEWIDTH_F(PageWidth)), S3C_VIDW00ADD2+(0x04*win_num));

	#endif
	
	switch(win_num){
		case 0:
			__raw_writel(mach_info.wincon0, S3C_WINCON0);
			__raw_writel(mach_info.vidcon0, S3C_VIDCON0);
			__raw_writel(mach_info.vidcon1, S3C_VIDCON1);
			__raw_writel(mach_info.vidtcon0, S3C_VIDTCON0);
			__raw_writel(mach_info.vidtcon1, S3C_VIDTCON1);
			__raw_writel(mach_info.vidtcon2, S3C_VIDTCON2);
	
			#if defined(CONFIG_CPU_S3C2443)
			__raw_writel(mach_info.vidintcon, S3C_VIDINTCON);
			#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
			__raw_writel(mach_info.dithmode, S3C_DITHMODE);
			__raw_writel(mach_info.vidintcon0, S3C_VIDINTCON0);
			__raw_writel(mach_info.vidintcon1, S3C_VIDINTCON1);
			#endif
			
			__raw_writel(mach_info.vidosd0a, S3C_VIDOSD0A);
			__raw_writel(mach_info.vidosd0b, S3C_VIDOSD0B);
			
			#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
			__raw_writel(mach_info.vidosd0c, S3C_VIDOSD0C);
			#endif
			
			__raw_writel(mach_info.wpalcon, S3C_WPALCON);
			s3c_fb_win_onoff(fbi, ON);
			break;

		case 1:
			__raw_writel(mach_info.wincon1, S3C_WINCON1);
			__raw_writel(mach_info.vidosd1a, S3C_VIDOSD1A);
			__raw_writel(mach_info.vidosd1b, S3C_VIDOSD1B);
			__raw_writel(mach_info.vidosd1c, S3C_VIDOSD1C);
			
			#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
			__raw_writel(mach_info.vidosd1d, S3C_VIDOSD1D);
			#endif
			
			__raw_writel(mach_info.wpalcon, S3C_WPALCON);
			s3c_fb_win_onoff(fbi, OFF);
			break;

		#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
		case 2:
			__raw_writel(mach_info.wincon2, S3C_WINCON2);
			__raw_writel(mach_info.vidosd2a, S3C_VIDOSD2A);
			__raw_writel(mach_info.vidosd2b, S3C_VIDOSD2B);
			__raw_writel(mach_info.vidosd2c, S3C_VIDOSD2C);
			__raw_writel(mach_info.vidosd2d, S3C_VIDOSD2D);
			__raw_writel(mach_info.wpalcon, S3C_WPALCON);
			s3c_fb_win_onoff(fbi, OFF);
			break; 

		case 3:
			__raw_writel(mach_info.wincon3, S3C_WINCON3);
			__raw_writel(mach_info.vidosd3a, S3C_VIDOSD3A);
			__raw_writel(mach_info.vidosd3b, S3C_VIDOSD3B);
			__raw_writel(mach_info.vidosd3c, S3C_VIDOSD3C);
			__raw_writel(mach_info.wpalcon, S3C_WPALCON);
			s3c_fb_win_onoff(fbi, OFF);
			break;

		case 4:
			__raw_writel(mach_info.wincon4, S3C_WINCON4);
			__raw_writel(mach_info.vidosd4a, S3C_VIDOSD4A);
			__raw_writel(mach_info.vidosd4b, S3C_VIDOSD4B);
			__raw_writel(mach_info.vidosd4c, S3C_VIDOSD4C);
			__raw_writel(mach_info.wpalcon, S3C_WPALCON);
			s3c_fb_win_onoff(fbi, OFF);
			break;
		#endif
	}

	local_irq_restore(flags);

	return 0;
 }
 
/* s3c_fb_set_lcdaddr
 *
 * initialise lcd controller address pointers
 */
 void s3c_fb_set_lcdaddr(struct s3c_fb_info *fbi)
{
	u32 start;
	unsigned long VideoPhysicalTemp_f1 = fbi->screen_dma_f1;
	
	start = fbi->fb.fix.line_length * fbi->fb.var.yoffset;
	
        /* For buffer start address */
	mach_info.vidw00add0b0 = VideoPhysicalTemp_f1+start;
        mach_info.vidw00add1b0 = mach_info.vidw00add0b0+(fbi->fb.fix.line_length*fbi->fb.var.yres);

	__raw_writel(mach_info.vidw00add0b0, S3C_VIDW00ADD0B0);
	__raw_writel(mach_info.vidw00add1b0, S3C_VIDW00ADD1B0);

}


void s3c_fb_set_fb_change(int req_fb) {

	info[req_fb].fb_change_ready = 0;
	
	#if defined(CONFIG_CPU_S3C2443) && defined(CONFIG_FB_DOUBLE_BUFFERING)	
		// Software-based trigger
		__raw_writel((1<<0), S3C_CPUTRIGCON2);
	#elif (defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)) && defined(CONFIG_FB_DOUBLE_BUFFERING)
		// Software-based trigger
		__raw_writel((3<<0), S3C_TRIGCON);
	#endif // #if defined(CONFIG_CPU_S3C2443) 
}


/* s3c_fb_activate_var
 *
 * activate (set) the controller from the given framebuffer
 * information
 */
void s3c_fb_activate_var(struct s3c_fb_info *fbi,
				   struct fb_var_screeninfo *var)
{
	dprintk("%s: var->bpp   = %d\n", __FUNCTION__, var->bits_per_pixel);

	switch (var->bits_per_pixel) {
	case 8:
		mach_info.wincon0=  S3C_WINCONx_BYTSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_8BPP_PAL;  // 4word burst, 8bpp-palletized,
		mach_info.wincon1=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 16bpp for OSD

		#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
		mach_info.wincon2=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 16bpp for OSD
		mach_info.wincon3=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 16bpp for OSD
		mach_info.wincon4=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 16bpp for OSD
		#endif
		
		mach_info.bpp=		PIXEL_BPP8;
		mach_info.bytes_per_pixel= 1;

		#if defined(CONFIG_CPU_S3C2443)		
		mach_info.wpalcon=	S3C_WPALCON_W0PAL_24BIT;
		#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
		mach_info.wpalcon=	S3C_WPALCON_W0PAL_16BIT; 
		#endif
		break;
	case 16:
		mach_info.wincon0=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565;  // 4word burst, 16bpp,
		mach_info.wincon1=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  //
		#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
		mach_info.wincon2=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  //
		mach_info.wincon3=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  //
		mach_info.wincon4=  S3C_WINCONx_HAWSWP_ENABLE | S3C_WINCONx_BURSTLEN_4WORD |  S3C_WINCONx_BPPMODE_F_16BPP_565 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  //
		#endif

		mach_info.bpp=		PIXEL_BPP16;
		mach_info.bytes_per_pixel= 2;
		break;
	case 24:
		mach_info.wincon0=  S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888;  // 4word burst, 24bpp,,
		mach_info.wincon1= S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 24bpp for OSD

		#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
		mach_info.wincon2= S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 24bpp for OSD
		mach_info.wincon3= S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 24bpp for OSD
		mach_info.wincon4= S3C_WINCONx_HAWSWP_DISABLE | S3C_WINCONx_BURSTLEN_16WORD |  S3C_WINCONx_BPPMODE_F_24BPP_888 | S3C_WINCONx_BLD_PIX_PLANE | S3C_WINCONx_ALPHA_SEL_1;  // 4word burst, 24bpp for OSD
		#endif
		
        	mach_info.bpp=		PIXEL_BPP24;
		mach_info.bytes_per_pixel= 4;
		break;

	case 32:
		mach_info.bytes_per_pixel= 4;
		break;
	}

	/* write new registers */
	__raw_writel(mach_info.wincon0, S3C_WINCON0);
	__raw_writel(mach_info.wincon1, S3C_WINCON1);
	
	#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	__raw_writel(mach_info.wincon2, S3C_WINCON2);
	__raw_writel(mach_info.wincon3, S3C_WINCON3);
	__raw_writel(mach_info.wincon4, S3C_WINCON4);
	#endif
	
	__raw_writel(mach_info.wpalcon, S3C_WPALCON);

#if 0	
	/* set lcd address pointers */
	s3c_fb_set_lcdaddr(fbi);
#endif

	//__raw_writel(mach_info.wincon0|S3C_WINCONx_ENWIN_F_ENABLE|S3C_WINCONx_BUFAUTOEN_ENABLE, S3C_WINCON0);   /* Double buffer auto enable bit */
	__raw_writel(mach_info.wincon0|S3C_WINCONx_ENWIN_F_ENABLE, S3C_WINCON0);   
	__raw_writel(mach_info.vidcon0|S3C_VIDCON0_ENVID_ENABLE|S3C_VIDCON0_ENVID_F_ENABLE, S3C_VIDCON0);

}


int s3c_fb_init_win (struct s3c_fb_info *fbi, int Bpp, int LeftTop_x, int LeftTop_y, int Width, int Height, int OnOff)
{
	s3c_fb_win_onoff(fbi, OFF); // off
	s3c_fb_set_bpp(fbi, Bpp); 
	s3c_fb_set_position_win(fbi, LeftTop_x, LeftTop_y, Width, Height);
	s3c_fb_set_size_win(fbi, Width, Height);
	s3c_fb_set_memory_size_win(fbi);
	s3c_fb_win_onoff(fbi, OnOff); // off

	return 0;
}


int s3c_fb_win_onoff(struct s3c_fb_info *fbi, int On)
{
	int win_num =  fbi->win_id;

	if(On)
		__raw_writel(__raw_readl(S3C_WINCON0+(0x04*win_num))|S3C_WINCONx_ENWIN_F_ENABLE, 
					S3C_WINCON0+(0x04*win_num));   // ON
	else
		__raw_writel(__raw_readl(S3C_WINCON0+(0x04*win_num))&~(S3C_WINCONx_ENWIN_F_ENABLE), 
					S3C_WINCON0+(0x04*win_num));   // OFF

	return 0;
}


int s3c_fb_set_alpha_level(struct s3c_fb_info *fbi)
{
	unsigned long alpha_val;

	int win_num =  fbi->win_id;
	
	if(win_num==0){ 
		printk("WIN0 do not support alpha blending.\n");
		return -1;
	}

	alpha_val = S3C_VIDOSDxC_ALPHA1_B(osd_alpha_level) | S3C_VIDOSDxC_ALPHA1_G(osd_alpha_level) |S3C_VIDOSDxC_ALPHA1_R(osd_alpha_level);

	#if defined(CONFIG_CPU_S3C2443)
	__raw_writel(alpha_val, S3C_VIDOSD1C);

	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	__raw_writel(alpha_val, S3C_VIDOSD0C+(0x10*win_num));
	#endif

	return 0;

}


int s3c_fb_set_alpha_mode(struct s3c_fb_info *fbi, int Alpha_mode, int Alpha_level)
{
	unsigned long alpha_val;

	int win_num =  fbi->win_id;
	
	if(win_num==0){ 
		printk("WIN0 do not support alpha blending.\n");
		return -1;
	}

	switch(Alpha_mode){
		case 0: // Plane Blending
			__raw_writel(__raw_readl(S3C_WINCON0+(0x04*win_num))|S3C_WINCONx_BLD_PIX_PLANE, 
					S3C_WINCON0+(0x04*win_num));  
			break;

		case 1: // Pixel Blending & chroma(color) key
			__raw_writel(__raw_readl(S3C_WINCON0+(0x04*win_num))|S3C_WINCONx_BLD_PIX_PIXEL|S3C_WINCONx_ALPHA_SEL_0, 
					S3C_WINCON0+(0x04*win_num));   
			break;
	}
					
	osd_alpha_level = Alpha_level;
	alpha_val = S3C_VIDOSDxC_ALPHA1_B(osd_alpha_level) | S3C_VIDOSDxC_ALPHA1_G(osd_alpha_level) |S3C_VIDOSDxC_ALPHA1_R(osd_alpha_level);

#if defined(CONFIG_CPU_S3C2443)
	__raw_writel(alpha_val, S3C_VIDOSD1C);

#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	__raw_writel(alpha_val, S3C_VIDOSD0C+(0x10*win_num));
#endif
	return 0;

}

/*------------------------- Set up color key ------------------------------*/

int s3c_fb_color_key_alpha_onoff(struct s3c_fb_info *fbi, int On)
{
	int win_num =  fbi->win_id;
	win_num--;
	
	if(On)
		__raw_writel(__raw_readl(S3C_W1KEYCON0+(0x08*win_num))|S3C_WxKEYCON0_KEYBLEN_ENABLE, 
					S3C_W1KEYCON0+(0x08*win_num));   // ON
	else
		__raw_writel(__raw_readl(S3C_W1KEYCON0+(0x08*win_num))&~(S3C_WxKEYCON0_KEYBLEN_ENABLE), 
					S3C_W1KEYCON0+(0x08*win_num));   // OFF
	return 0;
}

int s3c_fb_color_key_onoff(struct s3c_fb_info *fbi, int On)
{
	int win_num =  fbi->win_id;
	win_num--;

	if(On)
		__raw_writel(__raw_readl(S3C_W1KEYCON0+(0x08*win_num))|S3C_WxKEYCON0_KEYEN_F_ENABLE, 
					S3C_W1KEYCON0+(0x08*win_num));   // ON
	else
		__raw_writel(__raw_readl(S3C_W1KEYCON0+(0x08*win_num))&~(S3C_WxKEYCON0_KEYEN_F_ENABLE), 
					S3C_W1KEYCON0+(0x08*win_num));   // OFF
	return 0;
}


int s3c_fb_setup_color_key_register(struct s3c_fb_info *fbi, s3c_color_key_info_t colkey_info) 
{
	unsigned int compkey = 0;

	int win_num =  fbi->win_id;
	
	if(win_num==0){ 
		printk("WIN0 do not support color-key.\n");
		return -1;
	}

	win_num--;

	if(fbi->fb.var.bits_per_pixel == PIXEL_BPP16) {
		/* RGB 5-6-5 mode */
		compkey  = (((colkey_info.compkey_red & 0x1F) << 19) | 0x70000);
		compkey |= (((colkey_info.compkey_green & 0x3F) << 10) | 0x300);
		compkey |= (((colkey_info.compkey_blue  & 0x1F)  << 3 )| 0x7);
	}
	else if (fbi->fb.var.bits_per_pixel == PIXEL_BPP24) {
		/* currently RGB 8-8-8 mode  */
		compkey  = ((colkey_info.compkey_red & 0xFF) << 16);
		compkey |= ((colkey_info.compkey_green & 0xFF) << 8);
		compkey |= ((colkey_info.compkey_blue & 0xFF) << 0);
	}
	else 
		printk("Invalid BPP has been given !\n");
	
			
	if(colkey_info.direction == COLOR_KEY_DIR_BG_DISPLAY)
		__raw_writel(S3C_WxKEYCON0_COMPKEY(compkey) | S3C_WxKEYCON0_DIRCON_MATCH_FG_IMAGE, S3C_W1KEYCON0+(0x08*win_num));

	else if (colkey_info.direction == COLOR_KEY_DIR_FG_DISPLAY)
		__raw_writel(S3C_WxKEYCON0_COMPKEY(compkey) | S3C_WxKEYCON0_DIRCON_MATCH_BG_IMAGE, S3C_W1KEYCON0+(0x08*win_num));
	else
		printk("Color key direction is not correct :: %d!!!\n", colkey_info.direction);
	return 0;
	
}

	
int s3c_fb_set_color_value(struct s3c_fb_info *fbi, s3c_color_val_info_t colval_info)
{
	unsigned int colval = 0;

	int win_num =  fbi->win_id;
	
	if(win_num==0){ 
		printk("WIN0 do not support color-key value.\n");
		return -1;
	}

	win_num--;
	
	if(fbi->fb.var.bits_per_pixel == PIXEL_BPP16) {
		/* RGB 5-6-5 mode */
		colval  = (((colval_info.colval_red   & 0x1F) << 19) | 0x70000);
		colval |= (((colval_info.colval_green & 0x3F) << 10) | 0x300);
		colval |= (((colval_info.colval_blue  & 0x1F)  << 3 )| 0x7);
	}
	else if (fbi->fb.var.bits_per_pixel == PIXEL_BPP24) {
		/* currently RGB 8-8-8 mode  */
		colval  = ((colval_info.colval_red  & 0xFF) << 16);
		colval |= ((colval_info.colval_green & 0xFF) << 8);
		colval |= ((colval_info.colval_blue  & 0xFF) << 0);
	}
	else 
		printk("Invalid BPP has been given !\n");
	
	__raw_writel(S3C_WxKEYCON1_COLVAL(colval), S3C_W1KEYCON1+(0x08*win_num));

	return 0;
}


/*--------------------------------------------------------------------------*/

int s3c_fb_set_position_win(struct s3c_fb_info *fbi, int LeftTop_x, int LeftTop_y, int Width, int Height)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num =  fbi->win_id;
#if 0
	if(win_num==0){ 
		printk("WIN0 do not support window position control.\n");
		return -1;
	}
#endif

#if defined(CONFIG_CPU_S3C2443)
	__raw_writel(S3C_VIDOSDxA_OSD_LTX_F(LeftTop_x) | S3C_VIDOSDxA_OSD_LTY_F(LeftTop_y), 
				S3C_VIDOSD0A+(0x0c*win_num));
	__raw_writel(S3C_VIDOSDxB_OSD_RBX_F(Width-1 + LeftTop_x) | S3C_VIDOSDxB_OSD_RBY_F(Height-1 + LeftTop_y), 
				S3C_VIDOSD0B+(0x0c*win_num));

#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	__raw_writel(S3C_VIDOSDxA_OSD_LTX_F(LeftTop_x) | S3C_VIDOSDxA_OSD_LTY_F(LeftTop_y), 
				S3C_VIDOSD0A+(0x10*win_num));
	__raw_writel(S3C_VIDOSDxB_OSD_RBX_F(Width-1 + LeftTop_x) | S3C_VIDOSDxB_OSD_RBY_F(Height-1 + LeftTop_y), 
				S3C_VIDOSD0B+(0x10*win_num));
#endif

	var->xoffset=LeftTop_x;
	var->yoffset=LeftTop_y;

	return 0;
}


int s3c_fb_set_size_win(struct s3c_fb_info *fbi, int Width, int Height)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num =  fbi->win_id;

#if 0
	if(win_num==0){ 
		printk("WIN0 do not support window size control.\n");
		return -1;
	}
#endif
	#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	if(win_num==1) 
		__raw_writel(S3C_VIDOSD0C_OSDSIZE(Width*Height),S3C_VIDOSD1D);

	else if(win_num==2)
		__raw_writel(S3C_VIDOSD0C_OSDSIZE(Width*Height),S3C_VIDOSD2D);
	#endif
	
	var->xres = Width;
	var->yres = Height;
	var->xres_virtual = Width;
	var->yres_virtual= Height;

	return 0;
}


int s3c_fb_set_bpp(struct s3c_fb_info *fbi, int Bpp)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num =  fbi->win_id;

	switch(Bpp){
		case 1:
		case 2:
		case 4:	 
		case 8:
			// What should I do???   
			break;
			
		case 16:
			__raw_writel(__raw_readl(S3C_WINCON0+(0x04*win_num))|S3C_WINCONx_BPPMODE_F_16BPP_565, 
					S3C_WINCON0+(0x04*win_num));   
			var->bits_per_pixel=16;
			break;
			
		case 24:
			__raw_writel(__raw_readl(S3C_WINCON0+(0x04*win_num))|S3C_WINCONx_BPPMODE_F_24BPP_888, 
					S3C_WINCON0+(0x04*win_num));   
			var->bits_per_pixel=24;
			break;

		case 32:
			var->bits_per_pixel=32;
			break;
	}
	
	return 0;
}


int s3c_fb_set_memory_size_win(struct s3c_fb_info *fbi)
{
	struct fb_var_screeninfo *var= &fbi->fb.var;
	int win_num =  fbi->win_id;

	unsigned long Offset = 0;
	unsigned long PageWidth = 0;
	unsigned long FrameBufferSize = 0;

	PageWidth = var->xres * mach_info.bytes_per_pixel;
	Offset = (var->xres_virtual - var->xres) * mach_info.bytes_per_pixel;
	#if defined(CONFIG_FB_VIRTUAL_SCREEN)
	if(win_num==0) Offset=0;
	#endif

	__raw_writel(S3C_VIDWxxADD1_VBASEL_F((unsigned long)__raw_readl(S3C_VIDW00ADD0B0+(0x08*win_num)) + (PageWidth + Offset) * (var->yres)), 
				S3C_VIDW00ADD1B0+(0x08*win_num));
	if(win_num==1) 
		__raw_writel(S3C_VIDWxxADD1_VBASEL_F((unsigned long)__raw_readl(S3C_VIDW00ADD0B1+(0x08*win_num)) + (PageWidth + Offset) * (var->yres)), 
				S3C_VIDW00ADD1B1+(0x08*win_num));	

	/* size of frame buffer */
	FrameBufferSize = S3C_VIDWxxADD2_OFFSIZE_F(Offset) | (S3C_VIDWxxADD2_PAGEWIDTH_F(PageWidth));
	#if defined(CONFIG_CPU_S3C2443)
	__raw_writel(FrameBufferSize, S3C_VIDW00ADD2B0+(0x08*win_num));

	if(win_num==0)
		__raw_writel(FrameBufferSize, S3C_VIDW00ADD2B1);

	#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	__raw_writel(FrameBufferSize, S3C_VIDW00ADD2+(0x04*win_num));
	#endif
	return 0;

}


int s3c_fb_set_out_path(struct s3c_fb_info *fbi, int Path)
{
#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	switch(Path){
		case 0: // RGB I/F 
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_RGB_IF | S3C_VIDCON0_PNRMODE_RGB_P, S3C_VIDCON0); 
			break;

		case 1: // TV Encoder T/F
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_TV, S3C_VIDCON0); 
			break;
			
		case 2: // Indirect I80 I/F-0
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_I80IF0, S3C_VIDCON0); 
			break;
			
		case 3: // Indirect I80 I/F-1
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_I80IF1, S3C_VIDCON0); 
			break;
			
		case 4: // TV Encoder & RGB I/F
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_TVNRGBIF, S3C_VIDCON0); 
			break;
			
		case 6: // TV Encoder &  Indirect I80 I/F-0
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_TVNI80IF0, S3C_VIDCON0); 
			break;
			
		case 7: // TV Encoder &  Indirect I80 I/F-1
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_TVNI80IF1, S3C_VIDCON0); 
			break;
			
		default: // RGB I/F 
			__raw_writel(__raw_readl(S3C_VIDCON0) | S3C_VIDCON0_VIDOUT_RGB_IF | S3C_VIDCON0_PNRMODE_RGB_P, S3C_VIDCON0); 
			break;
	}
#endif

	return 0;
}


#if defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
#define LCD_TV 1

// LCD display controller configuration functions
void s3c_fb_set_output_path(int out)
{
	u32 tmp;
	//printk("s3c_fb_set_output_path() called.\n");
	
	tmp = __raw_readl(S3C_VIDCON0);

	// if output mode is LCD mode, Scan mode always should be progressive mode
	if(out == LCD_TV) {
		tmp &=~(1<<29);
	}

	tmp &=~(0x7<<26);
	tmp |= (out<<26);
	__raw_writel(tmp, S3C_VIDCON0);
}

void s3c_fb_set_clkval(u32 clkval)
{
	u32 tmp;
	//printk("s3c_fb_set_clkval() called.\n");

	tmp = __raw_readl(S3C_VIDCON0);

	tmp &=~(0x1<<4);
	tmp &=~(0xff<<6);

	__raw_writel(tmp | (clkval<<6) | (1<<4), S3C_VIDCON0);
}

void s3c_fb_enable_rgbport(u32 on_off)
{
	//printk("s3c_fb_enable_rgbport() called.\n");
	if(on_off)	//enable
		__raw_writel(0x380, S3C_VIDCON2);
	else		//disable
		__raw_writel(0x0, S3C_VIDCON2);
}

EXPORT_SYMBOL(s3c_fb_set_output_path);
EXPORT_SYMBOL(s3c_fb_set_clkval);
EXPORT_SYMBOL(s3c_fb_enable_rgbport);
#endif


void SetLcdPort(void)
{
	unsigned long gpdat;
	unsigned long gpcon;

	unsigned long hclkcon; //, sclkcon;

#if defined(CONFIG_CPU_S3C2443)
       //Enable clock to LCD
	hclkcon = __raw_readl(S3C_HCLKCON);
	hclkcon |= S3C_HCLKCON_LCD;
	__raw_writel(hclkcon, S3C_HCLKCON);

	// To select TFT LCD type
	gpdat = __raw_readl(S3C_MISCCR);
	gpdat  |= (1<<28);
	__raw_writel(gpdat, S3C_MISCCR);

	__raw_writel(0xaaaaaaaa, S3C_GPCCON);  // CTRL,  VD[7:0]
	__raw_writel(0xaaaaaaaa, S3C_GPDCON);  // VD[23:8]

	/* LCD Backlight Enable control */
	gpcon = __raw_readl(S3C_GPBCON);
	gpcon  = (gpcon & ~(3<<6)) | (1<<6);   // Backlight(GPB3) Enable control
	__raw_writel(gpcon, S3C_GPBCON);

	/* LCD Backlight ON */
	gpdat = __raw_readl(S3C_GPBDAT);
	gpdat  |= (1<<3);
	__raw_writel(gpdat, S3C_GPBDAT);

	// LCD _nRESET control
	gpcon = __raw_readl(S3C_GPBCON);
	gpcon  = (gpcon & ~(3<<2)) |(1<<2);
	__raw_writel(gpcon, S3C_GPBCON);

	gpdat = __raw_readl(S3C_GPBDAT);
	gpdat  |= (1<<1);
	__raw_writel(gpdat, S3C_GPBDAT);

	// LCD module reset
	gpdat = __raw_readl(S3C_GPBDAT);
	gpdat  |= (1<<2);
	__raw_writel(gpdat, S3C_GPBDAT);

	gpdat = __raw_readl(S3C_GPBDAT);
	gpdat  &= ~(1<<2);   // goes to LOW
	__raw_writel(gpdat, S3C_GPBDAT);
	mdelay(10);

	gpdat = __raw_readl(S3C_GPBDAT);
	gpdat  |= (1<<2);   // goes to HIGH
	__raw_writel(gpdat, S3C_GPBDAT);

#elif defined(CONFIG_CPU_S3C6400) || defined(CONFIG_CPU_S3C6410)
	//Must be '0' for Normal-path instead of By-pass
	// (*(volatile unsigned *)0x7410800c)=0;
	__raw_writel(0x0, S3C_HOSTIFB_MIFPCON);

	//Enable clock to LCD
	hclkcon = __raw_readl(S3C_HCLK_GATE);
	hclkcon |= S3C_CLKCON_HCLK_LCD;
	__raw_writel(hclkcon, S3C_HCLK_GATE);

	// To select TFT LCD type (RGB I/F)
	gpdat = __raw_readl(S3C_SPCON);
	gpdat &= ~0x3;
	gpdat |= (1<<0);
	__raw_writel(gpdat, S3C_SPCON);

	__raw_writel(0xaaaaaaaa, S3C_GPICON);  // VD[15:0]
	__raw_writel(0xaaaaaaaa, S3C_GPJCON);  // VD[23:16], Ctrl

	/* PWM for back-light control */
	s3c_gpio_cfgpin(S3C_GPF15, 1);
	s3c_gpio_setpin(S3C_GPF15, 1);

	s3c_gpio_cfgpin(S3C_GPN5, 1);
	s3c_gpio_setpin(S3C_GPN5, 0);
	s3c_gpio_setpin(S3C_GPN5, 1);

	mdelay(100);

#if 0
	/* Dummy code : remove to LCD pwm control (ryu)*/
	/* LCD Backlight Enable control */
	gpcon = __raw_readl(S3C_GPFCON);
	gpcon  = (gpcon & ~(3<<30)) | (1<<30);   // Backlight(GPF15) Enable control
	__raw_writel(gpcon, S3C_GPFCON);

	/* LCD Backlight ON */
	gpdat = __raw_readl(S3C_GPFDAT);
	gpdat  |= (1<<15);
	__raw_writel(gpdat, S3C_GPFDAT);

#endif
	// LCD _nRESET control
	gpcon = __raw_readl(S3C_GPNCON);
	gpcon  = (gpcon & ~(3<<10)) |(1<<10);
	__raw_writel(gpcon, S3C_GPNCON);
	gpdat = __raw_readl(S3C_GPNDAT);
	gpdat  |= (0<<5);
	__raw_writel(gpdat, S3C_GPNDAT);
	mdelay(10);

	gpdat = __raw_readl(S3C_GPNDAT);
	gpdat  |= (1<<5);
	__raw_writel(gpdat, S3C_GPNDAT);
#endif

}

/****************************************************************************/
#define LCD_DELAY_1MS	18000

static const unsigned int lms480qc0_rgb_initialize[][3] =
{
	{0x0063,0,0},	// default vlaue // set driver output 100% , hareware pin stanby
	{0x0155,0,0},	// default vlaue // interlaced display mode
	{0x0203,0,0},	// vertical display position setting 0x3(-13H) backporch period setting. default 0x10
	{0x0311,0,0},	// horizontal display position setting 0x11(-111NCLK) default 0x80
	{0x0400,0,0},	// default vlaue
	{0x0511,0,0},	// default vlaue
	{0x0600,0,0},	// signal polarity setting ODD/EVEN(high), CK(falling edge), HD(Low), VD(Low)
	{0x0700,0,0},	// default vlaue
	{0x0844,0,0},	// default vlaue
	{0x0902,0,0},	// BSEQ(0x2) ODD/Even Line 1,4,2,5,3,6 ?
	{0x0a2a,0,0},	// Bank Non Overlap time BNO[5:0]=0x2a(+10NCLK)
	{0x0b28,0,0},	// bank switch turn on time // default 0x40
	{0x0c2a,0,0},	// Gate non-overlap setting // default 0x40
	{0x0d3e,0,0},	// Gate source ouput delay setting // default 0x20
	{0x0e20,0,0},	// default vlaue
	{0x0f20,0,0},	// default vlaue
	{0x10f9,0,0},	// default 0xf0
	{0x11f4,0,0},	// default 0xf4
	{0x1210,0,0},	// default vlaue
	{0x1320,0,0},	// default vlaue
	{0x1420,0,0},	// default vlaue
	{0x1520,0,0},	// default vlaue
	{0x1680,0,0},	// default vlaue
	{0x1780,0,0},	// default vlaue
	{0x1880,0,0},	// default vlaue
	{0x2086,0,0},
	{0x2106,0,0},
	{0x220b,0,0},
	{0x2316,0,0},
	{0x2458,0,0},
	{0x2594,0,0},
	{0x26cf,0,0},
	{0x2755,0,0},
	{0x28bc,0,0},
	{0x2918,0,0},
	{0x2a69,0,0},
	{0x2bfe,0,0},
	{0x2c54,0,0},
	{0x2d95,0,0},
	{0x2efe,0,0},
	{0x5086,0,0},
	{0x5106,0,0},
	{0x520b,0,0},
	{0x5316,0,0},
	{0x5458,0,0},
	{0x5594,0,0},
	{0x56cf,0,0},
	{0x5755,0,0},
	{0x58bc,0,0},
	{0x5918,0,0},
	{0x5a69,0,0},
	{0x5bfe,0,0},
	{0x5c54,0,0},
	{0x5d95,0,0},
	{0x5efe,0,0},
	{0x2f21,0,0},
	{0,0,0}
};

static void spi_init(void)
{
	spi_base = ioremap(S3C24XX_PA_SPI, S3C24XX_SZ_SPI);

	s3c_gpio_cfgpin(S3C_GPC4, 2);
	s3c_gpio_cfgpin(S3C_GPC5, 2);
	s3c_gpio_cfgpin(S3C_GPC6, 2);
	s3c_gpio_cfgpin(S3C_GPC7, 2);
	s3c_gpio_pullup(S3C_GPC4, 0);
	s3c_gpio_pullup(S3C_GPC5, 0);
	s3c_gpio_pullup(S3C_GPC6, 0);
	s3c_gpio_pullup(S3C_GPC7, 0);

	writel(SPI_CH_FORMAT_B | SPI_CH_FALLING, spi_base + S3C_CH_CFG);
	writel(SPI_ENCLK_ENABLE | 0xff, spi_base + S3C_CLK_CFG);
	writel(0x0, spi_base + S3C_MODE_CFG);	
	writel(0x1, spi_base + S3C_SLAVE_SEL);
	writel(0x0, spi_base + S3C_SPI_INT_EN);
}

static void spi_start(void)
{
	writel(readl(spi_base + S3C_CH_CFG) | 0x1, spi_base + S3C_CH_CFG);
}

static void spi_cscontrol(int cs)
{
	volatile u32 tmp;

	tmp = readl(spi_base + S3C_SLAVE_SEL);

	if (cs)
		tmp |= 0x1;
	else
		tmp &= ~0x1;

	writel(tmp, spi_base + S3C_SLAVE_SEL);
}

static void spi_delay_loop(u32 count) 
{ 
	u32 i;

	for(i = 0; i < count; i++);
}


static void spi_delay_1ms(int msec)
{
	volatile int i;

	for(i = 0; i < LCD_DELAY_1MS * msec; i++);
}

static void spi_write(u8 data1, u8 data2)
{
	spi_cscontrol(0);
	spi_delay_loop(0xffff);

	writeb(data1, spi_base + S3C_SPI_TX_DATA);
	writeb(data2, spi_base + S3C_SPI_TX_DATA);

	spi_delay_loop(0xffff);
	spi_cscontrol(1);
	spi_delay_loop(0xffff);
}

static void lti_command(u16 data)
{
	spi_write((u8) ((data >> 8) & 0xff), (u8) (data & 0xff));
}

void Init_LDI(void)
{
	int i;
	
	printk(KERN_INFO "LCD TYPE :: S3C_LMS480QC LCD will be initialized\n");

	SetLcdPort();

	/* init & start spi */
	spi_init();
	spi_start();

	i = 0;
	
	while (1) {
		lti_command(lms480qc0_rgb_initialize[i][0]);
		
		if (lms480qc0_rgb_initialize[i][2])
			spi_delay_1ms(lms480qc0_rgb_initialize[i][2]);

		i++;

		if ((lms480qc0_rgb_initialize[i][0] == 0) && (lms480qc0_rgb_initialize[i][1] == 0))
			break;
	}

	spi_delay_1ms(1);
	mdelay(100);
}

