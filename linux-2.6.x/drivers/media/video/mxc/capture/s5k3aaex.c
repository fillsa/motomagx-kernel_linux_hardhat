/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file s5k3aaex.c
 *
 * @brief s5k3aaex camera driver functions
 *
 * @ingroup Camera
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include "asm/arch/mxc_i2c.h"
#include "s5k3aaex.h"
#include "../drivers/media/video/mxc/capture/mxc_v4l2_capture.h"
//#define S5K3_DEBUG
#ifdef S5K3_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* S5K3_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* S5K3_DEBUG */

static s5k3aaex_conf s5k3aaex_device;
static sensor_interface *interface_param = NULL;

static int s5k3aaex_attach(struct i2c_adapter *adapter);
static int s5k3aaex_detach(struct i2c_client *client);

static struct i2c_driver s5k3aaex_i2c_driver = {
	.owner = THIS_MODULE,
	.name = "S5K3AAEX Client",
	.flags = I2C_DF_NOTIFY,
	.attach_adapter = s5k3aaex_attach,
	.detach_client = s5k3aaex_detach,
};

static struct i2c_client s5k3aaex_i2c_client = {
	.name = "s5k3aaex I2C dev",
	.id = 1,
	.addr = S5K3AAEX_I2C_ADDRESS,
	.driver = &s5k3aaex_i2c_driver,
};

static s5k3aaex_image_format format[2] = {
	{
	 .index = 0,
	 .imageFormat = S5K3AAEX_OutputResolution_SXGA,
	 .width = S5K3AAEX_WINWIDTH,
	 .height = S5K3AAEX_WINHEIGHT,
	 },
	{
	 .index = 1,
	 .imageFormat = S5K3AAEX_OutputResolution_VGA,
	 .width = 640,
	 .height = 512,
	 },
};

extern void gpio_sensor_setup(void);
extern void gpio_sensor_reset(bool flag);
extern void gpio_sensor_suspend(bool flag);

/*
 * Function definitions
 */
static int s5k3aaex_i2c_client_xfer(unsigned int addr, char *reg,
				    int reg_len, char *buf, int num,
				    int tran_flag)
{
	struct i2c_msg msg[2];
	int ret;

	msg[0].addr = addr;
	msg[0].len = reg_len;
	msg[0].buf = reg;
	msg[0].flags = tran_flag;
	msg[0].flags &= ~I2C_M_RD;

	msg[1].addr = addr;
	msg[1].len = num;
	msg[1].buf = buf;
	msg[1].flags = tran_flag;

	if (tran_flag & MXC_I2C_FLAG_READ) {
		msg[1].flags |= I2C_M_RD;
	} else {
		msg[1].flags &= ~I2C_M_RD;
	}

	ret = i2c_transfer(s5k3aaex_i2c_client.adapter, msg, 2);
	if (ret >= 0)
		return 0;
	return ret;
}

static int s5k3aaex_read_reg(u8 * reg, u8 * val)
{
	msleep(100);
	return s5k3aaex_i2c_client_xfer(S5K3AAEX_I2C_ADDRESS, reg, 1, val, 1,
					MXC_I2C_FLAG_READ);
}

static int s5k3aaex_write_reg(u8 reg, u8 val)
{
	u8 temp1, temp2;
	temp1 = reg;
	temp2 = val;
	msleep(100);
	return s5k3aaex_i2c_client_xfer(S5K3AAEX_I2C_ADDRESS, &temp1, 1, &temp2,
					1, 0);
}

static u8 s5k3aaex_sensor_downscale(bool downscale)
{
	u8 error = 0;
	u8 reg;
	u8 data;

	if (downscale == true) {
		printk("VGA\n");
		reg = 0xEC;
		data = 0;
		s5k3aaex_write_reg(reg, data);

		reg = 0x2;
		data = 0x30;
		s5k3aaex_write_reg(reg, data);
	} else {
		printk("SXGA\n");
		reg = 0xEC;
		data = 0;
		s5k3aaex_write_reg(reg, data);

		reg = 0x2;
		data = 0;
		s5k3aaex_write_reg(reg, data);
	}

	return error;
}

/*!
 * Initialize s5k3aaex_sensor_lib
 * Libarary for Sensor configuration through I2C
 *
 * @param       page0        page0 Registers
 * @param       page2        page2 Registers
 *
 * @return status
 */
static u8 s5k3aaex_sensor_lib(s5k3aaex_page0 * page0, s5k3aaex_page2 * page2)
{
	u8 error = 0;
	u8 reg;
	u8 data;

	FUNC_START;

	// changed to ARM command register map page 0
	reg = 0xEC;
	data = page0->addressSelect;
	s5k3aaex_write_reg(reg, data);
	// set the main clock
	reg = 0x72;
	data = page0->mainClock;
	s5k3aaex_write_reg(reg, data);

	// changed to CIS register map page 02
	reg = 0xEC;
	data = page2->addressSelect;
	s5k3aaex_write_reg(reg, data);

	// write the Hblank width
	reg = 0x1e;
	data = (u8) (page2->hblank & 0xFF);
	s5k3aaex_write_reg(reg, data);
	reg = 0x1d;
	data = (u8) ((page2->hblank >> 8) & 0xFF);
	s5k3aaex_write_reg(reg, data);

	// write the Vblank width
	reg = 0x18;
	data = (u8) (page2->vblank & 0xFF);
	s5k3aaex_write_reg(reg, data);
	reg = 0x17;
	data = (u8) ((page2->vblank >> 8) & 0xFF);
	s5k3aaex_write_reg(reg, data);

	// write the WRP
	reg = 0x5;
	data = (u8) (page2->wrp & 0xFF);
	s5k3aaex_write_reg(reg, data);
	reg = 0x4;
	data = (u8) ((page2->wrp >> 8) & 0xFF);
	s5k3aaex_write_reg(reg, data);

	// write the WCP
	reg = 0x7;
	data = (u8) (page2->wcp & 0xFF);
	s5k3aaex_write_reg(reg, data);
	reg = 0x6;
	data = (u8) ((page2->wcp >> 8) & 0xFF);
	s5k3aaex_write_reg(reg, data);

	// write DEFCOR_MOV_ADC 8 bit
	reg = 0x2;
	data = 0x8;
	s5k3aaex_write_reg(reg, data);

	// changed to ARM command register map page 1
	reg = 0xEC;
	data = 1;
	s5k3aaex_write_reg(reg, data);

	// write size to itu r 601
	reg = 0x6a;
	data = 0x5;
	s5k3aaex_write_reg(reg, data);

	FUNC_END;
	return error;
}

/*!
 * s5k3aaex sensor interface Initialization
 * @param param            sensor_interface *
 * @param width            u32
 * @param height           u32
 * @return  None
 */
static void s5k3aaex_interface(sensor_interface * param, u32 width, u32 height)
{
	param->clk_mode = 0x0;	//gated
	param->pixclk_pol = 0x0;
	param->data_width = 0x1;
	param->data_pol = 0x0;
	param->ext_vsync = 0x0;
	param->Vsync_pol = 0x0;
	param->Hsync_pol = 0x1;
	param->width = width - 1;
	param->height = ((height == 512) ? 480 : height) - 1;
	param->pixel_fmt = IPU_PIX_FMT_UYVY;
}

static int s5k3aaex_rate_cal(int *frame_rate, int mclk)
{
	int num_clock_per_row;
	int max_rate = 0;
	int index = 0;
	u16 width;
	u16 height;

	do {
		s5k3aaex_device.page0->imageFormat = format[index].imageFormat;
		height = format[index].height;
		width = format[index++].width;
		s5k3aaex_device.page2->hblank = S5K3AAEX_HORZBLANK_DEFAULT;
		s5k3aaex_device.page2->vblank = S5K3AAEX_VERTBLANK_DEFAULT;

		num_clock_per_row = (width + s5k3aaex_device.page2->hblank) * 2;
		max_rate = mclk / (num_clock_per_row *
				   (height + s5k3aaex_device.page2->vblank));
	} while ((index < 2) && (max_rate < *frame_rate));

	s5k3aaex_interface(interface_param, width, height);

	if (max_rate < *frame_rate)
		*frame_rate = max_rate;
	if (*frame_rate == 0)
		*frame_rate = max_rate;

	s5k3aaex_device.page2->vblank = mclk /
	    (*frame_rate * num_clock_per_row) - height;

	return index;
}

/*!
 * s5k3aaex sensor configuration
 *
 * @param frame_rate       int *
 * @param high_quality     int
 * @return  sensor_interface *
 */
static sensor_interface *s5k3aaex_config(int *frame_rate, int high_quality)
{
	int index;
	FUNC_START;

	index = s5k3aaex_rate_cal(frame_rate, interface_param->mclk);

	if (index == 1) {
		s5k3aaex_sensor_downscale(false);
	} else {
		s5k3aaex_sensor_downscale(true);
	}

	s5k3aaex_device.page0->mainClock = interface_param->mclk / 1048576 * 5;
	s5k3aaex_sensor_lib(s5k3aaex_device.page0, s5k3aaex_device.page2);

	FUNC_END;

	return interface_param;
}

/*!
 * s5k3aaex sensor set color configuration
 *
 * @param bright       int
 * @param saturation   int
 * @param red          int
 * @param green        int
 * @param blue         int
 * @return  None
 */
static void s5k3aaex_set_color(int bright, int saturation, int red, int green,
			       int blue)
{
	u8 reg;
	u8 data;

	FUNC_START;

	reg = 0xEC;
	data = 0;
	s5k3aaex_write_reg(reg, data);

	// set Brightness/Color Level balance
	reg = 0x76;
	data = (u8) bright;
	s5k3aaex_write_reg(reg, data);
	reg = 0x77;
	data = (u8) saturation;
	s5k3aaex_write_reg(reg, data);

	reg = 0xEC;
	data = 1;
	s5k3aaex_write_reg(reg, data);

	// set Red
	reg = 0x10;
	data = (u8) red;
	s5k3aaex_write_reg(reg, data);
	// set Blue
	reg = 0x18;
	data = (u8) blue;
	s5k3aaex_write_reg(reg, data);

	FUNC_END;
}

/*!
 * s5k3aaex sensor get color configuration
 *
 * @param bright       int *
 * @param saturation   int *
 * @param red          int *
 * @param green        int *
 * @param blue         int *
 * @return  None
 */
static void s5k3aaex_get_color(int *bright, int *saturation, int *red,
			       int *green, int *blue)
{
	u8 reg;
	u8 data;
	u8 *pdata;

	FUNC_START;

	reg = 0xEC;
	data = 0;
	s5k3aaex_write_reg(reg, data);

	// get Brightness/Color Level balance
	reg = 0x76;
	pdata = (u8 *) bright;
	s5k3aaex_read_reg(&reg, pdata);

	reg = 0x77;
	pdata = (u8 *) saturation;
	s5k3aaex_read_reg(&reg, pdata);

	reg = 0xEC;
	data = 1;
	s5k3aaex_write_reg(reg, data);

	// get Red
	reg = 0x10;
	pdata = (u8 *) red;
	s5k3aaex_read_reg(&reg, pdata);

	// get Blue
	reg = 0x18;
	pdata = (u8 *) blue;
	s5k3aaex_read_reg(&reg, pdata);

	FUNC_END;
}

/*!
 * s5k3aaex Reset function
 *
 * @return  None
 */
static sensor_interface *s5k3aaex_reset(void)
{
	set_mclk_rate(&interface_param->mclk);

	/* Reset for 10 cycle */
	gpio_sensor_reset(true);
	msleep(10);
	gpio_sensor_reset(false);
	msleep(30);

	s5k3aaex_interface(interface_param, format[0].width, format[0].height);
	return interface_param;
}

struct camera_sensor camera_sensor_if = {
      set_color:s5k3aaex_set_color,
      get_color:s5k3aaex_get_color,
      config:s5k3aaex_config,
      reset:s5k3aaex_reset,
};

#if 0
static void s5k3aaex_test_pattern(bool flag)
{
	u8 reg;
	u8 data;

	FUNC_START;

	// changed to ARM command register map page 0
	reg = 0xEC;
	data = 0;
	s5k3aaex_write_reg(reg, data);

	if (flag == true) {
		reg = 0xb;
		data = 0x1;
		s5k3aaex_write_reg(reg, data);
	} else {
		reg = 0xb;
		data = 0x0;
		s5k3aaex_write_reg(reg, data);
	}

	FUNC_END;
}
#endif

/*!
 * s5k3aaex I2C attach function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int s5k3aaex_attach(struct i2c_adapter *adapter)
{
	if (strcmp(adapter->name, MXC_ADAPTER_NAME) != 0) {
		printk("s5k3aaex_attach: %s\n", adapter->name);
		return -1;
	}

	s5k3aaex_i2c_client.adapter = adapter;
	if (i2c_attach_client(&s5k3aaex_i2c_client)) {
		s5k3aaex_i2c_client.adapter = NULL;
		printk("s5k3aaex_attach: i2c_attach_client failed\n");
		return -1;
	}

	interface_param = (sensor_interface *)
	    kmalloc(sizeof(sensor_interface), GFP_KERNEL);
	if (!interface_param) {
		printk("s5k3aaex_attach: kmalloc failed \n");
		return -1;
	}

	gpio_sensor_setup();

	gpio_sensor_suspend(false);

	interface_param->mclk = 0x2000000;

	return 0;
}

/*!
 * s5k3aaex I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int s5k3aaex_detach(struct i2c_client *client)
{
	int err;

	if (!s5k3aaex_i2c_client.adapter)
		return -1;

	err = i2c_detach_client(&s5k3aaex_i2c_client);
	s5k3aaex_i2c_client.adapter = NULL;

	if (interface_param)
		kfree(interface_param);
	interface_param = NULL;

	return err;
}

/*!
 * s5k3aaex init function
 *
 * @return  Error code indicating success or failure
 */
static __init int s5k3aaex_init(void)
{
	u8 err = 0;
	FUNC_START;

	s5k3aaex_device.page0 = (s5k3aaex_page0 *)
	    kmalloc(sizeof(s5k3aaex_page0), GFP_KERNEL);

	if (!s5k3aaex_device.page0)
		return -1;
	memset(s5k3aaex_device.page0, 0, sizeof(s5k3aaex_page0));
	s5k3aaex_device.page0->addressSelect = 0;
	s5k3aaex_device.page0->functionOnOff = 0x48;

	s5k3aaex_device.page2 = (s5k3aaex_page2 *)
	    kmalloc(sizeof(s5k3aaex_page2), GFP_KERNEL);
	if (!s5k3aaex_device.page2) {
		kfree(s5k3aaex_device.page0);
		s5k3aaex_device.page0 = NULL;
		return -1;
	}
	memset(s5k3aaex_device.page2, 0, sizeof(s5k3aaex_page2));
	s5k3aaex_device.page2->addressSelect = 2;
	s5k3aaex_device.page2->wrp = 14;
	s5k3aaex_device.page2->wcp = 14;

	s5k3aaex_device.page2->hblank = S5K3AAEX_HORZBLANK_DEFAULT;
	s5k3aaex_device.page2->vblank = S5K3AAEX_VERTBLANK_DEFAULT;

	err = i2c_add_driver(&s5k3aaex_i2c_driver);

	FUNC_END;
	return err;
}

/*!
 * s5k3aaex cleanup function
 *
 * @return  Error code indicating success or failure
 */
static void __exit s5k3aaex_clean(void)
{
	FUNC_START;

	i2c_del_driver(&s5k3aaex_i2c_driver);

	if (s5k3aaex_device.page0) {
		kfree(s5k3aaex_device.page0);
		s5k3aaex_device.page0 = NULL;
	}

	if (s5k3aaex_device.page2) {
		kfree(s5k3aaex_device.page2);
		s5k3aaex_device.page2 = NULL;
	}

	FUNC_END;
}

module_init(s5k3aaex_init);
module_exit(s5k3aaex_clean);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("s5k3aaex Driver");
MODULE_LICENSE("GPL");
