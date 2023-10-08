/*
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 *  Copyright (C) 2007, 2008 Motorola, Inc.
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
 * Changelog:
 *
 * Date         Author      Comment
 * ==========   ==========  ================================================
 * 02/06/2007   Motorola    Modified arch_reset so that it will use
 *                          software reset to reboot the phone.
 * 10/31/2007   Motorola    Integrate the patch for removing WFI SW workaround.
 * 11/14/2007   Motorola    Add fix for errata issues.
 * 04/22/2008   Motorola    Add fix of the keywest reboot issue.
 * 21/05/2008   Motorola    Add ATLAS reboot mode.
 *
 */

#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/system.h>
#include "crm_regs.h"

#ifdef CONFIG_MOT_FEAT_PM
#include <asm/arch/mxc_pm.h>
#endif

#include <asm/mot-gpio.h>

#if defined(CONFIG_MOT_FEAT_SYSREBOOT_ATLAS)
#include <asm/power-ic-api.h>
#endif

/*!
 * @defgroup MSL Machine Specific Layer (MSL)
 */

/*!
 * @defgroup System System-wide Misc Files for MSL
 * @ingroup MSL
 */

/*!
 * @file system.h
 * @brief This file contains idle and reset functions.
 *
 * @ingroup System
 */

#define AVIC_NIPNDH             IO_ADDRESS(AVIC_BASE_ADDR + 0x58)
#define AVIC_NIPNDL             IO_ADDRESS(AVIC_BASE_ADDR + 0x5c)
#define LO_INTR_EN              IO_ADDRESS(AVIC_BASE_ADDR + 0x14)

#ifdef  CONFIG_MOT_FEAT_SYSREBOOT
#define WCR_SRS_BIT		(1 << 4)
#endif

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
#ifdef CONFIG_MOT_FEAT_PM
	unsigned long mpdr0;
#endif
	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks.
	 */
	if ((__raw_readl(AVIC_VECTOR) & MXC_WFI_ENABLE) != 0) {
		writel((readl(MXC_CCM_MCR) | MXC_CCM_MCR_LPM_1), MXC_CCM_MCR);
		/* WFI */
#ifdef CONFIG_MOT_FEAT_PM
                mpdr0 = mxc_pm_l1_instr_cache_corruption_fix_before_wfi();
#endif

		cpu_do_idle();
#ifdef CONFIG_MOT_FEAT_PM
                mxc_pm_l1_instr_cache_corruption_fix_after_wfi(mpdr0);
#endif

	}
}

/*
 * This function resets the system. It is called by machine_restart().
 *
 * @param  mode         indicates different kinds of resets
 */
void arch_reset(char mode)
{
#if defined(CONFIG_MOT_FEAT_SYSREBOOT_ATLAS)
        int retval, mema, step = 0;
        struct timeval tv;
#elif defined(CONFIG_MOT_FEAT_SYSREBOOT_CRM)
        unsigned long ccmr;
#endif  

#if defined(CONFIG_MOT_FEAT_SYSREBOOT_ATLAS)
        /*
         * Set bit 4 of the Atlas MEMA register. This bit indicates to the
         * MBM that the phone is to be reset.
         */
        retval = kernel_power_ic_backup_memory_read(KERNEL_BACKUP_MEMORY_ID_SOFT_RESET, &mema);
        if(retval != 0) {
            step = 1;
            goto die;
        }

        mema |= 1;

        retval = kernel_power_ic_backup_memory_write(KERNEL_BACKUP_MEMORY_ID_SOFT_RESET, mema);
        if(retval != 0) {
            step = 2;
            goto die;
        }

        /*
         * Set the Atlas RTC alarm to go off in 2 seconds. Experience indicates
         * that values less than 2 may not work reliably.
         */
        retval = kernel_power_ic_rtc_get_time(&tv);
        if(retval != 0) {
            step = 3;
            goto die;
        }

        tv.tv_sec += 2;

        retval = kernel_power_ic_rtc_set_time_alarm(&tv);
        if(retval != 0) {
            step = 4;
            goto die;
        }

        /* power down the system */
	 gpio_signal_set_data(GPIO_SIGNAL_WDOG_AP, GPIO_DATA_LOW);
die:
        printk("recieved error %d from power_ic driver at step %d in reboot",
                retval, step);

        /* fall through to existing reset code */
#elif defined(CONFIG_MOT_FEAT_SYSREBOOT_CRM)
       /*
        *  Need to Reboot via Clock Reset Module with Software reset
        */
	ccmr = __raw_readl(MXC_CCM_RCSR);
	ccmr |= MXC_CCM_RCSR_NF16B;
	__raw_writel(ccmr, MXC_CCM_RCSR);
	
	while(1) {
          /* stay here, software reset */
                arch_idle();
        }
#endif 

	/* The GPS_RESET line is tied to "Boot Pin 1" on the ArgonLVi.
         * If this bit is high level, will cause software reboot hang up.
	 */
	 gpio_signal_set_data(GPIO_SIGNAL_GPS_RESET, GPIO_DATA_LOW);

#ifdef  CONFIG_MOT_FEAT_SYSREBOOT
	/* Write 0 to the WDOG WCR SRS bit to reset the ArgonLV */
	__raw_writew(__raw_readw(IO_ADDRESS(WDOG1_BASE_ADDR)) & ~WCR_SRS_BIT, 
		IO_ADDRESS(WDOG1_BASE_ADDR));
#else
	if ((__raw_readw(IO_ADDRESS(WDOG1_BASE_ADDR)) & 0x4) != 0) {
		asm("cpsid iaf");
		while (1) {
		}
	} else {
		__raw_writew(__raw_readw(IO_ADDRESS(WDOG1_BASE_ADDR)) | 0x4,
			     IO_ADDRESS(WDOG1_BASE_ADDR));
	}
#endif
}
