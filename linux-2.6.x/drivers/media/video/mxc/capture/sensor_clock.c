/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file sensor_clock.c
 *
 * @brief camera clock function
 *
 * @ingroup Camera
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/device.h>
#include "asm/arch/clock.h"

/*
 * set_mclk_rate
 *
 * @param       p_mclk_freq  mclk frequence
 *
 */
void set_mclk_rate(uint32_t * p_mclk_freq)
{
	uint32_t div;
	// Calculate the divider using the requested, minimum mclk freq
	div = (mxc_get_clocks_parent(CSI_BAUD) * 2) / *p_mclk_freq;
	/* Calculate and return the actual mclk frequency.
	   The integer division error/truncation will ensure the actual freq is
	   greater than the requested freq.
	 */
	if (*p_mclk_freq < (mxc_get_clocks_parent(CSI_BAUD) * 2) / div) {
		div++;
	}

	*p_mclk_freq = (mxc_get_clocks_parent(CSI_BAUD) * 2) / div;

	mxc_set_clocks_div(CSI_BAUD, div);

	printk("mclk frequency = %d\n", *p_mclk_freq);
}
