/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2004-2008 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2008-Apr-22 - Support shared led region
 * Motorola 2008-Feb-13 - Add support for Nevis.
 * Motorola 2008-Jan-29 - Add support for xPixl.
 * Motorola 2007-Jul-09 - Do not initialize the power path bits on init.
 * Motorola 2007-Jun-22 - Remember register initialization values after init.
 * Motorola 2007-Jun-19 - NAND secure boot time improvement 
 * Motorola 2007-Apr-23 - Do not set Power Cut related bits on init.
 * Motorola 2007-Feb-19 - Move fl_initialize call to lights_init
 * Motorola 2007-Jan-25 - Add support for power management
 * Motorola 2007-Jan-16 - Add support for MAX6946 LED controller chip.
 * Motorola 2006-Nov-20 - Update File
 * Motorola 2006-Nov-10 - Add support for Marco.
 * Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
 * Motorola 2006-Oct-06 - Update File
 * Motorola 2006-Sep-25 - Add support for MAX7314 LED controller chip.
 * Motorola 2006-Sep-20 - Support Pico product
 * Motorola 2006-Aug-16 - Add TCMD support for BUTE
 * Motorola 2006-Aug-15 - Add Elba support.
 * Motorola 2006-Aug-13 - Add SCMA11 P3BW init values
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-31 - One wire module support for ArgonLV
 * Motorola 2006-Jul-25 - Add Saipan init support
 * Motorola 2006-Jul-07 - Moved SPI init tables to seperate header files
 * Motorola 2006-Jul-26 - Modify BUTE P4A init
 * Motorola 2006-Jul-25 - Add SCMA11 P2A init values
 * Motorola 2006-Jul-25 - Change default transflash voltage
 * Motorola 2006-Jun-22 - Move location of atlas_spi_inter.h
 * Motorola 2006-Jul-14 - Dim the blue light
 * Motorola 2006-Jun-22 - Move location of atlas_spi_inter.h
 * Motorola 2006-Jun-19 - Fix montavista upmerge conditionals
 * Motorola 2006-Jul-14 - Add Lido & Saipan Support
 * Motorola 2006-Jul-11 - Remove setting of GPO for Thermistor from power up
 * Motorola 2006-Jun-20 - Changes for SCMA11 P3A
 * Mororola 2006-May-26 - Reverse the order of lighting initialize
 * Motorola 2006-May-17 - Separate SW2A and SW2B for BUTE hardware above P4AW and P5A
 * Motorola 2006-May-15 - Remove duplicate include of init.h
 * Motorola 2006-May-11 - Added case to initialize the rest of the SPI registers for P2
 * Motorola 2006-May-06 - Implement EMU Common Algorithm
 * Motorola 2006-May-05 - Modified the initial value of Atlas LED register value.
 * Motorola 2006-May-03 - Enable VMMC2 for BT for Ascension
 * Motorola 2006-May-03 - Changes for SCMA11 P2A wingboard
 * Motorola 2006-Apr-26 - Check for root permissions before register access ioctls.
 * Motorola 2006-Apr-11 - Enable thermistor bias for SCM-A11, Ascension, and Bute.
 * Motorola 2006-Apr-07 - Add in initialization for Ascension P2
 * Motorola 2006-Apr-05 - Change Factory cable FET bits to 11
 * Motorola 2006-Apr-03 - Add in Bute 4 initialization values
 * Motorola 2006-Feb-24 - Modification to bit settings for ATLAS power up
 * Motorola 2006-Feb-17 - Modification to default setting for the main backlight
 * Motorola 2006-Feb-10 - Add ArgonLV support
 * Motorola 2006-Feb-09 - Modification to ATLAS init values for Ascension P1 hardware change
 * Motorola 2006-Feb-06 - Add battery rom support for SCM-A11
 * Motorola 2006-Feb-03 - Enable CHRGRAWDIV bit in ADC 0 register.
 * Motorola 2006-Jan-12 - Add in ATLAS init values for Ascension and lights_init() call
 * Motorola 2006-Jan-10 - Finalize the design of the core functions for power ic.
 * Motorola 2005-May-12 - Add TCMD support
 * Motorola 2005-Mar-25 - Add Atlas support
 * Motorola 2005-Feb-04 - Add Charger support
 * Motorola 2004-Dec-06 - Redesign of the core functions for power ic
 */


/*!
 * @file core.c
 *
 * @ingroup poweric_core
 *
 * @brief The main file and user-space interface to the power IC driver.
 *
 * This file includes all of the initialization and tear-down code for the power IC
 * low-level driver and the basic user-mode interface (open(), close(), ioctl(), etc.).
 *
 * @todo Does power control need to be set for software override dual-path like AUL?
 * On other hardware, allowing the hardware to control this results in ambulances
 * when a charger is attached while the battery is low.
 */

#include <linux/config.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mpm.h>
#include <linux/moto_accy.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <asm/boardrev.h>
#include <asm/arch/mc13783_spi_inter.h>

#include <linux/lights_funlights.h>

#include "os_independent.h"
#include "event.h"
#include "atlas_register.h"
#include "core.h"

#if defined(CONFIG_MACH_ASCENSION)
#include "../hdr/ascension_p2a.h"
#include "../hdr/ascension_p3a.h"
#elif defined(CONFIG_ARCH_MXC91321) // Argon
  #if defined(CONFIG_MOT_FEAT_RF_PYTHON)
  #include "../hdr/python_base.h"
  #elif defined(CONFIG_MACH_BUTEREF)
  #include "../hdr/bute_base.h"
  #include "../hdr/bute_p4a.h"
  #include "../hdr/bute_p4aw.h"
  #else
  #include "../hdr/argonref71_base.h"
  #endif
#elif defined(CONFIG_MACH_ELBA)
#include "../hdr/elba_base.h"
#elif defined(CONFIG_MACH_SAIPAN)
#include "../hdr/saipan_base.h"
#elif defined(CONFIG_MACH_LIDO)
#include "../hdr/lido_base.h"
#elif defined(CONFIG_MACH_PICO)
#include "../hdr/pico_base.h"
#elif defined(CONFIG_MACH_MARCO)
#include "../hdr/marco_base.h"
#elif defined(CONFIG_MACH_XPIXL)
#include "../hdr/xpixl_base.h"
#elif defined(CONFIG_MACH_NEVIS)
#include "../hdr/nevis_base.h"
#else
#include "../hdr/scma11_base.h"
#include "../hdr/scma11_p2aw.h"
#include "../hdr/scma11_p2a.h"
#include "../hdr/scma11_p3bw.h"
#endif

#include "../module/brt.h"
#include "../module/owire.h"

#include "../module/audio.h"
#include "../module/charger.h"
#include "../module/debounce.h"
#include "../module/emu_glue_utils.h"
#include "../module/peripheral.h"
#include "../module/rtc.h"
#include "../module/tcmd_ioctl.h"

#include "../module/atod.h"

/******************************************************************************
* Constants
******************************************************************************/

/* Masks for determining the power path. */
#define USB2V0_MASK 0x0020000
#define IDF_MASK    0x0080000
#define IDG_MASK    0x0100000
#define SE1_MASK    0x0200000

/* Mask for preventing the power cut bits from being set. */
#define PC0_POWERCUT_MASK   0x0FFFFFC

/* Mask for preventing the power path bits from being set */
#define CHRGR0_POWER_PATH_MASK 0x0FFF3FF

/******************************************************************************
* Global Variables
******************************************************************************/

unsigned long int reg_init_tbl[POWER_IC_REG_NUM_REGS_ATLAS];

/******************************************************************************
* Local Variables
******************************************************************************/

/*! Holds the power-up reason (determined at module initialization) */
static POWER_IC_POWER_UP_REASON_T power_up_reason = POWER_IC_POWER_UP_REASON_NONE;

/******************************************************************************
* Local Functions
******************************************************************************/
#ifdef CONFIG_PM
/*!
 *
 * @ brief Determines if the power IC code is busy
 *
 * This function is used by the power management to determine if the phone
 * can be suspended
 *
 * @param   dev   the device structure used to give information to suspend.
 * @param   state the power state the device is entering.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in.
 *
 * @return  0 if successful, -EBUSY if the power IC is busy
 */
static int power_ic_pm_suspend(struct device *dev, u32 state, u32 level)
{
    POWER_IC_PM_COMPONENT_T i;
        
    for (i = 0; i < POWER_IC_PM__NUM; i++)
    {
        if (power_ic_pm_suspend_mask_tbl[i] != 0)
        {
            return -EBUSY;
        }
    }
    
    return 0;
}
#else
/*!
 * @ brief Determines if the power IC code is busy
 *
 * This function is used by the power management to determine if the phone
 * can be suspended
 *
 * @param   dev   the device structure used to give information to suspend.
 * @param   state the power state the device is entering.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in.
 *
 * @return  0 if successful.
 */
static int power_ic_pm_suspend(struct device *dev, u32 state, u32 level)
{
    return 0;
}
#endif /* CONFIG_PM */

/*!
 * @ brief Not used by the power IC code
 *
 * This function is called when power management wakes up from suspend mode.
 *
 * @param   dev   the device structure used to give information to resume.
 * @param   level the stage in device resumption process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
static int power_ic_pm_resume(struct device *dev, u32 level)
{
    return 0;
}

/*! @brief  This structure contains pointers to the power management callback functions. */
static const struct device_driver power_ic_pm_sr =
{
    .name           = POWER_IC_DEV_NAME,
    .bus            = &platform_bus_type,
    .suspend        = power_ic_pm_suspend,
    .resume         = power_ic_pm_resume,
};

static const struct platform_device power_ic_pm_sr_device =
{
    .name           = POWER_IC_DEV_NAME,
    .id             = 0,
};

/*!
 * @brief determines power-up reason
 *
 * This function determines the reason that the phone powered up.  This
 * determination is done by reading the contents of the interrupt status
 * and sense registers in the power IC(s).
 *
 * @return power-up reason
 */

static POWER_IC_POWER_UP_REASON_T determine_power_up_reason (void)
{
    POWER_IC_POWER_UP_REASON_T retval = POWER_IC_POWER_UP_REASON_NONE;
    int i;
    int value;

    /* Table of power-up reasons */
    static const struct
    {
        POWER_IC_REGISTER_T reg;
        int bit;
        int value;
        POWER_IC_POWER_UP_REASON_T reason;
    } reasons[] =
    {
        /* Reasons are listed in priority order with the highest priority power-up reason first */
        { POWER_IC_REG_ATLAS_INT_SENSE_1,     3,    0, POWER_IC_POWER_UP_REASON_FIRST_POWER_KEY_LONG },
        { POWER_IC_REG_ATLAS_INT_STAT_1,      3,    1, POWER_IC_POWER_UP_REASON_FIRST_POWER_KEY_SHORT },
        { POWER_IC_REG_ATLAS_INT_SENSE_1,     4,    0, POWER_IC_POWER_UP_REASON_SECOND_POWER_KEY_LONG },
        { POWER_IC_REG_ATLAS_INT_STAT_1,      4,    1, POWER_IC_POWER_UP_REASON_SECOND_POWER_KEY_SHORT },
        { POWER_IC_REG_ATLAS_INT_SENSE_1,     5,    0, POWER_IC_POWER_UP_REASON_THIRD_POWER_KEY_LONG },
        { POWER_IC_REG_ATLAS_INT_STAT_1,      5,    1, POWER_IC_POWER_UP_REASON_THIRD_POWER_KEY_SHORT },
        { POWER_IC_REG_ATLAS_INT_STAT_1,      8,    1, POWER_IC_POWER_UP_REASON_POWER_CUT },
        { POWER_IC_REG_ATLAS_INT_STAT_1,      1,    1, POWER_IC_POWER_UP_REASON_ALARM },
        { POWER_IC_REG_ATLAS_INT_SENSE_0,     6,    1, POWER_IC_POWER_UP_REASON_CHARGER }
    };

    /* Loop through the reasons (in order) to find the power-up reason */
    for (i = 0; i < sizeof(reasons) / sizeof(reasons[0]); i++)
    {
        /* Read the bit from the power IC.  If set, we have found the reason */
        if ((power_ic_get_reg_value (reasons[i].reg, reasons[i].bit, &value, 1) == 0) &&
            (value == reasons[i].value))
        {
            retval = reasons[i].reason;
            break;
        }
    }

    /* Return the reason that we found (or will default to none if we didn't find anything) */
    return retval;
}

/*!
 * @brief Retrieves information about the hardware.
 *
 * This function populates a POWER_IC_HARDWARE_T structure with information about the hardware
 * of the phone. This includes the type of the power IC hardware (Atlas/whatever) and some
 * revision information about the hardware, if available.
 *
 * @param        info       Pointer to structure to be populated with hardware information.
 */

static void get_hardware_info(POWER_IC_HARDWARE_T * info)
{
    int revision;

    info->revision1 = POWER_IC_HARDWARE_NO_REV_INFO;
    info->revision2 = POWER_IC_HARDWARE_NO_REV_INFO;
    info->soc_bt_shared = 0;
    
#if defined(CONFIG_MOT_POWER_IC_ATLAS)
    info->chipset = POWER_IC_CHIPSET_ATLAS;

    /* Atlas has revision information available. There is currently no other identifiable
     * hardware alongside Atlas. */
    if(power_ic_read_reg(POWER_IC_REG_ATLAS_REVISION, &revision) == 0)
    {
        info->revision1 = revision;
    }

#else /* Don't know what this is. */
    info->chipset = POWER_IC_CHIPSET_UNKNOWN;
#endif
#if defined(CONFIG_MOT_FEAT_FLIP)
    info->form_factor = POWER_IC_FORM_FACTOR_FLIP;
#elif defined(CONFIG_MOT_FEAT_SLIDER)
    info->form_factor = POWER_IC_FORM_FACTOR_SLIDER;
#elif defined(CONFIG_MOT_FEAT_CANDYBAR)
    info->form_factor = POWER_IC_FORM_FACTOR_CANDYBAR;  
#else
    info->form_factor = POWER_IC_FORM_FACTOR_UNKNOWN;
#endif
//#if defined CONFIG_MACH_NEVIS
    info->soc_bt_shared = 1;
//#endif
}

/*!
 * @brief the ioctl() handler for the power IC device node
 *
 * This function implements the ioctl() system call for the power IC device node.
 * Based on the provided command, the function will pass the request on to the correct
 * power IC module for handling.  In the case that the command one of the "core" power
 * IC requests (i.e., read/write register commands), the command is handled directly
 * by calling the appropriate function (with any required user/kernel-space conversions).
 *
 * @param        inode       inode pointer
 * @param        file        file pointer
 * @param        cmd         the ioctl() command
 * @param        arg         the ioctl() argument
 *
 * @return 0 if successful
 */

static int power_ic_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    POWER_IC_REG_ACCESS_T reg_access;
    POWER_IC_HARDWARE_T hardware_info;
    POWER_IC_BACKUP_MEM_ACCESS_T bckpmem_access;
    int * usr_spc_data_ptr = NULL;
    int retval;

    /* Get the actual command from the ioctl request. */
    unsigned int cmd_num = _IOC_NR(cmd);

    if((cmd_num >= POWER_IC_IOC_CMD_CORE_BASE) && (cmd_num <= POWER_IC_IOC_CMD_CORE_LAST_CMD))
    {
        tracemsg(_k_d("core ioctl(), request 0x%X (cmd %d)"),(int) cmd, (int)cmd_num);

        /* Both backup memory functions share the same data, so handle them commonly. */
        if ((cmd == POWER_IC_IOCTL_READ_BACKUP_MEMORY) ||
            (cmd == POWER_IC_IOCTL_WRITE_BACKUP_MEMORY))
        {
            /* Fetch the data passed from user space. Access OK checking is done for us. */
            if(copy_from_user((void *)&bckpmem_access, (void *)arg, sizeof(bckpmem_access)) != 0)
            {
                tracemsg(_k_d("error copying data from user space."));
                return -EFAULT;
            }

            usr_spc_data_ptr = &(((POWER_IC_BACKUP_MEM_ACCESS_T *)arg)->value);
        }
        /* All of the register access functions share the same data, so handle it commonly */
        else if ((cmd != POWER_IC_IOCTL_GET_POWER_UP_REASON) &&
            (cmd != POWER_IC_IOCTL_GET_HARDWARE_INFO)   &&
            (cmd != POWER_IC_IOCTL_GET_POWER_UP_REASON))
        {
            /* Fetch the data passed from user space. Access OK checking is done for us. */
            if(copy_from_user((void *)&reg_access, (void *)arg, sizeof(reg_access)) != 0)
            {
                tracemsg(_k_d("error copying data from user space."));
                return -EFAULT;
            }

            usr_spc_data_ptr = &(((POWER_IC_REG_ACCESS_T *)arg)->value);
        }

        /* Handle the request. */
        switch(cmd)
        {
            case POWER_IC_IOCTL_READ_REG:
                /* Read the entire register and pass the data back the the caller in user space. */
                tracemsg(_k_d("=> Read register %d"), reg_access.reg);

                retval = power_ic_read_reg(reg_access.reg, &(reg_access.value));
                if (retval != 0)
                {
                    return retval;
                }

                /* Only the value read needs to be sent back to the caller. */
                if(put_user(reg_access.value, usr_spc_data_ptr) != 0)
                {
                    tracemsg(_k_d("error copying read value to user space."));
                    return -EFAULT;
                }
                break;

            case POWER_IC_IOCTL_WRITE_REG:
                /* Check for root permission before allowing write access to registers. */
                if (! capable(CAP_SYS_ADMIN))
                {
                    return -EPERM;
                }

                /* Write the entire register with the data provided. No data needs to be passed
                 * back to user-space. */
                tracemsg(_k_d("=> Write register %d, value 0x%X"), reg_access.reg, reg_access.value);

                retval = power_ic_write_reg(reg_access.reg, &(reg_access.value));
                if (retval != 0)
                {
                    return retval;
                }

                break;

            case POWER_IC_IOCTL_READ_REG_BITS:
                /* Read the bits requested and pass the data back the the caller in user space. */
                tracemsg(_k_d("=> Read register bits - reg %d, index %d, num bits %d"),
                               reg_access.reg, reg_access.index, reg_access.num_bits);

                retval = power_ic_get_reg_value(reg_access.reg, reg_access.index,
                                                &(reg_access.value), reg_access.num_bits);
                if (retval != 0)
                {
                    return retval;
                }

                /* Only the value read needs to be sent back to the caller. */
                if(put_user(reg_access.value, usr_spc_data_ptr) != 0)
                {
                    tracemsg(_k_d("error copying read bits to user space."));
                    return -EFAULT;
                }
                break;

            case POWER_IC_IOCTL_WRITE_REG_BITS:
                /* Check for root permission before allowing write access to registers. */
                if (! capable(CAP_SYS_ADMIN))
                {
                    return -EPERM;
                }

                /* Write the bits specified. No data needs to be passed back to user-space. */
                tracemsg(_k_d("=> Write register bits - reg %d, index %d, num bits %d, value 0x%X"),
                               reg_access.reg, reg_access.index,
                               reg_access.num_bits, reg_access.value);

                retval = power_ic_set_reg_value(reg_access.reg, reg_access.index,
                                                reg_access.value, reg_access.num_bits);
                if (retval != 0)
                {
                    return retval;
                }

                break;

            case POWER_IC_IOCTL_WRITE_REG_MASK:
                /* Check for root permission before allowing write access to registers. */
                if (! capable(CAP_SYS_ADMIN))
                {
                    return -EPERM;
                }

                /* Write the data specified. No data needs to be passed back to user-space. */
                tracemsg(_k_d("=> Write register mask - reg %d, mask %d, value 0x%X"),
                               reg_access.reg, reg_access.index, reg_access.value);

                retval = power_ic_set_reg_mask(reg_access.reg, reg_access.index, reg_access.value);
                if (retval != 0)
                {
                    return retval;
                }

                break;

            case POWER_IC_IOCTL_GET_POWER_UP_REASON:
                /* Copy the power-up reason to user-space */
                retval = copy_to_user ((void *)arg, (void *)&power_up_reason,
                    sizeof(power_up_reason));

                /* If the copy failed, return an error */
                if (retval != 0)
                {
                    return -EFAULT;
                }

                break;

            case POWER_IC_IOCTL_GET_HARDWARE_INFO:
                /* The caller wants to know about the hardware. Go get the necessary information. */
                get_hardware_info(&hardware_info);

                /* Copy the hardware info over to user-space */
                retval = copy_to_user ((void *)arg, (void *)&hardware_info,
                    sizeof(hardware_info));

                /* If the copy failed, return an error */
                if (retval != 0)
                {
                    return -EFAULT;
                }

                break;

            case POWER_IC_IOCTL_READ_BACKUP_MEMORY:
                /* Read the bits requested and pass the data back to the caller in user space. */
                tracemsg(_k_d("=> Read backup memory bits - id %d"),
                               bckpmem_access.id);

                retval = power_ic_backup_memory_read_sec(bckpmem_access.id,
                                                         &(bckpmem_access.value),
                                                         POWER_IC_USER_USER_SPACE);
                if (retval != 0)
                {
                    return retval;
                }

                /* Only the value read needs to be sent back to the caller. */
                if(put_user(bckpmem_access.value, usr_spc_data_ptr) != 0)
                {
                    tracemsg(_k_d("error copying read bits to user space."));
                    return -EFAULT;
                }
                break;

            case POWER_IC_IOCTL_WRITE_BACKUP_MEMORY:
                /* Write the bits specified. No data needs to be passed back to user-space. */
                tracemsg(_k_d("=> Write backup memory bits - id %d, value 0x%X"),
                               bckpmem_access.id, bckpmem_access.value);

                retval = power_ic_backup_memory_write_sec(bckpmem_access.id,
                                                          bckpmem_access.value,
                                                          POWER_IC_USER_USER_SPACE);

                if (retval != 0)
                {
                    return retval;
                }

                break;

            default: /* This shouldn't be able to happen, but just in case... */
                tracemsg(_k_d("0x%X unsupported ioctl command"), (int) cmd);
                return -ENOTTY;
                break;
        }
    }

    /* Is this a request for the RTC interface? */
    else if((cmd_num >= POWER_IC_IOC_RTC_BASE) && (cmd_num <= POWER_IC_IOC_RTC_LAST_CMD))
    {
        return rtc_ioctl(cmd, arg);
    }

    /* Is this a request for the peripheral interface? */
    else if((cmd_num >= POWER_IC_IOC_CMD_PERIPH_BASE) && (cmd_num <= POWER_IC_IO_CMD_PERIPH_LAST_CMD))
    {
        return periph_ioctl(cmd, arg);
    }

    /*audio interface module*/
    else if((cmd_num >= POWER_IC_IOC_CMD_AUDIO_BASE) && (cmd_num <= POWER_IC_IOC_CMD_AUDIO_LAST_CMD))
    {
        return audio_ioctl(cmd,arg);
    }
    /* Is this a request for the AtoD interface? */
    else if((cmd_num >= POWER_IC_IOC_ATOD_BASE) && (cmd_num <= POWER_IC_IOC_ATOD_LAST_CMD))
    {
        return atod_ioctl(cmd, arg);
    }
#if defined(CONFIG_ARCH_MXC91231)
    /* Lights request */
    else if((cmd_num >= POWER_IC_IOC_LIGHTS_BASE) && (cmd_num <= POWER_IC_IOC_LIGHTS_LAST_CMD))
    {
        return lights_ioctl(cmd,arg);
    }
#endif
    /* Charger control request */
    else if((cmd_num >= POWER_IC_IOC_CMD_CHARGER_BASE) && (cmd_num <= POWER_IC_IOC_CHARGER_LAST_CMD))
    {
        return charger_ioctl(cmd, arg);
    }
    /* This will handle the request that address test commands*/
    else if((cmd_num >= POWER_IC_IOC_CMD_TCMD_BASE) && (cmd_num <= POWER_IC_IOC_CMD_TCMD_LAST_CMD))
    {
        return tcmd_ioctl(cmd, arg);
    }

    /* This will handle the request that address battery rom*/
    else if((cmd_num >= POWER_IC_IOC_CMD_BRT_BASE) && (cmd_num <= POWER_IC_IOC_CMD_BRT_LAST_CMD))
    {
        return brt_ioctl(cmd, arg);
    }

    /* ioctl handling for other modules goes here... */

    else /* The driver doesn't support this request. */
    {
        tracemsg(_k_d("0x%X unsupported ioctl command"), (int) cmd);
        return -ENOTTY;
    }

    return 0;
}

/*!
 * @brief the open() handler for the power IC device node
 *
 * This function implements the open() system call on the power IC device.  Currently,
 * this function does nothing.
 *
 * @param        inode       inode pointer
 * @param        file        file pointer
 *
 * @return 0
 */

static int power_ic_open(struct inode *inode, struct file *file)
{
    tracemsg(_k_d("Power IC: open()"));
    return 0;
}

/*!
 * @brief the close() handler for the power IC device node
 *
 * This function implements the close() system call on the power IC device.   Currently,
 * this function does nothing.
 *
 * @param        inode       inode pointer
 * @param        file        file pointer
 *
 * @return 0
 */

static int power_ic_free(struct inode *inode, struct file *file)
{
    tracemsg(_k_d("Power IC: free()"));
    return 0;
}



/*!
 * @brief initialzes the Atlas registers
 *
 * Using the register initialization table, #tab_init_reg_???, this function initializes
 * all of the required Atlas registers.
 *
 */
static void __init initialize_atlas_registers (void)
{
    const unsigned int (* init_tbl_ptr)[NUM_INIT_REGS][NUM_INIT_TABLE_COLUMNS] = NULL;
    int i;

    /* Use correct register settings per product and per boardrev. */
#if defined(CONFIG_ARCH_MXC91321)
#if defined(CONFIG_MOT_FEAT_RF_PYTHON)
    init_tbl_ptr = &tab_init_reg_python_base;
#elif defined(CONFIG_MACH_BUTEREF)
    if(((boardrev() >= BOARDREV_P4AW) || (boardrev() >= BOARDREV_P5A)) && (boardrev() != BOARDREV_UNKNOWN))
    {
        init_tbl_ptr = &tab_init_reg_bute_p4aw;
    }
    else if((boardrev() >= BOARDREV_P4A) && (boardrev() != BOARDREV_UNKNOWN))
    {
        init_tbl_ptr = &tab_init_reg_bute_p4a;
    }
    else
    {
        init_tbl_ptr = &tab_init_reg_bute_base;
    }
#else /* Non-BUTEREF ArgonLV Products */
    init_tbl_ptr = &tab_init_reg_argonref71_base;
#endif
#elif defined(CONFIG_MACH_ASCENSION)
    if(boardrev() >= BOARDREV_P3A)
    {
        init_tbl_ptr = &tab_init_reg_ascension_p3a;
    }
    else
    {
        init_tbl_ptr = &tab_init_reg_ascension_p2a;
    }
#elif defined(CONFIG_MACH_SAIPAN)
    init_tbl_ptr = &tab_init_reg_saipan_base;
#elif defined(CONFIG_MACH_LIDO)
    init_tbl_ptr = &tab_init_reg_lido_base;
#elif defined(CONFIG_MACH_ELBA)
    init_tbl_ptr = &tab_init_reg_elba_base;
#elif defined(CONFIG_MACH_PICO)
    init_tbl_ptr = &tab_init_reg_pico_base;
#elif defined(CONFIG_MACH_MARCO)
    init_tbl_ptr = &tab_init_reg_marco_base;
#elif defined(CONFIG_MACH_XPIXL)
    init_tbl_ptr = &tab_init_reg_xpixl_base;
#elif defined(CONFIG_MACH_NEVIS)
    init_tbl_ptr = &tab_init_reg_nevis_base;
#else
    if(boardrev() >= BOARDREV_P3BW)
    {
        init_tbl_ptr = &tab_init_reg_scma11_p3bw;
    }
    else if(boardrev() == BOARDREV_P2AW)
    {
        init_tbl_ptr = &tab_init_reg_scma11_p2aw;
    }
    else if(boardrev() >= BOARDREV_P2A)
    {
        init_tbl_ptr = &tab_init_reg_scma11_p2a;
    }
    else
    {
        init_tbl_ptr = &tab_init_reg_scma11_base;
    }
#endif

    /* Loop through register settings. */
    for (i = 0; i < NUM_INIT_REGS; i++)
    {
        /* Only initialize registers if we support this product, and the register is valid. */
        if ((init_tbl_ptr != NULL) && (POWER_IC_REGISTER_IS_ATLAS((*init_tbl_ptr)[i][0])))
        {
            /* Remember the initial register values. */
            reg_init_tbl[(*init_tbl_ptr)[i][0]] = (*init_tbl_ptr)[i][1];

            /* Don't set PCEN or PCCOUNTEN. */
            if ((*init_tbl_ptr)[i][0] == POWER_IC_REG_ATLAS_PWR_CONTROL_0)
            {
                power_ic_set_reg_mask((*init_tbl_ptr)[i][0], PC0_POWERCUT_MASK, (*init_tbl_ptr)[i][1]);
            }
            else if ((*init_tbl_ptr)[i][0] == POWER_IC_REG_ATLAS_CHARGER_0)
            {
                power_ic_set_reg_mask((*init_tbl_ptr)[i][0], CHRGR0_POWER_PATH_MASK, (*init_tbl_ptr)[i][1]);
            }
            else
            {
                power_ic_write_reg_value ((*init_tbl_ptr)[i][0], (*init_tbl_ptr)[i][1]);
            }
        }
    }
}

/*! This structure defines the file operations for the power IC device */
static struct file_operations poweric_fops =
{
    .owner =    THIS_MODULE,
    .ioctl =    power_ic_ioctl,
    .open =     power_ic_open,
    .release =  power_ic_free,
    .poll =     atod_poll,
    .read =     atod_read,
};

/******************************************************************************
* Global Functions
******************************************************************************/

/*!
 * @brief power IC initialization function
 *
 * This function implements the initialization function of the power IC low-level
 * driver.  The function is called during system initialization or when the module
 * is loaded (if compiled as a module).  It is responsible for performing the
 * power IC register initialization, initializing the power IC interrupt (event)
 * handling, and performing any initialization required for the handling the
 * power IC device node.
 *
 * @return 0
 */

int __init power_ic_init(void)
{
    int ret;

    tracemsg ("Power IC driver init");

    ret = register_chrdev(POWER_IC_MAJOR_NUM, POWER_IC_DEV_NAME, &poweric_fops);
    if (ret < 0)
    {
        tracemsg(_k_d("unable to get a major (%d) for power ic"), (int)POWER_IC_MAJOR_NUM);
        return ret;
    }

    devfs_mk_cdev(MKDEV(POWER_IC_MAJOR_NUM,0), S_IFCHR | S_IRUGO | S_IWUGO, "power_ic");

    spi_init();
    
    /* Initialize the ATLAS driver */
    initialize_atlas_registers();

#ifdef CONFIG_ARCH_MXC91231
    /* Initialize the lighting driver */
    lights_init();
#endif

    /* Before initializing events or any sub-modules, determine the power-up reason */
    power_up_reason = determine_power_up_reason();

    /* Initialize the event handling system */
    power_ic_event_initialize();

    /* Initialize the AtoD interface. */
    atod_init();

    /* Register with power management */
    if (driver_register((struct device_driver *)&power_ic_pm_sr) != 0)
    {
        tracemsg("power_ic_init: power management registration failed\n");
    }
    else
    {
        tracemsg("power_ic_init: power management regristration passed\n");
        
        if (platform_device_register((struct platform_device *)&power_ic_pm_sr_device) != 0)
        {
            tracemsg("power_ic_init: power management device registration failed\n");
        }
        else
        {
            tracemsg("power_ic_init: power management device registration passed\n");
        }
    }
    /* Initialize the power ic debounce thread */
    power_ic_debounce_init();
    tracemsg ("Power IC driver init complete\n");

    /* Initialize the accessory driver */
    moto_accy_init();

    /* Initialize the EMU glue utils and create /proc/emu entry */
    emu_glue_utils_init();

    tracemsg ("EMU accessory detection driver init complete\n");

    /* Initialize one wire bus */
    owire_init();

    return 0;
}

/*!
 * @brief power IC cleanup function
 *
 * This function implements the exit function of the power IC low-level driver.
 * The function is called when the kernel is being taken down or when the module
 * is unloaded (if compiled as a module).  It is currently responsible for
 * unregistering the power IC character device and calling the tear-down function
 * for each of the modules that require tear-down.
 */

static void __exit power_ic_exit(void)
{
    unregister_chrdev(POWER_IC_MAJOR_NUM, POWER_IC_DEV_NAME);

    driver_unregister((struct device_driver *)&power_ic_pm_sr);
    platform_device_unregister((struct platform_device *)&power_ic_pm_sr);

    /* Tear-down the accessory driver */
    moto_accy_destroy();

    tracemsg("Power IC core: successfully unloaded\n");
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/*
 * Module entry points
 */
module_init(power_ic_init);
module_exit(power_ic_exit);

MODULE_DESCRIPTION("Power IC char device driver");
MODULE_AUTHOR("Motorola/FreeScale");
MODULE_LICENSE("GPL");
#endif
