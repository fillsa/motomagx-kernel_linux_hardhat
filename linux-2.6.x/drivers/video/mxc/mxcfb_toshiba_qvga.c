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
 * @file mxcfb_MXCFB_qvga.c
 *
 * @brief MXC Frame buffer driver for ADC
 *
 * @ingroup Framebuffer
 */

/*!
 * Include files
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/arch/gpio.h>
#include "../../mxc/ipu/ipu.h"

#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
#include "../../mxc/mc13783_legacy/module/mc13783_power.h"
#include "../../mxc/mc13783_legacy/module/mc13783_light.h"
#endif

#include "mxcfb.h"

//#define PARTIAL_REFRESH
#define MXCFB_REFRESH_DEFAULT MXCFB_REFRESH_AUTO
/*
 * Driver name
 */
#define MXCFB_NAME      "mxcfb_toshiba_qvga"

/*
 * Debug Macros
 */
//#define MXCFB_DEBUG
#ifdef MXCFB_DEBUG

#define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " fmt, \
                                __FILE__,__LINE__,__FUNCTION__ , ## args)
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)

#define FUNC_START	DPRINTK(" func start\n")
#define FUNC_END	DPRINTK(" func end\n")

#define FUNC_ERR        printk(KERN_ERR"%s :: %d :: %s  err= %d \n", __FILE__, \
                        __LINE__,__FUNCTION__ ,err)

#else				//MXCFB_DEBUG

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)

#define FUNC_START
#define FUNC_END

#endif				//MXCFB_DEBUG

#define MXCFB_SCREEN_TOP_OFFSET         0
#define MXCFB_SCREEN_LEFT_OFFSET        0
#define MXCFB_SCREEN_WIDTH              240
#define MXCFB_SCREEN_HEIGHT             320

struct mxcfb_data {
	struct fb_info *fbi;
	volatile int32_t vsync_flag;
	wait_queue_head_t vsync_wq;
	wait_queue_head_t suspend_wq;
	bool suspended;
	int backlight_level;
};

static struct mxcfb_data mxcfb_drv_data;

void slcd_gpio_config(void);
static int mxcfb_blank(int blank, struct fb_info *fbi);

/*!
 * Function to create and initiate template command buffer for ADC. This
 * template will be written to Panel memory.
 */
static void init_channel_template(int disp)
{
	/* template command buffer for ADC is 32 */
	uint32_t tempCmd[TEMPLATE_BUF_SIZE];
	uint32_t i = 0;

	memset(tempCmd, 0, sizeof(uint32_t) * TEMPLATE_BUF_SIZE);
	/* setup update display region */
	/* whole the screen during init */
	/*WRITE Y COORDINATE CMND */
	tempCmd[i++] = ipu_adc_template_gen(WR_CMND, 0, SINGLE_STEP, 0x07);
	/*WRITE Y START ADDRESS CMND [22:8] */
	tempCmd[i++] = ipu_adc_template_gen(WR_YADDR, 1, SINGLE_STEP, 0x01);
	/*WRITE X COORDINATE CMND */
	tempCmd[i++] = ipu_adc_template_gen(WR_CMND, 0, SINGLE_STEP, 0x06);
	/*WRITE X START ADDRESS CMND [7:0] */
	tempCmd[i++] = ipu_adc_template_gen(WR_XADDR, 1, SINGLE_STEP, 0x01);
	/*WRITE RAM CMND */
	tempCmd[i++] = ipu_adc_template_gen(WR_CMND, 0, SINGLE_STEP, 0x0E);
	/*WRITE DATA CMND and STP */
	tempCmd[i++] = ipu_adc_template_gen(WR_DATA, 1, STOP, 0);

	ipu_adc_write_template(disp, tempCmd, true);
}

/*!
 * Function to initialize the panel. First it resets the panel and then
 * initilizes panel.
 */
static void _init_panel(int disp)
{
	uint32_t cmd_param;
	uint32_t i;

	FUNC_START;

	// Reset
	cmd_param = 0x01;
	ipu_adc_write_cmd(disp, CMD, 0x03, &cmd_param, 1);

	// Turn on oscillator
	cmd_param = 0x01;
	ipu_adc_write_cmd(disp, CMD, 0x3A, &cmd_param, 1);

	cmd_param = 0x02;
	ipu_adc_write_cmd(disp, CMD, 0x32, &cmd_param, 1);

	cmd_param = 0x01;
	ipu_adc_write_cmd(disp, CMD, 0x33, &cmd_param, 1);

	cmd_param = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0x37, &cmd_param, 1);

	cmd_param = 0x0FFF;
	ipu_adc_write_cmd(disp, CMD, 0x77, &cmd_param, 1);

	cmd_param = 0x01;
	ipu_adc_write_cmd(disp, CMD, 0x72, &cmd_param, 1);

	cmd_param = 0x1C3B;
	ipu_adc_write_cmd(disp, CMD, 0x1C, &cmd_param, 1);

	cmd_param = 0x21;
	ipu_adc_write_cmd(disp, CMD, 0x52, &cmd_param, 1);

	cmd_param = 0x11;
	ipu_adc_write_cmd(disp, CMD, 0x53, &cmd_param, 1);

	cmd_param = 0x79;
	ipu_adc_write_cmd(disp, CMD, 0x24, &cmd_param, 1);

	cmd_param = 0x79;
	ipu_adc_write_cmd(disp, CMD, 0x25, &cmd_param, 1);

	cmd_param = 0x10;
	ipu_adc_write_cmd(disp, CMD, 0x26, &cmd_param, 1);

	cmd_param = 0x10;
	ipu_adc_write_cmd(disp, CMD, 0x27, &cmd_param, 1);

	cmd_param = 0x28;
	ipu_adc_write_cmd(disp, CMD, 0x61, &cmd_param, 1);

	cmd_param = 0x1A;
	ipu_adc_write_cmd(disp, CMD, 0x62, &cmd_param, 1);

	cmd_param = 0x1E;
	ipu_adc_write_cmd(disp, CMD, 0x63, &cmd_param, 1);

	cmd_param = 0x21;
	ipu_adc_write_cmd(disp, CMD, 0x64, &cmd_param, 1);

	cmd_param = 0x1B;
	ipu_adc_write_cmd(disp, CMD, 0x65, &cmd_param, 1);

	cmd_param = 0x29;
	ipu_adc_write_cmd(disp, CMD, 0x66, &cmd_param, 1);

	cmd_param = 0x205;
	ipu_adc_write_cmd(disp, CMD, 0x4D, &cmd_param, 1);

	cmd_param = 0x01;
	ipu_adc_write_cmd(disp, CMD, 0x4E, &cmd_param, 1);

	cmd_param = 0x104;
	ipu_adc_write_cmd(disp, CMD, 0x4F, &cmd_param, 1);

	cmd_param = 0x2F;
	ipu_adc_write_cmd(disp, CMD, 0x2E, &cmd_param, 1);

	cmd_param = 0x0;
	ipu_adc_write_cmd(disp, CMD, 0x29, &cmd_param, 1);

	cmd_param = 0x0;
	ipu_adc_write_cmd(disp, CMD, 0x2A, &cmd_param, 1);

	cmd_param = 0xEF;
	ipu_adc_write_cmd(disp, CMD, 0x2B, &cmd_param, 1);

	cmd_param = 0x13F;
	ipu_adc_write_cmd(disp, CMD, 0x2C, &cmd_param, 1);

	/* Window area setting */
	cmd_param = 0x0;
	ipu_adc_write_cmd(disp, CMD, 0x08, &cmd_param, 1);
	cmd_param = 0xEF;
	ipu_adc_write_cmd(disp, CMD, 0x09, &cmd_param, 1);
	cmd_param = 0x0;
	ipu_adc_write_cmd(disp, CMD, 0x0A, &cmd_param, 1);
	cmd_param = 0x13F;
	ipu_adc_write_cmd(disp, CMD, 0x0B, &cmd_param, 1);

	/* Window mode setting */
	cmd_param = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0x05, &cmd_param, 1);

	/* Ram address setting */
	cmd_param = 0x0;
	ipu_adc_write_cmd(disp, CMD, 0x06, &cmd_param, 1);
	cmd_param = 0x0;
	ipu_adc_write_cmd(disp, CMD, 0x07, &cmd_param, 1);

	/* Initialize RAM */
	ipu_adc_write_cmd(disp, CMD, 0x0E, 0, 0);
	for (i = 0; i < (MXCFB_SCREEN_WIDTH * MXCFB_SCREEN_HEIGHT); i++)
		ipu_adc_write_cmd(disp, DAT, 0x3FFFF, 0, 0);

	cmd_param = 0x1F6A;
	ipu_adc_write_cmd(disp, CMD, 0x18, &cmd_param, 1);

	cmd_param = 0x00A2;
	ipu_adc_write_cmd(disp, CMD, 0x1A, &cmd_param, 1);

	cmd_param = 0x0028;
	ipu_adc_write_cmd(disp, CMD, 0x1B, &cmd_param, 1);

	cmd_param = 0x1C3B;
	ipu_adc_write_cmd(disp, CMD, 0x1C, &cmd_param, 1);

	cmd_param = 0x0075;
	ipu_adc_write_cmd(disp, CMD, 0x1D, &cmd_param, 1);

	cmd_param = 0x003D;
	ipu_adc_write_cmd(disp, CMD, 0x1F, &cmd_param, 1);

	cmd_param = 0x0080;
	ipu_adc_write_cmd(disp, CMD, 0x20, &cmd_param, 1);

	/* DC/DC on */
	cmd_param = 0x1F6B;
	ipu_adc_write_cmd(disp, CMD, 0x18, &cmd_param, 1);
	msleep(100);

	/* VCOM on */
	cmd_param = 0x0021;
	ipu_adc_write_cmd(disp, CMD, 0x1E, &cmd_param, 1);

	/* GOE1,GOE2 setting (Gate output enable) */
	cmd_param = 0x0001;
	ipu_adc_write_cmd(disp, CMD, 0x3B, &cmd_param, 1);

	cmd_param = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0x2, &cmd_param, 1);
	/* Display on */
	cmd_param = 0x00;
	ipu_adc_write_cmd(disp, CMD, 0x0, &cmd_param, 1);

	msleep(10);

	DPRINTK("initialized panel\n");
}

#ifdef PARTIAL_REFRESH
static irqreturn_t mxcfb_sys2_eof_irq_handler(int irq, void *dev_id,
					      struct pt_regs *regs)
{
	ipu_channel_params_t params;
	struct fb_info *fbi = dev_id;
	struct mxcfb_info *mxc_fbi = fbi->par;
	uint32_t stat[2], seg_size;
	uint32_t lsb, msb;
	uint32_t update_height, start_line, start_addr, end_line, end_addr;
	uint32_t stride_pixels = (fbi->fix.line_length * 8) /
	    fbi->var.bits_per_pixel;

	ipu_adc_get_snooping_status(&stat[0], &stat[1]);
	//DPRINTK("snoop status = 0x%08X%08X\n", stat[1], stat[0]);

	if (!stat[0] && !stat[1]) {
		printk("mxcfb: error no bus snooping bits set\n");
		return IRQ_HANDLED;
	}
	ipu_disable_irq(IPU_IRQ_ADC_SYS2_EOF);

	lsb = ffs(stat[0]);
	if (lsb) {
		lsb--;
	} else {
		lsb = ffs(stat[1]);
		lsb += 32 - 1;
	}
	msb = fls(stat[1]);
	if (msb) {
		msb += 32;
	} else {
		msb = fls(stat[0]);
	}

	seg_size = mxc_fbi->snoop_window_size / 64;

	start_addr = lsb * seg_size;	// starting address offset
	start_line = start_addr / fbi->fix.line_length;
	start_addr = start_line * fbi->fix.line_length;	// Addr aligned to line
	start_addr += fbi->fix.smem_start;

	end_addr = msb * seg_size;	// ending address offset
	end_line = end_addr / fbi->fix.line_length;
	end_line++;

	if (end_line > fbi->var.yres) {
		end_line = fbi->var.yres;
	}

	update_height = end_line - start_line;
	//DPRINTK("updating rows %d to %d, start addr = 0x%08X\n",
	//        start_line, end_line, start_addr);

	ipu_uninit_channel(ADC_SYS1);
	params.adc_sys1.disp = mxc_fbi->disp_num;
	params.adc_sys1.ch_mode = WriteTemplateNonSeq;
	params.adc_sys1.out_left = MXCFB_SCREEN_LEFT_OFFSET;
	params.adc_sys1.out_top = start_line;
	ipu_init_channel(ADC_SYS1, &params);

	ipu_init_channel_buffer(ADC_SYS1, IPU_INPUT_BUFFER,
				mxc_fbi->cur_pixel_fmt, MXCFB_SCREEN_WIDTH,
				update_height,
				stride_pixels,
				IPU_ROTATE_NONE, (void *)start_addr, NULL);
	ipu_enable_channel(ADC_SYS1);
	ipu_select_buffer(ADC_SYS1, IPU_INPUT_BUFFER, 0);
	ipu_enable_irq(IPU_IRQ_ADC_SYS1_EOF);

	return IRQ_HANDLED;
}

static irqreturn_t mxcfb_sys1_eof_irq_handler(int irq, void *dev_id,
					      struct pt_regs *regs)
{
	ipu_disable_irq(IPU_IRQ_ADC_SYS1_EOF);
	ipu_disable_channel(ADC_SYS1, false);

	ipu_enable_channel(ADC_SYS2);
	ipu_enable_irq(IPU_IRQ_ADC_SYS2_EOF);

	return IRQ_HANDLED;
}
#endif

/*!
 * Function to initialize Asynchronous Display Controller. It also initilizes
 * the ADC System 1 channel. Configure ADC display 0 parallel interface for
 * the panel.
 *
 * @param       fbi     framebuffer information pointer
 */
static void mxcfb_init_panel(struct fb_info *fbi)
{
	int msb;
	int panel_stride;
	ipu_channel_params_t params;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;

#ifdef CONFIG_FB_MXC_ASYNC_PANEL_IFC_16_BIT
	uint32_t pix_fmt = IPU_PIX_FMT_RGB666;
	ipu_adc_sig_cfg_t sig = { 0, 0, 0, 0, 0, 0, 0, 0,
		IPU_ADC_BURST_WCS,
		IPU_ADC_IFC_MODE_SYS80_TYPE2,
		16, 0, 0, IPU_ADC_SER_NO_RW
	};
	mxc_fbi->disp_num = DISP0;
#elif defined(CONFIG_FB_MXC_ASYNC_PANEL_IFC_8_BIT)
	uint32_t pix_fmt = IPU_PIX_FMT_RGB666;
	ipu_adc_sig_cfg_t sig = { 0, 0, 0, 0, 0, 0, 0, 0,
		IPU_ADC_BURST_WCS,
		IPU_ADC_IFC_MODE_SYS80_TYPE2,
		8, 0, 0, IPU_ADC_SER_NO_RW
	};
	mxc_fbi->disp_num = DISP0;
#endif

	DPRINTK("Enabling LCD controller line_length=%d BPP=%d\n",
		fbi->fix.line_length, fbi->var.bits_per_pixel);

#ifdef PARTIAL_REFRESH
	if (ipu_request_irq(IPU_IRQ_ADC_SYS2_EOF, mxcfb_sys2_eof_irq_handler, 0,
			    MXCFB_NAME, fbi) != 0) {
		printk("mxcfb: Error registering irq handler.\n");
		return;
	}

	if (ipu_request_irq(IPU_IRQ_ADC_SYS1_EOF, mxcfb_sys1_eof_irq_handler, 0,
			    MXCFB_NAME, fbi) != 0) {
		printk("mxcfb: Error registering irq handler.\n");
		return;
	}
	ipu_disable_irq(IPU_IRQ_ADC_SYS1_EOF);
	ipu_disable_irq(IPU_IRQ_ADC_SYS2_EOF);
#endif
	// Init DI interface
	msb = fls(MXCFB_SCREEN_WIDTH);
	if (!(MXCFB_SCREEN_WIDTH & ((1UL << msb) - 1)))
		msb--;		// Already aligned to power 2
	panel_stride = 1UL << msb;
	ipu_adc_init_panel(mxc_fbi->disp_num,
			   MXCFB_SCREEN_WIDTH + MXCFB_SCREEN_LEFT_OFFSET,
			   MXCFB_SCREEN_HEIGHT,
			   pix_fmt, panel_stride, sig, XY, 0, VsyncInternal);

	ipu_adc_init_ifc_timing(mxc_fbi->disp_num, true,
				190, 17, 104, 190, 5000000);
	ipu_adc_init_ifc_timing(mxc_fbi->disp_num, false, 90, 10, 60, 0, 0);

	// Needed to turn on ADC clock for panel init
	memset(&params, 0, sizeof(params));
	params.adc_sys1.disp = mxc_fbi->disp_num;
	params.adc_sys1.ch_mode = WriteTemplateNonSeq;
	params.adc_sys1.out_left = MXCFB_SCREEN_LEFT_OFFSET;
	params.adc_sys1.out_top = MXCFB_SCREEN_TOP_OFFSET;
	ipu_init_channel(ADC_SYS1, &params);

	_init_panel(mxc_fbi->disp_num);
	init_channel_template(mxc_fbi->disp_num);

	FUNC_END;
}

int mxcfb_set_refresh_mode(struct fb_info *fbi, int mode,
			   struct mxcfb_rect *update_region)
{
	unsigned long start_addr;
	int ret_mode;
	uint32_t dummy;
	ipu_channel_params_t params;
	struct mxcfb_rect rect;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;
	uint32_t stride_pixels = (fbi->fix.line_length * 8) /
	    fbi->var.bits_per_pixel;
	uint32_t memsize = fbi->fix.smem_len;

	if (mxc_fbi->cur_update_mode == mode)
		return mode;

	ret_mode = mxc_fbi->cur_update_mode;

	ipu_disable_irq(IPU_IRQ_ADC_SYS1_EOF);
	ipu_adc_set_update_mode(ADC_SYS1, IPU_ADC_REFRESH_NONE, 0, 0, 0);
#ifdef PARTIAL_REFRESH
	ipu_disable_irq(IPU_IRQ_ADC_SYS2_EOF);
	ipu_adc_set_update_mode(ADC_SYS2, IPU_ADC_REFRESH_NONE, 0, 0, 0);
#endif

	ipu_disable_channel(ADC_SYS1, true);
	ipu_clear_irq(IPU_IRQ_ADC_SYS1_EOF);
#ifdef PARTIAL_REFRESH
	ipu_disable_channel(ADC_SYS2, true);
	ipu_clear_irq(IPU_IRQ_ADC_SYS2_EOF);
#endif
	ipu_adc_get_snooping_status(&dummy, &dummy);

	mxc_fbi->cur_update_mode = mode;

	switch (mode) {
	case MXCFB_REFRESH_OFF:
		if (ipu_adc_set_update_mode(ADC_SYS1, IPU_ADC_REFRESH_NONE,
					    0, 0, 0) < 0)
			printk("mxcfb: Error enabling auto refesh.\n");
//              if (ipu_adc_set_update_mode(ADC_SYS2, IPU_ADC_REFRESH_NONE,
//                                          0, 0, 0) < 0)
//                      printk("mxcfb: Error enabling auto refesh.\n");
#if 0
		ipu_init_channel_buffer(ADC_SYS2, IPU_INPUT_BUFFER,
					mxc_fbi->cur_pixel_fmt,
					1, 1, 4,
					IPU_ROTATE_NONE,
					(void *)fbi->fix.smem_start,
					(void *)fbi->fix.smem_start);
		ipu_enable_channel(ADC_SYS2);
		ipu_select_buffer(ADC_SYS2, IPU_INPUT_BUFFER, 0);
		ipu_select_buffer(ADC_SYS2, IPU_INPUT_BUFFER, 1);
		msleep(10);
#endif
#ifdef PARTIAL_REFRESH
		ipu_uninit_channel(ADC_SYS2);
#endif
		break;
	case MXCFB_REFRESH_PARTIAL:
#ifdef PARTIAL_REFRESH
		params.adc_sys2.disp = DISP0;
		params.adc_sys2.ch_mode = WriteTemplateNonSeq;
		params.adc_sys2.out_left = 0;
		params.adc_sys2.out_top = 0;
		ipu_init_channel(ADC_SYS2, &params);

		if (ipu_adc_set_update_mode(ADC_SYS1, IPU_ADC_REFRESH_NONE,
					    0, 0, 0) < 0) {
			printk("mxcfb: Error enabling auto refesh.\n");
		}
		if (ipu_adc_set_update_mode
		    (ADC_SYS2, IPU_ADC_AUTO_REFRESH_SNOOP, 30,
		     fbi->fix.smem_start, &memsize) < 0) {
			printk("mxcfb: Error enabling auto refesh.\n");
		}
		mxc_fbi->snoop_window_size = memsize;

		ipu_init_channel_buffer(ADC_SYS2, IPU_INPUT_BUFFER,
					mxc_fbi->cur_pixel_fmt,
					1, 1, 4,
					IPU_ROTATE_NONE,
					(void *)fbi->fix.smem_start, NULL);

		params.adc_sys1.disp = mxc_fbi->disp_num;
		params.adc_sys1.ch_mode = WriteTemplateNonSeq;
		params.adc_sys1.out_left = MXCFB_SCREEN_LEFT_OFFSET;
		params.adc_sys1.out_top = MXCFB_SCREEN_TOP_OFFSET;
		ipu_init_channel(ADC_SYS1, &params);

		ipu_init_channel_buffer(ADC_SYS1, IPU_INPUT_BUFFER,
					mxc_fbi->cur_pixel_fmt,
					MXCFB_SCREEN_WIDTH, MXCFB_SCREEN_HEIGHT,
					stride_pixels, IPU_ROTATE_NONE,
					(void *)fbi->fix.smem_start, NULL);
		ipu_enable_channel(ADC_SYS1);
		ipu_select_buffer(ADC_SYS1, IPU_INPUT_BUFFER, 0);
		ipu_enable_irq(IPU_IRQ_ADC_SYS1_EOF);
		break;
#endif
	case MXCFB_REFRESH_AUTO:
		if (update_region == NULL) {
			update_region = &rect;
			rect.top = 0;
			rect.left = 0;
			rect.height = MXCFB_SCREEN_HEIGHT;
			rect.width = MXCFB_SCREEN_WIDTH;
		}
		params.adc_sys1.disp = mxc_fbi->disp_num;
		params.adc_sys1.ch_mode = WriteTemplateNonSeq;
		params.adc_sys1.out_left = MXCFB_SCREEN_LEFT_OFFSET +
		    update_region->left;
		params.adc_sys1.out_top = MXCFB_SCREEN_TOP_OFFSET +
		    update_region->top;
		ipu_init_channel(ADC_SYS1, &params);

		// Address aligned to line
		start_addr = update_region->top * fbi->fix.line_length;
		start_addr += fbi->fix.smem_start;
		start_addr += update_region->left * fbi->var.bits_per_pixel / 8;

		ipu_init_channel_buffer(ADC_SYS1, IPU_INPUT_BUFFER,
					mxc_fbi->cur_pixel_fmt,
					update_region->width,
					update_region->height, stride_pixels,
					IPU_ROTATE_NONE, (void *)start_addr,
					NULL);
		ipu_enable_channel(ADC_SYS1);
		ipu_select_buffer(ADC_SYS1, IPU_INPUT_BUFFER, 0);

		if (ipu_adc_set_update_mode
		    (ADC_SYS1, IPU_ADC_AUTO_REFRESH_SNOOP, 30,
		     fbi->fix.smem_start, &memsize) < 0)
			printk("mxcfb: Error enabling auto refesh.\n");

		mxc_fbi->snoop_window_size = memsize;

		break;
	}
	return ret_mode;
}

/*
 * Open the main framebuffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int mxcfb_open(struct fb_info *fbi, int user)
{
	int retval = 0;
	struct mxcfb_info *mxc_fbi = fbi->par;
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
	struct t_sw3_conf sw3_conf;
#endif

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	if (mxc_fbi->open_count == 0) {
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
		sw3_conf.sw3_en = true;
		sw3_conf.sw3_op_mode = false;
		sw3_conf.sw3_ctr_stby = true;
		sw3_conf.sw3_setting = 0;
		mc13783_power_set_sw3(&sw3_conf);

		mc13783_light_set_bl_and_led_bais(true);
		mc13783_light_set_bl_master_egde_slowing(true);
		mc13783_light_set_bl_period(0);
#endif
	}
	mxc_fbi->open_count++;

	retval = mxcfb_blank(FB_BLANK_UNBLANK, fbi);
	FUNC_END;
	return retval;
}

/*
 * Close the main framebuffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @param       user    Set if opened by user or clear if opened by kernel
 */
static int mxcfb_release(struct fb_info *fbi, int user)
{
	int retval = 0;
	struct mxcfb_info *mxc_fbi = fbi->par;
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
	struct t_sw3_conf sw3_conf;
#endif

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	--mxc_fbi->open_count;
	if (mxc_fbi->open_count == 0) {
		DPRINTK("Disabling LCD controller\n");

		retval = mxcfb_blank(FB_BLANK_POWERDOWN, fbi);
/*
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
		sw3_conf.sw3_en = false;
		sw3_conf.sw3_op_mode = false;
		sw3_conf.sw3_ctr_stby = true;
		sw3_conf.sw3_setting = 0;
		mc13783_power_set_sw3(&sw3_conf);
#endif
*/
	}
	FUNC_END;
	return retval;
}

/*
 * Set fixed framebuffer parameters based on variable settings.
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_set_fix(struct fb_info *info)
{
	struct fb_fix_screeninfo *fix = &info->fix;
	struct fb_var_screeninfo *var = &info->var;
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)info->par;
	FUNC_START;

	// Set framebuffer id to IPU display number.
	strcpy(fix->id, "DISP0 FB");
	fix->id[4] = '0' + mxc_fbi->disp_num;

	// Init settings based on the panel size
	fix->line_length = MXCFB_SCREEN_WIDTH * var->bits_per_pixel / 8;

	fix->type = FB_TYPE_PACKED_PIXELS;
	fix->accel = FB_ACCEL_NONE;
	fix->visual = FB_VISUAL_TRUECOLOR;
	fix->xpanstep = 0;
	fix->ypanstep = 0;

	FUNC_END;
	return 0;
}

/*
 * Set framebuffer parameters and change the operating mode.
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_set_par(struct fb_info *fbi)
{
	int retval = 0;
	int mode;

	FUNC_START;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	mode = mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_OFF, NULL);

	mxcfb_set_fix(fbi);

	if (mode != MXCFB_REFRESH_OFF) {
#ifdef PARTIAL_REFRESH
		mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_PARTIAL, NULL);
#else
		mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_AUTO, NULL);
#endif
	}
	FUNC_END;
	return 0;
}

/*
 * Check framebuffer variable parameters and adjust to valid values.
 *
 * @param       var      framebuffer variable parameters
 *
 * @param       info     framebuffer information pointer
 */
static int mxcfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fbi)
{
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;
	FUNC_START;

	if (var->xres > MXCFB_SCREEN_WIDTH)
		var->xres = MXCFB_SCREEN_WIDTH;
	if (var->yres > MXCFB_SCREEN_HEIGHT)
		var->yres = MXCFB_SCREEN_HEIGHT;
	if (var->xres_virtual < var->xres)
		var->xres_virtual = var->xres;
	if (var->yres_virtual < var->yres)
		var->yres_virtual = var->yres;

	if ((var->bits_per_pixel != 32) && (var->bits_per_pixel != 24) &&
	    (var->bits_per_pixel != 16)) {
		var->bits_per_pixel = MXCFB_DEFUALT_BPP;
	}

	switch (var->bits_per_pixel) {
	case 16:
		mxc_fbi->cur_pixel_fmt = IPU_PIX_FMT_RGB565;
		var->red.length = 5;
		var->red.offset = 11;
		var->red.msb_right = 0;

		var->green.length = 6;
		var->green.offset = 5;
		var->green.msb_right = 0;

		var->blue.length = 5;
		var->blue.offset = 0;
		var->blue.msb_right = 0;

		var->transp.length = 0;
		var->transp.offset = 0;
		var->transp.msb_right = 0;
		break;
	case 24:
		mxc_fbi->cur_pixel_fmt = IPU_PIX_FMT_BGR24;
		var->red.length = 8;
		var->red.offset = 16;
		var->red.msb_right = 0;

		var->green.length = 8;
		var->green.offset = 8;
		var->green.msb_right = 0;

		var->blue.length = 8;
		var->blue.offset = 0;
		var->blue.msb_right = 0;

		var->transp.length = 0;
		var->transp.offset = 0;
		var->transp.msb_right = 0;
		break;
	case 32:
		mxc_fbi->cur_pixel_fmt = IPU_PIX_FMT_BGR32;
		var->red.length = 8;
		var->red.offset = 16;
		var->red.msb_right = 0;

		var->green.length = 8;
		var->green.offset = 8;
		var->green.msb_right = 0;

		var->blue.length = 8;
		var->blue.offset = 0;
		var->blue.msb_right = 0;

		var->transp.length = 8;
		var->transp.offset = 24;
		var->transp.msb_right = 0;
		break;
	}

	var->height = -1;
	var->width = -1;
	var->grayscale = 0;
	var->nonstd = 0;

	var->pixclock = -1;
	var->left_margin = -1;
	var->right_margin = -1;
	var->upper_margin = -1;
	var->lower_margin = -1;
	var->hsync_len = -1;
	var->vsync_len = -1;

	var->vmode = FB_VMODE_NONINTERLACED;
	var->sync = 0;

	FUNC_END;
	return 0;
}

static inline u_int _chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}

static int
mxcfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		u_int trans, struct fb_info *fbi)
{
	unsigned int val;
	int ret = 1;

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no matter what visual we are using.
	 */
	if (fbi->var.grayscale)
		red = green = blue = (19595 * red + 38470 * green +
				      7471 * blue) >> 16;
	switch (fbi->fix.visual) {
	case FB_VISUAL_TRUECOLOR:
		/*
		 * 16-bit True Colour.  We encode the RGB value
		 * according to the RGB bitfield information.
		 */
		if (regno < 16) {
			u32 *pal = fbi->pseudo_palette;

			val = _chan_to_field(red, &fbi->var.red);
			val |= _chan_to_field(green, &fbi->var.green);
			val |= _chan_to_field(blue, &fbi->var.blue);

			pal[regno] = val;
			ret = 0;
		}
		break;

	case FB_VISUAL_STATIC_PSEUDOCOLOR:
	case FB_VISUAL_PSEUDOCOLOR:
		break;
	}

	return ret;
}

/*
 * Function to handle custom ioctls for MXC framebuffer.
 *
 * @param       inode   inode struct
 *
 * @param       file    file struct
 *
 * @param       cmd     Ioctl command to handle
 *
 * @param       arg     User pointer to command arguments
 *
 * @param       fbi     framebuffer information pointer
 */
static int mxcfb_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg, struct fb_info *fbi)
{
	int retval = 0;

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	switch (cmd) {
	case MXCFB_SET_BRIGHTNESS:
		{
			uint8_t level;
			if (copy_from_user(&level, (void *)arg, sizeof(level))) {
				retval = -EFAULT;
				break;
			}
			mxcfb_drv_data.backlight_level = level;
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
			mc13783_light_bl_set_conf(BACKLIGHT_MAIN,
						  mxcfb_drv_data.
						  backlight_level, 15);
#endif
			DPRINTK("Set brightness to %d\n", level);
			break;
		}
	default:
		retval = -EINVAL;
	}
	return retval;
}

/*
 * mxcfb_blank():
 *      Blank the display.
 */
static int mxcfb_blank(int blank, struct fb_info *fbi)
{
	int retval = 0;
	struct mxcfb_info *mxc_fbi = fbi->par;
	FUNC_START;
	DPRINTK("blank = %d\n", blank);

	if ((retval = wait_event_interruptible(mxcfb_drv_data.suspend_wq,
					       (mxcfb_drv_data.suspended ==
						false))) < 0) {
		return retval;
	}

	switch (blank) {
	case FB_BLANK_POWERDOWN:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
		mxc_fbi->enabled = false;
		mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_OFF, NULL);
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
		mc13783_light_bl_set_conf(BACKLIGHT_MAIN, LEVEL_0, 0);
#endif
		break;
	case FB_BLANK_UNBLANK:
		mxc_fbi->enabled = true;
		mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_DEFAULT, NULL);
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
		mc13783_light_bl_set_conf(BACKLIGHT_MAIN, LEVEL_7, 15);
#endif
		break;
	}
	return 0;
}

/*!
 * This structure contains the pointers to the control functions that are
 * invoked by the core framebuffer driver to perform operations like
 * blitting, rectangle filling, copy regions and cursor definition.
 */
static struct fb_ops mxcfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = mxcfb_open,
	.fb_release = mxcfb_release,
	.fb_set_par = mxcfb_set_par,
	.fb_check_var = mxcfb_check_var,
	.fb_setcolreg = mxcfb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_blank = mxcfb_blank,
	.fb_ioctl = mxcfb_ioctl,
	.fb_cursor = soft_cursor,
};

/*!
 * Allocates the DRAM memory for the frame buffer.      This buffer is remapped
 * into a non-cached, non-buffered, memory region to allow palette and pixel
 * writes to occur without flushing the cache.  Once this area is remapped,
 * all virtual memory access to the video memory should occur at the new region.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @return      Error code indicating success or failure
 */
static int mxcfb_map_video_memory(struct fb_info *fbi)
{
	u32 msb;
	struct mxcfb_info *mxcfbi = fbi->par;

	fbi->fix.smem_len = fbi->var.xres_virtual * fbi->var.yres_virtual * 4;

	// Set size to power of 2.
	msb = fls(fbi->fix.smem_len);
	if (!(fbi->fix.smem_len & ((1UL << msb) - 1)))
		msb--;		// Already aligned to power 2
	if (msb < 11)
		msb = 11;
	mxcfbi->alloc_size = (1UL << msb) * 2;

	mxcfbi->alloc_start_paddr = ipu_malloc(mxcfbi->alloc_size);
	if (mxcfbi->alloc_start_paddr == 0) {
		printk("MXCFB - Unable to allocate framebuffer memory\n");
		return -EBUSY;
	}
	DPRINTK("allocated fb memory @ paddr=0x%08X, size=%d.\n",
		(uint32_t) mxcfbi->alloc_start_paddr, mxcfbi->alloc_size);

	fbi->fix.smem_start = (mxcfbi->alloc_start_paddr +
			       (mxcfbi->alloc_size / 2) -
			       1) & ~((mxcfbi->alloc_size / 2) - 1);
	DPRINTK("aligned fb start @ paddr=0x%08lX, size=%u.\n",
		fbi->fix.smem_start, fbi->fix.smem_len);

	if (!request_mem_region(fbi->fix.smem_start, fbi->fix.smem_len, "LCD")) {
		return -EBUSY;
	}

	if (!(fbi->screen_base =
	      ioremap(fbi->fix.smem_start, fbi->fix.smem_len))) {
		release_mem_region(fbi->fix.smem_start, fbi->fix.smem_len);
		return -EIO;
	}

	/* Clear the screen */
	memset(fbi->screen_base, 0, fbi->fix.smem_len);
	return 0;
}

/*!
 * De-allocates the DRAM memory for the frame buffer.
 *
 * @param       fbi     framebuffer information pointer
 *
 * @return      Error code indicating success or failure
 */
static int mxcfb_unmap_video_memory(struct fb_info *fbi)
{
	struct mxcfb_info *mxc_fbi = (struct mxcfb_info *)fbi->par;

	iounmap(fbi->screen_base);
	release_mem_region(fbi->fix.smem_start, fbi->fix.smem_len);
	ipu_free(mxc_fbi->alloc_start_paddr);
	return 0;
}

/*!
 * Initializes the framebuffer information pointer. After allocating
 * sufficient memory for the framebuffer structure, the fields are
 * filled with custom information passed in from the configurable
 * structures.  This includes information such as bits per pixel,
 * color maps, screen width/height and RGBA offsets.
 *
 * @return      Framebuffer structure initialized with our information
 */
static struct fb_info *mxcfb_init_fbinfo(struct device *dev, struct fb_ops *ops)
{
	struct fb_info *fbi;
	struct mxcfb_info *mxcfbi;

	/*
	 * Allocate sufficient memory for the fb structure
	 */
	fbi = framebuffer_alloc(sizeof(struct mxcfb_info), dev);
	if (!fbi)
		return NULL;

	mxcfbi = (struct mxcfb_info *)fbi->par;

	/*
	 * Fill in fb_info structure information
	 */
	fbi->var.xres = fbi->var.xres_virtual = MXCFB_SCREEN_WIDTH;
	fbi->var.yres = fbi->var.yres_virtual = MXCFB_SCREEN_HEIGHT;
	fbi->var.activate = FB_ACTIVATE_NOW;
	mxcfb_check_var(&fbi->var, fbi);

	mxcfb_set_fix(fbi);

	fbi->fbops = ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->pseudo_palette = mxcfbi->pseudo_palette;

	spin_lock_init(&mxcfbi->fb_lock);

	/*
	 * Allocate colormap
	 */
	fb_alloc_cmap(&fbi->cmap, 16, 0);

	return fbi;
}

/*!
 * Probe routine for the framebuffer driver. It is called during the
 * driver binding process.      The following functions are performed in
 * this routine: Framebuffer initialization, Memory allocation and
 * mapping, Framebuffer registration, IPU initialization.
 *
 * @return      Appropriate error code to the kernel common code
 */
static int mxcfb_probe(struct device *dev)
{
	struct fb_info *fbi;
	struct mxcfb_info *mxc_fbi;
	int ret;

	FUNC_START;

	dev_set_drvdata(dev, &mxcfb_drv_data);

	/*
	 * Initialize FB structures
	 */
	fbi = mxcfb_init_fbinfo(dev, &mxcfb_ops);
	if (!fbi) {
		ret = -ENOMEM;
		goto err0;
	}
	mxcfb_drv_data.fbi = fbi;
	mxc_fbi = fbi->par;

#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
	mxcfb_drv_data.backlight_level = LEVEL_7;
#endif
	mxcfb_drv_data.suspended = false;
	init_waitqueue_head(&mxcfb_drv_data.suspend_wq);
	mxc_fbi->enabled = false;

	/*
	 * Allocate memory
	 */
	ret = mxcfb_map_video_memory(fbi);
	if (ret < 0) {
		goto err1;
	}

	mxcfb_init_panel(fbi);

	/*
	 * Register framebuffer
	 */
	ret = register_framebuffer(fbi);
	if (ret < 0) {
		goto err2;
	}

	printk("%s registered\n", MXCFB_NAME);

	FUNC_END;
	return 0;

      err2:
	mxcfb_unmap_video_memory(fbi);
      err1:
	if (&fbi->cmap)
		fb_dealloc_cmap(&fbi->cmap);
	framebuffer_release(fbi);
      err0:
	return ret;
}

#ifdef CONFIG_PM
/*!
 * Power management hooks.      Note that we won't be called from IRQ context,
 * unlike the blank functions above, so we may sleep.
 */

/*!
 * Suspends the framebuffer and blanks the screen. Power management support
 *
 * @param	dev	pointer to device structure.
 * @param	state	state of the device.
 * @param	level	level of suspend.
 *
 * @return	success
 */
static int mxcfb_suspend(struct device *dev, u32 state, u32 level)
{
	uint32_t cmd_param;
	struct mxcfb_data *drv_data = dev_get_drvdata(dev);
	struct fb_info *fbi = drv_data->fbi;
	struct mxcfb_info *mxc_fbi = fbi->par;

	FUNC_START;
	DPRINTK("level = %d\n", level);

	switch (level) {
	case SUSPEND_DISABLE:
		drv_data->suspended = true;

		if (mxc_fbi->enabled)
			mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_OFF, NULL);
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
		mc13783_light_bl_set_conf(BACKLIGHT_MAIN, LEVEL_0, 0);
#endif
		break;
	case SUSPEND_POWER_DOWN:
		// Display OFF
		cmd_param = 0x08;
		ipu_adc_write_cmd(mxc_fbi->disp_num, CMD, 0x0, &cmd_param, 1);
		break;
	}
	FUNC_END;
	return 0;
}

/*!
 * Resumes the framebuffer and unblanks the screen. Power management support
 *
 * @param       dev     pointer to device structure.
 * @param       level   level of suspend.
 *
 * @return      success
 */
static int mxcfb_resume(struct device *dev, u32 level)
{
	uint32_t cmd_param;
	struct mxcfb_data *drv_data = dev_get_drvdata(dev);
	struct fb_info *fbi = drv_data->fbi;
	struct mxcfb_info *mxc_fbi = fbi->par;

	FUNC_START;
	DPRINTK("level = %d\n", level);

	switch (level) {
	case RESUME_POWER_ON:
		// Display ON
		cmd_param = 0x00;
		ipu_adc_write_cmd(mxc_fbi->disp_num, CMD, 0x0, &cmd_param, 1);
		break;
	case RESUME_ENABLE:
		drv_data->suspended = false;

		if (mxc_fbi->enabled)
			mxcfb_set_refresh_mode(fbi, MXCFB_REFRESH_DEFAULT,
					       NULL);
#if defined(CONFIG_MXC_MC13783_LIGHT) && defined(CONFIG_MXC_MC13783_POWER)
		mc13783_light_bl_set_conf(BACKLIGHT_MAIN,
					  drv_data->backlight_level, 15);
#endif
		wake_up_interruptible(&drv_data->suspend_wq);
	}
	FUNC_END;
	return 0;
}
#else
#define mxcfb_suspend   NULL
#define mxcfb_resume    NULL
#endif

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxcfb_driver = {
	.name = MXCFB_NAME,
	.bus = &platform_bus_type,
	.probe = mxcfb_probe,
	.suspend = mxcfb_suspend,
	.resume = mxcfb_resume,
};

/*!
 * Device definition for the Framebuffer
 */
static struct platform_device mxcfb_device = {
	.name = MXCFB_NAME,
	.id = 0,
};

/*!
 * Main entry function for the framebuffer. The function registers the power
 * management callback functions with the kernel and also registers the MXCFB
 * callback functions with the core Linux framebuffer driver \b fbmem.c
 *
 * @return      Error code indicating success or failure
 */
static int mxcfb_init(void)
{
	int ret = 0;

	FUNC_START;

	ret = driver_register(&mxcfb_driver);
	if (ret == 0) {
		ret = platform_device_register(&mxcfb_device);
		if (ret != 0) {
			driver_unregister(&mxcfb_driver);
		}
	}

	FUNC_END;
	return ret;
}

static void mxcfb_exit(void)
{
	struct fb_info *fbi = dev_get_drvdata(&mxcfb_device.dev);

	if (fbi) {
		mxcfb_unmap_video_memory(fbi);

		if (&fbi->cmap)
			fb_dealloc_cmap(&fbi->cmap);

		unregister_framebuffer(fbi);
		framebuffer_release(fbi);
	}

	platform_device_unregister(&mxcfb_device);
	driver_unregister(&mxcfb_driver);
}

module_init(mxcfb_init);
module_exit(mxcfb_exit);

EXPORT_SYMBOL(mxcfb_set_refresh_mode);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC Toshiba QVGA framebuffer driver");
MODULE_SUPPORTED_DEVICE("fb");
