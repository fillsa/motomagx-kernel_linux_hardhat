/*
 * Copyright (C) 2004-2008 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  
 * 02111-1307, USA
 *
 * Motorola 2008-Jan-29 - Add support for xPIXL
 * Motorola 2007-Dec-06 - Support TF voltage disable for Pico. 
 * Motorola 2007-Jan-25 - Export transflash voltage interface. 
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Nov-27 - Transflash voltage support for Pico.
 * Motorola 2006-Nov-10 - Add support for Marco.
 * Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Aug-04 - Turn off transflash power when card not in use
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2005-Mar-25 - Update doxygen documentation
 * Motorola 2004-Dec-17 - Design of interfaces to control power-related functions.
 */

/*!
 * @file power_management.c
 *
 * @ingroup poweric_power_management
 *
 * @brief This module provides interfaces to control various power-related
 * functions, (e.g. changing the processor core voltage).
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <asm/delay.h>
#include <linux/errno.h>
#include <asm/boardrev.h>
#include "../core/os_independent.h"

/*******************************************************************************************
 * CONSTANTS
 ******************************************************************************************/
/*! The number of settings available for the VGEN regulator supplying the transflash. */
#define ATLAS_VGEN_ARRAY_SIZE           8

/*! The number of settings available for the VMMC1 regulator supplying the transflash. */
#define ATLAS_VMMC1_ARRAY_SIZE          8

/*!
 * Since the supplies have a +/-3% error, if a voltage being requested is at the lower
 * usable limit, an offset should be added to make sure a usable voltage is supplied (in mV).
 */
#define SUPPLY_ERROR_OFFSET             75

/*! The software must delay for this duration after changing the transflash voltage (in microsecs). */
#define TFLASH_STABILIZING_DELAY        1000

/*******************************************************************************************
 * TYPES
 ******************************************************************************************/
/*!
 * The different output voltages possible for VGEN on Atlas.
 */
static const struct atlas_settings_struct
{
    unsigned int mv;
    int reg_val;
} atlas_vgen_settings[ATLAS_VGEN_ARRAY_SIZE] =
{ 
    {1100, 4}, {1200, 0}, {1300, 1}, {1500, 2},
    {1800, 3}, {2000, 5}, {2400, 7}, {2775, 6}
};

 
 /*!
 * The different output voltages possible for VMMC1 on Atlas.
 */
static const unsigned int atlas_vmmc1_settings[ATLAS_VMMC1_ARRAY_SIZE] =
{ 
    1600, 1800, 2000, 2600, 2700, 2800, 2900, 3000
};

/*******************************************************************************************
 * EXPORTED SYMBOLS
 ******************************************************************************************/
EXPORT_SYMBOL(power_ic_set_transflash_voltage);

/*******************************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************************/

/*!
 * This function is called by the transflash block driver to adjust the transflash voltage.
 *
 * The voltage will be set as close as possible to the passed in value.  If possible, the voltage
 * will be at least the value requested, but if the supply is unable to supply that voltage,
 * the highest voltage it can provide will be selected.
 *
 * @param   millivolts   The desired setting for the transflash supply
 *
 * @return  Voltage in millivolts that regulator was actually set to.
 *
 * @note    This function does not turn supply off for Elba, Pico, or SCMA11 reference boards
 *          because the SDHC needs to be powered up for detection.
 *
 * @note    This function supports Pico, Ascension P2, Elba P0, Bute P4 and SCMA11 reference boards P2 and
 *          later.  Other platforms, products, and hardware revisions are not supported.
 */
int power_ic_set_transflash_voltage(unsigned int millivolts)
{
    int index = 0;

    if (millivolts == 0)
    {
#if defined(CONFIG_MACH_ASCENSION) || defined(CONFIG_MACH_MARCO) || defined(CONFIG_MACH_PICO) || defined(CONFIG_MACH_XPIXL)
        tracemsg(_k_d("power_ic_set_transflash_voltage: TransFlash powered OFF"));
        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_REG_MODE_0, 12, 0);
#elif defined(CONFIG_ARCH_MXC91321)
#if defined(CONFIG_MACH_BUTEREF)
        if(boardrev() >= BOARDREV_P4A)
        {
            tracemsg(_k_d("power_ic_set_transflash_voltage: TransFlash powered OFF"));
            power_ic_set_reg_bit(POWER_IC_REG_ATLAS_REG_MODE_1, 18, 0);
        }
#else /* Non-BUTEREF ArgonLV Products */
        tracemsg(_k_d("power_ic_set_transflash_voltage: TransFlash powered OFF"));
        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_REG_MODE_1, 18, 0);
#endif /* BUTEREF */
#endif /* MXC91321 */

        return 0;
    }

    millivolts += SUPPLY_ERROR_OFFSET;

#if defined(CONFIG_MACH_ASCENSION) || defined(CONFIG_MACH_SCMA11REF) || defined(CONFIG_MACH_ELBA) \
    || defined(CONFIG_MACH_MARCO) || defined(CONFIG_MACH_PICO) || defined(CONFIG_MACH_XPIXL)
# ifdef CONFIG_MACH_SCMA11REF
    if(boardrev() >= BOARDREV_P2AW)
# endif
    {
        /*
         * ATLAS_VGEN_ARRAY_SIZE-1 is used to make sure we never let index be larger than the
         * table size.  This way, we can use index without any further checks.
         */
        while ((index < ATLAS_VGEN_ARRAY_SIZE-1) && (millivolts > atlas_vgen_settings[index].mv))
        {
            index++;
        }

        /* Write determined voltage setting to power ic */
        power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_SET_0, 6, atlas_vgen_settings[index].reg_val, 3);

        /* Confirm regulator is enabled */
        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_REG_MODE_0, 12, 1);

        tracemsg(_k_d("power_ic_set_transflash_voltage: TransFlash powered ON at %d millivolts"),
                 atlas_vgen_settings[index].mv);

        /*
         * After turning on the output of the transflash regulator the function will wait for the
         * supply to stabilze before reporting that the adjustment has been made.
         */
        udelay (TFLASH_STABILIZING_DELAY);

        return atlas_vgen_settings[index].mv;
    }

#elif defined(CONFIG_ARCH_MXC91321)
#if defined(CONFIG_MACH_BUTEREF)
    if(boardrev() >= BOARDREV_P4A)
#endif /* BUTEREF */
    {
        /*
         * ATLAS_VMMC1_ARRAY_SIZE-1 is used to make sure we never let index be larger than the
         * table size.  This way, we can use index without any further checks.
         */
        while ((index < ATLAS_VMMC1_ARRAY_SIZE-1) && (millivolts > atlas_vmmc1_settings[index]))
        {
            index++;
        }

        /* Write determined voltage setting to power ic */
        power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_SET_1, 6, index, 3);

        /* Confirm regulator is enabled */
        power_ic_set_reg_bit(POWER_IC_REG_ATLAS_REG_MODE_1, 18, 1);

        tracemsg(_k_d("power_ic_set_transflash_voltage: TransFlash powered ON at %d millivolts"),
                 atlas_vmmc1_settings[index]);

        /*
         * After turning on the output of the transflash regulator the function will wait for the
         * supply to stabilze before reporting that the adjustment has been made.
         */
        udelay (TFLASH_STABILIZING_DELAY);

        return atlas_vmmc1_settings[index];
    }
#endif /* MXC91321 */

    return index;
}
