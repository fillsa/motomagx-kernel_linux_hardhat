/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2008 Motorola, Inc. 
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Date     Author      Comment
 * 10/2006  Motorola    Removed unnecessary delays in ipu_adc_write_cmd
 *                      and filed a WFN bug fix for DISP2 clock polarity.
 * 06/2007  Motorola    Fix to support setting of ADC serial interface
 *                      bit width in ipu driver.
 * 08/2007  Motorola    Add comments for oss compliance.
 * 08/2007  Motorola    Add comments.
 * 01/2008  Motorola    Add error check in ipu_adc_write_cmd.
 * 02/2008  Motorola    Fix signal polarity issue in ipu_adc_init_panel() for DISP0
 * 04/2008  Motorola    Add code for new display
 */

/*
 * @file ipu_adc.c
 *
 * @brief IPU ADC functions
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
#include "ipu_param_mem.h"
#include "asm-arm/io.h"
#include <asm/arch/board.h>
#include "asm/arch/clock.h"

/* M3IF Snooping Configuration Register 0 (M3IFSCFG0) READ/WRITE*/
#define M3IFSCFG0 IO_ADDRESS(M3IF_BASE_ADDR + 0x28)
/* M3IF Snooping Configuration Register 1 (M3IFSCFG1) READ/WRITE*/
#define M3IFSCFG1 IO_ADDRESS(M3IF_BASE_ADDR + 0x2C)
/* M3IF Snooping Configuration Register 2 (M3IFSCFG2) READ/WRITE*/
#define M3IFSCFG2 IO_ADDRESS(M3IF_BASE_ADDR + 0x30)
/* M3IF Snooping Status Register 0 (M3IFSSR0) READ/WRITE */
#define M3IFSSR0 IO_ADDRESS(M3IF_BASE_ADDR + 0x34)
/* M3IF Snooping Status Register 1 (M3IFSSR1) */
#define M3IFSSR1 IO_ADDRESS(M3IF_BASE_ADDR + 0x38)

/*#define ADC_CHAN1_SA_MASK 0xFF800000 */

static void _ipu_set_cmd_data_mappings(display_port_t disp,
				       uint32_t pixel_fmt, int ifc_width);

int32_t _ipu_adc_init_channel(ipu_channel_t chan, display_port_t disp,
			      mcu_mode_t cmd, int16_t x_pos, int16_t y_pos)
{
	uint32_t reg;
	uint32_t start_addr, stride;
	uint32_t lock_flags;
	uint32_t size;

	size = 0; 

	switch (disp) {
	case DISP0:
		reg = __raw_readl(ADC_DISP0_CONF);
		stride = reg & ADC_DISP_CONF_SL_MASK;
		break;
	case DISP1:
		reg = __raw_readl(ADC_DISP1_CONF);
		stride = reg & ADC_DISP_CONF_SL_MASK;
		break;
	case DISP2:
		reg = __raw_readl(ADC_DISP2_CONF);
		stride = reg & ADC_DISP_CONF_SL_MASK;
		break;
	default:
		return -EINVAL;
	}

	if (stride == 0)
		return -EINVAL;

	stride++;
	start_addr = (y_pos * stride) + x_pos;

	spin_lock_irqsave(&ipu_lock, lock_flags);
	reg = __raw_readl(ADC_CONF);

	switch (chan) {
	case ADC_SYS1:
		reg &= ~0x00FF4000;
		reg |=
		    ((uint32_t) size << 21 | (uint32_t) disp << 19 | (uint32_t)
		     cmd << 16);

		__raw_writel(start_addr, ADC_SYSCHA1_SA);
		break;

	case ADC_SYS2:
		reg &= ~0xFF008000;
		reg |=
		    ((uint32_t) size << 29 | (uint32_t) disp << 27 | (uint32_t)
		     cmd << 24);

		__raw_writel(start_addr, ADC_SYSCHA2_SA);
		break;

	case CSI_PRP_VF_ADC:
	case MEM_PRP_VF_ADC:
		reg &= ~0x000000F9;
		reg |=
		    ((uint32_t) size << 5 | (uint32_t) disp << 3 |
		     ADC_CONF_PRP_EN);

		__raw_writel(start_addr, ADC_PRPCHAN_SA);
		break;

	case MEM_PP_ADC:
		reg &= ~0x00003F02;
		reg |=
		    ((uint32_t) size << 10 | (uint32_t) disp << 8 |
		     ADC_CONF_PP_EN);

		__raw_writel(start_addr, ADC_PPCHAN_SA);
		break;
	default:
		spin_unlock_irqrestore(&ipu_lock, lock_flags);
		return -1;
		break;
	}
	__raw_writel(reg, ADC_CONF);
	spin_unlock_irqrestore(&ipu_lock, lock_flags);
	return 0;
}

int32_t _ipu_adc_uninit_channel(ipu_channel_t chan)
{
	uint32_t reg;
	uint32_t lock_flags;

	spin_lock_irqsave(&ipu_lock, lock_flags);
	reg = __raw_readl(ADC_CONF);

	switch (chan) {
	case ADC_SYS1:
		reg &= ~0x00FF4000;
		break;
	case ADC_SYS2:
		reg &= ~0xFF008000;
		break;
	case CSI_PRP_VF_ADC:
	case MEM_PRP_VF_ADC:
		reg &= ~0x000000F9;
		break;
	case MEM_PP_ADC:
		reg &= ~0x00003F02;
		break;
	default:
		spin_unlock_irqrestore(&ipu_lock, lock_flags);
		return -1;
		break;
	}
	__raw_writel(reg, ADC_CONF);
	spin_unlock_irqrestore(&ipu_lock, lock_flags);
	return 0;
}

int32_t ipu_adc_write_template(display_port_t disp, uint32_t * pCmd, bool write)
{
	uint32_t ima_addr = 0;
	uint32_t row_nu;
	int i;

	/* Set IPU_IMA_ADDR (IPU Internal Memory Access Address) */
	/* MEM_NU = 0x0001 (CPM) */
	/* ROW_NU = 2*N ( N is channel number) */
	/* WORD_NU = 0	*/
	if (write) {
		row_nu = (uint32_t) disp *2 * ATM_ADDR_RANGE;
	} else {
		row_nu = ((uint32_t) disp * 2 + 1) * ATM_ADDR_RANGE;
	}

	/* form template addr for IPU_IMA_ADDR */
	ima_addr = (0x3 << 16 /*Template memory */  | row_nu << 3);

	__raw_writel(ima_addr, IPU_IMA_ADDR);

	/* write template data for IPU_IMA_DATA	*/
	for (i = 0; i < TEMPLATE_BUF_SIZE; i++)
		/* only DATA field are needed */
		__raw_writel(pCmd[i], IPU_IMA_DATA);

	return 0;
}

int32_t
ipu_adc_write_cmd(display_port_t disp, cmddata_t type,
		  uint32_t cmd, const uint32_t * params, uint16_t numParams)
{
	uint16_t i;
        uint32_t ipu_conf_reg_val;

        ipu_conf_reg_val = __raw_readl(IPU_CONF) & (IPU_CONF_DI_EN | IPU_CONF_ADC_EN);
        if(ipu_conf_reg_val != (IPU_CONF_DI_EN | IPU_CONF_ADC_EN))
        {
            printk(KERN_ALERT " adc write command error, di_en=%08X or adc_en=%08X\n", 
                              ipu_conf_reg_val & IPU_CONF_DI_EN, ipu_conf_reg_val & IPU_CONF_ADC_EN);
            printk(KERN_ALERT " adc write command error, cmd=%d(%08X), numParams=%d, params=%d\n", 
                              cmd, cmd, numParams, params[0]);
            return 0;    /*no error code is returned*/
        }

	__raw_writel((uint32_t) ((type == CMD ? 0x0 : 0x1) | disp << 1 | 0x10),
		     DI_DISP_LLA_CONF);
	__raw_writel(cmd, DI_DISP_LLA_DATA);


        if (params)
        {
	        __raw_writel((uint32_t) (disp << 1 | 0x11), DI_DISP_LLA_CONF);
	        for (i = 0; i < numParams; i++) {
		        __raw_writel(params[i], DI_DISP_LLA_DATA);
	        }
        }
	return 0;
}

int32_t ipu_adc_set_update_mode(ipu_channel_t channel,
				ipu_adc_update_mode_t mode,
				uint32_t refresh_rate, unsigned long addr,
				uint32_t * size)
{
	int32_t err = 0;
	uint32_t ref_per, reg, src = 0;
	uint32_t lock_flags;

	spin_lock_irqsave(&ipu_lock, lock_flags);

	reg = __raw_readl(IPU_FS_DISP_FLOW);
	reg &= ~FS_AUTO_REF_PER_MASK;
	switch (mode) {
	case IPU_ADC_REFRESH_NONE:
		src = 0;
		break;
	case IPU_ADC_AUTO_REFRESH:
		if (refresh_rate == 0) {
			err = -EINVAL;
			goto err0;
		}
		ref_per = (refresh_rate * g_ipu_clk) / 217;
		ref_per--;
		reg |= ref_per << FS_AUTO_REF_PER_OFFSET;

		src = FS_SRC_AUTOREF;
		break;
	case IPU_ADC_AUTO_REFRESH_SNOOP:
		if (refresh_rate == 0) {
			err = -EINVAL;
			goto err0;
		}
		ref_per = (refresh_rate * g_ipu_clk) / 217;
		ref_per--;
		reg |= ref_per << FS_AUTO_REF_PER_OFFSET;

		src = FS_SRC_AUTOREF_SNOOP;
		break;
	case IPU_ADC_SNOOPING:
		src = FS_SRC_SNOOP;
		break;
	}

	switch (channel) {
	case ADC_SYS1:
		reg &= ~FS_ADC1_SRC_SEL_MASK;
		reg |= src << FS_ADC1_SRC_SEL_OFFSET;
		break;
	case ADC_SYS2:
		reg &= ~FS_ADC2_SRC_SEL_MASK;
		reg |= src << FS_ADC2_SRC_SEL_OFFSET;
		break;
	default:
		spin_unlock_irqrestore(&ipu_lock, lock_flags);
		return -EINVAL;
	}
	__raw_writel(reg, IPU_FS_DISP_FLOW);

	/* Setup M3IF for snooping */
	if ((mode == IPU_ADC_AUTO_REFRESH_SNOOP) || (mode == IPU_ADC_SNOOPING)) {
		uint32_t msb;
		uint32_t seg_size;
		uint32_t window_size;
		int i;

		if ((addr == 0) || (size == NULL) || (*size == 0)) {
			err = -EINVAL;
			goto err0;
		}

		msb = fls(*size);
		if (!(*size & ((1UL << msb) - 1)))
			msb--;	/* Already aligned to power 2 */
		if (msb < 11)
			msb = 11;

		window_size = (1UL << msb);
		seg_size = window_size / 64;

		msb -= 11;

		reg = addr & ~((1UL << msb) - 1);
		reg |= msb << 1;
		reg |= 1;	/* enable snooping */
		__raw_writel(reg, M3IFSCFG0);

		reg = 0;
		for (i = 0; i < 32; i++) {
			if (i * seg_size >= *size)
				break;
			reg |= 1UL << i;
		}
		__raw_writel(reg, M3IFSCFG1);

		reg = 0;
		for (i = 32; i < 64; i++) {
			if (i * seg_size >= *size)
				break;
			reg |= 1UL << (i - 32);
		}
		__raw_writel(reg, M3IFSCFG2);

		*size = window_size;

		DPRINTK
		    ("Snooping enabled: window size = 0x%X, M3IFSCFG0=0x%08X, M3IFSCFG1=0x%08X, M3IFSCFG2=0x%08X\n",
		     window_size, __raw_readl(M3IFSCFG0),
		     __raw_readl(M3IFSCFG1), __raw_readl(M3IFSCFG2));
	}

      err0:
	spin_unlock_irqrestore(&ipu_lock, lock_flags);
	return err;
}

int32_t ipu_adc_get_snooping_status(uint32_t * statl, uint32_t * stath)
{
	*statl = __raw_readl(M3IFSSR0);
	*stath = __raw_readl(M3IFSSR1);
	/* DPRINTK("status = 0x%08X%08X\n", stat[1], stat[0]); */

	__raw_writel(0x0, M3IFSSR0);
	__raw_writel(0x0, M3IFSSR1);
	return 0;
}

int32_t ipu_adc_init_panel(display_port_t disp,
			   uint16_t width, uint16_t height,
			   uint32_t pixel_fmt,
			   uint32_t stride,
			   ipu_adc_sig_cfg_t sig,
			   display_addressing_t addr,
			   uint32_t vsync_width, vsync_t mode)
{
	uint32_t temp;
	uint32_t lock_flags;
	uint32_t ser_conf;
	uint32_t disp_conf;
	uint32_t adc_disp_conf;
	uint32_t adc_disp_vsync;
	uint32_t old_pol;

	if ((disp != DISP1) && (disp != DISP2) &&
	    (sig.ifc_mode >= IPU_ADC_IFC_MODE_3WIRE_SERIAL)) {
		return -EINVAL;
	}
/*        adc_disp_conf = ((uint32_t)((((size == 3)||(size == 2))?1:0)<<14) |  */
/*                         (uint32_t)addr<<12 | (stride-1)); */
	adc_disp_conf = (uint32_t) addr << 12 | (stride - 1);

	_ipu_set_cmd_data_mappings(disp, pixel_fmt, sig.ifc_width);

	spin_lock_irqsave(&ipu_lock, lock_flags);
	disp_conf = __raw_readl(DI_DISP_IF_CONF);
	old_pol = __raw_readl(DI_DISP_SIG_POL);
	adc_disp_vsync = __raw_readl(ADC_DISP_VSYNC);

	switch (disp) {
	case DISP0:
		__raw_writel(adc_disp_conf, ADC_DISP0_CONF);
		__raw_writel((((height - 1) << 16) | (width - 1)),
			     ADC_DISP0_SS);

		adc_disp_vsync &= ~(ADC_DISP_VSYNC_D0_MODE_MASK |
				    ADC_DISP_VSYNC_D0_WIDTH_MASK);
		adc_disp_vsync |= (vsync_width << 16) | (uint32_t) mode;

		old_pol &= ~0x2000003FL;
		old_pol |= sig.data_pol | sig.cs_pol << 1 |
		    sig.addr_pol << 2 | sig.read_pol << 4 |
		    sig.write_pol << 3 | sig.Vsync_pol << 5 |
		    sig.burst_pol << 29;
		__raw_writel(old_pol, DI_DISP_SIG_POL);

		disp_conf &= ~0x0000001FL;
		disp_conf |= (sig.burst_mode << 3) | (sig.ifc_mode << 1) |
		    DI_CONF_DISP0_EN;
		__raw_writel(disp_conf, DI_DISP_IF_CONF);
		break;
	case DISP1:
		__raw_writel(adc_disp_conf, ADC_DISP1_CONF);
		__raw_writel((((height - 1) << 16) | (width - 1)),
			     ADC_DISP12_SS);

		adc_disp_vsync &= ~(ADC_DISP_VSYNC_D12_MODE_MASK |
				    ADC_DISP_VSYNC_D12_WIDTH_MASK);
		adc_disp_vsync |= (vsync_width << 16) | (uint32_t) mode;

		old_pol &= ~0x4000FF00L;
		old_pol |= (uint32_t) (sig.Vsync_pol << 6 | sig.data_pol << 8 |
				       sig.cs_pol << 9 | sig.addr_pol << 10 |
				       sig.read_pol << 11 | sig.
				       write_pol << 12 | sig.
				       clk_pol << 14 | sig.burst_pol << 30);
		__raw_writel(old_pol, DI_DISP_SIG_POL);

		disp_conf &= ~0x00003F00L;
		if (sig.ifc_mode >= IPU_ADC_IFC_MODE_3WIRE_SERIAL) {
			ser_conf =
			    (sig.ifc_width -
			     1) << DI_SER_DISPx_CONF_SER_BIT_NUM_OFFSET;
			if (sig.ser_preamble_len) {
				ser_conf |= DI_SER_DISPx_CONF_PREAMBLE_EN;
				ser_conf |=
				    sig.
				    ser_preamble <<
				    DI_SER_DISPx_CONF_PREAMBLE_OFFSET;
				ser_conf |=
				    (sig.ser_preamble_len -
				     1) <<
				    DI_SER_DISPx_CONF_PREAMBLE_LEN_OFFSET;

			}

			ser_conf |=
			    sig.ser_rw_mode << DI_SER_DISPx_CONF_RW_CFG_OFFSET;

			if (sig.burst_mode == IPU_ADC_BURST_SERIAL)
				ser_conf |= DI_SER_DISPx_CONF_BURST_MODE_EN;
			__raw_writel(ser_conf, DI_SER_DISP1_CONF);
		} else {	/* parallel interface */
			disp_conf |= (uint32_t) (sig.burst_mode << 12);
		}
		disp_conf |= (sig.ifc_mode << 9) | DI_CONF_DISP1_EN;
		__raw_writel(disp_conf, DI_DISP_IF_CONF);
		break;
	case DISP2:
		__raw_writel(adc_disp_conf, ADC_DISP2_CONF);
		__raw_writel((((height - 1) << 16) | (width - 1)),
			     ADC_DISP12_SS);

		adc_disp_vsync &= ~(ADC_DISP_VSYNC_D12_MODE_MASK |
				    ADC_DISP_VSYNC_D12_WIDTH_MASK);
		adc_disp_vsync |= (vsync_width << 16) | (uint32_t) mode;

		old_pol &= ~0x80FF0000L;
		temp = (uint32_t) (sig.data_pol << 16 | sig.cs_pol << 17 |
				   sig.addr_pol << 18 | sig.read_pol << 19 |
				   sig.write_pol << 20 | sig.Vsync_pol << 6 |
#if defined(CONFIG_MOT_WFN418)
                                   sig.burst_pol << 31 | sig.clk_pol << 22);
#else
				   sig.burst_pol << 31);
#endif
		__raw_writel(temp | old_pol, DI_DISP_SIG_POL);

		disp_conf &= ~0x003F0000L;
		if (sig.ifc_mode >= IPU_ADC_IFC_MODE_3WIRE_SERIAL) {
			ser_conf =
			    (sig.ifc_width -
			     1) << DI_SER_DISPx_CONF_SER_BIT_NUM_OFFSET;
			if (sig.ser_preamble_len) {
				ser_conf |= DI_SER_DISPx_CONF_PREAMBLE_EN;
				ser_conf |=
				    sig.
				    ser_preamble <<
				    DI_SER_DISPx_CONF_PREAMBLE_OFFSET;
				ser_conf |=
				    (sig.ser_preamble_len -
				     1) <<
				    DI_SER_DISPx_CONF_PREAMBLE_LEN_OFFSET;

			}

			ser_conf |=
			    sig.ser_rw_mode << DI_SER_DISPx_CONF_RW_CFG_OFFSET;

			if (sig.burst_mode == IPU_ADC_BURST_SERIAL)
				ser_conf |= DI_SER_DISPx_CONF_BURST_MODE_EN;
			__raw_writel(ser_conf, DI_SER_DISP2_CONF);
		} else {	/* parallel interface */
			disp_conf |= (uint32_t) (sig.burst_mode << 20);
		}
		disp_conf |= (sig.ifc_mode << 17) | DI_CONF_DISP2_EN;
		__raw_writel(disp_conf, DI_DISP_IF_CONF);
		break;
	default:
		break;
	}

	__raw_writel(adc_disp_vsync, ADC_DISP_VSYNC);

	spin_unlock_irqrestore(&ipu_lock, lock_flags);

	return 0;
}

int32_t ipu_adc_init_ifc_timing(display_port_t disp, bool read,
				uint32_t cycle_time,
				uint32_t up_time,
				uint32_t down_time,
				uint32_t read_latch_time, uint32_t pixel_clk)
{
	uint32_t reg;
	uint32_t time_conf3 = 0;
	uint32_t clk_per;
	uint32_t up_per;
	uint32_t down_per;
	uint32_t read_per;
	uint32_t pixclk_per = 0;

	clk_per = (cycle_time * (g_ipu_clk / 1000L) * 16L) / 1000000L;
	up_per = (up_time * (g_ipu_clk / 1000L) * 4L) / 1000000L;
	down_per = (down_time * (g_ipu_clk / 1000L) * 4L) / 1000000L;

	reg = (clk_per << DISPx_IF_CLK_PER_OFFSET) |
	    (up_per << DISPx_IF_CLK_UP_OFFSET) |
	    (down_per << DISPx_IF_CLK_DOWN_OFFSET);

	if (read) {
		read_per =
		    (read_latch_time * (g_ipu_clk / 1000L) * 4L) / 1000000L;
		if (pixel_clk)
			pixclk_per = (g_ipu_clk * 16L) / pixel_clk;
		time_conf3 = (read_per << DISPx_IF_CLK_READ_EN_OFFSET) |
		    (pixclk_per << DISPx_PIX_CLK_PER_OFFSET);
	}

	DPRINTK("DI_DISPx_TIME_CONF_1/2 = 0x%08X\n", reg);
	DPRINTK("DI_DISPx_TIME_CONF_3 = 0x%08X\n", time_conf3);

	switch (disp) {
	case DISP0:
		if (read) {
			__raw_writel(reg, DI_DISP0_TIME_CONF_2);
			__raw_writel(time_conf3, DI_DISP0_TIME_CONF_3);
		} else {
			__raw_writel(reg, DI_DISP0_TIME_CONF_1);
		}
		break;
	case DISP1:
		if (read) {
			__raw_writel(reg, DI_DISP1_TIME_CONF_2);
			__raw_writel(time_conf3, DI_DISP1_TIME_CONF_3);
		} else {
			__raw_writel(reg, DI_DISP1_TIME_CONF_1);
		}
		break;
	case DISP2:
		if (read) {
			__raw_writel(reg, DI_DISP2_TIME_CONF_2);
			__raw_writel(time_conf3, DI_DISP2_TIME_CONF_3);
		} else {
			__raw_writel(reg, DI_DISP2_TIME_CONF_1);
		}
		break;
	default:
		return -EINVAL;
		break;
	}

	return 0;
}

int32_t
ipu_adc_set_ifc_width(display_port_t disp, uint32_t pixel_fmt, uint16_t ifc_width)
{
    uint32_t lock_flags;
    uint32_t reg;
    uint32_t adc_disp_conf;
    
    /* Set DI mapping for selected width/pixel format */
    _ipu_set_cmd_data_mappings(disp, pixel_fmt, ifc_width);

    spin_lock_irqsave(&ipu_lock, lock_flags);
    
    /* Set bit interface to desired width */
    switch(disp) // which DISP was specified?
    {
        case DISP1:
            adc_disp_conf = DI_SER_DISP1_CONF;
        break;

        case DISP2:
            adc_disp_conf = DI_SER_DISP2_CONF;
        break;

        default:
        	spin_unlock_irqrestore(&ipu_lock, lock_flags);
            return -EINVAL;
    } // switch

    reg = __raw_readl(adc_disp_conf);
    reg &= ~0x001F0000L;                 /* Clear DISPx_SER_BIT_NUM field: bits 16-20 */
    reg |= (uint32_t)(((ifc_width -1) & 0x0000001F) << DI_SER_DISPx_CONF_SER_BIT_NUM_OFFSET);
    __raw_writel(reg, adc_disp_conf);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    
    return 0;
}

struct ipu_adc_di_map {
	uint32_t map_byte1;
	uint32_t map_byte2;
	uint32_t map_byte3;
	uint32_t cycle_cnt;
};

static const struct ipu_adc_di_map di_mappings[] = {
	{
	 /* RGB888, 8-bit bus */
	 .map_byte1 = 0x1C00AAAA,
	 .map_byte2 = 0x00E05555,
	 .map_byte3 = 0x00070000,
	 .cycle_cnt = 3,
	 }, {
	     /* RGB666, 8-bit bus */
	     .map_byte1 = 0x1C00AAAF,
	     .map_byte2 = 0x00E0555F,
	     .map_byte3 = 0x0007000F,
	     .cycle_cnt = 3,
	     }, {
		 /* RGB565, 8-bit bus */
		 .map_byte1 = 0x008055BF,
		 .map_byte2 = 0x0142015F,
		 .map_byte3 = 0x0007003F,
		 .cycle_cnt = 2,
		 }, {
		     /* RGB888, 24-bit bus */
		     .map_byte1 = 0x0007000F,
		     .map_byte2 = 0x000F000F,
		     .map_byte3 = 0x0017000F,
		     .cycle_cnt = 1,
		     }, {
			 /* RGB666, 18-bit bus */
			 .map_byte1 = 0x0005000F,
			 .map_byte2 = 0x000B000F,
			 .map_byte3 = 0x0011000F,
			 .cycle_cnt = 1,
			 }, {
			     /* RGB565, 16-bit bus */
			     .map_byte1 = 0x0004003F,
			     .map_byte2 = 0x000A000F,
			     .map_byte3 = 0x000F003F,
			     .cycle_cnt = 1,
			     }, {
                     /* RGB888, 16-bit bus */
                     .map_byte1 = 0x000F0000,
                     .map_byte2 = 0x00070000,
                     .map_byte3 = 0x00000000,
                     .cycle_cnt = 1,
                     } 
};

/* Private methods */
static void _ipu_set_cmd_data_mappings(display_port_t disp,
				       uint32_t pixel_fmt, int ifc_width)
{
	uint32_t reg;
	u32 map = 0;

	if (ifc_width == 8) {
		switch (pixel_fmt) {
		case IPU_PIX_FMT_BGR24:
			map = 0;
			break;
		case IPU_PIX_FMT_RGB666:
			map = 1;
			break;
		case IPU_PIX_FMT_RGB565:
			map = 2;
			break;
		default:
			break;
		}
	} else if (ifc_width >= 16) {
		switch (pixel_fmt) {
		case IPU_PIX_FMT_BGR24:
			map = 3;
			break;
		case IPU_PIX_FMT_RGB666:
			map = 4;
			break;
		case IPU_PIX_FMT_RGB565:
			map = 5;
			break;
        case IPU_PIX_FMT_GENERIC_16:
            map = 6;
            break;
		default:
			break;
		}
	}

	switch (disp) {
	case DISP0:
		if (ifc_width == 8) {
			__raw_writel(0x00070000, DI_DISP0_CB0_MAP);
			__raw_writel(0x0000FFFF, DI_DISP0_CB1_MAP);
			__raw_writel(0x0000FFFF, DI_DISP0_CB2_MAP);
		} else {
			__raw_writel(0x00070000, DI_DISP0_CB0_MAP);
			__raw_writel(0x000F0000, DI_DISP0_CB1_MAP);
			__raw_writel(0x0000FFFF, DI_DISP0_CB2_MAP);
		}
		__raw_writel(di_mappings[map].map_byte1, DI_DISP0_DB0_MAP);
		__raw_writel(di_mappings[map].map_byte2, DI_DISP0_DB1_MAP);
		__raw_writel(di_mappings[map].map_byte3, DI_DISP0_DB2_MAP);
		reg = __raw_readl(DI_DISP_ACC_CC);
		reg &= ~DISP0_IF_CLK_CNT_D_MASK;
		reg |= (di_mappings[map].cycle_cnt - 1) <<
		    DISP0_IF_CLK_CNT_D_OFFSET;
		__raw_writel(reg, DI_DISP_ACC_CC);
		break;
	case DISP1:
		if (ifc_width == 8) {
			__raw_writel(0x00070000, DI_DISP1_CB0_MAP);
			__raw_writel(0x0000FFFF, DI_DISP1_CB1_MAP);
			__raw_writel(0x0000FFFF, DI_DISP1_CB2_MAP);
		} else {
			__raw_writel(0x00070000, DI_DISP1_CB0_MAP);
			__raw_writel(0x000F0000, DI_DISP1_CB1_MAP);
			__raw_writel(0x0000FFFF, DI_DISP1_CB2_MAP);
		}
		__raw_writel(di_mappings[map].map_byte1, DI_DISP1_DB0_MAP);
		__raw_writel(di_mappings[map].map_byte2, DI_DISP1_DB1_MAP);
		__raw_writel(di_mappings[map].map_byte3, DI_DISP1_DB2_MAP);
		reg = __raw_readl(DI_DISP_ACC_CC);
		reg &= ~DISP1_IF_CLK_CNT_D_MASK;
		reg |= (di_mappings[map].cycle_cnt - 1) <<
		    DISP1_IF_CLK_CNT_D_OFFSET;
		__raw_writel(reg, DI_DISP_ACC_CC);
		break;
	case DISP2:
		if (ifc_width == 8) {
			__raw_writel(0x00070000, DI_DISP2_CB0_MAP);
			__raw_writel(0x0000FFFF, DI_DISP2_CB1_MAP);
			__raw_writel(0x0000FFFF, DI_DISP2_CB2_MAP);
		} else {
			__raw_writel(0x00070000, DI_DISP2_CB0_MAP);
			__raw_writel(0x000F0000, DI_DISP2_CB1_MAP);
			__raw_writel(0x0000FFFF, DI_DISP2_CB2_MAP);
		}
		__raw_writel(di_mappings[map].map_byte1, DI_DISP2_DB0_MAP);
		__raw_writel(di_mappings[map].map_byte2, DI_DISP2_DB1_MAP);
		__raw_writel(di_mappings[map].map_byte3, DI_DISP2_DB2_MAP);
		reg = __raw_readl(DI_DISP_ACC_CC);
		reg &= ~DISP2_IF_CLK_CNT_D_MASK;
		reg |= (di_mappings[map].cycle_cnt - 1) <<
		    DISP2_IF_CLK_CNT_D_OFFSET;
		__raw_writel(reg, DI_DISP_ACC_CC);
		break;
	default:
		break;
	}
}

EXPORT_SYMBOL(ipu_adc_write_template);
EXPORT_SYMBOL(ipu_adc_write_cmd);
EXPORT_SYMBOL(ipu_adc_set_update_mode);
EXPORT_SYMBOL(ipu_adc_init_panel);
EXPORT_SYMBOL(ipu_adc_init_ifc_timing);
EXPORT_SYMBOL(ipu_adc_set_ifc_width);
