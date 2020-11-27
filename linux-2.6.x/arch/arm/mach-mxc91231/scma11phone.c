/*
 *  linux/arch/arm/mach-mxc91231/scma11phone.c
 *
 *  Hardware initialization for Motorola's SCM-A11-based phones.
 *
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright (C) 2002 Shane Nay (shane@minirl.com)
 *  Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright (C) 2006-2007 Motorola, Inc.
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
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Modified function and data structure naming
 *                    to reference Motorola specific products
 *                    Added USB highspeed chipset support
 * 12/2006  Motorola  Added power_ic_kernel.h include
 * 01/2007  Motorola  Set clkctl_gp_ctrl[7] to keep ARM on when JTAG attached.
 * 05/2007  Motorola  Expose reboot procedure via __mxc_power_off().
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/pm.h>
#include <linux/nodemask.h>
#include <linux/reboot.h>

#include <linux/power_ic_kernel.h>

#include <asm/hardware.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/keypad.h>
#include <asm/arch/gpio.h>
#if defined(CONFIG_MOT_FEAT_PM)
#include <asm/arch/clock.h>
#endif

#include "crm_regs.h"
#include "iomux.h"

#include "mot-gpio/mot-gpio-scma11.h"

/*
 * Define for power-off
 */ 

#define  _reg_WDOG_WCR    ((volatile __u16 *)( IO_ADDRESS(WDOG1_BASE_ADDR + 0x00)))  


/*!
 * @file scma11phone.c
 * 
 * @brief This file contains the board specific initialization routines. 
 * 
 * @ingroup System
 */

extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_cpu_init(void) __init;
extern struct sys_timer mxc_timer;
extern void mxc_cpu_common_init(void);
extern void mxc_gpio_init(void) __init;

#ifdef CONFIG_MOT_FEAT_FX2LP_I2C
extern int start_fx2lp_fw_dl(void) __init;
#endif


/*!
 * SCM-A11 specific fixup function. It is called by \b setup_arch() in 
 * setup.c file very early on during kernel starts. It allows the user to 
 * statically fill in the proper values for the passed-in parameters. None of 
 * the parameters is used currently.
 * 
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line.  cmdline defaults to 
 *                      CONFIG_CMDLINE, and should not be read from in this 
 *                      function.  The cmdline may change in setup_arch()
 *                      if the user specifies a custom command line through 
 *                      the bootloader or provides ATAGs.
 * @param  mi           pointer to \b struct \meminfo
 */
static void __init fixup_mxc_board(struct machine_desc *desc, 
                                      struct tag *tags, char **cmdline, 
                                      struct meminfo *mi)
{
        struct tag * t;

	mxc_cpu_init();
	for_each_tag(t, tags) {
		if (t->hdr.tag == ATAG_MEM) {
			t->u.mem.size = MEM_SIZE;
			break;
		}
	}
	if (t->hdr.size == 0) {
		printk("%s: no mem tag found\n", __FUNCTION__);
	}

#ifdef CONFIG_DISCONTIGMEM
	do {
		int nid;
		mi->nr_banks = MXC_NUMNODES;
		for (nid = 0; nid < mi->nr_banks; nid++) {
			SET_NODE(mi, nid);
		}
	} while (0);
#endif
}


/**
 * Low-level power off function. This is used by mxc_power_off below
 * and by machine_restart when CONFIG_MOT_FEAT_SYSREBOOT_ATLAS is set.
 */
void __mxc_power_off(void)
{
        local_irq_disable();
	*_reg_WDOG_WCR |= 0x0040;
	*_reg_WDOG_WCR &= ~(0x0020);
        while(1);
}


/*
 * Default power-off for MXC91231
 */
static void mxc_power_off(void)
{
        /* If charger is attached, perform a reset instead of shutdown. */
        if(power_ic_is_charger_attached() > 0) {
                machine_restart(0);
        }

        __mxc_power_off();
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
	mxc_cpu_common_init();

        /*
         * for poweroff 
         */ 
        pm_power_off = mxc_power_off;

#if defined(CONFIG_MOT_FEAT_PM)
        /* 
         * Set clkctl_gp_ctrl[7]. This allows the ARM clock to remain running
         * during standby when the JTAG is connected. This prevents a
         * watchdog reset encountered when the ARM issues a WFI while the
         * JTAG is connected.
         *
         * CLKCTL_GP_SER is write-one-to-set. This eliminates the need for
         * a read-modify-write.
         */
        __raw_writel(MXC_CLKCTL_GP_CTRL_BIT7, MXC_CLKCTL_GP_SER);
#endif /* CONFIG_MOT_FEAT_PM */

	mxc_gpio_init();
        scma11phone_gpio_init();

        /* download firmware to USB HS chip */
#if defined(CONFIG_MOT_FEAT_FX2LP_I2C)
        if(start_fx2lp_fw_dl()) {
                printk(KERN_ERR "FX2LP firmware download over i2c failed!");
        } else {
                printk("FX2LP firmware download completed\n");
        }
#endif

#if defined(CONFIG_MOT_FEAT_PM)
        /* Turn off the USBPLL early during bootup process
         * This will ensure that the physical USBPLL state
         * is consistent with that maintained by pll_requests[]
         * From this point on, any module will need to explicitly
         * request for this PLL to be turned ON by using
         * mxc_pll_request_pll
         * */
        (void) mxc_pll_release_pll(USBPLL);
#endif

}

/*
 * The following uses standard kernel macros define in arch.h in order to 
 * initialize __mach_desc_SCMA11PHONE data structure.
 */
MACHINE_START(SCMA11PHONE, "Motorola Product - SCM-A11 Phone")
        MAINTAINER("Motorola, Inc.")
        /*       physical memory    physical IO        virtual IO     */
        BOOT_MEM(PHYS_OFFSET_ASM, AIPS1_BASE_ADDR, AIPS1_BASE_ADDR_VIRT)
	BOOT_PARAMS(PHYS_OFFSET_ASM + 0x100)
        FIXUP(fixup_mxc_board)
        MAPIO(mxc_map_io)
        INITIRQ(mxc_init_irq)
        INIT_MACHINE(mxc_board_init)
        .timer		= &mxc_timer,
MACHINE_END
