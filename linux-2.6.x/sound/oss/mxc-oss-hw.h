/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
 *
 * Copyright (C) 2005 Freescale Semiconductor, Inc. All rights reserved.
 *
 */

/*!
 * @file mxc-oss-hw.h
 * @brief Definitions and prototypes for mxc-oss-hw.c
 *
 * @ingroup SOUND_OSS_MXC_PMIC
 */

#ifndef _MXC_OSS_HW_H
#define _MXC_OSS_HW_H

#include "mxc-oss-defs.h"

int pmic_select_dev_in(mxc_state_t * const drv_inst);
int pmic_select_dev_out(mxc_state_t * const drv_inst);

int pmic_enable_in_device(mxc_state_t * const drv_inst, const int en_flag);
int pmic_enable_out_device(mxc_state_t * const drv_inst, const int en_flag);

int pmic_select_src_in(mxc_state_t * const drv_inst, const int src_in);
int pmic_get_src_in_selected(mxc_state_t * const drv_inst);

int pmic_select_src_out(mxc_state_t * const drv_inst, const int src_out);
int pmic_get_src_out_selected(mxc_state_t * const drv_inst);

int pmic_set_input_bias(mxc_state_t * const drv_inst, const int onoff);
int pmic_get_input_bias(mxc_state_t * const drv_inst);

int pmic_set_output_balance(mxc_state_t * const drv_inst, const int val);
int pmic_get_output_balance(mxc_state_t * const drv_inst);

int pmic_set_output_mixer(mxc_state_t * const drv_inst);
int pmic_get_output_mixer(mxc_state_t * const drv_inst);

int pmic_set_output_adder(mxc_state_t * const drv_inst, const int val);
int pmic_get_output_adder(mxc_state_t * const drv_inst);

int pmic_set_src_out_volume(mxc_state_t * const drv_inst, const int src_mask,
			    const int val);
int pmic_get_src_out_volume(mxc_state_t * const drv_inst, const int src_mask);

int pmic_set_src_in_volume(mxc_state_t * const drv_inst, const int src_mask,
			   const int val);
int pmic_get_src_in_volume(mxc_state_t * const drv_inst, const int src_mask);

int pmic_set_codec_filter(mxc_state_t * const drv_inst, const int mask);
int pmic_get_codec_filter(mxc_state_t * const drv_inst);

void pmic_close_audio(mxc_state_t * const drv_inst);

void mxc_audio_hw_reset(void);

void mxc_reset_ssi(const int index);
void mxc_setup_ssi(const mxc_state_t * const drv_inst);

int mxc_setup_pmic_stdac(const mxc_state_t * const drv_inst);
int mxc_setup_pmic_codec(const mxc_state_t * const drv_inst);

int mxc_setup_dam(const mxc_state_t * const drv_inst);

#endif				/* _MXC_OSS_HW_H */
