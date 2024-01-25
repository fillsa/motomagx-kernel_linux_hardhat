/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2005-2006, 2008 Motorola Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
Revision History:
                   Modification    
Author                 Date        Description of Changes
----------------   ------------    -------------------------
Motorola            10/18/2005     Disable the IC when disabling
                                   the CSI to return IPU to
                                   initial state.  Otherwise,
                                   the CSI may hang the next time
                                   it is enabled.
Motorola            11/16/2005     Momentarily disable both CSI
                                   and IC when reconfiguring CSI
                                   parameters.
Motorola            06/09/2006     Upmerge to MV 05/12/2006
Motorola            05/16/2008     delete  printk for CSI Reg  
Motorola            09/04/2008     Export CSI position and size functions
*/

/*!
 * @file ipu_csi.c
 *
 * @brief IPU CMOS Sensor interface functions
 *
 * @ingroup IPU
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include "ipu.h"
#include "ipu_prv.h"
#include "ipu_regs.h"
#include "asm/arch/clock.h"

static bool gipu_csi_get_mclk_flag = false;
static int csi_mclk_flag = 0;

extern void gpio_sensor_suspend(bool flag);

/*!
 * ipu_csi_init_interface
 *    Sets initial values for the CSI registers.
 *    The width and height of the sensor and the actual frame size will be
 *    set to the same values.
 * @param       width        Sensor width
 * @param       height       Sensor height
 * @param       pixel_fmt    pixel format
 * @param       sig          ipu_csi_signal_cfg_t structure
 *
 * @return      0 for success, -EINVAL for error
 */
int32_t
ipu_csi_init_interface(uint16_t width, uint16_t height, uint32_t pixel_fmt,
		       ipu_csi_signal_cfg_t sig)
{
	uint32_t data = 0;

	FUNC_START;

	/* Set SENS_DATA_FORMAT bits (8 and 9)
	   RGB or YUV444 is 0 which is current value in data so not set explicitly
	   This is also the default value if attempts are made to set it to
	   something invalid. */
	switch (pixel_fmt) {
	case IPU_PIX_FMT_UYVY:
		data = CSI_SENS_CONF_DATA_FMT_YUV422;
		break;
	case IPU_PIX_FMT_RGB24:
	case IPU_PIX_FMT_BGR24:
		data = CSI_SENS_CONF_DATA_FMT_RGB_YUV444;
		break;
	case IPU_PIX_FMT_GENERIC:
		data = CSI_SENS_CONF_DATA_FMT_BAYER;
		break;
	default:
		return -EINVAL;
	}

	/* Set the CSI_SENS_CONF register remaining fields */
	data |= sig.data_width << CSI_SENS_CONF_DATA_WIDTH_SHIFT |
	    sig.data_pol << CSI_SENS_CONF_DATA_POL_SHIFT |
	    sig.Vsync_pol << CSI_SENS_CONF_VSYNC_POL_SHIFT |
	    sig.Hsync_pol << CSI_SENS_CONF_HSYNC_POL_SHIFT |
	    sig.pixclk_pol << CSI_SENS_CONF_PIX_CLK_POL_SHIFT |
	    sig.ext_vsync << CSI_SENS_CONF_EXT_VSYNC_SHIFT |
	    sig.clk_mode << CSI_SENS_CONF_SENS_PRTCL_SHIFT |
	    sig.sens_clksrc << CSI_SENS_CONF_SENS_CLKSRC_SHIFT;

	__raw_writel(data, CSI_SENS_CONF);

	/* Setup frame size	*/
	__raw_writel(width | height << 16, CSI_SENS_FRM_SIZE);

	__raw_writel(width << 16, CSI_FLASH_STROBE_1);
	__raw_writel(height << 16 | 0x22, CSI_FLASH_STROBE_2);

	/* Set CCIR registers */
	if ((sig.clk_mode == IPU_CSI_CLK_MODE_CCIR656_PROGRESSIVE) ||
	    (sig.clk_mode == IPU_CSI_CLK_MODE_CCIR656_INTERLACED)) {
#ifdef CONFIG_VIDEO_MXC_V4L1
	  /* KANG_NOTE: hardcode CCIR sync codes and enable error detection */
		__raw_writel(0x01040010, CSI_CCIR_CODE_1);
#else
		__raw_writel(0x40030, CSI_CCIR_CODE_1);
#endif
		__raw_writel(0xFF0000, CSI_CCIR_CODE_3);
	}

	printk("CSI_SENS_CONF = 0x%08X\n", __raw_readl(CSI_SENS_CONF)); // 05/16/2008     delete  printk for CSI Reg 
	printk("CSI_ACT_FRM_SIZE = 0x%08X\n", __raw_readl(CSI_ACT_FRM_SIZE)); // 05/16/2008     delete  printk for CSI Reg 

	FUNC_END;
	return 0;
}

/*!
 * ipu_csi_flash_strobe
 *
 * @param       flag         true to turn on flash strobe
 *
 * @return      0 for success
 */
void ipu_csi_flash_strobe(bool flag)
{
	FUNC_START;

	if (flag == true) {
		__raw_writel(__raw_readl(CSI_FLASH_STROBE_2) | 0x1,
			     CSI_FLASH_STROBE_2);
	}

	FUNC_END;
}

/*!
 * ipu_csi_enable_mclk
 *
 * @param       src         enum define which source to control the clk
 *                          CSI_MCLK_VF CSI_MCLK_ENC CSI_MCLK_RAW CSI_MCLK_I2C
 * @param       flag        true to enable mclk, false to disable mclk
 * @param       wait        true to wait 100ms make clock stable, false not wait
 *
 * @return      0 for success
 */
int32_t ipu_csi_enable_mclk(int src, bool flag, bool wait)
{
	if (flag == true) {
		csi_mclk_flag |= src;
	} else {
		csi_mclk_flag &= ~src;
	}

	if (gipu_csi_get_mclk_flag == flag)
		return 0;

	if (flag == true) {
#ifdef CONFIG_ARCH_MXC91231
	        mxc_set_clocks_pll(CSI_BAUD, MCUPLL);
#endif
		mxc_clks_enable(CSI_BAUD);
		if (wait == true)
			msleep(10);
#ifdef CONFIG_MXC_CAMERA_S5K3AAEX
		gpio_sensor_suspend(false);
#endif
		/*printk("enable csi clock from source %d\n", src);	*/
		gipu_csi_get_mclk_flag = true;
	} else if (csi_mclk_flag == 0) {
#ifdef CONFIG_MXC_CAMERA_S5K3AAEX
		gpio_sensor_suspend(true);
		msleep(10);
#endif
#ifdef CONFIG_ARCH_MXC91231
		mxc_set_clocks_pll(CSI_BAUD, USBPLL);
#endif
		mxc_clks_disable(CSI_BAUD);
		/*printk("disable csi clock from source %d\n", src); */
		gipu_csi_get_mclk_flag = flag;
	}

	return 0;
}

/*!
 * ipu_csi_read_mclk_flag
 *
 * @return  csi_mclk_flag
 */
int ipu_csi_read_mclk_flag(void)
{
	return csi_mclk_flag;
}

/*!
 * ipu_csi_get_window_size
 *
 * @param       width        pointer to window width
 * @param       height       pointer to window height
 *
 */
void ipu_csi_get_window_size(uint32_t * width, uint32_t * height)
{
	uint32_t reg;

	reg = __raw_readl(CSI_ACT_FRM_SIZE);
	*width = (reg & 0xFFFF) + 1;
	*height = (reg >> 16 & 0xFFFF) + 1;
}

/*!
 * ipu_csi_set_window_size
 *
 * @param       width        window width
 * @param       height       window height
 *
 */
void ipu_csi_set_window_size(uint32_t width, uint32_t height)
{
	__raw_writel((width - 1) | (height - 1) << 16, CSI_ACT_FRM_SIZE);
}

/*!
 * ipu_csi_set_window_pos
 *
 * @param       left        uint32 window x start
 * @param       top         uint32 window y start
 *
 */
void ipu_csi_set_window_pos(uint32_t left, uint32_t top)
{
	uint32_t temp = __raw_readl(CSI_OUT_FRM_CTRL);
	temp &= 0xffff0000;
	temp = top | (left << 8) | temp;
	__raw_writel(temp, CSI_OUT_FRM_CTRL);
}


EXPORT_SYMBOL(ipu_csi_enable_mclk);
EXPORT_SYMBOL(ipu_csi_init_interface);
EXPORT_SYMBOL(ipu_csi_set_window_pos);
EXPORT_SYMBOL(ipu_csi_set_window_size);
