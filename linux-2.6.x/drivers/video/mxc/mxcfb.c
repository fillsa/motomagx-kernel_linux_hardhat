/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2008 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Added support for the BGRA6666 pixel packing format, 
 *                    an emulated CLI on the main display, selective hardware
 *                    initialization for the power-up logo, removal of the
 *                    console cursor, and additional backlight and EzX
 *                    compatibility IOCTLs.
 * 11/2006  Motorola  Adjusted ArgonLV related defines.
 * 01/2007  Motorola  Added support for dynamic IPU pool config.
 * 08/2007  Motorola  remove unused mxcfb_suspend/resume definition
 * 09/2007  Motorola  Modified comments.
 * 11/2007  Motorola  remove display init calls in open/close.
 * 11/2007  Motorola  add function to set global variables in ipu sdc
 * 03/2008  Motorola  remove calls to power_ic lighting
 * 04/2008  Motorola  Modified comments.
 */

/*!
 * @defgroup Framebuffer Framebuffer Driver for SDC and ADC.
 */

/*
 * @file mxcfb.c
 *
 * @brief MXC Frame buffer driver for SDC
 *
 * @ingroup Framebuffer
 */

/*!
 * Include files
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fb.h>
#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)
#include <linux/motfb.h>    /* Motorola specific FB iotcl definitions */
#include <linux/power_ic.h> /* Phone's backlights ioctls */
#endif
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API_LCD) || \
    defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
#include <asm/mot-gpio.h>
#endif /* CONFIG_MOT_FEAT_GPIO_API_LCD||CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */

#include "mxcfb.h"

/*
 * Driver name
 */
#define MXCFB_NAME      "MXCFB"

/*
 * Debug Macros
 */
//#define MXCFB_DEBUG
#ifdef MXCFB_DEBUG

#define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " fmt, \
        __FILE__,__LINE__,__FUNCTION__ , ## args)
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#define FUNC_START    DPRINTK(" func start\n")
#define FUNC_END      DPRINTK(" func end\n")

#define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", __FILE__, \
        __LINE__, __FUNCTION__, err)

#else				//MXCFB_DEBUG

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				//MXCFB_DEBUG

struct mxcfb_data {
	struct fb_info *fbi;
	struct fb_info *fbi_ovl;
#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
        struct fb_info *        fbi_cli;
	int                     cli_active;
#endif
	volatile int32_t vsync_flag;
	wait_queue_head_t vsync_wq;
	wait_queue_head_t suspend_wq;
	bool suspended;
	int backlight_level;
};

static struct mxcfb_data mxcfb_drv_data;
static uint32_t def_vram = 0;

#ifdef CONFIG_FB_MXC_SANYO_QVGA_PANEL
 #ifndef CONFIG_MOT_FEAT_LANDSCAPE_IMODULE
struct panel_info sanyo_qvga_panel = {
        .name                   = "Sanyo QVGA Panel",
        .type                   = IPU_PANEL_SANYO_TFT,
        .pixel_fmt              = IPU_PIX_FMT_RGB666,
        .width                  = 240,
        .height                 = 320,
        .vSyncWidth             = 2,
        .vStartWidth            = 4,
        .vEndWidth              = 2,
        .hSyncWidth             = 10,
        .hStartWidth            = 30,
        .hEndWidth              = 10,
        .sig_pol.datamask_en    = false,
        .sig_pol.clkidle_en     = false,
        .sig_pol.clksel_en      = false,
        .sig_pol.Vsync_pol      = false,
        .sig_pol.enable_pol     = true,
        .sig_pol.data_pol       = false,
        .sig_pol.clk_pol        = true,
        .sig_pol.Hsync_pol      = false,
 };
 #else
  struct panel_info sanyo_qvga_panel = {
        .name                   = "Sanyo QVGA Panel_Landscape",
        .type                   = IPU_PANEL_SANYO_TFT,
        .pixel_fmt              = IPU_PIX_FMT_RGB666,
        .width                  = 320,
        .height                 = 240,
        .vSyncWidth             = 2,
        .vStartWidth            = 4,
        .vEndWidth              = 2,
        .hSyncWidth             = 10,
        .hStartWidth            = 30,
        .hEndWidth              = 10,
        .sig_pol.datamask_en    = false,
        .sig_pol.clkidle_en     = false,
        .sig_pol.clksel_en      = false,
        .sig_pol.Vsync_pol      = false,
        .sig_pol.enable_pol     = true,
        .sig_pol.data_pol       = false,
        .sig_pol.clk_pol        = true,
        .sig_pol.Hsync_pol      = false,
   };
  #endif
struct panel_info *mxcfb_panel = &sanyo_qvga_panel;
#endif

#ifdef CONFIG_FB_MXC_SHARP_QVGA_PANEL
struct panel_info sharp_qvga_panel = {
	.name = "Sharp QVGA Panel",
	.type = IPU_PANEL_SHARP_TFT,
	.pixel_fmt = IPU_PIX_FMT_RGB666,
	.width = 240,
	.height = 320,
	.vSyncWidth = 1,
	.vStartWidth = 8,
	.vEndWidth = 40,
	.hSyncWidth = 1,
	.hStartWidth = 3,
	.hEndWidth = 16,
	.sig_pol.datamask_en = false,
	.sig_pol.clkidle_en = true,
	.sig_pol.clksel_en = false,
	.sig_pol.Vsync_pol = false,
	.sig_pol.enable_pol = false,
	.sig_pol.data_pol = true,
	.sig_pol.clk_pol = true,
	.sig_pol.Hsync_pol = true,
};
struct panel_info *mxcfb_panel = &sharp_qvga_panel;
#endif

#ifdef CONFIG_FB_MXC_VGA_PANEL
struct panel_info nec_vga_panel = {
	.name = "NEC VGA Panel",
	.type = IPU_PANEL_TFT,
	.pixel_fmt = IPU_PIX_FMT_RGB666,
	.width = 640,
	.height = 480,
	.vSyncWidth = 1,
	.vStartWidth = 0x22,
	.vEndWidth = 40,
	.hSyncWidth = 1,
	.hStartWidth = 0x90,	//0x69, for Sharp
	.hEndWidth = 0,
	.sig_pol.datamask_en = false,
	.sig_pol.clkidle_en = false,
	.sig_pol.clksel_en = false,
	.sig_pol.Vsync_pol = false,
	.sig_pol.enable_pol = true,
	.sig_pol.data_pol = false,	//true, for Sharp
	.sig_pol.clk_pol = false,
	.sig_pol.Hsync_pol = false,
};
struct panel_info *mxcfb_panel = &nec_vga_panel;
#endif

#ifdef CONFIG_FB_MXC_TVOUT_VGA
struct panel_info tvout_vga_panel = {
	.name = "TV Encoder VGA Mode",
	.type = IPU_PANEL_SHARP_TFT,
	.pixel_fmt = IPU_PIX_FMT_RGB666,
	.width = 640,
	.height = 480,
	.vSyncWidth = 1,
	.vStartWidth = 10,
	.vEndWidth = 0,
	.hSyncWidth = 45,
	.hStartWidth = 244,
	.hEndWidth = 0,
	.sig_pol.datamask_en = false,
	.sig_pol.clkidle_en = false,
	.sig_pol.clksel_en = false,
	.sig_pol.Vsync_pol = false,
	.sig_pol.enable_pol = true,
	.sig_pol.data_pol = false,
	.sig_pol.clk_pol = false,
	.sig_pol.Hsync_pol = false,
};
struct panel_info *mxcfb_panel = &tvout_vga_panel;
#endif

#ifdef CONFIG_FB_MXC_TVOUT_NTSC
#error "TV-out NTSC mode not yet supported"
struct panel_info tvout_ntsc_panel = {
	.name = "TV Encoder NTSC Mode",
	.type = IPU_PANEL_TFT,
	.pixel_fmt = IPU_PIX_FMT_RGB666,
	.width = 640,
	.height = 480,
	.vSyncWidth = 1,
	.vStartWidth = 0x22,
	.vEndWidth = 40,
	.hSyncWidth = 1,
	.hStartWidth = 0x90,
	.hEndWidth = 0,
	.sig_pol.datamask_en = false,
	.sig_pol.clkidle_en = false,
	.sig_pol.clksel_en = false,
	.sig_pol.Vsync_pol = false,
	.sig_pol.enable_pol = true,
	.sig_pol.data_pol = false,	//true, for Sharp
	.sig_pol.clk_pol = false,
	.sig_pol.Hsync_pol = false,
};
struct panel_info *mxcfb_panel = &tvout_ntsc_panel;
#endif

#ifdef CONFIG_FB_MXC_TVOUT_PAL
#error "TV-out PAL mode not yet supported"
struct panel_info tvout_pal_panel = {
	.name = "TV Encoder PAL Mode",
	.type = IPU_PANEL_TFT,
	.pixel_fmt = IPU_PIX_FMT_RGB666,
	.width = 640,
	.height = 480,
	.vSyncWidth = 1,
	.vStartWidth = 0x22,
	.vEndWidth = 40,
	.hSyncWidth = 1,
	.hStartWidth = 0x90,
	.hEndWidth = 0,
	.sig_pol.datamask_en = false,
	.sig_pol.clkidle_en = false,
	.sig_pol.clksel_en = false,
	.sig_pol.Vsync_pol = false,
	.sig_pol.enable_pol = true,
	.sig_pol.data_pol = false,	//true, for Sharp
	.sig_pol.clk_pol = false,
	.sig_pol.Hsync_pol = false,
};
struct panel_info *mxcfb_panel = &tvout_pal_panel;
#endif

#if defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT)
#include <linux/console.h>	/* acquire_console_sem() */
static struct global_state mxcfb_global_state;
static LIGHTS_BACKLIGHT_IOCTL_T backlight_set;
#endif


static uint32_t bpp_to_pixfmt(int bpp)
{
	uint32_t pixfmt = 0;
	switch (bpp) {
	case 24:
#if defined(CONFIG_MOT_FEAT_IPU_BGRA6666)
		pixfmt = IPU_PIX_FMT_BGRA6666;
#elif defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		pixfmt = IPU_PIX_FMT_RGB24;
#else
		pixfmt = IPU_PIX_FMT_BGR24;
#endif
		break;
	case 32:
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		pixfmt = IPU_PIX_FMT_RGB32;
#else
		pixfmt = IPU_PIX_FMT_BGR32;
#endif
		break;
	case 16:
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB) 
		/* The V4L2 pixel packing definition of BGR565 is reversed */
		pixfmt = IPU_PIX_FMT_BGR565;
#else
		pixfmt = IPU_PIX_FMT_RGB565;
#endif
		break;
	}
	return pixfmt;
}

extern int fs453_setup(void);
static irqreturn_t mxcfb_irq_handler(int irq, void *dev_id,
				     struct pt_regs *regs);
static int mxcfb_blank(int blank, struct fb_info *info);
#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
static int enable_emulated_cli(void);
static int disable_emulated_cli(void);
#endif

#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
extern u32 mot_mbm_is_ipu_initialized;
extern u32 mot_mbm_ipu_buffer_address;
#endif

/*
 * Open the main framebuffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int mxcfb_open(struct fb_info *fbi, int user)
{
	int retval = 0;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;
#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
	char __iomem* io_remapped_logo;
#endif

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	if (mxc_fbi->open_count == 0) {
#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
                if (mot_mbm_is_ipu_initialized) {
                        /*
                         * Check if mot_mbm_ipu_buffer_address != this-framebuffer's smem_start 
                         * NOTE: fbi->fix.smem_start maynot be equal to MXCIPU_MEM_ADDRESS depending on whether
                         * IRAM is available on the hw
                         * */
                        if( mot_mbm_ipu_buffer_address && 
                                mot_mbm_ipu_buffer_address != (u32)fbi->fix.smem_start) {
                                
                                DPRINTK("MXCIPU_MEM_ADDRESS = 0x%08X\n", MXCIPU_MEM_ADDRESS);
                                DPRINTK("smem_len= %d\n", fbi->fix.smem_len);
                                
                                /* Copy the logo only if:
                                 *      a) MXCIPU_MEM_ADDRESS <= mot_mbm_ipu_buffer_address
                                 *      b) MXCIPU_MEM_ADDRESS + frame_size <= MXCIPU_MEM_ADDRESS + MXCIPU_MEM_SIZE
                                 *      c) dest-address (fb0->fix.smem_start) <= source (mot_mbm_ipu_buffer_address)
                                 *              --because memcpy_fromio() doesnot do reverse copy
                                 * */
                                if(MXCIPU_MEM_ADDRESS <= mot_mbm_ipu_buffer_address &&
                                        ((u32)fbi->fix.smem_start <= mot_mbm_ipu_buffer_address) &&
                                        ((mot_mbm_ipu_buffer_address + fbi->fix.smem_len) <= (MXCIPU_MEM_ADDRESS + MXCIPU_MEM_SIZE))
                                ){

                                        /* ioremap mot_mbm_ipu_buffer_address so that we can relocate the image*/
                                        if (!(io_remapped_logo = ioremap(mot_mbm_ipu_buffer_address, fbi->fix.smem_len))) {
                                                printk("MXCFB - Unable to io-remap logo memory to virtual address: Not relocating\n");
                                        } else {
                                                DPRINTK("ioremaped 0x%08X:to 0x%08X\n", 
                                                                mot_mbm_ipu_buffer_address, io_remapped_logo);

                                                memcpy_fromio(fbi->screen_base, io_remapped_logo, fbi->fix.smem_len);
                                                
                                                DPRINTK("memcpy_fromio(0x%08X, 0x%08X, %d)\n", 
                                                                fbi->screen_base, io_remapped_logo, fbi->fix.smem_len);
                                                iounmap(io_remapped_logo);
                                                
                                                DPRINTK("Bootlogo relocated to fb0's start address: \n");
                                        }
                                }else{
                                        printk("Bootlogo cannot be relocated to fb0's start address: \n");
                                        printk("\tmot_mbm_ipu_buffer_address=0x%08X IPU-memory: 0x%08lX size=%lu\n", 
                                                        mot_mbm_ipu_buffer_address, MXCIPU_MEM_ADDRESS, fbi->screen_size);
                                }
                        }
                        /* set mot_mbm_ipu_buffer_address to where pwruplogo is relocated to, i.e fbi->fix.smem_start
                         * This will make sure that when the open_count reaches zero, and when fb-dev is opened again
                         * we dont try to relocate the logo again . This will ensure that poweruplogo is relocated
                         * ONLY once i.e at boot time
                         * */
                        mot_mbm_ipu_buffer_address = fbi->fix.smem_start;
                }
#endif
		mxc_fbi->ipu_ch_irq = IPU_IRQ_SDC_BG_EOF;
		ipu_clear_irq(mxc_fbi->ipu_ch_irq);

#ifndef CONFIG_MOT_FEAT_POWERUP_LOGO
        /* Only re-init display if no power up logo */
		if (ipu_sdc_init_panel(mxcfb_panel->type, 60,
				       mxcfb_panel->width, mxcfb_panel->height,
				       mxcfb_panel->pixel_fmt,
				       mxcfb_panel->hStartWidth,
				       mxcfb_panel->hSyncWidth,
				       mxcfb_panel->hEndWidth,
				       mxcfb_panel->vStartWidth,
				       mxcfb_panel->vSyncWidth,
				       mxcfb_panel->vEndWidth,
				       mxcfb_panel->sig_pol) != 0) {
			printk("mxcfb: Error initializing panel.\n");
			return -EINVAL;
		}
#endif        

#ifdef CONFIG_FB_MXC_TVOUT
		if ((retval = fs453_setup()) < 0) {
			return retval;
		}
#endif

#ifndef CONFIG_MOT_FEAT_POWERUP_LOGO
        /* Only re-init display if no power up logo */
		ipu_sdc_set_window_pos(MEM_SDC_BG, 0, 0);

		ipu_sdc_set_brightness(255);
		ipu_sdc_set_global_alpha(false, 0);
		ipu_sdc_set_color_key(MEM_SDC_BG, false, 0);

		ipu_init_channel(MEM_SDC_BG, NULL);
		ipu_init_channel_buffer(MEM_SDC_BG, IPU_INPUT_BUFFER,
					bpp_to_pixfmt(fbi->var.bits_per_pixel),
					fbi->var.xres, fbi->var.yres,
					fbi->var.xres_virtual,
					IPU_ROTATE_NONE,
					(void *)fbi->fix.smem_start,
					(void *)fbi->fix.smem_start);
#else
        ipu_update_channel_buffer(MEM_SDC_BG, IPU_INPUT_BUFFER, 0, (void *)fbi->fix.smem_start);
        ipu_sdc_sync_bg_window_pos_to_global();
#endif
		sema_init(&mxc_fbi->flip_sem, 1);
		mxc_fbi->cur_ipu_buf = 0;

		ipu_select_buffer(MEM_SDC_BG, IPU_INPUT_BUFFER, 0);

		mxc_fbi->ipu_ch = MEM_SDC_BG;

		if (ipu_request_irq(IPU_IRQ_SDC_BG_EOF, mxcfb_irq_handler, 0,
				    MXCFB_NAME, fbi) != 0) {
			printk("mxcfb: Error registering irq handler.\n");
			return -EBUSY;
		}
		ipu_disable_irq(mxc_fbi->ipu_ch_irq);
                /* do not UNBLANK the fb-device everytime the device is opened */
                mxcfb_blank(FB_BLANK_UNBLANK, fbi);
	}
	mxc_fbi->open_count++;

	return 0;
}

/*
 * Close the main framebuffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int mxcfb_release(struct fb_info *fbi, int user)
{
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;
	FUNC_START;

	--mxc_fbi->open_count;
	if (mxc_fbi->open_count == 0) {
		mxcfb_blank(FB_BLANK_POWERDOWN, fbi);

		ipu_uninit_channel(MEM_SDC_BG);
		ipu_free_irq(IPU_IRQ_SDC_BG_EOF, fbi);
	}
	return 0;
}

/*
 * Set fixed framebuffer parameters based on variable settings.
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_set_fix(struct fb_info *info)
{
	struct fb_fix_screeninfo *fix = &info->fix;
	struct fb_var_screeninfo *var = &info->var;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;
	FUNC_START;

	if (mxc_fbi->ipu_ch == MEM_SDC_FG)
		strncpy(fix->id, "DISP3 FG", 8);
	else
		strncpy(fix->id, "DISP3 BG", 8);

	fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;

	fix->type = FB_TYPE_PACKED_PIXELS;

	/* 
	 * 1) predefined unique accel number is used by a directfb mxc gfx-driver
	 * to identify fb device it can handle.
	 * 2) gfx driver requires acces to hw registers. fields mmio_start and mmio_len
	 * are used to provide registers mapping.
	 */
	fix->accel = 0x90 ;
	fix->mmio_start  = IPU_CTRL_BASE_ADDR;
	fix->mmio_len    = 0x1BD ;
		
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->xpanstep = 1;
	fix->ypanstep = 1;

	return 0;
}

/*
 * Set framebuffer parameters and change the operating mode.
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_set_par(struct fb_info *info)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	ipu_disable_irq(mxc_fbi->ipu_ch_irq);
	ipu_disable_channel(mxc_fbi->ipu_ch, true);
	ipu_clear_irq(mxc_fbi->ipu_ch_irq);
	mxcfb_set_fix(info);

	mxc_fbi->cur_ipu_buf = 0;
	sema_init(&mxc_fbi->flip_sem, 1);

	ipu_sdc_set_window_pos(mxc_fbi->ipu_ch, 0, 0);
	ipu_init_channel_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER,
				bpp_to_pixfmt(info->var.bits_per_pixel),
				info->var.xres, info->var.yres,
				info->var.xres_virtual,
				IPU_ROTATE_NONE,
				(void *)info->fix.smem_start,
				(void *)info->fix.smem_start);

	ipu_select_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER, 0);

	ipu_enable_channel(mxc_fbi->ipu_ch);
	return 0;
}

/*
 * Check framebuffer variable parameters and adjust to valid values.
 *
 * @param       var      framebuffer variable parameters
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	FUNC_START;

	if (var->xres > mxcfb_panel->width)
		var->xres = mxcfb_panel->width;
	if (var->yres > mxcfb_panel->height)
		var->yres = mxcfb_panel->height;
	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

#ifdef CONFIG_FB_MXC_INTERNAL_MEM
	if ((var->bits_per_pixel != 24) &&
#else
	if ((var->bits_per_pixel != 32) && (var->bits_per_pixel != 24) &&
#endif
	    (var->bits_per_pixel != 16)) {
		var->bits_per_pixel = MXCFB_DEFUALT_BPP;
	}

	switch (var->bits_per_pixel) {
	case 16:
		var->red.length = 5;
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		var->red.offset = 0;
#else
		var->red.offset = 11;
#endif
		var->red.msb_right = 0;

		var->green.length = 6;
		var->green.offset = 5;
		var->green.msb_right = 0;

		var->blue.length = 5;
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		var->blue.offset = 11;
#else
		var->blue.offset = 0;
#endif
		var->blue.msb_right = 0;

		var->transp.length = 0;
		var->transp.offset = 0;
		var->transp.msb_right = 0;
		break;
	case 24:
#if defined(CONFIG_MOT_FEAT_IPU_BGRA6666)
		/* As the feature name suggests B,G,R and A bitfields' in that 
		 * order are defined. All offsets are from the right,  inside a
		 * "pixel" value, which is exactly 'bits_per_pixel' wide. 
		 * Refer to <LinuxSource>/ include/linux/fb.h for an 
		 * interpretation of offset for color fields.
		 */ 
 		var->red.length = 6;
		var->red.offset = 12;
		var->red.msb_right = 0;

		var->green.length = 6;
		var->green.offset = 6;
		var->green.msb_right = 0;

		var->blue.length = 6;
		var->blue.offset = 0;
		var->blue.msb_right = 0;

		var->transp.length = 6;
		var->transp.offset = 18;
		var->transp.msb_right = 0;
#else /* RGB24 or BGR24 formats */
		var->red.length = 8;
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		var->red.offset = 0;
#else
		var->red.offset = 16;
#endif
		var->red.msb_right = 0;

		var->green.length = 8;
		var->green.offset = 8;
		var->green.msb_right = 0;

		var->blue.length = 8;
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		var->blue.offset = 16;
#else
		var->blue.offset = 0;
#endif
		var->blue.msb_right = 0;

		var->transp.length = 0;
		var->transp.offset = 0;
		var->transp.msb_right = 0;
#endif /* MOT_FEAT_IPU_BGRA6666 */ 
		break;
	case 32:
		var->red.length = 8;
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		var->red.offset = 0;
#else
		var->red.offset = 16;
#endif
		var->red.msb_right = 0;

		var->green.length = 8;
		var->green.offset = 8;
		var->green.msb_right = 0;

		var->blue.length = 8;
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
		var->blue.offset = 16;
#else
		var->blue.offset = 0;
#endif
		var->blue.msb_right = 0;

		var->transp.length = 8;
		var->transp.offset = 24;
		var->transp.msb_right = 0;
		break;
	}

	var->height = -1;
	var->width = -1;
	var->grayscale = 0;
	var->nonstd = 0;

	var->pixclock = -1;
	var->left_margin = -1;
	var->right_margin = -1;
	var->upper_margin = -1;
	var->lower_margin = -1;
	var->hsync_len = -1;
	var->vsync_len = -1;

	var->vmode = FB_VMODE_NONINTERLACED;
	var->sync = 0;
	return 0;

}

#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)
#define MXCFB_DOWN_INTERRUPTIBLE(sem) 		\
do {						\
	if(down_interruptible(sem) != 0) {	\
		return -ERESTARTSYS;		\
	}					\
} while(0)

#define MXCFB_UP(sem)				\
do { 						\
	up(sem);				\
} while(0)	
#endif /* defined(CONFIG_MOT_FEAT_IPU_IOCTL) */

static inline u_int _chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}
static int
mxcfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int trans, struct fb_info *fbi)
{
	unsigned int val;
	int ret = 1;

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no matter what visual we are using.
	 * Y (luminance) =  0.299 * R + 0.587 * G + 0.114 * B 
	 */
	if (fbi->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green +
				      7471 * blue) >> 16;
	switch (fbi->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/*
		 * 16-bit True Colour.  We encode the RGB value
		 * according to the RGB bitfield information.
		 */
		if (regno < 16) {
			u32 *pal = fbi->pseudo_palette;

			val = _chan_to_field(red, &fbi->var.red);
			val |= _chan_to_field(green, &fbi->var.green);
			val |= _chan_to_field(blue, &fbi->var.blue);

			pal[regno] = val;
			ret = 0;
		}
		break;

	case FB_VISUAL_STATIC_PSEUDOCOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		break;
	}

	return ret;
}

/*
 * Function to handle custom ioctls for MXC framebuffer.
 *
 * @param       inode   inode struct
 *
 * @param       file    file struct
 *
 * @param       cmd     Ioctl command to handle
 *
 * @param       arg     User pointer to command arguments
 *
 * @param       fbi     framebuffer information pointer
 */
static int mxcfb_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg, struct fb_info *fbi)
{
	int retval = 0;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	switch (cmd) {
#if defined(CONFIG_FB_MXC_OVERLAY)
	case MXCFB_SET_GBL_ALPHA:
		{
			struct mxcfb_gbl_alpha ga;
			if (copy_from_user(&ga, (void *)arg, sizeof(ga))) {
				retval = -EFAULT;
				break;
			}
			retval =
			    ipu_sdc_set_global_alpha((bool) ga.enable,
						     ga.alpha);
			DPRINTK("Set global alpha to %d\n", ga.alpha);
			break;
		}
	case MXCFB_SET_CLR_KEY:
		{
			struct mxcfb_color_key key;
			if (copy_from_user(&key, (void *)arg, sizeof(key))) {
				retval = -EFAULT;
				break;
			}
			retval = ipu_sdc_set_color_key(MEM_SDC_BG, key.enable,
						       key.color_key);
			DPRINTK("Set color key to 0x%08X\n", key.color_key);
			break;
		}
#endif
#ifdef NEW_MBX
	case MXCFB_WAIT_FOR_VSYNC:
		{
			mxcfb_drv_data.vsync_flag = 0;
			ipu_enable_irq(IPU_IRQ_SDC_DISP3_VSYNC);
			if (!wait_event_interruptible_timeout
			    (mxcfb_drv_data.vsync_wq,
			     mxcfb_drv_data.vsync_flag != 0, 1 * HZ)) {
				printk("MXCFB_WAIT_FOR_VSYNC: timeout\n");
				retval = -ETIME;
				break;
			} else if (signal_pending(current)) {
				printk
				    ("MXCFB_WAIT_FOR_VSYNC: interrupt received\n");
				retval = -ERESTARTSYS;
				break;
			}
			break;
		}
#endif
	case MXCFB_SET_BRIGHTNESS:
		{
			uint8_t level;
			if (copy_from_user(&level, (void *)arg, sizeof(level))) {
				retval = -EFAULT;
				break;
			}

			mxcfb_drv_data.backlight_level = level;
			retval = ipu_sdc_set_brightness(level);
			DPRINTK("Set brightness to %d\n", level);
			break;
		}
#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)
        case FBIOSETBKLIGHT:
        {
                DPRINTK("mxcfb_ioctl:FBIOSETBKLIGHT,arg(%d)\n",arg);
                switch (arg) {
                case BKLIGHT_OFF:
			MXCFB_DOWN_INTERRUPTIBLE(&mxcfb_global_state.g_sem);
#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
                        gpio_lcd_backlight_enable(false);
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */
			mxcfb_global_state.backlight_state &= ~BKLIGHT_ON;
			MXCFB_UP(&mxcfb_global_state.g_sem);
			break;
                case BKLIGHT_ON:
			MXCFB_DOWN_INTERRUPTIBLE(&mxcfb_global_state.g_sem);
#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
                        gpio_lcd_backlight_enable(true);
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */
			mxcfb_global_state.backlight_state |= BKLIGHT_ON;
			MXCFB_UP(&mxcfb_global_state.g_sem);
			break;
		default:
			retval = -EINVAL;
		}
		break;
        }
	case FBIOGETBKLIGHT:
	{
		unsigned long i;

		MXCFB_DOWN_INTERRUPTIBLE(&mxcfb_global_state.g_sem);
#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
		i = gpio_get_lcd_backlight();
#else
		i = ( (mxcfb_global_state.backlight_state & BKLIGHT_ON) == 0 
			? false : true);
#endif
		MXCFB_UP(&mxcfb_global_state.g_sem);

		if (put_user(i, (u8*)arg) != 0)
		{
			retval = -EFAULT;
		}
		break;
	}
	case FBIOSETBRIGHTNESS:
	{
		if (arg > mxcfb_global_state.bklight_range.max || 
			arg < mxcfb_global_state.bklight_range.min)
		{
			retval = -EINVAL;
			break;
		}
		MXCFB_DOWN_INTERRUPTIBLE(&mxcfb_global_state.g_sem);
#if defined(CONFIG_MACH_MXC27530EVB) || defined(CONFIG_MACH_I30030EVB) \
			           || defined(CONFIG_MACH_MXC91131EVB)
		if ((retval = ipu_sdc_set_brightness(arg)) != 0) {
			MXCFB_UP(&mxcfb_global_state.g_sem);
			retval = -EFAULT;
			break;
		}
#elif defined(CONFIG_MACH_ARGONLVPHONE) \
                && defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
	        pwm_set_lcd_bkl_brightness(arg);	
#else	
		backlight_set.bl_select = LIGHTS_BACKLIGHT_DISPLAY;
		backlight_set.bl_brightness = arg;
#endif
		mxcfb_global_state.brightness = arg;
		MXCFB_UP(&mxcfb_global_state.g_sem);
		break;
	}
	case FBIOGETBRIGHTNESS:
	{
		MXCFB_DOWN_INTERRUPTIBLE(&mxcfb_global_state.g_sem);
#if defined(CONFIG_MACH_ARGONLVPHONE) \
                && defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
	        mxcfb_global_state.brightness = pwm_get_lcd_bkl_brightness();	
#endif
		retval = copy_to_user((unsigned long *)arg, 
			&(mxcfb_global_state.brightness), 
			sizeof(unsigned long));
		MXCFB_UP(&mxcfb_global_state.g_sem);
		retval = retval ? -EFAULT : 0;
		break;
	}
	case FBIOSET_BRIGHTNESSRANGE:
	{
		struct backlight_brightness_range bklight_range;
		MXCFB_DOWN_INTERRUPTIBLE(&mxcfb_global_state.g_sem);
                if (copy_from_user(&bklight_range, (void *)arg, 
		    sizeof(bklight_range))) 
		{
			MXCFB_UP(&mxcfb_global_state.g_sem);
                        retval = -EFAULT;
                        break;
                }
		mxcfb_global_state.bklight_range = bklight_range;
		MXCFB_UP(&mxcfb_global_state.g_sem);
		break;
	}
#endif /* CONFIG_MOT_FEAT_IPU_IOCTL */
#if defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT)
	case FBIOENABLE2BFS:
	{
		mxcfb_global_state.dbuffer_enabled = DBUFFER_ENABLED;
		break;
	}
	case FBIODISABLE2BFS:
	{
		int i;
		struct fb_var_screeninfo var = mxcfb_drv_data.fbi->var;
		var.yoffset = 0;
		acquire_console_sem();
		i = fb_pan_display(mxcfb_drv_data.fbi, &var);
		release_console_sem();

		if (i) 
		{
			retval = i;
			break;
		}
		mxcfb_global_state.dbuffer_enabled = DBUFFER_DISABLED;
		break;
	}
	case FBIOCHECK2BFS:
	{
		unsigned long screen_addr;
		retval = (mxcfb_global_state.dbuffer_enabled == DBUFFER_ENABLED);
		if (retval)
		{
			screen_addr = mxcfb_drv_data.fbi->fix.smem_start + 
			    mxcfb_drv_data.fbi->var.yres * mxcfb_drv_data.fbi->var.xres * 
			    ( mxcfb_drv_data.fbi->var.bits_per_pixel / 8 );

			if (copy_to_user((unsigned long *)arg, &screen_addr, 
			    sizeof(unsigned long)) != 0)
			{
				retval = -EFAULT;
			}
		}
		break;
	}
	case FBIOCKMAINVALIDFB:
	{
#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
		retval = 1 - mxcfb_drv_data.cli_active;
#else
		retval = 1;
#endif
		break;
	}
#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
	/* ioctl stub for EzX compatibility on the emulated CLI */
	case FBIOSMARTUPDATE:
	{
		/* Return success if emulated CLI device is registered */
		if (mxcfb_drv_data.fbi_cli == NULL) {
			retval = -EIO;
		}
		break;
	}
	case FBIOENTERCLIPWRSAVE:
	{
		/* Return success if emulated CLI device is registered */
		if (mxcfb_drv_data.fbi_cli == NULL) {
			retval = -EIO;
		}
		break;
	}
	case FBIOEXITCLIPWRSAVE:
	{
		/* Return success if emulated CLI device is registered */
		if (mxcfb_drv_data.fbi_cli == NULL) {
			retval = -EIO;
		}
		break;
	}
#endif /* defined(CONFIG_MOT_FEAT_EMULATED_CLI) */
#endif /* defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT) */
#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
	case FBIOENABLE_EMULATEDCLI:
	{
		retval = enable_emulated_cli();
		break;
	}
	case FBIODISABLE_EMULATEDCLI:
	{
		retval = disable_emulated_cli();
		break;
	}
#endif /* defined(CONFIG_MOT_FEAT_EMULATED_CLI) */
#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)
        case FBIO_QUERY_DISPLAY_TYPE:
        {
                unsigned long disp_type = TRANSMISSIVE_DISPLAY; /* default to TRANSMISSIVE */

#if defined(CONFIG_FB_MXC_MAIN_TRANSFLECTIVE_DISPLAY)
                disp_type = TRANSFLECTIVE_DISPLAY;
#endif
                retval = copy_to_user((unsigned long *)arg, &disp_type, sizeof(unsigned long));
                break;
        }
#endif /* CONFIG_MOT_FEAT_IPU_IOCTL */
	default:
		retval = -EINVAL;
	}
	return retval;
}

#ifdef CONFIG_FB_MXC_OVERLAY
/*
 * Open the overlay framebuffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int mxcfb_ovl_open(struct fb_info *fbi, int user)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	if (mxc_fbi->open_count == 0) {
		mxc_fbi->ipu_ch_irq = IPU_IRQ_SDC_FG_EOF;
		ipu_clear_irq(mxc_fbi->ipu_ch_irq);

		ipu_sdc_set_window_pos(MEM_SDC_FG, fbi->var.xoffset,
				       fbi->var.yoffset);

		ipu_init_channel(MEM_SDC_FG, NULL);
		ipu_init_channel_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER,
					bpp_to_pixfmt(fbi->var.bits_per_pixel),
					fbi->var.xres, fbi->var.yres,
					fbi->var.xres_virtual,
					IPU_ROTATE_NONE,
					(void *)fbi->fix.smem_start,
					(void *)fbi->fix.smem_start);
		sema_init(&mxc_fbi->flip_sem, 1);
		mxc_fbi->cur_ipu_buf = 0;

		ipu_select_buffer(MEM_SDC_FG, IPU_INPUT_BUFFER, 0);
		mxc_fbi->ipu_ch = MEM_SDC_FG;

		if (ipu_request_irq(IPU_IRQ_SDC_FG_EOF, mxcfb_irq_handler, 0,
				    MXCFB_NAME, fbi) != 0) {
			printk("mxcfb: Error registering irq handler.\n");
			return -EBUSY;
		}
		ipu_disable_irq(mxc_fbi->ipu_ch_irq);
		ipu_enable_channel(MEM_SDC_FG);
	}
	mxc_fbi->open_count++;
	return 0;
}

/*
 * Close the overlay framebuffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int mxcfb_ovl_release(struct fb_info *fbi, int user)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	--mxc_fbi->open_count;
	if (mxc_fbi->open_count == 0) {
		ipu_disable_channel(MEM_SDC_FG, true);
		ipu_uninit_channel(MEM_SDC_FG);
		ipu_free_irq(IPU_IRQ_SDC_FG_EOF, fbi);
	}
	return 0;
}
#endif


#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
/* 
 * Update the framebuffer DMA pointer
 * 
 * @param       base    new base framebuffer address 
 *
 */
static int 
mxcfb_update_main_channel_buffer(unsigned long base)
{
	int retval;
	uint32_t lock_flags = 0;
	struct mxcfb_info * mxc_fbi;

        if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq, 
                (mxcfb_drv_data.suspended == false))) < 0) {
                return retval;
        }

	/* 
	 * If the DMA address to be updated is from the emulated CLI, then perform
	 * the update using the main display's inactive double buffer of the base
	 * address.  We must update the inactive base address register and set it 
	 * to be active at the next end of frame 
	 */
	mxc_fbi = (struct mxcfb_info *)mxcfb_drv_data.fbi->par;

        spin_lock_irqsave(&mxc_fbi->fb_lock, lock_flags);

	/* Update the double buffered address of the inactive buffer */
        mxc_fbi->cur_ipu_buf = 1 - mxc_fbi->cur_ipu_buf;
        if (ipu_update_channel_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER, 
			mxc_fbi->cur_ipu_buf, (void*)base) == 0) {
		ipu_select_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER, 
			    mxc_fbi->cur_ipu_buf);
                ipu_clear_irq(mxc_fbi->ipu_ch_irq);
                ipu_enable_irq(mxc_fbi->ipu_ch_irq);
        }
        else {
		printk("ERROR updating SDC buf %d to address=0x%08X\n", 
		mxc_fbi->cur_ipu_buf, (uint32_t)base);
        }
		
        spin_unlock_irqrestore(&mxc_fbi->fb_lock, lock_flags); 

	return 0;
}

static int enable_emulated_cli()
{
	int retval;

	if (mxcfb_drv_data.cli_active) {
		/* Already enabled */
		retval = -EINVAL;
	} else {
		retval = mxcfb_update_main_channel_buffer(mxcfb_drv_data.fbi_cli->fix.smem_start);
		mxcfb_drv_data.cli_active = 1;
	}
	return retval;
}

static int disable_emulated_cli()
{
	int retval;

	if (mxcfb_drv_data.cli_active == 0) {
		/* Already disabled */
		retval = -EINVAL;
	} else {
		retval = mxcfb_update_main_channel_buffer(mxcfb_drv_data.fbi->fix.smem_start);
		mxcfb_drv_data.cli_active = 0;
	}
	return retval;
}


/*
 * Open the CLI virtual device
 * 
 * @param       fbi     framebuffer information pointer
 * 
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int 
mxcfb_cli_open(struct fb_info *fbi, int user)
{
        struct mxcfb_info * mxc_fbi = (struct mxcfb_info *)fbi->par;

        FUNC_START;

	++mxc_fbi->open_count;

	FUNC_END;
        return 0;
}

/*
 * Close the CLI virtual framebuffer.
 * 
 * @param       fbi     framebuffer information pointer
 * 
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int 
mxcfb_cli_release(struct fb_info *fbi, int user)
{
        int retval = 0;
        struct mxcfb_info * mxc_fbi = (struct mxcfb_info *)fbi->par;

        FUNC_START;

        --mxc_fbi->open_count;
        if (mxc_fbi->open_count == 0)
        {
		/* Disable the Emulated CLI when the device is closed */
		retval = disable_emulated_cli();
        }

	FUNC_END;
        return retval;
}
#endif /* defined(CONFIG_MOT_FEAT_EMULATED_CLI) */



/*
 * mxcfb_blank():
 *      Blank the display.
 */
static int mxcfb_blank(int blank, struct fb_info *info)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;

	FUNC_START;
	DPRINTK("blank = %d\n", blank);

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	mxc_fbi->blank = blank;

	switch (blank) {
	case FB_BLANK_POWERDOWN:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
		ipu_disable_channel(MEM_SDC_BG, true);
		gpio_lcd_inactive();
		ipu_sdc_set_brightness(0);
		break;
	case FB_BLANK_UNBLANK:
		gpio_lcd_active();
		ipu_enable_channel(MEM_SDC_BG);
		ipu_sdc_set_brightness(mxcfb_drv_data.backlight_level);
		break;
	}
	return 0;
}

/*
 * Pan or Wrap the Display
 *
 * This call looks only at xoffset, yoffset and the FB_VMODE_YWRAP flag
 *
 * @param               var     Variable screen buffer information
 * @param               info    Framebuffer information pointer
 */
static int
mxcfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;
	uint32_t lock_flags = 0;
	int retval;
	u_int y_bottom;
	unsigned long base;

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	if (var->xoffset > 0) {
		DPRINTK("x panning not supported\n");
		return -EINVAL;
	}

	if ((info->var.xoffset == var->xoffset) &&
	    (info->var.yoffset == var->yoffset)) {
		return 0;	// No change, do nothing
	}

	y_bottom = var->yoffset;

	if (!(var->vmode & FB_VMODE_YWRAP)) {
		y_bottom += var->yres;
	}

	if (y_bottom > info->var.yres_virtual) {
		return -EINVAL;
	}

	base = (var->yoffset * var->xres_virtual + var->xoffset);
	base *= (var->bits_per_pixel) / 8;
	base += info->fix.smem_start;

	down(&mxc_fbi->flip_sem);

	spin_lock_irqsave(&mxc_fbi->fb_lock, lock_flags);

//        printk("Updating SDC BG buf %d address=0x%08X\n",
//                mxc_fbi->cur_ipu_buf, base);

	mxc_fbi->cur_ipu_buf = !mxc_fbi->cur_ipu_buf;
	if (ipu_update_channel_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER,
				      mxc_fbi->cur_ipu_buf,
				      (void *)base) == 0) {
		ipu_select_buffer(mxc_fbi->ipu_ch, IPU_INPUT_BUFFER,
				  mxc_fbi->cur_ipu_buf);
		ipu_clear_irq(mxc_fbi->ipu_ch_irq);
		ipu_enable_irq(mxc_fbi->ipu_ch_irq);
	} else {
		printk("Error updating SDC buf %d to address=0x%08X\n",
		       mxc_fbi->cur_ipu_buf, (uint32_t) base);
	}

	spin_unlock_irqrestore(&mxc_fbi->fb_lock, lock_flags);

//        printk("Update complete\n");

	info->var.xoffset = var->xoffset;
	info->var.yoffset = var->yoffset;

	if (var->vmode & FB_VMODE_YWRAP) {
		info->var.vmode |= FB_VMODE_YWRAP;
	} else {
		info->var.vmode &= ~FB_VMODE_YWRAP;
	}

	return 0;
}

#ifdef CONFIG_MOT_FEAT_DISABLE_SW_CURSOR
static int disable_sw_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	return 0;
}
#endif

/*!
 * This structure contains the pointers to the control functions that are
 * invoked by the core framebuffer driver to perform operations like
 * blitting, rectangle filling, copy regions and cursor definition.
 */
static struct fb_ops mxcfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = mxcfb_open,
	.fb_release = mxcfb_release,
	.fb_set_par = mxcfb_set_par,
	.fb_check_var = mxcfb_check_var,
	.fb_setcolreg = mxcfb_setcolreg,
#ifndef CONFIG_FB_MXC_INTERNAL_MEM
	.fb_pan_display = mxcfb_pan_display,
#endif
	.fb_ioctl = mxcfb_ioctl,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_blank = mxcfb_blank,
#ifdef CONFIG_MOT_FEAT_DISABLE_SW_CURSOR
        .fb_cursor = disable_sw_cursor,
#else
	.fb_cursor = soft_cursor,
#endif
};

#ifdef CONFIG_FB_MXC_OVERLAY
static struct fb_ops mxcfb_ovl_ops = {
	.owner = THIS_MODULE,
	.fb_open = mxcfb_ovl_open,
	.fb_release = mxcfb_ovl_release,
	.fb_set_par = mxcfb_set_par,
	.fb_check_var = mxcfb_check_var,
	.fb_setcolreg = mxcfb_setcolreg,
	.fb_pan_display = mxcfb_pan_display,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
#ifdef CONFIG_MOT_FEAT_DISABLE_SW_CURSOR
        .fb_cursor = disable_sw_cursor,
#else
	.fb_cursor = soft_cursor,
#endif
};
#endif

#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
static struct fb_ops mxcfb_cli_ops = {
	.owner = THIS_MODULE,
	.fb_open = mxcfb_cli_open,
	.fb_release = mxcfb_cli_release,
	.fb_ioctl = mxcfb_ioctl,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_blank = mxcfb_blank,
#ifdef CONFIG_MOT_FEAT_DISABLE_SW_CURSOR
        .fb_cursor = disable_sw_cursor,
#else
	.fb_cursor = soft_cursor,
#endif
};
#endif

#ifdef NEW_MBX
static irqreturn_t mxcfb_vsync_irq_handler(int irq, void *dev_id,
					   struct pt_regs *regs)
{
	struct mxcfb_data *fb_data = dev_id;

	ipu_disable_irq(irq);

	fb_data->vsync_flag = 1;
	wake_up_interruptible(&fb_data->vsync_wq);
	return IRQ_HANDLED;
}
#endif

static irqreturn_t mxcfb_irq_handler(int irq, void *dev_id,
				     struct pt_regs *regs)
{
	struct fb_info *fbi = dev_id;
	struct mxcfb_info *mxc_fbi = fbi->par;

	up(&mxc_fbi->flip_sem);
	ipu_disable_irq(irq);
	return IRQ_HANDLED;
}

/* remove unused mxcfb_suspend/resume definition for LJ6.3
#ifdef CONFIG_PM

/* 
 * XXX : Testing of the LPMC hardware uncovered an unexpected and infrequent 
 * error in suspend/resume.  We are disabling this until the root cause can be
 * identified.  Also, please note that this is not a real Kconfig option, but 
 * rather is intended to #if 0 the LPMC code out.
 */
/*
#undef CONFIG_MOT_FEAT_ENABLE_LPMC

/*
 * Power management hooks.      Note that we won't be called from IRQ context,
 * unlike the blank functions above, so we may sleep.
 */

/*
 * Suspends the framebuffer and blanks the screen. Power management support
 */
/*
static int mxcfb_suspend(struct device *dev, u32 state, u32 level)
{
	struct mxcfb_data *drv_data = dev_get_drvdata(dev);
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)drv_data->fbi->par;
#ifdef CONFIG_FB_MXC_OVERLAY
	struct mxcfb_info *mxc_fbi_ovl =
	    (struct mxcfb_info *)drv_data->fbi_ovl->par;
#endif
#ifdef CONFIG_MOT_FEAT_ENABLE_LPMC
#ifdef CONFIG_FB_MXC_INTERNAL_MEM
	ipu_lpmc_reg_t save_list[2];
	ipu_lpmc_reg_t lpm_list[2];
#endif
#endif /* CONFIG_MOT_FEAT_ENABLE_LPMC */

/*
	FUNC_START;
	DPRINTK("level = %d\n", level);

	switch (level) {
	case SUSPEND_DISABLE:
		drv_data->suspended = true;
#ifdef CONFIG_FB_MXC_OVERLAY
		if (mxc_fbi_ovl->enabled)
			ipu_disable_channel(MEM_SDC_FG, true);
#endif

		if (mxc_fbi->blank == FB_BLANK_UNBLANK) {
#ifdef CONFIG_FB_MXC_INTERNAL_MEM
			// LPMC init
#ifdef CONFIG_MOT_FEAT_ENABLE_LPMC
			save_list[0].reg = 0x15C;	// DI_DISP3_TIME_CONF
			save_list[0].reg |= IPU_LPMC_REG_READ;
			save_list[0].value = 0;

			lpm_list[0].reg = 0x15C;	// DI_DISP3_TIME_CONF
			lpm_list[0].value = 0x02000040;

			ipu_lpmc_init(25000000, save_list, 1, lpm_list, 1);
#else
			ipu_disable_channel(MEM_SDC_BG, true);
#endif /* CONFIG_MOT_FEAT_ENABLE_LPMC */
/*
#if defined(CONFIG_MOT_FEAT_IPU_GPIO)
			gpio_lcd_inactive();
#endif
#else /* !defined(CONFIG_FB_MXC_INTERNAL_MEM) */
/*
			ipu_disable_channel(MEM_SDC_BG, true);
#if defined(CONFIG_MOT_FEAT_IPU_GPIO)
			gpio_lcd_inactive();
#endif
			ipu_sdc_set_brightness(0);
#endif /* defined(CONFIG_FB_MXC_INTERNAL_MEM) */
/*		
		}
		break;
	case SUSPEND_POWER_DOWN:
		break;
	}
	FUNC_END;
	return 0;
}

/*
 * Resumes the framebuffer and unblanks the screen. Power management support
 */
/*
static int mxcfb_resume(struct device *dev, u32 level)
{
	struct mxcfb_data *drv_data = dev_get_drvdata(dev);
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)drv_data->fbi->par;
#ifdef CONFIG_FB_MXC_OVERLAY
	struct mxcfb_info *mxc_fbi_ovl =
	    (struct mxcfb_info *)drv_data->fbi_ovl->par;
#endif

	FUNC_START;
	DPRINTK("level = %d\n", level);

	switch (level) {
	case RESUME_POWER_ON:
		// Display ON
		break;
	case RESUME_ENABLE:
		if (mxc_fbi->blank == FB_BLANK_UNBLANK) {
#ifdef CONFIG_FB_MXC_INTERNAL_MEM
#if CONFIG_MOT_FEAT_ENABLE_LPMC 
			// LPMC init
			ipu_lpmc_uninit();
#endif
			ipu_enable_channel(MEM_SDC_BG);
#if defined(CONFIG_MOT_FEAT_IPU_GPIO)
			gpio_lcd_active();
#endif
#else /* !defined(CONFIG_FB_MXC_INTERNAL_MEM) */
/*			
			ipu_enable_channel(MEM_SDC_BG);
#if defined(CONFIG_MOT_FEAT_IPU_GPIO)
			gpio_lcd_active();
#endif
			ipu_sdc_set_brightness(drv_data->backlight_level);
#endif
		}
#ifdef CONFIG_FB_MXC_OVERLAY
		if (mxc_fbi_ovl->blank)
			ipu_enable_channel(MEM_SDC_FG);
#endif
		drv_data->suspended = false;

		wake_up_interruptible(&drv_data->suspend_wq);
	}
	FUNC_END;
	return 0;
}
#else
*/
#define mxcfb_suspend   NULL
#define mxcfb_resume    NULL
/*
#endif
*/

/*
 * Main framebuffer functions
 */

/*!
 * Allocates the DRAM memory for the frame buffer.      This buffer is remapped
 * into a non-cached, non-buffered, memory region to allow palette and pixel
 * writes to occur without flushing the cache.  Once this area is remapped,
 * all virtual memory access to the video memory should occur at the new region.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @return      Error code indicating success or failure
 */
static int mxcfb_map_video_memory(struct fb_info *fbi, bool use_internal_ram)
{

#ifdef CONFIG_FB_MXC_INTERNAL_MEM
	if (use_internal_ram) {
		fbi->fix.smem_len = FB_RAM_SIZE;
		fbi->fix.smem_start = FB_RAM_BASE_ADDR;
	} else
#endif
	{
		fbi->fix.smem_len = (def_vram == 0)?fbi->var.xres_virtual *
#if defined(CONFIG_MOT_WFN394)
		    fbi->var.yres_virtual * (fbi->var.bits_per_pixel >> 3) :def_vram;
#else
		    fbi->var.yres_virtual * 4 :def_vram;
#endif
		fbi->fix.smem_start = ipu_malloc(fbi->fix.smem_len);
		printk("smem_length = 0x%08x, def_vram = 0x%08x\n",fbi->fix.smem_len, def_vram );
	}

	if (fbi->fix.smem_start == 0) {
		printk("MXCFB - Unable to allocate framebuffer memory\n");
		return -EBUSY;
	}
	DPRINTK("mxcfb: allocated fb @ paddr=0x%08X, size=%d.\n",
		(uint32_t) fbi->fix.smem_start, fbi->fix.smem_len);
#ifdef CONFIG_MOT_FEAT_IPU_MEM_ADDR
	if (!ipu_dynamic_pool && !request_mem_region(fbi->fix.smem_start, fbi->fix.smem_len, "LCD")) {
#else
	if (!request_mem_region(fbi->fix.smem_start, fbi->fix.smem_len, "LCD")) {
#endif /* CONFIG_MOT_FEAT_IPU_MEM_ADDR */

		return -EBUSY;
	}

	if (!(fbi->screen_base = ioremap(fbi->fix.smem_start,
					 fbi->fix.smem_len))) {
		release_mem_region(fbi->fix.smem_start, fbi->fix.smem_len);
		printk("MXCFB - Unable to map fb memory to virtual address\n");
		return -EIO;
	}
	fbi->screen_size = fbi->fix.smem_len;

#ifndef CONFIG_MOT_FEAT_POWERUP_LOGO
        /* Clear the screen only if IPU was not initialized by MBM 
         * Doing this will cause the logo be cleared 
         * This may not happen if IRAM is used for fb0, but other platforms
         * can be affected
         *
         * Prevent this only for the main framebuffer device(fb0)
         * NB: mxcfb_drv_data.fbi is set by the time this function is called
         */
        if(fbi != mxcfb_drv_data.fbi)
#endif
	/* Clear the screen */
	memset((char *)fbi->screen_base, 0, fbi->fix.smem_len);

	return 0;
}

/*!
 * De-allocates the DRAM memory for the frame buffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @return      Error code indicating success or failure
 */
static int mxcfb_unmap_video_memory(struct fb_info *fbi)
{
	iounmap(fbi->screen_base);
	release_mem_region(fbi->fix.smem_start, fbi->fix.smem_len);
#ifdef CONFIG_FB_MXC_INTERNAL_MEM
	if (fbi->fix.smem_start == FB_RAM_BASE_ADDR) {
		return 0;
	}
#endif
	ipu_free(fbi->fix.smem_start);
	return 0;
}

#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)
static void init_global_state(void)
{
	init_MUTEX(&mxcfb_global_state.g_sem);

	mxcfb_global_state.backlight_state = BKLIGHT_ON;
	mxcfb_global_state.brightness = DEFAULT_BRIGHTNESS;
	mxcfb_global_state.bklight_range.min = MIN_BRIGHTNESS;
	mxcfb_global_state.bklight_range.max = MAX_BRIGHTNESS;
#if defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT)
	mxcfb_global_state.dbuffer_enabled = DBUFFER_ENABLED;
#endif
}
#endif /* defined(CONFIG_MOT_FEAT_IPU_IOCTL) */

/*!
 * Initializes the framebuffer information pointer. After allocating
 * sufficient memory for the framebuffer structure, the fields are
 * filled with custom information passed in from the configurable
 * structures.  This includes information such as bits per pixel,
 * color maps, screen width/height and RGBA offsets.
 *
 * @return      Framebuffer structure initialized with our information
 */
static struct fb_info *mxcfb_init_fbinfo(struct device *dev, struct fb_ops *ops, int ovl)
{
	struct fb_info *fbi;
	struct mxcfb_info *mxcfbi;

	/*
	 * Allocate sufficient memory for the fb structure
	 */
	fbi = framebuffer_alloc(sizeof(struct mxcfb_info), dev);
	if (!fbi)
		return NULL;

	mxcfbi = (struct mxcfb_info *)fbi->par;

	/*
	 * Fill in fb_info structure information
	 */
	fbi->var.xres = fbi->var.xres_virtual = mxcfb_panel->width;
	fbi->var.yres = mxcfb_panel->height;
#ifdef CONFIG_FB_MXC_INTERNAL_MEM
	fbi->var.yres_virtual = mxcfb_panel->height;
	if (ovl) /* Overlay should always be double buffered */
		fbi->var.yres_virtual += mxcfb_panel->height;
#else
	fbi->var.yres_virtual = mxcfb_panel->height * 2;
#endif
	fbi->var.activate = FB_ACTIVATE_NOW;
	mxcfb_check_var(&fbi->var, fbi);

	mxcfb_set_fix(fbi);

	fbi->fbops = ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = mxcfbi->pseudo_palette;

	spin_lock_init(&mxcfbi->fb_lock);

	/*
	 * Allocate colormap
	 */
	fb_alloc_cmap(&fbi->cmap, 16, 0);

#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)
	init_global_state();
#endif

	return fbi;
}

#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
/*!
 * Probe routine for the CLI framebuffer driver. It is called during the
 * driver binding process.  The following functions are performed in
 * this routine: Framebuffer initialization, Memory allocation and
 * mapping, and Framebuffer registration
 * 
 * @param 	dev 	Pointer to the MXCFB framebuffer device 
 *
 * @return      Appropriate error code to the kernel common code
 */
static int mxcfb_cli_probe(struct device *dev)
{
        struct fb_info *fbi;
        int ret = 0;
        struct mxcfb_info * mxc_fbi;

        FUNC_START;

        fbi = mxcfb_init_fbinfo(dev, &mxcfb_cli_ops, 0);

        if (!fbi) {
                ret = -ENOMEM;
                goto err0;
        }

        mxcfb_drv_data.fbi_cli = fbi;
	mxcfb_drv_data.cli_active = 0;
	mxc_fbi = (struct mxcfb_info *)fbi->par;
        mxc_fbi->open_count = 0;

        /*
         * Allocate memory
         */
        ret = mxcfb_map_video_memory(fbi, false);
        if (ret < 0) {
                goto err1;
        }

        /*
         * Register framebuffer
         */
        ret = register_framebuffer(fbi);
        if (ret < 0) {
                goto err2;
        }
        
        FUNC_END;
        return 0;
        
err2:
        mxcfb_unmap_video_memory(fbi);
err1:
        if (&fbi->cmap)
                fb_dealloc_cmap(&fbi->cmap);
        framebuffer_release(fbi);
err0:
        return ret;
}
#endif /* defined(CONFIG_MOT_FEAT_EMULATED_CLI) */


#if defined(CONFIG_MOT_FEAT_DISABLE_SW_CURSOR)
static void dont_flash_cursor(void *priv)
{
        /*
         * This is a dummy function so that fbconsole
         * doesnt try to use its timer-based cursor flashing function.
         * This function should not get called
         */
        BUG();
}
#endif

/*!
 * Probe routine for the framebuffer driver. It is called during the
 * driver binding process.      The following functions are performed in
 * this routine: Framebuffer initialization, Memory allocation and
 * mapping, Framebuffer registration, IPU initialization.
 *
 * @return      Appropriate error code to the kernel common code
 */
static int mxcfb_probe(struct device *dev)
{
	struct fb_info *fbi;
#ifdef CONFIG_FB_MXC_OVERLAY
	struct fb_info *fbi_ovl;
#endif
	int ret = 0;

	FUNC_START;

	/*
	 * Initialize FB structures
	 */
	fbi = mxcfb_init_fbinfo(dev, &mxcfb_ops, 0);
	if (!fbi) {
		ret = -ENOMEM;
		goto err0;
	}
#if defined(CONFIG_MOT_FEAT_DISABLE_SW_CURSOR)
	/*
	 * Define a dummy function so that fbconsole doesnt try to use
	 * its own timer-based cursor flashing function
	 * This is to disable fbconsole displaying software cursor
	 */
	fbi->queue.func = dont_flash_cursor;
#endif
	mxcfb_drv_data.fbi = fbi;
	mxcfb_drv_data.backlight_level = 255;
	mxcfb_drv_data.suspended = false;
	init_waitqueue_head(&mxcfb_drv_data.suspend_wq);

	/*
	 * Allocate memory
	 */
	ret = mxcfb_map_video_memory(fbi, true);
	if (ret < 0) {
		goto err1;
	}

	/*
	 * Register framebuffer
	 */
	ret = register_framebuffer(fbi);
	if (ret < 0) {
		goto err2;
	}

#ifdef CONFIG_FB_MXC_OVERLAY
	/*
	 * Initialize Overlay FB structures
	 */
        fbi_ovl = mxcfb_init_fbinfo(dev, &mxcfb_ovl_ops, 1);
	if (!fbi_ovl) {
		ret = -ENOMEM;
		goto err3;
	}
	mxcfb_drv_data.fbi_ovl = fbi_ovl;

	ret = mxcfb_map_video_memory(fbi_ovl, false);
	if (ret < 0) {
		goto err4;
	}

	/*
	 * Register overlay framebuffer
	 */
	ret = register_framebuffer(fbi_ovl);
	if (ret < 0) {
		goto err5;
	}
#else
	mxcfb_drv_data.fbi_ovl = NULL;
#endif
	dev_set_drvdata(dev, &mxcfb_drv_data);

#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
	mxcfb_drv_data.fbi_cli = NULL;
	mxcfb_cli_probe(dev);
#endif

#ifdef NEW_MBX
	init_waitqueue_head(&mxcfb_drv_data.vsync_wq);
	if ((ret = ipu_request_irq(IPU_IRQ_SDC_DISP3_VSYNC,
				   mxcfb_vsync_irq_handler,
				   0, MXCFB_NAME, &mxcfb_drv_data)) < 0) {
		goto err6;
	}
	ipu_disable_irq(IPU_IRQ_SDC_DISP3_VSYNC);
#endif

#if defined(CONFIG_MOT_FEAT_BOOT_BACKLIGHT)
#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
        gpio_lcd_backlight_enable(true);
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */
#endif /* CONFIG_MOT_FEAT_BOOT_BACKLIGHT */

	FUNC_END;
	return 0;

      err6:
#ifdef CONFIG_FB_MXC_OVERLAY
	unregister_framebuffer(fbi_ovl);
      err5:
	mxcfb_unmap_video_memory(fbi_ovl);
      err4:
	if (&fbi_ovl->cmap)
		fb_dealloc_cmap(&fbi_ovl->cmap);
	framebuffer_release(fbi_ovl);
      err3:
#endif
	unregister_framebuffer(fbi);

      err2:
	mxcfb_unmap_video_memory(fbi);
      err1:
	if (&fbi->cmap)
		fb_dealloc_cmap(&fbi->cmap);
	framebuffer_release(fbi);
      err0:
	return ret;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxcfb_driver = {
	.name = MXCFB_NAME,
	.bus = &platform_bus_type,
	.probe = mxcfb_probe,
	.suspend = mxcfb_suspend,
	.resume = mxcfb_resume,
};

/*!
 * Device definition for the Framebuffer
 */
static struct platform_device mxcfb_device = {
	.name = MXCFB_NAME,
	.id = 0,
};

#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
/*
 * Set up "emulated_cli" as a driver attribute.
 */
static ssize_t show_emulated_cli(struct device_driver *drv, char *buf)
{
	strncpy(buf, mxcfb_drv_data.cli_active ?
                "CLI Emulation Mode: ON\n" :
                "CLI Emulation Mode: OFF\n", PAGE_SIZE);
	return strnlen(buf, PAGE_SIZE) + 1;
}
static ssize_t store_emulated_cli(struct device_driver *drv, const char *buf, size_t count) 
{
	int retval;
	const char * on = "on";
	const char * off = "off";

	if (strnicmp(buf, on, strnlen(on, PAGE_SIZE)) == 0) {
		retval = enable_emulated_cli();
	} else if (strnicmp(buf, off, strnlen(off, PAGE_SIZE)) == 0) {
		retval = disable_emulated_cli();
	} else {
		retval = -EINVAL;
	}
	
	return (retval ? retval : count);
}
static DRIVER_ATTR(emulated_cli, S_IRUGO|S_IWUGO, show_emulated_cli, store_emulated_cli);
#endif /* defined(CONFIG_MOT_FEAT_EMULATED_CLI) */


int __init mxcfb_setup(char *options)
{
        char *this_opt = NULL;

        if (!options || !*options)
                return -1;

	printk("mxcfb_setup: %s\n",options);

        while ((this_opt = strsep(&options, ",")) != NULL) {
                if (!strncmp(this_opt, "vram:", 5)) {
                        char *suffix;
                        def_vram = (simple_strtoul(this_opt + 5, &suffix, 0));
                        switch (suffix[0]) {
                        case 'm':
                        case 'M':
                                def_vram *= 1024 * 1024;
                                break;
                        case 'k':
                        case 'K':
                                def_vram *= 1024;
                                break;
                        default:
				;
			}
                }
                
	}
	return 0;
}
/*!
 * Main entry function for the framebuffer. The function registers the power
 * management callback functions with the kernel and also registers the MXCFB
 * callback functions with the core Linux framebuffer driver \b fbmem.c
 *
 * @return      Error code indicating success or failure
 */
int __init mxcfb_init(void)
{
	int ret = 0;
#ifndef MODULE
	char *option = NULL;
#endif
	FUNC_START;

#ifndef MODULE
	if (fb_get_options("mxcfb", &option))
		return -ENODEV;
	mxcfb_setup(option);
#endif

	ret = driver_register(&mxcfb_driver);
#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
	driver_create_file(&mxcfb_driver, &driver_attr_emulated_cli);
#endif
	if (ret == 0) {
		ret = platform_device_register(&mxcfb_device);
		if (ret != 0) {
			driver_unregister(&mxcfb_driver);
		}
	}

	return ret;
}

void mxcfb_exit(void)
{
	struct fb_info *fbi = mxcfb_drv_data.fbi;

	if (fbi) {
		mxcfb_unmap_video_memory(fbi);

		if (&fbi->cmap)
			fb_dealloc_cmap(&fbi->cmap);

		unregister_framebuffer(fbi);
		framebuffer_release(fbi);
	}

	fbi = mxcfb_drv_data.fbi_ovl;
	if (fbi) {
		mxcfb_unmap_video_memory(fbi);

		if (&fbi->cmap)
			fb_dealloc_cmap(&fbi->cmap);

		unregister_framebuffer(fbi);
		framebuffer_release(fbi);
	}

#if defined(CONFIG_MOT_FEAT_EMULATED_CLI)
	fbi = mxcfb_drv_data.fbi_cli;
	if (fbi) {
		mxcfb_unmap_video_memory(fbi);

		if (&fbi->cmap)
			fb_dealloc_cmap(&fbi->cmap);

		unregister_framebuffer(fbi);
		framebuffer_release(fbi);
	}
#endif

#ifdef NEW_MBX
	ipu_free_irq(IPU_IRQ_SDC_DISP3_VSYNC, &mxcfb_drv_data);
#endif

	platform_device_unregister(&mxcfb_device);
	driver_unregister(&mxcfb_driver);
}

module_init(mxcfb_init);
module_exit(mxcfb_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC framebuffer driver");
MODULE_SUPPORTED_DEVICE("fb");
