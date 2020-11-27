/*
 * drivers/video/pnx4008/sdum.c
 *
 * Display Update Master support
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
#include <linux/dma-mapping.h>
#include <asm/hardware/clock.h>
#include <asm/arch/pm.h>

#include "sdum.h"
#include "fbcommon.h"
#include "dum.h"

#define ASSERT(x) do { \
	if(!(x)) { \
		printk("ASSERTION FAILED at %s:%d\n", __FILE__, __LINE__); \
	} \
} while(0)

static struct pnx4008_fb_addr {
	int fb_type;
	long addr_offset;
	long fb_length;
} fb_addr[] = {
	[0] = {
	FB_TYPE_YUV, 0, 0xB0000},[1] = {
FB_TYPE_RGB, 0xB0000, 0x50000},};

struct dumchanregs {
	u32 dum_ch_min;
	u32 dum_ch_max;
	u32 dum_ch_conf;
	u32 dum_ch_stat;
	u32 dum_ch_ctrl;

} __attribute__ ((aligned(0x100)));

struct dumglobregs {
	u32 dum_conf;		/* 0x400BC000 */
	u32 dum_ctrl;
	u32 dum_stat;
	u32 dum_decode;
	u32 dum_com_base;	/* 0x400BC010 */
	u32 dum_sync_c;
	u32 dum_clk_div;
	u32 dum_hf_count;
	u32 dummy1;		/* 0x400BC020 */
	u32 dummy2;
	u32 dum_format;
	u32 dum_cs_ctrl;
	u32 dum_wtcfg1;		/* 0x400BC030 */
	u32 dum_rtcfg1;
	u32 dum_wtcfg2;
	u32 dum_rtcfg2;
	u32 dum_tcfg;		/* 0x400BC040 */
	u32 dum_outp_format1;
	u32 dum_outp_format2;
	u32 dum_sync_mode;
	u32 dum_sync_out_c;	/* 0x400BC050 */
};

static u32 lcd_phys_video_start = 0;
static u32 lcd_video_start = 0;

int pnx4008_get_fb_addresses(int fb_type, void **virt_addr,
			     dma_addr_t * phys_addr, int *fb_length)
{
	int i;
	int ret = -1;
	for (i = 0; i < ARRAY_SIZE(fb_addr); i++)
		if (fb_addr[i].fb_type == fb_type) {
			*virt_addr =
			    (void *)(lcd_video_start + fb_addr[i].addr_offset);
			*phys_addr =
			    lcd_phys_video_start + fb_addr[i].addr_offset;
			*fb_length = fb_addr[i].fb_length;
			ret = 0;
			break;
		}

	return ret;
}

EXPORT_SYMBOL(pnx4008_get_fb_addresses);

static u32 dum_ch_setup(int ch_no, dum_ch_setup_t * ch_setup);

static u32 display_open(int ch_no, int auto_update, u32 * dirty_buffer,
			u32 * frame_buffer, u32 xpos, u32 ypos, u32 w, u32 h);

static int put_channel(struct dumchannel chan);
static int get_channel(struct dumchannel *p_chan);

/* Debug/assertion functions */
static u32 nof_pixels_dx(dum_ch_setup_t * ch_setup);
static u32 nof_pixels_dy(dum_ch_setup_t * ch_setup);
static u32 nof_pixels_dxy(dum_ch_setup_t * ch_setup);

static u32 nof_pixels_mx(dum_ch_setup_t * ch_setup);
static u32 nof_pixels_my(dum_ch_setup_t * ch_setup);
static u32 nof_pixels_mxy(dum_ch_setup_t * ch_setup);
/* end of Debug/assertion funstions */

static struct dumglobregs *dum_glob_reg;
static struct dumchanregs *dum_chan_reg;
static volatile u32 *dum_slave_reg;

static int fb_owning_channel[MAX_DUM_CHANNELS];

static void clear_channel(int channr);

static struct dumchannel_uf chan_uf_store[MAX_DUM_CHANNELS];

static int put_cmd_string(struct cmdstring cmds);

static int __init sdum_probe(struct device *device);
static int sdum_remove(struct device *device);
static int __init sdum_init(void);
static void __exit sdum_exit(void);
static int sdum_suspend(struct device *dev, u32 state, u32 level);
static int sdum_resume(struct device *dev, u32 level);
static void lcd_init(void);
static void lcd_reset(void);

static struct device_driver sdum_driver = {
	.name = "sdum",
	.bus = &platform_bus_type,
	.probe = sdum_probe,
	.remove = sdum_remove,
	.suspend = sdum_suspend,
	.resume = sdum_resume
};

static int sdum_suspend(struct device *dev, u32 state, u32 level)
{
	int retval = 0;
	struct clk *clk;

	switch (level) {
	case SUSPEND_DISABLE:
	case SUSPEND_SAVE_STATE:
		break;

	case SUSPEND_POWER_DOWN:
		clk = clk_get(0, "dum_ck");
		if (!IS_ERR(clk)) {
			clk_set_rate(clk, 0);
			clk_put(clk);
		} else
			retval = PTR_ERR(clk);

		/* disable BAC */
		dum_glob_reg->dum_ctrl = V_BAC_DISABLE_IDLE;

		/* LCD standby & turn off display */
		lcd_reset();

		break;

	default:
		break;
	}

	return retval;
}

static int sdum_resume(struct device *dev, u32 level)
{
	int retval = 0;
	struct clk *clk;

	switch (level) {
	case RESUME_POWER_ON:
	case RESUME_RESTORE_STATE:
		break;

	case RESUME_ENABLE:
		clk = clk_get(0, "dum_ck");
		if (!IS_ERR(clk)) {
			clk_set_rate(clk, 1);
			clk_put(clk);
		} else
			retval = PTR_ERR(clk);

		/* wait for BAC disable */
		dum_glob_reg->dum_ctrl = V_BAC_DISABLE_TRIG;
		while (dum_glob_reg->dum_stat & BAC_ENABLED)
			udelay(10);

		/* re-init LCD */
		lcd_init();

		/* enable BAC and reset MUX */
		dum_glob_reg->dum_ctrl = V_BAC_ENABLE;
		udelay(1);
		dum_glob_reg->dum_ctrl = V_MUX_RESET;

		break;

	default:
		break;
	}

	return retval;
}

static void lcd_reset(void)
{
	volatile u32 *dum_pio_base =
	    (volatile u32 *)IO_ADDRESS(PNX4008_PIO_BASE);

	mdelay(1);		/* wait 1 ms */
	dum_pio_base[2] = BIT(19);

	mdelay(1);		/* wait 1 ms */
	dum_pio_base[1] = BIT(19);

	mdelay(1);		/* wait 1 ms */
}

static int dum_init(void)
{
	struct clk *clk;

	/* enable DUM clock */
	clk = clk_get(0, "dum_ck");
	if (IS_ERR(clk)) {
		printk(KERN_ERR "pnx4008_dum: Unable to access DUM clock\n");
		return PTR_ERR(clk);
	}

	clk_set_rate(clk, 1);
	clk_put(clk);

	dum_glob_reg->dum_ctrl = V_DUM_RESET;
	dum_glob_reg->dum_conf = BIT(9);	/* set priority to "round-robin". All other params to "false" */

	/* Display 1 */
	dum_glob_reg->dum_wtcfg1 = PNX4008_DUM_WT_CFG1;
	dum_glob_reg->dum_rtcfg1 = PNX4008_DUM_RT_CFG1;

	dum_glob_reg->dum_tcfg = PNX4008_DUM_T_CFG;

	return 0;
}

static void dum_chan_init(void)
{
	int i = 0, ch = 0;
	volatile u32 *cmdptrs;
	volatile int *cmdstrings;

	dum_glob_reg->dum_com_base =
	    CMDSTRING_BASEADDR + BYTES_PER_CMDSTRING * NR_OF_CMDSTRINGS;

	if ((cmdptrs =
	     (u32 *) ioremap_nocache(dum_glob_reg->dum_com_base,
				     sizeof(int) * NR_OF_CMDSTRINGS)) == NULL)
		return;

	for (ch = 0; ch < NR_OF_CMDSTRINGS; ch++)
		cmdptrs[ch] = CMDSTRING_BASEADDR + BYTES_PER_CMDSTRING * ch;

	for (ch = 0; ch < MAX_DUM_CHANNELS; ch++)
		clear_channel(ch);

/* clear the cmdstrings */

	cmdstrings =
	    (int *)ioremap_nocache(*cmdptrs,
				   BYTES_PER_CMDSTRING * NR_OF_CMDSTRINGS);

	if (!cmdstrings)
		goto out;

	for (i = 0; i < NR_OF_CMDSTRINGS * BYTES_PER_CMDSTRING / sizeof(int);
	     i++)
		cmdstrings[i] = 0x0;

	iounmap((int *)cmdstrings);

      out:
	iounmap((int *)cmdptrs);
}

static void lcd_init(void)
{
	lcd_reset();

	dum_glob_reg->dum_outp_format1 = 0;	/* DUM_OUTP_FORMAT1, RGB666 */

	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_STANDBY_OFF;
	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_USE_9BIT_BUS;
	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_SYNC_RISE_L;
	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_SYNC_RISE_H;
	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_SYNC_FALL_L;
	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_SYNC_FALL_H;
	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_SYNC_ENABLE;
	udelay(1);
	*(unsigned int *)dum_slave_reg = V_LCD_DISPLAY_ON;
	udelay(1);
}

int __init sdum_init(void)
{
	int ret = 0;

	ret = driver_register(&sdum_driver);

	return ret;

}

static void __exit sdum_exit(void)
{
	driver_unregister(&sdum_driver);
};

module_init(sdum_init);
module_exit(sdum_exit);

MODULE_LICENSE("GPL");

int __init sdum_probe(struct device *device)
{
	int ret = 0, i = 0;

	/* map frame buffer */
	lcd_video_start = (u32) dma_alloc_writecombine(device,
						       FB_DMA_SIZE,
						       &lcd_phys_video_start,
						       GFP_KERNEL);

	if (!lcd_video_start) {
		ret = -ENOMEM;
		goto out_3;
	}

	/* map control registers */
	dum_chan_reg = (struct dumchanregs *)IO_ADDRESS(PNX4008_DUMCONF_BASE);
	dum_glob_reg =
	    (struct dumglobregs *)IO_ADDRESS(PNX4008_DUM_MAINCFG_BASE);
	dum_slave_reg =
	    (u32 *) ioremap_nocache(PNX4008_DUM_SLAVE_BASE, sizeof(u32));

	if (dum_slave_reg == NULL) {
		ret = -ENOMEM;
		goto out_2;
	}

	/* initialize DUM and LCD display */
	ret = dum_init();
	if (ret)
		goto out_1;

	dum_chan_init();
	lcd_init();

	dum_glob_reg->dum_ctrl = V_BAC_ENABLE;
	udelay(1);
	dum_glob_reg->dum_ctrl = V_MUX_RESET;

	/* set decode address and sync clock divider */
	dum_glob_reg->dum_decode = lcd_phys_video_start & DUM_DECODE_MASK;
	dum_glob_reg->dum_clk_div = PNX4008_DUM_CLK_DIV;

	for (i = 0; i < MAX_DUM_CHANNELS; i++)
		fb_owning_channel[i] = -1;

	/*setup wakeup interrupt */
	start_int_set_rising_edge(SE_DISP_SYNC_INT);
	start_int_ack(SE_DISP_SYNC_INT);
	start_int_umask(SE_DISP_SYNC_INT);

	return 0;

      out_1:
	sdum_remove(device);
	iounmap((void *)dum_slave_reg);
      out_2:
	dma_free_writecombine(device, FB_DMA_SIZE, (void *)lcd_video_start,
			      lcd_phys_video_start);
      out_3:
	return ret;
}

static int sdum_remove(struct device *device)
{
	struct clk *clk;

	start_int_mask(SE_DISP_SYNC_INT);

	clk = clk_get(0, "dum_ck");
	if (!IS_ERR(clk)) {
		clk_set_rate(clk, 0);
		clk_put(clk);
	}

	iounmap((void *)dum_slave_reg);

	dma_free_writecombine(device, FB_DMA_SIZE, (void *)lcd_video_start,
			      lcd_phys_video_start);

	return 0;
}

int pnx4008_alloc_dum_channel(int dev_id)
{
	int i = 0;

	while ((i < MAX_DUM_CHANNELS) && (fb_owning_channel[i] != -1))
		i++;

	if (i == MAX_DUM_CHANNELS)
		return -ENORESOURCESLEFT;
	else {
		fb_owning_channel[i] = dev_id;
		return i;
	}
}

EXPORT_SYMBOL(pnx4008_alloc_dum_channel);

int pnx4008_free_dum_channel(int channr, int dev_id)
{
	if (channr < 0 || channr > MAX_DUM_CHANNELS)
		return -EINVAL;
	else if (fb_owning_channel[channr] != dev_id)
		return -EFBNOTOWNER;
	else {
		clear_channel(channr);
		fb_owning_channel[channr] = -1;
	}

	return 0;
}

EXPORT_SYMBOL(pnx4008_free_dum_channel);

static int get_channel(struct dumchannel *p_chan)
{
	int i = p_chan->channelnr;

	if (i < 0 || i > MAX_DUM_CHANNELS)
		return -EINVAL;
	else {
		p_chan->dum_ch_min = dum_chan_reg[i].dum_ch_min;
		p_chan->dum_ch_max = dum_chan_reg[i].dum_ch_max;
		p_chan->dum_ch_conf = dum_chan_reg[i].dum_ch_conf;
		p_chan->dum_ch_stat = dum_chan_reg[i].dum_ch_stat;
		p_chan->dum_ch_ctrl = 0;	/* WriteOnly control register */
	}

	return 0;
}

static int put_channel(struct dumchannel chan)
{
	int i = chan.channelnr;

	if (i < 0 || i > MAX_DUM_CHANNELS)
		return -EINVAL;
	else {
		dum_chan_reg[i].dum_ch_min = chan.dum_ch_min;
		dum_chan_reg[i].dum_ch_max = chan.dum_ch_max;
		dum_chan_reg[i].dum_ch_conf = chan.dum_ch_conf;
		dum_chan_reg[i].dum_ch_ctrl = chan.dum_ch_ctrl;
	}

	return 0;
}

static void clear_channel(int channr)
{
	struct dumchannel chan;

	chan.channelnr = channr;
	chan.dum_ch_min = 0;
	chan.dum_ch_max = 0;
	chan.dum_ch_conf = 0;
	chan.dum_ch_ctrl = 0;

	put_channel(chan);
}

int pnx4008_get_dum_channel_uf(struct dumchannel_uf *p_chan_uf, int dev_id)
{
	int i = p_chan_uf->channelnr;

	if (i < 0 || i > MAX_DUM_CHANNELS)
		return -EINVAL;
	else if (fb_owning_channel[i] != dev_id)
		return -EFBNOTOWNER;
	else {
		p_chan_uf->dirty = chan_uf_store[i].dirty;
		p_chan_uf->source = chan_uf_store[i].source;
		p_chan_uf->x_offset = chan_uf_store[i].x_offset;
		p_chan_uf->y_offset = chan_uf_store[i].y_offset;
		p_chan_uf->width = chan_uf_store[i].width;
		p_chan_uf->height = chan_uf_store[i].height;
	}

	return 0;
}

EXPORT_SYMBOL(pnx4008_get_dum_channel_uf);

int pnx4008_put_dum_channel_uf(struct dumchannel_uf chan_uf, int dev_id)
{
	int i = chan_uf.channelnr;
	int ret;

	if (i < 0 || i > MAX_DUM_CHANNELS)
		return -EINVAL;
	else if (fb_owning_channel[i] != dev_id)
		return -EFBNOTOWNER;
	else if ((ret =
		  display_open(chan_uf.channelnr, 0, chan_uf.dirty,
			       chan_uf.source, chan_uf.y_offset,
			       chan_uf.x_offset, chan_uf.height,
			       chan_uf.width)) != 0)
		return ret;
	else {
		chan_uf_store[i].dirty = chan_uf.dirty;
		chan_uf_store[i].source = chan_uf.source;
		chan_uf_store[i].x_offset = chan_uf.x_offset;
		chan_uf_store[i].y_offset = chan_uf.y_offset;
		chan_uf_store[i].width = chan_uf.width;
		chan_uf_store[i].height = chan_uf.height;
	}

	return 0;
}

EXPORT_SYMBOL(pnx4008_put_dum_channel_uf);

int pnx4008_get_dum_channel_config(int channr, int dev_id)
{
	int ret;
	struct dumchannel chan;

	if (channr < 0 || channr > MAX_DUM_CHANNELS)
		return -EINVAL;
	else if (fb_owning_channel[channr] != dev_id)
		return -EFBNOTOWNER;
	else {
		chan.channelnr = channr;
		if ((ret = get_channel(&chan)) != 0)
			return ret;
	}

	return (chan.dum_ch_conf & DUM_CHANNEL_CFG_MASK);
}

EXPORT_SYMBOL(pnx4008_get_dum_channel_config);

int pnx4008_set_dum_channel_sync(int channr, int val, int dev_id)
{
	if (channr < 0 || channr > MAX_DUM_CHANNELS)
		return -EINVAL;
	else if (fb_owning_channel[channr] != dev_id)
		return -EFBNOTOWNER;
	else {
		if (val == CONF_SYNC_ON) {
			dum_chan_reg[channr].dum_ch_conf |= CONF_SYNCENABLE;
			/* setting sync regval to 0xCA : */
			dum_chan_reg[channr].dum_ch_conf =
			    (dum_chan_reg[channr].
			     dum_ch_conf & DUM_CHANNEL_CFG_SYNC_MASK) |
			    DUM_CHANNEL_CFG_SYNC_MASK_SET;
		} else if (val == CONF_DIRTYDETECTION_OFF)
			dum_chan_reg[channr].dum_ch_conf &= CONF_SYNCENABLE;
		else
			return -EINVAL;
	}

	return 0;
}

EXPORT_SYMBOL(pnx4008_set_dum_channel_sync);

int pnx4008_set_dum_chanel_dirty_detect(int channr, int val, int dev_id)
{
	if (channr < 0 || channr > MAX_DUM_CHANNELS)
		return -EINVAL;
	else if (fb_owning_channel[channr] != dev_id)
		return -EFBNOTOWNER;
	else {
		if (val == CONF_DIRTYDETECTION_ON)
			dum_chan_reg[channr].dum_ch_conf |= CONF_DIRTYENABLE;
		else if (val == CONF_DIRTYDETECTION_OFF)
			dum_chan_reg[channr].dum_ch_conf &= CONF_DIRTYENABLE;
		else
			return -EINVAL;
	}

	return 0;
}

EXPORT_SYMBOL(pnx4008_set_dum_chanel_dirty_detect);

int pnx4008_force_update_dum_channel(int channr, int dev_id)
{
	if (channr < 0 || channr > MAX_DUM_CHANNELS)
		return -EINVAL;

	else if (fb_owning_channel[channr] != dev_id)
		return -EFBNOTOWNER;
	else
		dum_chan_reg[channr].dum_ch_ctrl = CTRL_SETDIRTY;

	return 0;
}

EXPORT_SYMBOL(pnx4008_force_update_dum_channel);

static int put_cmd_string(struct cmdstring cmds)
{
	u16 *cmd_str_virtaddr;
	int *cmd_ptr0_virtaddr;
	int cmd_str_physaddr;

	int i = cmds.channelnr;

	if (i < 0 || i > MAX_DUM_CHANNELS)
		return -EINVAL;
	else if ((cmd_ptr0_virtaddr =
		  (int *)ioremap_nocache(dum_glob_reg->dum_com_base,
					 sizeof(int) * MAX_DUM_CHANNELS)) ==
		 NULL)
		return -EIOREMAPFAILED;
	else {
		cmd_str_physaddr = cmd_ptr0_virtaddr[cmds.channelnr];
		if ((cmd_str_virtaddr =
		     (u16 *) ioremap_nocache(cmd_str_physaddr,
					     sizeof(cmds))) == NULL) {
			iounmap(cmd_ptr0_virtaddr);
			return -EIOREMAPFAILED;
		} else {
			int t;
			for (t = 0; t < 8; t++)
				cmd_str_virtaddr[t] =
				    *((u16 *) & (cmds.prestringlen) + t);

			for (t = 0; t < cmds.prestringlen / 2; t++)
				cmd_str_virtaddr[8 + t] =
				    *((u16 *) & (cmds.precmd) + t);

			for (t = 0; t < cmds.poststringlen / 2; t++)
				cmd_str_virtaddr[8 + t +
						 cmds.prestringlen / 2] =
				    *((u16 *) & (cmds.postcmd) + t);

			iounmap(cmd_ptr0_virtaddr);
			iounmap(cmd_str_virtaddr);
		}
	}

	return 0;
}

int pnx4008_sdum_mmap(struct fb_info *info, struct file *file,
		      struct vm_area_struct *vma, int dev_id)
{
	unsigned long off = vma->vm_pgoff << PAGE_SHIFT;

	off += info->fix.smem_start;

	if (off >= __pa(high_memory) || (file->f_flags & O_SYNC))
		vma->vm_flags |= VM_IO;

	vma->vm_flags |= VM_RESERVED;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	return io_remap_page_range(vma, vma->vm_start, off,
				   vma->vm_end - vma->vm_start,
				   vma->vm_page_prot);
}

EXPORT_SYMBOL(pnx4008_sdum_mmap);

int pnx4008_set_dum_exit_notification(int dev_id)
{
	int i;

	for (i = 0; i < MAX_DUM_CHANNELS; i++)
		if (fb_owning_channel[i] == dev_id)
			return -ERESOURCESNOTFREED;

	return 0;
}

EXPORT_SYMBOL(pnx4008_set_dum_exit_notification);

static u32 dum_ch_setup(int ch_no, dum_ch_setup_t * ch_setup)
{
	struct cmdstring cmds_c;
	struct cmdstring *cmds = &cmds_c;
	disp_window_t dw;
	int standard;
	u32 orientation = 0;
	struct dumchannel chan = { 0 };
	int ret;

	/* check indata */
	dum_ch_setup_chk_input(ch_setup);

	if ((ch_setup->xmirror) || (ch_setup->ymirror) || (ch_setup->rotate)) {
		standard = 0;

		orientation = BIT(1);	/* always set 9-bit-bus */
		if (ch_setup->xmirror)
			orientation |= BIT(4);
		if (ch_setup->ymirror)
			orientation |= BIT(3);
		if (ch_setup->rotate)
			orientation |= BIT(0);
	} else
		standard = 1;

	cmds->channelnr = ch_no;

	/* build command string header */
	if (standard) {
		cmds->prestringlen = 32;
		cmds->poststringlen = 0;
	} else {
		cmds->prestringlen = 48;
		cmds->poststringlen = 16;
	}

	cmds->format =
	    (uint16_t) ((ch_setup->disp_no << 4) | (BIT(3)) | (ch_setup->
							       format));
	cmds->reserved = 0x0;	/*(uint16_t)ch_no; */
	cmds->startaddr_low = (ch_setup->minadr & 0xFFFF);
	cmds->startaddr_high = (ch_setup->minadr >> 16);

	if ((ch_setup->minadr == 0) && (ch_setup->maxadr == 0)
	    && (ch_setup->xmin == 0)
	    && (ch_setup->ymin == 0) && (ch_setup->xmax == 0)
	    && (ch_setup->ymax == 0)) {
		cmds->pixdatlen_low = 0;
		cmds->pixdatlen_high = 0;
	} else {
		u32 nbytes = nof_bytes(ch_setup);
		cmds->pixdatlen_low = (nbytes & 0xFFFF);
		cmds->pixdatlen_high = (nbytes >> 16);
	}

	if (ch_setup->slave_trans)
		cmds->pixdatlen_high |= BIT(15);

/* build pre-string */
	build_disp_window(ch_setup, &dw);

	if (standard) {
		cmds->precmd[0] =
		    build_command(ch_setup->disp_no, DISP_XMIN_L_REG, 0x99);
		cmds->precmd[1] =
		    build_command(ch_setup->disp_no, DISP_XMIN_L_REG,
				  dw.xmin_l);
		cmds->precmd[2] =
		    build_command(ch_setup->disp_no, DISP_XMIN_H_REG,
				  dw.xmin_h);
		cmds->precmd[3] =
		    build_command(ch_setup->disp_no, DISP_YMIN_REG, dw.ymin);
		cmds->precmd[4] =
		    build_command(ch_setup->disp_no, DISP_XMAX_L_REG,
				  dw.xmax_l);
		cmds->precmd[5] =
		    build_command(ch_setup->disp_no, DISP_XMAX_H_REG,
				  dw.xmax_h);
		cmds->precmd[6] =
		    build_command(ch_setup->disp_no, DISP_YMAX_REG, dw.ymax);
		cmds->precmd[7] =
		    build_double_index(ch_setup->disp_no, DISP_PIXEL_REG);
	} else {
		if (dw.xmin_l == ch_no)
			cmds->precmd[0] =
			    build_command(ch_setup->disp_no, DISP_XMIN_L_REG,
					  0x99);
		else
			cmds->precmd[0] =
			    build_command(ch_setup->disp_no, DISP_XMIN_L_REG,
					  ch_no);

		cmds->precmd[1] =
		    build_command(ch_setup->disp_no, DISP_XMIN_L_REG,
				  dw.xmin_l);
		cmds->precmd[2] =
		    build_command(ch_setup->disp_no, DISP_XMIN_H_REG,
				  dw.xmin_h);
		cmds->precmd[3] =
		    build_command(ch_setup->disp_no, DISP_YMIN_REG, dw.ymin);
		cmds->precmd[4] =
		    build_command(ch_setup->disp_no, DISP_XMAX_L_REG,
				  dw.xmax_l);
		cmds->precmd[5] =
		    build_command(ch_setup->disp_no, DISP_XMAX_H_REG,
				  dw.xmax_h);
		cmds->precmd[6] =
		    build_command(ch_setup->disp_no, DISP_YMAX_REG, dw.ymax);
		cmds->precmd[7] =
		    build_command(ch_setup->disp_no, DISP_1_REG, orientation);
		cmds->precmd[8] =
		    build_double_index(ch_setup->disp_no, DISP_PIXEL_REG);
		cmds->precmd[9] =
		    build_double_index(ch_setup->disp_no, DISP_PIXEL_REG);
		cmds->precmd[0xA] =
		    build_double_index(ch_setup->disp_no, DISP_PIXEL_REG);
		cmds->precmd[0xB] =
		    build_double_index(ch_setup->disp_no, DISP_PIXEL_REG);
		cmds->postcmd[0] =
		    build_command(ch_setup->disp_no, DISP_1_REG, BIT(1));
		cmds->postcmd[1] =
		    build_command(ch_setup->disp_no, DISP_DUMMY1_REG, 1);
		cmds->postcmd[2] =
		    build_command(ch_setup->disp_no, DISP_DUMMY1_REG, 2);
		cmds->postcmd[3] =
		    build_command(ch_setup->disp_no, DISP_DUMMY1_REG, 3);
	}

	if ((ret = put_cmd_string(cmds_c)) != 0) {
		return ret;
	}

	chan.channelnr = cmds->channelnr;
	chan.dum_ch_min = ch_setup->dirtybuffer + ch_setup->minadr;
	chan.dum_ch_max = ch_setup->dirtybuffer + ch_setup->maxadr;
	chan.dum_ch_conf = 0x002;
	chan.dum_ch_ctrl = 0x04;

	put_channel(chan);

	return 0;
}

static u32 display_open(int ch_no, int auto_update, u32 * dirty_buffer,
			u32 * frame_buffer, u32 xpos, u32 ypos, u32 w, u32 h)
{

	dum_ch_setup_t k;
	int ret;

	/* keep width & height within display area */
	if ((xpos + w) > DISP_MAX_X_SIZE)
		w = DISP_MAX_X_SIZE - xpos;

	if ((ypos + h) > DISP_MAX_Y_SIZE)
		h = DISP_MAX_Y_SIZE - ypos;

	/* assume 1 display only */
	k.disp_no = 0;
	k.xmin = xpos;
	k.ymin = ypos;
	k.xmax = xpos + (w - 1);
	k.ymax = ypos + (h - 1);

	/* adjust min and max values if necessary */
	if (k.xmin > DISP_MAX_X_SIZE - 1)
		k.xmin = DISP_MAX_X_SIZE - 1;
	if (k.ymin > DISP_MAX_Y_SIZE - 1)
		k.ymin = DISP_MAX_Y_SIZE - 1;

	if (k.xmax > DISP_MAX_X_SIZE - 1)
		k.xmax = DISP_MAX_X_SIZE - 1;
	if (k.ymax > DISP_MAX_Y_SIZE - 1)
		k.ymax = DISP_MAX_Y_SIZE - 1;

	k.xmirror = 0;
	k.ymirror = 0;
	k.rotate = 0;
	k.minadr = (u32) frame_buffer;
	k.maxadr = (u32) frame_buffer + (((w - 1) << 10) | ((h << 2) - 2));
	k.pad = PAD_1024;
	k.dirtybuffer = (u32) dirty_buffer;
	k.format = RGB888;
	k.hwdirty = 0;
	k.slave_trans = 0;

	ret = dum_ch_setup(ch_no, &k);

	return ret;
}

u32 build_command(int disp_no, u32 reg, u32 val)
{
	return ((disp_no << 26) | BIT(25) | (val << 16) | (disp_no << 10) |
		(reg << 0));
}

u32 build_double_index(int disp_no, u32 val)
{
	return ((disp_no << 26) | (val << 16) | (disp_no << 10) | (val << 0));
}

void build_disp_window(dum_ch_setup_t * ch_setup, disp_window_t * dw)
{
	dw->ymin = ch_setup->ymin;
	dw->ymax = ch_setup->ymax;
	dw->xmin_l = ch_setup->xmin & 0xFF;
	dw->xmin_h = (ch_setup->xmin & BIT(8)) >> 8;
	dw->xmax_l = ch_setup->xmax & 0xFF;
	dw->xmax_h = (ch_setup->xmax & BIT(8)) >> 8;
}

void dum_setup_chk_input(dum_setup_t * setup)
{
	/* check that command base address in located in iram or dram */
	ASSERT(((setup->command_base_adr > 0x00000000)
		&& (setup->command_base_adr < 0x00010000))
	       || ((setup->command_base_adr > 0x08000000)
		   && (setup->command_base_adr < 0x08010000))
	       || ((setup->command_base_adr > 0x80000000)));
	ASSERT(setup->sync_clk_div < 0x10000);
	ASSERT(setup->sync_restart_val < 512);
	ASSERT(setup->set_sync_high < 512);
	ASSERT(setup->set_sync_low < 512);
	ASSERT(setup->sync_output == 0);
}

void dum_ch_setup_chk_input(dum_ch_setup_t * ch_setup)
{

	ASSERT((ch_setup->format >= 0) && (ch_setup->format < 8));
	ASSERT((ch_setup->pad >= 0) && (ch_setup->pad < 3));
	ASSERT(ch_setup->minadr <= ch_setup->maxadr);
	/* check that min&max addresses are located in iram, dram or YUV2RGB */
	ASSERT((ch_setup->minadr < 0x00010000) ||
	       ((ch_setup->minadr >= 0x08000000)
		&& (ch_setup->minadr < 0x08010000))
	       || ((ch_setup->minadr >= 0x10000000)
		   && (ch_setup->minadr < 0x18000000))
	       || ((ch_setup->minadr >= 0x80000000)));
	ASSERT((ch_setup->maxadr < 0x00010000)
	       || ((ch_setup->maxadr >= 0x08000000)
		   && (ch_setup->maxadr < 0x08010000))
	       || ((ch_setup->maxadr >= 0x10000000)
		   && (ch_setup->maxadr < 0x18000000))
	       || ((ch_setup->maxadr >= 0x80000000)));

	ASSERT(ch_setup->xmin <= ch_setup->xmax);
	ASSERT(ch_setup->ymin <= ch_setup->ymax);
	ASSERT(ch_setup->xmax < 320);
	ASSERT((ch_setup->minadr & BIT(0)) == 0);
	ASSERT((ch_setup->maxadr & BIT(0)) == 0);
	switch (ch_setup->format) {
	case RGB888:
	case RGB666:		/* fullword formats */
		ASSERT((ch_setup->minadr & BIT(1)) == 0);
		ASSERT((ch_setup->maxadr & BIT(1)) == BIT(1));
		break;
	default:		/* halfword formats */
		break;
	}

	if (ch_setup->pad == PAD_NONE)
		ASSERT(ch_setup->hwdirty == 0);
	if (ch_setup->hwdirty) {
		ASSERT((ch_setup->pad == PAD_512)
		       || (ch_setup->pad == PAD_1024));
		ASSERT((ch_setup->minadr & 0x1FF00000) == DUM_DECODE_RD);
		ASSERT((ch_setup->maxadr & 0x1FF00000) == DUM_DECODE_RD);
	}

	switch (ch_setup->pad) {
	case PAD_NONE:
		ASSERT(nof_pixels_mxy(ch_setup) == nof_pixels_dxy(ch_setup));
		break;
	case PAD_512:
	case PAD_1024:
		if (!(ch_setup->rotate)) {
			ASSERT(nof_pixels_mx(ch_setup) ==
			       nof_pixels_dx(ch_setup));
			ASSERT(nof_pixels_my(ch_setup) ==
			       nof_pixels_dy(ch_setup));
		} else {
			ASSERT(nof_pixels_mx(ch_setup) ==
			       nof_pixels_dy(ch_setup));
			ASSERT(nof_pixels_my(ch_setup) ==
			       nof_pixels_dx(ch_setup));
		}
		break;
	default:
		ASSERT(0);
	}

}

/* Debug/Assert functions */
u32 nof_pixels_my(dum_ch_setup_t * ch_setup)
{
	u32 ymin = 0;
	u32 ymax = 0;
	u32 r = 0;

	switch (ch_setup->pad) {
	case PAD_512:
		ymin = ch_setup->minadr & 0x1FE;
		ymax = ch_setup->maxadr & 0x1FE;
		break;

	case PAD_1024:
		ymin = ch_setup->minadr & 0x3FE;
		ymax = ch_setup->maxadr & 0x3FE;
		break;

	default:
		ASSERT(0);
	}

	ASSERT(ymax >= ymin);

	switch (ch_setup->format) {
	case RGB888:
	case RGB666:		/* fullword formats */
		r = (ymax - ymin + 4) / 4;
		break;

	case RGB565:
	case BGR565:
	case ARGB1555:
	case ABGR1555:
	case ARGB4444:
	case ABGR4444:		/* halfword formats */
		r = (ymax - ymin + 2) / 2;
		break;

	default:
		ASSERT(0);
	}

	return (r);
}

u32 nof_pixels_mx(dum_ch_setup_t * ch_setup)
{
	u32 xmin = 0;
	u32 xmax = 0;
	u32 r = 0;

	switch (ch_setup->pad) {
	case PAD_512:
		xmin = ch_setup->minadr & 0x000FFE00;
		xmax = ch_setup->maxadr & 0x000FFE00;
		r = ((xmax - xmin) >> 9) + 1;
		break;

	case PAD_1024:
		xmin = ch_setup->minadr & 0x000FFC00;
		xmax = ch_setup->maxadr & 0x000FFC00;
		r = ((xmax - xmin) >> 10) + 1;
		break;

	default:
		ASSERT(0);
	}

	return (r);
}

u32 nof_pixels_mxy(dum_ch_setup_t * ch_setup)
{
	u32 r = 0;

	switch (ch_setup->pad) {
	case PAD_NONE:
		switch (ch_setup->format) {
		case RGB888:
		case RGB666:	/* fullword formats */
			r = (ch_setup->maxadr - ch_setup->minadr + 4) / 4;
			break;

		default:	/* halfword formats */
			r = (ch_setup->maxadr - ch_setup->minadr + 2) / 2;
			break;
		}
		break;

	case PAD_512:
	case PAD_1024:
		r = nof_pixels_mx(ch_setup) * nof_pixels_my(ch_setup);
		break;

	default:
		ASSERT(0);
		break;
	}

	return (r);
}

u32 nof_pixels_dx(dum_ch_setup_t * ch_setup)
{
	return (ch_setup->xmax - ch_setup->xmin + 1);
}

u32 nof_pixels_dy(dum_ch_setup_t * ch_setup)
{
	return (ch_setup->ymax - ch_setup->ymin + 1);
}

u32 nof_pixels_dxy(dum_ch_setup_t * ch_setup)
{
	return (nof_pixels_dx(ch_setup) * nof_pixels_dy(ch_setup));
}

u32 nof_bytes(dum_ch_setup_t * ch_setup)
{
	u32 r;

	r = nof_pixels_dxy(ch_setup);
	switch (ch_setup->format) {
	case RGB888:
	case RGB666:
		r *= 4;
		break;

	default:
		r *= 2;
		break;
	}

	return r;
}
