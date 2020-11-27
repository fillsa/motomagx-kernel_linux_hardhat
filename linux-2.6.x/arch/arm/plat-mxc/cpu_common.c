/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/hardware.h>
#include <asm/setup.h>

extern const u32 system_rev_tbl[SYSTEM_REV_NUM][2];

/*!
 * @file cpu_common.c
 *
 * @brief This file contains the common CPU initialization code.
 *
 * @ingroup System
 */

static int system_rev_updated = 0;

static void __init system_rev_setup(char **p)
{
	system_rev = simple_strtoul(*p, NULL, 16);
	system_rev_updated = 1;
}

/* system_rev=0x10 for pass 1; 0x20 for pass 2.0; 0x21 for pass 2.1; etc */
__early_param("system_rev=", system_rev_setup);

/*
 * This functions reads the IIM module and returns the system revision number.
 * It does the translation from the IIM silicon revision reg value to our own
 * convention so that it is more readable.
 */
static u32 read_system_rev(void)
{
	u32 val;
	u32 i;

	val = __raw_readl(SYSTEM_PREV_REG);

	/* If the IIM doesn't contain valid product signature, return
	 * the lowest revision number */
	if (MXC_GET_FIELD(val, IIM_PROD_REV_LEN, IIM_PROD_REV_SH) !=
	    PROD_SIGNATURE) {
		printk(KERN_ALERT
		       "Can't find valid PROD_REV. Default Rev=0x%x\n",
		       SYSTEM_REV_MIN);
		return SYSTEM_REV_MIN;
	}

	/* Now trying to retrieve the silicon rev from IIM's SREV register */
	val = __raw_readl(SYSTEM_SREV_REG);

	/* ckeck SILICON_REV[7:0] first with ROM ver at [3:2] masked off */
	for (i = 0; i < SYSTEM_REV_NUM; i++) {
		if ((val & 0xF3) == system_rev_tbl[i][0]) {
			return system_rev_tbl[i][1];
		}
	}
	/* check again with SILICON_REV[3:0] masked off */
	for (i = 0; i < SYSTEM_REV_NUM; i++) {
		if ((val & 0xF0) == system_rev_tbl[i][0]) {
			return system_rev_tbl[i][1];
		}
	}
	/* Reach here only the SREV value unknown. Maybe due to new tapeout? */
	if (i == SYSTEM_REV_NUM) {
		panic("Found wrong system_rev 0x%x. \n\n", val);
	}
}

/*
 * Update the system_rev value.
 * If no system_rev is passed in through the command line, it gets the value
 * from the IIM module. Otherwise, it uses the pass-in value.
 */
static void system_rev_update(void)
{
	int i;

	if (!system_rev_updated) {
		/* means NO value passed-in through command line */
		system_rev = read_system_rev();
		pr_info("system_rev is: 0x%x\n", system_rev);
	} else {
		pr_info("Command line passed system_rev: 0x%x\n", system_rev);
		for (i = 0; i < SYSTEM_REV_NUM; i++) {
			if (system_rev == system_rev_tbl[i][1]) {
				break;
			}
		}
		/* Reach here only the SREV value unknown. Maybe due to new tapeout? */
		if (i == SYSTEM_REV_NUM) {
			panic("Command system_rev is unknown\n");
		}
	}
}

static int mxc_jtag_enabled __initdata;	/* OFF: 0 (default), ON: 1 */

/*
 * Here are the JTAG options from the command line. By default JTAG
 * is OFF which means JTAG is not connected and WFI is enabled
 *
 *       "on" --  JTAG is connected, so WFI is disabled
 *       "off" -- JTAG is disconnected, so WFI is enabled
 */

static void __init jtag_wfi_setup(char **p)
{
	if (memcmp(*p, "on", 2) == 0) {
		mxc_jtag_enabled = 1;
		*p += 2;
	} else if (memcmp(*p, "off", 3) == 0) {
		mxc_jtag_enabled = 0;
		*p += 3;
	}
}

__early_param("jtag=", jtag_wfi_setup);

/*!
 * Enable or Disable WFI based on JTAG on boot command line
 */
static void __init jtag_wfi_init(void)
{
	if (mxc_jtag_enabled) {
		/* Disable WFI as JTAG is connected */
		__raw_writel(__raw_readl(AVIC_VECTOR) & ~(MXC_WFI_ENABLE),
			     AVIC_VECTOR);
		pr_debug("jtag: on\n");
	} else {
		/* Enable WFI as JTAG is not connected */
		__raw_writel(__raw_readl(AVIC_VECTOR) | (MXC_WFI_ENABLE),
			     AVIC_VECTOR);
		pr_debug("jtag: off\n");
	}
}

void mxc_cpu_common_init(void)
{
	system_rev_update();
	jtag_wfi_init();
}
