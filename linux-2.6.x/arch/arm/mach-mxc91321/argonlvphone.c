/*
 * linux/arch/arm/mach-mxc91321/argonlvphone.c
 *
 * Copyright 2006-2007 Motorola, Inc.
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
 * Date         Author      Comment
 * ==========   ==========  ================================================
 * 10/25/2006   Motorola    Modified function and data structure naming
 *                          to reference Motorola specific product
 *                          Added product specific initialization routines
 *                          Enabled 26 mhz clock for sound support
 *                          Added high speed USB support
 * 10/26/2006   Motorola    Renamed to argonlvphone
 * 01/17/2007   Motorola    Fixed argonlvbute_power_off
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/nodemask.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/arch/memory.h>
#include <asm/arch/board.h>
#include <linux/pm.h>

#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>
#include "crm_regs.h"

#include "mot-gpio/mot-gpio-argonlv.h"

/*!
 * @file argonlvphone.c
 * 
 * @brief This file contains the board specific initialization routines. 
 * 
 * @ingroup System
 */

/*
 * Define for power-off
 */ 

#define  _reg_WDOG_WCR    ((volatile __u16 *)( IO_ADDRESS(WDOG1_BASE_ADDR + 0x00)))  



extern void mxc_map_io(void);
extern void mxc_init_irq(void);
extern void mxc_cpu_init(void) __init;
extern void mxc_gpio_init(void) __init;
extern struct sys_timer mxc_timer;
extern void mxc_cpu_common_init(void);

static char command_line[COMMAND_LINE_SIZE];

#ifdef CONFIG_MOT_FEAT_FX2LP_I2C
extern int start_fx2lp_fw_dl(void) __init;
#endif

/*!
 * Board specific fixup function. It is called by \b setup_arch() in 
 * setup.c file very early on during kernel starts. It allows the user to 
 * statically fill in the proper values for the passed-in parameters. None of 
 * the parameters is used currently.
 * 
 * @param  desc         pointer to \b struct \b machine_desc
 * @param  tags         pointer to \b struct \b tag
 * @param  cmdline      pointer to the command line
 * @param  mi           pointer to \b struct \b meminfo
 */
static void __init fixup_mxc_board(struct machine_desc *desc, struct tag *tags,
                                   char **cmdline, struct meminfo *mi) 
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

	/* Store command line for use on mxc_board_init */
	strcpy(command_line, *cmdline);

#ifdef CONFIG_DISCONTIGMEM
        do {
                int nid;
                mi->nr_banks = MXC_NUMNODES;
                for (nid=0; nid < mi->nr_banks; nid++)
                        SET_NODE(mi, nid);
        } while (0);
#endif
}

//Default power-off for argonlv
static void argonlvbute_power_off(void)
{
	/* Set the GPIO_SIGNAL_WDOG_AP to LOW to power off */
        gpio_signal_set_data(GPIO_SIGNAL_WDOG_AP, GPIO_DATA_LOW);
}




/*!
 * This function is used to enable 26 mhz clock on CKO1. This pad
 * is connected to MC13783 in order to derive all clocks
 * needed to make sound work (bit clock and frame sync clock)
 */
static void mxc_init_pmic_clock(void)
{
	mxc_ccm_modify_reg(MXC_CCM_COSR,
			   (MXC_CCM_COSR_CKO1EN | MXC_CCM_COSR_CKO1S_MASK |
			    MXC_CCM_COSR_CKO1DV_MASK),
			   MXC_CCM_COSR_CKO1EN | 0x01);
}

/*!
 * Board specific initialization.
 */
static void __init mxc_board_init(void)
{
        pm_power_off = argonlvbute_power_off;
 
        mxc_cpu_common_init();

	/* Enable 26 mhz clock on CKO1 for MC13783 audio */
	mxc_init_pmic_clock();
    
        mxc_gpio_init();
        argonlvphone_gpio_init();

#ifdef CONFIG_MOT_FEAT_FX2LP_I2C
        if(start_fx2lp_fw_dl()) {
                printk(KERN_ERR "FX2LP firmware download over i2c failed!");
        }
        else {
                printk("FX2LP firmware download completed");
        }
#endif
}

/*
 * The following uses standard kernel macros define in arch.h in order to 
 * initialize __mach_desc_ARGONLVPHONE data structure.
 */
MACHINE_START(ARGONLVPHONE, "Motorola Product - ArgonLV Phone")
        MAINTAINER("Motorola, Inc.")
        /*       physical memory    physical IO        virtual IO     */
        BOOT_MEM(PHYS_OFFSET_ASM, AIPS1_BASE_ADDR, AIPS1_BASE_ADDR_VIRT)
        BOOT_PARAMS(PHYS_OFFSET_ASM + 0x100)
        FIXUP(fixup_mxc_board)
        MAPIO(mxc_map_io)
        INITIRQ(mxc_init_irq)
        INIT_MACHINE(mxc_board_init)
        .timer = &mxc_timer,
MACHINE_END
