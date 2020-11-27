/*
 * Copyright (C) 2008 Motorola, Inc.
*/
   
/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 *
 * Date     Author    Comment
 * 05/2008  Motorola  Add new support for powerup logo
 * 06/2008  Motorola  Remove two test functions
 * 07/2008  Motorola  Support 32bit color display
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
#include <linux/lights_backlight.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/mot-gpio.h>
#include <linux/console.h>
#include <linux/motfb.h>
#include <linux/power_ic.h>
#include <asm/arch/mxc_pm.h>

#include "../../mxc/ipu/ipu.h"
#include "../../mxc/ipu/ipu_regs.h"
#include "mxcfb_gd2.h"

/*****************************************************************************/
#define MXCFB_NAME      "MXCFB"

/*
 * Debug Macros
 */
#define MXCFB_DEBUG
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

struct mxcfb_data mxcfb_drv_data;
uint32_t def_vram = 0;

static int mxcfb_blank(int blank, struct fb_info *fbi);
extern void mxcfb_fill_memory(struct fb_info *fbi, int color);
extern void mxcfb_fill_panel(int color);
extern void mxcfb_init_panel(int disp);
extern void mxcfb_init_ipu();
extern void mxcfb_set_refresh(struct fb_info *fbi, int mode);

extern void gpio_lcd_display_reset(enum gpio_signal_assertion asserted);
extern void gpio_lcd_serializer_reset(enum gpio_signal_assertion asserted);
extern void gpio_lcd_serializer_stby(enum gpio_signal_assertion asserted);

extern u32 mot_mbm_is_ipu_initialized;
extern u32 mot_mbm_ipu_buffer_address;

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

#if defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT)
#include <linux/console.h>	/* acquire_console_sem() */
static struct global_state mxcfb_global_state;
static LIGHTS_BACKLIGHT_IOCTL_T backlight_set;
#endif

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


#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
extern u32 mot_mbm_is_ipu_initialized;
extern u32 mot_mbm_ipu_buffer_address;
#endif

/*****************************************************************************/
static int mxcfb_open(struct fb_info *fbi, int user)
{
	int retval = 0;
	struct mxcfb_info * mxc_fbi = (struct mxcfb_info *)fbi->par;
#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
    char __iomem* io_remapped_logo;
#endif

	FUNC_START;
	
	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq, (mxcfb_drv_data.suspended == false))) < 0)
	{
		return retval;
	}
	
	if (mxc_fbi->open_count == 0)
	{
		if(fbi == mxcfb_drv_data.fbi) 
			retval = mxcfb_blank(FB_BLANK_UNBLANK, fbi);
		mxc_fbi->open_count++;

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
	}
	
	FUNC_END;
	return retval;
}

/*****************************************************************************/
static int mxcfb_release(struct fb_info *fbi, int user)
{
	int retval = 0;/*
	struct mxcfb_info * mxc_fbi = (struct mxcfb_info *)fbi->par;
	FUNC_START;
	
	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq, (mxcfb_drv_data.suspended == false))) < 0)
	{
		return retval;
	}
	
	BUG_ON(mxc_fbi->open_count <= 0);
	--mxc_fbi->open_count;
	
	if (mxc_fbi->open_count == 0)
	{
//		if(fbi == mxcfb_drv_data.fbi_ovl)
			retval = mxcfb_blank(FB_BLANK_POWERDOWN, fbi);	
	}
	
	FUNC_END;*/
	return retval;	
}

/*****************************************************************************/
static int mxcfb_set_fix(struct fb_info *info)
{
	struct fb_fix_screeninfo *fix = &info->fix;
	struct fb_var_screeninfo *var = &info->var;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;
	FUNC_START;
	
	strcpy(fix->id, "DISP0 FB");
	fix->id[4] = '0' + mxc_fbi->disp_num;
	
	fix->line_length = GD2_SCREEN_WIDTH * var->bits_per_pixel / 8;
	
	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->xpanstep = 0;
	fix->ypanstep = 0;
	
	FUNC_END;
	return 0;
}

/*****************************************************************************/
static int mxcfb_set_par(struct fb_info *info)
{
	int retval;
	struct mxcfb_info * mxc_fbi = (struct mxcfb_info *)info->par;
	FUNC_START;
	
	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq, (mxcfb_drv_data.suspended == false))) < 0)
	{
		return retval;
	}
	
	//
	// Set Parameter code goes here if its ever really needed. (So far no-one seems to use it)
	//
	
	FUNC_END;
	return 0;
}

/*****************************************************************************/
static int mxcfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	FUNC_START;
	
	var->xres = GD2_SCREEN_WIDTH;
	var->yres = GD2_SCREEN_HEIGHT;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;
#if defined(CONFIG_MOT_FEAT_32_BIT_DISPLAY)	
	var->bits_per_pixel = 32;
#else
	var->bits_per_pixel = 24;
#endif/* CONFIG_MOT_FEAT_32_BIT_DISPLAY */
	var->red.length = 8;
	var->red.offset = 16;
	var->red.msb_right = 0;
	
	var->green.length = 8;
	var->green.offset = 8;
	var->green.msb_right = 0;
	
	var->blue.length = 8;
	var->blue.offset = 0;
	var->blue.msb_right = 0;
	
	var->transp.length = 0;
	var->transp.offset = 0;
	var->transp.msb_right = 0;
	
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

/*****************************************************************************/
static inline u_int _chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

/*****************************************************************************/
static int mxcfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue, u_int trans, struct fb_info *fbi)
{
	unsigned int val;
	int ret = 1;
	
	/*
	* If greyscale is true, then we convert the RGB value
	* to greyscale no matter what visual we are using.
	* Y (luminance) =  0.299 * R + 0.587 * G + 0.114 * B 
	*/
	if (fbi->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green + 7471 * blue) >> 16;
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

/*****************************************************************************/
static int mxcfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;
	FUNC_START;
	retval = 0;
	FUNC_END;
	return 0;
}

/*****************************************************************************/
static int mxcfb_blank(int blank, struct fb_info *fbi)
{
	int retval;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;
	FUNC_START;
	
	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq, (mxcfb_drv_data.suspended == false))) < 0) {
		return retval;
	}
	
	mxc_fbi->blank = blank;
	
	switch (blank) {
	case FB_BLANK_POWERDOWN:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
		//mxcfb_set_refresh(fbi, MXCFB_REFRESH_OFF);
		break;
	case FB_BLANK_UNBLANK:		
		//mxcfb_set_refresh(fbi, MXCFB_REFRESH_AUTO);
		break;
	}
	
	FUNC_END;
	return 0;
}

/*****************************************************************************/
static int mxcfb_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg, struct fb_info *fbi)
{
	int retval = 0;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
		(mxcfb_drv_data.suspended ==
		false))) < 0) {
			return retval;
	}

	switch (cmd) {
		/*#if defined(CONFIG_FB_MXC_OVERLAY)
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
		#endif*/
		/*#ifdef NEW_MBX
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
		#endif*/
case MXCFB_SET_BRIGHTNESS:
	{
		uint8_t level;
		if (copy_from_user(&level, (void *)arg, sizeof(level))) {
			retval = -EFAULT;
			break;
		}

		mxcfb_drv_data.backlight_level = level;
		//retval = ipu_sdc_set_brightness(level);
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
#else
	/* Ascension does not have a separate GPIO to control
	* backlight on/off */
	lights_backlightset(LIGHTS_BACKLIGHT_DISPLAY, 0);
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */
	mxcfb_global_state.backlight_state &= ~BKLIGHT_ON;
	MXCFB_UP(&mxcfb_global_state.g_sem);
	break;
case BKLIGHT_ON:
	MXCFB_DOWN_INTERRUPTIBLE(&mxcfb_global_state.g_sem);
#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
	gpio_lcd_backlight_enable(true);
#else
	/* Ascension does not have a separate GPIO to control
	* backlight on/off */
	lights_backlightset(LIGHTS_BACKLIGHT_DISPLAY,
		mxcfb_global_state.brightness);
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
#if !defined(CONFIG_MACH_SCMA11REF)
		/* Do not change the brightness if the backlight state is
		* currently off */
		if (mxcfb_global_state.backlight_state & BKLIGHT_ON) {
#endif /* !CONFIG_MACH_SCMA11REF */
			lights_backlightset(backlight_set.bl_select,
				backlight_set.bl_brightness);
#if !defined(CONFIG_MACH_SCMA11REF)
		}
#endif /* !CONFIG_MACH_SCMA11REF */
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

/*****************************************************************************/
#ifdef CONFIG_MOT_FEAT_DISABLE_SW_CURSOR
static int disable_sw_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	return 0;
}
#endif

/*****************************************************************************/
static struct fb_ops mxcfb_ops = {
	.owner				= THIS_MODULE,
		.fb_open			= mxcfb_open,
		.fb_release			= mxcfb_release,
		.fb_set_par			= mxcfb_set_par,
		.fb_check_var		= mxcfb_check_var,
		.fb_setcolreg		= mxcfb_setcolreg,
		.fb_pan_display		= mxcfb_pan_display,
		.fb_fillrect		= cfb_fillrect,
		.fb_copyarea		= cfb_copyarea,
		.fb_imageblit		= cfb_imageblit,
		.fb_blank			= mxcfb_blank,
		.fb_ioctl			= mxcfb_ioctl,
		.fb_cursor			= disable_sw_cursor,
};

/*****************************************************************************/
static int mxcfb_map_video_memory(struct fb_info *fbi)
{
	fbi->fix.smem_len = (def_vram == 0)?fbi->var.xres_virtual * fbi->var.yres_virtual * (fbi->var.bits_per_pixel >> 3) :def_vram;
	fbi->fix.smem_start = ipu_malloc(fbi->fix.smem_len);
	 
	if (fbi->fix.smem_start == 0) 
	{
		printk("MXCFB - Unable to allocate framebuffer memory\n");
		return -EBUSY;
	}
	if (!ipu_dynamic_pool && !request_mem_region(fbi->fix.smem_start, fbi->fix.smem_len, "LCD")) 
	{		
		return -EBUSY;
	}
	if (!(fbi->screen_base = ioremap(fbi->fix.smem_start, fbi->fix.smem_len))) 
	{
		release_mem_region(fbi->fix.smem_start, fbi->fix.smem_len);
		printk("MXCFB - Unable to map fb memory to virtual address\n");
		return -EIO;
	}

	fbi->screen_size = fbi->fix.smem_len;

	memset((char *)fbi->screen_base, 0xFF, fbi->fix.smem_len);

	return 0;
}

/*****************************************************************************/
static int mxcfb_unmap_video_memory(struct fb_info *fbi)
{
	iounmap(fbi->screen_base);
	release_mem_region(fbi->fix.smem_start, fbi->fix.smem_len);
	ipu_free(fbi->fix.smem_start);
	return 0;
}

/*****************************************************************************/
static struct fb_info *mxcfb_init_fbinfo(struct device *dev, struct fb_ops *ops)
{
	struct fb_info *fbi;
	struct mxcfb_info *mxcfbi;
	
	fbi = framebuffer_alloc(sizeof(struct mxcfb_info), dev);
	if (!fbi)
		return NULL;
	
	mxcfbi = (struct mxcfb_info *)fbi->par;

	fbi->var.xres = fbi->var.xres_virtual = GD2_SCREEN_WIDTH;
	fbi->var.yres = fbi->var.yres_virtual = GD2_SCREEN_HEIGHT;
	
	fbi->var.activate = FB_ACTIVATE_NOW;
	mxcfb_check_var(&fbi->var, fbi);
	mxcfb_set_fix(fbi);
	
	fbi->fbops                   = ops;
	fbi->flags                   = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette          = mxcfbi->pseudo_palette;
	
	spin_lock_init(&mxcfbi->fb_lock);
	fb_alloc_cmap(&fbi->cmap, 16, 0);
	
#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)
	init_global_state();
#endif

	return fbi; 
}

/*****************************************************************************/
#if defined(CONFIG_MOT_FEAT_DISABLE_SW_CURSOR)
static void dont_flash_cursor(void *priv)
{
	BUG();
}
#endif

/*****************************************************************************/
static struct workqueue_struct *rotate0_workqueue;
static void rotate0_work_handler(void * data)
{
	uint32_t param[2];
	param[0] = 0x40;
	//mxcfb_set_refresh(mxcfb_drv_data.fbi, MXCFB_REFRESH_OFF);
	ipu_adc_write_cmd(DISP0, CMD, MADCTL, param, 1);
	//mxcfb_set_refresh(mxcfb_drv_data.fbi, MXCFB_REFRESH_AUTO);
}
static DECLARE_WORK(rotate0_work, rotate0_work_handler, NULL);

static struct workqueue_struct *rotate180_workqueue;
static void rotate180_work_handler(void * data)
{
	uint32_t param[2];
	param[0] = 0x80;
	//mxcfb_set_refresh(mxcfb_drv_data.fbi, MXCFB_REFRESH_OFF);
	ipu_adc_write_cmd(DISP0, CMD, MADCTL, param, 1);
	//mxcfb_set_refresh(mxcfb_drv_data.fbi, MXCFB_REFRESH_AUTO);
}
static DECLARE_WORK(rotate180_work, rotate180_work_handler, NULL);

void rotate_screen(int rot)
{
	if (rot==0)
		queue_work(rotate0_workqueue, &rotate0_work);
	else
		queue_work(rotate180_workqueue, &rotate180_work);
}

/*****************************************************************************/
static int mxcfb_probe(struct device *dev)
{
	int retval;
	struct fb_info *fbi;
	struct fb_info *fbi_ovl;
	FUNC_START;
	
	fbi = mxcfb_init_fbinfo(dev, &mxcfb_ops);	
	if (!fbi) {										
		retval = -ENOMEM;								
		goto err0;									
	}		
	fbi->queue.func = dont_flash_cursor;	
	mxcfb_drv_data.fbi = fbi;
	retval = mxcfb_map_video_memory(fbi);
	if (retval < 0) {
		retval = -ENOMEM;
		goto err1;
	}

	//mxcfb_init_ipu();

	//gpio_lcd_serializer_stby(DEASSERT_GPIO_SIGNAL);
	//ipu_adc_write_cmd(DISP0, CMD, 0, 0, 0);	//dummy command
	//msleep(10);
	//gpio_lcd_display_reset(ASSERT_GPIO_SIGNAL);
	//gpio_lcd_serializer_reset(ASSERT_GPIO_SIGNAL);
	//ipu_adc_write_cmd(DISP0, CMD, 0, 0, 0);	//dummy command
	//msleep(10);
	//gpio_lcd_serializer_reset(DEASSERT_GPIO_SIGNAL);
	//gpio_lcd_display_reset(DEASSERT_GPIO_SIGNAL);
	//ipu_adc_write_cmd(DISP0, CMD, 0, 0, 0);	//dummy command
	//msleep(10);

	//mxcfb_init_panel(DISP0);

	retval = register_framebuffer(fbi);
	if (retval < 0) {
		goto err2;
	}

fbi_ovl = mxcfb_init_fbinfo(dev, &mxcfb_ops);	
	if (!fbi_ovl) {										
		retval = -ENOMEM;								
		goto err3;									
	}		
	fbi_ovl->queue.func = dont_flash_cursor;	
	mxcfb_drv_data.fbi_ovl = fbi_ovl;
#if defined(CONFIG_MOT_FEAT_32_BIT_DISPLAY) 
	fbi_ovl->var.bits_per_pixel = 24;
#endif/* CONFIG_MOT_FEAT_32_BIT_DISPLAY */
	retval = mxcfb_map_video_memory(fbi_ovl);
	if (retval < 0) {
		retval = -ENOMEM;
		goto err4;
	}

	retval = register_framebuffer(fbi_ovl);
	if (retval < 0) {
		goto err5;
	}

	dev_set_drvdata(dev, &mxcfb_drv_data);


	//mxc_pm_setup_lcd_callback(mxcfb_dvfs_handler); // greg

#if defined(CONFIG_MOT_FEAT_BOOT_BACKLIGHT)
#if defined(CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD)
	gpio_lcd_backlight_enable(true);
#else
	/* Ascension does not have a separate GPIO to control backlight on/off */
	lights_backlightset(LIGHTS_BACKLIGHT_DISPLAY,
		mxcfb_global_state.bklight_main_brightness);
#endif /* CONFIG_MOT_FEAT_GPIO_API_LIGHTING_LCD */
#endif /* CONFIG_MOT_FEAT_BOOT_BACKLIGHT */

	FUNC_END;
	return 0;

err5:
	mxcfb_unmap_video_memory(fbi_ovl);
err4:
	if (fbi_ovl) {
		fb_dealloc_cmap(&fbi_ovl->cmap);
		framebuffer_release(fbi_ovl);
	}
err3:
	unregister_framebuffer(fbi);
err2:
	mxcfb_unmap_video_memory(fbi);
err1:
	if (fbi) {
		fb_dealloc_cmap(&fbi->cmap);
		framebuffer_release(fbi);
	}
err0:
	return retval;
}

/*****************************************************************************/
static int mxcfb_suspend(struct device *dev, u32 state, u32 level)
{
	struct mxcfb_data *drv_data = dev_get_drvdata(dev);
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)drv_data->fbi->par;
	FUNC_START;
	switch (level) {
	case SUSPEND_DISABLE:
		break;
	case SUSPEND_SAVE_STATE:
		break;
	case SUSPEND_POWER_DOWN:
		break;
	}
	FUNC_END;
	return 0;
}

/*****************************************************************************/
static int mxcfb_resume(struct device *dev, u32 level)
{
	struct mxcfb_data *drv_data = dev_get_drvdata(dev);
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)drv_data->fbi->par;
	FUNC_START;
	switch (level) {
	case RESUME_POWER_ON:
		break;
	case RESUME_ENABLE:
		break;
	}
	FUNC_END;
	return 0;
}

/*****************************************************************************/
static struct device_driver mxcfb_driver = {
	.name			= MXCFB_NAME,
	.bus			= &platform_bus_type,
	.probe			= mxcfb_probe,
	.suspend		= mxcfb_suspend,
	.resume			= mxcfb_resume,
};
static struct platform_device mxcfb_device = {
	.name			= MXCFB_NAME,
	.id				= 0,
};

/*****************************************************************************/
static ssize_t store_disfill(struct device_driver *drv, const char *buf, size_t count) 
{
	int retval = 0;
	int testval;
	if (sscanf(buf, "%u", &testval)!=1)
		return -EINVAL; //Invalid String
	mxcfb_fill_memory(mxcfb_drv_data.fbi, testval);
	return (retval ? retval : count);
}
static DRIVER_ATTR(disfill, S_IRUGO|S_IWUGO, NULL, store_disfill);

static ssize_t store_disgo(struct device_driver *drv, const char *buf, size_t count) 
{
	int retval = 0;
	int testval;
	if (sscanf(buf, "%u", &testval)!=1)
		return -EINVAL; //Invalid String
	mxcfb_set_refresh(mxcfb_drv_data.fbi, MXCFB_REFRESH_AUTO);
	return (retval ? retval : count);
}
static DRIVER_ATTR(disgo, S_IRUGO|S_IWUGO, NULL, store_disgo);

static ssize_t store_disno(struct device_driver *drv, const char *buf, size_t count) 
{
	int retval = 0;
	int testval;
	if (sscanf(buf, "%u", &testval)!=1)
		return -EINVAL; //Invalid String
	mxcfb_set_refresh(mxcfb_drv_data.fbi, MXCFB_REFRESH_OFF);
	return (retval ? retval : count);
}
static DRIVER_ATTR(disno, S_IRUGO|S_IWUGO, NULL, store_disno);
/*****************************************************************************/
int __init mxcfb_setup(char *options)
{
	char *this_opt = NULL;
	if (!options || !*options)
		return -1;
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

/*****************************************************************************/
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
	driver_create_file(&mxcfb_driver, &driver_attr_disfill);
	driver_create_file(&mxcfb_driver, &driver_attr_disgo);
	driver_create_file(&mxcfb_driver, &driver_attr_disno);
	if (ret == 0) {
		ret = platform_device_register(&mxcfb_device);
		if (ret != 0) {
			driver_unregister(&mxcfb_driver);
		}
	}
	FUNC_END;
	return ret;
}

/*****************************************************************************/
void mxcfb_exit(void)
{
	struct mxcfb_data * drvdata = dev_get_drvdata(&mxcfb_device.dev);
	struct fb_info *fbi = drvdata->fbi;

	if (fbi) {
		mxcfb_unmap_video_memory(fbi);
		if (&fbi->cmap)
			fb_dealloc_cmap(&fbi->cmap);
		unregister_framebuffer(fbi);
		framebuffer_release(fbi);
	}

	flush_workqueue(rotate0_workqueue);
	destroy_workqueue(rotate0_workqueue);
	flush_workqueue(rotate180_workqueue);
	destroy_workqueue(rotate180_workqueue);

	platform_device_unregister(&mxcfb_device);
	driver_unregister(&mxcfb_driver);
}

/*****************************************************************************/
module_init(mxcfb_init);
module_exit(mxcfb_exit);

EXPORT_SYMBOL(rotate_screen);

MODULE_AUTHOR("Motorola");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MXC GD2 framebuffer driver");
MODULE_SUPPORTED_DEVICE("fb");
