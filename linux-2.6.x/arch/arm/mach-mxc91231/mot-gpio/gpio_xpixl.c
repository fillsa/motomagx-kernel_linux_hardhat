/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/gpio_xpixl.c
 *
 * Copyright 2007-2008 Motorola, Inc.
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
 * 30-May-2007  Motorola        Initial version (Copy from Marco)
 * 30-May-2007  Motorola        Added support for xPixl PWM backlight on pin AP_GPIO_AP_B17
 * 24-Sep-2007  Motorola        Added support for xPixl lens cover on pin AP_GPIO_AP_C22
 * 06-Nov-2007  Motorola        Camera gpio set low on boot
 * 12-Dec-2007  Motorola        Changed CSPI IOMUX pad settings.
 * 28-Apr-2008  Motorola        Changed display pins handling
 */

#include <linux/config.h>
#include <asm/mot-gpio.h>

#include "mot-gpio-scma11.h"



/**
 * Initial GPIO register settings.
 */
struct gpio_signal_settings initial_gpio_settings[MAX_GPIO_SIGNAL] = {
    /*
     * SCM-A11 Package Pin Name: GP_AP_C8
     * Pico P0 Signal: BT_RESET_B (Bluetooth)
     * Selected Primary Function: GP_AP_C8 (Output)
     *
     * Array index: 0   GPIO_SIGNAL_BT_POWER
     *
     * Power off Bluetooth at boot time. (Signal is connected to Bluetooth's
     * VREG_CTL.
     *
     */
    { GPIO_AP_C_PORT,    8, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: IPU_D3_CONTR
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_CLI_SER_RS
     * xPixl Signal: USB_HS_RESET
     * Selected Primary Function: GP_AP_A25
     * Mux0 Function: GP_AP_A25
     *
     * Array index: 1	GPIO_SIGNAL_USB_HS_RESET
     */
    { GPIO_AP_A_PORT,		25,  GPIO_GDIR_OUTPUT,	GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: GP_AP_B22
     * Pico P0 Signal: NC (USB HS)
     * Selected Primary Function: GP_AP_B22 (Input)
     *
     * Array index: 2   GPIO_SIGNAL_USB_HS_DMA_REQ
     */
    { GPIO_AP_B_PORT,   22, GPIO_GDIR_INPUT,  GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C9
     * Pico P0 Signal: NC (USB HS)
     * Selected Primary Function: GP_AP_C9 (Output)
     *
     * Array index: 3   GPIO_SIGNAL_USB_HS_WAKEUP
     */
    { GPIO_AP_C_PORT,    9, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C10
     * Pico P0 Signal: NC (USB HS)
     * Selected Primary Function: GP_AP_C10 (Input)
     *
     * Array index: 4   GPIO_SIGNAL_USB_HS_FLAGC
     */
    { GPIO_AP_C_PORT,   10, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: ED_INT6
     * Pico P0 Signal: NC (USB HS)
     * Selected Primary Function: GP_AP_C24 (Input)
     * Selected Secondary Function: ED_INT6 (Input)
     *
     * Array index: 5   GPIO_SIGNAL_USB_HS_INT
     */
    { GPIO_AP_C_PORT,   24, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: UH2_TXDP
     * Pico P0 Signal: NC (USB HS)
     * Selected Primary Function: GP_SP_A11 (Output)
     *
     * Array index: 6   GPIO_SIGNAL_USB_HS_SWITCH
     */
    { GPIO_SP_A_PORT,   11, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: IPU_PAR_RS
     * Pico P0 Signal: SER_EN (Display)
     * xPixl Signal: GPIO_SIGNAL_DISP_RST_B
     * Selected Primary Function: GP_AP_A29 (Output)
     *
     * Array index: 7   GPIO_SIGNAL_DISP_RST_B
     *
     * MBM will disable/enable the serializer at boot as appropriate.
     */
    { GPIO_AP_A_PORT,	29, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },
    

    /*
     * SCM-A11 Package Pin Name: IPU_D0_CS
     * Pico P0 Signal: GPIO_DISP_SD (Display)
     * Selected Primary Function: GP_AP_A26 (Output)
     *
     * Array index: 8   GPIO_SIGNAL_DISP_SD
     *
     * MBM will disable/enable the display at boot as appropriate.
     */
    { GPIO_AP_A_PORT,   26, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: CSI_D0
     * Pico P0 Signal: GPIO_CAM_RST_B (camera)
     * Selected Primary Function: GP_AP_B24 (Output)
     *
     * Array index: 9   GPIO_SIGNAL_CAM_RST_B
     *
     * Take camera out of reset at boot.
     */
    { GPIO_AP_B_PORT,   24, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: CSI_D1
     * Pico P0 Signal: GPIO_CAM_PD (camera)
     * Selected Primary Function: GP_AP_B25 (Output)
     *
     * Array index: 10  GPIO_SIGNAL_CAM_PD
     *
     * Power down camera at boot.
     */
    { GPIO_AP_B_PORT,   25, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_RXDM
     * Pico P0 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A13 (Output)
     *
     * Array index: 11  GPIO_SIGNAL_SP_A13
     */
    { GPIO_SP_A_PORT,   13, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: SPI1_SS0
     * Pico P0 Signal: CAM_FLASH_EN (Camera Flash)
     * Selected Primary Function: GP_SP_A30 (Output)
     *
     * Array index: 12  GPIO_SIGNAL_XENON_CHARGE_EN
     * PIXL: DM check
     */
    { GPIO_SP_A_PORT,   30, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
    
    /*
     * SCM-A11 Package Pin Name: ED_INT3
     * Pico P0 Signal: FLIP_OPEN (Misc)
     * Selected Primary Function: GP_AP_C21 (Input)
     * Selected Secondary Function: ED_INT3 (Input)
     *
     * Array index: 13  GPIO_SIGNAL_KEYPAD_LOCK
     */
    { GPIO_AP_C_PORT,   21, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },
    
    /*
     * SCM-A11 Package Pin Name: GP_AP_B18
     * Pico P0 Signal: GPIO_DISP_RST_B (Display)
     * Selected Primary Function: GP_AP_B18 (Output)
     *
     * Array index: 14  GPIO_SIGNAL_XENON_QUENCH
     *
     * MBM will set this at boot.
     */
    { GPIO_AP_B_PORT,   18, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C12
     * Pico P0 Signal: NC (NC)
     * Selected Primary Function: GP_AP_C12 (Output)
     *
     * Array index: 15  GPIO_SIGNAL_WLAN_RESET
     */
    { GPIO_AP_C_PORT,   12, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C13
     * Pico P0 Signal: BT_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C13 (Output)
     *
     * Array index: 16  GPIO_SIGNAL_BT_WAKE_B
     */
    { GPIO_AP_C_PORT,   13, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: U3_RTS_B
     * Pico P0 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A2 (input)
     *
     * Array index: 17  GPIO_SIGNAL_DM299_REQ_INT
     */
    { GPIO_SP_A_PORT,    2, GPIO_GDIR_INPUT,   GPIO_DATA_INVALID},

    /*
     * SCM-A11 Package Pin Name: U3_CTS_B
     * Pico P0 Signal: VVIB_EN (Atlas)
     * Selected Primary Function: GP_SP_A3 (Output)
     *
     * Array index: 18  GPIO_SIGNAL_WLAN_CLIENT_WAKE_B
     */
    { GPIO_SP_A_PORT,    3, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: UH2_TXOE_B
     * Pico P0 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A8 (Output)
     *
     * Array index: 19  GPIO_SIGNAL_SP_A8
     */
    { GPIO_SP_A_PORT,    8, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_SPEED
     * Pico P0 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A9 (Output)
     *
     * Array index: 20  GPIO_SIGNAL_SP_A9
     */
    { GPIO_SP_A_PORT,    9, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_RXDP
     * Pico P0 Signal: TOUCH_INTB (Morphing)
     * Selected Primary Function: GP_SP_A12 (Output)
     *
     * Array index: 21  GPIO_SIGNAL_SP_A12
     *
     * Capacitive Touch Keys -- Interrupt
     */
    { GPIO_SP_A_PORT,   12, GPIO_GDIR_INPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: UH2_OVR
     * Pico P0 Signal: USB_XCVR_EN (Atlas)
     * Selected Primary Function: GP_SP_A14 (Output)
     *
     * Array index: 22  GPIO_SIGNAL_USB_XCVR_EN
     */
    { GPIO_SP_A_PORT,   14, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_PWR
     * Pico P0 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A15 (Output)
     *
     * Array index: 23  GPIO_SIGNAL_SP_A15
     */
    { GPIO_SP_A_PORT,   15, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_SUSPEND
     * Pico P0 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A10 (Output)
     *
     * Array index: 24  GPIO_SIGNAL_SP_A10
     */
    { GPIO_SP_A_PORT,   10, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: ED_INT1
     * Pico P0 Signal: BT_HOST_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C19 (Input)
     * Selected Secondary Function: ED_INT1 (Input)
     *
     * Array index: 25  GPIO_SIGNAL_BT_HOST_WAKE_B
     *
     * Host wake interrupt from bluetooth controller.
     */
    { GPIO_AP_C_PORT,   19, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: SD1_DAT3
     * SCM-A11 Reference P3A Wingboard Signal: SD1_DATA(3) (MMC)
     * Ascension P3 Signal: SD1_DATA(3) (MMC)
     * Selected Primary Function: SD1_DAT3 (Input/Output)
     * Mux0 Function: GP_SP_A19
     *
     * Array index: 26  GPIO_SIGNAL_SD1_DAT3
     */
    { GPIO_SP_A_PORT,       19, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: SD2_DAT3
     * SCM-A11 Reference P3A Wingboard Signal: SD2_DATA(3) (WLAN)
     * Ascension P3 Signal: SD2_DATA(3) (WLAN)
     * Selected Primary Function: SD2_DAT3 (Input/Output)
     * Mux0 Function: GP_SP_A25
     *
     * Array index: 27  GPIO_SIGNAL_SD2_DAT3
     */
    { GPIO_SP_A_PORT,       25, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: ED_INT0
     * Marco P1 Signal: WLAN_HOST_WAKE_B (WLAN)
     * Selected Primary Function: GP_AP_C18
     (Input) * Selected Secondary Function: ED_INT0 (Input)
     *
     * Array index: 28	GPIO_SIGNAL_WLAN_HOST_WAKE_B
     */
    { GPIO_AP_C_PORT,	18, GPIO_GDIR_INPUT,	GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: ED_INT4
     * Selected Primary Function: GP_AP_C22 (Input)
     * Selected Secondary Function: ED_INT4 (Input)
     *
     * Array index: 29	GPIO_SIGNAL_LENS_COVER_INT
     */
    { GPIO_AP_A_PORT,	28, GPIO_GDIR_INPUT,	GPIO_DATA_INVALID },
		
    /*
     * SCM-A11 Package Pin Name: ED_INT7
     * Selected Primary Function: GP_AP_C25 (Input)
     * Selected Secondary Function: ED_INT7 (Input)
     *
     * Array index: 30	GPIO_SIGNAL_FM_INT
     */
    { GPIO_AP_C_PORT,	25, GPIO_GDIR_INPUT,	GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_SP_A26
     * Marco P1 Signal: WLAN_PWR_DWN_B (WLAN)
     * Selected Primary Function: GP_SP_A26 (Input)
     *
     * Array index: 31	GPIO_SIGNAL_WLAN_PWR_DWN_B
     */
    { GPIO_SP_A_PORT,	26, GPIO_GDIR_OUTPUT,	 GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_SP_A1
     * Selected Primary Function: GP_SP_A1 (Output)
     *
     * Array index: 32	GPIO_SIGNAL_TV_OUT_RST
     */
    { GPIO_SP_A_PORT,	0, GPIO_GDIR_OUTPUT,	 GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: U3_RXD
     * Selected Primary Function: GP_SP_A1 (Input)
     *
     * Array index: 33	GPIO_SIGNAL_XENON_HWID
     */
    { GPIO_SP_A_PORT,	1, GPIO_GDIR_INPUT,	 GPIO_DATA_INVALID},

    /*
     * SCM-A11 Package Pin Name: GP_AP_C11
     * Selected Primary Function: GP_AP_C11 (Output)
     *
     * Array index: 34	GPIO_SIGNAL_I_PEEK
     */
    { GPIO_AP_C_PORT,	11, GPIO_GDIR_OUTPUT,	 GPIO_DATA_LOW},

    /*
     * SCM-A11 Package Pin Name: IPU_D3_DRDY
     * Selected Primary Function: GP_AP_A24 (Output)
     *
     * Array index: 35	GPIO_SIGNAL_DM299_PWR_EN	 */
    { GPIO_AP_A_PORT,	24, GPIO_GDIR_OUTPUT,	 GPIO_DATA_LOW},

    /*
     * SCM-A11 Package Pin Name: GP_AP_B17
     * Selected Primary Function: GP_AP_B17 (Output)
     *
     * Array index: 36	GPIO_SIGNAL_LCD_BACKLIGHT
     */
    { GPIO_AP_B_PORT, 17, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID},    

    /*
     * SCM-A11 Package Pin Name: SPI1_SS1
     * Selected Primary Function: GP_SP_A31 (Output)
     *
     * Array index: 37 GPIO_SIGNAL_DMSPI_CS
     */
    { GPIO_SP_A_PORT, 31, GPIO_GDIR_OUTPUT, GPIO_DATA_LOW},

    /*
        * SCM-A11 Package Pin Name: ED_INT4
	* Used for Lens Cover
        * Selected Primary Function: GP_AP_C22 (Input)
        * Selected Secondary Function: ED_INT4 (Input)
        * Array index: 38 GPIO_SIGNAL_LENS_COVER
        */
    { GPIO_AP_C_PORT, 22, GPIO_GDIR_INPUT, GPIO_DATA_INVALID}
};

/**
 * Update the initial_gpio_settings array based on the board revision. This
 * function is called by scma11phone_gpio_init() at boot.
 */
void __init pixl_gpio_signal_fixup(void)
{
/* For future purpose*/
	return;
}

/**
 * Default IOMUX pad register settings.
 */
struct iomux_pad_setting iomux_pad_register_settings[IOMUX_PAD_SETTING_COUNT] __initdata = {
    /* SDRAM -- DSE_HIGH | PKE_ENABLE */
    { IOPAD_GROUP1, 0x0082 },

    /* SDRAM -- DSE_HIGH */
    { IOPAD_GROUP2, 0x0002 },

    /* SDRAM -- DSE_HIGH */
    { IOPAD_GROUP3, 0x0002 },

    /* SDRAM -- DSE_HIGH | PKE_ENABLE */
    { IOPAD_GROUP4, 0x0082 },

    /* SDRAM -- SRE_FAST | DSE_HIGH */
    { IOPAD_GROUP5, 0x0003 },

    /* SDRAM -- SRE_FAST | DSE_HIGH | PUS_100K_PULLUP | PKE_ENABLE */
    { IOPAD_GROUP6, 0x00C3 },

    /* SDRAM -- SRE_FAST | DSE_MIN | PKE_ENABLE */
    { IOPAD_GROUP7, 0x0087 },

    /* SDRAM -- SRE_FAST */
    { IOPAD_GROUP8, 0x0001 },

    /* SDRAM -- SRE_FAST | DSE_MIN | PKE_ENABLE */
    { IOPAD_GROUP9, 0x0087 },

    /* Audio & AP GPIOs -- PUS_22K_PULLUP */
    { IOPAD_GROUP10, 0x0060 },

    /* Keypad -- PUS_47K_PULLUP */
    { IOPAD_GROUP11, 0x0020 },

    /* Keypad -- DSE_MAX | DDR_MODE_DDR | PUS_100K_PULLUP */
    { IOPAD_GROUP12, 0x0244 },

    /* I2C -- DDR_MODE_DDR | PUS_100K_PULLUP */
    { IOPAD_GROUP13, 0x0240 },

    /* CSPI1 -- 0 */
    { IOPAD_GROUP14, DSE_MAX},

    /* CKO & CKOH -- DSE_HIGH */
    { IOPAD_GROUP15, 0x0002 },

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

    /* CSI (Camera) -- 0 */
    { IOPAD_GROUP22, 0x0000 },

    /* EDIO -- PUS_100K_PULLUP */
    { IOPAD_GROUP23, 0x0040 },

    /* EDIO -- PUS_100K_PULLUP */
    { IOPAD_GROUP24, 0x0040 },

    /* DIG --  HYS_SCHMITZ | PKE_ENABLE | PUS_22K_PULLUP | SRE_FAST*/
    { IOPAD_GROUP25, 0x01E1 },

    /* SDHC -- PUS_100K_PULLUP | PKE_ENABLE */
    { IOPAD_GROUP26, 0x00C0 },

    /* SDHC -- PUS_100K_PULLUP */
    { IOPAD_GROUP27, 0x0040 },

    /* Not Used -- 0 */
    { IOPAD_GROUP28, 0x0000 },
};

/**
 * Update the IOMUX settings where SCM-A11 Ref. varies from the other products.
 */
void __init pixl_iomux_mux_fixup(void)
{
    /*
     * SCM-A11 Package Pin Name: SPI1_CLK
     * SCM-A11 Reference P3A Wingboard Signal: NC (NC)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A27 (Output)
     *
     * Primary function out of reset: SPI1_CLK
     * Out of Reset State: High-Z
     * Mux0 Function: GP_SP_A27
     */
    iomux_config_mux(SP_SPI1_CLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: SPI1_MOSI
     * SCM-A11 Reference P3A Wingboard Signal: USB_HS_RESET (Wing Connector)
     * Ascension P3 Signal: USB_HS_RESET (USB HS)
     * Selected Primary Function: GP_SP_A28 (Output)
     *
     * Primary function out of reset: SPI1_MOSI
     * Out of Reset State: Output
     * Mux0 Function: GP_SP_A28
     */
    iomux_config_mux(SP_SPI1_MOSI, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: SPI1_MISO
     * SCM-A11 Reference P3A Wingboard Signal: EL_EN (Backlight)
     * Ascension P3 Signal: EL_NAV_EN (Backlight)
     * Selected Primary Function: GP_SP_A29 (Output)
     *
     * Primary function out of reset: SPI1_MISO
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A29
     */
    iomux_config_mux( SP_SPI1_MISO, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1);
    /*
     * SCM-A11 Package Pin Name: SPI1_SS1
     * SCM-A11 Reference P3A Wingboard Signal: NC (NC)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: Unassigned (Input)
     *
     * Primary function out of reset: SPI1_SS1
     * Out of Reset State: Output
     * Mux0 Function: GP_SP_A31
     */
    iomux_config_mux(SP_SPI1_SS1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: GP_AP_C10
     * SCM-A11 Reference P3A Wingboard Signal: USB_HS_DMA_ACK (Wing Connector)
     * Ascension P3 Signal: USB_HS_FLAGC (USB HS)
     * Selected Primary Function: GP_AP_C10 (Input)
     *
     * Primary function out of reset: GP_AP_C10
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C10
     */
    iomux_config_mux(AP_GPIO_AP_C10, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1);
    /*
     * SCM-A11 Package Pin Name: IPU_D3_DRDY
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D3_DRDY (Display)
     * Ascension P3 Signal: IPU_D3_DRDY (Display)
     * Selected Primary Function: IPU_D3_DRDY (Output)
     *
     * Primary function out of reset: IPU_D3_DRDY
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A24
     */
    iomux_config_mux(AP_IPU_D3_DRDY, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: IPU_D3_CONTR
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_CLI_SER_RS (CLI)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: IPU_SER_RS (Output)
     *
     * Primary function out of reset: IPU_D3_CONTR
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A25
     */
    iomux_config_mux(AP_IPU_D3_CONTR, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: IPU_D2_CS
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D2_CS_B (CLI)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: IPU_D2_CS (Output)
     *
     * Primary function out of reset: IPU_D2_CS
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A28
     */
    iomux_config_mux(AP_IPU_D2_CS, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: GP_AP_B18
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_DISP_RST_B (CLI)
     * Ascension P3 Signal: GPIO_DISP_RST_B (Display)
     * Selected Primary Function: GP_AP_B18 (Output)
     *
     * Primary function out of reset: GP_AP_B18
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B18
     */
    iomux_config_mux(AP_GPIO_AP_B18, OUTPUTCONFIG_FUNC2, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: GP_AP_B22
     * SCM-A11 Reference P3A Wingboard Signal: USB_HS_DMA_REQ (Wing Connector)
     * Ascension P3 Signal: USB_HS_DMA_REQ (USB HS)
     * Selected Primary Function: GP_AP_B22 (Input)
     *
     * Primary function out of reset: GP_AP_B22
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B22
     * PIXL: USB team - check
     */
    iomux_config_mux(AP_GPIO_AP_B22, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC2);
    /*
     * SCM-A11 Package Pin Name: U3_TXD
     * SCM-A11 Reference P3A Wingboard Signal: GPS_U3_TX (GPS)
     * Ascension P3 Signal: GPS_U3_TX (Linux Terminal)
     * Selected Primary Function: U3_TXD (Output)
     *
     * Primary function out of reset: U3_TXD
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A0
     */
    iomux_config_mux(SP_U3_TXD, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE );
    /*
     * SCM-A11 Package Pin Name: GP_AP_B17
     * Selected Primary Function: GP_AP_B17 (Output)
     *
     * Primary function out of reset: GP_AP_B17
     * Out of Reset State: Low
     * Mux2 Function: AP_PWM
     */
    iomux_config_mux(AP_GPIO_AP_B17, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE );
}
