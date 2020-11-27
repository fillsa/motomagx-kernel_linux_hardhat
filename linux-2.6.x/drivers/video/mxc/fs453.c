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
  * @file fs453.c
  * @brief Driver for FS453/4 TV encoder
  *
  * @ingroup Framebuffer
  */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/arch/mxc_i2c.h>

#include "fs453.h"

static int fs453_found = 0;

static int fs453_read(u32 reg, u32 * word, u32 len)
{
	int i;
	u8 *wp = (u8 *) word;

	*word = 0;

	for (i = 0; i < len; i++) {
		int ret = mxc_i2c_read(I2C1_BUS, FS453_I2C_ADDR,
				       (char *)&reg, 1, wp, 1);
		if (ret < 0)
			return ret;
		wp++;
		reg++;
	}
	return 0;
}

static int fs453_write(u32 reg, u32 word, u32 len)
{
	return mxc_i2c_write(I2C1_BUS, FS453_I2C_ADDR, (char *)&reg, 1,
			     (u8 *) & word, len);
}

struct fs453_presets {
	char *name;		/*! Name */
	u16 qpr;		/*! Quick Program Register */
	u16 pwr_mgmt;		/*! Power Management */
	u16 iho;		/*! Input Horizontal Offset */
	u16 ivo;		/*! Input Vertical Offset */
	u16 ihw;		/*! Input Horizontal Width */
	u16 vsc;		/*! Vertical Scaling Coefficient */
	u16 hsc;		/*! Horizontal Scaling Coefficient */
	u16 bypass;		/*! Bypass */
	u16 misc;		/*! Miscellaneous Bits Register */
	u8 misc46;		/*! Miscellaneous Bits Register 46 */
	u8 misc47;		/*! Miscellaneous Bits Register 47 */
	u32 ncon;		/*! Numerator of NCO Word */
	u32 ncod;		/*! Denominator of NCO Word */
	u16 pllm;		/*! PLL M and Pump Control */
	u16 plln;		/*! PLL N */
	u16 pllpd;		/*! PLL Post-Divider */
	u16 vid_cntl0;		/*! Video Control 0 */
	u16 dac_cntl;		/*! DAC Control */
	u16 fifo_lat;		/*! FIFO Latency */
	u16 red_scl;		/*! RGB to YCrCb Scaling Red Coef. */
	u16 grn_scl;		/*! RGB to YCrCb Scaling Grn Coef. */
	u16 blu_scl;		/*! RGB to YCrCb Scaling Blu Coef. */
};

#ifdef CONFIG_FB_MXC_TVOUT_VGA
static struct fs453_presets fs453_presets = {
	.name = "VGA Monitor mode",
	.qpr = 0x9cb0,
	.pwr_mgmt = 0x0408,
	.misc = 0x0003,
	.ncon = 0x00000000,
	.ncod = 0x00000000,
	.misc46 = 0xa9,
	.misc47 = 0x00,
	.pllm = 0x317f,
	.plln = 0x008e,
	.pllpd = 0x0202,
	.vid_cntl0 = 0x4006,
	.dac_cntl = 0x00E4,
	.fifo_lat = 0x0082,
	.red_scl = 138,
	.grn_scl = 138,
	.blu_scl = 138,
};
#endif

#ifdef CONFIG_FB_MXC_TVOUT_NTSC
static struct fs453_presets fs453_presets = {
	.name = "NTSC mode",
	.qpr = 0x9c48,
	.pwr_mgmt = 0x0200,
	.iho = 0x0004,
	.ivo = 0x0046,
	.ihw = 0x02a0,
	.vsc = 0xeaf7,
	.hsc = 0x0200,
	.bypass = 0x000a,
	.misc = 0x0002,
	.misc46 = 0x01,
	.pllm = 0x410f,
	.plln = 0x001a,
	.pllpd = 0x110b,
	.vid_cntl0 = 0x0340,
	.dac_cntl = 0x00E4,
	.fifo_lat = 0x0082,
};
#endif

#ifdef CONFIG_FB_MXC_TVOUT_PAL
static struct fs453_presets fs453_presets = {
	.name = "PAL mode",
	.qpr = 0x9c41,
	.pwr_mgmt = 0x0200,
	.iho = 0x003f,
	.ivo = 0x00e3,
	.ihw = 0x02a2,
	.vsc = 0xf9db,
	.hsc = 0x0400,
	.bypass = 0x000a,
	.misc = 0x0002,
	.misc46 = 0x01,
	.pllm = 0x410f,
	.plln = 0x001a,
	.pllpd = 0x110b,
	.vid_cntl0 = 0x0340,
	.dac_cntl = 0x00E4,
	.fifo_lat = 0x0082,
};
#endif

int fs453_setup(void)
{
	u32 data;

	if (!fs453_found)
		return -ENODEV;

	printk(KERN_ERR "fs453: %s\n", fs453_presets.name);

	/* set the clock level */

	fs453_write(FS453_CR, CR_GCC_CK_LVL, 2);

	/* soft reset the encoder */

	fs453_write(FS453_CR, data | CR_SRESET, 2);
	fs453_write(FS453_CR, data & ~CR_SRESET, 2);

	/* set up the NCO and PLL */

	fs453_write(FS453_NCON, fs453_presets.ncon, 4);
	fs453_write(FS453_NCOD, fs453_presets.ncod, 4);
	fs453_write(FS453_PLL_M_PUMP, fs453_presets.pllm, 2);
	fs453_write(FS453_PLL_N, fs453_presets.plln, 2);
	fs453_write(FS453_PLL_PDIV, fs453_presets.pllpd, 2);

	/* latch the NCO and PLL settings */

	fs453_read(FS453_CR, &data, 2);
	fs453_write(FS453_CR, data | CR_NCO_EN, 2);
	fs453_write(FS453_CR, data & ~CR_NCO_EN, 2);

	fs453_write(FS453_BYPASS, fs453_presets.bypass, 2);

	/*
	 * Write the QPR (Quick Programming Register).
	 * VGA mode jitters unless its written quite late.
	 */

	fs453_write(FS453_QPR, fs453_presets.qpr, 2);

	/* customize */

	fs453_write(FS453_PWR_MGNT, fs453_presets.pwr_mgmt, 2);

	fs453_write(FS453_IHO, fs453_presets.iho, 2);
	fs453_write(FS453_IVO, fs453_presets.ivo, 2);
	fs453_write(FS453_IHW, fs453_presets.ihw, 2);
	fs453_write(FS453_VSC, fs453_presets.vsc, 2);
	fs453_write(FS453_HSC, fs453_presets.hsc, 2);

	fs453_write(FS453_RED_SCL, fs453_presets.red_scl, 2);
	fs453_write(FS453_GRN_SCL, fs453_presets.grn_scl, 2);
	fs453_write(FS453_BLU_SCL, fs453_presets.blu_scl, 2);

	fs453_write(FS453_MISC, fs453_presets.misc, 2);

	fs453_write(FS453_VID_CNTL0, fs453_presets.vid_cntl0, 2);
	fs453_write(FS453_MISC_46, fs453_presets.misc46, 1);
	fs453_write(FS453_MISC_47, fs453_presets.misc47, 1);

	fs453_write(FS453_DAC_CNTL, fs453_presets.dac_cntl, 2);
	fs453_write(FS453_FIFO_LAT, fs453_presets.fifo_lat, 2);

	return 0;
}

static int __init fs453_init(void)
{
	int ret;
	u32 ID;

	printk(KERN_ERR
	       "Freescale FS453/4 driver, (c) 2005 Freescale Semiconductor, Inc.\n");

	ret = fs453_read(FS453_ID, &ID, 2);
	if (ret < 0 || ID != FS453_CHIP_ID) {
		printk(KERN_ERR "fs453: TV encoder not present\n");
		return -ENODEV;
	}

	printk(KERN_ERR "fs453: TV encoder present, ID: 0x%04X\n", ID);

	fs453_found = 1;

	return 0;
}

static void __exit fs453_exit(void)
{
	return;
}

module_init(fs453_init);
module_exit(fs453_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("FS453/4 TV encoder driver");
MODULE_LICENSE("GPL");
