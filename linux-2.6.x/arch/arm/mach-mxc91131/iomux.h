/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef _MACH_MXC_IOMUX_H
#define _MACH_MXC_IOMUX_H

#include <linux/types.h>

/*!
 * @name IOMUX/PAD Bit field definitions
 */

/*!
 * IOMUX PAD Groups
 */
enum iopad_group {

	/* IOMUX_AP pad groups */
	PAD_GROUP10 = 0,
	PAD_GROUP11,
	PAD_GROUP12,
	PAD_GROUP13,
	PAD_GROUP14,
	PAD_GROUP15,
	PAD_GROUP16,
	PAD_GROUP17,
	PAD_GROUP18,
	PAD_GROUP19,
	PAD_GROUP20,
	PAD_GROUP21,
	PAD_GROUP22,
	PAD_GROUP23,
	PAD_GROUP24,
	PAD_GROUP25,
	PAD_GROUP26,
	PAD_GROUP27,
	PAD_GROUP28,
	PAD_GROUP29,
	PAD_GROUP30,
	PAD_GROUP31,
	PAD_GROUP32,
	PAD_GROUP33,
	PAD_GROUP34,
	PAD_GROUP35,
	PAD_GROUP36,
	PAD_GROUP37,
	PAD_GROUP38,
	PAD_GROUP39,
	PAD_GROUP40,
	PAD_GROUP41,
	PAD_GROUP42,
	PAD_GROUP43,
	PAD_GROUP44,
	PAD_GROUP45,
	PAD_GROUP46,
	PAD_GROUP47,
	PAD_GROUP48,
	PAD_GROUP49,
	PAD_GROUP50,
	PAD_GROUP51,
	PAD_GROUP52,
	PAD_GROUP53,
	PAD_GROUP54,
	PAD_GROUP55,
	PAD_GROUP56,
	PAD_GROUP57,
	PAD_GROUP58,

	/* IOMUX_COM pad groups */
	PAD_GROUP1,
	PAD_GROUP2,
	PAD_GROUP3,
	PAD_GROUP4,
	PAD_GROUP5,
	PAD_GROUP6,
	PAD_GROUP7,
	PAD_GROUP8,
	PAD_GROUP9,
	PAD_GROUP59,
	PAD_GROUP60,
	PAD_GROUP61,
	PAD_GROUP62,
	PAD_GROUP63,
	PAD_GROUP64,
	PAD_GROUP65,
	PAD_GROUP66,
	PAD_GROUP67,
	PAD_GROUP68,
	PAD_GROUP69,
	PAD_GROUP70,
	PAD_GROUP71,
	PAD_GROUP73,
	PAD_GROUP74,
	PAD_GROUP75,
	PAD_GROUP99,
	PAD_GROUP100,
	PAD_GROUP101,
	PAD_GROUP102,
	PAD_GROUP103,

	AP_MAX_PAD_GROUP = PAD_GROUP58,
};

/*!
 * various IOMUX output functions
 */
enum iomux_output_config {
	GPIO_MUX1_OUT = 0,
	MUX0_OUT,
	MUX2_OUT,
	MUX3_OUT,
	MUX4_OUT,
	MUX5_OUT,
	MUX6_OUT,
	MUX7_OUT,
};

/*!
 * various IOMUX input functions
 */
enum iomux_input_config {
	NONE_IN = 0,
	GPIO_MUX1_IN = (1 << 0),
	MUX0_IN = (1 << 1),
	MUX2_IN = (1 << 2),
	MUX3_IN = (1 << 3),
};

/*!
 * various IOMUX pad functions
 */
enum iomux_pad_config {
	SRE_FAST = 0x1 << 0,
	SRE_SLOW = 0x0 << 0,
	DSE_NORMAL = 0x0 << 1,
	DSE_HIGH = 0x1 << 1,
	DSE_MAX = 0x2 << 1,
	DSE_MIN = 0x3 << 1,
	ODE_CMOS = 0x0 << 3,
	ODE_OPEN_DRAIN = 0x1 << 3,
	PKE_ENABLE = 0x1 << 7,
	PKE_DISABLE = 0x0 << 7,
	PUE_ENABLE = 0x1 << 4,
	PUE_KEEPER = 0x0 << 4,
	HYS_SCHMITZ = 0x1 << 8,
	HYS_CMOS = 0x0 << 8,
	DDR_MODE_DDR = 0x1 << 9,
	DDR_MODE_CMOS = 0x0 << 9,
	DDR_INPUT_DDR = 0x1 << 10,
	DDR_INPUT_CMOS = 0x0 << 10,
};

/*!
 * This function is used to configure a pin through the IOMUX module.
 *
 * @param  pin          pin number as defined in \b #iomux_pins
 * @param  out          output function as defined in \b #iomux_output_config
 * @param  in           input function as defined in \b #iomux_input_config
 */
void iomux_config_mux(enum iomux_pins pin,
		      enum iomux_output_config out, enum iomux_input_config in);

/*!
 * This function returns the pad value for a IOMUX group.
 *
 * @param  grp          a group number as defined in \b enum \b #iopad_group
 *
 * @return A group pad value
 */
__u16 iomux_get_pad(enum iopad_group grp);

/*!
 * This function configures the pad value for a IOMUX group.
 *
 * @param  grp          a group number as defined in \b enum \b #iopad_group
 * @param  config       the ORed value of elements defined in \b enum \b #iomux_pad_config
 */
void iomux_set_pad(enum iopad_group grp, __u16 config);

#endif
