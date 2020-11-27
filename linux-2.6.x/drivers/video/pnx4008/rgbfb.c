/*
 * drivers/video/pnx4008/sdum.c
 *
 * PNX4008's framebuffer support
 *
 * Author: Grigory Tolstolytkin <gtolstolytkin@ru.mvista.com>
 * Based on Philips Semiconductors's code
 *
 * Copyrght (c) 2005 MontaVista Software, Inc.
 * Copyright (c) 2005 Philips Semiconductors
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/fb.h>
#include <linux/init.h>

#include "sdum.h"
#include "fbcommon.h"

#define DEV_ID	( (int) &rgbfb_init )

MODULE_LICENSE("GPL");
static u32 colreg[16];

extern u32 rgb_lcd_video_start;
extern u32 rgb_lcd_video_size;
extern u32 lcd_phys_video_start;

static int rgbfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			   u_int transp, struct fb_info *info);

static int rgbfb_open(struct fb_info *info, int user);
static int rgbfb_release(struct fb_info *info, int user);
static int rgbfb_mmap(struct fb_info *info, struct file *file,
		      struct vm_area_struct *vma);

int __init rgbfb_init(void);
void __exit rgbfb_exit(void);

int __init rgbfb_probe(struct device *device);
static int rgbfb_remove(struct device *device);

static int no_cursor(struct fb_info *info, struct fb_cursor *cursor);

static struct fb_var_screeninfo rgbfb_var __initdata = {
	.xres = LCD_X_RES,
	.yres = LCD_Y_RES,
	.xres_virtual = LCD_X_RES,
	.yres_virtual = LCD_Y_RES,
	.bits_per_pixel = 32,
	.red.offset = 16,
	.red.length = 8,
	.green.offset = 8,
	.green.length = 8,
	.blue.offset = 0,
	.blue.length = 8,
	.left_margin = 0,
	.right_margin = 0,
	.upper_margin = 0,
	.lower_margin = 0,
	.vmode = FB_VMODE_NONINTERLACED,
};
static struct fb_fix_screeninfo rgbfb_fix __initdata = {
	.id = "RGBFB",
	.line_length = LCD_X_PAD * LCD_BBP,
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR,
	.xpanstep = 0,
	.ypanstep = 0,
	.ywrapstep = 0,
	.accel = FB_ACCEL_NONE,
};

static struct fb_ops rgbfb_ops = {
	.fb_open = rgbfb_open,
	.fb_release = rgbfb_release,
	.fb_mmap = rgbfb_mmap,
	.fb_setcolreg = rgbfb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_cursor = no_cursor,
};

static struct device_driver rgbfb_driver = {
	.name = "rgbfb",
	.bus = &platform_bus_type,
	.probe = rgbfb_probe,
	.remove = rgbfb_remove,
};

static int channel_owned;

static int no_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	return 0;
}

static int rgbfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			   u_int transp, struct fb_info *info)
{
	if (regno > 15)
		return 1;

	colreg[regno] = ((red & 0xff00) << 8) | (green & 0xff00) |
	    ((blue & 0xff00) >> 8);
	return 0;
}

static int rgbfb_open(struct fb_info *info, int user)
{
	return 0;
}

static int rgbfb_release(struct fb_info *info, int user)
{
	return 0;
}

static int rgbfb_mmap(struct fb_info *info, struct file *file,
		      struct vm_area_struct *vma)
{
	return pnx4008_sdum_mmap(info, file, vma, DEV_ID);
}

static int rgbfb_remove(struct device *device)
{
	struct fb_info *info = dev_get_drvdata(device);

	if (info) {
		unregister_framebuffer(info);
		fb_dealloc_cmap(&info->cmap);
		framebuffer_release(info);
	}

	pnx4008_free_dum_channel(channel_owned, DEV_ID);
	pnx4008_set_dum_exit_notification(DEV_ID);

	return 0;
}

int __init rgbfb_probe(struct device *device)
{
	struct platform_device *dev = to_platform_device(device);
	struct fb_info *info;
	struct dumchannel_uf chan_uf;
	int ret;

	info = framebuffer_alloc(sizeof(u32) * 256, &dev->dev);
	if (!info) {
		ret = -ENOMEM;
		goto err;
	}

	pnx4008_get_fb_addresses(FB_TYPE_RGB, (void **)&info->screen_base,
				 (dma_addr_t *) & rgbfb_fix.smem_start,
				 &rgbfb_fix.smem_len);

	if ((ret = pnx4008_alloc_dum_channel(DEV_ID)) < 0)
		goto err;
	else {
		channel_owned = ret;
		chan_uf.channelnr = channel_owned;
		chan_uf.dirty = (u32 *) NULL;	/* offset tov source */
		chan_uf.source = (u32 *) rgbfb_fix.smem_start;
		chan_uf.x_offset = 0;
		chan_uf.y_offset = 0;
		chan_uf.width = LCD_X_RES;
		chan_uf.height = LCD_Y_RES;

		if ((ret = pnx4008_put_dum_channel_uf(chan_uf, DEV_ID)) != 0)
			goto err;

		if ((ret =
		     pnx4008_set_dum_channel_sync(channel_owned, CONF_SYNC_ON,
						  DEV_ID)) != 0)
			goto err;

		if ((ret =
		     pnx4008_set_dum_chanel_dirty_detect(channel_owned,
							 CONF_DIRTYDETECTION_ON,
							 DEV_ID)) != 0)
			goto err;
	}

	info->node = -1;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->fbops = &rgbfb_ops;
	info->fix = rgbfb_fix;
	info->var = rgbfb_var;
	info->screen_size = rgbfb_fix.smem_len;
	info->pseudo_palette = info->par;
	info->par = NULL;

	ret = fb_alloc_cmap(&info->cmap, 256, 0);
	if (ret < 0)
		goto err1;

	ret = register_framebuffer(info);
	if (ret < 0)
		goto err2;
	dev_set_drvdata(&dev->dev, info);

	return 0;

      err2:
	fb_dealloc_cmap(&info->cmap);
      err1:
	framebuffer_release(info);
      err:
	pnx4008_free_dum_channel(channel_owned, DEV_ID);
	return ret;
}

int __init rgbfb_init(void)
{
	return driver_register(&rgbfb_driver);
}

void __exit rgbfb_exit(void)
{
	driver_unregister(&rgbfb_driver);
}

module_init(rgbfb_init);
module_exit(rgbfb_exit);
