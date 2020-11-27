/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/gpio_scma11ref.c
 *
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * 13-Nov-2006  Motorola        Change pad group 25 settings.
 * 26-Jan-2007  Motorola        Bluetooth current drain improvements.
 * 29-Jan-2007  Motorola        Added support for P3C wingboards.
 * 02-Apr-2007  Motorola        Pad group 10 changed to 6.
 */

#include <linux/config.h>
#include <asm/mot-gpio.h>
#ifdef CONFIG_MOT_FEAT_BRDREV
#include <asm/boardrev.h>
#endif /* CONFIG_MOT_FEAT_BRDREV */

#include "../iomux.h"
#include "mot-gpio-scma11.h"

#ifdef CONFIG_MOT_FEAT_BRDREV
/* adjust GPIO signals to reflect earlier hardware revisions */
static void gpio_signal_fixup_p2aw(void);
static void gpio_signal_fixup_p2bw(void);

static void gpio_signal_fixup_p3aw(void);
#endif /* CONFIG_MOT_FEAT_BRDREV */


/**
 * Initial GPIO register settings.
 */
struct gpio_signal_settings initial_gpio_settings[MAX_GPIO_SIGNAL] = {
    /*
     * SCM-A11 Package Pin Name: U3_RTS_B
     * SCM-A11 Reference P1A Wingboard Signal: GPS_U3_RTS_B (GPS)
     * SCM-A11 Reference P1D Wingboard Signal: GPS_U3_RTS_B (GPS)
     * SCM-A11 Reference P2B Wingboard Signal: GPS_RESET (GPS)
     * Selected Primary Function: GP_SP_A2 (Output)
     *
     * Array index: 0   GPIO_SIGNAL_GPS_RESET
     *
     * Disable GPS at reset to avoid interference with UART3.
     */
    { GPIO_SP_A_PORT,    2, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C14
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_CLI_BKL (Backlight)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_CLI_BKL (Backlight)
     * Selected Primary Function: GP_AP_C14 (Output)
     *
     * Array index: 1   GPIO_SIGNAL_CLI_BKL
     *
     * Disable secondary display backlight at boot to prevent interference
     * with main display backlight.
     */
    { GPIO_AP_C_PORT,   14, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
    
    /*
     * SCM-A11 Package Pin Name: GP_AP_C8
     * SCM-A11 Reference P1A Wingboard Signal: BT_RESET_B (Bluetooth)
     * SCM-A11 Reference P1D Wingboard Signal: BT_RESET_B (Bluetooth)
     * SCM-A11 Reference P2B Wingboard Signal: BT_RESET_B (Bluetooth)
     * Selected Primary Function: GP_AP_C8 (Output)
     *
     * Array index: 2   GPIO_SIGNAL_BT_POWER
     *
     * Power off Bluetooth at boot time. (Signal is connected to Bluetooth's
     * VREG_CTL on P2A wingboard and later.)
     */
    { GPIO_AP_C_PORT,    8, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C8
     * SCM-A11 Reference P1A Wingboard Signal: BT_RESET_B (Bluetooth)
     * SCM-A11 Reference P1D Wingboard Signal: BT_RESET_B (Bluetooth)
     * SCM-A11 Reference P2B Wingboard Signal: BT_RESET_B (Bluetooth)
     * Selected Primary Function: GP_AP_C8 (Output)
     *
     * Array index: 3   GPIO_SIGNAL_BT_RESET_B
     *
     * On P1 wingboards, the BT controller is brought out of reset
     * at boot by setting GP_AP_C8 high and then powered down
     * by setting GP_AP_C12 low.
     */
    { GPIO_INVALID_PORT,     8, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: SPI1_MOSI
     * SCM-A11 Reference P1A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P1D Wingboard Signal: SPI1_MOSI (Wing Connector)
     * SCM-A11 Reference P2A Wingboard Signal: USB_HS_RESET (Wing Connector)
     * Selected Primary Function: GP_SP_A28 (Output)
     *
     * Array index: 4   GPIO_SIGNAL_USB_HS_RESET
     *
     * Put the high speed USB controller into reset at boot.
     */
    { GPIO_SP_A_PORT,   28, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_B22
     * SCM-A11 Reference P1A Wingboard Signal: GP_AP_B22 (Wing Connector)
     * SCM-A11 Reference P1D Wingboard Signal: GP_AP_B22 (Wing Connector)
     * Selected Primary Function: GP_AP_B22 (Input)
     *
     * Array index: 5   GPIO_SIGNAL_USB_HS_DMA_REQ
     */
    { GPIO_AP_B_PORT,   22, GPIO_GDIR_INPUT,  GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C9
     * SCM-A11 Reference P1A Wingboard Signal: BOOT_RS232_USB (Misc)
     * SCM-A11 Reference P1D Wingboard Signal: 10K Pulldown (Misc)
     * Selected Primary Function: GP_AP_C9 (Output)
     *
     * Array index: 6   GPIO_SIGNAL_USB_HS_WAKEUP
     *
     * Put the high speed USB controller into sleep mode.
     */
    { GPIO_AP_C_PORT,    9, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
    
    /*
     * SCM-A11 Package Pin Name: GP_AP_C10
     * SCM-A11 Reference P1A Wingboard Signal: USB_HS_DMA_ACK (Wing Connector)
     * SCM-A11 Reference P1D Wingboard Signal: USB_HS_DMA_ACK (Wing Connector)
     * Selected Primary Function: GP_AP_C10 (Input)
     *
     * Array index: 7   GPIO_SIGNAL_USB_HS_FLAGC
     */
    { GPIO_AP_C_PORT,   10, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: ED_INT6
     * SCM-A11 Reference P1A Wingboard Signal: USB_HS_INT (Wing Connector)
     * SCM-A11 Reference P1D Wingboard Signal: USB_HS_INT (Wing Connector)
     * Selected Primary Function: GP_AP_C24 (Input)
     * Selected Secondary Function: ED_INT6 (Input)
     *
     * Array index: 8   GPIO_SIGNAL_USB_HS_INT
     *
     * High speed USB interrupt; setup to receive interrupts later.
     */
    { GPIO_AP_C_PORT,   24, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: UH2_TXDP
     * SCM-A11 Reference P1A Wingboard Signal: IO_LED_DBG (Misc)
     * SCM-A11 Reference P1D Wingboard Signal: IO_LED_DBG (Misc)
     * SCM-A11 Reference P2B Wingboard Signal: USB_HS_SWITCH (Wing Connector)
     * Selected Primary Function: GP_SP_A11 (Output)
     *
     * Array index: 9   GPIO_SIGNAL_USB_HS_SWITCH
     *
     * USB connected to Atlas when low.
     */
    { GPIO_SP_A_PORT,   11, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: U2_RI_B
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_DISP_CM/DAI_CLK (Display)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_DISP_CM/DAI_CLK (Display)
     * Selected Primary Function: GP_AP_C27 (Output)
     *
     * Array index: 10  GPIO_SIGNAL_DISP_CM
     *
     * Main display color mode; set low for high color mode.
     */
    { GPIO_AP_C_PORT,   27, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: IPU_D0_CS
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_DISP_SD (Display)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_DISP_SD (Display)
     * Selected Primary Function: GP_AP_A26 (Output)
     *
     * Array index: 11  GPIO_SIGNAL_DISP_SD
     *
     * Set low to enable main display; high to shut down. Set by MBM at boot.
     */
    { GPIO_AP_A_PORT,   26, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_B17
     * SCM-A11 Reference P1A Wingboard Signal: PWM_BKL (Backlight)
     * SCM-A11 Reference P1D Wingboard Signal: PWM_BKL (Backlight)
     * Selected Primary Function: GP_AP_B17 (Output)
     *
     * Array index: 12  GPIO_SIGNAL_PWM_BKL
     *
     * This signal is no longer connected to the backlight driver for
     * P1A and later wingboards.
     */
    { GPIO_INVALID_PORT,    17, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: IPU_D3_SPL
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_MAIN_BKL (Backlight)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_MAIN_BKL (Backlight)
     * Selected Primary Function: GP_AP_A30 (Output)
     *
     * Array index: 13  GPIO_SIGNAL_MAIN_BKL
     *
     * Set high to turn on the main display backlight. Set by MBM at boot.
     */
    { GPIO_AP_A_PORT,   30, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: CSI_D0
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_CAM_RST_B (camera)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_CAM_RST_B (camera)
     * Selected Primary Function: GP_AP_B24 (Output)
     *
     * Array index: 14  GPIO_SIGNAL_CAM_RST_B
     *
     * Take camera out of reset at boot.
     */
    { GPIO_AP_B_PORT,   24, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: CSI_D1
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_CAM_PD (camera)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_CAM_PD (camera)
     * Selected Primary Function: GP_AP_B25 (Output)
     *
     * Array index: 15  GPIO_SIGNAL_CAM_PD
     *
     * Power down camera at boot.
     */
    { GPIO_AP_B_PORT,   25, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: ED_INT3
     * SCM-A11 Reference P1A Wingboard Signal: CPLD_IO_ENET_INT (Ethernet)
     * SCM-A11 Reference P1D Wingboard Signal: CPLD_IO_ENET_INT/(hall effect)
     *                                          (Ethernet)
     * Selected Primary Function: GP_AP_C21 (Input)
     * Selected Secondary Function: ED_INT3 (Input)
     *
     * Array index: 16  GPIO_SIGNAL_ENET_INT
     */
    { GPIO_AP_B_PORT,   21, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_B18
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_DISP_RST_B (CLI)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_DISP_RST_B (CLI)
     * Selected Primary Function: GP_AP_B18 (Output)
     *
     * Array index: 17  GPIO_SIGNAL_DISP_RST_B
     *
     * This is described as the secondary display reset in the ICD; however, for
     * P2A closedphone this line is used as the LCD clock select.
     */
    { GPIO_AP_B_PORT,   18, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: IPU_PAR_RS
     * SCM-A11 Reference P1A Wingboard Signal: IPU_PAR_RS (Display)
     * SCM-A11 Reference P1D Wingboard Signal: IPU_PAR_RS (Display)
     * SCM-A11 Reference P2B Wingboard Signal: SER_EN (Display)
     * Selected Primary Function: GP_AP_A29 (Output)
     *
     * Array index: 18  GPIO_SIGNAL_SER_EN
     *
     * Serializer enable; set high to enable. (Configured at boot by MBM.)
     */
    { GPIO_AP_A_PORT,   29, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: U3_CTS_B
     * SCM-A11 Reference P1A Wingboard Signal: VVIB_EN (Atlas)
     * SCM-A11 Reference P1D Wingboard Signal: VVIB_EN (Atlas)
     * SCM-A11 Reference P2B Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * SCM-A11 Reference P3C Wingboard Signal: WLAN_CLIENT_WAKE_B
     *                                          (WLAN)/UI_IC_DBG (Morphing)
     * Selected Primary Function: GP_SP_A3 (Output)
     *
     * Array index: 19  GPIO_SIGNAL_WLAN_CLIENT_WAKE_B
     *
     * Put WLAN to sleep at boot.
     */
    { GPIO_SP_A_PORT,    3, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: UH2_RXDM
     * SCM-A11 Reference P1A Wingboard Signal: TF_ENABLE (MMC)
     * SCM-A11 Reference P1D Wingboard Signal: TF_ENABLE (MMC)
     * SCM-A11 Reference P2B Wingboard Signal: CAM_TORCH_EN (Camera Flash)
     * Selected Primary Function: GP_SP_A13 (Output)
     *
     * Array index: 20  GPIO_SIGNAL_CAM_TORCH_EN
     */
    { GPIO_SP_A_PORT,   13, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_SP_A26
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_CLK_EN_B (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_CLK_EN_B (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: WLAN_PWR_DWN_B (WLAN)
     * Selected Primary Function: GP_SP_A26 (Input)
     *
     * Array index: 21  GPIO_SIGNAL_WLAN_PWR_DWN_B
     */
    { GPIO_SP_A_PORT,       26, GPIO_GDIR_OUTPUT,    GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: SPI1_CLK
     * SCM-A11 Reference P1A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P1D Wingboard Signal: NC (NC)
     * SCM-A11 Reference P2B Wingboard Signal: EL_NUM_EN (Backlight)
     * SCM-A11 Reference P3A Wingboard Signal: NC (NC)
     * Selected Primary Function: GP_SP_A27 (Output)
     *
     * Array index: 22  GPIO_SIGNAL_EL_NUM_EN
     */
    { GPIO_INVALID_PORT,    27, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: SPI1_MISO
     * SCM-A11 Reference P1A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P1D Wingboard Signal: NC (NC)
     * SCM-A11 Reference P2B Wingboard Signal: SPI1_MISO (Backlight)
     * SCM-A11 Reference P3A Wingboard Signal: EL_EN (Backlight)
     * Selected Primary Function: GP_SP_A29 (Output)
     *
     * Array index: 23  GPIO_SIGNAL_EL_NAV_EN
     */
    { GPIO_INVALID_PORT,    29, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C12
     * SCM-A11 Reference P1A Wingboard Signal: BT_REG_CTL (Bluetooth)
     * SCM-A11 Reference P1D Wingboard Signal: BT_REG_CTL (Bluetooth)
     * SCM-A11 Reference P2A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P2B Wingboard Signal: WLAN_RESET (WLAN)
     * Selected Primary Function: GP_AP_C12 (Output)
     *
     * Array index: 24   GPIO_SIGNAL_WLAN_RESET
     *
     * Take WLAN out of reset. (Put to sleep by WLAN_CLIENT_WAKE_B.)
     */
    { GPIO_AP_C_PORT,   12, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: ED_INT4
     * SCM-A11 Reference P1A Wingboard Signal: SD2_DET_B (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: SD2_DET_B (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: TF_DET (MMC)
     * Selected Primary Function: GP_AP_C22 (Input)
     * Selected Secondary Function: ED_INT4 (Input)
     *
     * Array index: 25  GPIO_SIGNAL_TF_DET
     */
    { GPIO_AP_C_PORT,   22, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C13
     * SCM-A11 Reference P1A Wingboard Signal: BT_WAKE_B (Bluetooth)
     * SCM-A11 Reference P1D Wingboard Signal: BT_WAKE_B (Bluetooth)
     * SCM-A11 Reference P2B Wingboard Signal: BT_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C13 (Output)
     *
     * Array index: 26  GPIO_SIGNAL_BT_WAKE_B
     *
     * Set low to wakeup blueooth.
     */
    { GPIO_AP_C_PORT,   13, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: ED_INT1
     * SCM-A11 Reference P1A Wingboard Signal: BT_HOST_WAKE_B (Bluetooth)
     * SCM-A11 Reference P1D Wingboard Signal: BT_HOST_WAKE_B (Bluetooth)
     * SCM-A11 Reference P2A Wingboard Signal: BT_HOST_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C19 (Input)
     * Selected Secondary Function: ED_INT1 (Input)
     *
     * Array index: 27  GPIO_SIGNAL_BT_HOST_WAKE_B
     *
     * Host wake interrupt from bluetooth controller.
     */
    { GPIO_AP_C_PORT,   19, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: U3_RTS_B
     * SCM-A11 Reference P1A Wingboard Signal: GPS_U3_RTS_B (GPS)
     * SCM-A11 Reference P1D Wingboard Signal: GPS_U3_RTS_B (GPS)
     * SCM-A11 Reference P2B Wingboard Signal: GPS_RESET (GPS)
     * SCM-A11 Reference P3C Wingboard Signal: GPS_RESET (GPS)/LIN_VIB_AMP_EN
     *                                           (Linear Vibrator)
     * Selected Primary Function: GP_SP_A2 (Output)
     *
     * Array index: 28  GPIO_SIGNAL_LIN_VIB_AMP_EN
     *
     * Set high to enable vibrator.
     */
    { GPIO_INVALID_PORT,     2, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: ED_INT0
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_HOST_WAKE_B (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_HOST_WAKE_B (WLAN)
     * Selected Primary Function: GP_AP_C18 (Input)
     * Selected Secondary Function: ED_INT0 (Input)
     *
     * Array index: 29  GPIO_SIGNAL_WLAN_HOST_WAKE_B
     */
    { GPIO_AP_C_PORT,   18, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: SPI1_MISO
     * SCM-A11 Reference P1A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P1D Wingboard Signal: NC (NC)
     * SCM-A11 Reference P2B Wingboard Signal: SPI1_MISO (Backlight)
     * SCM-A11 Reference P3A Wingboard Signal: EL_EN (Backlight)
     * Selected Primary Function: GP_SP_A29 (Output)
     *
     * Array index: 30  GPIO_SIGNAL_EL_EN
     */
    { GPIO_SP_A_PORT,       29, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_SPEED
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P2A Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: VFUSE_SELECT (Misc)
     * SCM-A11 Reference P3A Wingboard Signal: FM_RESET (FM Radio)
     * Selected Primary Function: GP_SP_A9 (Output)
     *
     * Array index: 31  GPIO_SIGNAL_FM_RESET
     */
    { GPIO_SP_A_PORT,        9, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C11
     * SCM-A11 Reference P1A Wingboard Signal: BT_CLK_EN_B (Bluetooth)
     * SCM-A11 Reference P1D Wingboard Signal: BT_CLK_EN_B (Bluetooth)
     * SCM-A11 Reference P2B Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3A Wingboard Signal: FM_INTERRUPT (FM Radio)
     * Selected Primary Function: GP_AP_C11 (Input)
     *
     * Array index: 32  GPIO_SIGNAL_FM_INTERRUPT
     */
    { GPIO_AP_C_PORT,       11, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: UH2_RXDP
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: UH2_RXDP (Saipan Connector)
     * SCM-A11 Reference P3A Wingboard Signal: UH2_RXDP (INTERRUPT) (Morphing)
     * SCM-A11 Reference P3C Wingboard Signal: TNLC_KCHG_INT (Morphing)
     * Selected Primary Function: GP_SP_A12 (Output)
     *
     * Array index: 33  GPIO_SIGNAL_TNLC_KCHG_INT
     */
    { GPIO_SP_A_PORT,       12, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: SPI1_SS1
     * SCM-A11 Reference P1A Wingboard Signal: USER_OFF (Atlas)
     * SCM-A11 Reference P1D Wingboard Signal: USER_OFF (Atlas)
     * SCM-A11 Reference P2B Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3A Wingboard Signal: SPI1_SS1 (2nd MORPH RESET)
     *                                                  (Morphing)
     * Selected Primary Function: GP_SP_A31 (Output)
     *
     * Array index: 34  GPIO_SIGNAL_TNLC_RESET
     */
    { GPIO_INVALID_PORT,    31, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_PWR
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_PWR_DWN_B (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_PWR_DWN_B (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: UH2_PWR (Saipan Connector)
     * SCM-A11 Reference P3A Wingboard Signal: UH2_RXDM (RESET) (Morphing)
     * Selected Primary Function: GP_SP_A15 (Output)
     * 
     * Array index: 35  GPIO_SIGNAL_CAP_RESET
     */
    { GPIO_SP_A_PORT,       15, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_TXOE_B
     * SCM-A11 Reference P1A Wingboard Signal: GPS_RESET (GPS)
     * SCM-A11 Reference P1D Wingboard Signal: GPS_RESET (GPS)
     * SCM-A11 Reference P2B Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3A Wingboard Signal: OMEGA_INTERRUPT (Omega Wheel)
     * SCM-A11 Reference P3C Wingboard Signal: TNLC_RCHG (Morphing)
     * Selected Primary Function: GP_SP_A8 (Output)
     *
     * Array index: 36  GPIO_SIGNAL_TNLC_RCHG
     */
    { GPIO_SP_A_PORT,        8, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: U1_TXD
     * SCM-A11 Reference P3A Signal: BT_RX_AP_TX (Bluetooth)
     * Selected Primary Function: U1_TXD (Output)
     * Mux0 Function: GP_AP_A7
     *
     * SCM-A11 Reference Design uses Broadcom Bluetooth controller with
     * internal pull downs, so drive BT pins low when inactive to decrease
     * current drain.
     *
     * Array index: 37  GPIO_SIGNAL_U1_TXD
     */
    { GPIO_AP_A_PORT,        7, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: U1_CTS_B
     * SCM-A11 Reference P3A Signal: BT_CTS_B (Bluetooth)
     * Selected Primary Function: U1_CTS_B (Output)
     * Mux0 Function: GP_AP_A10
     *
     * SCM-A11 Reference Design uses Broadcom Bluetooth controller with
     * internal pull downs, so drive BT pins low when inactive to decrease
     * current drain.
     *
     * Array index: 38  GPIO_SIGNAL_U1_CTS_B
     */
    { GPIO_AP_A_PORT,       10, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: U3_CTS_B
     * SCM-A11 Reference P2B Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * SCM-A11 Reference P3A Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * SCM-A11 Reference P3C Wingboard Signal: WLAN_CLIENT_WAKE_B
     *                                          (WLAN)/UI_IC_DBG (Morphing)
     * Selected Primary Function: GP_SP_A3 (Output)
     *
     * Array index: 39  GPIO_SIGNAL_UI_IC_DBG
     */
    { GPIO_INVALID_PORT,     3, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },
    
    /*
     * SCM-A11 Package Pin Name: SPI1_SS1
     * SCM-A11 Reference P1A Wingboard Signal: USER_OFF (Atlas)
     * SCM-A11 Reference P1D Wingboard Signal: USER_OFF (Atlas)
     * SCM-A11 Reference P2B Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3A Wingboard Signal: SPI1_SS1 (2nd MORPH RESET)
     *                                                  (Morphing)
     * SCM-A11 Reference P3C Wingboard Signal: FSS_HYST (Morphing)
     * Selected Primary Function: GP_SP_A31 (Output)
     *
     * Array index: 40  GPIO_SIGNAL_FSS_HYST
     */
    { GPIO_SP_A_PORT,   31, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: IPU_WR
     * SCM-A11 Reference P3A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3C Signal: GPIO_SER_RST_B (Display)
     * Selected Primary Function: GP_AP_B1 (Output)
     *
     * Array index: 41  GPIO_SIGNAL_SER_RST_B
     *
     * Low means serializer is in reset.
     */
    { GPIO_AP_B_PORT,    1, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },
    
    /*
     * SCM-A11 Package Pin Name: SD1_DAT3
     * SCM-A11 Reference P3A Wingboard Signal: SD1_DATA(3) (MMC)
     * Ascension P3 Signal: SD1_DATA(3) (MMC)
     * Selected Primary Function: SD1_DAT3 (Input/Output)
     * Mux0 Function: GP_SP_A19
     *
     * Array index: 42  GPIO_SIGNAL_SD1_DAT3
     */
    { GPIO_SP_A_PORT,       19, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: SD2_DAT3
     * SCM-A11 Reference P3A Wingboard Signal: SD2_DATA(3) (WLAN)
     * Ascension P3 Signal: SD2_DATA(3) (WLAN)
     * Selected Primary Function: SD2_DAT3 (Input/Output)
     * Mux0 Function: GP_SP_A25
     *
     * Array index: 43  GPIO_SIGNAL_SD2_DAT3
     */
    { GPIO_SP_A_PORT,       25, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },
};


/**
 * Default IOMUX pad register settings.
 */
struct iomux_pad_setting iomux_pad_register_settings[IOMUX_PAD_SETTING_COUNT] __initdata = {
    /* SDRAM -- DSE_HIGH | PKE_ENABLE */
    { IOPAD_GROUP1, 0x0082 },

    /* SDRAM -- DSE_HIGH */
    { IOPAD_GROUP2, 0x0002 },

    /* SDRAM -- 0 */
    { IOPAD_GROUP3, 0x0000 },

    /* SDRAM -- DSE_HIGH | PKE_ENABLE */
    { IOPAD_GROUP4, 0x0082 },

    /* SDRAM -- DSE_HIGH */
    { IOPAD_GROUP5, 0x0002 },

    /* SDRAM -- SRE_FAST | PUS_100K_PULLUP | PKE_ENABLE */
    { IOPAD_GROUP6, 0x00C1 },

    /* SDRAM -- SRE_FAST | DSE_MIN | PKE_ENABLE */
    { IOPAD_GROUP7, 0x0087 },

    /* SDRAM -- SRE_FAST */
    { IOPAD_GROUP8, 0x0001 },

    /* SDRAM -- SRE_FAST | PKE_ENABLE */
    { IOPAD_GROUP9, 0x0081 },

    /* Audio & AP GPIOs -- PUS_22K_PULLUP */
    { IOPAD_GROUP10, 0x0060 },

    /* Keypad -- PUS_47K_PULLUP */
    { IOPAD_GROUP11, 0x0020 },

    /* Keypad -- DDR_MODE_DDR | PUS_47K_PULLUP */
    { IOPAD_GROUP12, 0x0220 },

    /* I2C -- DDR_MODE_DDR | PUS_100K_PULLUP */
    { IOPAD_GROUP13, 0x0240 },

    /* CSPI1 -- 0 */
    { IOPAD_GROUP14, 0x0000 },

    /* CKO & CKOH -- 0 */
    { IOPAD_GROUP15, 0x0000 },

    /* MQSPI -- 0 */
    { IOPAD_GROUP16, 0x0000 },

    /* EL1T -- 0 */
    { IOPAD_GROUP17, 0x0000 },

    /* ETM/NEXUS -- 0 */
    { IOPAD_GROUP18, 0x0000 },

    /* JTAG -- 0 */
    { IOPAD_GROUP19, 0x0000 },

    /* DIGRF -- 0 */
    { IOPAD_GROUP20, 0x0000 },

    /* ETM/NEXUS -- 0 */
    { IOPAD_GROUP21, 0x0000 },

    /* CSI (Camera) -- SRE_FAST | DSE_MIN */
    { IOPAD_GROUP22, 0x0007 },

    /* EDIO -- PUS_100K_PULLUP */
    { IOPAD_GROUP23, 0x0040 },

    /* EDIO -- PUS_100K_PULLUP */
    { IOPAD_GROUP24, 0x0040 },

    /* DIG --  HYS_SCHMITZ | PKE_ENABLE | PUS_22K_PULLUP | SRE_FAST*/
    { IOPAD_GROUP25, 0x01E1 },

    /* SDHC -- PUS_100K_PULLUP */
    { IOPAD_GROUP26, 0x0040 },

    /* SDHC -- PUS_100K_PULLUP */
    { IOPAD_GROUP27, 0x0040 },

    /* Not Used -- 0 */
    { IOPAD_GROUP28, 0x0000 },
};


/**
 * Update the initial_gpio_settings array based on the board revision. This
 * function is called by scma11phone_gpio_init() at boot.
 */
void __init scma11ref_gpio_signal_fixup(void)
{
#ifdef CONFIG_MOT_FEAT_BRDREV
    /* assuming BOARDREV_UNKNOWN means latest hardware revision */
    if( boardrev() < BOARDREV_P2BW) {
        gpio_signal_fixup_p2aw();
    } else if( boardrev() < BOARDREV_P3AW) {
        gpio_signal_fixup_p2bw();
    } else if( boardrev() < BOARDREV_P3CW) {
        gpio_signal_fixup_p3aw();
    }
#endif /* CONFIG_MOT_FEAT_BRDREV */

    return;
}

/**
 * Update the IOMUX settings where SCM-A11 Ref. varies from the other products.
 */
void __init scma11ref_iomux_mux_fixup(void)
{
    /*
     * SCM-A11 Package Pin Name: IPU_D3_SPL
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_MAIN_BKL (Backlight)
     * Selected Primary Function: GP_AP_A30 (Output)
     *
     * Primary function out of reset: IPU_D3_SPL
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A30
     */
    iomux_config_mux(AP_IPU_D3_PS, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);

    /*
     * SCM-A11 Package Pin Name: GP_AP_C14
     * SCM-A11 Reference P1A Wingboard Signal: GPIO_CLI_BKL (Backlight)
     * SCM-A11 Reference P1D Wingboard Signal: GPIO_CLI_BKL (Backlight)
     * Selected Primary Function: GP_AP_C14 (Output)
     *
     * Primary function out of reset: GP_AP_C14
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C14
     */
    iomux_config_mux(AP_GPIO_AP_C14, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
}


#ifdef CONFIG_MOT_FEAT_BRDREV
/**
 * Adjust GPIO settings to reflect P3A Wingboard configuration.
 */
void __init gpio_signal_fixup_p3aw(void)
{
    /*
     * SCM-A11 Package Pin Name: UH2_TXOE_B
     * SCM-A11 Reference P1A Wingboard Signal: GPS_RESET (GPS)
     * SCM-A11 Reference P1D Wingboard Signal: GPS_RESET (GPS)
     * SCM-A11 Reference P2B Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3A Wingboard Signal: OMEGA_INTERRUPT (Omega Wheel)
     * Selected Primary Function: GP_SP_A8 (Output)
     *
     * Array index: 33  GPIO_SIGNAL_TNLC_KCHG_INT
     */
    initial_gpio_settings[GPIO_SIGNAL_TNLC_KCHG_INT].port   = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_KCHG_INT].sig_no = 8;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_KCHG_INT].out    = GPIO_GDIR_INPUT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_KCHG_INT].data   = GPIO_DATA_INVALID;

    /*
     * SCM-A11 Package Pin Name: UH2_RXDP
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: UH2_RXDP (Saipan Connector)
     * SCM-A11 Reference P3A Wingboard Signal: UH2_RXDP (INTERRUPT) (Morphing)
     * Selected Primary Function: GP_SP_A12 (Output)
     *
     * Array index: 36  GPIO_SIGNAL_TNLC_RCHG
     */
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RCHG].port   = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RCHG].sig_no = 12;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RCHG].out    = GPIO_GDIR_INPUT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RCHG].data   = GPIO_DATA_INVALID;

    /*
     * SCM-A11 Package Pin Name: UH2_SUSPEND
     * SCM-A11 Reference P1A Wingboard Signal: CAM_TORCH_EN (Camera Flash)
     * SCM-A11 Reference P1D Wingboard Signal: CAM_TORCH_EN (Camera Flash)
     * SCM-A11 Reference P2B Wingboard Signal: LIN_VIB_AMP_EN (Linear Vibrator)
     * Selected Primary Function: GP_SP_A10 (Output)
     *
     * Array index: 28  GPIO_SIGNAL_LIN_VIB_AMP_EN
     *
     * Set high to enable vibrator.
     */
    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].port   = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].sig_no = 10;
    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].data   = GPIO_DATA_LOW;

    /*
     * SCM-A11 Package Pin Name: SPI1_SS1
     * SCM-A11 Reference P1A Wingboard Signal: USER_OFF (Atlas)
     * SCM-A11 Reference P1D Wingboard Signal: USER_OFF (Atlas)
     * SCM-A11 Reference P2B Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3A Wingboard Signal: SPI1_SS1 (2nd MORPH RESET)
     *                                                  (Morphing)
     * Selected Primary Function: GP_SP_A31 (Output)
     *
     * Array index: 34  GPIO_SIGNAL_TNLC_RESET
     */
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RESET].port      = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RESET].sig_no    = 31;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RESET].out       = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RESET].data      = GPIO_DATA_LOW;

    /* mark signals first used on P3C wingboard as invalid */
    initial_gpio_settings[GPIO_SIGNAL_UI_IC_DBG].port   = GPIO_INVALID_PORT;
    initial_gpio_settings[GPIO_SIGNAL_FSS_HYST].port    = GPIO_INVALID_PORT;
    initial_gpio_settings[GPIO_SIGNAL_SER_RST_B].port   = GPIO_INVALID_PORT;
}


/**
 * Adjust GPIO settings to reflect P2B Wingboard configuration.
 */
void __init gpio_signal_fixup_p2bw(void)
{
    /* revert table to P3A wingboard settings first */
    gpio_signal_fixup_p3aw();

    /*
     * SCM-A11 Package Pin Name: UH2_SPEED
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P2A Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: VFUSE_SELECT (Misc)
     * SCM-A11 Reference P3A Wingboard Signal: FM_RESET (FM Radio)
     * Selected Primary Function: GP_SP_A9 (Output)
     */
    initial_gpio_settings[GPIO_SIGNAL_FM_RESET].port    = GPIO_INVALID_PORT;

    /*
     * SCM-A11 Package Pin Name: GP_AP_C11
     * SCM-A11 Reference P1A Wingboard Signal: BT_CLK_EN_B (Bluetooth)
     * SCM-A11 Reference P1D Wingboard Signal: BT_CLK_EN_B (Bluetooth)
     * SCM-A11 Reference P2B Wingboard Signal: NC (NC)
     * SCM-A11 Reference P3A Wingboard Signal: FM_INTERRUPT (FM Radio)
     * Selected Primary Function: GP_AP_C11 (Input)
     */
    initial_gpio_settings[GPIO_SIGNAL_FM_INTERRUPT].port    = GPIO_INVALID_PORT;

    /* p2b wingboard doesn't support morphing */
    initial_gpio_settings[GPIO_SIGNAL_TNLC_KCHG_INT].port   = GPIO_INVALID_PORT;
    initial_gpio_settings[GPIO_SIGNAL_CAP_RESET].port       = GPIO_INVALID_PORT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RESET].port      = GPIO_INVALID_PORT;
    initial_gpio_settings[GPIO_SIGNAL_TNLC_RCHG].port       = GPIO_INVALID_PORT;
    
    /*
     * SCM-A11 Package Pin Name: SPI1_CLK
     * SCM-A11 Reference P1A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P1D Wingboard Signal: NC (NC)
     * SCM-A11 Reference P2B Wingboard Signal: EL_NUM_EN (Backlight)
     * SCM-A11 Reference P3A Wingboard Signal: NC (NC)
     * Selected Primary Function: GP_SP_A27 (Output)
     */
    initial_gpio_settings[GPIO_SIGNAL_EL_NUM_EN].port   = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_EL_NUM_EN].sig_no = 27;
    initial_gpio_settings[GPIO_SIGNAL_EL_NUM_EN].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_EL_NUM_EN].data   = GPIO_DATA_LOW;

    /*
     * SCM-A11 Package Pin Name: SPI1_MISO
     * SCM-A11 Reference P1A Wingboard Signal: NC (NC)
     * SCM-A11 Reference P1D Wingboard Signal: NC (NC)
     * SCM-A11 Reference P2B Wingboard Signal: SPI1_MISO (Backlight)
     * SCM-A11 Reference P3A Wingboard Signal: EL_EN (Backlight)
     * Selected Primary Function: GP_SP_A29 (Output)
     */
    initial_gpio_settings[GPIO_SIGNAL_EL_NAV_EN].port   = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_EL_NAV_EN].sig_no = 29;
    initial_gpio_settings[GPIO_SIGNAL_EL_NAV_EN].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_EL_NAV_EN].data   = GPIO_DATA_LOW;

    /* p2b wingboard doesn't have EL_EN signal */
    initial_gpio_settings[GPIO_SIGNAL_EL_EN].port       = GPIO_INVALID_PORT;
}


/**
 * Adjust GPIO settings to reflect P2A Wingboard configuration.
 */
void __init gpio_signal_fixup_p2aw(void)
{
    /* revert to p2bw state before reverting to p2aw */
    gpio_signal_fixup_p2bw();

    /*
     * SCM-A11 Package Pin Name: UH2_SPEED
     * SCM-A11 Reference P1A Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P1D Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P2A Wingboard Signal: WLAN_RESET (WLAN)
     * SCM-A11 Reference P2B Wingboard Signal: VFUSE_SELECT (Misc)
     * Selected Primary Function: GP_SP_A9 (Output)
     */
    initial_gpio_settings[GPIO_SIGNAL_WLAN_RESET].port   = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_WLAN_RESET].sig_no = 9;
    initial_gpio_settings[GPIO_SIGNAL_WLAN_RESET].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_WLAN_RESET].data   = GPIO_DATA_LOW;
}
#endif /* CONFIG_MOT_FEAT_BRDREV */
