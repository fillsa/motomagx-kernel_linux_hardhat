/*
 *  linux/arch/arm/kernel/power-ic-api.c
 *
 *           Copyright Motorola 2007
 *
 * This file implements an abtration to power ic APIs so the
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
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ------------------------
 * 02/16/2007   Motorola  Support for power ic module APIs in core kernel
 * 05/30/2007   Motorola  Added couple more power ic module APIs 
 *
 */


#include <linux/module.h>
#include <asm/power-ic-api.h>

int (*__power_ic_rtc_get_time)(struct timeval * ) = NULL;
EXPORT_SYMBOL(__power_ic_rtc_get_time);

int (*__power_ic_rtc_set_time_alarm)(struct timeval * ) = NULL;
EXPORT_SYMBOL(__power_ic_rtc_set_time_alarm);

int (*__power_ic_backup_memory_read)(KERNEL_BACKUP_MEMORY_ID_T , int*) = NULL;
EXPORT_SYMBOL(__power_ic_backup_memory_read);

int (*__power_ic_backup_memory_write)(KERNEL_BACKUP_MEMORY_ID_T , int ) = NULL;
EXPORT_SYMBOL(__power_ic_backup_memory_write);

int (*__power_ic_is_charger_attached)(void) = NULL;
EXPORT_SYMBOL(__power_ic_is_charger_attached);

void (*__power_ic_backlightset)(KERNEL_BACKLIGHTS_T, unsigned char ) = NULL;
EXPORT_SYMBOL(__power_ic_backlightset);






