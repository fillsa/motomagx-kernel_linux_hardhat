/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file mt9v111.c
 *
 * @brief mt9v111 camera driver functions
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
#include <asm/arch/mxc_i2c.h>
#include "mxc_v4l2_capture.h"
#include "mt9v111.h"

/*#define MT9V111_DEBUG	*/
#ifdef MT9V111_DEBUG

#  define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " \
          fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#  define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#  define FUNC_START DPRINTK(" func start\n")
#  define FUNC_END DPRINTK(" func end\n")

#  define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", \
          __FILE__,__LINE__,__FUNCTION__ ,err)

#else				/* MT9V111_DEBUG */

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				/* MT9V111_DEBUG */

#ifdef MT9V111_DEBUG
static u16 testpattern = 0;
#endif

static sensor_interface *interface_param = NULL;
static mt9v111_conf mt9v111_device;
static int reset_frame_rate = 30;

#define MT9V111_FRAME_RATE_NUM    20

static mt9v111_image_format format[2] = {
	{
	 .index = 0,
	 .width = 640,
	 .height = 480,
	 },
	{
	 .index = 1,
	 .width = 352,
	 .height = 288,
	 },
};

static int mt9v111_attach(struct i2c_adapter *adapter);
static int mt9v111_detach(struct i2c_client *client);

static struct i2c_driver mt9v111_i2c_driver = {
	.owner = THIS_MODULE,
	.name = "MT9V111 Client",
	.flags = I2C_DF_NOTIFY,
	.attach_adapter = mt9v111_attach,
	.detach_client = mt9v111_detach,
};

static struct i2c_client mt9v111_i2c_client = {
	.name = "mt9v111 I2C dev",
	.id = 1,
	.addr = MT9V111_I2C_ADDRESS,
	.driver = &mt9v111_i2c_driver,
};

/*
 * Function definitions
 */

static u16 mt9v111_endian_swap16(u16 data)
{
	u16 temp;

	temp = data;
	temp = ((data >> 8) & 0xff) | ((data << 8) & 0xff00);

	return temp;
}

static int mt9v111_i2c_client_xfer(unsigned int addr, char *reg, int reg_len,
				   char *buf, int num, int tran_flag)
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

	ret = i2c_transfer(mt9v111_i2c_client.adapter, msg, 2);
	if (ret >= 0)
		return 0;

	return ret;
}

#ifdef MT9V111_DEBUG
static int mt9v111_read_reg(u8 * reg, u16 * val)
{
	return mt9v111_i2c_client_xfer(MT9V111_I2C_ADDRESS, reg, 1,
				       (u8 *) val, 2, MXC_I2C_FLAG_READ);
}
#endif

static int mt9v111_write_reg(u8 reg, u16 val)
{
	u8 temp1;
	u16 temp2;
	temp1 = reg;
	temp2 = mt9v111_endian_swap16(val);
	DPRINTK("write reg %x val %x.\n", reg, val);
	return mt9v111_i2c_client_xfer(MT9V111_I2C_ADDRESS, &temp1, 1,
				       (u8 *) & temp2, 2, 0);
}

/*!
 * Initialize mt9v111_sensor_lib
 * Libarary for Sensor configuration through I2C
 *
 * @param       coreReg       Core Registers
 * @param       ifpReg        IFP Register
 *
 * @return status
 */
static u8 mt9v111_sensor_lib(mt9v111_coreReg * coreReg, mt9v111_IFPReg * ifpReg)
{
	u8 reg;
	u16 data;
	u8 error = 0;

	FUNC_START;
	/*
	 * setup to IFP registers
	 */
	reg = MT9V111I_ADDR_SPACE_SEL;
	data = ifpReg->addrSpaceSel;
	mt9v111_write_reg(reg, data);

	// Operation Mode Control
	reg = MT9V111I_MODE_CONTROL;
	data = ifpReg->modeControl;
	mt9v111_write_reg(reg, data);

	// Output format
	reg = MT9V111I_FORMAT_CONTROL;
	data = ifpReg->formatControl;	// Set bit 12
	mt9v111_write_reg(reg, data);

	// AE limit 4
	reg = MT9V111I_SHUTTER_WIDTH_LIMIT_AE;
	data = ifpReg->gainLimitAE;
	mt9v111_write_reg(reg, data);

	reg = MT9V111I_OUTPUT_FORMAT_CTRL2;
	data = ifpReg->outputFormatCtrl2;
	mt9v111_write_reg(reg, data);

	reg = MT9V111I_AE_SPEED;
	data = ifpReg->AESpeed;
	mt9v111_write_reg(reg, data);

#if 0
	reg = MT9V111I_UPPER_SHUTTER_DELAY_LIM;
	data = ifpReg->upperShutterDelayLi;
	mt9v111_write_reg(reg, data);

	reg = MT9V111I_SHUTTER_60;
	data = ifpReg->shutter_width_60;
	mt9v111_write_reg(reg, data);

	reg = MT9V111I_SEARCH_FLICK_60;
	data = ifpReg->search_flicker_60;
	mt9v111_write_reg(reg, data);
#endif

	/*
	 * setup to sensor core registers
	 */
	reg = MT9V111I_ADDR_SPACE_SEL;
	data = coreReg->addressSelect;
	mt9v111_write_reg(reg, data);

	// enable changes and put the Sync bit on
	reg = MT9V111S_OUTPUT_CTRL;
	data = MT9V111S_OUTCTRL_SYNC | MT9V111S_OUTCTRL_CHIP_ENABLE | 0x3000;
	mt9v111_write_reg(reg, data);

	// min PIXCLK - Default
	reg = MT9V111S_PIXEL_CLOCK_SPEED;
	data = coreReg->pixelClockSpeed;
	mt9v111_write_reg(reg, data);

	//Setup image flipping / Dark rows / row/column skip
	reg = MT9V111S_READ_MODE;
	data = coreReg->readMode;
	mt9v111_write_reg(reg, data);

	//zoom 0
	reg = MT9V111S_DIGITAL_ZOOM;
	data = coreReg->digitalZoom;
	mt9v111_write_reg(reg, data);

	// min H-blank
	reg = MT9V111S_HOR_BLANKING;
	data = coreReg->horizontalBlanking;
	mt9v111_write_reg(reg, data);

	// min V-blank
	reg = MT9V111S_VER_BLANKING;
	data = coreReg->verticalBlanking;
	mt9v111_write_reg(reg, data);

	reg = MT9V111S_SHUTTER_WIDTH;
	data = coreReg->shutterWidth;
	mt9v111_write_reg(reg, data);

	reg = MT9V111S_SHUTTER_DELAY;
	data = ifpReg->upperShutterDelayLi;
	mt9v111_write_reg(reg, data);

	// changes become effective
	reg = MT9V111S_OUTPUT_CTRL;
	data = MT9V111S_OUTCTRL_CHIP_ENABLE | 0x3000;
	mt9v111_write_reg(reg, data);

	FUNC_END;
	return error;
}

/*!
 * mt9v111 sensor interface Initialization
 * @param param            sensor_interface *
 * @param width            u32
 * @param height           u32
 * @return  None
 */
static void mt9v111_interface(sensor_interface * param, u32 width, u32 height)
{
	param->Vsync_pol = 0x0;
	param->clk_mode = 0x0;	//gated
	param->pixclk_pol = 0x0;
	param->data_width = 0x1;
	param->data_pol = 0x0;
	param->ext_vsync = 0x0;
	param->Vsync_pol = 0x0;
	param->Hsync_pol = 0x0;
	param->width = width - 1;
	param->height = height - 1;
	param->pixel_fmt = IPU_PIX_FMT_UYVY;
	param->mclk = 27000000;
}

/*!
 * MT9V111 frame rate calculate
 *
 * @param frame_rate       int *
 * @param mclk             int
 * @return  None
 */
static void mt9v111_rate_cal(int *frame_rate, int mclk)
{
	int num_clock_per_row;
	int max_rate = 0;
 
	mt9v111_device.coreReg->horizontalBlanking = MT9V111_HORZBLANK_MIN;

	num_clock_per_row = (format[0].width + 114 + MT9V111_HORZBLANK_MIN) * 2;
	max_rate = mclk / (num_clock_per_row * 
	    (format[0].height + MT9V111_VERTBLANK_DEFAULT));

	if ((*frame_rate > max_rate) || (*frame_rate == 0)) {
		*frame_rate = max_rate;
	}

    mt9v111_device.coreReg->verticalBlanking 
        = mclk / (*frame_rate * num_clock_per_row) - format[0].height;

	reset_frame_rate = *frame_rate;
}

/*!
 * MT9V111 sensor configuration
 *
 * @param frame_rate       int *
 * @param high_quality     int
 * @return  sensor_interface *
 */
sensor_interface *mt9v111_config(int *frame_rate, int high_quality)
{
	FUNC_START;

	mt9v111_device.coreReg->addressSelect = MT9V111I_SEL_SCA;
	mt9v111_device.ifpReg->addrSpaceSel = MT9V111I_SEL_IFP;

	mt9v111_device.coreReg->windowHeight = MT9V111_WINHEIGHT;
	mt9v111_device.coreReg->windowWidth = MT9V111_WINWIDTH;
	mt9v111_device.coreReg->zoomColStart = 0;
	mt9v111_device.coreReg->zomRowStart = 0;
	mt9v111_device.coreReg->digitalZoom = 0x0;

	mt9v111_device.coreReg->verticalBlanking = MT9V111_VERTBLANK_DEFAULT;
	mt9v111_device.coreReg->horizontalBlanking = MT9V111_HORZBLANK_MIN;
	mt9v111_device.coreReg->pixelClockSpeed = 0;
	mt9v111_device.coreReg->readMode = 0xd0a1;

	mt9v111_device.ifpReg->outputFormatCtrl2 = 0;
	mt9v111_device.ifpReg->gainLimitAE = 0x300;
	mt9v111_device.ifpReg->AESpeed = 0x80;

	// here is the default value
	mt9v111_device.ifpReg->formatControl = 0xc800;
	mt9v111_device.ifpReg->modeControl = 0x708e;
	mt9v111_device.ifpReg->awbSpeed = 0x4514;	
	mt9v111_device.coreReg->shutterWidth = 0xf8;

	mt9v111_interface(interface_param, 640, 480);
	set_mclk_rate(&interface_param->mclk);
	mt9v111_rate_cal(frame_rate, interface_param->mclk);
	mt9v111_sensor_lib(mt9v111_device.coreReg, mt9v111_device.ifpReg);

	FUNC_END;

	return interface_param;
}

/*!
 * mt9v111 sensor set color configuration
 *
 * @param bright       int
 * @param saturation   int
 * @param red          int
 * @param green        int
 * @param blue         int
 * @return  None
 */
static void
mt9v111_set_color(int bright, int saturation, int red, int green, int blue)
{
	u8 reg;
	u16 data;

	switch (saturation)
	{
		case 100:
			mt9v111_device.ifpReg->awbSpeed = 0x4514;	
			break;
		case 150:
			mt9v111_device.ifpReg->awbSpeed = 0x6D14;	
			break;
		case 75:
			mt9v111_device.ifpReg->awbSpeed = 0x4D14;	
			break;
		case 50:
			mt9v111_device.ifpReg->awbSpeed = 0x5514;	
			break;
		case 37:
			mt9v111_device.ifpReg->awbSpeed = 0x5D14;	
			break;
		case 25:
			mt9v111_device.ifpReg->awbSpeed = 0x6514;	
			break;
		default:
			mt9v111_device.ifpReg->awbSpeed = 0x4514;	
			break;
	}

	reg = MT9V111I_ADDR_SPACE_SEL;
	data = mt9v111_device.ifpReg->addrSpaceSel;
	mt9v111_write_reg(reg, data);

	// Operation Mode Control
	reg = MT9V111I_AWB_SPEED;
	data = mt9v111_device.ifpReg->awbSpeed;
	mt9v111_write_reg(reg, data);
}

/*!
 * mt9v111 sensor get color configuration
 *
 * @param bright       int *
 * @param saturation   int *
 * @param red          int *
 * @param green        int *
 * @param blue         int *
 * @return  None
 */
static void
mt9v111_get_color(int *bright, int *saturation, int *red, int *green, int *blue)
{
	*saturation = (mt9v111_device.ifpReg->awbSpeed & 0x3800) >> 11;
	switch (*saturation)
	{
		case 0:
			*saturation = 100;	
			break;
		case 1:
			*saturation = 75;	
			break;
		case 2:
			*saturation = 50;	
			break;
		case 3:
			*saturation = 37;	
			break;
		case 4:
			*saturation = 25;	
			break;
		case 5:
			*saturation = 150;	
			break;
		case 6:
			*saturation = 0;	
			break;
        default:
			*saturation = 0;	
			break;
	}
}

/*!
 * mt9v111 Reset function
 *
 * @return  None
 */
static sensor_interface *mt9v111_reset(void)
{
	return mt9v111_config(&reset_frame_rate, 0);
}

struct camera_sensor camera_sensor_if = {
      set_color:mt9v111_set_color,
      get_color:mt9v111_get_color,
      config:mt9v111_config,
      reset:mt9v111_reset,
};

#ifdef MT9V111_DEBUG
/*!
 * Set sensor to test mode, which will generate test pattern.
 *
 * @return none
 */
static void mt9v111_test_pattern(bool flag)
{
	u8 reg;
	u16 data;

	FUNC_START;

	// switch to sensor registers
	reg = MT9V111I_ADDR_SPACE_SEL;
	data = MT9V111I_SEL_SCA;
	mt9v111_write_reg(reg, data);

	if (flag == true) {
		testpattern = MT9V111S_OUTCTRL_TEST_MODE;

		reg = MT9V111S_ROW_NOISE_CTRL;
		data = mt9v111_read_reg(&reg, &data) & 0xBF;
		data = mt9v111_endian_swap16(data);
		mt9v111_write_reg(reg, data);

		reg = MT9V111S_TEST_DATA;
		data = 0;
		mt9v111_write_reg(reg, data);

		reg = MT9V111S_OUTPUT_CTRL;
		// changes take effect
		data = MT9V111S_OUTCTRL_CHIP_ENABLE | testpattern | 0x3000;
		mt9v111_write_reg(reg, data);
	} else {
		testpattern = 0;

		reg = MT9V111S_ROW_NOISE_CTRL;
		data = mt9v111_read_reg(&reg, &data) | 0x40;
		data = mt9v111_endian_swap16(data);
		mt9v111_write_reg(reg, data);

		reg = MT9V111S_OUTPUT_CTRL;
		// changes take effect
		data = MT9V111S_OUTCTRL_CHIP_ENABLE | testpattern | 0x3000;
		mt9v111_write_reg(reg, data);
	}

	FUNC_END;
}
#endif

/*!
 * mt9v111 I2C attach function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int mt9v111_attach(struct i2c_adapter *adapter)
{
	if (strcmp(adapter->name, MXC_ADAPTER_NAME) != 0) {
		printk("mt9v111_attach: %s\n", adapter->name);
		return -1;
	}

	mt9v111_i2c_client.adapter = adapter;
	if (i2c_attach_client(&mt9v111_i2c_client)) {
		mt9v111_i2c_client.adapter = NULL;
		printk("mt9v111_attach: i2c_attach_client failed\n");
		return -1;
	}

	interface_param = (sensor_interface *)
	    kmalloc(sizeof(sensor_interface), GFP_KERNEL);
	if (!interface_param) {
		printk("mt9v111_attach: kmalloc failed \n");
		return -1;
	}

	return 0;
}

/*!
 * mt9v111 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int mt9v111_detach(struct i2c_client *client)
{
	int err;

	if (!mt9v111_i2c_client.adapter)
		return -1;

	err = i2c_detach_client(&mt9v111_i2c_client);
	mt9v111_i2c_client.adapter = NULL;

	if (interface_param)
		kfree(interface_param);
	interface_param = NULL;

	return err;
}

extern void gpio_sensor_active(void);

/*!
 * MT9V111 init function
 *
 * @return  Error code indicating success or failure
 */
static __init int mt9v111_init(void)
{
	u8 err;

	gpio_sensor_active();

	mt9v111_device.coreReg = (mt9v111_coreReg *)
	    kmalloc(sizeof(mt9v111_coreReg), GFP_KERNEL);
	if (!mt9v111_device.coreReg)
		return -1;

	memset(mt9v111_device.coreReg, 0, sizeof(mt9v111_coreReg));

	mt9v111_device.ifpReg = (mt9v111_IFPReg *)
	    kmalloc(sizeof(mt9v111_IFPReg), GFP_KERNEL);
	if (!mt9v111_device.ifpReg) {
		kfree(mt9v111_device.coreReg);
		mt9v111_device.coreReg = NULL;
		return -1;
	}

	memset(mt9v111_device.ifpReg, 0, sizeof(mt9v111_IFPReg));

	err = i2c_add_driver(&mt9v111_i2c_driver);

	return err;
}

extern void gpio_sensor_inactive(void);
/*!
 * MT9V111 cleanup function
 *
 * @return  Error code indicating success or failure
 */
static void __exit mt9v111_clean(void)
{
	FUNC_START;

	if (mt9v111_device.coreReg) {
		kfree(mt9v111_device.coreReg);
		mt9v111_device.coreReg = NULL;
	}

	if (mt9v111_device.ifpReg) {
		kfree(mt9v111_device.ifpReg);
		mt9v111_device.ifpReg = NULL;
	}

	i2c_del_driver(&mt9v111_i2c_driver);

	gpio_sensor_inactive();
	FUNC_END;
}

module_init(mt9v111_init);
module_exit(mt9v111_clean);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("Mt9v111 Camera Driver");
MODULE_LICENSE("GPL");
