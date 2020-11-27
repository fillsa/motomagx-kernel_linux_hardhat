/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2006 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Additional defines for IOMUX pad settings.
 */

#ifndef _MACH_MXC91231_IOMUX_H_
#define _MACH_MXC91231_IOMUX_H_

#include <linux/types.h>

#include <asm/hardware.h>

/*!
 * @name IOMUX/PAD Bit field definitions
 */

/*!
 * IOMUX PAD Groups
 */
enum iopad_group {
	IOPAD_GROUP1,
	IOPAD_GROUP2,
	IOPAD_GROUP3,
	IOPAD_GROUP4,
	IOPAD_GROUP5,
	IOPAD_GROUP6,
	IOPAD_GROUP7,
	IOPAD_GROUP8,
	IOPAD_GROUP9,
	IOPAD_GROUP10,
	IOPAD_GROUP11,
	IOPAD_GROUP12,
	IOPAD_GROUP13,
	IOPAD_GROUP14,
	IOPAD_GROUP15,
	IOPAD_GROUP16,
	IOPAD_GROUP17,
	IOPAD_GROUP18,
	IOPAD_GROUP19,
	IOPAD_GROUP20,
	IOPAD_GROUP21,
	IOPAD_GROUP22,
	IOPAD_GROUP23,
	IOPAD_GROUP24,
	IOPAD_GROUP25,
	IOPAD_GROUP26,
	IOPAD_GROUP27,
	IOPAD_GROUP28,
};

/*!
 * various IOMUX output functions
 */
enum iomux_output_config {
	OUTPUTCONFIG_DEFAULT = 0,
	OUTPUTCONFIG_FUNC1,
	OUTPUTCONFIG_FUNC2,
	OUTPUTCONFIG_FUNC3,
	OUTPUTCONFIG_FUNC4,
	OUTPUTCONFIG_FUNC5,
	OUTPUTCONFIG_FUNC6,
	OUTPUTCONFIG_FUNC7,
};

/*!
 * various IOMUX input functions
 */
enum iomux_input_config {
	INPUTCONFIG_NONE = 0,
	INPUTCONFIG_DEFAULT = 1 << 0,
	INPUTCONFIG_FUNC1 = 1 << 1,
	INPUTCONFIG_FUNC2 = 1 << 2,
	INPUTCONFIG_FUNC3 = 1 << 3,
};

/*!
 * various IOMUX pad functions
 */
enum iomux_pad_config {
#if defined(CONFIG_MOT_FEAT_GPIO_API)
        SRE_FAST        = 0x1 << 0,
        SRE_SLOW        = 0x0 << 0,
        DSE_NORMAL      = 0x0 << 1,
        DSE_HIGH        = 0x1 << 1,
        DSE_MAX         = 0x2 << 1,
        DSE_MIN         = 0x3 << 1,
        ODE_OPEN_DRAIN  = 0x1 << 3,
        ODE_CMOS        = 0x0 << 3,
        PUE_ENABLE      = 0x1 << 4,
        PUE_KEEPER      = 0x0 << 4,
        PUS_PULLDOWN    = 0x0 << 5,
        PUS_47K_PULLUP  = 0x1 << 5,
        PUS_100K_PULLUP = 0x2 << 5,
        PUS_22K_PULLUP  = 0x3 << 5,
        PKE_ENABLE      = 0x1 << 7,
        PKE_DISABLE     = 0x0 << 7,
        HYS_SCHMITZ     = 0x1 << 8,
        HYS_CMOS        = 0x0 << 8,
        DDR_MODE_DDR    = 0x1 << 9,
        DDR_MODE_CMOS   = 0x0 << 9,
        DDR_INPUT_DDR   = 0x1 << 10,
        DDR_INPUT_CMOS  = 0x0 << 10,
#else
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
#endif
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
void iomux_set_pad(enum iopad_group grp, __u32 config);

#endif
