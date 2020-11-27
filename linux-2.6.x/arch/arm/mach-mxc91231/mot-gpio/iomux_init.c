/*
 * linux/arch/arm/mach-mxc91231/mot-gpio/iomux_init.c
 *
 * Initial IOMUX register settings for SCM-A11 platform. Settings based
 * on SCM-A11 platform pin list dated 22-Sep-2006.
 *
 * Copyright (C) 2007-2008 Motorola, Inc.
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
 * 30-Jan-2007  Motorola        Initial revision.
 * 01-Apr-2008  Motorola        Set input_config of ED_INT0 to FUNC1 for Nevis.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/mot-gpio.h>

#include "mot-gpio-scma11.h"

/**
 * Initial IOMUX settings.
 */
struct iomux_initialization {
    enum iomux_pins             pin;    
    enum iomux_output_config    output_config;
    enum iomux_input_config     input_config;
};

/**
 * 'AUDIO' block IOMUX settings
 */
static struct iomux_initialization audio_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: AD1_TXD
     * SCM-A11 Reference P3A Wingboard Signal: ASAP_TX (Atlas)
     * Ascension P3 Signal: ASAP_TX (Atlas)
     * Selected Primary Function: AD1_TXD (Output)
     *
     * Primary function out of reset: AD1_TXD
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A11
     */
    { AP_AD1_TXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: AD1_RXD
     * SCM-A11 Reference P3A Wingboard Signal: ASAP_RX (Atlas)
     * Ascension P3 Signal: ASAP_RX (Atlas)
     * Selected Primary Function: AD1_RXD (Input)
     *
     * Primary function out of reset: AD1_RXD
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A12
     */
    { AP_AD1_RXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: AD1_TXC
     * SCM-A11 Reference P3A Wingboard Signal: ASAP_CLK (Atlas)
     * Ascension P3 Signal: ASAP_CLK (Atlas)
     * Selected Primary Function: AD1_TXC (Input/Output)
     *
     * Primary function out of reset: AD1_TXC
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A13
     */
    { AP_AD1_TXC, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: AD1_TXFS
     * SCM-A11 Reference P3A Wingboard Signal: ASAP_FS (Atlas)
     * Ascension P3 Signal: ASAP_FS (Atlas)
     * Selected Primary Function: AD1_TXFS (Input/Output)
     *
     * Primary function out of reset: AD1_TXFS
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A14
     */
    { AP_AD1_TXFS, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: AD2_TXD
     * SCM-A11 Reference P3A Wingboard Signal: MMSAP_TX (Atlas)
     * Ascension P3 Signal: MMSAP_TX (Atlas)
     * Selected Primary Function: AD2_TXD (Output)
     *
     * Primary function out of reset: AD2_TXD
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A15
     */
    { AP_AD2_TXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: AD2_RXD
     * SCM-A11 Reference P3A Wingboard Signal: MMSAP_RX (Atlas)
     * Ascension P3 Signal: MMSAP_RX (Atlas)
     * Selected Primary Function: AD2_RXD (Input)
     *
     * Primary function out of reset: AD2_RXD
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A16
     */
    { AP_AD2_RXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: AD2_TXC
     * SCM-A11 Reference P3A Wingboard Signal: MMSAP_CLK (Atlas)
     * Ascension P3 Signal: MMSAP_CLK (Atlas)
     * Selected Primary Function: AD2_TXC (Input/Output)
     *
     * Primary function out of reset: AD2_TXC
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A17
     */
    { AP_AD2_TXC, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: AD2_TXFS
     * SCM-A11 Reference P3A Wingboard Signal: MMSAP_FS (Atlas)
     * Ascension P3 Signal: MMSAP_FS (Atlas)
     * Selected Primary Function: AD2_TXFS (Input/Output)
     *
     * Primary function out of reset: AD2_TXFS
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A18
     */
    { AP_AD2_TXFS, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'CSI' block IOMUX settings
 */
static struct iomux_initialization csi_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: CSI_D0
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_CAM_RST_B (camera)
     * Ascension P3 Signal: GPIO_CAM_RST_B (camera)
     * Selected Primary Function: GP_AP_B24 (Output)
     *
     * Primary function out of reset: CSI_D0
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B24
     */
    { AP_CSI_D0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D1
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_CAM_PD (camera)
     * Ascension P3 Signal: GPIO_CAM_PD (camera)
     * Selected Primary Function: GP_AP_B25 (Output)
     *
     * Primary function out of reset: CSI_D1
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B25
     */
    { AP_CSI_D1, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D2
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(0) (camera)
     * Ascension P3 Signal: CSI_D(0) (camera)
     * Selected Primary Function: CSI_D2 (Input)
     *
     * Primary function out of reset: CSI_D2
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B26
     */
    { AP_CSI_D2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D3
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(1) (camera)
     * Ascension P3 Signal: CSI_D(1) (camera)
     * Selected Primary Function: CSI_D3 (Input)
     *
     * Primary function out of reset: CSI_D3
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B27
     */
    { AP_CSI_D3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D4
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(2) (camera)
     * Ascension P3 Signal: CSI_D(2) (camera)
     * Selected Primary Function: CSI_D4 (Input)
     *
     * Primary function out of reset: CSI_D4
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B28
     */
    { AP_CSI_D4, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D5
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(3) (camera)
     * Ascension P3 Signal: CSI_D(3) (camera)
     * Selected Primary Function: CSI_D5 (Input)
     *
     * Primary function out of reset: CSI_D5
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B29
     */
    { AP_CSI_D5, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D6
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(4) (camera)
     * Ascension P3 Signal: CSI_D(4) (camera)
     * Selected Primary Function: CSI_D6 (Input)
     *
     * Primary function out of reset: CSI_D6
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B30
     */
    { AP_CSI_D6, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D7
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(5) (camera)
     * Ascension P3 Signal: CSI_D(5) (camera)
     * Selected Primary Function: CSI_D7 (Input)
     *
     * Primary function out of reset: CSI_D7
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B31
     */
    { AP_CSI_D7, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D8
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(6) (camera)
     * Ascension P3 Signal: CSI_D(6) (camera)
     * Selected Primary Function: CSI_D8 (Input)
     *
     * Primary function out of reset: CSI_D8
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C0
     */
    { AP_CSI_D8, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_D9
     * SCM-A11 Reference P3A Wingboard Signal: CSI_D(7) (camera)
     * Ascension P3 Signal: CSI_D(7) (camera)
     * Selected Primary Function: CSI_D9 (Input)
     *
     * Primary function out of reset: CSI_D9
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C1
     */
    { AP_CSI_D9, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_MCLK
     * SCM-A11 Reference P3A Wingboard Signal: CSI_MCLK (camera)
     * Ascension P3 Signal: CSI_MCLK (camera)
     * Selected Primary Function: CSI_MCLK (Output)
     *
     * Primary function out of reset: CSI_MCLK
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_C2
     */
    { AP_CSI_MCLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_VSYNC
     * SCM-A11 Reference P3A Wingboard Signal: CSI_VSYNC (camera)
     * Ascension P3 Signal: CSI_VSYNC (camera)
     * Selected Primary Function: CSI_VSYNC (Input)
     *
     * Primary function out of reset: CSI_VSYNC
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C3
     */
    { AP_CSI_VSYNC, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_HSYNC
     * SCM-A11 Reference P3A Wingboard Signal: CSI_HSYNC (camera)
     * Ascension P3 Signal: CSI_HSYNC (camera)
     * Selected Primary Function: CSI_HSYNC (Input)
     *
     * Primary function out of reset: CSI_HSYNC
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C4
     */
    { AP_CSI_HSYNC, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CSI_PIXCLK
     * SCM-A11 Reference P3A Wingboard Signal: CSI_PIXCLK (camera)
     * Ascension P3 Signal: CSI_PIXCLK (camera)
     * Selected Primary Function: CSI_PIXCLK (Input)
     *
     * Primary function out of reset: CSI_PIXCLK
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C5
     */
    { AP_CSI_PIXCLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'CSPI' block IOMUX settings
 */
static struct iomux_initialization cspi_iomux_settings[] __initdata = {
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
    { SP_SPI1_CLK, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
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
    { SP_SPI1_MOSI, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
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
    { SP_SPI1_MISO, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: SPI1_SS0
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_CAM_FLASH_STROBE (Camera Flash)
     * Ascension P3 Signal: CAM_FLASH_EN (Camera Flash)
     * Selected Primary Function: GP_SP_A30 (Output)
     *
     * Primary function out of reset: SPI1_SS0
     * Out of Reset State: Output
     * Mux0 Function: GP_SP_A30
     */
    { SP_SPI1_SS0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
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
    { SP_SPI1_SS1, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'GPIO / INT' block IOMUX settings
 */
static struct iomux_initialization gpio_int_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: GP_AP_C8
     * SCM-A11 Reference P3A Wingboard Signal: BT_RESET_B (Bluetooth)
     * Ascension P3 Signal: BT_RESET_B (Bluetooth)
     * Selected Primary Function: GP_AP_C8 (Output)
     *
     * Primary function out of reset: GP_AP_C8
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C8
     */
    { AP_GPIO_AP_C8, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C9
     * SCM-A11 Reference P3A Wingboard Signal: BOOT_RS232_USB (Misc/Wing Conn)
     * Ascension P3 Signal: USB_HS_WAKEUP/10K PD (USB HS)
     * Selected Primary Function: GP_AP_C9 (Output)
     *
     * Primary function out of reset: GP_AP_C9
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C9
     */
    { AP_GPIO_AP_C9, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
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
    { AP_GPIO_AP_C10, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C11
     * SCM-A11 Reference P3A Wingboard Signal: FM_INTERRUPT (FM Radio)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_AP_C11 (Input)
     *
     * Primary function out of reset: GP_AP_C11
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C11
     */
    { AP_GPIO_AP_C11, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C12
     * SCM-A11 Reference P3A Wingboard Signal: WLAN_RESET (WLAN)
     * Ascension P3 Signal: WLAN_RESET (WLAN)
     * Selected Primary Function: GP_AP_C12 (Output)
     *
     * Primary function out of reset: GP_AP_C12
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C12
     */
    { AP_GPIO_AP_C12, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C13
     * SCM-A11 Reference P3A Wingboard Signal: BT_WAKE_B (Bluetooth)
     * Ascension P3 Signal: BT_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C13 (Output)
     *
     * Primary function out of reset: GP_AP_C13
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C13
     */
    { AP_GPIO_AP_C13, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C14
     * SCM-A11 Reference P3A Wingboard Signal: IPU_SD_D1_CS (Display)
     * Ascension P3 Signal: IPU_SD_D1_CS (Display)
     * Selected Primary Function: IPU_D1_CS (Output)
     *
     * Primary function out of reset: GP_AP_C14
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C14
     */
    { AP_GPIO_AP_C14, OUTPUTCONFIG_FUNC3, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C15
     * SCM-A11 Reference P3A Wingboard Signal: WDOG_AP (Atlas)
     * Ascension P3 Signal: WDOG_AP (Atlas)
     * Selected Primary Function: WDOG_AP (Output)
     *
     * Primary function out of reset: GP_AP_C15
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C15
     */
    { AP_GPIO_AP_C15, OUTPUTCONFIG_FUNC4, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C16
     * SCM-A11 Reference P3A Wingboard Signal: USB_VPIN (Atlas)
     * Ascension P3 Signal: USB_VPIN (Atlas)
     * Selected Primary Function: GP_AP_C16 (Input)
     * Selected Secondary Function: USB_VP1 (Input)
     *
     * Primary function out of reset: GP_AP_C16
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C16
     */
    { AP_GPIO_AP_C16, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC3 },
    /*
     * SCM-A11 Package Pin Name: GP_AP_C17
     * SCM-A11 Reference P3A Wingboard Signal: USB_VMIN (Atlas)
     * Ascension P3 Signal: USB_VMIN (Atlas)
     * Selected Primary Function: GP_AP_C17 (Input)
     * Selected Secondary Function: USB_VM1 (Input)
     *
     * Primary function out of reset: GP_AP_C17
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C17
     */
    { AP_GPIO_AP_C17, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC3 },
#if defined(CONFIG_MACH_NEVIS)    
    /*
     * SCM-A11 Package Pin Name: ED_INT0
     * SCM-A11 Reference P3A Wingboard Signal: FLIP_OPEN (MISC)
     * Nevis P1A Signal: FLIP_OPEN (MISC)
     * Selected Primary Function: GP_AP_C18 (Input)
     * Selected Secondary Function: ED_INT0 (Input)
     *
     * Primary function out of reset: ED_INT0
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C18
     */
    { AP_ED_INT0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
#elif defined(CONFIG_MACH_MARCO) || defined(CONFIG_MACH_PICO) || defined(CONFIG_MACH_XPIXL)
    /*
     * SCM-A11 Package Pin Name: ED_INT0
     * SCM-A11 Reference P3A Wingboard Signal: WLAN_HOST_WAKE_B (WLAN)
     * Ascension P3 Signal: WLAN_HOST_WAKE_B (WLAN)
     * Selected Primary Function: GP_AP_C18 (Input)
     * Selected Secondary Function: ED_INT0 (Input)
     *
     * Primary function out of reset: ED_INT0
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C18
     */
    { AP_ED_INT0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
#else 
    /*
     * SCM-A11 Package Pin Name: ED_INT0
     * SCM-A11 Reference P3A Wingboard Signal: WLAN_HOST_WAKE_B (WLAN)
     * Ascension P3 Signal: WLAN_HOST_WAKE_B (WLAN)
     * Selected Primary Function: GP_AP_C18 (Input)
     * Selected Secondary Function: ED_INT0 (Input)
     *
     * Primary function out of reset: ED_INT0
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C18
     */
    { AP_ED_INT0, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#endif    
    /*
     * SCM-A11 Package Pin Name: ED_INT1
     * SCM-A11 Reference P3A Wingboard Signal: BT_HOST_WAKE_B (Bluetooth)
     * Ascension P3 Signal: BT_HOST_WAKE_B (Bluetooth)
     * Selected Primary Function: GP_AP_C19 (Input)
     * Selected Secondary Function: ED_INT1 (Input)
     *
     * Primary function out of reset: ED_INT1
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C19
     */
    { AP_ED_INT1, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: ED_INT2
     * SCM-A11 Reference P3A Wingboard Signal: WDOG_BP (Misc)
     * Ascension P3 Signal: WDOG_BP (Misc)
     * Selected Primary Function: GP_AP_C20 (Input)
     * Selected Secondary Function: ED_INT2 (Input)
     *
     * Primary function out of reset: ED_INT2
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C20
     */
    { AP_ED_INT2, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: ED_INT3
     * SCM-A11 Reference P3A Wingboard Signal: SLIDER_OPEN / CPLD_INT/LOCK (Misc)
     * Ascension P3 Signal: SLIDER_OPEN (Misc)
     * Selected Primary Function: GP_AP_C21 (Input)
     * Selected Secondary Function: ED_INT3 (Input)
     *
     * Primary function out of reset: ED_INT3
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C21
     */
    { AP_ED_INT3, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: ED_INT4
     * SCM-A11 Reference P3A Wingboard Signal: TF_DET (MMC)
     * Ascension P3 Signal: TF_DET (MMC)
     * Selected Primary Function: GP_AP_C22 (Input)
     * Selected Secondary Function: ED_INT4 (Input)
     *
     * Primary function out of reset: ED_INT4
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C22
     */
#if defined(CONFIG_MACH_NEVIS) 
    { AP_ED_INT4, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#else  
    { AP_ED_INT4, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
#endif
    /*
     * SCM-A11 Package Pin Name: ED_INT5
     * SCM-A11 Reference P3A Wingboard Signal: PM_INT (Atlas)
     * Ascension P3 Signal: PM_INT (Atlas)
     * Selected Primary Function: GP_AP_C23 (Input)
     * Selected Secondary Function: ED_INT5 (Input)
     *
     * Primary function out of reset: ED_INT5
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C23
     */
    { AP_ED_INT5, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: ED_INT6
     * SCM-A11 Reference P3A Wingboard Signal: USB_HS_INT (Wing Connector)
     * Ascension P3 Signal: USB_HS_INT (USB HS)
     * Selected Primary Function: GP_AP_C24 (Input)
     * Selected Secondary Function: ED_INT6 (Input)
     *
     * Primary function out of reset: ED_INT6
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C24
     */
    { AP_ED_INT6, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: ED_INT7
     * SCM-A11 Reference P3A Wingboard Signal: NC (NC)
     * Ascension P3 Signal: 2CON_DET_B (Misc)
     * Selected Primary Function: GP_AP_C25 (Input)
     * Selected Secondary Function: ED_INT7 (Input)
     *
     * Primary function out of reset: ED_INT7
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C25
     */
#if defined(CONFIG_MACH_NEVIS) 
    { AP_ED_INT7, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#else 
    { AP_ED_INT7, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_FUNC1 },
#endif
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'I2C' block IOMUX settings
 */
static struct iomux_initialization i2c_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: I2CLK
     * SCM-A11 Reference P3A Wingboard Signal: I2C_CLK (camera)
     * Ascension P3 Signal: I2C_CLK (Camera/USB_HS)
     * Selected Primary Function: I2CLK (Input/Output)
     *
     * Primary function out of reset: I2CLK
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C6
     */
    { AP_I2CLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: I2DAT
     * SCM-A11 Reference P3A Wingboard Signal: I2C_DAT (camera)
     * Ascension P3 Signal: I2C_DAT (Camera/USB_HS)
     * Selected Primary Function: I2DAT (Input/Output)
     *
     * Primary function out of reset: I2DAT
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C7
     */
    { AP_I2DAT, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'IPU' block IOMUX settings
 */
static struct iomux_initialization ipu_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: IPU_BE1_LD17
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D0_VSYNC_LD17 (Display)
     * Ascension P3 Signal: IPU_D0_VSYNC_LD17 (Display)
     * Selected Primary Function: IPU_BE1_LD17 (Output)
     *
     * Primary function out of reset: IPU_LD17
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A20
     */
#if defined(CONFIG_MACH_NEVIS) 
    { AP_IPU_LD17, OUTPUTCONFIG_FUNC2, INPUTCONFIG_NONE },
#else 
    { AP_IPU_LD17, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
#endif
    /*
     * SCM-A11 Package Pin Name: IPU_D3_VSYNC
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D3_VSYNC (Display)
     * Ascension P3 Signal: IPU_D3_VSYNC (Display)
     * Selected Primary Function: IPU_D3_VSYNC (Output)
     *
     * Primary function out of reset: IPU_D3_VSYNC
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A21
     */
    { AP_IPU_D3_VSYNC, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_D3_HSYNC
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D3_HSYNC (Display)
     * Ascension P3 Signal: IPU_D3_HSYNC (Display)
     * Selected Primary Function: IPU_D3_HSYNC (Output)
     *
     * Primary function out of reset: IPU_D3_HSYNC
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A22
     */
    { AP_IPU_D3_HSYNC, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_D3_CLK
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D3_CLK (Display)
     * Ascension P3 Signal: IPU_D3_CLK (Display)
     * Selected Primary Function: IPU_D3_CLK (Output)
     *
     * Primary function out of reset: IPU_D3_CLK
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A23
     */
    { AP_IPU_D3_CLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
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
    { AP_IPU_D3_DRDY, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
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
#if defined(CONFIG_MACH_NEVIS) 
    { AP_IPU_D3_CONTR, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#else  
    { AP_IPU_D3_CONTR, OUTPUTCONFIG_FUNC3, INPUTCONFIG_NONE },
#endif
    /*
     * SCM-A11 Package Pin Name: IPU_D0_CS
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_DISP_SD (Display)
     * Ascension P3 Signal: GPIO_DISP_SD (Display)
     * Selected Primary Function: GP_AP_A26 (Output)
     *
     * Primary function out of reset: IPU_D0_CS
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A26
     */
#if defined(CONFIG_MACH_NEVIS) 
    { AP_IPU_D0_CS, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
#else  
    { AP_IPU_D0_CS, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#endif
    /*
     * SCM-A11 Package Pin Name: IPU_BE0_LD16
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D1CS_LD16 (Display)
     * Ascension P3 Signal: IPU_D1CS_LD16 (Display)
     * Selected Primary Function: IPU_BE0_LD16 (Output)
     *
     * Primary function out of reset: IPU_LD16
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A27
     */
    { AP_IPU_LD16, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
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
#if defined(CONFIG_MACH_NEVIS) 
    { AP_IPU_D2_CS, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#else 
    { AP_IPU_D2_CS, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
#endif
    /*
     * SCM-A11 Package Pin Name: IPU_PAR_RS
     * SCM-A11 Reference P3A Wingboard Signal: SER_EN (Display)
     * Ascension P3 Signal: SER_EN (Display)
     * Selected Primary Function: GP_AP_A29 (Output)
     *
     * Primary function out of reset: IPU_PAR_RS
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A29
     */
#if defined(CONFIG_MACH_NEVIS) 
    { AP_IPU_PAR_RS, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
#else 
    { AP_IPU_PAR_RS, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#endif
    /*
     * SCM-A11 Package Pin Name: IPU_D3_SPL
     * SCM-A11 Reference P3A Wingboard Signal: IPU_SD_MISO (Display)
     * Ascension P3 Signal: IPU_SD_MISO (Display)
     * Selected Primary Function: IPU_SD_D_INPUT (Input)
     *
     * Primary function out of reset: IPU_D3_SPL
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A30
     */
    { AP_IPU_D3_PS, OUTPUTCONFIG_FUNC3, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_D3_CLS
     * SCM-A11 Reference P3A Wingboard Signal: IPU_D3_CLS (Display)
     * Ascension P3 Signal: IPU_SD_CLK (Display)
     * Selected Primary Function: IPU_SD_CLK (Output)
     *
     * Primary function out of reset: IPU_D3_CLS
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A31
     */
    { AP_IPU_D3_CLS, OUTPUTCONFIG_FUNC3, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_RD
     * SCM-A11 Reference P3A Wingboard Signal: IPU_SD_MOSI (Display)
     * Ascension P3 Signal: IPU_SD_MOSI (Display)
     * Selected Primary Function: IPU_SD_D_INOUT (Input/Output)
     *
     * Primary function out of reset: IPU_RD
     * Out of Reset State: High
     * Mux0 Function: GP_AP_B0
     */
#if defined(CONFIG_MACH_NEVIS) 
    { AP_IPU_RD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC3 },
#else 
    { AP_IPU_RD, OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3 },
#endif
    /*
     * SCM-A11 Package Pin Name: IPU_WR
     * SCM-A11 Reference P3A Wingboard Signal: IPU_WR (Display)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_AP_B1 (Output)
     *
     * Primary function out of reset: IPU_WR
     * Out of Reset State: High
     * Mux0 Function: GP_AP_B1
     */
#if defined(CONFIG_MACH_NEVIS) 
    { AP_IPU_WR, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
#else 
    { AP_IPU_WR, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
#endif
    /*
     * SCM-A11 Package Pin Name: IPU_LD0
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(0) (Display)
     * Ascension P3 Signal: IPU_LD(0) (Display)
     * Selected Primary Function: IPU_LD0 (Output)
     *
     * Primary function out of reset: IPU_LD0
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_IPU_LD0, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD1
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(1) (Display)
     * Ascension P3 Signal: IPU_LD(1) (Display)
     * Selected Primary Function: IPU_LD1 (Output)
     *
     * Primary function out of reset: IPU_LD1
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_IPU_LD1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD2
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(2) (Display)
     * Ascension P3 Signal: IPU_LD(2) (Display)
     * Selected Primary Function: IPU_LD2 (Output)
     *
     * Primary function out of reset: IPU_LD2
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_IPU_LD2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD3
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(3) (Display)
     * Ascension P3 Signal: IPU_LD(3) (Display)
     * Selected Primary Function: IPU_LD3 (Output)
     *
     * Primary function out of reset: IPU_LD3
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B2
     */
    { AP_IPU_LD3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD4
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(4) (Display)
     * Ascension P3 Signal: IPU_LD(4) (Display)
     * Selected Primary Function: IPU_LD4 (Output)
     *
     * Primary function out of reset: IPU_LD4
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B3
     */
    { AP_IPU_LD4, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD5
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(5) (Display)
     * Ascension P3 Signal: IPU_LD(5) (Display)
     * Selected Primary Function: IPU_LD5 (Output)
     *
     * Primary function out of reset: IPU_LD5
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B4
     */
    { AP_IPU_LD5, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD6
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(6) (Display)
     * Ascension P3 Signal: IPU_LD(6) (Display)
     * Selected Primary Function: IPU_LD6 (Output)
     *
     * Primary function out of reset: IPU_LD6
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B5
     */
    { AP_IPU_LD6, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD7
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(7) (Display)
     * Ascension P3 Signal: IPU_LD(7) (Display)
     * Selected Primary Function: IPU_LD7 (Output)
     *
     * Primary function out of reset: IPU_LD7
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B6
     */
    { AP_IPU_LD7, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD8
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(8) (Display)
     * Ascension P3 Signal: IPU_LD(8) (Display)
     * Selected Primary Function: IPU_LD8 (Output)
     *
     * Primary function out of reset: IPU_LD8
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B7
     */
    { AP_IPU_LD8, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD9
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(9) (Display)
     * Ascension P3 Signal: IPU_LD(9) (Display)
     * Selected Primary Function: IPU_LD9 (Output)
     *
     * Primary function out of reset: IPU_LD9
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B8
     */
    { AP_IPU_LD9, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD10
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(10) (Display)
     * Ascension P3 Signal: IPU_LD(10) (Display)
     * Selected Primary Function: IPU_LD10 (Output)
     *
     * Primary function out of reset: IPU_LD10
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B9
     */
    { AP_IPU_LD10, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD11
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(11) (Display)
     * Ascension P3 Signal: IPU_LD(11) (Display)
     * Selected Primary Function: IPU_LD11 (Output)
     *
     * Primary function out of reset: IPU_LD11
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B10
     */
    { AP_IPU_LD11, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD12
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(12) (Display)
     * Ascension P3 Signal: IPU_LD(12) (Display)
     * Selected Primary Function: IPU_LD12 (Output)
     *
     * Primary function out of reset: IPU_LD12
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B11
     */
    { AP_IPU_LD12, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD13
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(13) (Display)
     * Ascension P3 Signal: IPU_LD(13) (Display)
     * Selected Primary Function: IPU_LD13 (Output)
     *
     * Primary function out of reset: IPU_LD13
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B12
     */
    { AP_IPU_LD13, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD14
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(14) (Display)
     * Ascension P3 Signal: IPU_LD(14) (Display)
     * Selected Primary Function: IPU_LD14 (Output)
     *
     * Primary function out of reset: IPU_LD14
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B13
     */
    { AP_IPU_LD14, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: IPU_LD15
     * SCM-A11 Reference P3A Wingboard Signal: IPU_LD(15) (Display)
     * Ascension P3 Signal: IPU_LD(15) (Display)
     * Selected Primary Function: IPU_LD15 (Output)
     *
     * Primary function out of reset: IPU_LD15
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B14
     */
    { AP_IPU_LD15, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'KEYPAD' block IOMUX settings
 */
static struct iomux_initialization keypad_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: KPROW4
     * SCM-A11 Reference P3A Wingboard Signal: KPP_ROW(4) (Keypad)
     * Ascension P3 Signal: KPP_ROW(4) (Keypad)
     * Selected Primary Function: KPROW4 (Input/Output)
     *
     * Primary function out of reset: KPROW4
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_KPROW4, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPROW5
     * SCM-A11 Reference P3A Wingboard Signal: LOBAT_B (Atlas)
     * Ascension P3 Signal: LOBAT_B (Atlas)
     * Selected Primary Function: GP_AP_B16 (Input)
     *
     * Primary function out of reset: KPROW5
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B16
     */
    { AP_KPROW5, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_B17
     * SCM-A11 Reference P3A Wingboard Signal: PWM_BKL (Linear Vibrator)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: AP_PWM (Output)
     *
     * Primary function out of reset: GP_AP_B17
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B17
     */
    { AP_GPIO_AP_B17, OUTPUTCONFIG_FUNC2, INPUTCONFIG_NONE },
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
    { AP_GPIO_AP_B18, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: KPCOL3
     * SCM-A11 Reference P3A Wingboard Signal: KPP_COL(3) (Keypad)
     * Ascension P3 Signal: KPP_COL(3) (Keypad)
     * Selected Primary Function: KPCOL3 (Input/Output)
     *
     * Primary function out of reset: KPCOL3
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B19
     */
    { AP_KPCOL3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPCOL4
     * SCM-A11 Reference P3A Wingboard Signal: KPP_COL(4) (Keypad)
     * Ascension P3 Signal: KPP_COL(4) (Keypad)
     * Selected Primary Function: KPCOL4 (Input/Output)
     *
     * Primary function out of reset: KPCOL4
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B20
     */
    { AP_KPCOL4, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPCOL5
     * SCM-A11 Reference P3A Wingboard Signal: KPP_COL(5) (Keypad)
     * Ascension P3 Signal: KPP_COL(5) (Keypad)
     * Selected Primary Function: KPCOL5 (Input/Output)
     *
     * Primary function out of reset: KPCOL5
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B21
     */
    { AP_KPCOL5, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: GP_AP_B22
     * SCM-A11 Reference P3A Wingboard Signal: USB_HS_DMA_REQ (Wing Connector)
     * Ascension P3 Signal: USB_HS_DMA_REQ (USB HS)
     * Selected Primary Function: GP_AP_B22 (Input)
     *
     * Primary function out of reset: GP_AP_B22
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B22
     */
    { AP_GPIO_AP_B22, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: GP_AP_B23
     * SCM-A11 Reference P3A Wingboard Signal: POWER_RDY (Atlas)
     * Ascension P3 Signal: POWER_RDY (Atlas)
     * Selected Primary Function: GP_AP_B23 (Input)
     *
     * Primary function out of reset: GP_AP_B23
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B23
     */
    { AP_GPIO_AP_B23, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: KPROW0
     * SCM-A11 Reference P3A Wingboard Signal: KPP_ROW(0) (Keypad)
     * Ascension P3 Signal: KPP_ROW(0) (Keypad)
     * Selected Primary Function: KPROW0 (Input/Output)
     *
     * Primary function out of reset: KPROW0
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_KPROW0, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPROW1
     * SCM-A11 Reference P3A Wingboard Signal: KPP_ROW(1) (Keypad)
     * Ascension P3 Signal: KPP_ROW(1) (Keypad)
     * Selected Primary Function: KPROW1 (Input/Output)
     *
     * Primary function out of reset: KPROW1
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_B15
     */
    { AP_KPROW1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPROW2
     * SCM-A11 Reference P3A Wingboard Signal: KPP_ROW(2) (Keypad)
     * Ascension P3 Signal: KPP_ROW(2) (Keypad)
     * Selected Primary Function: KPROW2 (Input/Output)
     *
     * Primary function out of reset: KPROW2
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_KPROW2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPROW3
     * SCM-A11 Reference P3A Wingboard Signal: KPP_ROW(3) (Keypad)
     * Ascension P3 Signal: KPP_ROW(3) (Keypad)
     * Selected Primary Function: KPROW3 (Input/Output)
     *
     * Primary function out of reset: KPROW3
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_KPROW3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPCOL0
     * SCM-A11 Reference P3A Wingboard Signal: KPP_COL(0) (Keypad)
     * Ascension P3 Signal: KPP_COL(0) (Keypad)
     * Selected Primary Function: KPCOL0 (Input/Output)
     *
     * Primary function out of reset: KPCOL0
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_KPCOL0, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPCOL1
     * SCM-A11 Reference P3A Wingboard Signal: KPP_COL(1) (Keypad)
     * Ascension P3 Signal: KPP_COL(1) (Keypad)
     * Selected Primary Function: KPCOL1 (Input/Output)
     *
     * Primary function out of reset: KPCOL1
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_KPCOL1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: KPCOL2
     * SCM-A11 Reference P3A Wingboard Signal: KPP_COL(2) (Keypad)
     * Ascension P3 Signal: KPP_COL(2) (Keypad)
     * Selected Primary Function: KPCOL2 (Input/Output)
     *
     * Primary function out of reset: KPCOL2
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { AP_KPCOL2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'MMC_SDIO / GPIO' block IOMUX settings
 */
static struct iomux_initialization mmc_sdio_gpio_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: SD1_DAT0
     * SCM-A11 Reference P3A Wingboard Signal: SD1_DATA(0)/MMC_DATA(0) (MMC/MMSIM)
     * Ascension P3 Signal: SD1_DATA(0) (MMC)
     * Selected Primary Function: SD1_DAT0 (Input/Output)
     *
     * Primary function out of reset: SD1_DAT0
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A16
     */
    { SP_SD1_DAT0, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD1_DAT1
     * SCM-A11 Reference P3A Wingboard Signal: SD1_DATA(1) (MMC)
     * Ascension P3 Signal: SD1_DATA(1) (MMC)
     * Selected Primary Function: SD1_DAT1 (Input/Output)
     *
     * Primary function out of reset: SD1_DAT1
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A17
     */
    { SP_SD1_DAT1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD1_DAT2
     * SCM-A11 Reference P3A Wingboard Signal: SD1_DATA(2) (MMC)
     * Ascension P3 Signal: SD1_DATA(2) (MMC)
     * Selected Primary Function: SD1_DAT2 (Input/Output)
     *
     * Primary function out of reset: SD1_DAT2
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A18
     */
    { SP_SD1_DAT2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD1_DAT3
     * SCM-A11 Reference P3A Wingboard Signal: SD1_DATA(3) (MMC)
     * Ascension P3 Signal: SD1_DATA(3) (MMC)
     * Selected Primary Function: SD1_DAT3 (Input/Output)
     *
     * Primary function out of reset: SD1_DAT3
     * Out of Reset State: Hi-Z
     * Mux0 Function: GP_SP_A19
     */
    { SP_SD1_DAT3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD1_CMD
     * SCM-A11 Reference P3A Wingboard Signal: SD1_CMD/MMC_CMD (MMC/MMSIM)
     * Ascension P3 Signal: SD1_CMD (MMC)
     * Selected Primary Function: SD1_CMD (Input/Output)
     *
     * Primary function out of reset: SD1_CMD
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A20
     */
    { SP_SD1_CMD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD1_CLK
     * SCM-A11 Reference P3A Wingboard Signal: SD1_CLK/MMC_SD_CLK (MMC/MMSIM)
     * Ascension P3 Signal: SD1_CLK (MMC)
     * Selected Primary Function: SD1_CLK (Output)
     *
     * Primary function out of reset: SD1_CLK
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A21
     */
    { SP_SD1_CLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: SD2_DAT0
     * SCM-A11 Reference P3A Wingboard Signal: SD2_DATA(0) (WLAN)
     * Ascension P3 Signal: SD2_DATA(0) (WLAN)
     * Selected Primary Function: SD2_DAT0 (Input/Output)
     *
     * Primary function out of reset: SD2_DAT0
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A22
     */
    { SP_SD2_DAT0, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD2_DAT1
     * SCM-A11 Reference P3A Wingboard Signal: SD2_DATA(1) (WLAN)
     * Ascension P3 Signal: SD2_DATA(1) (WLAN)
     * Selected Primary Function: SD2_DAT1 (Input/Output)
     *
     * Primary function out of reset: SD2_DAT1
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A23
     */
    { SP_SD2_DAT1, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD2_DAT2
     * SCM-A11 Reference P3A Wingboard Signal: SD2_DATA(2) (WLAN)
     * Ascension P3 Signal: SD2_DATA(2) (WLAN)
     * Selected Primary Function: SD2_DAT2 (Input/Output)
     *
     * Primary function out of reset: SD2_DAT2
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A24
     */
    { SP_SD2_DAT2, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD2_DAT3
     * SCM-A11 Reference P3A Wingboard Signal: SD2_DATA(3) (WLAN)
     * Ascension P3 Signal: SD2_DATA(3) (WLAN)
     * Selected Primary Function: SD2_DAT3 (Input/Output)
     *
     * Primary function out of reset: SD2_DAT3
     * Out of Reset State: Hi-Z
     * Mux0 Function: GP_SP_A25
     */
    { SP_SD2_DAT3, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: GP_SP_A26
     * SCM-A11 Reference P3A Wingboard Signal: WLAN_PWR_DWN_B (WLAN)
     * Ascension P3 Signal: WLAN_PWR_DWN_B (WLAN)
     * Selected Primary Function: GP_SP_A26 (Output)
     *
     * Primary function out of reset: GP_SP_A26
     * Out of Reset State: Input
     * Mux0 Function: GP_SP_A26
     */
    { SP_GPIO_Shared26, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: SD2_CMD
     * SCM-A11 Reference P3A Wingboard Signal: SD2_CMD (WLAN)
     * Ascension P3 Signal: SD2_CMD (WLAN)
     * Selected Primary Function: SD2_CMD (Input/Output)
     *
     * Primary function out of reset: SD2_CMD
     * Out of Reset State: High
     * Mux0 Function: (not defined)
     */
    { SP_SD2_CMD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SD2_CLK
     * SCM-A11 Reference P3A Wingboard Signal: SD2_CLK (WLAN)
     * Ascension P3 Signal: SD2_CLK (WLAN)
     * Selected Primary Function: SD2_CLK (Output)
     *
     * Primary function out of reset: SD2_CLK
     * Out of Reset State: Low
     * Mux0 Function: (not defined)
     */
    { SP_SD2_CLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'NADF' block IOMUX settings
 */
static struct iomux_initialization nadf_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: CLE
     * SCM-A11 Reference P3A Wingboard Signal: NAND_CLE (Memory)
     * Ascension P3 Signal: NAND_CLE (NAND)
     * Selected Primary Function: CLE (Output)
     *
     * Primary function out of reset: CLE
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A0
     */
    { AP_CLE, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: ALE
     * SCM-A11 Reference P3A Wingboard Signal: NAND_ALE (Memory)
     * Ascension P3 Signal: NAND_ALE (NAND)
     * Selected Primary Function: ALE (Output)
     *
     * Primary function out of reset: ALE
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A1
     */
    { AP_ALE, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: CE_B
     * SCM-A11 Reference P3A Wingboard Signal: NAND_CS_B (Memory)
     * Ascension P3 Signal: NAND_CS_B (NAND)
     * Selected Primary Function: CE_B (Output)
     *
     * Primary function out of reset: CE_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A2
     */
    { AP_CE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: RE_B
     * SCM-A11 Reference P3A Wingboard Signal: NAND_RE_B (Memory)
     * Ascension P3 Signal: NAND_RE_B (NAND)
     * Selected Primary Function: RE_B (Output)
     *
     * Primary function out of reset: RE_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A3
     */
    { AP_RE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: WE_B
     * SCM-A11 Reference P3A Wingboard Signal: NAND_WE_B (Memory)
     * Ascension P3 Signal: NAND_WE_B (NAND)
     * Selected Primary Function: WE_B (Output)
     *
     * Primary function out of reset: WE_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A4
     */
    { AP_WE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: WP_B
     * SCM-A11 Reference P3A Wingboard Signal: NAND_WP_B (Memory)
     * Ascension P3 Signal: NAND_WP_B (NAND)
     * Selected Primary Function: WP_B (Output)
     *
     * Primary function out of reset: WP_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A5
     */
    { AP_WP_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: BSY_B
     * SCM-A11 Reference P3A Wingboard Signal: NAND_R_B (Memory)
     * Ascension P3 Signal: NAND_R_B (NAND)
     * Selected Primary Function: BSY_B (Input)
     *
     * Primary function out of reset: BSY_B
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A6
     */
    { AP_BSY_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'ONEWIRE' block IOMUX settings
 */
static struct iomux_initialization onewire_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: OWDAT
     * SCM-A11 Reference P3A Wingboard Signal: BATT_DAT (Misc)
     * Ascension P3 Signal: BATT_DAT (Misc)
     * Selected Primary Function: OWDAT (Input/Output)
     *
     * Primary function out of reset: OWDAT
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A19
     */
    { AP_OWDAT, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'SIM' block IOMUX settings
 */
static struct iomux_initialization sim_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: SIM1_RST_B
     * SCM-A11 Reference P3A Wingboard Signal: SIM_RST (SIM)
     * Ascension P3 Signal: SIM_RST (SIM)
     * Selected Primary Function: SIM1_RST_B (Output)
     *
     * Primary function out of reset: SIM1_RST_B
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_C30
     */
    { SP_SIM1_RST_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: SIM1_SVEN
     * SCM-A11 Reference P3A Wingboard Signal: VSIM_EN (Atlas)
     * Ascension P3 Signal: VSIM_EN (Atlas)
     * Selected Primary Function: SIM1_SVEN (Output)
     *
     * Primary function out of reset: SIM1_SVEN
     * Out of Reset State: Low
     * Mux0 Function: (not defined)
     */
    { SP_SIM1_SVEN, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: SIM1_CLK
     * SCM-A11 Reference P3A Wingboard Signal: SIM_CLK (SIM)
     * Ascension P3 Signal: SIM_CLK (SIM)
     * Selected Primary Function: SIM1_CLK (Output)
     *
     * Primary function out of reset: SIM1_CLK
     * Out of Reset State: Low
     * Mux0 Function: (not defined)
     */
    { SP_SIM1_CLK, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: SIM1_TRXD
     * SCM-A11 Reference P3A Wingboard Signal: SIM_IO (SIM)
     * Ascension P3 Signal: SIM_IO (SIM)
     * Selected Primary Function: SIM1_TRXD (Input/Output)
     *
     * Primary function out of reset: SIM1_TRXD
     * Out of Reset State: Low
     * Mux0 Function: (not defined)
     */
    { SP_SIM1_TRXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: SIM1_PD
     * SCM-A11 Reference P3A Wingboard Signal: BAT_DETB (Atlas)
     * Ascension P3 Signal: BAT_DETB (Atlas)
     * Selected Primary Function: SIM1_PD (Input)
     *
     * Primary function out of reset: SIM1_PD
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C31
     */
    { SP_SIM1_PD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'UART' block IOMUX settings
 */
static struct iomux_initialization uart_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: U1_TXD
     * SCM-A11 Reference P3A Wingboard Signal: BT_TX (Bluetooth)
     * Ascension P3 Signal: BT_RX_AP_TX (Bluetooth)
     * Selected Primary Function: U1_TXD (Output)
     *
     * Primary function out of reset: U1_TXD
     * Out of Reset State: Low
     * Mux0 Function: GP_AP_A7
     */
    { AP_U1_TXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: U1_RXD
     * SCM-A11 Reference P3A Wingboard Signal: BT_RX (Bluetooth)
     * Ascension P3 Signal: BT_TX_AP_RX (Bluetooth)
     * Selected Primary Function: U1_RXD (Input)
     *
     * Primary function out of reset: U1_RXD
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A8
     */
    { AP_U1_RXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: U1_RTS_B
     * SCM-A11 Reference P3A Wingboard Signal: BT_RTS_B (Bluetooth)
     * Ascension P3 Signal: BT_RTS_B (Bluetooth)
     * Selected Primary Function: U1_RTS_B (Input)
     *
     * Primary function out of reset: U1_RTS_B
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_A9
     */
    { AP_U1_RTS_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: U1_CTS_B
     * SCM-A11 Reference P3A Wingboard Signal: BT_CTS_B (Bluetooth)
     * Ascension P3 Signal: BT_CTS_B (Bluetooth)
     * Selected Primary Function: U1_CTS_B (Output)
     *
     * Primary function out of reset: U1_CTS_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_A10
     */
    { AP_U1_CTS_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
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
    { SP_U3_TXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: U3_RXD
     * SCM-A11 Reference P3A Wingboard Signal: GPS_U3_RX (GPS)
     * Ascension P3 Signal: GPS_U3_RX (Linux Terminal)
     * Selected Primary Function: U3_RXD (Input)
     *
     * Primary function out of reset: U3_RXD
     * Out of Reset State: Input
     * Mux0 Function: GP_SP_A1
     */
    { SP_U3_RXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: U3_RTS_B
     * SCM-A11 Reference P3A Wingboard Signal: GPS_RESET (GPS)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A2 (Output)
     *
     * Primary function out of reset: U3_RTS_B
     * Out of Reset State: Input
     * Mux0 Function: GP_SP_A2
     */
    { SP_U3_RTS_B, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: U3_CTS_B
     * SCM-A11 Reference P3A Wingboard Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * Ascension P3 Signal: WLAN_CLIENT_WAKE_B (WLAN)
     * Selected Primary Function: GP_SP_A3 (Output)
     *
     * Primary function out of reset: U3_CTS_B
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A3
     */
    { SP_U3_CTS_B, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'UART2 EMU Bus' block IOMUX settings
 */
static struct iomux_initialization uart2_emu_bus_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: U2_DSR_B
     * SCM-A11 Reference P3A Wingboard Signal: DAI_TX (Misc)
     * Ascension P3 Signal: DAI_TX (Misc)
     * Selected Primary Function: AD4_TXD (Output)
     *
     * Primary function out of reset: U2_DSR_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C26
     */
    { AP_U2_DSR_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: U2_RI_B
     * SCM-A11 Reference P3A Wingboard Signal: GPIO_DISP_CM/DAI_CLK (Display)
     * Ascension P3 Signal: DAI_CLK (Misc)
     * Selected Primary Function: AD4_TXC (Input/Output)
     *
     * Primary function out of reset: U2_RI_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C27
     */
    { AP_U2_RI_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2 },
    /*
     * SCM-A11 Package Pin Name: U2_CTS_B
     * SCM-A11 Reference P3A Wingboard Signal: DAI_FS (Misc)
     * Ascension P3 Signal: DAI_FS (Misc)
     * Selected Primary Function: AD4_TXFS (Input/Output)
     *
     * Primary function out of reset: U2_CTS_B
     * Out of Reset State: High
     * Mux0 Function: GP_AP_C28
     */
    { AP_U2_CTS_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2 },
    /*
     * SCM-A11 Package Pin Name: U2_DTR_B
     * SCM-A11 Reference P3A Wingboard Signal: DAI_RX (Misc)
     * Ascension P3 Signal: DAI_RX (Misc)
     * Selected Primary Function: AD4_RXD (Input)
     *
     * Primary function out of reset: U2_DTR_B
     * Out of Reset State: Input
     * Mux0 Function: GP_AP_C29
     */
    { AP_U2_DTR_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * 'USB' block IOMUX settings
 */
static struct iomux_initialization usb_iomux_settings[] __initdata = {
    /*
     * SCM-A11 Package Pin Name: USB_TXOE_B
     * SCM-A11 Reference P3A Wingboard Signal: USB_TXEN_B (Atlas)
     * Ascension P3 Signal: USB_TXEN_B (Atlas)
     * Selected Primary Function: USB_TXOE_B (Output)
     *
     * Primary function out of reset: USB_TXOE_B
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A4
     */
    { SP_USB_TXOE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: USB_DAT_VP
     * SCM-A11 Reference P3A Wingboard Signal: USB_VPOUT (Atlas)
     * Ascension P3 Signal: USB_VPOUT (Atlas)
     * Selected Primary Function: USB_DAT_VP (Input/Output)
     *
     * Primary function out of reset: USB_DAT_VP
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A5
     */
    { SP_USB_DAT_VP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: USB_SE0_VM
     * SCM-A11 Reference P3A Wingboard Signal: USB_VMOUT (Atlas)
     * Ascension P3 Signal: USB_VMOUT (Atlas)
     * Selected Primary Function: USB_SE0_VM (Input/Output)
     *
     * Primary function out of reset: USB_SE0_VM
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A6
     */
    { SP_USB_SE0_VM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1 },
    /*
     * SCM-A11 Package Pin Name: USB_RXD
     * SCM-A11 Reference P3A Wingboard Signal: USB_XRXD (Atlas)
     * Ascension P3 Signal: USB_XRXD (Atlas)
     * Selected Primary Function: USB_RXD (Input)
     *
     * Primary function out of reset: USB_RXD
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A7
     */
    { SP_USB_RXD, OUTPUTCONFIG_FUNC1, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_TXOE_B
     * SCM-A11 Reference P3A Wingboard Signal: OMEGA_INTERRUPT (Omega Wheel)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A8 (Input)
     *
     * Primary function out of reset: UH2_TXOE_B
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A8
     */
    { SP_UH2_TXOE_B, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_SPEED
     * SCM-A11 Reference P3A Wingboard Signal: VFUSE_SELECT/FM_RESET (Misc/FM Radio)
     * Ascension P3 Signal: VFUSE_SELECT (Misc)
     * Selected Primary Function: GP_SP_A9 (Output)
     *
     * Primary function out of reset: UH2_SPEED
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A9
     */
    { SP_UH2_SPEED, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_SUSPEND
     * SCM-A11 Reference P3A Wingboard Signal: LIN_VIB_AMP_EN (Linear Vibrator)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A10 (Output)
     *
     * Primary function out of reset: UH2_SUSPEND
     * Out of Reset State: High
     * Mux0 Function: GP_SP_A10
     */
    { SP_UH2_SUSPEND, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_TXDP
     * SCM-A11 Reference P3A Wingboard Signal: USB_HS_SWITCH (Wing Connector)
     * Ascension P3 Signal: USB_HS_SWITCH (USB HS)
     * Selected Primary Function: GP_SP_A11 (Output)
     *
     * Primary function out of reset: UH2_TXDP
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A11
     */
    { SP_UH2_TXDP, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_RXDP
     * SCM-A11 Reference P3A Wingboard Signal: UH2_RXDP (Morphing/Saipan C)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A12 (Input)
     *
     * Primary function out of reset: UH2_RXDP
     * Out of Reset State: Input
     * Mux0 Function: GP_SP_A12
     */
    { SP_UH2_RXDP, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_RXDM
     * SCM-A11 Reference P3A Wingboard Signal: CAM_TORCH_EN (Camera Flash)
     * Ascension P3 Signal: CAM_TORCH_EN (Camera Flash)
     * Selected Primary Function: GP_SP_A13 (Output)
     *
     * Primary function out of reset: UH2_RXDM
     * Out of Reset State: Input
     * Mux0 Function: GP_SP_A13
     */
    { SP_UH2_RXDM, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_OVR
     * SCM-A11 Reference P3A Wingboard Signal: USB_XCVR_EN (Atlas)
     * Ascension P3 Signal: USB_XCVR_EN (Atlas)
     * Selected Primary Function: GP_SP_A14 (Output)
     *
     * Primary function out of reset: UH2_OVR
     * Out of Reset State: Input
     * Mux0 Function: GP_SP_A14
     */
    { SP_UH2_OVR, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_PWR
     * SCM-A11 Reference P3A Wingboard Signal: UH2_PWR (Saipan Connector)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: GP_SP_A15 (Output)
     *
     * Primary function out of reset: UH2_PWR
     * Out of Reset State: Low
     * Mux0 Function: GP_SP_A15
     */
    { SP_UH2_PWR, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_TXDM
     * SCM-A11 Reference P3A Wingboard Signal: TP (NC)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: Unassigned (Input)
     *
     * Primary function out of reset: UH2_TXDM
     * Out of Reset State: Low
     * Mux0 Function: (not defined)
     */
    { SP_UH2_TXDM, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /*
     * SCM-A11 Package Pin Name: UH2_RXD
     * SCM-A11 Reference P3A Wingboard Signal: TP (NC)
     * Ascension P3 Signal: NC (NC)
     * Selected Primary Function: Unassigned (Input)
     *
     * Primary function out of reset: UH2_RXD
     * Out of Reset State: Input
     * Mux0 Function: (not defined)
     */
    { SP_UH2_RXD, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE },
    /* list terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_DEFAULT }
};


/**
 * Global IOMUX settings
 */
static struct iomux_initialization *initial_iomux_settings[] __initdata = {
    audio_iomux_settings,
    csi_iomux_settings,
    cspi_iomux_settings,
    gpio_int_iomux_settings,
    i2c_iomux_settings,
    ipu_iomux_settings,
    keypad_iomux_settings,
    mmc_sdio_gpio_iomux_settings,
    nadf_iomux_settings,
    onewire_iomux_settings,
    sim_iomux_settings,
    uart_iomux_settings,
    uart2_emu_bus_iomux_settings,
    usb_iomux_settings,
    /* list terminator */
    NULL
};


/**
 * Set IOMUX pad registers to the state defined in the SCM-A11 platform
 * pin list.
 */
void __init scma11_iomux_pad_init(void)
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
void __init scma11_iomux_mux_init(void)
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
