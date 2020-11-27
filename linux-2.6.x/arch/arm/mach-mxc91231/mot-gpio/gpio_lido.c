/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/gpio_lido.c
 *
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * 02-Nov-2006  Motorola        Changed IOMUX pad group 22 setting.
 * 13-Nov-2006  Motorola        Change pad group 25 settings.
 * 15-Dec-2006  Motorola        IOMUX pad 12 current drain improvement
 * 02-Jan-2007  Motorola        Added support for Lido P2.
 * 26-Jan-2007  Motorola        Bluetooth current drain improvements.
 * 20-Feb-2007  Motorola        Add SDHC API.
 * 20-Mar-2007  Motorola        Adjust GPIO settings for IPU_D3_CLK.
 * 02-Apr-2007  Motorola        Pad group 10 setting changed to 6.
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
static void gpio_setting_fixup_p1(void);
#endif /* CONFIG_MOT_FEAT_BRDREV */

/**
 * Initial GPIO register settings.
 */
struct gpio_signal_settings initial_gpio_settings[MAX_GPIO_SIGNAL] = {
    /*
     * SCM-A11 Package Pin Name: GP_AP_C8
     * Lido P1 Signal: BT_RESET_B (Bluetooth)
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
     * SCM-A11 Package Pin Name: SPI1_MOSI
     * Lido P1 Signal: USB_HS_RESET (USB HS)
     * Selected Primary Function: GP_SP_A28 (Output)
     *
     * Array index: 1   GPIO_SIGNAL_USB_HS_RESET
     * 
     * Disable high speed USB controller at boot.
     */
    { GPIO_SP_A_PORT,   28, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_B22
     * Lido P1 Signal: GP_AP_B22 (Wing Connector)
     * Selected Primary Function: GP_AP_B22 (Input)
     *
     * Array index: 2   GPIO_SIGNAL_USB_HS_DMA_REQ
     */
    { GPIO_AP_B_PORT,   22, GPIO_GDIR_INPUT,  GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C9
     * Lido P1 Signal: USB_HS_WAKEUP/10K PD (USB HS)
     * Selected Primary Function: GP_AP_C9 (Output)
     *
     * Array index: 3   GPIO_SIGNAL_USB_HS_WAKEUP
     *
     * Put highspeed USB into sleep mode.
     */
    { GPIO_AP_C_PORT,    9, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C10
     * Lido P1 Signal: USB_HS_FLAGC (USB HS)
     * Selected Primary Function: GP_AP_C10 (Input)
     *
     * Array index: 4   GPIO_SIGNAL_USB_HS_FLAGC
     */
    { GPIO_AP_C_PORT,   10, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: ED_INT6
     * Lido P1 Signal: USB_HS_INT (USB HS)
     * Selected Primary Function: GP_AP_C24 (Input)
     * Selected Secondary Function: ED_INT6 (Input)
     *
     * Array index: 5   GPIO_SIGNAL_USB_HS_INT
     *
     * High speed USB interrupt.
     */
    { GPIO_AP_C_PORT,   24, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: UH2_TXDP
     * Lido P1 Signal: USB_HS_SWITCH (USB HS)
     * Selected Primary Function: GP_SP_A11 (Output)
     *
     * Array index: 6   GPIO_SIGNAL_USB_HS_SWITCH
     *
     * Low setting means USB HS is connected to Atlas.
     */
    { GPIO_SP_A_PORT,   11, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: IPU_PAR_RS
     * Lido P1 Signal: SER_EN (Display)
     * Selected Primary Function: GP_AP_A29 (Output)
     *
     * Array index: 7   GPIO_SIGNAL_SER_EN
     *
     * MBM will disable/enable the serializer at boot as appropriate.
     */
    { GPIO_AP_A_PORT,   29, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: IPU_D0_CS
     * Lido P1 Signal: GPIO_DISP_SD (Display)
     * Selected Primary Function: GP_AP_A26 (Output)
     *
     * Array index: 8   GPIO_SIGNAL_DISP_SD
     *
     * MBM will disable/enable the display at boot as appropriate.
     */
    { GPIO_AP_A_PORT,   26, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: CSI_D0
     * Lido P1 Signal: GPIO_CAM_RST_B (camera)
     * Selected Primary Function: GP_AP_B24 (Output)
     *
     * Array index: 9   GPIO_SIGNAL_CAM_RST_B
     *
     * Take camera out of reset at boot.
     */
    { GPIO_AP_B_PORT,   24, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: CSI_D1
     * Lido P1 Signal: GPIO_CAM_PD (camera)
     * Selected Primary Function: GP_AP_B25 (Output)
     *
     * Array index: 10  GPIO_SIGNAL_CAM_PD
     *
     * Power down camera at boot.
     */
    { GPIO_AP_B_PORT,   25, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: UH2_RXDM
     * Lido P1 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A13 (Output)
     *
     * Array index: 11  GPIO_SIGNAL_SP_A13
     */
    { GPIO_SP_A_PORT,   13, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: SPI1_SS0
     * Lido P1 Signal: CAM_FLASH_EN (Camera Flash)
     * Selected Primary Function: GP_SP_A30 (Output)
     *
     * Array index: 12  GPIO_SIGNAL_CAM_FLASH_EN
     */
    { GPIO_SP_A_PORT,   30, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
    
    /*
     * SCM-A11 Package Pin Name: ED_INT3
     * Lido P1 Signal: FLIP_OPEN (Misc)
     * Selected Primary Function: GP_AP_C21 (Input)
     * Selected Secondary Function: ED_INT3 (Input)
     *
     * Array index: 13  GPIO_SIGNAL_FLIP_OPEN
     */
    { GPIO_AP_C_PORT,   21, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },
    
    /*
     * SCM-A11 Package Pin Name: SPI1_CLK
     * Lido P1 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A27 (Output)
     *
     * Array index: 14  GPIO_SIGNAL_SP_A27
     */
    { GPIO_SP_A_PORT,   27, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
    
    /*
     * SCM-A11 Package Pin Name: SPI1_MISO
     * Lido P1 Signal: EL_EN (Backlight)
     * Selected Primary Function: GP_SP_A29 (Output)
     *
     * Array index: 15  GPIO_SIGNAL_EL_EN
     */
    { GPIO_SP_A_PORT,   29, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_B18
     * Lido P1 Signal: GPIO_DISP_RST_B (Display)
     * Selected Primary Function: GP_AP_B18 (Output)
     *
     * Array index: 16  GPIO_SIGNAL_DISP_RST_B
     *
     * MBM will set this at boot.
     */
    { GPIO_AP_B_PORT,   18, GPIO_GDIR_OUTPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C12
     * Lido P1 Signal: NC (NC)
     * Selected Primary Function: GP_AP_C12 (Output)
     *
     * Array index: 17  GPIO_SIGNAL_AP_C12
     */
    { GPIO_AP_C_PORT,   12, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: GP_AP_C13
     * Lido P1 Signal: BT_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C13 (Output)
     *
     * Array index: 18  GPIO_SIGNAL_BT_WAKE_B
     */
    { GPIO_AP_C_PORT,   13, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: U3_RTS_B
     * Lido P1 Signal: NC (NC)
     * Lido P2 Signal: LIN_VIB_AMP_EN (Haptics)
     * Selected Primary Function: GP_SP_A2 (Output)
     *
     * Array index: 19  GPIO_SIGNAL_SP_A2
     */
    { GPIO_INVALID_PORT,     2, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: U3_CTS_B
     * Lido P1 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A3 (Output)
     *
     * Array index: 20  GPIO_SIGNAL_SP_A3
     */
    { GPIO_SP_A_PORT,    3, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_TXOE_B
     * Lido P1 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A8 (Output)
     *
     * Array index: 21  GPIO_SIGNAL_SP_A8
     */
    { GPIO_SP_A_PORT,    8, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_SPEED
     * Lido P1 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A9 (Output)
     *
     * Array index: 22  GPIO_SIGNAL_SP_A9
     */
    { GPIO_SP_A_PORT,    9, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_RXDP
     * Lido P1 Signal: TOUCH_INTB (Morphing)
     * Selected Primary Function: GP_SP_A12 (Output)
     *
     * Array index: 23  GPIO_SIGNAL_TOUCH_INTB
     *
     * Capacitive Touch Keys -- Interrupt
     */
    { GPIO_SP_A_PORT,   12, GPIO_GDIR_INPUT,   GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: UH2_OVR
     * Lido P1 Signal: USB_XCVR_EN (Atlas)
     * Selected Primary Function: GP_SP_A14 (Output)
     *
     * Array index: 24  GPIO_SIGNAL_USB_XCVR_EN
     */
    { GPIO_SP_A_PORT,   14, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: UH2_PWR
     * Lido P1 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A15 (Output)
     *
     * Array index: 25  GPIO_SIGNAL_SP_A15
     */
    { GPIO_SP_A_PORT,   15, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: U3_RTS_B
     * Lido P1 Signal: NC (NC)
     * Lido P2 Signal: LIN_VIB_AMP_EN (Haptics)
     * Selected Primary Function: GP_SP_A2 (Output)
     *
     * Array index: 26  GPIO_SIGNAL_LIN_VIB_AMP_EN
     *
     * Dispable vibrator amplifier at boot.
     */
    { GPIO_SP_A_PORT,    2, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: ED_INT1
     * Ascension P1 Signal: BT_HOST_WAKE_B (Bluetooth)
     * Ascension P2 Signal: BT_HOST_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C19 (Input)
     * Selected Secondary Function: ED_INT1 (Input)
     *
     * Array index: 27  GPIO_SIGNAL_BT_HOST_WAKE_B
     *
     * Host wake interrupt from bluetooth controller.
     */
    { GPIO_AP_C_PORT,   19, GPIO_GDIR_INPUT,    GPIO_DATA_INVALID },

    /*
     * SCM-A11 Package Pin Name: UH2_SUSPEND
     * Lido P1 Signal: LIN_VIB_AMP_EN (Haptics)
     * Lido P2 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A10 (Output)
     *
     * Array index: 28  GPIO_SIGNAL_SP_A10
     */
    { GPIO_SP_A_PORT,   10, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: IPU_WR
     * Lido P1 Signal: NC (NC)
     * Lido P2 Signal: GPIO_SER_RST_B (Display)
     * Selected Primary Function: GP_AP_B1 (Output)
     *
     * Array index: 29  GPIO_SIGNAL_SER_RST_B
     *
     * Low means serializer is in reset.
     */
    { GPIO_AP_B_PORT,    1, GPIO_GDIR_OUTPUT,   GPIO_DATA_HIGH },

    /*
     * SCM-A11 Package Pin Name: U1_TXD
     * Lido P1 Signal: BT_RX_AP_TX (Bluetooth)
     * Selected Primary Function: U1_TXD (Output)
     * Mux0 Function: GP_AP_A7
     *
     * Lido uses Broadcom Bluetooth controller with internal pull downs,
     * so drive BT pins low when inactive to decrease current drain.
     *
     * Array index: 30  GPIO_SIGNAL_U1_TXD
     */
    { GPIO_AP_A_PORT,        7, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: U1_CTS_B
     * Lido P1 Signal: BT_CTS_B (Bluetooth)
     * Selected Primary Function: U1_CTS_B (Output)
     * Mux0 Function: GP_AP_A10
     *
     * Lido uses Broadcom Bluetooth controller with internal pull downs,
     * so drive BT pins low when inactive to decrease current drain.
     *
     * Array index: 31  GPIO_SIGNAL_U1_CTS_B
     */
    { GPIO_AP_A_PORT,       10, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },

    /*
     * SCM-A11 Package Pin Name: IPU_D3_CLK
     * Lido P1 Signal: IPU_D3_CLK (Display)
     * Selected Primary Function: IPU_D3_CLK (Output)
     *
     * Array index: 32  GPIO_SIGNAL_IPU_D3_CLK
     *
     * Prevent IPU_D3_CLK from floating when the display clock is 
     * disabled.
     */
    { GPIO_AP_A_PORT,   23, GPIO_GDIR_OUTPUT,   GPIO_DATA_LOW },
};


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
    { IOPAD_GROUP14, 0x0000 },

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

    /* CSI (Camera) -- DSE_MAX */
    { IOPAD_GROUP22, 0x0004 },

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
 * Update the initial_gpio_settings array based on the board revision. This
 * function is called by scma11phone_gpio_init() at boot.
 */
void __init lido_gpio_signal_fixup(void)
{
#ifdef CONFIG_MOT_FEAT_BRDREV
    if( (boardrev() < BOARDREV_P2A) || (boardrev() == BOARDREV_UNKNOWN) ) {
        gpio_setting_fixup_p1();
    }
#endif /* CONFIG_MOT_FEAT_BRDREV */

    return;
}


/**
 * Update the IOMUX settings where Lido varies from the other products.
 */
void __init lido_iomux_mux_fixup(void)
{
    /*
     * SCM-A11 Package Pin Name: IPU_WR
     * Lido P1 Signal: NC (NC)
     * Lido P2 Signal: GPIO_SER_RST_B (Display)
     * Selected Primary Function: GP_AP_B1 (Output)
     *
     * Primary function out of reset: IPU_WR
     * Out of Reset State: High
     * Mux0 Function: GP_AP_B1
     */
    iomux_config_mux(AP_IPU_WR, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE);
}


#ifdef CONFIG_MOT_FEAT_BRDREV
void __init gpio_setting_fixup_p1(void)
{
    /*
     * SCM-A11 Package Pin Name: U3_RTS_B
     * Lido P1 Signal: NC (NC)
     * Lido P2 Signal: LIN_VIB_AMP_EN (Haptics)
     * Selected Primary Function: GP_SP_A2 (Output)
     */
    initial_gpio_settings[GPIO_SIGNAL_SP_A2].port    = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_SP_A2].sig_no  = 2;
    initial_gpio_settings[GPIO_SIGNAL_SP_A2].out     = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_SP_A2].data    = GPIO_DATA_LOW;

    /*
     * SCM-A11 Package Pin Name: UH2_SUSPEND
     * Lido P1 Signal: LIN_VIB_AMP_EN (Haptics)
     * Lido P2 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A10 (Output)
     */
    initial_gpio_settings[GPIO_SIGNAL_SP_A10].port   = GPIO_INVALID_PORT;

    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].port   = GPIO_SP_A_PORT;
    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].sig_no = 10;
    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].out    = GPIO_GDIR_OUTPUT;
    initial_gpio_settings[GPIO_SIGNAL_LIN_VIB_AMP_EN].data   = GPIO_DATA_LOW;

    /*
     * SCM-A11 Package Pin Name: IPU_WR
     * Lido P1 Signal: NC (NC)
     * Lido P2 Signal: GPIO_SER_RST_B (Display)
     * Selected Primary Function: GP_AP_B1 (Output)
     */
    initial_gpio_settings[GPIO_SIGNAL_SER_RST_B].port    = GPIO_INVALID_PORT;
}
#endif /* CONFIG_MOT_FEAT_BRDREV */
