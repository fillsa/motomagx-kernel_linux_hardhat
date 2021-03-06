/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2008 Motorola, Inc. 
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Date     Author      Comment
 * 07/2008  Motorola    add two interface for reset IC_EN
 */
#ifndef _INCLUDE_IPU_PRV_H_
#define _INCLUDE_IPU_PRV_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <asm/arch/hardware.h>

/*
 * IPU Debug Macros
 */
/*#define IPU_DEBUG */

#ifdef IPU_DEBUG

#define DDPRINTK(fmt, args...) printk(KERN_ERR"%s :: %d :: %s - " fmt, __FILE__,__LINE__,__FUNCTION__ , ## args)
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#define PRINTK(fmt, args...) printk(fmt, ## args)

#define FUNC_START  DPRINTK(" func start\n")
#define FUNC_END    DPRINTK(" func end\n")

#define FUNC_ERR printk(KERN_ERR"%s :: %d :: %s  err= %d \n", __FILE__,__LINE__,__FUNCTION__ ,err)

#else               /*IPU_DEBUG*/

#define DDPRINTK(fmt, args...)  do {} while(0)
#define DPRINTK(fmt, args...)   do {} while(0)
#define PRINTK(fmt, args...)    do {} while(0)

#define FUNC_START
#define FUNC_END

#endif              /*IPU_DEBUG*/

/* Globals */
extern raw_spinlock_t ipu_lock;
extern uint32_t g_ipu_clk;

uint32_t bytes_per_pixel(uint32_t fmt);
ipu_color_space_t format_to_colorspace(uint32_t fmt);

uint32_t _ipu_channel_status(ipu_channel_t channel);

void _ipu_sdc_fg_init(ipu_channel_params_t * params);
uint32_t _ipu_sdc_fg_uninit(void);
void _ipu_sdc_bg_init(ipu_channel_params_t * params);
uint32_t _ipu_sdc_bg_uninit(void);



void _ipu_ic_enable_task(ipu_channel_t channel);
void _ipu_ic_disable_task(ipu_channel_t channel);

void _ipu_ic_init_prpvf(ipu_channel_params_t * params, bool src_is_csi);
void _ipu_ic_uninit_prpvf(void);
void _ipu_ic_init_rotate_vf(ipu_channel_params_t * params);
void _ipu_ic_uninit_rotate_vf(void);
void _ipu_ic_init_csi(ipu_channel_params_t * params);
void _ipu_ic_uninit_csi(void);
void _ipu_ic_init_prpenc(ipu_channel_params_t * params, bool src_is_csi);
void _ipu_ic_uninit_prpenc(void);
void _ipu_ic_init_rotate_enc(ipu_channel_params_t * params);
void _ipu_ic_uninit_rotate_enc(void);
void _ipu_ic_init_pp(ipu_channel_params_t * params);
void _ipu_ic_uninit_pp(void);
void _ipu_ic_init_rotate_pp(ipu_channel_params_t * params);
void _ipu_ic_uninit_rotate_pp(void);

int32_t _ipu_adc_init_channel(ipu_channel_t chan, display_port_t disp,
                  mcu_mode_t cmd, int16_t x_pos, int16_t y_pos);
int32_t _ipu_adc_uninit_channel(ipu_channel_t chan);

#endif              /* _INCLUDE_IPU_PRV_H_ */
