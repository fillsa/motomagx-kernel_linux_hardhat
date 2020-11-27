/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/iomux_buteref.c
 *
 * IOMUX mux control register settings for BUTE boards using IPU display.
 *
 * Copyright 2006 Motorola, Inc.
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
 */

#include "mot-gpio-argonlv.h"

/**
 * CSI Camera Sensor Interface IOMUX settings
 */
struct iomux_initialization csi_iomux_settings[] __initdata = {
    /*
     * Pin Name: IPU_CSI_DATA_0
     * Bute3A Signal Name: IPU_CSI_DATA[0]
     * Bute4A Signal Name: CSI_D[0]
     * Functional Mode: ipp_ind_sensb_data[6] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_1
     * Bute3A Signal Name: IPU_CSI_DATA[1]
     * Bute4A Signal Name: CSI_D[1]
     * Functional Mode: ipp_ind_sensb_data[7] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_2
     * Bute3A Signal Name: IPU_CSI_DATA[2]
     * Bute4A Signal Name: CSI_D[2]
     * Functional Mode: ipp_ind_sensb_data[8] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_3
     * Bute3A Signal Name: IPU_CSI_DATA[3]
     * Bute4A Signal Name: CSI_D[3]
     * Functional Mode: ipp_ind_sensb_data[9] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_4
     * Bute3A Signal Name: IPU_CSI_DATA[4]
     * Bute4A Signal Name: CSI_D[4]
     * Functional Mode: ipp_ind_sensb_data[10] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_5
     * Bute3A Signal Name: IPU_CSI_DATA[5]
     * Bute4A Signal Name: CSI_D[5]
     * Functional Mode: ipp_ind_sensb_data[11] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_6
     * Bute3A Signal Name: IPU_CSI_DATA[6]
     * Bute4A Signal Name: CSI_D[6]
     * Functional Mode: ipp_ind_sensb_data[12] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_7
     * Bute3A Signal Name: IPU_CSI_DATA[7]
     * Bute4A Signal Name: CSI_D[7]
     * Functional Mode: ipp_ind_sensb_data[13] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_8
     * Bute3A Signal Name: IPU_CSI_DATA[8]
     * Bute4A Signal Name: CSI_D[8]
     * Functional Mode: ipp_ind_sensb_data[14] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_DATA_9
     * Bute3A Signal Name: IPU_CSI_DATA[9]
     * Bute4A Signal Name: CSI_D[9]
     * Functional Mode: ipp_ind_sensb_data[15] (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_DATA_9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_MCLK
     * Bute3A Signal Name: IPU_CSI_MCLK
     * Bute4A Signal Name: CSI_MCLK
     * Functional Mode: ipp_do_sensb_mstr_clk (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_MCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_VSYNC
     * Bute3A Signal Name: IPU_CSI_VSYNC
     * Bute4A Signal Name: CSI_VSYNC
     * Functional Mode: ipp_ind_sensb_vsync (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_VSYNC, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_HSYNC
     * Bute3A Signal Name: IPU_CSI_HSYNC
     * Bute4A Signal Name: CSI_HSYNC
     * Functional Mode: ipp_ind_sensb_hsync (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_HSYNC, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: IPU_CSI_PIXCLK
     * Bute3A Signal Name: IPU_CSI_PIXCLK
     * Bute4A Signal Name: CSI_PIXCLK
     * Functional Mode: ipp_ind_sensb_pix_clk (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_IPU_CSI_PIXCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};

/**
 *  IPU Image Processing Unit IOMUX settings
 */
struct iomux_initialization ipu_iomux_settings[] __initdata = {
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};

/**
 *  GPIO General Purpose Input/Output IOMUX settings
 */
struct iomux_initialization gpio_iomux_settings[] __initdata = {
    /*
     * Pin Name: GPIO7
     * ButeP3 (brass) Signal: GPS_RESET_B
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: GPU_VCORE1_EN/PWGT1EN
     * Functional Mode: GPIO MCU Port Pin 7 (Output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO12
     * ButeP4 (close) Signal: KPD_EL_EN2
     * ButeP4 (wing)  Signal: KPD_EL_EN2
     * ButeP5 (close) Signal: GPS_RESET_B
     * Functional Mode: GPIO MCU Port Pin 12 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO36
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: GPU_DPD_B
     * Functional Mode: GPIO MCU Port Pin 23 (output) (_NOT_ present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO36, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: USB1_VPOUT
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: APPS_CLK_EN_B
     * GPIO Mode: MCU Port Pin 31 (output) (_NOT_ present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_USB1_VPOUT, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * Pin Name: USB1_VMOUT
     * Bute3A Signal Name: GPU_RESET_B / GPIO_DISP_RST_B
     * Bute4A Signal Name: SERDES_RESET_B
     * GPIO Mode: MCU Port Pin 28 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_USB1_VMOUT, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * Pin Name: USB1_TXENB
     * Bute3A Signal Name: GPU_INT_B / GPIO_DISP_CM
     * Bute4A Signal Name: STBY
     * GPIO Mode: MCU Port Pin 29 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_USB1_TXENB, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * Pin Name: USB1_VPIN
     * Bute3A Signal Name: IPU_MAIN_BKLT_EN_B / GPU_A2
     * Bute4A Signal Name: LCD_Backlight
     * GPIO Mode: MCU Port Pin 30 (output) (not present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_USB1_VPIN, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * Pin Name: USB1_XRXD
     * ButeP3 (brass) Signal: GPIO_CLI_RST_B
     * ButeP4 (close) Signal: GPIO_CLI_RST_B
     * ButeP4 (wing)  Signal: GPIO_CLI_RST_B/GPS_RESET_B
     * ButeP5 (close) Signal: BT_RX
     * GPIO Mode: MCU Port Pin 1 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_USB1_XRXD, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * Pin Name: GPIO23
     * Bute3A Signal Name: GPIO_CAM_RST_B
     * Bute4A Signal Name: GPIO_CAM_RST_B
     * Functional Mode: GPIO Shared Port Pin 3 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO23, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO15
     * Bute3A Signal Name: CSI_CS0
     * Bute4A Signal Name: GPIO_CAM_EXT_PWRDN
     * Functional Mode: GPIO MCU Port Pin 15 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO16
     * Bute3A Signal Name: CSI_CS1
     * Bute4A Signal Name: GPIO_CAM_INT_PWRDN
     * Functional Mode: GPIO MCU Port Pin 16 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO17
     * Bute3A Signal Name: SD1_DET_B
     * Bute4A Signal Name: SD1_DET_B
     * Functional Mode: GPIO MCU Port Pin 17 (input) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     *
     */
    { PIN_GPIO17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO14
     * Bute3A Signal: IRDA_SD
     * Bute4A Signal: LCD_SD
     * Functional Mode: GPIO MCU Port Pin 14 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* 
     * Pin Name: GPIO8
     * Bute3A Signal: DCM_HS_DET_B/ENET_INT_B
     * Bute4A Signal: GPIO_CAM_Flash_T_F
     * Functional Mode: GPIO MCU Port Pin 8 (input) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO37
     * Bute3A Signal: FLIP_DETECT
     * Bute4A Signal: FLIP_DETECT
     * Functional Mode: GPIO MCU Port Pin 24 (input) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO37, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};

/**
 * EDIO External Interrupt IOMUX settings
 */
struct iomux_initialization edio_iomux_settings[] __initdata = {
    /*
     * Pin Name: PM_INT
     * Bute3A Signal: PM_INT
     * Bute4A Signal: PM_INT
     * Mux2 Mode: ED_INT1 -- ipp_ind_extintr[1]/ipp_obe_extintr[1]
     * Out of Reset Setting: 0x00 (OUTPUTCONFIG_GPIO, INPUTCONFIG_NONE)
     */
    { PIN_PM_INT, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2 },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};

/**
 * Bluetooth IOMUX settings
 */
struct iomux_initialization bluetooth_iomux_settings[] __initdata = {
    /*
     * Pin Name: GPIO21
     * Bute3A Signal: BT_HOST_WAKE_B
     * Bute4A Signal: BT_HOST_WAKE_B
     * Functional Mode: GPIO Shared Port Pin 1 (input) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO21, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO26
     * Bute3A Signal: BT_WAKE_B
     * Bute4A Signal: BT_WAKE_B
     * Functional Mode: GPIO Shared Port Pin 6 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO26, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO27
     * Bute3A Signal: BT_RESET_B
     * Bute4A Signal: BT_RESET_B
     * Functional Mode: GPIO Shared Port Pin 7 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO27, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * Pin Name: GPIO13
     * ButeP4 (close) Signal: NC
     * ButeP4 (wing)  Signal: NC
     * ButeP5 (close) Signal: BT_RESET_B
     * Functional Mode: GPIO MCU Port Pin 13 (output) (present on LVLT)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_GPIO13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};


/**
 * SDHC IOMUX settings
 */
struct iomux_initialization sdhc_iomux_settings[] __initdata = {
    /*
     * Pin: MMC1_CLK
     * Bute3A Signal Name: MMC1_CLK
     * Bute4A Signal Name: SD1_CLK
     * Functional Mode: ipp_do_sdhc1_mmc_clk (output)
     * Out of Reset Setting:
     */
    { PIN_MMC1_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC1_CMD
     * Bute3A Signal Name: MMC1_CMD
     * Bute4A Signal Name: SD1_CMD
     * Functional Mode: ipp_ind_sdhc1_cmd/ipp_do_sdhc1_cmd
     * Out of Reset Setting:
     */
    { PIN_MMC1_CMD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC1_DATA_0
     * Bute3A Signal Name: MMC1_DATA[0]
     * Bute4A Signal Name: SD1_DATA[0]
     * Functional Mode: ipp_ind_sdhc1_data0/ipp_do_sdhc1_data0
     * Out of Reset Setting:
     */
    { PIN_MMC1_DATA_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC1_DATA_1
     * Bute3A Signal Name: MMC1_DATA[1]
     * Bute4A Signal Name: SD1_DATA[1]
     * Functional Mode: ipp_ind_sdhc1_data1/ipp_do_sdhc1_data1
     * Out of Reset Setting:
     */
    { PIN_MMC1_DATA_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC1_DATA_2
     * Bute3A Signal Name: MMC1_DATA[2]
     * Bute4A Signal Name: SD1_DATA[2]
     * Functional Mode: ipp_ind_sdhc1_data2/ipp_do_sdhc1_data2
     * Out of Reset Setting:
     */
    { PIN_MMC1_DATA_2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC1_DATA_3
     * Bute3A Signal Name: MMC1_DATA[3]
     * Bute4A Signal Name: SD1_DATA[3]
     * Functional Mode: ipp_ind_sdhc1_data3/ipp_do_sdhc1_data3
     * Out of Reset Setting:
     */
    { PIN_MMC1_DATA_3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC2_CLK
     * Bute3A Signal Name: MMC2_CLK
     * Bute4A Signal Name: (no connection)
     * Functional Mode: ipp_do_sdhc2_mmc_clk (output)
     * Out of Reset Setting:
     */
    { PIN_MMC2_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC2_CMD
     * Bute3A Signal Name: MMC2_CMD
     * Bute4A Signal Name: (no connection)
     * Functional Mode: ipp_ind_sdhc2_cmd/ipp_do_sdhc2_cmd
     * Out of Reset Setting:
     */
    { PIN_MMC2_CMD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC2_DATA_0
     * Bute3A Signal Name: MMC2_DATA[0]
     * Bute4A Signal Name: (no connection)
     * Functional Mode: ipp_ind_sdhc2_data0/ipp_do_sdhc2_data0
     * Out of Reset Setting:
     */
    { PIN_MMC2_DATA_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC2_DATA_1
     * Bute3A Signal Name: MMC2_DATA[1]
     * Bute4A Signal Name: (no connection)
     * Functional Mode: ipp_ind_sdhc2_data1/ipp_do_sdhc2_data1
     * Out of Reset Setting:
     */
    { PIN_MMC2_DATA_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC2_DATA_2
     * Bute3A Signal Name: MMC2_DATA[2]
     * Bute4A Signal Name: (no connection)
     * Functional Mode: ipp_ind_sdhc2_data2/ipp_do_sdhc2_data2
     * Out of Reset Setting:
     */
    { PIN_MMC2_DATA_2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin: MMC2_DATA_3
     * Bute3A Signal Name: MMC2_DATA[3]
     * Bute4A Signal Name: (no connection)
     * Functional Mode: ipp_ind_sdhc2_data3/ipp_do_sdhc2_data3
     * Out of Reset Setting:
     */
    { PIN_MMC2_DATA_3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};


/**
 * UART IOMUX settings
 */
struct iomux_initialization uart_iomux_settings[] __initdata = {
    /* 
     * Pin Name: UART_TXD1
     * Bute3A Signal: BT_TX
     * Bute4A Signal: BT_TX
     * Functional Mode: ipp_uart1_txd (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_TXD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin Name: UART_RXD1
     * Bute3A Signal: BT_RX
     * Bute4A Signal: BT_RX
     * Functional Mode: ipp_uart1_rxd (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_RXD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin Name: UART_RTS1_B
     * Bute3A Signal: BT_RTS_B
     * Bute4A Signal: BT_RTS_B
     * Functional Mode: ipp_uart1_rts_b (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_RTS1_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_CTS1_B
     * Bute3A Signal: BT_CTS_B
     * Bute4A Signal: BT_CTS_B
     * Functional Mode: ipp_uart1_cts_b (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_CTS1_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: USB_VMOUT
     * Bute3A Signal: USB_VMOUT
     * Bute4A Signal: USB_VMOUT
     * Mux2 Mode: UART2_TXD, ipp_uart2_txd (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_USB_VMOUT, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2 },

    /*
     * Pin name: USB_VPOUT
     * Bute3A Signal: USB_VPOUT
     * Bute4A Signal: USB_VPOUT
     * Mux2 Mode: UART2_RXD, ipp_uart2_rxd (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_USB_VPOUT, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2 }, 

    /*
     * Pin name: USB_XRXD
     * Bute3A Signal: USB_XRXD
     * Bute4A Signal: USB_XRXD
     * Mux1 Mode: UART_RTS2_B, ipp_uart2_rts_b (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_USB_XRXD, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1 },

    /* 
     * Pin name: UART_CTS2_B
     * Bute3A Signal: CTS2_B
     * Bute4A Signal: CTS2_B
     * Functional Mode: ipp_uart2_cts_b (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_CTS2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /* 
     * Pin name: UART_DSR2_B
     * Bute3A Signal: DSR2_B
     * Bute4A Signal: DAI_STDA
     * Functional Mode: ipp_uart2_dsr_dce_o_b (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_DSR2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_DTR2_B
     * Bute3A Signal: DTR2_B
     * Bute3A Signal: DAI_SRDA
     * Functional Mode: ipp_uart2_dtr_dce_i_b (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_DTR2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_RI2_B
     * Bute3A Signal: RI2_B
     * Bute4A Signal: DAI_SCKA
     * Functional Mode: ipp_uart2_ri_dce_o_b (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_RI2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_DCD2_B
     * Bute3A Signal: DCD2_B
     * Bute4A Signal: DAI_FS
     * Functional Mode: ipp_uart2_dcd_dce_o_b (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_DCD2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_TXD3
     * Bute3A Signal: GPS_TXD
     * Bute4A Signal: TXD3
     * Functional Mode: ipp_uart3_txd (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_TXD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_RXD3
     * Bute3A Signal: GPS_RXD
     * Bute4A Signal: RXD3
     * Functional Mode: ipp_uart3_rxd (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_RXD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_RTS3_B
     * Bute3A Signal: GPS_RTS_B
     * Bute4A Signal: RTS3_B
     * Functional Mode: ipp_uart3_rts_b (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_RTS3_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin name: UART_CTS3_B
     * Bute3A Signal: GPS_CTS_B
     * Bute4A Signal: CTS3_B
     * Functional Mode: ipp_uart3_cts_b (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_UART_CTS3_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },


    /*
     * Pin Name: IRDA_TX4
     * Bute3A Signal: IRDA_TX
     * Bute4A Signal: IRDA_TX
     * Functional Mode: ipp_uart4_txd_ir (output)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, 
     *      INPUTCONFIG_FUNC)
     */
    { PIN_IRDA_TX4, OUTPUTCONFIG_FUNC, INPUTCONFIG_NONE },

    /*
     * Pin Name: IRDA_RX4
     * Bute3A Signal: IRDA_RX
     * Bute4A Signal: IRDA_RX
     * Functional Mode: ipp_uart4_rxd_ir (input)
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC,
     *      INPUTCONFIG_FUNC)
     */
    { PIN_IRDA_RX4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};


/**
 * Keypad IOMUX settings
 */
struct iomux_initialization keypad_iomux_settings[] __initdata = {
    { PIN_KEY_COL0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_COL1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_COL2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_COL3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_COL4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_COL5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_COL6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_COL7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    { PIN_KEY_ROW7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};


/**
 * CSPI IOMUX settings
 */
struct iomux_initialization cspi_iomux_settings[] __initdata = {
    /*
     * Pin Name: CSPI2_CS_0 (CSPI2_CS0)
     * Bute3A Signal: PM_SPI_CS
     * Bute4A Signal: PM_SPI_CS
     * Functional mode: SPI2_SS0
     *   ipp_ind_cspi2_ss0_b in; ipp_do_cspi2_ss0_b out
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_CSPI2_CS_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin Name: CSPI2_DI
     * Bute3A Signal: BB_SPI_MISO
     * Bute4A Signal: BB_SPI_MISO
     * Functional mode: SPI2_MISO
     *   ipp_cspi2_ind_miso in; ipp_cspi2_do_miso out
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_CSPI2_DI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin Name: CSPI2_DO
     * Bute3A Signal: BB_SPI_MOSI
     * Bute4A Signal: BB_SPI_MOSI
     * Functional mode: SPI2_MOSI
     *   ipp_cspi2_ind_mosi in; ipp_cspi2_do_mosi out
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_CSPI2_DO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin Name: CSPI2_CS_1 (CSPI2_CS1)
     * Bute3A Signal: (testpoint 1106)
     * Bute4A Signal: (no connection)
     * Functional mode: SPI2_SS1
     *   ipp_ind_cspi2_ss1_b in; ipp_do_cspi2_ss1_b out
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_CSPI2_CS_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin Name: CSPI2_CK
     * Bute3A Signal: BB_SPI_CLK
     * Bute4A Signal: BB_SPI_CLK
     * Functional Mode: SPI2_CLK
     *   ipp_cspi2_clk_in in; ipp_cspi2_clk_out out 
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_CSPI2_CK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};


/**
 * I2C IOMUX settings
 */
struct iomux_initialization i2c_iomux_settings[] __initdata = {
    /*
     * Pin Name: I2C_CLK
     * Bute3A Signal: I2C_CLK
     * Bute4A Signal: I2C_CLK
     * Functional Mode: ipp_i2c_scl_in/ipp_i2c_scl_out
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_I2C_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },

    /*
     * Pin Name: I2C_DAT
     * Bute3A Signal: I2C_DATA / I2C_DAT
     * Bute4A Signal: I2C_DAT
     * Functional Mode: ipp_i2c_sda_in/ipp_i2c_sda_out
     * Out of Reset Setting: 0x12 (OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC)
     */
    { PIN_I2C_DAT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};


/**
 * Global IOMUX settings.
 */
struct iomux_initialization *initial_iomux_settings[] __initdata = {
    csi_iomux_settings,
    ipu_iomux_settings,
    gpio_iomux_settings,
    edio_iomux_settings,
    bluetooth_iomux_settings,
    sdhc_iomux_settings,
    uart_iomux_settings,
    keypad_iomux_settings,
    cspi_iomux_settings,
    i2c_iomux_settings,
    /* End of List */
    NULL
};
