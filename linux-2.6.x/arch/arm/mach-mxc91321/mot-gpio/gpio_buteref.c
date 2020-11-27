/*
 * GPIO and IOMUX Configuration for ArgonLV-based Bute
 * 
 * linux/arch/arm/mach-mxc91321/gpio_buteref.c
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
 * 04-Oct-2006  Motorola        Initial revision.
 * 03-Nov-2006  Motorola        Split up into peripheral related files.
 * 28-Nov-2006  Motorola        Created place holders for new GPU signals.
 */

#include <linux/config.h>
#include <linux/module.h>

#ifdef CONFIG_MOT_FEAT_BRDREV
#include <asm/boardrev.h>
#endif /* CONFIG_MOT_FEAT_BRDREV */

#include "mot-gpio-argonlv.h"


/*
 * Local functions.
 */
#ifdef CONFIG_MOT_FEAT_BRDREV
static void gpio_setting_fixup_p3_brassboard(void);
static void gpio_setting_fixup_p4(void);
#endif /* CONFIG_MOT_FEAT_BRDREV */


/**
 * Initial GPIO register settings.
 */
struct gpio_signal_settings initial_gpio_settings[MAX_GPIO_SIGNAL] = {
    /*
     * MCU GPIO Port Pin 1 -- Secondary Display Reset (active low)
     * ButeP3 (brass) Signal: GPIO_CLI_RST_B
     * ButeP4 (close) Signal: GPIO_CLI_RST_B
     * ButeP4 (wing)  Signal: GPIO_CLI_RST_B/GPS_RESET_B
     * ButeP5 (close) Signal: BT_RX
     * Pin: USB1_XRXD Mux Setting: GPIO
     *
     * Array index:  0  GPIO_SIGNAL_CLI_RST_B
     */
    { GPIO_INVALID_PORT,     1, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 8 -- Ethernet Interrupt
     * ButeP3 (brass) Signal: ENET_INT_B
     * ButeP4 (close) Signal: GPIO_CAM_Flash_T_F
     * ButeP4 (wing)  Signal: GPIO_CAM_Flash_T_F
     * ButeP5 (close) Signal: GPU_VCORE2_EN/PWGT2EN
     * Pin: GPIO8 Mux Setting: Func
     *
     * GPIO_MCU_8 also on pin IPU_CSI_DATA_6 mux setting GPIO and
     * pin GPIO33 mux setting Mux2.
     *
     * Array index:  1  GPIO_SIGNAL_ENET_INT_B
     */
    { GPIO_INVALID_PORT,     8, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 14 -- IRDA Shutdown
     * ButeP3 (brass) Signal: IRDA_SD
     * ButeP4 (close) Signal: LCD_SD
     * ButeP4 (wing)  Signal: LCD_SD
     * ButeP5 (close) Signal: LCD_SD
     * Pin: GPIO14 Mux Setting: Func
     *
     * GPIO_MCU_14 also on pin GPIO19 mux setting GPIO.
     *
     * Array index:  2  GPIO_SIGNAL_IRDA_SD
     */
    { GPIO_INVALID_PORT,    14, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 15 -- 2MP Imager Power Down (set high to disable)
     * ButeP3 (brass) Signal: CSI_CS0
     * ButeP4 (close) Signal: GPIO_CAM_EXT_PWRDN
     * ButeP4 (wing)  Signal: GPIO_CAM_EXT_PWRDN
     * ButeP5 (close) Signal: GPIO_CAM_EXT_PWRDN
     * Pin: GPIO15 Mux Setting: Func
     *
     * GPIO_MCU_15 also on pin GPIO22 mux setting GPIO.
     *
     * Array index:  3  GPIO_SIGNAL_CAM_EXT_PWRDN
     */
    { GPIO_MCU_PORT,        15, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * MCU GPIO Port Pin 16 -- 1.3MP Imager Power Down (set high to disable)
     * ButeP3 (brass) Signal: CSI_CS1
     * ButeP4 (close) Signal: GPIO_CAM_INT_PWRDN
     * ButeP4 (wing)  Signal: GPIO_CAM_INT_PWRDN
     * ButeP5 (close) Signal: GPIO_CAM_INT_PWRDN
     * Pin: GPIO16 Mux Setting: Func
     *
     * Array index:  4  GPIO_SIGNAL_CAM_INT_PWRDN
     */
    { GPIO_MCU_PORT,        16, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },


    /*
     * MCU GPIO Port Pin 17 -- SDHC Port 1 Card Detect
     * ButeP3 (brass) Signal: SD1_DET_B
     * ButeP4 (close) Signal: SD1_DET_B
     * ButeP4 (wing)  Signal: SD1_DET_B
     * ButeP5 (close) Signal: SD1_DET_B
     * Pin: GPIO17 Mux Setting: Func
     *
     * GPIO_MCU_17 also on pin GPIO22 mux setting GPIO.
     *
     * Array index:  5  GPIO_SIGNAL_SD1_DET_B
     */
    { GPIO_MCU_PORT,        17, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 24 -- Flip Detect
     * ButeP3 (brass) Signal: FLIP_DETECT
     * ButeP4 (close) Signal: FLIP_DETECT
     * ButeP4 (wing)  Signal: FLIP_DETECT
     * ButeP5 (close) Signal: FLIP_DETECT
     * Pin: GPIO37 Mux Setting: Func
     *
     * GPIO_MCU_24 also on pin SIM1_RST0 mux setting GPIO.
     *
     * Array index:  6  GPIO_SIGNAL_FLIP_DETECT
     */
    { GPIO_MCU_PORT,        24, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 28 -- Main Display Reset
     * ButeP3 (brass) Signal: GPIO_DISP_RST_B
     * ButeP4 (close) Signal: SERDES_RESET_B
     * ButeP4 (wing)  Signal: SERDES_RESET_B
     * ButeP5 (close) Signal: SERDES_RESET_B/GPU_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     *
     * Array index:  7  GPIO_SIGNAL_DISP_RST_B
     *
     * This line must toggle from high to low to enable the display.
     */
    { GPIO_INVALID_PORT,    28, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 29 -- Main Display Color Mode (low=high color mode)
     * ButeP3 (brass) Signal: GPIO_DISP_CM
     * ButeP4 (close) Signal: STBY_B
     * ButeP4 (wing)  Signal: STBY_B
     * ButeP5 (close) Signal: STBY_B/GPU_INT_B
     * Pin: USB1_TXENB Mux Setting: GPIO
     *
     * Array index:  8  GPIO_SIGNAL_DISP_CM
     */
    { GPIO_INVALID_PORT,    29, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 30 -- Main Display Backlight Enable
     * ButeP3 (brass) Signal: IPU_MAIN_BKLT_EN_B
     * ButeP4 (close) Signal: LCD_Backlight
     * ButeP4 (wing)  Signal: LCD_Backlight
     * ButeP5 (brass) Signal: LCD_Backlight/GPU_ADDR_LATCH
     * Pin: USB1_VPIN Mux Setting: GPIO
     *
     * Array index:  9  GPIO_SIGNAL_LCD_BACKLIGHT
     */
    { GPIO_INVALID_PORT,    30, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * Shared GPIO Port Pin 1 -- Bluetooth Host Wake
     * ButeP3 (brass) Signal: BT_HOST_WAKE_B
     * ButeP4 (close) Signal: BT_HOST_WAKE_B
     * ButeP4 (wing)  Signal: BT_HOST_WAKE_B
     * ButeP5 (close) Signal: BT_HOST_WAKE_B
     * Pin: GPIO21 Mux Setting: Func
     *
     * GPIO_Shared_1 also on pin IPU_CSI_MCLK mux setting GPIO.
     *
     * Array index: 10  GPIO_SIGNAL_BT_HOST_WAKE_B
     */
    { GPIO_SHARED_PORT,      1, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * Shared GPIO Port Pin 3 -- Camera Reset
     * ButeP3 (brass) Signal: GPIO_CAM_RST_B
     * ButeP4 (close) Signal: GPIO_CAM_RST_B
     * ButeP4 (wing)  Signal: GPIO_CAM_RST_B
     * ButeP5 (close) Signal: GPIO_CAM_RST_B
     * Pin: GPIO23 Mux Setting: Func
     *
     * GPIO_Shared_3 also on pin IPU_CSI_VSYNC mux setting GPIO.
     *
     * Array index: 11  GPIO_SIGNAL_CAM_RST_B
     */
    { GPIO_SHARED_PORT,      3, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * Shared GPIO Port Pin 6 -- Bluetooth Controller Wake
     * ButeP3 (brass) Signal: BT_WAKE_B
     * ButeP4 (close) Signal: BT_WAKE_B
     * ButeP4 (wing)  Signal: BT_WAKE_B
     * ButeP5 (close) Signal: BT_WAKE_B
     * Pin: GPIO26 Mux Setting: Func
     *
     * GPIO_Shared_6 also on pin IPU_CSI_DATA_2 mux setting GPIO.
     *
     * Array index: 12  GPIO_SIGNAL_BT_WAKE_B
     */
    { GPIO_SHARED_PORT,      6, GPIO_GDIR_OUTPUT, GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Pin 13 -- Bluetooth Reset
     * ButeP3 (brass) Signal: WLAN_CLIENT_WAKE_B
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: BT_RESET_B
     * Pin: GPIO13 Mux Setting: Func
     *
     * GPIO_MCU_13 also on pin GPIO18 mux setting GPIO.
     *
     * Array index: 13  GPIO_SIGNAL_BT_POWER
     */
    { GPIO_MCU_PORT,        13, GPIO_GDIR_OUTPUT, GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Pin 14 -- Main Display Shut Down
     * ButeP3 (brass) Signal: IRDA_SD
     * ButeP4 (close) Signal: LCD_SD
     * ButeP4 (wing)  Signal: LCD_SD
     * ButeP5 (close) Signal: LCD_SD
     * Pin: GPIO14 Mux Setting: Func
     *
     * GPIO_MCU_14 also on pin GPIO19 mux setting GPIO.
     *
     * Array index: 14  GPIO_SIGNAL_LCD_SD
     */
    { GPIO_MCU_PORT,        14, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 28 -- Main Display Serializer/Deserializer Enable
     * ButeP3 (brass) Signal: GPIO_DISP_RST_B
     * ButeP4 (close) Signal: SERDES_RESET_B 
     * ButeP4 (wing)  Signal: SERDES_RESET_B
     * ButeP5 (close) Signal: SERDES_RESET_B/GPU_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     *
     * Array index: 15  GPIO_SIGNAL_SERDES_RESET_B
     *
     * Set high to enable serializer at boot.
     */
    { GPIO_INVALID_PORT,    28, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 29 -- Serializer Standby (in low-power mode when low)
     * ButeP3 (brass) Signal: GPIO_DISP_CM
     * ButeP4 (close) Signal: STBY_B
     * ButeP4 (wing)  Signal: STBY_B
     * ButeP5 (close) Signal: STBY_B/GPU_INT_B
     * Pin: USB1_TXENB Mux Setting: GPIO
     *
     * Array index: 16  GPIO_SIGNAL_STBY
     *
     * Set low to put serializer into standby mode at boot.
     */
    { GPIO_INVALID_PORT,    29, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 23 -- GPU Deep-Power-Down
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: GPU_DPD_B
     * Pin: GPIO36 Mux Setting: Func
     *
     * GPIO_MCU_23 is also on pin SIM1_CLK0 mux setting GPIO.
     *
     * Array index: 17  GPIO_SIGNAL_GPU_DPD_B
     */
    { GPIO_MCU_PORT,            23, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 28 -- GPU Reset
     * ButeP3 (brass) Signal: GPIO_DISP_RST_B
     * ButeP4 (close) Signal: SERDES_RESET_B 
     * ButeP4 (wing)  Signal: SERDES_RESET_B
     * ButeP5 (close) Signal: SERDES_RESET_B/GPU_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     *
     * Array index: 18  GPIO_SIGNAL_GPU_RESET_B
     */
    { GPIO_MCU_PORT,            28, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 31 -- APPS_CLK_EN_B
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: APPS_CLK_EN_B
     * Pin: USB1_VPOUT Mux Setting: GPIO
     *
     * Array index: 19  GPIO_SIGNAL_APP_CLK_EN_B
     */
    { GPIO_MCU_PORT,            31, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 12 -- GPS Reset
     * ButeP4 (close) Signal: KPD_EL_EN2
     * ButeP4 (wing)  Signal: KPD_EL_EN2
     * ButeP5 (close) Signal: GPS_RESET_B
     * Pin: GPIO12 Mux Setting: Func
     *
     * GPIO_MCU_12 is also available pin SDMA_EVNT1 mux setting Alt2 and
     * pin IPU_CSI_PIXCLK mux setting GPIO.
     *
     * Array index: 20  GPIO_SIGNAL_GPS_RESET
     */
    { GPIO_MCU_PORT,            12, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * MCU GPIO Port Pin 7 -- SW_GPU_CORE1
     * ButeP3 (brass) Signal: GPS_RESET_B
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: GPU_VCORE1_EN/PWGT1EN
     * Pin: GPIO7 Mux Setting: Func
     *
     * Array index: 21  GPIO_SIGNAL_GPU_VCORE1_EN
     */
    { GPIO_MCU_PORT,             7, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * MCU GPIO Port Pin 8 -- SW_GPU_CORE2
     * ButeP3 (brass) Signal: ENET_INT_B
     * ButeP4 (close) Signal: GPIO_CAM_Flash_T_F
     * ButeP4 (wing)  Signal: GPIO_CAM_Flash_T_F
     * ButeP5 (close) Signal: GPU_VCORE2_EN/PWGT2EN
     * Pin: GPIO8 Mux Setting: Func
     *
     * Array index: 22  GPIO_SIGNAL_GPU_VCORE2_EN
     */
    { GPIO_MCU_PORT,             8, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },
    
    /* 
     * Array index: 23  GPIO_SIGNAL_GPU_A2
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 24  GPIO_SIGNAL_GPU_INT_B
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 25  GPIO_SIGNAL_GPU_CORE1_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 26  GPIO_SIGNAL_WLAN_CLIENT_WAKE_B
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 27  GPIO_SIGNAL_WLAN_PWR_DWN_B
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 28  GPIO_SIGNAL_LIN_VIB_AMP_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 29  GPIO_SIGNAL_WDOG_AP
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 30  GPIO_SIGNAL_USB_HS_WAKEUP
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 31  GPIO_SIGNAL_USB_HS_INT
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 32  GPIO_SIGNAL_VFUSE_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 33  GPIO_SIGNAL_UART_DETECT
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 34  GPIO_SIGNAL_POWER_DVS0
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 35  GPIO_SIGNAL_TNLC_RESET
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 36  GPIO_SIGNAL_MEGA_SIM_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 37  GPIO_SIGNAL_TNLC_RCHG
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 38  GPIO_SIGNAL_TNLC_KCHG
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 39  GPIO_SIGNAL_FM_INT
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 40  GPIO_SIGNAL_WLAN_HOST_WAKE_B
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 41  GPIO_SIGNAL_USB_HS_RESET
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 42  GPIO_SIGNAL_USB_HS_DMA_REQ
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 43  GPIO_SIGNAL_USB_XCVR_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 44  GPIO_SIGNAL_CAM_FLASH_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 45  GPIO_SIGNAL_WDOG_B
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 46  GPIO_SIGNAL_GPS_PWR_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 47  GPIO_SIGNAL_WLAN_DBHD_MUX
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 48  GPIO_SIGNAL_WLAN_RESET
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 49  GPIO_SIGNAL_EL_EN
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 50  GPIO_SIGNAL_CLI_BKL
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 51  GPIO_SIGNAL_LOBAT_B
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },

    /*
     * Array index: 52  GPIO_SIGNAL_POWER_RDY
     */
    { GPIO_INVALID_PORT,         0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
    
    /*
     * Array index: 53  GPIO_SIGNAL_GPU_RESET
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
    
    /*
     * Array index: 54  GPIO_SIGNAL_GPU_DPD
     */
    { GPIO_INVALID_PORT,     0, GPIO_GDIR_INVALID,  GPIO_DATA_INVALID },
};


/**
 * Update the initial_gpio_settings array based on the board revision. This
 * function is called by argonlvphone_gpio_init() at boot.
 */
void __init buteref_gpio_signal_fixup(void)
{
#if defined(CONFIG_MOT_FEAT_BRDREV)
    if((boardrev() < BOARDREV_P4A) || (boardrev() == BOARDREV_UNKNOWN)) {
        gpio_setting_fixup_p3_brassboard();
    } else if(boardrev() < BOARDREV_P5A) {
        gpio_setting_fixup_p4();
    }
#endif /* CONFIG_MOT_FEAT_BRDREV */

    return;
}


#if defined(CONFIG_MOT_FEAT_BRDREV)
/**
 * Adjust initial_iomux_settings array to reflect P3 brassboard configuration.
 */
void __init gpio_setting_fixup_p3_brassboard(void)
{
    /* first go into P4 state */
    gpio_setting_fixup_p4();

    /*
     * MCU GPIO Port Pin 7 -- GPS Reset
     * ButeP3 (brass) Signal: GPS_RESET_B
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: GPU_VCORE1_EN/PWGT1EN
     * Pin: GPIO7 Mux Setting: Func
     *
     * GPIO_MCU_7 is also on pin USB_XRXD mux setting GPIO.
     */
    initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].port   = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].sig_no = 7;
    initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].data   = GPIO_DATA_LOW;

    /*
     * MCU GPIO Port Pin 8 -- Ethernet Interrupt
     * ButeP3 (brass) Signal: ENET_INT_B
     * ButeP4 (close) Signal: GPIO_CAM_Flash_T_F
     * ButeP4 (wing)  Signal: GPIO_CAM_Flash_T_F
     * ButeP5 (close) Signal: GPU_VCORE2_EN/PWGT2EN
     * Pin: GPIO8 Mux Setting: Func
     */
    initial_gpio_settings[GPIO_SIGNAL_ENET_INT_B].port      = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_ENET_INT_B].sig_no    = 8;
    initial_gpio_settings[GPIO_SIGNAL_ENET_INT_B].out       = GPIO_GDIR_INPUT;
    initial_gpio_settings[GPIO_SIGNAL_ENET_INT_B].data      = GPIO_DATA_INVALID;

    /*
     * MCU GPIO Port Pin 14 -- IRDA Shutdown
     * ButeP3 (brass) Signal: IRDA_SD
     * ButeP4 (close) Signal: LCD_SD
     * ButeP4 (wing)  Signal: LCD_SD
     * ButeP5 (close) Signal: LCD_SD
     * Pin: GPIO14 Mux Setting: Func
     */
    initial_gpio_settings[GPIO_SIGNAL_IRDA_SD].port     = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_IRDA_SD].sig_no   = 14;
    initial_gpio_settings[GPIO_SIGNAL_IRDA_SD].out      = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_IRDA_SD].data     = GPIO_DATA_HIGH;

    /*
     * MCU GPIO Port Pin 28 -- Main Display Reset
     * ButeP3 (brass) Signal: GPIO_DISP_RST_B
     * ButeP4 (close) Signal: SERDES_RESET_B
     * ButeP4 (wing)  Signal: SERDES_RESET_B
     * ButeP5 (close) Signal: SERDES_RESET_B/GPU_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     *
     * This line must toggle from high to low to enable the display.
     */
    initial_gpio_settings[GPIO_SIGNAL_DISP_RST_B].port      = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_DISP_RST_B].sig_no    = 28;
    initial_gpio_settings[GPIO_SIGNAL_DISP_RST_B].out       = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_DISP_RST_B].data      = GPIO_DATA_HIGH;

    /*
     * MCU GPIO Port Pin 29 -- Main Display Color Mode (low=high color mode)
     * ButeP3 (brass) Signal: GPIO_DISP_CM
     * ButeP4 (close) Signal: STBY_B
     * ButeP4 (wing)  Signal: STBY_B
     * ButeP5 (close) Signal: STBY_B/GPU_INT_B
     * Pin: USB1_TXENB Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_DISP_CM].port     = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_DISP_CM].sig_no   = 29;
    initial_gpio_settings[GPIO_SIGNAL_DISP_CM].out      = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_DISP_CM].data     = GPIO_DATA_LOW;

    /*
     * MCU GPIO Port Pin 14 -- Main Display Shut Down
     * ButeP3 (brass) Signal: IRDA_SD
     * ButeP4 (close) Signal: LCD_SD
     * ButeP4 (wing)  Signal: LCD_SD
     * ButeP5 (close) Signal: LCD_SD
     * Pin: GPIO14 Mux Setting: Func
     *
     * GPIO_MCU_14 also on pin GPIO19 mux setting GPIO.
     */
    initial_gpio_settings[GPIO_SIGNAL_LCD_SD].port  = GPIO_INVALID_PORT;

    /*
     * MCU GPIO Port Pin 28 -- Main Display Serializer/Deserializer Enable
     * ButeP3 (brass) Signal: GPIO_DISP_RST_B
     * ButeP4 (close) Signal: SERDES_RESET_B 
     * ButeP4 (wing)  Signal: SERDES_RESET_B
     * ButeP5 (close) Signal: SERDES_RESET_B/GPU_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_SERDES_RESET_B].port  = GPIO_INVALID_PORT;

    /*
     * MCU GPIO Port Pin 29 -- Serializer Standby (in low-power mode when low)
     * ButeP3 (brass) Signal: GPIO_DISP_CM
     * ButeP4 (close) Signal: STBY_B
     * ButeP4 (wing)  Signal: STBY_B
     * ButeP5 (close) Signal: STBY_B/GPU_INT_B
     * Pin: USB1_TXENB Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_STBY].port    = GPIO_INVALID_PORT;
}


/**
 * Adjust initial_gpio_settings array to reflect P4 closed-phone and
 * wingboard.
 */
void __init gpio_setting_fixup_p4(void)
{
    /*
     * MCU GPIO Port Pin 1 -- Secondary Display Reset (active low)
     * ButeP3 (brass) Signal: GPIO_CLI_RST_B
     * ButeP4 (close) Signal: GPIO_CLI_RST_B
     * ButeP4 (wing)  Signal: GPIO_CLI_RST_B/GPS_RESET_B
     * ButeP5 (close) Signal: BT_RX
     * Pin: USB1_XRXD Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_CLI_RST_B].port   = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_CLI_RST_B].sig_no = 1;
    initial_gpio_settings[GPIO_SIGNAL_CLI_RST_B].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_CLI_RST_B].data   = GPIO_DATA_LOW;

    /* Pin USB1_XRXD is also used to control GPS_RESET_B on some modified
     * P4A Wingboards. This means there is a conflict between the ability
     * to control the secondary display and the GPS module. */
    if( (boardrev() >= BOARDREV_P4AW) && (boardrev() < BOARDREV_P5A) ) {
        /*
         * MCU GPIO Port Pin 1 -- Secondary Display Reset (active low)
         * ButeP3 (brass) Signal: GPIO_CLI_RST_B
         * ButeP4 (close) Signal: GPIO_CLI_RST_B
         * ButeP4 (wing)  Signal: GPIO_CLI_RST_B/GPS_RESET_B
         * ButeP5 (close) Signal: BT_RX
         * Pin: USB1_XRXD Mux Setting: GPIO
         */
        initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].port   = GPIO_MCU_PORT;
        initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].sig_no = 1;
        initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].out    = GPIO_GDIR_OUTPUT;
        initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].data   = GPIO_DATA_INVALID;
    } else {
        /*
         * MCU GPIO Port Pin 12 -- GPS Reset
         * ButeP4 (close) Signal: KPD_EL_EN2
         * ButeP4 (wing)  Signal: KPD_EL_EN2
         * ButeP5 (close) Signal: GPS_RESET_B
         * Pin: GPIO12 Mux Setting: Func
         */
        initial_gpio_settings[GPIO_SIGNAL_GPS_RESET].port   = GPIO_INVALID_PORT;
    }

    /*
     * MCU GPIO Port Pin 30 -- Main Display Backlight Enable
     * ButeP3 (brass) Signal: IPU_MAIN_BKLT_EN_B
     * ButeP4 (close) Signal: LCD_Backlight
     * ButeP4 (wing)  Signal: LCD_Backlight
     * ButeP5 (brass) Signal: LCD_Backlight/GPU_ADDR_LATCH
     * Pin: USB1_VPIN Mux Setting: GPIO
     *
     * Configured by MBM at boot time.
     */
    initial_gpio_settings[GPIO_SIGNAL_LCD_BACKLIGHT].port   = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_LCD_BACKLIGHT].sig_no = 30;
    initial_gpio_settings[GPIO_SIGNAL_LCD_BACKLIGHT].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_LCD_BACKLIGHT].data   = GPIO_DATA_INVALID;

    /*
     * Shared GPIO Port Pin 7 -- Bluetooth Reset
     * ButeP3 (brass) Signal: BT_RESET_B
     * ButeP4 (close) Signal: BT_RESET_B
     * ButeP4 (wing)  Signal: BT_RESET_B
     * ButeP5 (close) Signal: NC
     * Pin: GPIO27 Mux Setting: Func
     *
     * GPIO_Shared_7 also on pin IPU_CSI_DATA_3 mux setting GPIO.
     */
    initial_gpio_settings[GPIO_SIGNAL_BT_POWER].port    = GPIO_SHARED_PORT;
    initial_gpio_settings[GPIO_SIGNAL_BT_POWER].sig_no  = 7;
    initial_gpio_settings[GPIO_SIGNAL_BT_POWER].out     = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_BT_POWER].data    = GPIO_DATA_LOW;
    
    /*
     * MCU GPIO Port Pin 28 -- Main Display Serializer/Deserializer Enable
     * ButeP3 (brass) Signal: GPIO_DISP_RST_B
     * ButeP4 (close) Signal: SERDES_RESET_B 
     * ButeP4 (wing)  Signal: SERDES_RESET_B
     * ButeP5 (close) Signal: SERDES_RESET_B/GPU_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_SERDES_RESET_B].port   = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_SERDES_RESET_B].sig_no = 28;
    initial_gpio_settings[GPIO_SIGNAL_SERDES_RESET_B].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_SERDES_RESET_B].data   = GPIO_DATA_HIGH;

    /*
     * MCU GPIO Port Pin 29 -- Serializer Standby (in low-power mode when low)
     * ButeP3 (brass) Signal: GPIO_DISP_CM
     * ButeP4 (close) Signal: STBY_B
     * ButeP4 (wing)  Signal: STBY_B
     * ButeP5 (close) Signal: STBY_B/GPU_INT_B
     * Pin: USB1_TXENB Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_STBY].port    = GPIO_MCU_PORT;
    initial_gpio_settings[GPIO_SIGNAL_STBY].sig_no  = 29;
    initial_gpio_settings[GPIO_SIGNAL_STBY].out     = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_STBY].data    = GPIO_DATA_LOW;

    /*
     * MCU GPIO Port Pin 7 -- SW_GPU_CORE1
     * ButeP3 (brass) Signal: GPS_RESET_B
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: GPU_VCORE1_EN/PWGT1EN
     * Pin: GPIO7 Mux Setting: Func
     */
    initial_gpio_settings[GPIO_SIGNAL_GPU_VCORE1_EN].port   = GPIO_INVALID_PORT;

    /*
     * MCU GPIO Port Pin 8 -- SW_GPU_CORE2
     * ButeP3 (brass) Signal: ENET_INT_B
     * ButeP4 (close) Signal: GPIO_CAM_Flash_T_F
     * ButeP4 (wing)  Signal: GPIO_CAM_Flash_T_F
     * ButeP5 (close) Signal: GPU_VCORE2_EN/PWGT2EN
     * Pin: GPIO8 Mux Setting: Func
     */
    initial_gpio_settings[GPIO_SIGNAL_GPU_VCORE2_EN].port   = GPIO_INVALID_PORT;

    /*
     * MCU GPIO Port Pin 23 -- GPU Deep-Power-Down
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: GPU_DPD_B
     * Pin: GPIO36 Mux Setting: Func
     */
    initial_gpio_settings[GPIO_SIGNAL_GPU_DPD_B].port   = GPIO_INVALID_PORT;

    /*
     * MCU GPIO Port Pin 28 -- GPU Reset
     * ButeP3 (brass) Signal: GPIO_DISP_RST_B
     * ButeP4 (close) Signal: SERDES_RESET_B 
     * ButeP4 (wing)  Signal: SERDES_RESET_B
     * ButeP5 (close) Signal: SERDES_RESET_B/GPU_RESET_B
     * Pin: USB1_VMOUT Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_GPU_RESET_B].port = GPIO_INVALID_PORT;

    /*
     * MCU GPIO Port Pin 31 -- APPS_CLK_EN_B
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: APPS_CLK_EN_B
     * Pin: USB1_VPOUT Mux Setting: GPIO
     */
    initial_gpio_settings[GPIO_SIGNAL_APP_CLK_EN_B].port    = GPIO_INVALID_PORT;
}
#endif /* CONFIG_MOT_FEAT_BRDREV */
