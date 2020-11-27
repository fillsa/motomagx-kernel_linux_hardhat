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

#define FUNC_START
#define FUNC_END

void mxcfb_set_refresh(struct fb_info *fbi, int mode);

/*****************************************************************************/
void mxcfb_fill_memory(struct fb_info *fbi, int color)
{
/*	int i;
	switch (color)
	{
	case 0: // black
		{
			memset((char *)fbi->screen_base, 0, fbi->fix.smem_len);
			break;
		}
	case 1: // white
		{
			memset((char *)fbi->screen_base, 0xFF, fbi->fix.smem_len);
			break;
		}
	case 2: // red
		{
			for (i = 0; i < fbi->fix.smem_len; i += 3)
			{
				memset((char *)fbi->screen_base + i, 0xFF, 1);
				memset((char *)fbi->screen_base + i + 1, 0x00, 1);
				memset((char *)fbi->screen_base + i + 2, 0x00, 1);
			}
			break;
		}
	case 3: // green
		{
			for (i = 0; i < fbi->fix.smem_len; i += 3)
			{
				memset((char *)fbi->screen_base + i, 0x00, 1);
				memset((char *)fbi->screen_base + i + 1, 0xFF, 1);
				memset((char *)fbi->screen_base + i + 2, 0x00, 1);
			}
			break;
		}
	case 4: // blue
		{
			for (i = 0; i < fbi->fix.smem_len; i += 3)
			{
				memset((char *)fbi->screen_base + i, 0x00, 1);
				memset((char *)fbi->screen_base + i + 1, 0x00, 1);
				memset((char *)fbi->screen_base + i + 2, 0xFF, 1);
			}
			break;
		}
	}*/
}

/*****************************************************************************/
void mxcfb_fill_panel(int color)
{
/*	int i;
	ipu_adc_write_cmd(DISP0, CMD, RAMWR, 0, 0);	// Begin writing display data
	switch (color)
	{
	case 0:
		{
			for (i = 0; i < 691200; i++)
				ipu_adc_write_cmd(DISP0, DAT, 0x00, 0, 0);
			break;
		}
	case 1:
		{
			for (i = 0; i < 691200; i++)
				ipu_adc_write_cmd(DISP0, DAT, 0xFF, 0, 0);
			break;
		}
	case 2:
		{
			for (i = 0; i < 691200/6; i++)
			{
				ipu_adc_write_cmd(DISP0, DAT, 0xFF, 0, 0);
				ipu_adc_write_cmd(DISP0, DAT, 0x00, 0, 0);
				ipu_adc_write_cmd(DISP0, DAT, 0x00, 0, 0);
			}
			break;
		}
	case 3:
		{
			for (i = 0; i < 691200/6; i++)
			{
				ipu_adc_write_cmd(DISP0, DAT, 0x00, 0, 0);
				ipu_adc_write_cmd(DISP0, DAT, 0xFF, 0, 0);
				ipu_adc_write_cmd(DISP0, DAT, 0x00, 0, 0);
			}
			break;
		}
	case 4:
		{
			for (i = 0; i < 691200/6; i++)
			{
				ipu_adc_write_cmd(DISP0, DAT, 0x00, 0, 0);
				ipu_adc_write_cmd(DISP0, DAT, 0x00, 0, 0);
				ipu_adc_write_cmd(DISP0, DAT, 0xFF, 0, 0);
			}
			break;
		}
	}*/
}

/*****************************************************************************/
void mxcfb_init_panel(int disp)
{
/*	uint32_t param[16];
//	int i;

	mdelay(120); // Wait 5 ms
        ipu_adc_write_cmd(disp, CMD, SLPOUT, 0, 0);
	mdelay(5); // Wait 5 ms

	param[0] = 0x00;
	param[1] = 0x00;
	param[2] = 0x00;
	param[3] = 0x7F;
	ipu_adc_write_cmd(disp, CMD, PTLAR, param, 4);

	param[0] = 0x00;
	ipu_adc_write_cmd(disp, CMD, TEON, param, 1);

	param[0] = 0x40;
	ipu_adc_write_cmd(disp, CMD, MADCTL, param, 1);

	param[0] = 0x00; // spec says 03, 00 is 8-bit interface
	ipu_adc_write_cmd(disp, CMD, 0xC2, param, 1);

	param[0] = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0xC3, param, 1);

	param[0] = 0x10;
	param[1] = 0x44;
	param[2] = 0x04;
	param[3] = 0x02;
	ipu_adc_write_cmd(disp, CMD, 0xC5, param, 4);

	param[0] = 0x10;
	param[1] = 0x44;
	param[2] = 0x04;
	param[3] = 0x02;
	ipu_adc_write_cmd(disp, CMD, 0xC6, param, 4);

	param[0] = 0x00;
	param[1] = 0x44;
	param[2] = 0x04;
	param[3] = 0x02;
	ipu_adc_write_cmd(disp, CMD, 0xC7, param, 4);

	param[0] = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0xCA, param, 1);

	param[0] = 0x10;
	ipu_adc_write_cmd(disp, CMD, 0xCB, param, 1);

	param[0] = 0x01;
	ipu_adc_write_cmd(disp, CMD, 0xCF, param, 1);

	param[0] = 0x23;
	param[1] = 0x32;
	param[2] = 0x0D;
	ipu_adc_write_cmd(disp, CMD, 0xD0, param, 3);

	param[0] = 0x23;
	param[1] = 0x32;
	param[2] = 0x0D;
	ipu_adc_write_cmd(disp, CMD, 0xD1, param, 3);

	param[0] = 0x23;
	param[1] = 0x32;
	param[2] = 0x0D;
	ipu_adc_write_cmd(disp, CMD, 0xD2, param, 3);

	param[0] = 0x27;
	param[1] = 0x6E;
	param[2] = 0x3B;
	param[3] = 0x39;
	ipu_adc_write_cmd(disp, CMD, 0xD4, param, 4);

	param[0] = 0x37;
	param[1] = 0x02;
	param[2] = 0x58;
	param[3] = 0x05;
	ipu_adc_write_cmd(disp, CMD, 0xD5, param, 4);

	param[0] = 0x80;
	param[1] = 0x10;
	param[2] = 0x09;
	param[3] = 0x37;
	ipu_adc_write_cmd(disp, CMD, 0xD7, param, 4);

	param[0] = 0x55;
	param[1] = 0x66;
	ipu_adc_write_cmd(disp, CMD, 0xD8, param, 2);

	param[0] = 0x77;
	param[1] = 0x34;
	param[2] = 0x9D;
	param[3] = 0x32;
	param[4] = 0xC0;
	param[5] = 0x34;
	param[6] = 0x17;
	param[7] = 0x00;
	param[8] = 0x77;
	param[9] = 0x34;
	param[10] = 0x9D;
	param[11] = 0x32;
	param[12] = 0xC0;
	param[13] = 0x34;
	param[14] = 0x1F;
	param[15] = 0x08;
	ipu_adc_write_cmd(disp, CMD, 0xDD, param, 16);

	param[0] = 0x77;
	param[1] = 0x34;
	param[2] = 0x9D;
	param[3] = 0x32;
	param[4] = 0xC0;
	param[5] = 0x34;
	param[6] = 0x17;
	param[7] = 0x00;
	param[8] = 0x77;
	param[9] = 0x34;
	param[10] = 0x9D;
	param[11] = 0x32;
	param[12] = 0xC0;
	param[13] = 0x34;
	param[14] = 0x1F;
	param[15] = 0x08;
	ipu_adc_write_cmd(disp, CMD, 0xDE, param, 16);

	param[0] = 0x77;
	param[1] = 0x34;
	param[2] = 0x9D;
	param[3] = 0x32;
	param[4] = 0xC0;
	param[5] = 0x34;
	param[6] = 0x17;
	param[7] = 0x00;
	param[8] = 0x77;
	param[9] = 0x34;
	param[10] = 0x9D;
	param[11] = 0x32;
	param[12] = 0xC0;
	param[13] = 0x34;
	param[14] = 0x1F;
	param[15] = 0x08;
	ipu_adc_write_cmd(disp, CMD, 0xDF, param, 16);

	param[0] = 0x13;
	param[1] = 0x0E;
	param[2] = 0x05;
	ipu_adc_write_cmd(disp, CMD, 0xE3, param, 3);

	param[0] = 0x0E;
	param[1] = 0x77;
	ipu_adc_write_cmd(disp, CMD, 0xE4, param, 2);

	param[0] = 0x03;
	param[1] = 0x0B;
	ipu_adc_write_cmd(disp, CMD, 0xE5, param, 2);

	param[0] = 0x03;
	ipu_adc_write_cmd(disp, CMD, 0xE6, param, 1);

	param[0] = 0x03;
	param[1] = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0xE8, param, 2);

	param[0] = 0x00;
	param[1] = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0xEC, param, 2);

	ipu_adc_write_cmd(disp, CMD, 0xBF, 0, 0);

	mdelay(80);

// Temp loop for display
//	ipu_adc_write_cmd(disp, CMD, RAMWR, 0, 0);	// Begin writing display data	greg
//	for (i = 0; i < 345600; i++)
//		ipu_adc_write_cmd(disp, DAT, 0xFFFF, 0, 0);

	ipu_adc_write_cmd(disp, CMD, DISPON, 0, 0);

	__raw_writel(0x1C00AAAA, DI_DISP0_DB0_MAP);*/
}

/*****************************************************************************/
void mxcfb_init_ipu()
{
/*	int msb;
	int panel_stride;
	ipu_channel_params_t params;

	ipu_adc_sig_cfg_t sig = {   0, // data_pol	straight
							    0, // clk_pol	straight
							    0, // cs_pol	active low
							    0, // addr_pol	straight
							    1, // E(RD)_pol active high
							    0, // write_pol active low
							    0, // Vsync_pol active low
				 			    0, // burst_pol
	   	       IPU_ADC_BURST_NONE, // burst_mode
    IPU_ADC_IFC_MODE_SYS68K_TYPE2, // ifc_mode
							    8, // ifc_width
							    0, // ser_preamble_len
							    0, // ser_preamble
               IPU_ADC_SER_NO_RW}; // ser_rw_mode
	

	// Init DI interface
	msb = fls(GD2_SCREEN_WIDTH);
	if (!(GD2_SCREEN_WIDTH & ((1UL << msb) - 1)))
		msb--;		// Already aligned to power 2
	panel_stride = 1UL << msb;

	ipu_adc_init_panel(		DISP0, // display_port_t
				 GD2_SCREEN_WIDTH, // width
				GD2_SCREEN_HEIGHT, // height
				IPU_PIX_FMT_RGB24, // pixel_format
					 panel_stride, // stride
							  sig, // ipu_adc_sig_cfg_t
							   XY, // display_addressing_t
								0, // vsync_width
				   VsyncExternal); // vsync_t
	
	ipu_adc_init_ifc_timing(DISP0, // display_port_t
							 true, // read
							  540, // cycle_time
						   	  360, // up_time
							  120, // down_time
							  420, // read_latch_time
							   0); // pixel_clk

	ipu_adc_init_ifc_timing(DISP0, // display_port_t
							false, // read
							   92, // cycle_time
							   36, // up_time
							   36, // down_time
							    0, // read_latch_time
							   0); // pixel_clk

	memset(&params, 0, sizeof(params));
	params.adc_sys1.disp = DISP0;
	params.adc_sys1.ch_mode = WriteDataWoRS;
	params.adc_sys1.out_left = GD2_SCREEN_LEFT_OFFSET;
	params.adc_sys1.out_top = GD2_SCREEN_TOP_OFFSET;
	ipu_init_channel(ADC_SYS1, &params);	

	__raw_writel(0x001900C8, DI_DISP0_TIME_CONF_1);
*/
}

/*****************************************************************************/
void mxcfb_set_refresh(struct fb_info *fbi, int mode)
{
	ipu_channel_params_t params;

	unsigned long start_addr =  fbi->fix.smem_start;
	uint32_t memsize = fbi->fix.smem_len;

	uint32_t stride_pixels = GD2_SCREEN_WIDTH;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;

	if (mxc_fbi->cur_update_mode == mode)
		return;

	mxc_fbi->cur_update_mode = mode;
/*
	ipu_adc_set_update_mode(ADC_SYS1, IPU_ADC_REFRESH_NONE, 0, 0, 0);
	ipu_disable_channel(ADC_SYS1, true);
	ipu_adc_write_cmd(DISP0, CMD, NOP, 0, 0);*/

	switch (mode) {
	case MXCFB_REFRESH_OFF:
		break;
	case MXCFB_REFRESH_PARTIAL:
		break;
	case MXCFB_REFRESH_AUTO:
/*		params.adc_sys1.disp = DISP0;
		params.adc_sys1.ch_mode = WriteDataWoRS;
		params.adc_sys1.out_left = GD2_SCREEN_LEFT_OFFSET;
		params.adc_sys1.out_top = GD2_SCREEN_TOP_OFFSET;
		ipu_init_channel(ADC_SYS1, &params);

		ipu_init_channel_buffer(ADC_SYS1, // ipu_channel_t
						IPU_INPUT_BUFFER, // ipu_buffer_t
					   IPU_PIX_FMT_BGR24, // pixel_fmt 
						GD2_SCREEN_WIDTH, // width
					   GD2_SCREEN_HEIGHT, // height
						   stride_pixels, // stride
									   0, // ipu_rotate_mode_t
					  (void *)start_addr, // *phyaddr_0
								   NULL); // *phyaddr_1

		__raw_writel(0x00070000, DI_DISP0_DB2_MAP);
		__raw_writel(0x00E05555, DI_DISP0_DB1_MAP);
		__raw_writel(0x1C00AAAA, DI_DISP0_DB0_MAP);
		ipu_adc_write_cmd(DISP0, CMD, RAMWR, 0, 0);

		ipu_enable_channel(ADC_SYS1);
		ipu_select_buffer(ADC_SYS1, IPU_INPUT_BUFFER, 0);
		ipu_adc_set_update_mode(ADC_SYS1, 
			        IPU_ADC_SNOOPING,
									  30, // refresh rate
							  start_addr, 
							   &memsize);*/
		break;
	}
	return;
}

