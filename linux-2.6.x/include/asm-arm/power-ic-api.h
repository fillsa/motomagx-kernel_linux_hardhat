/*
 *  linux/include/asm-arm/power-ic-api.h
 *
 * Copyright (C) 2007 Motorola, Inc.
 *
 * This file implements an abstraction to power ic APIs so the
 * power ic driver can be modularized.
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

/* Date         Author      Comment
 * ===========  ==========  ==================================================
 * 02/16/2007   Motorola    Initial revision.
 * 05/30/2007   Motorola    Added some more function pointers
 * 07/03/2007   Motorola    Modify the enum value.
 * 10/28/2007   Motorola    Align enum values with power ic changes
 */

#ifndef __ASM_ARM_POWER_IC_API_H
#define __ASM_ARM_POWER_IC_API_H

#ifdef CONFIG_MOT_FEAT_POWER_IC_API
#include <asm/bug.h>
#include <linux/time.h>


typedef enum
{
    KERNEL_BACKUP_MEMORY_ID_FLASH_MODE,
    KERNEL_BACKUP_MEMORY_ID_PANIC,
    KERNEL_BACKUP_MEMORY_ID_FOTA_MODE,
    KERNEL_BACKUP_MEMORY_ID_FLASH_FAIL,
    KERNEL_BACKUP_MEMORY_ID_SOFT_RESET,
    KERNEL_BACKUP_MEMORY_ID_WDOG_RESET,
    KERNEL_BACKUP_MEMORY_ID_ROOTFS_SEC_FAIL,
    KERNEL_BACKUP_MEMORY_ID_LANG_PACK_SEC_FAIL,
    KERNEL_BACKUP_MEMORY_ID_NUM_ELEMENTS
} KERNEL_BACKUP_MEMORY_ID_T;

typedef enum
{
    KERNEL_BACKLIGHT_DISPLAY = 1
} KERNEL_BACKLIGHTS_T;


extern int (*__power_ic_rtc_get_time)(struct timeval * );
extern int (*__power_ic_rtc_set_time_alarm)(struct timeval * );
extern int (*__power_ic_backup_memory_read)(KERNEL_BACKUP_MEMORY_ID_T , int* );
extern int (*__power_ic_backup_memory_write)(KERNEL_BACKUP_MEMORY_ID_T , int );
extern int (*__power_ic_is_charger_attached)(void);
extern void (*__power_ic_backlightset)(KERNEL_BACKLIGHTS_T, unsigned char );

static void inline set_kernel_power_ic_rtc_get_time(int (*rtc_get_time)(struct timeval * )) {
    __power_ic_rtc_get_time = rtc_get_time;
}


static int inline kernel_power_ic_rtc_get_time(struct timeval * tv) {
    
    memset(tv, 0, sizeof(struct timeval));
    
    BUG_ON(__power_ic_rtc_get_time == NULL); 
    return __power_ic_rtc_get_time(tv);
}

static void inline set_kernel_power_ic_rtc_set_time_alarm(int (*rtc_set_time_alarm)(struct timeval * )) {
    __power_ic_rtc_set_time_alarm = rtc_set_time_alarm;
}


static int inline kernel_power_ic_rtc_set_time_alarm(struct timeval * tv) {
    
    
    BUG_ON(__power_ic_rtc_set_time_alarm == NULL); 
    return __power_ic_rtc_set_time_alarm(tv);
}


static void inline set_kernel_power_ic_backup_memory_read(int (*backup_memory_read)(KERNEL_BACKUP_MEMORY_ID_T , int* ) ) {
    __power_ic_backup_memory_read = backup_memory_read;
}


static int inline kernel_power_ic_backup_memory_read(KERNEL_BACKUP_MEMORY_ID_T id, int* value) {
    BUG_ON(__power_ic_backup_memory_read == NULL);
    return __power_ic_backup_memory_read(id, value);
}


static void inline set_kernel_power_ic_backup_memory_write(int (*backup_memory_write)(KERNEL_BACKUP_MEMORY_ID_T , int ) ) {
    __power_ic_backup_memory_write = backup_memory_write;
}


static int inline kernel_power_ic_backup_memory_write(KERNEL_BACKUP_MEMORY_ID_T id, int value) {
    BUG_ON(__power_ic_backup_memory_write == NULL);
    return __power_ic_backup_memory_write(id, value);
}



static void inline set_kernel_power_ic_is_charger_attached(int (*is_charger_attached)(void)) {
    __power_ic_is_charger_attached = is_charger_attached;
}


static int inline kernel_power_ic_is_charger_attached(void) {
    BUG_ON(__power_ic_is_charger_attached == NULL);
    return __power_ic_is_charger_attached();
}



static void inline set_kernel_power_ic_backlightset(void (*display_backlightset)(KERNEL_BACKLIGHTS_T, unsigned char)) {
    __power_ic_backlightset = display_backlightset;
}


static void inline kernel_power_ic_backlightset(KERNEL_BACKLIGHTS_T bl, unsigned char value) {
    BUG_ON(__power_ic_backlightset == NULL);
    __power_ic_backlightset(bl, value);
}

#endif  /* CONFIG_MOT_FEAT_POWER_IC_API */

#endif  /* __ASM_ARM_POWER_IC_API_H */
