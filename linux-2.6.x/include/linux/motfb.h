/*
 * motfb.h
 *
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright 2006, 2008 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Initial version.  Added Motorola specific IOCTL
 *                    interfaces and type definitions for both the MXCFB and
 *                    MXCFB_HVGA framebuffer drivers.
 * 04/2007  Motorola  Removed support for HVGA driver
 * 07/2008  Motorola  Added APP coredump prompt support
 */

#ifndef __LINUX_MOTFB_H__
#define __LINUX_MOTFB_H__

/* Panic text on display */
#if !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_FB_PANIC_TEXT)
#if defined(CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY)
/* Limit of 32 characters is to prompt more specific coredump */
#define PANIC_MAX_STR_LEN       32
#else
#define PANIC_MAX_STR_LEN       16
#endif /* defined(CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY) */
#define FBIOPANICTEXT		_IOW('M', 0xb3, char *)
#endif

#if !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_IPU_IOCTL)

typedef enum bklight_state {
	BKLIGHT_OFF,
	BKLIGHT_ON,
} bklight_state_t;

#define MIN_BRIGHTNESS		0
#define MAX_BRIGHTNESS		100
#define DEFAULT_BRIGHTNESS	MAX_BRIGHTNESS

struct backlight_brightness_range {
	unsigned long min;
	unsigned long max;
};

/* Mask for ON/OFF of the MAIN and CLI panels */
typedef enum panel {
	PANEL_OFF = 0,
	MAIN_PANEL = 1,
	CLI_PANEL = 2,
} panel_t;

/* Returns the type of display with respect to backlight brightness requirement i.e.,
 * TRANSMISSIVE : A TRANSMISSIVE display cannot be seen without backlight
 * TRANSFLECTIVE : A TRANSFLECTIVE display can still be seen without backlight
 * */
typedef enum {
        TRANSMISSIVE_DISPLAY,
        TRANSFLECTIVE_DISPLAY,
} display_type_t;

/* added for backlight control */
#define FBIOSETBKLIGHT          _IOW('M', 0xb4, size_t)
#define FBIOGETBKLIGHT          _IOR('M', 0xb5, size_t)
#define FBIOSETBRIGHTNESS       _IOW('M', 0xb6, __u8)
#define FBIOGETBRIGHTNESS       _IOR('M', 0xb7, __u8)
#define FBIOSET_BRIGHTNESSRANGE _IOW('M', 0xb8, struct backlight_brightness_range)

#endif				/* !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_IPU_IOCTL) */

#if !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT)
#include <linux/fb.h>		/* needed for struct fb_fix_screeninfo definition */

struct fb_ovl2_mapinfo {
	unsigned long ovl2_offset;
	unsigned long ovl2_size;
	unsigned long ovl2_priv;
	unsigned long ovl2_framelen;
	unsigned long aylen;
	unsigned long acblen;
	unsigned long acrlen;
};
#define FBIOENABLE2BFS          _IO( 'M', 0xb9)
#define FBIODISABLE2BFS         _IO( 'M', 0xba)
#define FBIOCHECK2BFS           _IOR('M', 0xbb, unsigned long)
#define FBIOCKMAINVALIDFB       _IO( 'M', 0xbc)
#define FBIOENABLEOVL2          _IOR('M', 0xbd, __u8)
#define FBIODISABLEOVL2         _IOR('M', 0xbe, __u8)
#define FBIOGET_OVL2_MAPINFO    _IOR('M', 0xbf, struct fb_ovl2_mapinfo)
#define FBIOGET_OVL2FIX		_IOR('M', 0xc0, struct fb_fix_screeninfo)
#define FBIOGET_OVL2VAR	        _IOR('M', 0xc1, struct fb_var_screeninfo)
#define FBIOPUT_OVL2VAR	        _IOW('M', 0xc2, struct fb_var_screeninfo)
#define FBIOPAN_DISPLAY_OVL2    _IOW('M', 0xc3, struct fb_var_screeninfo)
#define FBIOADJUST_TRANS	_IOW('M', 0xc4, unsigned short)
#if !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_EMULATED_CLI)
#define FBIOSMARTUPDATE         _IO( 'M', 0xd0)
#define FBIOENTERCLIPWRSAVE     _IO( 'M', 0xd1)
#define FBIOEXITCLIPWRSAVE      _IO( 'M', 0xd2)
#endif /* !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_EMULATED_CLI) */
#endif /* !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT) */
#if !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_EMULATED_CLI)
#define FBIOENABLE_EMULATEDCLI  _IO( 'M', 0xd3)
#define FBIODISABLE_EMULATEDCLI _IO( 'M', 0xd4)
#endif
#if !defined(__KERNEL__) || defined(CONFIG_MOT_FEAT_IPU_IOCTL)
#define FBIO_QUERY_DISPLAY_TYPE _IOR('M', 0xd5, display_type_t)
#endif

#if defined(__KERNEL__)

#if defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT)
#define DBUFFER_ENABLED  	true
#define DBUFFER_DISABLED	false
#endif

#if defined(CONFIG_MOT_FEAT_IPU_IOCTL)

/* Masks for brightness setting */
#define BACKLIGHT_ON		0x1
#define DISPLAY_ON    		0x2

/* Holds global information about the state of the panel */
struct global_state {
	struct semaphore g_sem; /* This mutex must be held whenever modifying global_state */
	uint32_t brightness; /* Current brightness setting of the display */
	bklight_state_t backlight_state; /* Backlight ON/OFF state */
	struct backlight_brightness_range bklight_range; /* MIN and MAX backlight brightness levels */
#if defined(CONFIG_MOT_FEAT_IPU_IOCTL_EZX_COMPAT)
	unsigned char dbuffer_enabled; /* True if Double Buffering is enabled */
#endif
};

#endif				/* defined(CONFIG_MOT_FEAT_IPU_IOCTL) */
#endif				/* defined(__KERNEL__) */
#endif				/* __LINUX_MOTFB_H__ */
