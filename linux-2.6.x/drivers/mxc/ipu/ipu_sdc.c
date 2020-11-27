/*
 * Copyright 2005 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (c) 2006-2008 Motorola, Inc.
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
 * 10/2006  Motorola  Added support for a BGR666 display mapping and the 
 *                    power-up logo.
 * 10/2007  Motorola  Fixed SDC_CONF register operation. 
 * 11/2007  Motorola  Add a function to syn global variables
 * 01/2008  Motorola  syn IPU_CHAN_ID(MEM_SDC_BG
 */
/*!
 * @file ipu_sdc.c
 *
 * @brief IPU SDC submodule API functions
 *
 * @ingroup IPU 
 */
#include <linux/types.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include "ipu.h"
#include "ipu_prv.h"
#include "ipu_regs.h"
#include "ipu_param_mem.h"
#include "asm-arm/io.h"

#ifdef CONFIG_MOT_FEAT_BRDREV
#include <asm/boardrev.h>
#endif /* CONFIG_MOT_FEAT_BRDREV */

static uint32_t g_hStartWidth;
static uint32_t g_vStartWidth;

static const uint32_t DI_mappings[] = {
	0x1600AAAA, 0x00E05555, 0x00070000, 3,	/* RGB888 */
	0x0005000F, 0x000B000F, 0x0011000F, 1,	/* RGB666 */
	0x0011000F, 0x000B000F, 0x0005000F, 1,	/* BGR666 */
	0x0004003F, 0x000A000F, 0x000F003F, 1	/* RGB565 */
};

/*!
 * This function is called to initialize a synchronous LCD panel.
 * 
 * @param       panel           The type of panel.
 *
 * @param       refreshRate     Desired refresh rate for the panel in Hz. 
 *
 * @param       pixel_fmt       Input parameter for pixel format of buffer. Pixel 
 *                              format is a FOURCC ASCII code.
 *
 * @param       width           The width of panel in pixels.
 *
 * @param       height          The height of panel in pixels.
 *
 * @param       hStartWidth     The number of pixel clocks between the HSYNC
 *                              signal pulse and the start of valid data.
 *
 * @param       hSyncWidth      The width of the HSYNC signal in units of pixel
 *                              clocks.
 *
 * @param       hEndWidth       The number of pixel clocks between the end of
 *                              valid data and the HSYNC signal for next line.
 *
 * @param       vStartWidth     The number of lines between the VSYNC
 *                              signal pulse and the start of valid data.
 *
 * @param       vSyncWidth      The width of the VSYNC signal in units of lines
 *
 * @param       vEndWidth       The number of lines between the end of valid
 *                              data and the VSYNC signal for next frame.
 *
 * @param       sig             Bitfield of signal polarities for LCD interface.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int32_t ipu_sdc_init_panel(ipu_panel_t panel,
			   uint8_t refreshRate,
			   uint16_t width, uint16_t height,
			   uint32_t pixel_fmt,
			   uint16_t hStartWidth, uint16_t hSyncWidth,
			   uint16_t hEndWidth, uint16_t vStartWidth,
			   uint16_t vSyncWidth, uint16_t vEndWidth,
			   ipu_di_signal_cfg_t sig)
{
	uint32_t lock_flags;
	uint32_t reg;
	uint32_t old_conf;
	uint32_t div;
	uint32_t pixel_clk;
	FUNC_START;

	DPRINTK("panel size = %d x %d\n", width, height);

	if ((vSyncWidth == 0) || (hSyncWidth == 0))
		return EINVAL;

	/* Init panel size and blanking periods */
	reg =
	    ((uint32_t) (hSyncWidth - 1) << 26) |
	    ((uint32_t) (width + hStartWidth + hEndWidth - 1) << 16);
	__raw_writel(reg, SDC_HOR_CONF);

	reg = ((uint32_t) (vSyncWidth - 1) << 26) | SDC_V_SYNC_WIDTH_L |
	    ((uint32_t) (height + vStartWidth + vEndWidth - 1) << 16);
	__raw_writel(reg, SDC_VER_CONF);

	g_hStartWidth = hStartWidth;
	g_vStartWidth = vStartWidth;

        reg = __raw_readl(SDC_COM_CONF); /* read the sdc conf register */
        reg &= ~(0x00001003); /* mask out sharp and sdc_mode bits, since we will set those bits  */
	switch (panel) {
	case IPU_PANEL_SANYO_TFT:
                reg |= SDC_COM_TFT_COLOR;
#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
                reg |= (SDC_COM_FG_EN | SDC_COM_BG_EN); /* enable bg and fg if feature enabled?? */
#endif
		break;
	case IPU_PANEL_SHARP_TFT:
		__raw_writel(0x00FD0102L, SDC_SHARP_CONF_1);
		__raw_writel(0x00F500F4L, SDC_SHARP_CONF_2);
                reg |= (SDC_COM_SHARP | SDC_COM_TFT_COLOR);
		break;
	case IPU_PANEL_TFT:
                reg |= SDC_COM_TFT_COLOR;
		break;
	default:
		return EINVAL;
	}

        __raw_writel(reg, SDC_COM_CONF); /* update the sdc conf register*/

	spin_lock_irqsave(&ipu_lock, lock_flags);

	/* Init clocking */

	/* Calculate total clocks per frame */
	pixel_clk = (width + hStartWidth + hSyncWidth + hEndWidth) *
	    (height + vStartWidth + vSyncWidth + vEndWidth) * refreshRate;

	/* Calculate divider */
	/* fractional part is 4 bits so simply multiple by 2^4 to get fractional part */
	DPRINTK("pixel clk = %d\n", pixel_clk);
	div = (g_ipu_clk * 16) / pixel_clk;
	if (div < 0x40)		/* Divider less than 4 */
	{
		printk("InitPanel() - Pixel clock divider less than 1\n");
		div = 0x40;
	}
	/* DISP3_IF_CLK_DOWN_WR is half the divider value and 2 less fraction bits */
	/* Subtract 1 extra from DISP3_IF_CLK_DOWN_WR based on timing debug	*/
	/* DISP3_IF_CLK_UP_WR is 0 */

        /* DI_DISP3_TIME_CONF has the lower 12 bits for DISP3_IF_CLK_PER_WR
         * Out of these 12 bits, lower 4 bits are for fractional portion of the
         * pixel clock. Zero out the fractional portion to have an integer pixelclock
         * */
        div = div & 0xFFFFFFF0;
	__raw_writel((((div / 8) - 1) << 22) | div, DI_DISP3_TIME_CONF);

	/* DI settings */
	old_conf = __raw_readl(DI_DISP_IF_CONF) & 0x78FFFFFF;
	old_conf |= sig.datamask_en << DI_D3_DATAMSK_SHIFT |
	    sig.clksel_en << DI_D3_CLK_SEL_SHIFT |
	    sig.clkidle_en << DI_D3_CLK_IDLE_SHIFT;
	__raw_writel(old_conf, DI_DISP_IF_CONF);

	old_conf = __raw_readl(DI_DISP_SIG_POL) & 0xE0FFFFFF;
	old_conf |= sig.data_pol << DI_D3_DATA_POL_SHIFT |
	    sig.clk_pol << DI_D3_CLK_POL_SHIFT |
	    sig.enable_pol << DI_D3_DRDY_SHARP_POL_SHIFT |
	    sig.Hsync_pol << DI_D3_HSYNC_POL_SHIFT |
	    sig.Vsync_pol << DI_D3_VSYNC_POL_SHIFT;
	__raw_writel(old_conf, DI_DISP_SIG_POL);

#if defined(CONFIG_MOT_FIX_ASCENSION) && defined(CONFIG_MOT_FEAT_BRDREV)
    /* Ascension HW has red and blue lines swapped on the display interface.
     * This format allows a work around it by defining a new DI mapping.
     * Input data/internal DI format: 24 bits/ 3 byte components
     * ------------------------
     *    2         1         
     * 321098765432109876543210
     * ------------------------
     * RRRRRRRRGGGGGGGGBBBBBBBB
     * Output data for DISP 3/main display: 18 bits
     * ------------------
     *        1          
     * 765432109876543210
     * ------------------
     * BBBBBBGGGGGGRRRRRR
     * ------------------
     * [Blue] BYTE 0: OFS0=17 OFS1=0 OFS2=0 M0=11 M1=11 M2 through M7=00
     * [Green]BYTE 1: OFS0=11 OFS1=0 OFS2=0 M0=11 M1=11 M2 through M7=00
     * [Red]  BYTE 2: OFS0=05 OFS1=0 OFS2=0 M0=11 M1=11 M2 through M7=00
     *  which implies that the B/G/R 2-7[6 msb] bits are available at offsets
     *  17,11 and 5 during the first display cycle and one cycle is required
     *  to output a pixel.
     */
     if( (boardrev() < BOARDREV_P2A) || (boardrev() == BOARDREV_UNKNOWN)) {
	 /* 
	  * The HW issue with P1 would be fixed in P2 and assumption is that
	  * the board revision will be set for that release.
	  */  
          pixel_fmt= IPU_PIX_FMT_BGR666;
     }
#endif /* CONFIG_MOT_FIX_ASCENSION && CONFIG_MOT_FEAT_BRDREV */

	switch (pixel_fmt) {
	case IPU_PIX_FMT_RGB24:
		__raw_writel(DI_mappings[0], DI_DISP3_B0_MAP);
		__raw_writel(DI_mappings[1], DI_DISP3_B1_MAP);
		__raw_writel(DI_mappings[2], DI_DISP3_B2_MAP);
		__raw_writel(__raw_readl(DI_DISP_ACC_CC) |
			     ((DI_mappings[3] - 1) << 12), DI_DISP_ACC_CC);
		break;
	case IPU_PIX_FMT_RGB666:
		__raw_writel(DI_mappings[4], DI_DISP3_B0_MAP);
		__raw_writel(DI_mappings[5], DI_DISP3_B1_MAP);
		__raw_writel(DI_mappings[6], DI_DISP3_B2_MAP);
		__raw_writel(__raw_readl(DI_DISP_ACC_CC) |
			     ((DI_mappings[7] - 1) << 12), DI_DISP_ACC_CC);
		break;
	case IPU_PIX_FMT_BGR666:
		__raw_writel(DI_mappings[8], DI_DISP3_B0_MAP);
		__raw_writel(DI_mappings[9], DI_DISP3_B1_MAP);
		__raw_writel(DI_mappings[10], DI_DISP3_B2_MAP);
		__raw_writel(__raw_readl(DI_DISP_ACC_CC) | 
			     ((DI_mappings[11] - 1) << 12), DI_DISP_ACC_CC);
		break;
	default:
		__raw_writel(DI_mappings[12], DI_DISP3_B0_MAP);
		__raw_writel(DI_mappings[13], DI_DISP3_B1_MAP);
		__raw_writel(DI_mappings[14], DI_DISP3_B2_MAP);
		__raw_writel(__raw_readl(DI_DISP_ACC_CC) |
			     ((DI_mappings[15] - 1) << 12), DI_DISP_ACC_CC);
		break;
	}

	spin_unlock_irqrestore(&ipu_lock, lock_flags);

	DPRINTK("DI_DISP_IF_CONF = 0x%08X\n", __raw_readl(DI_DISP_IF_CONF));
	DPRINTK("DI_DISP_SIG_POL = 0x%08X\n", __raw_readl(DI_DISP_SIG_POL));
	DPRINTK("DI_DISP3_TIME_CONF = 0x%08X\n",
		__raw_readl(DI_DISP3_TIME_CONF));

	FUNC_END;
	return 0;
}

/*!
 * This function sets the foreground and background plane global alpha blending
 * modes.
 *
 * @param       enable          Boolean to enable or disable global alpha
 *                              blending. If disabled, per pixel blending is used.
 *
 * @param       alpha           Global alpha value.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */
int32_t ipu_sdc_set_global_alpha(bool enable, uint8_t alpha)
{
	uint32_t reg;
	uint32_t lock_flags;

	FUNC_START;

	spin_lock_irqsave(&ipu_lock, lock_flags);

	if (enable) {
		reg = __raw_readl(SDC_GW_CTRL) & 0x00FFFFFFL;
		__raw_writel(reg | ((uint32_t) alpha << 24), SDC_GW_CTRL);

		reg = __raw_readl(SDC_COM_CONF);
		__raw_writel(reg | SDC_COM_GLB_A, SDC_COM_CONF);
	} else {
		reg = __raw_readl(SDC_COM_CONF);
		__raw_writel(reg & ~SDC_COM_GLB_A, SDC_COM_CONF);
	}

	spin_unlock_irqrestore(&ipu_lock, lock_flags);

	FUNC_END;
	return 0;
}

/*!
 * This function sets the transparent color key for SDC graphic plane.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       enable          Boolean to enable or disable color key
 *
 * @param       colorKey        24-bit RGB color to use as transparent color key.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */
int32_t ipu_sdc_set_color_key(ipu_channel_t channel, bool enable,
			      uint32_t colorKey)
{
	uint32_t reg, sdc_conf;
	uint32_t lock_flags;

	FUNC_START;

	spin_lock_irqsave(&ipu_lock, lock_flags);

	sdc_conf = __raw_readl(SDC_COM_CONF);
	if (channel == MEM_SDC_BG) {
		sdc_conf &= ~SDC_COM_GWSEL;
	} else {
		sdc_conf |= SDC_COM_GWSEL;
	}

	if (enable) {
		reg = __raw_readl(SDC_GW_CTRL) & 0xFF000000L;
		__raw_writel(reg | (colorKey & 0x00FFFFFFL), SDC_GW_CTRL);

		sdc_conf |= SDC_COM_KEY_COLOR_G;
	} else {
		sdc_conf &= ~SDC_COM_KEY_COLOR_G;
	}
	__raw_writel(sdc_conf, SDC_COM_CONF);

	spin_unlock_irqrestore(&ipu_lock, lock_flags);

	FUNC_END;
	return 0;
}

int32_t ipu_sdc_set_brightness(uint8_t value)
{
	FUNC_START;

	__raw_writel(0x03000000UL | value << 16, SDC_PWM_CTRL);

	FUNC_END;
	return 0;
}

/*!
 * This function sets the window position of the foreground or background plane.
 * modes.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       x_pos           The X coordinate position to place window at.
 *                              The position is relative to the top left corner.
 *
 * @param       y_pos           The Y coordinate position to place window at.
 *                              The position is relative to the top left corner.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */
int32_t ipu_sdc_set_window_pos(ipu_channel_t channel, int16_t x_pos,
			       int16_t y_pos)
{
	FUNC_START;

#ifndef CONFIG_VIRTIO_SUPPORT	
	x_pos += g_hStartWidth;
	y_pos += g_vStartWidth;
#endif

	if (channel == MEM_SDC_BG) {
		__raw_writel((x_pos << 16) | y_pos, SDC_BG_POS);
	} else if (channel == MEM_SDC_FG) {
#if defined(CONFIG_ARCH_MXC91231)	/* IPU rev 1 */
		if (system_rev == CHIP_REV_1_0) {
			x_pos++;	/* FG x position must be greater than BG for some reason */
		}
#endif
		__raw_writel((x_pos << 16) | y_pos, SDC_FG_POS);
	} else {
		return EINVAL;
	}

	FUNC_END;
	return 0;
}

void _ipu_sdc_fg_init(ipu_channel_params_t * params)
{
	uint32_t reg;
	(void)params;

	FUNC_START;

	/* Enable FG channel */
	reg = __raw_readl(SDC_COM_CONF);
	__raw_writel(reg | SDC_COM_FG_EN | SDC_COM_BG_EN, SDC_COM_CONF);

	FUNC_END;
}

uint32_t _ipu_sdc_fg_uninit(void)
{
	uint32_t reg;

	FUNC_START;

	/* Disable FG channel */
	reg = __raw_readl(SDC_COM_CONF);
	__raw_writel(reg & ~SDC_COM_FG_EN, SDC_COM_CONF);

	FUNC_END;
	return (reg & SDC_COM_FG_EN);
}

void _ipu_sdc_bg_init(ipu_channel_params_t * params)
{
	uint32_t reg;
	(void)params;

	FUNC_START;

	/* Enable FG channel */
	reg = __raw_readl(SDC_COM_CONF);
	__raw_writel(reg | SDC_COM_BG_EN, SDC_COM_CONF);

	FUNC_END;
}

uint32_t _ipu_sdc_bg_uninit(void)
{
	uint32_t reg;

	FUNC_START;

	/* Disable BG channel */
	reg = __raw_readl(SDC_COM_CONF);
	__raw_writel(reg & ~SDC_COM_BG_EN, SDC_COM_CONF);

	FUNC_END;
	return (reg & SDC_COM_BG_EN);
}

extern uint32_t gChannelInitMask;

int32_t ipu_sdc_sync_bg_window_pos_to_global(void)
{
        FUNC_START;
        int32_t reg;
        reg = __raw_readl(SDC_BG_POS);
        g_hStartWidth = (reg >> 16);
        g_vStartWidth = (reg & 0x0000FFFF);
    
        gChannelInitMask |= (1L << IPU_CHAN_ID(MEM_SDC_BG));
 
        FUNC_END;
        return 0;
}

EXPORT_SYMBOL(ipu_sdc_set_color_key);
EXPORT_SYMBOL(ipu_sdc_set_global_alpha);
EXPORT_SYMBOL(ipu_sdc_set_window_pos);
EXPORT_SYMBOL(ipu_sdc_init_panel);
EXPORT_SYMBOL(ipu_sdc_sync_bg_window_pos_to_global);

