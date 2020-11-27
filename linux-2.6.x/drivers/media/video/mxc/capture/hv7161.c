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
 * @file hv7161.c
 *
 * @brief hv7161 camera driver functions
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
#include "hv7161.h"
#include "../drivers/media/video/mxc/capture/mxc_v4l2_capture.h"
#include "asm/arch/mxc_i2c.h"

//#define HV7161_DEBUG
#ifdef HV7161_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* HV7161_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* HV7161_DEBUG */

#define HV7161_TERM 0xFF

static sensor_interface *interface_param = NULL;
static int reset_frame_rate = 30;

static hv7161_image_format format[2] = {
	{
	 .index = 0,
	 .width = 1280,
	 .height = 960,
	 },
	{
	 .index = 1,
	 .width = 640,
	 .height = 480,
	 },
};

const static struct hv7161_reg hv7161_common[] = {
	{0x31, 0x20}, {0x32, 0x3}, {0xee, 0x3},
	{HV7161_TERM, HV7161_TERM}
};

static int hv7161_attach(struct i2c_adapter *adapter);
static int hv7161_detach(struct i2c_client *client);

static struct i2c_driver hv7161_i2c_driver = {
	.owner = THIS_MODULE,
	.name = "HV7161 Client",
	.flags = I2C_DF_NOTIFY,
	.attach_adapter = hv7161_attach,
	.detach_client = hv7161_detach,
};

static struct i2c_client hv7161_i2c_client = {
	.name = "hv7161 I2C dev",
	.id = 1,
	.addr = HV7161_I2C_ADDRESS,
	.driver = &hv7161_i2c_driver,
};

extern void gpio_sensor_setup(void);
extern void gpio_sensor_reset(bool flag);
extern void gpio_sensor_suspend(bool flag);

/*
 * Function definitions
 */
static int hv7161_i2c_client_xfer(unsigned int addr, char *reg,
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

	ret = i2c_transfer(hv7161_i2c_client.adapter, msg, 2);
	if (ret >= 0)
		return 0;

	return ret;
}

static int hv7161_read_reg(u8 * reg, u8 * val)
{
	return hv7161_i2c_client_xfer(HV7161_I2C_ADDRESS, reg, 1, val, 1,
				      MXC_I2C_FLAG_READ);
}

static int hv7161_write_reg(u8 reg, u8 val)
{
	u8 temp1, temp2;
	temp1 = reg;
	temp2 = val;
	return hv7161_i2c_client_xfer(HV7161_I2C_ADDRESS, &temp1, 1, &temp2,
				      1, 0);
}

static int hv7161_write_regs(const struct hv7161_reg reglist[])
{
	int err;
	const struct hv7161_reg *next = reglist;

	while (!((next->reg == HV7161_TERM) && (next->val == HV7161_TERM))) {
		err = hv7161_write_reg(next->reg, next->val);
		if (err) {
			return err;
		}
		next++;
	}
	return 0;
}

/*!
 * hv7161 sensor downscale function
 * @param downscale            bool
 * @return  Error code indicating success or failure
 */
static u8 hv7161_sensor_downscale(bool downscale)
{
	u8 error = 0;
	u8 reg, data;

	if (downscale == true) {
		reg = 0x1;
		data = 0x16;
		hv7161_write_reg(reg, data);
	} else {
		reg = 0x1;
		data = 0x13;
		hv7161_write_reg(reg, data);
	}

	hv7161_write_regs(hv7161_common);

	return error;
}

/*!
 * hv7161 sensor interface Initialization
 * @param param            sensor_interface *
 * @param width            u32
 * @param height           u32
 * @return  None
 */
static void hv7161_interface(sensor_interface * param, u32 width, u32 height)
{
	param->clk_mode = 0x0;	//gated
	param->pixclk_pol = 0x0;
	param->data_width = 0x1;
	param->data_pol = 0x0;
	param->ext_vsync = 0x1;
	param->Vsync_pol = 0x1;
	param->Hsync_pol = 0x1;
	param->width = width - 1;
	param->height = height - 1;
	param->pixel_fmt = IPU_PIX_FMT_UYVY;
}

/*!
 * hv7161 sensor configuration
 *
 * @param frame_rate       int 	*
 * @param high_quality     int
 * @return  sensor_interface *
 */
static sensor_interface *hv7161_config(int *frame_rate, int high_quality)
{
	u8 reg, data;
	int num_clock_per_row;
	u16 h_blank;
	int max_rate = 0;
	int index = 1;

	FUNC_START;

	if (high_quality == 1)
		index = 0;

	hv7161_interface(interface_param, format[index].width,
			 format[index].height);

	if (index == 0) {
		printk("SXGA\n");
		hv7161_sensor_downscale(false);
	} else {
		printk("VGA\n");
		hv7161_sensor_downscale(true);
	}

	num_clock_per_row = format[0].width + 208;
	max_rate = interface_param->mclk / (num_clock_per_row *
					    (format[0].height + 8));

	if ((*frame_rate > max_rate) || (*frame_rate == 0)) {
		*frame_rate = max_rate;
	}

	num_clock_per_row = interface_param->mclk / *frame_rate;
	num_clock_per_row /= format[0].height + 8;
	h_blank = num_clock_per_row - format[0].width;
	reg = 0x11;
	data = (u8) (h_blank & 0xff);
	hv7161_write_reg(reg, data);
	reg = 0x10;
	data = (u8) ((h_blank >> 8) & 0xff);
	hv7161_write_reg(reg, data);

	reset_frame_rate = *frame_rate;

	FUNC_END;

	return interface_param;
}

/*!
 * hv7161 sensor set color configuration
 *
 * @param bright       int
 * @param saturation   int
 * @param red          int
 * @param green        int
 * @param blue         int
 * @return  None
 */
static void
hv7161_set_color(int bright, int saturation, int red, int green, int blue)
{
	u8 reg;
	u8 data;
	FUNC_START;

	// set Brightness
	reg = 0x5b;
	data = (u8) bright;
	hv7161_write_reg(reg, data);
	// set Saturation
	reg = 0x5c;
	data = (u8) saturation;
	hv7161_write_reg(reg, data);
	// set Red
	reg = 0x14;
	data = (u8) red;
	hv7161_write_reg(reg, data);
	// set Green
	reg = 0x15;
	data = (u8) green;
	hv7161_write_reg(reg, data);
	// set Blue
	reg = 0x16;
	data = (u8) blue;
	hv7161_write_reg(reg, data);

	FUNC_END;
}

/*!
 * hv7161 sensor get color configuration
 *
 * @param bright       int *
 * @param saturation   int *
 * @param red          int *
 * @param green        int *
 * @param blue         int *
 * @return  None
 */
static void
hv7161_get_color(int *bright, int *saturation, int *red, int *green, int *blue)
{
	u8 reg[1];
	u8 *pdata;

	FUNC_START;

	// get Brightness
	reg[0] = 0x5b;
	pdata = (u8 *) bright;
	hv7161_read_reg(reg, pdata);
	// get saturation
	reg[0] = 0x5c;
	pdata = (u8 *) saturation;
	hv7161_read_reg(reg, pdata);
	// get Red
	reg[0] = 0x14;
	pdata = (u8 *) red;
	hv7161_read_reg(reg, pdata);
	// get Green
	reg[0] = 0x15;
	pdata = (u8 *) red;
	hv7161_read_reg(reg, pdata);
	// get Blue
	reg[0] = 0x16;
	pdata = (u8 *) blue;
	hv7161_read_reg(reg, pdata);

	FUNC_END;
}

/*!
 * hv7161 Reset function
 *
 * @return  None
 */
static sensor_interface *hv7161_reset(void)
{
	set_mclk_rate(&interface_param->mclk);

	/* Reset for at least 4 cycles */
	gpio_sensor_reset(true);
	msleep(10);
	gpio_sensor_reset(false);
	msleep(10);

	hv7161_config(&reset_frame_rate, 0);
	return interface_param;
}

struct camera_sensor camera_sensor_if = {
      set_color:hv7161_set_color,
      get_color:hv7161_get_color,
      config:hv7161_config,
      reset:hv7161_reset,
};

/*!
 * hv7161 I2C attach function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int hv7161_attach(struct i2c_adapter *adapter)
{
	if (strcmp(adapter->name, MXC_ADAPTER_NAME) != 0) {
		printk("hv7161_attach: %s\n", adapter->name);
		return -1;
	}

	hv7161_i2c_client.adapter = adapter;
	if (i2c_attach_client(&hv7161_i2c_client)) {
		hv7161_i2c_client.adapter = NULL;
		printk("hv7161_attach: i2c_attach_client failed\n");
		return -1;
	}

	interface_param = (sensor_interface *)
	    kmalloc(sizeof(sensor_interface), GFP_KERNEL);
	if (!interface_param) {
		printk("hv7161_attach: kmalloc failed \n");
		return -1;
	}

	gpio_sensor_setup();

	gpio_sensor_suspend(false);

	interface_param->mclk = 0x2a00000;

	return 0;
}

/*!
 * hv7161 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int hv7161_detach(struct i2c_client *client)
{
	int err;

	if (!hv7161_i2c_client.adapter)
		return -1;

	err = i2c_detach_client(&hv7161_i2c_client);
	hv7161_i2c_client.adapter = NULL;

	if (interface_param)
		kfree(interface_param);
	interface_param = NULL;

	return err;
}

/*!
 * hv7161 init function
 *
 * @return  Error code indicating success or failure
 */
static __init int hv7161_init(void)
{
	u8 err;
	FUNC_START;

	err = i2c_add_driver(&hv7161_i2c_driver);

	FUNC_END;
	return err;
}

/*!
 * hv7161 cleanup function
 *
 * @return  Error code indicating success or failure
 */
static void __exit hv7161_clean(void)
{
	FUNC_START;

	i2c_del_driver(&hv7161_i2c_driver);

	FUNC_END;
}

module_init(hv7161_init);
module_exit(hv7161_clean);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("hv7161 Driver");
MODULE_LICENSE("GPL");
