/*
 *  Copyright (C) 1999 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
 *  Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 *  
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
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  -----------------------------------------------------
 * 07/27/2006   Motorola  System reboot via CRM module
 * 03/22/2007   Motorola  Add arch_idle at reset
 * 05/07/2007   Motorola  Implement reboot via Atlas RTC alarm.
 * 08/29/2007   Motorola  Correct Atlas Backup Memory register access for soft reset.
 *
 */

#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <asm/system.h>

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

#define CRM_CONTROL                     IO_ADDRESS(DSM_BASE_ADDR + 0x50)
#define LPMD1                           0x00000002
#define LPMD0                           0x00000001

#if defined(CONFIG_MOT_FEAT_SYSREBOOT_ATLAS)

#include <linux/power_ic_kernel.h>

/* defined in scma11phone.c */
extern void __mxc_power_off(void);

#elif defined(CONFIG_MOT_FEAT_SYSREBOOT_CRM)

#define CRM_AP_AMCR                     0x48
#define CRM_AP_AMCR_SW_AP               0x00004000

#endif  /* CONFIG_MOT_FEAT_SYSREBOOT_CRM */
           

/*!
 * This function puts the CPU into idle mode. It is called by default_idle()
 * in process.c file.
 */
void arch_idle(void)
{
	unsigned long crm_ctrl;

	/*
	 * This should do all the clock switching
	 * and wait for interrupt tricks.
	 */
	if ((__raw_readl(AVIC_VECTOR) & MXC_WFI_ENABLE) != 0) {
		crm_ctrl = (__raw_readl(CRM_CONTROL) & ~(LPMD1)) | (LPMD0);
		__raw_writel(crm_ctrl, CRM_CONTROL);
		cpu_do_idle();
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
        unsigned long amcr;
	unsigned long crm_ap_amcr = IO_ADDRESS(CRM_AP_BASE_ADDR + CRM_AP_AMCR);
#endif  /* CONFIG_MOT_FEAT_SYSREBOOT_CRM */
    
	if (system_rev == CHIP_REV_2_0) {
		/* Workaround reset problem on PASS2 by manually reset WEIM */
		__raw_writel(0x00001E00, IO_ADDRESS(WEIM_CTRL_CS0 + CSCRU));
		__raw_writel(0x20000D01, IO_ADDRESS(WEIM_CTRL_CS0 + CSCRL));
		__raw_writel(0x0, IO_ADDRESS(WEIM_CTRL_CS0 + CSCRA));
	}

#if defined(CONFIG_MOT_FEAT_SYSREBOOT_ATLAS)
        /*
         * Set bit 4 of the Atlas MEMA register. This bit indicates to the
         * MBM that the phone is to be reset.
         */
        retval = power_ic_backup_memory_read(POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_SOFT_RESET,
                &mema);
        if(retval != 0) {
            step = 1;
            goto die;
        }

        mema |= 1;

        retval = power_ic_backup_memory_write(POWER_IC_BACKUP_MEMORY_ID_ATLAS_BACKUP_SOFT_RESET,
                mema);
        if(retval != 0) {
            step = 2;
            goto die;
        }

        /*
         * Set the Atlas RTC alarm to go off in 2 seconds. Experience indicates
         * that values less than 2 may not work reliably.
         */
        retval = power_ic_rtc_get_time(&tv);
        if(retval != 0) {
            step = 3;
            goto die;
        }

        tv.tv_sec += 2;

        retval = power_ic_rtc_set_time_alarm(&tv);
        if(retval != 0) {
            step = 4;
            goto die;
        }

        /* power down the system */
        __mxc_power_off();

die:
        printk("recieved error %d from power_ic driver at step %d in reboot",
                retval, step);
        
        /* fall through to existing reset code */
#elif defined(CONFIG_MOT_FEAT_SYSREBOOT_CRM)
       /* 
	*  Need to Reboot via Clock Reset Module with Software reset 
	*/
        amcr = readl( crm_ap_amcr );
        amcr &= ~CRM_AP_AMCR_SW_AP; 
	writel( amcr, crm_ap_amcr );
		
	while(1) {
	  /* stay here, software reset */
		arch_idle();	
	}
#endif /* CONFIG_MOT_FEAT_SYSREBOOT_CRM */
	if ((__raw_readw(IO_ADDRESS(WDOG1_BASE_ADDR)) & 0x4) != 0) {
		asm("cpsid iaf");
		while (1) {
		}
	} else {
		__raw_writew(__raw_readw(IO_ADDRESS(WDOG1_BASE_ADDR)) | 0x4,
			     IO_ADDRESS(WDOG1_BASE_ADDR));
	}
}
