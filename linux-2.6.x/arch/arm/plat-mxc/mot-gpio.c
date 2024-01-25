/*
 * mot-gpio.c - Motorola-specific GPIO API
 *
 * Copyright (C) 2006-2008 Motorola, Inc.
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

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 * 30-Nov-2006  Motorola        remove exported gpio_sensor_active
 *                              and gpio_sensor_inactive interface
 * 06-Dec-2006  Motorola        Added etm_enable_trigger_clock export.
 * 02-Jan-2007  Motorola        Update high speed USB API.
 * 23-Jan-2007  Motorola        Added Bluetooth bt_power control API.
 * 12-Feb-2007  Motorola        Adjust for hw config.
 * 23-Feb-2007  Motorola        Export DAI functions.
 * 13-Feb-2007  Motorola        New GPIO IRQ manipulators.
 * 23-Mar-2007  Motorola        Add GPIO_SIGNAL_ANT_MMC_EN.
 * 24-May-2007  Motorola        Add GPIO_SIGNAL_LGT_CAP_RESET.
 * 09-Jul-2007  Motorola        Added gpio_free_irq_dev work around.
 * 19-Jul-2007  Motorola        Add dev_id to free_irq.
 * 21-Sep-2007  Motorola        Added GPIO_SIGNAL_USB_HS_REF_CLK_EN 
 *                              and GPIO_SIGNAL_MORPH_TKC_RESET
 * 25-Jan-2008  Motorola        Added gpio_usb_hs_ref_clk_en_set_data
 * 09-Apr-2008  Motorola        Added GPIO_SIGNAL_VMMC_2_9V_EN 
 * 21_Apr-2008  Motorola        Added GPIO_SIGNAL_PM_INT
 * 13-Aug-2008  Motorola        GP_AP_C8 toggle workaround for 300uA BT power issue
 * 07-Nov-2008  Motorola        Configure GPIO20 to function mode 
 *                              as PMICHE is disabled.
 */

#include <linux/module.h>
#include <linux/config.h>

#if defined(CONFIG_MOT_FEAT_GPIO_API)

#include <asm/arch/gpio.h>
#include <asm/mot-gpio.h>


#ifdef CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE
#include <asm/mothwcfg.h>


/**
 * GPIO signal name to <port, sig_no> pair mapping.
 */
struct gpio_signal_description gpio_signal_mapping[MAX_GPIO_SIGNAL] = {
    /*
     * Array index:  0  GPIO_SIGNAL_APP_CLK_EN_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  1  GPIO_SIGNAL_UART_DETECT
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  2  GPIO_SIGNAL_U1_TXD
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  3  GPIO_SIGNAL_U1_CTS_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  4  GPIO_SIGNAL_VFUSE_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  5  GPIO_SIGNAL_LIN_VIB_AMP_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  6  GPIO_SIGNAL_VVIB_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  7  GPIO_SIGNAL_WDOG_AP
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  8  GPIO_SIGNAL_WDOG_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index:  9  GPIO_SIGNAL_LOBAT_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 10  GPIO_SIGNAL_POWER_DVS0
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 11  GPIO_SIGNAL_POWER_RDY
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 12  GPIO_SIGNAL_BT_HOST_WAKE_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 13  GPIO_SIGNAL_BT_POWER
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 14  GPIO_SIGNAL_BT_RESET_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 15  GPIO_SIGNAL_BT_WAKE_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 16  GPIO_SIGNAL_CAM_EXT_PWRDN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 17  GPIO_SIGNAL_CAM_FLASH_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 18  GPIO_SIGNAL_CAM_INT_PWRDN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 19  GPIO_SIGNAL_CAM_PD
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 20  GPIO_SIGNAL_CAM_RST_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 21  GPIO_SIGNAL_CAM_TORCH_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 22  GPIO_SIGNAL_CLI_BKL
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 23  GPIO_SIGNAL_CLI_RST_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 24  GPIO_SIGNAL_DISP_RST_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 25  GPIO_SIGNAL_DISP_SD
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 26  GPIO_SIGNAL_LCD_BACKLIGHT
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 27  GPIO_SIGNAL_LCD_SD
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 28  GPIO_SIGNAL_DISP_CM
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 29  GPIO_SIGNAL_SERDES_RESET_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 30  GPIO_SIGNAL_STBY
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 31  GPIO_SIGNAL_SER_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 32  GPIO_SIGNAL_EL_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 33  GPIO_SIGNAL_CAP_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 34  GPIO_SIGNAL_FLIP_OPEN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 35  GPIO_SIGNAL_FLIP_DETECT
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 36  GPIO_SIGNAL_SLIDER_OPEN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 37  GPIO_SIGNAL_KEYS_LOCKED
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 38  GPIO_SIGNAL_TOUCH_INTB
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 39  GPIO_SIGNAL_FM_INT
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 40  GPIO_SIGNAL_FM_RST_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 41  GPIO_SIGNAL_FM_INTERRUPT
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 42  GPIO_SIGNAL_FM_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 43  GPIO_SIGNAL_GPS_PWR_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 44  GPIO_SIGNAL_GPS_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 45  GPIO_SIGNAL_GPU_A2
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 46  GPIO_SIGNAL_GPU_CORE1_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 47  GPIO_SIGNAL_GPU_DPD
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 48  GPIO_SIGNAL_GPU_INT_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 49  GPIO_SIGNAL_GPU_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 50  GPIO_SIGNAL_MEGA_SIM_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 51  GPIO_SIGNAL_SD1_DAT3
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 52  GPIO_SIGNAL_SD1_DET_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 53  GPIO_SIGNAL_SD2_DAT3
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 54  GPIO_SIGNAL_TF_DET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 55  GPIO_SIGNAL_TNLC_KCHG
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 56  GPIO_SIGNAL_TNLC_KCHG_INT
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 57  GPIO_SIGNAL_TNLC_RCHG
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 58  GPIO_SIGNAL_TNLC_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 59  GPIO_SIGNAL_FSS_HYST
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 60  GPIO_SIGNAL_UI_IC_DBG
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 61  GPIO_SIGNAL_USB_HS_DMA_REQ
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 62  GPIO_SIGNAL_USB_HS_FLAGC
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 63  GPIO_SIGNAL_USB_HS_INT
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 64  GPIO_SIGNAL_USB_HS_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 65  GPIO_SIGNAL_USB_HS_SWITCH
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 66  GPIO_SIGNAL_USB_HS_WAKEUP
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 67  GPIO_SIGNAL_USB_XCVR_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 68  GPIO_SIGNAL_WLAN_CLIENT_WAKE_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 69  GPIO_SIGNAL_WLAN_DBHD_MUX
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 70  GPIO_SIGNAL_WLAN_HOST_WAKE_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 71  GPIO_SIGNAL_WLAN_PWR_DWN_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 72  GPIO_SIGNAL_WLAN_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 73  GPIO_SIGNAL_PWM_BKL
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 74  GPIO_SIGNAL_MAIN_BKL
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 75  GPIO_SIGNAL_IRDA_SD
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 76  GPIO_SIGNAL_ANT_MMC_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 77  GPIO_SIGNAL_LGT_CAP_RESET
     */
    { GPIO_INVALID_PORT,   0 },
    
    /*
     * Array index: 78  GPIO_SIGNAL_DM500_WAKE_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 79  GPIO_SIGNAL_DM500_RESET_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 80  GPIO_SIGNAL_DM500_INT_B
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 81  GPIO_SIGNAL_DM500_VCC_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 82  GPIO_SIGNAL_MORPH_TKC_RESET
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 83  GPIO_SIGNAL_USB_HS_REF_CLK_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 84  GPIO_SIGNAL_VMMC_2_9V_EN
     */
    { GPIO_INVALID_PORT,   0 },

    /*
     * Array index: 85  GPIO_SIGNAL_PM_INT
     */
    { GPIO_INVALID_PORT,   0 },

};


/*
 * The following data structures are used to convert the values assigned
 * to GPIO signals in the hardware configuration tree to the zero-based
 * indexes used to describe GPIO signals in the kernel.
 */

/**
 * Number of "groups" defined HWCFG tree schema for GPIO signals.
 */
#define HWCFG_GPIO_GROUP_COUNT 17


/**
 * Number of signals in each HWCFG GPIO signal group.
 */
static const int __initdata hwcfg_gpio_group_sig_count[HWCFG_GPIO_GROUP_COUNT] = {
     0, /* MISC */
     4, /* Bluetooth */
     6, /* Camera */
    16, /* Display */
     4, /* FM */
     2, /* GPS */
     5, /* GPU */
     6, /* UserInput */
     9, /* USB */
     5, /* WLAN */
     6, /* SDHC */
     7, /* TNLC */
     4, /* Power */
     2, /* Vibrator */
     2, /* WDOG */
     1, /* VFUSE */
     3, /* UART */
};


static const enum gpio_signal __initdata misc_hwcfg_gpio_group[0] = {
};


static const enum gpio_signal __initdata bluetooth_hwcfg_gpio_group[4] = {
    GPIO_SIGNAL_BT_HOST_WAKE_B,          /* 0x00 */
    GPIO_SIGNAL_BT_POWER,                /* 0x01 */
    GPIO_SIGNAL_BT_RESET_B,              /* 0x02 */
    GPIO_SIGNAL_BT_WAKE_B,               /* 0x03 */
};


static const enum gpio_signal __initdata camera_hwcfg_gpio_group[6] = {
    GPIO_SIGNAL_CAM_EXT_PWRDN,           /* 0x00 */
    GPIO_SIGNAL_CAM_FLASH_EN,            /* 0x01 */
    GPIO_SIGNAL_CAM_INT_PWRDN,           /* 0x02 */
    GPIO_SIGNAL_CAM_PD,                  /* 0x03 */
    GPIO_SIGNAL_CAM_RST_B,               /* 0x04 */
    GPIO_SIGNAL_CAM_TORCH_EN,            /* 0x05 */
};


static const enum gpio_signal __initdata display_hwcfg_gpio_group[16] = {
    GPIO_SIGNAL_CLI_BKL,                 /* 0x00 */
    GPIO_SIGNAL_CLI_RST_B,               /* 0x01 */
    GPIO_SIGNAL_DISP_RST_B,              /* 0x02 */
    GPIO_SIGNAL_DISP_SD,                 /* 0x03 */
    GPIO_SIGNAL_LCD_BACKLIGHT,           /* 0x04 */
    GPIO_SIGNAL_LCD_SD,                  /* 0x05 */
    GPIO_SIGNAL_DISP_CM,                 /* 0x06 */
    GPIO_SIGNAL_SERDES_RESET_B,          /* 0x07 */
    GPIO_SIGNAL_STBY,                    /* 0x08 */
    GPIO_SIGNAL_SER_EN,                  /* 0x09 */
    GPIO_SIGNAL_EL_EN,                   /* 0x0A */
    GPIO_SIGNAL_APP_CLK_EN_B,            /* 0x0B */
    GPIO_SIGNAL_DM500_WAKE_B,            /* 0x0C */
    GPIO_SIGNAL_DM500_RESET_B,           /* 0x0D */
    GPIO_SIGNAL_DM500_INT_B,             /* 0x0E */
    GPIO_SIGNAL_DM500_VCC_EN,            /* 0x0F */
};


static const enum gpio_signal __initdata fm_hwcfg_gpio_group[4] = {
    GPIO_SIGNAL_FM_INT,                  /* 0x00 */
    GPIO_SIGNAL_FM_RST_B,                /* 0x01 */
    GPIO_SIGNAL_FM_INTERRUPT,            /* 0x02 */
    GPIO_SIGNAL_FM_RESET,                /* 0x03 */
};


static const enum gpio_signal __initdata gps_hwcfg_gpio_group[2] = {
    GPIO_SIGNAL_GPS_PWR_EN,              /* 0x00 */
    GPIO_SIGNAL_GPS_RESET,               /* 0x01 */
};


static const enum gpio_signal __initdata gpu_hwcfg_gpio_group[5] = {
    GPIO_SIGNAL_GPU_A2,                  /* 0x00 */
    GPIO_SIGNAL_GPU_CORE1_EN,            /* 0x01 */
    GPIO_SIGNAL_GPU_DPD,                 /* 0x02 */
    GPIO_SIGNAL_GPU_INT_B,               /* 0x03 */
    GPIO_SIGNAL_GPU_RESET,               /* 0x04 */
};


static const enum gpio_signal __initdata userinput_hwcfg_gpio_group[7] = {
    GPIO_SIGNAL_CAP_RESET,               /* 0x00 */
    GPIO_SIGNAL_FLIP_OPEN,               /* 0x01 */
    GPIO_SIGNAL_FLIP_DETECT,             /* 0x02 */
    GPIO_SIGNAL_SLIDER_OPEN,             /* 0x03 */
    GPIO_SIGNAL_KEYS_LOCKED,             /* 0x04 */
    GPIO_SIGNAL_TOUCH_INTB,              /* 0x05 */
    GPIO_SIGNAL_LGT_CAP_RESET,           /* 0x06 */
};


static const enum gpio_signal __initdata usb_hwcfg_gpio_group[9] = {
    GPIO_SIGNAL_USB_HS_DMA_REQ,          /* 0x00 */
    GPIO_SIGNAL_USB_HS_FLAGC,            /* 0x01 */
    GPIO_SIGNAL_USB_HS_INT,              /* 0x02 */
    GPIO_SIGNAL_USB_HS_RESET,            /* 0x03 */
    GPIO_SIGNAL_USB_HS_SWITCH,           /* 0x04 */
    GPIO_SIGNAL_USB_HS_WAKEUP,           /* 0x05 */
    GPIO_SIGNAL_USB_XCVR_EN,             /* 0x06 */
    GPIO_SIGNAL_ANT_MMC_EN,              /* 0x07 */
    GPIO_SIGNAL_USB_HS_REF_CLK_EN,       /* 0x08 */
};


static const enum gpio_signal __initdata wlan_hwcfg_gpio_group[5] = {
    GPIO_SIGNAL_WLAN_CLIENT_WAKE_B,      /* 0x00 */
    GPIO_SIGNAL_WLAN_DBHD_MUX,           /* 0x01 */
    GPIO_SIGNAL_WLAN_HOST_WAKE_B,        /* 0x02 */
    GPIO_SIGNAL_WLAN_PWR_DWN_B,          /* 0x03 */
    GPIO_SIGNAL_WLAN_RESET,              /* 0x04 */
};


static const enum gpio_signal __initdata sdhc_hwcfg_gpio_group[6] = {
    GPIO_SIGNAL_MEGA_SIM_EN,             /* 0x00 */
    GPIO_SIGNAL_SD1_DAT3,                /* 0x01 */
    GPIO_SIGNAL_SD1_DET_B,               /* 0x02 */
    GPIO_SIGNAL_SD2_DAT3,                /* 0x03 */
    GPIO_SIGNAL_TF_DET,                  /* 0x04 */
    GPIO_SIGNAL_VMMC_2_9V_EN,            /* 0x05 */
};


static const enum gpio_signal __initdata tnlc_hwcfg_gpio_group[7] = {
    GPIO_SIGNAL_TNLC_KCHG,               /* 0x00 */
    GPIO_SIGNAL_TNLC_KCHG_INT,           /* 0x01 */
    GPIO_SIGNAL_TNLC_RCHG,               /* 0x02 */
    GPIO_SIGNAL_TNLC_RESET,              /* 0x03 */
    GPIO_SIGNAL_FSS_HYST,                /* 0x04 */
    GPIO_SIGNAL_UI_IC_DBG,               /* 0x05 */
    GPIO_SIGNAL_MORPH_TKC_RESET,         /* 0x06 */
};


static const enum gpio_signal __initdata power_hwcfg_gpio_group[4] = {
    GPIO_SIGNAL_LOBAT_B,                 /* 0x00 */
    GPIO_SIGNAL_POWER_DVS0,              /* 0x01 */
    GPIO_SIGNAL_POWER_RDY,               /* 0x02 */
    GPIO_SIGNAL_PM_INT,                  /* 0x03 */
};


static const enum gpio_signal __initdata vibrator_hwcfg_gpio_group[2] = {
    GPIO_SIGNAL_LIN_VIB_AMP_EN,          /* 0x00 */
    GPIO_SIGNAL_VVIB_EN,                 /* 0x01 */
};


static const enum gpio_signal __initdata wdog_hwcfg_gpio_group[2] = {
    GPIO_SIGNAL_WDOG_AP,                 /* 0x00 */
    GPIO_SIGNAL_WDOG_B,                  /* 0x01 */
};


static const enum gpio_signal __initdata vfuse_hwcfg_gpio_group[1] = {
    GPIO_SIGNAL_VFUSE_EN,                /* 0x00 */
};


static const enum gpio_signal __initdata uart_hwcfg_gpio_group[3] = {
    GPIO_SIGNAL_UART_DETECT,             /* 0x00 */
    GPIO_SIGNAL_U1_TXD,                  /* 0x01 */
    GPIO_SIGNAL_U1_CTS_B,                /* 0x02 */
};


/**
 * Mappings from HWCFG tree <group, offset> values for GPIO signals
 * to the gpio_signal enumeration equivalents.
 */
static const enum gpio_signal const * hwcfg_gpio_signals[HWCFG_GPIO_GROUP_COUNT] = {
    misc_hwcfg_gpio_group,
    bluetooth_hwcfg_gpio_group,
    camera_hwcfg_gpio_group,
    display_hwcfg_gpio_group,
    fm_hwcfg_gpio_group,
    gps_hwcfg_gpio_group,
    gpu_hwcfg_gpio_group,
    userinput_hwcfg_gpio_group,
    usb_hwcfg_gpio_group,
    wlan_hwcfg_gpio_group,
    sdhc_hwcfg_gpio_group,
    tnlc_hwcfg_gpio_group,
    power_hwcfg_gpio_group,
    vibrator_hwcfg_gpio_group,
    wdog_hwcfg_gpio_group,
    vfuse_hwcfg_gpio_group,
    uart_hwcfg_gpio_group,
};


/**
 * Convert the value defined in the hardware config schema for a GPIO
 * signal to the internal (zero-based) definition.
 *
 * @param   schemavalue Value assocated with GPIO signal name in hwcfg.
 * 
 * @return  gpio_signal enumeration value for GPIO signal.
 */
static enum gpio_signal __init hwcfg_to_gpio_signal(u16 schemavalue)
{
    /* In the hardware config tree, signal names are mapped to 16-bit
     * values. Bits 8-15 are a group identifier, and bits 0-7 are a
     * unique value in that group.
     */

    u8 group, sig;

    sig = (u8)schemavalue; /* chomp */
    group = (u8)(schemavalue>>8);

    if( (group < HWCFG_GPIO_GROUP_COUNT)
            && ( sig < hwcfg_gpio_group_sig_count[group] ) ) {
        return hwcfg_gpio_signals[group][sig];
    } else {
        return MAX_GPIO_SIGNAL;
    }
}


/**
 * Path to node containing GPIO initial settings and mappings in device tree.
 */
static const char gpio_hwcfg_path[] __initdata = "/System@0/GPIO@0";


/**
 * Path to node containing IOMUX pad and mux settings in device tree.
 */
static const char iomux_hwcfg_path[] __initdata = "/System@0/IOMUX@0";


/**
 * Read a table from the hardware configuration tree containing records
 * of the size recordSize. This function will determine the size of the
 * table, check to see if the table size is a multiple of recordSize,
 * allocate space for the table, and read it from the hardware configuration.
 * It is up to the caller to free the memory allocated by this function (using
 * kfree).
 *
 * @param   path    Path to the MOTHWCFGNODE containing the table.
 * @param   prop    The name of the MOHWCFG property holding the data.
 * @param   recordSize  The size of each record in the tabke,
 * @param   tableSize   [out] The size of the data read from the config tree.
 *
 * @return  Pointer to data read from the table; NULL otherwise.
 */
static void * __init read_mothwcfg_table(const char *path, const char *prop,
        size_t recordSize, size_t *tableSize)
{
    u8 *data = NULL;
    MOTHWCFGNODE *node;
    int size;

    /* open the hardware configuration node */
    node = mothwcfg_get_node_by_path(path);
    if(node == NULL) {
        printk("%s: unable to read node \"%s\" from HWCFG\n",
                __FUNCTION__, path);
        return NULL;
    }

    /* get the size of the table */
    size = mothwcfg_read_prop(node, prop, NULL, 0);
    if(size < 0) {
        printk("%s: unable to determine size of property \"%s\" in \"%s\"\n",
                __FUNCTION__, prop, path);
        goto die;
    }

    /* determine if size of table is divisible by the size of an entry */
    if( ( size % recordSize ) != 0 ) {
        printk("%s: badly sized table \"%s\" in \"%s\"\n",
                __FUNCTION__, prop, path);
        goto die;
    }

    /* allocate space for the table */
    data = kmalloc(size, GFP_KERNEL);
    if(data == NULL) {
        printk("%s: unable to allocate space for table \"%s\" in \"%s\"\n",
                __FUNCTION__, prop, path);
        goto die;
    }

    /* read the initial configuration table */
    if(mothwcfg_read_prop(node, prop, (void *)data, size) != size) {
        printk("%s: unable to read table \"%s\" in \"%s\"\n",
                __FUNCTION__, prop, path);
        kfree(data);
        data = NULL;
        goto die;
    }

    /* if everything went well, return the size of the data allocated */
    if(tableSize != NULL) {
        *tableSize = size;
    }

die:
    mothwcfg_put_node(node);

    return (void *)data;
}


/**
 * Configure GPIO signals based on data in the HWCFG tree.
 *
 * @return 0 on success; less than zero on failure.
 */
int __init mot_gpio_init(void)
{
    struct gpio_init_entry {
        u16 port;
        u16 sig_no;
        u16 dir;
        u16 data;
    } __attribute__((__packed__));

    struct gpio_init_entry *init_data, *init_entry;
    int size, count;

    /* read the data from the hardware configuration tree */
    init_data = (struct gpio_init_entry *)read_mothwcfg_table(
            gpio_hwcfg_path, "init", sizeof(struct gpio_init_entry),
            &size);
    if(init_data == NULL) {
        printk("%s: unable to read initial GPIO configuration table\n",
                __FUNCTION__);
        return -EINVAL;
    }

    /* iterate through each entry in the table */
    for(init_entry = init_data, count = size/sizeof(struct gpio_init_entry);
            count > 0; init_entry++, count--) {

        /* configure GPIO data register */
        if(init_entry->data != GPIO_DATA_INVALID) {
            gpio_tracemsg("%s: setting port %d, sig_no %d data to %d",
                    __FUNCTION__, init_entry->port, init_entry->sig_no,
                    init_entry->data);

           gpio_set_data(init_entry->port, init_entry->sig_no,
                    init_entry->data);
        }

        /* configure GPIO direction register */
        if(init_entry->dir != GPIO_GDIR_INVALID) {
            gpio_tracemsg("%s: setting port %d, sig_no %d direction to %d",
                    __FUNCTION__, init_entry->port, init_entry->sig_no,
                    init_entry->dir);
            
            gpio_config(init_entry->port, init_entry->sig_no, init_entry->dir,
                    GPIO_INT_NONE); /* setup interrupts later */
        }
    }

    kfree(init_data);

#else // #if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
int __init mot_gpio_init(void)
{
    int i;

    for(i = 0; i < MAX_GPIO_SIGNAL; i++) {
        if(initial_gpio_settings[i].port != GPIO_INVALID_PORT) {
            gpio_tracemsg("GPIO port: 0x%08x signal: 0x%08x output: 0x%08x "
                    "data: 0x%08x",
                    initial_gpio_settings[i].port,
                    initial_gpio_settings[i].sig_no,
                    initial_gpio_settings[i].out,
                    initial_gpio_settings[i].data);

            /* 
             * We set the data for a signal and then configure the direction
             * of the signal.  Section 52.3.1.1 (GPIO Data Register) of
             * the SCM-A11 DTS indicates that this is a reasonable thing to do.
             */
            /* set data */
            if(initial_gpio_settings[i].data != GPIO_DATA_INVALID) {
                gpio_set_data(initial_gpio_settings[i].port,
                        initial_gpio_settings[i].sig_no,
                        initial_gpio_settings[i].data);
            }

            /* set direction */
            gpio_config(initial_gpio_settings[i].port,
                    initial_gpio_settings[i].sig_no,
                    initial_gpio_settings[i].out,
                    GPIO_INT_NONE); /* setup interrupts later */
        }
    }
#endif // #if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
    return 0;
}

#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)

/**
 * Update the initial_gpio_settings(gpio_signal_mapping) table based on information from the HWCFG
 * tree.
 *
 * @return 0 on success; less than zero on failure.
 */
int __init mot_gpio_update_mappings(void)
{
    struct gpio_map_entry {
        u16 hwcfg_sig;
        u16 port;
        u16 sig_no;
    } __attribute__((__packed__));

    struct gpio_map_entry *map_data, *map_entry;
    int size, count;

    /* read the data from the hardware configuration tree */
    map_data = (struct gpio_map_entry *)read_mothwcfg_table(
            gpio_hwcfg_path, "signalmap", sizeof(struct gpio_map_entry),
            &size);
    if(map_data == NULL) {
        printk("%s: unable to read GPIO mapping table\n",
                __FUNCTION__);
        return -EINVAL;
    }

    /* iterate through each entry in the table */
    for(map_entry = map_data, count = size/sizeof(struct gpio_map_entry);
            count > 0; map_entry++, count--) {
        enum gpio_signal signal = hwcfg_to_gpio_signal(map_entry->hwcfg_sig);

        if(signal == MAX_GPIO_SIGNAL) {
            gpio_tracemsg("%s: found an unknown GPIO signal: %d",
                    __FUNCTION__, map_entry->hwcfg_sig);
            continue;
        }

        gpio_tracemsg("%s: mapping signal %d to port %d, sig_no %d",
                __FUNCTION__, signal, map_entry->port, map_entry->sig_no);

        /* update mapping table */
        gpio_signal_mapping[signal].port = map_entry->port;
        gpio_signal_mapping[signal].sig_no = map_entry->sig_no;
    }

    kfree(map_data);

    return 0;
}


/**
 * Set IOMUX mux registers to the state defined by the HWCFG.
 *
 * @return 0 on success; less than zero on failure.
 */
int __init mot_iomux_mux_init(void)
{
    struct iomux_mux_entry {
        u16 hwcfg_pin;
        u16 output_config;
        u16 input_config;
    } __attribute__((__packed__));

    struct iomux_mux_entry *data, *entry;
    int size, count;

    /* read the data from the hardware configuration tree */
    data = (struct iomux_mux_entry *)read_mothwcfg_table(
            iomux_hwcfg_path, "pininit", sizeof(struct iomux_mux_entry),
            &size);
    if(data == NULL) {
        printk("%s: unable to read IOMUX pin settings table\n",
                __FUNCTION__);
        return -EINVAL;
    }

    /* configure IOMUX settings to their prescribed initial states */
    for(entry = data, count = size/sizeof(struct iomux_mux_entry); count > 0;
            entry++, count--) {
        enum iomux_pins fsl_pin = hwcfg_pin_to_fsl_pin(entry->hwcfg_pin);

        if(fsl_pin == IOMUX_INVALID_PIN) {
            printk("%s: Unrecognized pin found in HWCFG: 0x%04X\n",
                    __FUNCTION__, entry->hwcfg_pin);
            continue;
        }

        gpio_tracemsg( "%s: IOMUX pin: 0x%08x output: 0x%02x input: 0x%02x",
                __FUNCTION__, fsl_pin, entry->output_config,
                entry->input_config);
                
#if defined(CONFIG_MACH_ARGONLVPHONE)
	/*
	 * PMICHE is disabled, the PMIC_RDY signal would be ignored, here just
	 * put PIN_GPIO20 on a known state 1
	 */
	if (fsl_pin == PIN_GPIO20)
		iomux_config_mux(fsl_pin, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
	else
#endif
		iomux_config_mux(fsl_pin, entry->output_config, entry->input_config);
    }

    kfree(data);

    return 0;
}


/**
 * Set IOMUX pad registers to the state defined by the HWCFG.
 */
int __init mot_iomux_pad_init(void)
{
    struct iomux_pad_entry {
        u16 grp;
        u16 config;
    } __attribute__((__packed__));

    struct iomux_pad_entry *data, *entry;
    int size, count;
    
    /* read the data from the hardware configuration tree */
    data = (struct iomux_pad_entry *)read_mothwcfg_table(
            iomux_hwcfg_path, "padinit", sizeof(struct iomux_pad_entry),
            &size);
    if(data == NULL) {
        printk("%s: unable to read IOMUX pad settings table\n",
                __FUNCTION__);
        return -EINVAL;
    }

    /* set iomux pad registers to the prescribed state */
    for(entry = data, count = size/sizeof(struct iomux_pad_entry); count > 0;
            entry++, count--) {
        gpio_tracemsg("Setting pad register 0x%04x to: 0x%04x",
                entry->grp, entry->config);

        arch_mot_iomux_pad_init(entry->grp, entry->config);
    }

    kfree(data);

    return 0;
}
#else  // #if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)

/**
 * Set IOMUX pad registers to the state defined in the SCM-A11 platform
 * pin list.
 */
void __init mot_iomux_pad_init(void)
{
    int i;

    /* set iomux pad registers to the prescribed state */
    for(i = IOMUX_PAD_SETTING_START; i <= IOMUX_PAD_SETTING_STOP; i++) {
        gpio_tracemsg("Setting pad register 0x%08x to: 0x%08x",
                iomux_pad_register_settings[i].grp,
                iomux_pad_register_settings[i].config);

        iomux_set_pad(iomux_pad_register_settings[i].grp,
                iomux_pad_register_settings[i].config);
    }
}

/**
 * Set IOMUX mux registers to the state defiend in the SCM-A11 platform
 * pin list.
 */
void __init mot_iomux_mux_init(void)
{
    int i, j;

    /* configure IOMUX settings to their prescribed initial state */
    for(i = 0; initial_iomux_settings[i] != NULL; i++) {
        for(j = 0; initial_iomux_settings[i][j].pin != IOMUX_INVALID_PIN; j++) {
            gpio_tracemsg("IOMUX pin: 0x%08x output: 0x%02x input: 0x%02x",
                    initial_iomux_settings[i][j].pin,
                    initial_iomux_settings[i][j].output_config,
                    initial_iomux_settings[i][j].input_config);

            iomux_config_mux(initial_iomux_settings[i][j].pin,
                    initial_iomux_settings[i][j].output_config,
                    initial_iomux_settings[i][j].input_config);
        }           
    }               
}
#endif // defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)


/**
 * Configure a GPIO signal based on information from the GPIO settings
 * array.
 *
 * @param   index   Index into the init_gpio_settings array.
 * @param   out     Non-zero of GPIO signal should be configured for output.
 * @param   icr     Setting for GPIO signal interrupt control.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_config(enum gpio_signal index, bool out,
        enum gpio_int_cfg icr)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
        gpio_config(gpio_signal_mapping[index].port,
                gpio_signal_mapping[index].sig_no, out, icr);
#else
        gpio_config(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no, out, icr);
#endif // #if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
        return 0;
    }

    gpio_tracemsg("Attempt to configure invalid GPIO signal: %d", index);
    return -ENODEV;
}


/**
 * Setup an interrupt handler for a GPIO signal based on information from
 * the GPIO settings array.
 *
 * @param   index       Index into the initial GPIO settings array.
 * @param   prio        Interrupt priority.
 * @param   handler     Handler for the interrupt.
 * @param   irq_flags   Interrupt flags.
 * @param   devname     Name of the device driver.
 * @param   dev_id      Unique ID for the device driver.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_request_irq(enum gpio_signal index,
        enum gpio_prio prio, gpio_irq_handler handler, __u32 irq_flags,
        const char *devname, void *dev_id)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
        return gpio_request_irq(gpio_signal_mapping[index].port,
                gpio_signal_mapping[index].sig_no, prio, handler, irq_flags,
#else
        return gpio_request_irq(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no, prio, handler, irq_flags,
#endif // #if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
                devname, dev_id);
    }

    gpio_tracemsg("Attempt to request interrupt for invalid GPIO signal: %d",
            index);
    return -ENODEV;
}


/**
 * Remove an interrupt handler for a GPIO signal based on information from
 * the GPIO settings array.
 *
 * @param   index       Index into the initial GPIO settings array.
 * @param   prio        Interrupt priority.
 * @param   dev_id      Unique ID for the device driver.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_free_irq(enum gpio_signal index,
        enum gpio_prio prio
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) ||  defined(CONFIG_MACH_KEYWEST)  ||  defined(CONFIG_MACH_PEARL) 
			, void *dev_id
#endif
					)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) ||  defined(CONFIG_MACH_KEYWEST) ||  defined(CONFIG_MACH_PEARL) 
        gpio_free_irq_dev
#else
        gpio_free_irq
#endif
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE) 
        	(gpio_signal_mapping[index].port,
                gpio_signal_mapping[index].sig_no, prio
#else
        	(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no, prio
#endif /* defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE */
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA) ||  defined(CONFIG_MACH_KEYWEST) ||  defined(CONFIG_MACH_PEARL) 
							, dev_id
#endif
							);
        return 0;
    }

    gpio_tracemsg(
            "Attempt to free interrupt request for invalid GPIO signal: %d",
            index);
    return -ENODEV;
}

#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE) 
/**
 * Temporarily disable interrupts for the specified GPIO signal. Interrupts
 * can be re-enabled by calling gpio_signal_enable_irq.
 *
 * @param   index       Index into the initial GPIO settings array.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_disable_irq(enum gpio_signal index)
{
    if(!GPIO_SIGNAL_IS_VALID(index)) {
        gpio_tracemsg("Attempt to disable_irq for invalid GPIO signal: %d",
                index);
        return -ENODEV;
    }

    disable_irq(MXC_GPIO_TO_IRQ(GPIO_PORT_SIG_TO_MXC_GPIO(
                    gpio_signal_mapping[index].port,
                    gpio_signal_mapping[index].sig_no)));
    return 0;
}


/**
 * Enable interrupts for the specified GPIO signal. An interrupt handler
 * must already be registered using gpio_signal_request_irq.
 *
 * @param   index       Index into the initial GPIO settings array.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_enable_irq(enum gpio_signal index)
{
    if(!GPIO_SIGNAL_IS_VALID(index)) {
        gpio_tracemsg("Attempt to enable_irq for invalid GPIO signal: %d",
                index);
        return -ENODEV;
    }

    enable_irq(MXC_GPIO_TO_IRQ(GPIO_PORT_SIG_TO_MXC_GPIO(
                    gpio_signal_mapping[index].port,
                    gpio_signal_mapping[index].sig_no)));
    return 0;
}
#endif


/**
 * Clear a GPIO interrupt.
 *
 * @param   index       Index into the initial GPIO settings array.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_clear_int(enum gpio_signal index)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
        /* obsolete in in MVL 05/12
        gpio_clear_int(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no);
                */
        /* gpio_clear_int is no longer present or required */
        return 0;
    }

    gpio_tracemsg("Attempt to clear interrupt for invalid GPIO signal: %d",
            index);
    return -ENODEV;
}


/**
 * Set a GPIO signal's data register based on information from the initial
 * GPIO settings array.
 *
 * @param   index   Index into the initial GPIO settings array.
 * @param   data    New value for the data register.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_set_data(enum gpio_signal index, __u32 data)
{
    if(GPIO_SIGNAL_IS_VALID(index)) {
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE) 
        gpio_set_data(gpio_signal_mapping[index].port,
                gpio_signal_mapping[index].sig_no,
#else
        gpio_set_data(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no,
#endif /* CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE */
                data);
        return 0;
    }
    
    gpio_tracemsg("Attempt to set data for invalid signal: %d", index);
    return -ENODEV;
}


/**
 * Get a GPIO signal's state based on information from the initial
 * GPIO settings array.
 *
 * @param   index   [in] Index into the initial GPIO settings array.
 * @param   data    [out] Value of the data bit for the GPIO signal.
 *
 * @return  Zero on success; non-zero on failure.
 */
int gpio_signal_get_data(enum gpio_signal index, __u32 *data)
{
    if(data==NULL) {
        return -EFAULT;
    }

    if(GPIO_SIGNAL_IS_VALID(index)) {
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE) 
        *data = gpio_get_data(gpio_signal_mapping[index].port,
                gpio_signal_mapping[index].sig_no);
#else
        *data = gpio_get_data(initial_gpio_settings[index].port,
                initial_gpio_settings[index].sig_no);
#endif
        return 0;
    }

    gpio_tracemsg("Attempt to read data for invalid signal: %d", index);
    return -ENODEV;
}


/**
 * This is like gpio_signal_get_data but will print an error message
 * if index isn't a valid signal for the particular hardware
 * configuration (printk is called with the KERN_ERR message level). 
 * The caller will not be notified of the failure, so the return value
 * is potentially invalid.
 *
 * Only call this function if you know what you are doing.
 *
 * @param   index   [in] Index into the initial GPIO settings array.
 *
 * @return  Value of the data bit for the GPIO signal.
 */
__u32 gpio_signal_get_data_check(enum gpio_signal index)
{
    __u32 data;
    int ret;

    if((ret = gpio_signal_get_data(index, &data)) != 0) {
        printk(KERN_ERR "Encountered error %d reading GPIO signal %d!\n",
                ret, index);
    }

    return data;
}


#if defined(CONFIG_MOT_FEAT_GPIO_API_EDIO)
/**
 * Protect edio_set_data.
 */
static DEFINE_RAW_SPINLOCK(edio_lock);


/**
 * This function sets a EDIO signal value.
 *
 * @param  sig_no       EDIO signal (0 based) as defined in \b #edio_sig_no
 * @param  data         value to be set (only 0 or 1 is valid)
 */
void edio_set_data(enum edio_sig_no sig_no, __u32 data)
{
    unsigned long edio_flags;
    unsigned short rval;
    
    MXC_ERR_CHK((data > 1) || (sig_no > EDIO_MAX_SIG_NO));
    
    spin_lock_irqsave(&edio_lock, edio_flags);

    /* EDIO_EPDR is EDIO Edge Port Data Register */
    rval = __raw_readw(EDIO_EPDR);

    /* clear the bit */
    rval &= ~(1 << sig_no); 

    /* if data is set to a non-zero value, set the appropriate bit */
    if(data) {
        rval |= (1 << sig_no);
    }
    
    __raw_writew(rval, EDIO_EPDR);

    spin_unlock_irqrestore(&edio_lock, edio_flags);
}


/**
 * This function returns the value of the EDIO signal.
 *
 * @param  sig_no       EDIO signal (0 based) as defined in \b #edio_sig_no
 *
 * @return Value of the EDIO signal (either 0 or 1)
 */
__u32 edio_get_data(enum edio_sig_no sig_no)
{
    MXC_ERR_CHK(sig_no > EDIO_MAX_SIG_NO);
    return (__raw_readw(EDIO_EPDR) >> sig_no) & 1;
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_EDIO */


/*
 * Export initial_gpio_settings(gpio_signal_mapping) array for GPIODev.
 */
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE) 
EXPORT_SYMBOL(gpio_signal_mapping);
#else
EXPORT_SYMBOL(initial_gpio_settings); // EXPORT_SYMBOL(initial_gpio_settings);
#endif


/*
 * Export common GPIO API functions.
 */
EXPORT_SYMBOL(gpio_signal_config);
EXPORT_SYMBOL(gpio_signal_request_irq);
EXPORT_SYMBOL(gpio_signal_free_irq);
#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE) 
EXPORT_SYMBOL(gpio_signal_disable_irq);
EXPORT_SYMBOL(gpio_signal_enable_irq);
#endif
EXPORT_SYMBOL(gpio_signal_clear_int);
EXPORT_SYMBOL(gpio_signal_set_data);
EXPORT_SYMBOL(gpio_signal_get_data);
EXPORT_SYMBOL(gpio_signal_get_data_check);


/*
 * Export EDIO API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_EDIO)
EXPORT_SYMBOL(edio_set_data);
EXPORT_SYMBOL(edio_get_data);
#endif


/*
 * Export Digital Audio Interface functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_DAI)
EXPORT_SYMBOL(gpio_dai_enable);
EXPORT_SYMBOL(gpio_dai_disable);
#endif

/*
 * Export SDHC API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_SDHC)
EXPORT_SYMBOL(gpio_sdhc_active);
EXPORT_SYMBOL(gpio_sdhc_inactive);
EXPORT_SYMBOL(sdhc_intr_setup);
EXPORT_SYMBOL(sdhc_intr_destroy);
EXPORT_SYMBOL(sdhc_intr_clear);
EXPORT_SYMBOL(sdhc_get_min_clock);
EXPORT_SYMBOL(sdhc_get_max_clock);
EXPORT_SYMBOL(sdhc_find_card);
#endif /* CONFIG_MOT_FEAT_GPIO_API_SDHC */


/*
 * Export USB HS GPIO API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_USBHS)
EXPORT_SYMBOL(gpio_usb_hs_reset_set_data);
EXPORT_SYMBOL(gpio_usb_hs_wakeup_set_data);
EXPORT_SYMBOL(gpio_usb_hs_switch_set_data);
EXPORT_SYMBOL(gpio_usb_hs_ref_clk_en_set_data);
EXPORT_SYMBOL(gpio_usb_hs_flagc_request_irq);
EXPORT_SYMBOL(gpio_usb_hs_flagc_free_irq);
EXPORT_SYMBOL(gpio_usb_hs_flagc_clear_int);
EXPORT_SYMBOL(gpio_usb_hs_flagc_get_data);
EXPORT_SYMBOL(gpio_usb_hs_int_request_irq);
EXPORT_SYMBOL(gpio_usb_hs_int_free_irq);
EXPORT_SYMBOL(gpio_usb_hs_int_clear_int);
EXPORT_SYMBOL(gpio_usb_hs_int_get_data);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_config_gpio_mode);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_config_sdma_mode);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_config_dual_mode);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_request_irq);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_set_irq_type);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_free_irq);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_clear_int);
EXPORT_SYMBOL(gpio_usb_hs_dma_req_get_data);
#endif /* CONFIG_MOT_FEAT_GPIO_API_USBHS */


/*
 * Export Bluetooth power management API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_BTPOWER)
EXPORT_SYMBOL(gpio_bluetooth_hostwake_request_irq);
EXPORT_SYMBOL(gpio_bluetooth_hostwake_free_irq);
EXPORT_SYMBOL(gpio_bluetooth_hostwake_clear_int);
EXPORT_SYMBOL(gpio_bluetooth_hostwake_get_data);
EXPORT_SYMBOL(gpio_bluetooth_wake_set_data);
EXPORT_SYMBOL(gpio_bluetooth_wake_get_data);
EXPORT_SYMBOL(gpio_bluetooth_power_set_data);
EXPORT_SYMBOL(gpio_bluetooth_power_get_data);
#if defined(CONFIG_ARCH_MXC91231) //#if defined(CONFIG_MACH_NEVIS)
EXPORT_SYMBOL(gpio_bluetooth_power_fixup);
#endif /* CONFIG_MACH_NEVIS */
#endif /* CONFIG_MOT_FEAT_GPIO_API_BTPOWER */


/*
 * Export WLAN power management API functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_WLAN)
EXPORT_SYMBOL(gpio_wlan_reset_set_data);
EXPORT_SYMBOL(gpio_wlan_clientwake_set_data);
EXPORT_SYMBOL(gpio_wlan_powerdown_get_data);
EXPORT_SYMBOL(gpio_wlan_powerdown_set_data);
EXPORT_SYMBOL(gpio_wlan_hostwake_request_irq);
EXPORT_SYMBOL(gpio_wlan_hostwake_free_irq);
EXPORT_SYMBOL(gpio_wlan_hostwake_clear_int);
EXPORT_SYMBOL(gpio_wlan_hostwake_get_data);
#endif /* CONFIG_MOT_FEAT_GPIO_API_WLAN */


/*
 * Export ETM-related functions.
 */
#if defined(CONFIG_MOT_FEAT_GPIO_API_ETM)
EXPORT_SYMBOL(etm_iomux_config);
EXPORT_SYMBOL(etm_enable_trigger_clock);
#endif /* CONFIG_MOT_FEAT_GPIO_API_ETM */

#endif /* CONFIG_MOT_FEAT_GPIO_API */

