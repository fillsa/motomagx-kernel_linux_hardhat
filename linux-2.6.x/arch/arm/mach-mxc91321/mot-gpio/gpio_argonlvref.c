/*
 * GPIO and IOMUX Configuration for ArgonLV-based LJ7.1 Reference Design
 * 
 * linux/arch/arm/mach-mxc91321/gpio_argonlvref.c
 *
 * Copyright 2006 Motorola, Inc.
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * 01-Nov-2006  Motorola        Initial revision.
 * 17-Nov-2006  Motorola        Initialize VFUSE_EN to high.
 * 21-Nov-2006  Motorola        Add GPU signal control.
 */

#include <linux/module.h>
#include "mot-gpio-argonlv.h"

/**
 * GPIO signal description and initial states.
 */
struct gpio_signal_settings initial_gpio_settings[MAX_GPIO_SIGNAL] = {
    /*
     * MCU GPIO Port Signal 31 -- Secondary Display Reset (active low)
     * LJ7.1 Reference Design Signal: GPIO_CLI_RST_B
     * Pin: USB1_VPOUT Mux Setting: GPIO
     *
     * Array index:  0      GPIO_SIGNAL_CLI_RST_B
     */
    { GPIO_MCU_PORT,        31, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Array index:  1      GPIO_SIGNAL_ENET_INT_B
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index:  2      GPIO_SIGNAL_IRDA_SD
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 15 -- 2MP Imager Power Down (set high to disable)
     * LJ7.1 Reference Design Signal: GPIO_CAM_EXT_PWRDN
     * Pin: GPIO15 Mux Setting: Func
     *
     * GPIO_MCU_15 also on pin GPIO20 mux setting GPIO.
     *
     * Array index:  3  GPIO_SIGNAL_CAM_EXT_PWRDN
     */
    { GPIO_MCU_PORT,        15, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * MCU GPIO Port Signal 16 -- 1.3MP Imager Power Down (set high to disable)
     * LJ7.1 Reference Design Signal: GPIO_CAM_INT_PWRDN
     * Pin: GPIO16 Mux Setting: Func
     *
     * Array index:  4  GPIO_SIGNAL_CAM_INT_PWRDN
     */
    { GPIO_MCU_PORT,        16, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * MCU GPIO Port Signal 17 -- SDHC Port 1 Card Detect
     * LJ7.1 Reference Design Signal: SD1_DET_B
     * Pin: GPIO17 Mux Setting: Func
     *
     * GPIO_MCU_17 also on pin GPIO22 mux setting GPIO.
     *
     * Array index:  5  GPIO_SIGNAL_SD1_DET_B
     */
    { GPIO_MCU_PORT,        17, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 24 -- Flip Detect
     * LJ7.1 Reference Design Signal: FLIP_DETECT
     * Pin: GPIO37 Mux Setting: Func
     *
     * GPIO_MCU_24 also on pin SIM1_RST0 mux setting GPIO.
     *
     * Array index:  6  GPIO_SIGNAL_FLIP_DETECT
     */
    { GPIO_MCU_PORT,        24, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * Array index:  7  GPIO_SIGNAL_DISP_RST_B
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index:  8  GPIO_SIGNAL_DISP_CM
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 30 -- Main Display Backlight Enable
     * LJ7.1 Reference Design Signal: LCD_BACKLIGHT
     * Pin: USB1_VPIN Mux Setting: GPIO
     *
     * Array index:  9  GPIO_SIGNAL_LCD_BACKLIGHT
     */
    { GPIO_MCU_PORT,        30, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * Shared GPIO Port Signal 1 -- Bluetooth Host Wake
     * LJ7.1 Reference Design Signal: BT_HOST_WAKE_B
     * Pin: GPIO21 Mux Setting: Func
     *
     * GPIO_Shared_1 also on pin IPU_CSI_MCLK mux setting GPIO.
     *
     * Array index: 10  GPIO_SIGNAL_BT_HOST_WAKE_B
     */
    { GPIO_SHARED_PORT,      1, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * Shared GPIO Port Signal 3 -- Camera Reset
     * LJ7.1 Reference Design Signal: GPIO_CAM_RST_B
     * Pin: GPIO23 Mux Setting: Func
     *
     * GPIO_Shared_3 is also on pin IPU_CSI_VSYNC mux setting GPIO.
     *
     * This ICD also described this signal as GPIO_MCU_18. GPIO_MCU_18 is
     * also on pin GPIO28 mux setting func. The hardware team indicates
     * we should use GPIO_Shared_3.
     *
     * Array index: 11  GPIO_SIGNAL_CAM_RST_B
     */
    { GPIO_SHARED_PORT,      3, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 6 -- Bluetooth Controller Wake
     * LJ7.1 Reference Design Signal: BT_WAKE_B
     * Pin: GPIO26 Mux Setting: Func
     *
     * GPIO_Shared_6 also on pin IPU_CSI_DATA_2 mux setting GPIO.
     *
     * Array index: 12  GPIO_SIGNAL_BT_WAKE_B
     */
    { GPIO_SHARED_PORT,      6, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 13 -- Bluetooth Reset
     * LJ7.1 Reference Design Signal: BT_RESET
     * Pin: GPIO13 Mux Setting: Func
     *
     * GPIO_MCU_13 also on pin GPIO18 mux setting GPIO.
     *
     * Array index: 13  GPIO_SIGNAL_BT_POWER
     */
    { GPIO_MCU_PORT,        13, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /* 
     * MCU GPIO Port Signal 23 -- Main Display Shutdown
     * LJ7.1 Reference Design Signal: LCD_SD
     * Pin: GPIO36 Mux Setting: Func
     *
     * GPIO_MCU_23 also on pin SIM1_CLK0 mux setting GPIO.
     *
     * Array index: 14  GPIO_SIGNAL_LCD_SD
     */
    { GPIO_MCU_PORT,        23, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 28 -- Main Display Serializer/Deserializer Enable
     * LJ7.1 Reference Design Signal: SERDES_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     *
     * Array index: 15  GPIO_SIGNAL_SERDES_RESET_B
     *
     * Set high to enable serializer at boot.
     */
    { GPIO_MCU_PORT,        28, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * MCU GPIO Port Signal 29 -- Serializer Standby (low-power mode when low)
     * LJ7.1 Reference Design Signal: STBY_B
     * Pin: USB1_TXENB Mux Setting: GPIO
     *
     * Array index: 16  GPIO_SIGNAL_STBY
     *
     * Set low to put serializer into standby mode at boot.
     */
    { GPIO_MCU_PORT,        29, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
    
    /*
     * Array index: 17  GPIO_SIGNAL_GPU_DPD_B
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
    
    /*
     * Array index: 18  GPIO_SIGNAL_GPU_RESET_B
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
    
    /*
     * Array index: 19  GPIO_SIGNAL_APP_CLK_EN_B
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
    
    /*
     * Shared GPIO Port Signal 21 -- GPS Reset
     * LJ7.1 Reference Design Signal: GPS_RESET
     * Pin: BOOT_MODE1 Mux Setting: GPIO
     *
     * Array index: 20  GPIO_SIGNAL_GPS_RESET
     */
    { GPIO_SHARED_PORT,     21, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
    
    /*
     * Array index: 21  GPIO_SIGNAL_GPU_VCORE1_EN
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
    
    /*
     * Array index: 22  GPIO_SIGNAL_GPU_VCORE2_EN
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 23  GPIO_SIGNAL_GPU_A2
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 24  GPIO_SIGNAL_GPU_INT_B
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 25  GPIO_SIGNAL_GPU_CORE1_EN
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 22 -- WLAN client wake
     * LJ7.1 Reference Design Signal: WLAN_CLIENT_WAKE_B
     * Pin: GPIO32 Mux Setting: Func
     *
     * GPIO_MCU_32 is also on pin GPIO27 mux setting GPIO.
     *
     * Array index: 26  GPIO_SIGNAL_WLAN_CLIENT_WAKE_B
     */
    { GPIO_MCU_PORT,        22, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * MCU GPIO Port Signal 21 -- WLAN power down
     * LJ7.1 Reference Design Signal: WLAN_PWR_DWN_B
     * Pin: GPIO31 Mux Setting: Func
     *
     * GPIO_MCU_21 is also on pin GPIO26 mux setting GPIO.
     *
     * Array index: 27  GPIO_SIGNAL_WLAN_PWR_DWN_B
     */
    { GPIO_MCU_PORT,        21, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 20 -- Vibrator Amplifier Enable
     * LJ7.1 Reference Design Signal: VIB_EN/ELEN1
     * Pin: GPIO25 Mux Setting: GPIO
     *
     * GPIO_MCU_20 is also on pin GPIO30 mux setting Func.
     *
     * Array index: 28  GPIO_SIGNAL_LIN_VIB_AMP_EN
     */
    { GPIO_MCU_PORT,        20, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 19 -- WDOG AP
     * LJ7.1 Reference Design Signal: WDOG_AP
     * Pin: GPIO24 Mux Setting: GPIO
     *
     * GPIO_MCU_19 is also on pin GPIO29 mux setting Func.
     *
     * Array index: 29  GPIO_SIGNAL_WDOG_AP
     */
    { GPIO_MCU_PORT,        19, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * MCU GPIO Port Signal 14 -- High-speed USB wakeup
     * LJ7.1 Reference Design Signal: USB_HS_WAKEUP
     * Pin: GPIO19 Mux Setting: GPIO
     *
     * GPIO_MCU_14 is also on pin GPIO14 mux setting Func.
     *
     * Array index: 30  GPIO_SIGNAL_USB_HS_WAKEUP
     */
    { GPIO_MCU_PORT,        14, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 12 -- High-speed USB interrupt
     * LJ7.1 Reference Design Signal: USB_HS_INT
     * Pin: SDMA_EVNT1 Mux Setting: Alt2
     *
     * GPIO_MCU_12 is also on pin GPIO12 mux setting Func and IPU_CSI_PIXCLK
     * mux setting GPIO.
     *
     * Array index: 31  GPIO_SIGNAL_USB_HS_INT
     */
    { GPIO_MCU_PORT,        12, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 11 -- Argon fusing power rail
     * LJ7.1 Reference Design Signal: VFUSE_EN
     * Pin: GPIO11 Mux Setting: Func
     *
     * GPIO_MCU_11 is also on pin SDMA_EVNT0 mux setting Alt2 and
     * pin IPU_CSI_DATA_9 mux setting GPIO.
     *
     * Array index: 32  GPIO_SIGNAL_VFUSE_EN
     *
     * Setting VFUSE_EN low causes reads to the efuses to hang the system.
     */
    { GPIO_MCU_PORT,        11, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /* 
     * MCU GPIO Port Signal 10 -- Listed in ICD as "Not used"
     * LJ7.1 Reference Design Signal: UART_DETECT
     * Pin: GPIO10 Mux Setting: Func
     *
     * GPIO_MCU_10 is also on pin GPIO35 mux setting Alt2 and IPU_CSI_DATA_8
     * mux setting GPIO.
     *
     * Array index: 33  GPIO_SIGNAL_UART_DETECT
     */
    { GPIO_MCU_PORT,        10, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /* 
     * MCU GPIO Port Signal 9 -- Atlas control
     * LJ7.1 Reference Design Signal: POWER_DVS0
     * Pin: GPIO9 Mux Setting: Func
     *
     * GPIO_MCU_9 is also on pin GPIO34 mux setting Alt2 and IPU_CSI_DATA_7
     * mux setting GPIO.
     *
     * NOTE: This pin is muxed to Alt5 mode at boot. Without changing the
     * mux setting, changes to this GPIO signal have no external effect.
     *
     * Array index: 34  GPIO_SIGNAL_POWER_DVS0
     */
    { GPIO_MCU_PORT,         9, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 7 -- Reset morphing controller.
     * LJ7.1 Reference Design Signal: MORPHING_RESET
     * Pin: GPIO7 Mux Setting: Func
     *
     * GPIO_MCU_7 is also on pin USB_XRXD mux setting GPIO.
     *
     * Array index: 35  GPIO_SIGNAL_TNLC_RESET
     */
    { GPIO_MCU_PORT,         7, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 6 -- MegaSIM enable
     * LJ7.1 Reference Design Signal: MEGA_SIM_EN
     * Pin: GPIO6 Mux Setting: Func
     *
     * GPIO_MCU_6 is also on pin USB_VMIN mux setting GPIO.
     *
     * Array index: 36  GPIO_SIGNAL_MEGA_SIM_EN
     */
    { GPIO_MCU_PORT,         6, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 5 -- Morphing RCHG
     * LJ7.1 Reference Design Signal: MORPHING_RCHG
     * Pin: GPIO5 Mux Setting: Func
     *
     * GPIO_MCU_5 is also on pin USB_VPOUT mux setting GPIO.
     *
     * Array index: 37  GPIO_SIGNAL_TNLC_RCHG
     */
    { GPIO_MCU_PORT,         5, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 4 -- Morphing KCHG
     * LJ7.1 Reference Design Signal: MORPHING_KCHG
     * Pin: GPIO4 Mux Setting: Func
     *
     * GPIO_MCU_4 is also on pin USB_VPIN mux setting GPIO.
     *
     * Array index: 38  GPIO_SIGNAL_TNLC_KCHG
     */
    { GPIO_MCU_PORT,         4, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 3 -- FM Interrupt
     * LJ7.1 Reference Design Signal: FM_INT
     * Pin: GPIO3 Mux Setting: Func
     *
     * GPIO_MCU_3 is also on pin USB_TXENB mux setting GPIO.
     *
     * Array index: 39  GPIO_SIGNAL_FM_INT
     */
    { GPIO_MCU_PORT,         3, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 2 -- WLAN host wake
     * LJ7.1 Reference Design Signal: WLAN_HOST_WAKE
     * Pin: GPIO2 Mux Setting: Func
     *
     * GPIO_MCU_2 is also on pin USB_VMOUT
     *
     * Array index: 40  GPIO_SIGNAL_WLAN_HOST_WAKE_B
     */
    { GPIO_MCU_PORT,         2, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Signal 1 -- High-speed USB reset
     * LJ7.1 Reference Design Signal: USB_HS_RESET
     * Pin: USB1_XRXD Mux Setting: GPIO
     *
     * Array index: 41  GPIO_SIGNAL_USB_HS_RESET
     */
    { GPIO_MCU_PORT,         1, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Signal 0 -- High-speed USB DMA request
     * LJ7.1 Reference Design Signal: USB_HS_DMA_REQ
     * Pin: USB1_VMIN Mux Setting: GPIO
     *
     * Array index: 42  GPIO_SIGNAL_USB_HS_DMA_REQ
     *
     * In GPIO chapter, this signal is indicated as output. IOMUX table
     * suggests it should be input, as it is on SCM-A11.
     */
    { GPIO_MCU_PORT,         0, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * Shared GPIO Port Signal 27 -- USB Transceiver Enable
     * LJ7.1 Reference Design Signal: USB_XCVR_EN
     * Pin: GPIO1 Mux Setting: Func
     *
     * GPIO_SHARED_27 is also on pin IPU_CSI_DATA_5 mux setting GPIO.
     *
     * Array index: 43  GPIO_SIGNAL_USB_XCVR_EN
     */
    { GPIO_SHARED_PORT,     27, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 26 -- Camera Flash Enable
     * LJ7.1 Reference Design Signal: CAM_FLASH_EN
     * Pin: GPIO0 Mux Setting: Func
     *
     * GPIO_SHARED_26 is also on pin IPU_CSI_DATA_4 mux setting GPIO.
     *
     * Array index: 44  GPIO_SIGNAL_CAM_FLASH_EN
     */
    { GPIO_SHARED_PORT,     26, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 23 -- WDOG
     * LJ7.1 Reference Design Signal: WDOG_B
     * Pin: WDOG_B Mux Setting: GPIO
     *
     * Array index: 45  GPIO_SIGNAL_WDOG_B
     */
    { GPIO_SHARED_PORT,     23, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * Shared GPIO Port Signal 22 -- GPS_PWR_EN
     * LJ7.1 Reference Design Signal: GPS_PWR_EN
     * Pin: USER_OFF Mux Setting: GPIO
     *
     * Array index: 46  GPIO_SIGNAL_GPS_PWR_EN
     */
    { GPIO_SHARED_PORT,     22, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 8 -- Muxing for WLAN and DBHD
     * LJ7.1 Reference Design Signal: WLAN_DVBH_MUX_CTL
     * Pin: GPIO38? Mux Setting: XXX
     *
     * Hardware team indicates that this signal is connected to pin GPIO38 and
     * is currently unused.
     *
     * Array index: 47  GPIO_SIGNAL_WLAN_DBHD_MUX
     */
    { GPIO_SHARED_PORT,      8, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 7 -- WLAN Reset
     * LJ7.1 Reference Design Signal: WLAN_RESET_B
     * Pin: GPIO27 Mux Setting: Func
     *
     * GPIO_SHARED_7 is also on pin IPU_CSI_DATA_3 mux setting GPIO.
     *
     * Array index: 48  GPIO_SIGNAL_WLAN_RESET
     */
    { GPIO_SHARED_PORT,      7, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 5 -- EL Enable
     * LJ7.1 Reference Design Signal: VIB_EN/ELEN1
     * Pin: GPIO25 Mux Setting: Func
     *
     * GPIO_SHARED_5 is also on pin IPU_CSI_DATA_1 mux setting GPIO.
     *
     * Array index: 49  GPIO_SIGNAL_EL_EN
     */
    { GPIO_SHARED_PORT,      5, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 4 -- Secondary Display Backlight Enable
     * LJ7.1 Reference Design Signal: IPU_CLI_BKLT_EN_B
     * Pin: IPU_CSI_DATA_0 Mux Setting: GPIO
     *
     * GPIO_SHARED_4 is also on pin GPIO24 mux setting Func.
     *
     * Array index: 50  GPIO_SIGNAL_CLI_BKL
     */
    { GPIO_SHARED_PORT,      4, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Signal 2 -- Low battery
     * LJ7.1 Reference Design Signal: LOBAT_B
     * Pin: GPIO22 Mux Setting: Func
     *
     * GPIO_SHARED_2 is also on pin IPU_CSI_HSYNC mux setting GPIO.
     *
     * Array index: 51  GPIO_SIGNAL_LOBAT_B
     */
    { GPIO_SHARED_PORT,      2, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * Shared GPIO Port Signal 0 -- Power Ready
     * LJ7.1 Reference Design Signal: POWER_RDY
     * Pin: GPIO20 Mux Setting: Func
     *
     * Array index: 52  GPIO_SIGNAL_POWER_RDY
     */
    { GPIO_SHARED_PORT,      0, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },
    
    /*
     * Array index: 53  GPIO_SIGNAL_GPU_RESET
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
    
    /*
     * Array index: 54  GPIO_SIGNAL_GPU_DPD
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
};


#if defined(CONFIG_MOT_FEAT_GPIO_API_GPU)
/**
 * Adjust initial_gpio_settings to reflect the GPU GPIO configuration.
 */
void __init gpu_gpio_signal_fixup(void)
{
    /*
     * MCU GPIO Port Signal 28 -- GPU Reset
     * LJ7.1 Reference Design Signal: GPU_RESET
     * Pin: USB1_VMOUT Mux Setting: GPIO
     *
     * IPU signal: SERDES_RESET_B
     */
    initial_gpio_settings[GPIO_SIGNAL_SERDES_RESET_B].port  = GPIO_INVALID_PORT;

    initial_gpio_settings[GPIO_SIGNAL_GPU_RESET].port       = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_RESET].sig_no     = 28;
    initial_gpio_settings[GPIO_SIGNAL_GPU_RESET].out        = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_RESET].data       = GPIO_DATA_INVALID;
    
    /*
     * Shared GPIO Port Signal 4 -- GPU Deep Power Down
     * LJ7.1 Reference Design Signal: GPU_DPD
     * Pin: IPU_CSI_DATA_0 Mux Setting: GPIO
     *
     * IPU signal: IPU_CLI_BKLT_EN_B
     */
    initial_gpio_settings[GPIO_SIGNAL_CLI_BKL].port         = GPIO_INVALID_PORT;
    
    initial_gpio_settings[GPIO_SIGNAL_GPU_DPD].port         = GPIO_SHARED_PORT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_DPD].sig_no       = 4;
    initial_gpio_settings[GPIO_SIGNAL_GPU_DPD].out          = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_DPD].data         = GPIO_DATA_INVALID;

    /* 
     * MCU GPIO Port Signal 23 -- GPU VCore Enable
     * LJ7.1 Reference Design Signal: LCD_SD
     * Pin: GPIO36 Mux Setting: Func
     *
     * IPU signal: LCD_SD
     */
    initial_gpio_settings[GPIO_SIGNAL_LCD_SD].port          = GPIO_INVALID_PORT;
    
    initial_gpio_settings[GPIO_SIGNAL_GPU_CORE1_EN].port    = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_CORE1_EN].sig_no  = 23;
    initial_gpio_settings[GPIO_SIGNAL_GPU_CORE1_EN].out     = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_CORE1_EN].data    = GPIO_DATA_INVALID;

    /*
     * MCU GPIO Port Signal 31 -- APS_CLK_B_EN
     * LJ7.1 Reference Design Signal: APS_CLK_B_EN
     * Pin: USB1_VPOUT Mux Setting: GPIO
     *
     * IPU signal: GPIO_CLI_RST_B
     */
    initial_gpio_settings[GPIO_SIGNAL_CLI_RST_B].port       = GPIO_INVALID_PORT;

    initial_gpio_settings[GPIO_SIGNAL_APP_CLK_EN_B].port    = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_APP_CLK_EN_B].sig_no  = 31;
    initial_gpio_settings[GPIO_SIGNAL_APP_CLK_EN_B].out     = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_APP_CLK_EN_B].data    = GPIO_DATA_INVALID;

    /*
     * MCU GPIO Port Signal 30 -- GPU A2
     * LJ7.1 Reference Design Signal: GPU_A2
     * Pin: USB1_VPIN Mux Setting: GPIO
     *
     * IPU signal: LCD_BACKLIGHT
     */
    initial_gpio_settings[GPIO_SIGNAL_LCD_BACKLIGHT].port   = GPIO_INVALID_PORT;

    initial_gpio_settings[GPIO_SIGNAL_GPU_A2].port          = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_A2].sig_no        = 30;
    initial_gpio_settings[GPIO_SIGNAL_GPU_A2].out           = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_A2].data          = GPIO_DATA_INVALID;

    /*
     * MCU GPIO Port Signal 29 -- GPU Interrupt
     * LJ7.1 Reference Design Signal: GPU_INT_B
     * Pin: USB1_TXENB Mux Setting: GPIO
     *
     * IPU signal: STBY_B
     */
    initial_gpio_settings[GPIO_SIGNAL_STBY].port            = GPIO_INVALID_PORT;

    initial_gpio_settings[GPIO_SIGNAL_GPU_INT_B].port       = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_INT_B].sig_no     = 29;
    initial_gpio_settings[GPIO_SIGNAL_GPU_INT_B].out        = GPIO_GDIR_INPUT;
    initial_gpio_settings[GPIO_SIGNAL_GPU_INT_B].data       = GPIO_DATA_INVALID;

    return;
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_GPU */
