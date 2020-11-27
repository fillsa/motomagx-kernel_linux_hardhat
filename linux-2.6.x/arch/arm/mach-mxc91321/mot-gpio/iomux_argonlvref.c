/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/iomux_argonlvref.c
 *
 * IOMUX mux control register settings for LJ7.1 Reference Platform using
 * IPU display.
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
 * 17-Nov-2006  Motorola        Adjusted iomux settings after ICD review.
 * 07-Dec-2006  Motorola        Changed IOMUX setting for SDMA_EVNT1 pin.
 */

#include "mot-gpio-argonlv.h"

/**
 * IOMUX settings that have not yet been properly categorized by
 * function or peripheral.
 */
struct iomux_initialization uncategorized_iomux_settings[] __initdata = {
    /*
     * ArgonLV Pin: MMC2_DATA_2
     * LJ7.1 Reference Design Signal: WLAN or DVBH DATA2
     * Selected Primary Function: MMC2_DATA_2 (Input/Output)
     */
    { PIN_MMC2_DATA_2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC2_DATA_3
     * LJ7.1 Reference Design Signal: WLAN or DVBH DATA3
     * Selected Primary Function: NC (Input/Output)
     */
    { PIN_MMC2_DATA_3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC2_CLK
     * LJ7.1 Reference Design Signal: WLAN or DVBH CLK
     * Selected Primary Function: MMC2_CLK (Input/Output)
     */
    { PIN_MMC2_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: TRACE0
     * LJ7.1 Reference Design Signal: (not defined)
     * Selected Primary Function: (unknown) (Not Specified)
     */
    { PIN_TRACE0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO38
     * LJ7.1 Reference Design Signal: TP119
     * Selected Primary Function: Not Used (Not Specified)
     */
    { PIN_GPIO38, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC2_CMD
     * LJ7.1 Reference Design Signal: WLAN or DVBH CMD
     * Selected Primary Function: MMC2_CMD (Output)
     */
    { PIN_MMC2_CMD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC2_DATA_0
     * LJ7.1 Reference Design Signal: WLAN or DVBH DATA0
     * Selected Primary Function: MMC2_DATA_0 (Input/Output)
     */
    { PIN_MMC2_DATA_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC2_DATA_1
     * LJ7.1 Reference Design Signal: WLAN or DVBH DATA1
     * Selected Primary Function: MMC2_DATA_1 (Input/Output)
     */
    { PIN_MMC2_DATA_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC1_DATA_1
     * LJ7.1 Reference Design Signal: SD1_DATA_1
     * Selected Primary Function: MMC1_DATA_1 (Input/Output)
     */
    { PIN_MMC1_DATA_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC1_DATA_2
     * LJ7.1 Reference Design Signal: SD1_DATA_2
     * Selected Primary Function: MMC1_DATA_2 (Input/Output)
     */
    { PIN_MMC1_DATA_2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC1_DATA_3
     * LJ7.1 Reference Design Signal: SD1_DATA_3
     * Selected Primary Function: MMC1_DATA_3 (Input/Output)
     */
    { PIN_MMC1_DATA_3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC1_CL K
     * LJ7.1 Reference Design Signal: SD1_CLK Mega SIM
     * Selected Primary Function: MMC1_CLK (Input/Output)
     */
    { PIN_MMC1_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL6
     * LJ7.1 Reference Design Signal: KPP_COL6
     * Selected Primary Function: KEY_COL6 (Input)
     */
    { PIN_KEY_COL6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL7
     * LJ7.1 Reference Design Signal: KPP_COL7
     * Selected Primary Function: KEY_COL7 (Input)
     */
    { PIN_KEY_COL7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC1_CMD
     * LJ7.1 Reference Design Signal: SD1_CMD 
     * Selected Primary Function: MMC1_CMD (Output)
     */
    { PIN_MMC1_CMD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MMC1_DATA_0
     * LJ7.1 Reference Design Signal: SD1_DATA_0 
     * Selected Primary Function: MMC1_DATA_0 (Input/Output)
     */
    { PIN_MMC1_DATA_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL2
     * LJ7.1 Reference Design Signal: KPP_COL2
     * Selected Primary Function: KEY_COL2 (Input)
     */
    { PIN_KEY_COL2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL3
     * LJ7.1 Reference Design Signal: KPP_COL3
     * Selected Primary Function: KEY_COL3 (Input)
     */
    { PIN_KEY_COL3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL4
     * LJ7.1 Reference Design Signal: KPP_COL4
     * Selected Primary Function: KEY_COL4 (Input)
     */
    { PIN_KEY_COL4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL5
     * LJ7.1 Reference Design Signal: KPP_COL5
     * Selected Primary Function: KEY_COL5 (Input)
     */
    { PIN_KEY_COL5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW6
     * LJ7.1 Reference Design Signal: KPP_ROW6
     * Selected Primary Function: KEY_ROW6 (Input)
     */
    { PIN_KEY_ROW6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW7
     * LJ7.1 Reference Design Signal: KPP_ROW7
     * Selected Primary Function: KEY_ROW7 (Input)
     */
    { PIN_KEY_ROW7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL0
     * LJ7.1 Reference Design Signal: KPP_COL0
     * Selected Primary Function: KEY_COL0 (Input)
     */
    { PIN_KEY_COL0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_COL1
     * LJ7.1 Reference Design Signal: KPP_COL1
     * Selected Primary Function: KEY_COL1 (Input)
     */
    { PIN_KEY_COL1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW2
     * LJ7.1 Reference Design Signal: KPP_ROW2
     * Selected Primary Function: KEY_ROW2 (Input)
     */
    { PIN_KEY_ROW2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW3
     * LJ7.1 Reference Design Signal: KPP_ROW3
     * Selected Primary Function: KEY_ROW3 (Input)
     */
    { PIN_KEY_ROW3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW4
     * LJ7.1 Reference Design Signal: KPP_ROW4
     * Selected Primary Function: KEY_ROW4 (Input)
     */
    { PIN_KEY_ROW4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW5
     * LJ7.1 Reference Design Signal: KPP_ROW5
     * Selected Primary Function: KEY_ROW5 (Input)
     */
    { PIN_KEY_ROW5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: I2C_CLK
     * LJ7.1 Reference Design Signal: I2C_CLK
     * Selected Primary Function: I2C_CLK (Output)
     */
    { PIN_I2C_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: I2C_DAT
     * LJ7.1 Reference Design Signal: I2C_DAT
     * Selected Primary Function: I2C_DAT (Input/Output)
     */
    { PIN_I2C_DAT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW0
     * LJ7.1 Reference Design Signal: KPP_ROW0
     * Selected Primary Function: KEY_ROW0 (Input)
     */
    { PIN_KEY_ROW0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: KEY_ROW1
     * LJ7.1 Reference Design Signal: KPP_ROW1
     * Selected Primary Function: KEY_ROW1 (Input)
     */
    { PIN_KEY_ROW1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_7
     * LJ7.1 Reference Design Signal: CAM_D7*
     * Selected Primary Function: IPU_CSI_DA TA_7 (Input)
     */
    { PIN_IPU_CSI_DATA_7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_8
     * LJ7.1 Reference Design Signal: CAM_D8*
     * Selected Primary Function: IPU_CSI_DA TA_8 (Input)
     */
    { PIN_IPU_CSI_DATA_8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_9
     * LJ7.1 Reference Design Signal: CAM_D9*
     * Selected Primary Function: IPU_CSI_DA TA_9 (Input)
     */
    { PIN_IPU_CSI_DATA_9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_PIXCLK
     * LJ7.1 Reference Design Signal: CAM_PIXCLK*
     * Selected Primary Function: IPU_CSI_PI XCLK (Input)
     */
    { PIN_IPU_CSI_PIXCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_3
     * LJ7.1 Reference Design Signal: CAM_D3*
     * Selected Primary Function: IPU_CSI_DA TA_3 (Input)
     */
    { PIN_IPU_CSI_DATA_3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_4
     * LJ7.1 Reference Design Signal: CAM_D4*
     * Selected Primary Function: IPU_CSI_DA TA_4 (Input)
     */
    { PIN_IPU_CSI_DATA_4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_5
     * LJ7.1 Reference Design Signal: CAM_D5*
     * Selected Primary Function: IPU_CSI_DA TA_5 (Input)
     */
    { PIN_IPU_CSI_DATA_5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_6
     * LJ7.1 Reference Design Signal: CAM_D6*
     * Selected Primary Function: IPU_CSI_DA TA_6 (Input)
     */
    { PIN_IPU_CSI_DATA_6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_VSYNC
     * LJ7.1 Reference Design Signal: CAM_VS*
     * Selected Primary Function: IPU_CSI_VS YNC (Input)
     */
    { PIN_IPU_CSI_VSYNC, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_0
     * LJ7.1 Reference Design Signal: IPU_CLI_BKLT_EN_B
     * Selected Primary Function: GPIO_SHARED_4 (Output)
     *
     * NOTE: Pin list says Func mode; hardware team indicates it
     * should really be GPIO mode.
     */
    { PIN_IPU_CSI_DATA_0, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_1
     * LJ7.1 Reference Design Signal: CAM_Flash_T_F
     * Selected Primary Function: GPIO_SHARED_5 (Output)
     *
     * NOTE: ICD says Func mode; hardware team has revised this to GPIO.
     */
    { PIN_IPU_CSI_DATA_1, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: IPU_CSI_DATA_2
     * LJ7.1 Reference Design Signal: CAM_D2*
     * Selected Primary Function: IPU_CSI_DA TA_2 (Input)
     */
    { PIN_IPU_CSI_DATA_2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_D3_SPL
     * LJ7.1 Reference Design Signal: IPU_D3_SPL***
     * Selected Primary Function: IPU_D3_SPL (Output)
     */
    { PIN_IPU_D3_SPL, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_D3_REV
     * LJ7.1 Reference Design Signal: IPU_D3_REV***
     * Selected Primary Function: IPU_D3_REV (Output)
     */
    { PIN_IPU_D3_REV, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_MCLK
     * LJ7.1 Reference Design Signal: CAM_MCLK*
     * Selected Primary Function: IPU_CSI_MC LK (Output)
     */
    { PIN_IPU_CSI_MCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CSI_HSYNC
     * LJ7.1 Reference Design Signal: CAM_HS*
     * Selected Primary Function: IPU_CSI_HS YNC (Input)
     */
    { PIN_IPU_CSI_HSYNC, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_PAR_RS
     * LJ7.1 Reference Design Signal: CLKSEL
     * Selected Primary Function: IPU_PAR_RS (Output)
     */
    { PIN_IPU_PAR_RS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_WRITE
     * LJ7.1 Reference Design Signal: IPU_WR**
     * Selected Primary Function: IPU_WRITE (Output)
     */
    { PIN_IPU_WRITE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_READ
     * LJ7.1 Reference Design Signal: IPU_RD_D3PS**
     * Selected Primary Function: IPU_READ (Output)
     */
    { PIN_IPU_READ, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_D3_CLS
     * LJ7.1 Reference Design Signal: IPU_D3_CLS***
     * Selected Primary Function: IPU_D3_CLS (Output)
     */
    { PIN_IPU_D3_CLS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_SD_D_IO
     * LJ7.1 Reference Design Signal: CLI_SD_D*+
     * Selected Primary Function: IPU_SD_D_I O (Output)
     */
    { PIN_IPU_SD_D_IO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_SD_D_O
     * LJ7.1 Reference Design Signal: IPU_SD_D_O***
     * Selected Primary Function: IPU_SD_D_O (Output)
     */
    { PIN_IPU_SD_D_O, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_SD_D_CLK
     * LJ7.1 Reference Design Signal: CLI_SD_CLK*+
     * Selected Primary Function: IPU_SD_D_C LK (Output)
     */
    { PIN_IPU_SD_D_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_SER_RS
     * LJ7.1 Reference Design Signal: CLI_D_C*+
     * Selected Primary Function: IPU_SER_RS (Output)
     */
    { PIN_IPU_SER_RS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FPSHIFT
     * LJ7.1 Reference Design Signal: LCD_PIXCLK*+
     * Selected Primary Function: IPU_FPSHIF T (Output)
     */
    { PIN_IPU_FPSHIFT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_DRDY0
     * LJ7.1 Reference Design Signal: DISP_OE*+
     * Selected Primary Function: IPU_DRDY0 (Output)
     */
    { PIN_IPU_DRDY0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_CONTRAST1
     * LJ7.1 Reference Design Signal: IPU_CONTRAST1***
     * Selected Primary Function: IPU_CONTR AST1 (Output)
     */
    { PIN_IPU_CONTRAST1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_LCS1
     * LJ7.1 Reference Design Signal: CLI_CS_B*+
     * Selected Primary Function: IPU_LCS1 (Output)
     */
    { PIN_IPU_LCS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_VSYNC0
     * LJ7.1 Reference Design Signal: IPU_D0_VSYNC***
     * Selected Primary Function: IPU_VSYNC 0 (Output)
     */
    { PIN_IPU_VSYNC0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_HSYNC
     * LJ7.1 Reference Design Signal: LCD_HSYNC*+
     * Selected Primary Function: IPU_HSYNC (Output)
     */
    { PIN_IPU_HSYNC, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_VSYNC3
     * LJ7.1 Reference Design Signal: LCD_VSYNC*+
     * Selected Primary Function: IPU_VSYNC 3 (Output)
     */
    { PIN_IPU_VSYNC3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_LCS0
     * LJ7.1 Reference Design Signal: IPU_D0_CS_B**
     * Selected Primary Function: IPU_LCS0 (Output)
     */
    { PIN_IPU_LCS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_14
     * LJ7.1 Reference Design Signal: LCD_D14
     * Selected Primary Function: IPU_FDAT_1 4 (Output)
     */
    { PIN_IPU_FDAT_14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_15
     * LJ7.1 Reference Design Signal: LCD_D15
     * Selected Primary Function: IPU_FDAT_1 5 (Output)
     */
    { PIN_IPU_FDAT_15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_ 16
     * LJ7.1 Reference Design Signal: LCD_D16*
     * Selected Primary Function: IPU_FDAT_1 6 (Output)
     */
    { PIN_IPU_FDAT_16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_17
     * LJ7.1 Reference Design Signal: LCD_D17*
     * Selected Primary Function: IPU_FDAT_1 7 (Output)
     */
    { PIN_IPU_FDAT_17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_10
     * LJ7.1 Reference Design Signal: LCD_D10
     * Selected Primary Function: IPU_FDAT_1 0 (Output)
     */
    { PIN_IPU_FDAT_10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_11
     * LJ7.1 Reference Design Signal: LCD_D11
     * Selected Primary Function: IPU_FDAT_1 1 (Output)
     */
    { PIN_IPU_FDAT_11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_12
     * LJ7.1 Reference Design Signal: LCD_D12
     * Selected Primary Function: IPU_FDAT_1 2 (Output)
     */
    { PIN_IPU_FDAT_12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_13
     * LJ7.1 Reference Design Signal: LCD_D13
     * Selected Primary Function: IPU_FDAT_1 3 (Output)
     */
    { PIN_IPU_FDAT_13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_6
     * LJ7.1 Reference Design Signal: LCD_D6
     * Selected Primary Function: IPU_FDAT_6 (Output)
     */
    { PIN_IPU_FDAT_6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_7
     * LJ7.1 Reference Design Signal: LCD_D7
     * Selected Primary Function: IPU_FDAT_7 (Output)
     */
    { PIN_IPU_FDAT_7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_8
     * LJ7.1 Reference Design Signal: LCD_D8
     * Selected Primary Function: IPU_FDAT_8 (Output)
     */
    { PIN_IPU_FDAT_8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_9
     * LJ7.1 Reference Design Signal: LCD_D9
     * Selected Primary Function: IPU_FDAT_9 (Output)
     */
    { PIN_IPU_FDAT_9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_2
     * LJ7.1 Reference Design Signal: LCD_D2
     * Selected Primary Function: IPU_FDAT_2 (Output)
     */
    { PIN_IPU_FDAT_2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_3
     * LJ7.1 Reference Design Signal: LCD_D3
     * Selected Primary Function: IPU_FDAT_3 (Output)
     */
    { PIN_IPU_FDAT_3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_4
     * LJ7.1 Reference Design Signal: LCD_D4
     * Selected Primary Function: IPU_FDAT_4 (Output)
     */
    { PIN_IPU_FDAT_4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_5
     * LJ7.1 Reference Design Signal: LCD_D5
     * Selected Primary Function: IPU_FDAT_5 (Output)
     */
    { PIN_IPU_FDAT_5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI1_CS_0
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_CSPI1_CS_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI1_CS_1
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_CSPI1_CS_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_0
     * LJ7.1 Reference Design Signal: LCD_D0
     * Selected Primary Function: IPU_FDAT_0 (Output)
     */
    { PIN_IPU_FDAT_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IPU_FDAT_1
     * LJ7.1 Reference Design Signal: LCD_D1
     * Selected Primary Function: IPU_FDAT_1 (Output)
     */
    { PIN_IPU_FDAT_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI2_CS_1
     * LJ7.1 Reference Design Signal: PM_SPI_CS_2**
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_CSPI2_CS_1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI1_CK
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_CSPI1_CK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI1_DO
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_CSPI1_DO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI1_DI
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_CSPI1_DI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI2_CK
     * LJ7.1 Reference Design Signal: BB_SPI_CLK
     * Selected Primary Function: CSPI2_CK (Not Specified)
     */
    { PIN_CSPI2_CK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI2_DO
     * LJ7.1 Reference Design Signal: BB_SPI_MOSI
     * Selected Primary Function: CSPI2_DO (Output)
     */
    { PIN_CSPI2_DO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI2_DI
     * LJ7.1 Reference Design Signal: BB_SPI_MISO
     * Selected Primary Function: CSPI2_DI (Not Specified)
     */
    { PIN_CSPI2_DI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CSPI2_CS_0
     * LJ7.1 Reference Design Signal: PM_SPI_CS
     * Selected Primary Function: CSPI2_CS_0 (Output)
     */
    { PIN_CSPI2_CS_0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM3_R_CLK
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: DAM3_R_CL K (Output)
     */
    { PIN_DAM3_R_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM3_R_FS
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: DAM3_R_FS (Not Specified)
     */
    { PIN_DAM3_R_FS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: BATT_LINE
     * LJ7.1 Reference Design Signal: BATT_DAT
     * Selected Primary Function: BATT_LINE (Not Specified)
     */
    { PIN_BATT_LINE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: PWM
     * LJ7.1 Reference Design Signal: PWM*
     * Selected Primary Function: PWM (Output)
     */
    { PIN_PWM, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM3_TXD
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: DAM3_TXD (Output)
     */
    { PIN_DAM3_TXD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM3_RXD
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: DAM3_RXD (Input)
     */
    { PIN_DAM3_RXD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM3_T_CLK
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: DAM3_T_CL K (Not Specified)
     */
    { PIN_DAM3_T_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM3_T_FS
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: DAM3_T_FS (Not Specified)
     */
    { PIN_DAM3_T_FS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM2_TXD
     * LJ7.1 Reference Design Signal: MMSAP_TX
     * Selected Primary Function: DAM2_TXD (Output)
     */
    { PIN_DAM2_TXD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM2_RXD
     * LJ7.1 Reference Design Signal: MMSAP_RX
     * Selected Primary Function: DAM2_RXD (Input)
     */
    { PIN_DAM2_RXD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM2_T_CLK
     * LJ7.1 Reference Design Signal: MMSAP_CLK
     * Selected Primary Function: DAM2_T_CL K (Not Specified)
     */
    { PIN_DAM2_T_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM2_T_FS
     * LJ7.1 Reference Design Signal: MMSAP_FS
     * Selected Primary Function: DAM2_T_FS (Not Specified)
     */
    { PIN_DAM2_T_FS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM1_TXD
     * LJ7.1 Reference Design Signal: ASAP_TX
     * Selected Primary Function: DAM1_TXD (Input/Output)
     */
    { PIN_DAM1_TXD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM1_RXD
     * LJ7.1 Reference Design Signal: ASAP_RX
     * Selected Primary Function: DAM1_RXD (Input/Output)
     */
    { PIN_DAM1_RXD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM1_T_CLK
     * LJ7.1 Reference Design Signal: ASAP_CLK
     * Selected Primary Function: DAM1_T_CL K (Input/Output)
     */
    { PIN_DAM1_T_CLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DAM1_T_FS 
     * LJ7.1 Reference Design Signal: ASAP_FS
     * Selected Primary Function: DAM1_T_FS (Input/Output)
     */
    { PIN_DAM1_T_FS , OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_DSR2_B
     * LJ7.1 Reference Design Signal: DAI_STDA
     * Selected Primary Function: UART_DSR2 _B (Output)
     */
    { PIN_UART_DSR2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_MUX_CTL
     * LJ7.1 Reference Design Signal: (not defined)
     * Selected Primary Function: (unknown) (Input)
     */
    { PIN_UART_MUXCTL, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IRDA_TX4
     * LJ7.1 Reference Design Signal: IRDA_TX/Future Expansion
     * Selected Primary Function: IRDA_TX4 (Output)
     */
    { PIN_IRDA_TX4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: IRDA_RX4
     * LJ7.1 Reference Design Signal: IRDA_RX/Future Expansion
     * Selected Primary Function: IRDA_RX4 (Input)
     */
    { PIN_IRDA_RX4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_CTS2_B
     * LJ7.1 Reference Design Signal: CTS2_B
     * Selected Primary Function: UART_CTS2 _B (Output)
     */
    { PIN_UART_CTS2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_NONE },
    /*
     * ArgonLV Pin: UART_DTR2_B
     * LJ7.1 Reference Design Signal: DAI_SRDA
     * Selected Primary Function: UART_DTR2 _B (Input)
     */
    { PIN_UART_DTR2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_RI2_B
     * LJ7.1 Reference Design Signal: DAI_SCKA
     * Selected Primary Function: UART_RI2_B (Output)
     */
    { PIN_UART_RI2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_DCD2_B
     * LJ7.1 Reference Design Signal: DAI_FS
     * Selected Primary Function: UART_DCD 2_B (Output)
     */
    { PIN_UART_DCD2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_TXD1
     * LJ7.1 Reference Design Signal: BT_TX
     * Selected Primary Function: UART_TXD1 (Output)
     */
    { PIN_UART_TXD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_RXD1
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: UART_RXD1 (Input)
     */
    { PIN_UART_RXD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_RTS1
     * LJ7.1 Reference Design Signal: BT_RTS_B
     * Selected Primary Function: UART_RTS1 (Input)
     */
    { PIN_UART_RTS1_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_CTS1
     * LJ7.1 Reference Design Signal: BT_CTS_B
     * Selected Primary Function: UART_CTS1 (Output)
     */
    { PIN_UART_CTS1_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO24
     * LJ7.1 Reference Design Signal: WDOG_AP
     * Selected Primary Function: GPIO_MCU19 (Output)
     *
     * NOTE: Pin list says Func mode; hardware team indicates it
     * should really be GPIO mode.
     */
    { PIN_GPIO24, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: GPIO25
     * LJ7.1 Reference Design Signal: KPD_EL_EN1
     * Selected Primary Function: GPIO_MCU20 (Output)
     *
     * NOTE: ICD says Func mode; hardware team has revised this to GPIO.
     */
    { PIN_GPIO25, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: GPIO26
     * LJ7.1 Reference Design Signal: BT_WAKE_B
     * Selected Primary Function: GPIO_SHAR ED6 (Output)
     */
    { PIN_GPIO26, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO27
     * LJ7.1 Reference Design Signal: WLAN_RESET_B
     * Selected Primary Function: GPIO_SHAR ED7 (Output)
     */
    { PIN_GPIO27, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO20
     * LJ7.1 Reference Design Signal: POWER_RDY
     * Selected Primary Function: GPIO_SHAR ED0 (Input)
     */
    { PIN_GPIO20, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO21
     * LJ7.1 Reference Design Signal: BT_HOST_WAK E_B
     * Selected Primary Function: GPIO_SHAR ED1 (Output)
     */
    { PIN_GPIO21, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO22
     * LJ7.1 Reference Design Signal: LOBAT_B
     * Selected Primary Function: GPIO_SHAR ED2 (Input)
     */
    { PIN_GPIO22, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO23
     * LJ7.1 Reference Design Signal: CAM_RST_B
     * Selected Primary Function: GPIO_SHARED_3 (Output)
     *
     * NOTE: Pin list says GPIO mode; hardware team indicates it
     * should really be Func mode.
     */
    { PIN_GPIO23, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO36
     * LJ7.1 Reference Design Signal: LCD_SD
     * Selected Primary Function: GPIO_MCU23 (Output)
     */
    { PIN_GPIO36, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO37
     * LJ7.1 Reference Design Signal: FLIP_DETECT
     * Selected Primary Function: GPIO_MCU2 4 (Not Specified)
     */
    { PIN_GPIO37, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO18
     * LJ7.1 Reference Design Signal: AOC_PWR_UP_ DN
     * Selected Primary Function: GPIO_DSP0 (Input/Output)
     */
    { PIN_GPIO18, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO19
     * LJ7.1 Reference Design Signal: USB_HS_WAKE_UP
     * Selected Primary Function: GPIO_MCU14 (Output)
     *
     * NOTE: Pin list says Func mode; hardware team indicates it
     * should really be GPIO mode.
     */
    { PIN_GPIO19, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: GPIO14
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: GPIO_MCU14 (Output)
     */
    { PIN_GPIO14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO15
     * LJ7.1 Reference Design Signal: GPIO_CAM_EX T_PWRDN
     * Selected Primary Function: GPIO_MCU1 5 (Output)
     */
    { PIN_GPIO15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO16
     * LJ7.1 Reference Design Signal: GPIO_CAM_IN T_PWRDN
     * Selected Primary Function: GPIO_MCU1 6 (Output)
     */
    { PIN_GPIO16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO17
     * LJ7.1 Reference Design Signal: S1_DET_B
     * Selected Primary Function: GPIO_MCU1 7 (Input/Output)
     */
    { PIN_GPIO17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO10
     * LJ7.1 Reference Design Signal: UART_DETECT
     * Selected Primary Function: Not used (Input)
     */
    { PIN_GPIO10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO11
     * LJ7.1 Reference Design Signal: VEFUSE_EN 
     * Selected Primary Function: GPIO_MCU1 1 (Output)
     */
    { PIN_GPIO11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO12
     * LJ7.1 Reference Design Signal: MORPHING_PWM
     * Selected Primary Function: GPIO_MCU1 2 (Output)
     */
    { PIN_GPIO12, OUTPUTCONFIG_ALT3, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO13
     * LJ7.1 Reference Design Signal: BT_RESET
     * Selected Primary Function: GPIO_MCU13 (Output)
     */
    { PIN_GPIO13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO6
     * LJ7.1 Reference Design Signal: MEGA_SIM_VOLTAGE_EN
     * Selected Primary Function: GPIO_MCU6  (Output)
     */
    { PIN_GPIO6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO7
     * LJ7.1 Reference Design Signal: Morphing_Rest
     * Selected Primary Function: GPIO_MCU7 (Output)
     */
    { PIN_GPIO7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO8
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: GPIO_MCU8 (Output)
     */
    { PIN_GPIO8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO9
     * LJ7.1 Reference Design Signal: POWER_DVS0
     * Selected Primary Function: POWER_DVS0 (Output)
     *
     * NOTE: ICD says Alt5 mode; hardware team has revised this to Alt6.
     */
    { PIN_GPIO9, OUTPUTCONFIG_ALT6, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO2
     * LJ7.1 Reference Design Signal: WLAN_HOST_WAKE_B
     * Selected Primary Function: GPIO_MCU2 (Input)
     */
    { PIN_GPIO2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO3
     * LJ7.1 Reference Design Signal: FM_RADIO_INT
     * Selected Primary Function: GPIO_MCU3 (Input)
     */
    { PIN_GPIO3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO4
     * LJ7.1 Reference Design Signal: Morphing_KCHG
     * Selected Primary Function: GPIO_MCU4 (Input)
     *
     * NOTE: Pin list says GPIO mode; hardware team indicates it
     * should really be Func mode.
     */
    { PIN_GPIO4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO5
     * LJ7.1 Reference Design Signal: Morphine_RCHG
     * Selected Primary Function: GPIO_MCU5 (Output)
     */
    { PIN_GPIO5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_RTS3_B
     * LJ7.1 Reference Design Signal: RTS_B
     * Selected Primary Function: UART_RTS3 _B (Input)
     */
    { PIN_UART_RTS3_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_CTS3_B
     * LJ7.1 Reference Design Signal: CTS_B
     * Selected Primary Function: UART_CTS3 _B (Output)
     */
    { PIN_UART_CTS3_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO0
     * LJ7.1 Reference Design Signal: GPIO_CAM_FL ASH_EN
     * Selected Primary Function: GPIO_SHAR ED26 (Output)
     */
    { PIN_GPIO0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO1
     * LJ7.1 Reference Design Signal: USB_XCVR_EN
     * Selected Primary Function: GPIO_SHAR ED27 (Output)
     */
    { PIN_GPIO1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI2_CS0
     * LJ7.1 Reference Design Signal: SPIMB_CS
     * Selected Primary Function: MQSPI2_CS0 (Output)
     */
    { PIN_MQSPI2_CS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI2_CS1
     * LJ7.1 Reference Design Signal: MQSPI2_CS1
     * Selected Primary Function: MQSPI2_CS1 (Output)
     */
    { PIN_MQSPI2_CS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_TXD3
     * LJ7.1 Reference Design Signal: TXD3
     * Selected Primary Function: UART_TXD3 (Output)
     */
    { PIN_UART_TXD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: UART_RXD3
     * LJ7.1 Reference Design Signal: RXD3
     * Selected Primary Function: UART_RXD 3 (Input)
     */
    { PIN_UART_RXD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI1_CS2
     * LJ7.1 Reference Design Signal: MQSPI1_CS2
     * Selected Primary Function: MQSPI1_CS2 (Output)
     */
    { PIN_MQSPI1_CS2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI2_CK
     * LJ7.1 Reference Design Signal: SPIMB_CLK
     * Selected Primary Function: MQSPI2_CK (Output)
     */
    { PIN_MQSPI2_CK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI2_DI
     * LJ7.1 Reference Design Signal: SPIMB_DR
     * Selected Primary Function: MQSPI2_DI (Input)
     */
    { PIN_MQSPI2_DI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI2_DO
     * LJ7.1 Reference Design Signal: SPIMB_DW
     * Selected Primary Function: MQSPI2_DO (Output)
     */
    { PIN_MQSPI2_DO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI1_DI
     * LJ7.1 Reference Design Signal: SPIWB_DR
     * Selected Primary Function: MQSPI1_DI (Input)
     */
    { PIN_MQSPI1_DI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI1_DO
     * LJ7.1 Reference Design Signal: SPIWB_DW
     * Selected Primary Function: MQSPI1_DO (Output)
     */
    { PIN_MQSPI1_DO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI1_CS0
     * LJ7.1 Reference Design Signal: SPIWB_CS
     * Selected Primary Function: MQSPI1_CS0 (Output)
     */
    { PIN_MQSPI1_CS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI1_CS1
     * LJ7.1 Reference Design Signal: MQSPI1_CS1
     * Selected Primary Function: MQSPI1_CS 1 (Output)
     */
    { PIN_MQSPI1_CS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T2OUT1
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: (unknown) (Output)
     */
    { PIN_L1T2OUT1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T2OUT2
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: (unknown) (Output)
     */
    { PIN_L1T2OUT2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T2OUT3
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: L1T2OUT3 (Output)
     */
    { PIN_L1T2OUT3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MQSPI1_CK
     * LJ7.1 Reference Design Signal: SPIWB_CLK
     * Selected Primary Function: MQSPI1_CK (Output)
     */
    { PIN_MQSPI1_CK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T1OUT3
     * LJ7.1 Reference Design Signal: TX_SLOT_WB
     * Selected Primary Function: (unknown) (Output)
     */
    { PIN_L1T1OUT3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T1OUT7
     * LJ7.1 Reference Design Signal: HOST_GPS_SYN
     * Selected Primary Function: L1T1OUT10 (Output)
     */
    { PIN_L1T1OUT7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T1OUT8
     * LJ7.1 Reference Design Signal: L1T1OUT8
     * Selected Primary Function: L1T1OUT8 (Output)
     */
    { PIN_L1T1OUT8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T2OUT0
     * LJ7.1 Reference Design Signal: HOST_GPS_SYN
     * Selected Primary Function: L1T2OUT0 (Output)
     */
    { PIN_L1T2OUT0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DSM2_VCO_EN
     * LJ7.1 Reference Design Signal: DSM2_VCO_EN
     * Selected Primary Function: DSM2_VCO_ EN (Output)
     */
    { PIN_DSM2_VCO_EN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T1OUT0
     * LJ7.1 Reference Design Signal: RX_SLOT_WB
     * Selected Primary Function: L1T1OUT0 (Output)
     */
    { PIN_L1T1OUT0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T1OUT1
     * LJ7.1 Reference Design Signal: L1T1OUT1
     * Selected Primary Function: L1T1OUT1 (Output)
     */
    { PIN_L1T1OUT1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: L1T1OUT2
     * LJ7.1 Reference Design Signal: TX_WARMUP_ WB
     * Selected Primary Function: L1T1OUT2 (Output)
     */
    { PIN_L1T1OUT2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_DIV_RX_FRAME
     * LJ7.1 Reference Design Signal: WAMMO_DIV_ RX_FRAME
     * Selected Primary Function: WAMMO_DI V_RX_FRA ME (Input)
     */
    { PIN_WAMMO_DIV_RX_FRAME, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DSM1_STBY
     * LJ7.1 Reference Design Signal: STBY_WB
     * Selected Primary Function: DSM1_STBY (Output)
     */
    { PIN_DSM1_STBY, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DSM1_RX_ON
     * LJ7.1 Reference Design Signal: RX_WARMUP_ WB
     * Selected Primary Function: DSM1_RX_O N (Output)
     */
    { PIN_DSM1_RX_ON, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DSM2_STBY
     * LJ7.1 Reference Design Signal: STBY_MB
     * Selected Primary Function: DSM2_STBY (Output)
     */
    { PIN_DSM2_STBY, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_TX_FRAME
     * LJ7.1 Reference Design Signal: TX_FRAME_W B
     * Selected Primary Function: WAMMO_TX _FRAME (Output)
     */
    { PIN_WAMMO_TX_FRAME, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_RX_FRAME
     * LJ7.1 Reference Design Signal: RX_FRAME_W B
     * Selected Primary Function: WAMMO_R X_FRAME (Input)
     */
    { PIN_WAMMO_RX_FRAME, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_DIV_RX_I_CH
     * LJ7.1 Reference Design Signal: RX_I_WB
     * Selected Primary Function: WAMMO_DIV_RX_I_CH (Input)
     */
    { PIN_WAMMO_DIV_RX_I_CH, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_DIV_RX_Q _CH
     * LJ7.1 Reference Design Signal: WAMMO_DIV_ RX_Q_CH
     * Selected Primary Function: WAMMO_DI V_RX_Q_CH (Input)
     */
    { PIN_WAMMO_DIV_RX_Q_CH, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_TX_Q_CH
     * LJ7.1 Reference Design Signal: TX_Q_WB
     * Selected Primary Function: WAMMO_TX _Q_CH (Output)
     */
    { PIN_WAMMO_TX_Q_CH, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_TX_I_CH
     * LJ7.1 Reference Design Signal: TX_I_WB
     * Selected Primary Function: WAMMO_TX _I_CH (Output)
     */
    { PIN_WAMMO_TX_I_CH, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_RX_Q_CH
     * LJ7.1 Reference Design Signal: RX_Q_WB
     * Selected Primary Function: WAMMO_R X_Q_CH (Input)
     */
    { PIN_WAMMO_RX_Q_CH, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WAMMO_RX_I_CH
     * LJ7.1 Reference Design Signal: RX_I_WB
     * Selected Primary Function: WAMMO_R X_I_CH (Input)
     */
    { PIN_WAMMO_RX_I_CH, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO34
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_GPIO34, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO35
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_GPIO35, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDMA_EVNT0
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: (unknown) (Output)
     */
    { PIN_SDMA_EVNT0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDMA_EVNT1
     * LJ7.1 Reference Design Signal: USB_HS_INT
     * Selected Primary Function: GPIO_MCU12 (Not Specified)
     *
     * NOTE: Pin list says Func mode; hardware team indicates it
     * should really be Alt2 mode.
     */
    { PIN_SDMA_EVNT1, OUTPUTCONFIG_ALT2, INPUTCONFIG_ALT2 },
    /*
     * ArgonLV Pin: GPIO30
     * LJ7.1 Reference Design Signal: FE3
     * Selected Primary Function: GPIO_TFMC_D2 (Output)
     *
     * NOTE: ICD says Alt5; hardware team revised to Alt6.
     */
    { PIN_GPIO30, OUTPUTCONFIG_ALT6, INPUTCONFIG_NONE },
    /*
     * ArgonLV Pin: GPIO31
     * LJ7.1 Reference Design Signal: WLAN_PWR_DOWN_B
     * Selected Primary Function: GPIO_MCU2 1 (Output)
     */
    { PIN_GPIO31, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO32
     * LJ7.1 Reference Design Signal: WLAN_CLIENT_WAKE_B
     * Selected Primary Function: GPIO_MCU22 (Output)
     */
    { PIN_GPIO32, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: GPIO33
     * LJ7.1 Reference Design Signal: NC
     * Selected Primary Function: Not Used (Output)
     */
    { PIN_GPIO33, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: BBP_RX_DATA
     * LJ7.1 Reference Design Signal: SSI_RX_DATA
     * Selected Primary Function: BBP_RX_DA TA (Input/Output)
     */
    { PIN_BBP_RX_DATA, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: BBP_TX_DATA
     * LJ7.1 Reference Design Signal: GPS_HOST_SYNC
     * Selected Primary Function: GPIO_DSP16 (Input)
     */
    { PIN_BBP_TX_DATA, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: GPIO28
     * LJ7.1 Reference Design Signal: FE1
     * Selected Primary Function: TFMC_D0 (Output)
     *
     * NOTE: ICD says Alt5; hardware team revised to Alt6.
     */
    { PIN_GPIO28, OUTPUTCONFIG_ALT6, INPUTCONFIG_NONE },
    /*
     * ArgonLV Pin: GPIO29
     * LJ7.1 Reference Design Signal: FE2
     * Selected Primary Function: TFMC_D1 (Output)
     *
     * NOTE: ICD says Alt5; hardware team revised to Alt6.
     */
    { PIN_GPIO29, OUTPUTCONFIG_ALT6, INPUTCONFIG_NONE },
    /*
     * ArgonLV Pin: BBP_RX_CLK
     * LJ7.1 Reference Design Signal: SSI_RX_CLK
     * Selected Primary Function: BBP_RX_CL K (Input/Output)
     */
    { PIN_BBP_RX_CLK, OUTPUTCONFIG_ALT1, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: BBP_TX_CLK
     * LJ7.1 Reference Design Signal: SSI_RX_DATA
     * Selected Primary Function: TXRXDATA (Input/Output)
     */
    { PIN_BBP_TX_CLK, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1 },
    /*
     * ArgonLV Pin: BBP_RX_FRAME
     * LJ7.1 Reference Design Signal: SSI_TXRX_FRAME
     * Selected Primary Function: TXRXEN (Output)
     */
    { PIN_BBP_RX_FRAME, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1 },
    /*
     * ArgonLV Pin: BBP_TX_FRAME
     * LJ7.1 Reference Design Signal: SSI_TXRX_CLk
     * Selected Primary Function: SYSCLK (Input)
     */
    { PIN_BBP_TX_FRAME, OUTPUTCONFIG_ALT1, INPUTCONFIG_ALT1 },
    /*
     * ArgonLV Pin: USB_VPIN
     * LJ7.1 Reference Design Signal: USB_VPIN
     * Selected Primary Function: USB_VPIN (Input)
     *
     * NOTE: Hardware team revised to out:Func, in:GPIO
     */
    { PIN_USB_VPIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: USB_VPOUT
     * LJ7.1 Reference Design Signal: USB_VPOUT
     * Selected Primary Function: UART2_RXD (Output)
     */
    { PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: USB_VMIN
     * LJ7.1 Reference Design Signal: USB_VMIN
     * Selected Primary Function: USB_VMIN (Input)
     *
     * NOTE: Hardware team revised to out:Func, in:GPIO
     */
    { PIN_USB_VMIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: USB_XRXD
     * LJ7.1 Reference Design Signal: USB_XRXD
     * Selected Primary Function: UART_RTS2 _B (Input)
     *
     * NOTE: ICD says Alt1; hardware team revised to Func.
     */
    { PIN_USB_XRXD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: USB1_VMIN
     * LJ7.1 Reference Design Signal: USB_HS_DMA_REQ
     * Selected Primary Function: GPIO_MCU0 (Input)
     */
    { PIN_USB1_VMIN, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: USB1_XRXD
     * LJ7.1 Reference Design Signal: USB_HS_RESET
     * Selected Primary Function: GPIO_MCU1 (Output)
     */
    { PIN_USB1_XRXD, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: USB_VMOUNT
     * LJ7.1 Reference Design Signal: USB_VMOUT
     * Selected Primary Function: UART2_TXD (Output)
     * 
     * NOTE: ICD says Alt2|None; hardware team email says Func|Func.
     */
    { PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: USB_TXENB
     * LJ7.1 Reference Design Signal: USB_TXEN_B
     * Selected Primary Function: USB_TXEN B (Output)
     */
    { PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: USB1_VMOUT
     * LJ7.1 Reference Design Signal: SERDES_RST_B
     * Selected Primary Function: GPIO_MCU2 8 (Output)
     */
    { PIN_USB1_VMOUT, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: USB1_TXENB
     * LJ7.1 Reference Design Signal: STBY_B
     * Selected Primary Function: GPIO_MCU2 9 (Output)
     */
    { PIN_USB1_TXENB, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: USB1_VPIN
     * LJ7.1 Reference Design Signal: LCD_Backlight
     * Selected Primary Function: GPIO_MCU3 0 (Output)
     */
    { PIN_USB1_VPIN, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: USB1_VPOUT
     * LJ7.1 Reference Design Signal: GPIO_CLI_RST_B
     * Selected Primary Function: GPIO_MCU3 0 (Output)
     */
    { PIN_USB1_VPOUT, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: SIM1_RST0
     * LJ7.1 Reference Design Signal: USIM_RST
     * Selected Primary Function: SIM1_RST0 (Output)
     */
    { PIN_SIM1_RST0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SIM1_DATA0_RX_TX
     * LJ7.1 Reference Design Signal: USIM_IO
     * Selected Primary Function: SIM1_DATA0 _RX_TX (Input/Output)
     */
    { PIN_SIM1_DATA0_RX_TX, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SIM1_SEVN0
     * LJ7.1 Reference Design Signal: VSIM_EN
     * Selected Primary Function: SIM1_SEVN0 (Output)
     */
    { PIN_SIM1_SVEN0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SIM1_SIMPD0
     * LJ7.1 Reference Design Signal: USIM_PD_B
     * Selected Primary Function: (unknown) (Input)
     */
    { PIN_SIM1_SIMPD0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SJC_MOD
     * LJ7.1 Reference Design Signal: SJC_MOD
     * Selected Primary Function: SJC_MOD (Input)
     */
    { PIN_SJC_MOD, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DE_B
     * LJ7.1 Reference Design Signal: DE_B
     * Selected Primary Function: DE_B (Input/Output)
     */
    { PIN_DE_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: EVTI_B
     * LJ7.1 Reference Design Signal: EVTI_B
     * Selected Primary Function: EVTI_B (Input)
     */
    { PIN_EVTI_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SIM1_CLK0
     * LJ7.1 Reference Design Signal: USIM_CLK
     * Selected Primary Function: SIM1_CLK0 (Output)
     */
    { PIN_SIM1_CLK0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: TDI
     * LJ7.1 Reference Design Signal: TDI
     * Selected Primary Function: TDI (Input)
     */
    { PIN_TDI, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: RTCK
     * LJ7.1 Reference Design Signal: RTCK
     * Selected Primary Function: RTCK (Output)
     */
    { PIN_RTCK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: TCK
     * LJ7.1 Reference Design Signal: TCK
     * Selected Primary Function: TCK (Input)
     */
    { PIN_TCK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: TMS
     * LJ7.1 Reference Design Signal: TMS
     * Selected Primary Function: TMS (Input)
     */
    { PIN_TMS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: USER_OFF
     * LJ7.1 Reference Design Signal: GPS_LDO_CONTROL
     * Selected Primary Function: GPIO_SHAR ED22 (Output)
     */
    { PIN_USER_OFF, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: WDOG_B
     * LJ7.1 Reference Design Signal: WATCHDOG_B
     * Selected Primary Function: GPIO_SHARED23 (Output)
     *
     * NOTE: ICD says Func; hardware team revised to GPIO.
     */
    { PIN_WDOG_B, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: TRSTB
     * LJ7.1 Reference Design Signal: TRST_B
     * Selected Primary Function: TRSTB (Input)
     */
    { PIN_TRSTB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: TDO
     * LJ7.1 Reference Design Signal: TDO
     * Selected Primary Function: TDO (Output)
     */
    { PIN_TDO, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: BOOT_MODE1
     * LJ7.1 Reference Design Signal: GPS_RESET
     * Selected Primary Function: GPIO_SHARED_21 (Input)
     */
    { PIN_BOOT_MODE1, OUTPUTCONFIG_GPIO, INPUTCONFIG_GPIO },
    /*
     * ArgonLV Pin: REF_EN_B
     * LJ7.1 Reference Design Signal: ARGON_REF_C LK_EN_B
     * Selected Primary Function: REF_EN_B (Output)
     */
    { PIN_REF_EN_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: PM_INT
     * LJ7.1 Reference Design Signal: PM_INT
     * Selected Primary Function: ED_INT6 (Input)
     *
     * The power_ic driver sets this to 0x38.
     */
    { PIN_PM_INT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: POWER_FAIL
     * LJ7.1 Reference Design Signal: Future_expansion
     * Selected Primary Function: EINT7 (Input)
     */
    { PIN_POWER_FAIL, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: RESET_IN_B
     * LJ7.1 Reference Design Signal: RESET_IN_B
     * Selected Primary Function: RESET_IN_B (Input)
     */
    { PIN_RESET_IN_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: RESET_POR_IN_B
     * LJ7.1 Reference Design Signal: PROC_RESET_ B
     * Selected Primary Function: RESET_POR _IN_B (Input)
     */
    { PIN_RESET_POR_IN_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: TEST_MODE
     * LJ7.1 Reference Design Signal: (not defined)
     * Selected Primary Function: TEST_MODE (Input)
     */
    { PIN_TEST_MODE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: BOOT_MODE0
     * LJ7.1 Reference Design Signal: (not defined)
     * Selected Primary Function: BOOT_MOD E0 (Input)
     */
    { PIN_BOOT_MODE0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MB_REF_CLK_IN
     * LJ7.1 Reference Design Signal: MB_CKIH_CLK
     * Selected Primary Function: MB_REF_CL K_IN (Input)
     */
    { PIN_MB_REF_CLK_IN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CKIH_REF_CLK_IN
     * LJ7.1 Reference Design Signal: ARGON_REF_C LK
     * Selected Primary Function: CKIH_REF_ CLK_IN (Input)
     */
    { PIN_CKIH_REF_CLK_IN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CKO1
     * LJ7.1 Reference Design Signal: BB_SYS_CLK
     * Selected Primary Function: CKO1 (Output)
     */
    { PIN_CKO1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CKO2
     * LJ7.1 Reference Design Signal: (not defined)
     * Selected Primary Function: CKO2 (Output)
     */
    { PIN_CKO2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D14
     * LJ7.1 Reference Design Signal: D14
     * Selected Primary Function: D14 (Input/Output)
     */
    { PIN_D14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D15
     * LJ7.1 Reference Design Signal: D15
     * Selected Primary Function: D15 (Input/Output)
     */
    { PIN_D15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CKIL
     * LJ7.1 Reference Design Signal: PROC_32K_CL K
     * Selected Primary Function: CKIL (Input)
     */
    { PIN_CKIL, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WB_REF_CLK_IN
     * LJ7.1 Reference Design Signal: WB_CKIH_CLK
     * Selected Primary Function: WB_REF_CL K_IN (Input)
     */
    { PIN_WB_REF_CLK_IN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D10
     * LJ7.1 Reference Design Signal: D10
     * Selected Primary Function: D10 (Input/Output)
     */
    { PIN_D10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D11
     * LJ7.1 Reference Design Signal: D11
     * Selected Primary Function: D11 (Input/Output)
     */
    { PIN_D11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D12
     * LJ7.1 Reference Design Signal: D12
     * Selected Primary Function: D12 (Input/Output)
     */
    { PIN_D12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D13
     * LJ7.1 Reference Design Signal: D13
     * Selected Primary Function: D13 (Input/Output)
     */
    { PIN_D13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D6
     * LJ7.1 Reference Design Signal: D6
     * Selected Primary Function: D6 (Input/Output)
     */
    { PIN_D6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D7
     * LJ7.1 Reference Design Signal: D7
     * Selected Primary Function: D7 (Input/Output)
     */
    { PIN_D7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D8
     * LJ7.1 Reference Design Signal: D8
     * Selected Primary Function: D8 (Input/Output)
     */
    { PIN_D8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D9
     * LJ7.1 Reference Design Signal: D9
     * Selected Primary Function: D9 (Input/Output)
     */
    { PIN_D9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D2
     * LJ7.1 Reference Design Signal: D2
     * Selected Primary Function: D2 (Input/Output)
     */
    { PIN_D2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D3
     * LJ7.1 Reference Design Signal: D3
     * Selected Primary Function: D3 (Input/Output)
     */
    { PIN_D3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D4
     * LJ7.1 Reference Design Signal: D4
     * Selected Primary Function: D4 (Input/Output)
     */
    { PIN_D4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D5
     * LJ7.1 Reference Design Signal: D5
     * Selected Primary Function: D5 (Input/Output)
     */
    { PIN_D5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WE
     * LJ7.1 Reference Design Signal: WE
     * Selected Primary Function: WE (Output)
     */
    { PIN_WE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: R_B
     * LJ7.1 Reference Design Signal: (not defined)
     * Selected Primary Function: R_B (Not Specified)
     */
    { PIN_R_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D0
     * LJ7.1 Reference Design Signal: D0
     * Selected Primary Function: D0 (Input/Output)
     */
    { PIN_D0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: D1
     * LJ7.1 Reference Design Signal: D1
     * Selected Primary Function: D1 (Input/Output)
     */
    { PIN_D1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CLE
     * LJ7.1 Reference Design Signal: CLE
     * Selected Primary Function: CLE (Output)
     */
    { PIN_CLE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: WP
     * LJ7.1 Reference Design Signal: WP_B
     * Selected Primary Function: WP (Output)
     */
    { PIN_WP, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: ALE
     * LJ7.1 Reference Design Signal: ALE
     * Selected Primary Function: ALE (Output)
     */
    { PIN_ALE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: RE
     * LJ7.1 Reference Design Signal: RE
     * Selected Primary Function: RE (Output)
     */
    { PIN_RE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDCKE0
     * LJ7.1 Reference Design Signal: SDCKE0
     * Selected Primary Function: SDCKE0 (Output)
     */
    { PIN_SDCKE0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDCKE1
     * LJ7.1 Reference Design Signal: SDCKE1
     * Selected Primary Function: SDCKE1 (Output)
     */
    { PIN_SDCKE1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDCLK
     * LJ7.1 Reference Design Signal: SDCLK
     * Selected Primary Function: SDCLK (Output)
     */
    { PIN_SDCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDCLK_B
     * LJ7.1 Reference Design Signal: SDCLK_B
     * Selected Primary Function: SDCLK_B (Output)
     */
    { PIN_SDCLK_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: EB3_B
     * LJ7.1 Reference Design Signal: EB3_B
     * Selected Primary Function: EB3_B (Output)
     */
    { PIN_EB3_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: RAS
     * LJ7.1 Reference Design Signal: RAS_B
     * Selected Primary Function: RAS (Output)
     */
    { PIN_RAS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CAS
     * LJ7.1 Reference Design Signal: CAS_B
     * Selected Primary Function: CAS (Output)
     */
    { PIN_CAS, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDWE
     * LJ7.1 Reference Design Signal: SDWE_B
     * Selected Primary Function: SDWE (Output)
     */
    { PIN_SDWE, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: LBA_B
     * LJ7.1 Reference Design Signal: LBA_B
     * Selected Primary Function: LBA_B (Output)
     */
    { PIN_LBA_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: BCLK
     * LJ7.1 Reference Design Signal: BCLK
     * Selected Primary Function: BCLK (Output)
     */
    { PIN_BCLK, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: RW_B
     * LJ7.1 Reference Design Signal: RW_B
     * Selected Primary Function: RW_B (Output)
     */
    { PIN_RW_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: EB0_B
     * LJ7.1 Reference Design Signal: EB0_B
     * Selected Primary Function: EB0_B (Output)
     */
    { PIN_EB0_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CS4_B
     * LJ7.1 Reference Design Signal: CS4_B
     * Selected Primary Function: CS4_B (Output)
     */
    { PIN_CS4_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDBA0
     * LJ7.1 Reference Design Signal: SDBA0
     * Selected Primary Function: SDBA0 (Output)
     */
    { PIN_SDBA0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDBA1
     * LJ7.1 Reference Design Signal: SDBA1
     * Selected Primary Function: SDBA1 (Output)
     */
    { PIN_SDBA1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: ECB_B
     * LJ7.1 Reference Design Signal: ECB_B
     * Selected Primary Function: ECB_B (Input)
     */
    { PIN_ECB_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CS0_B
     * LJ7.1 Reference Design Signal: CS0_B
     * Selected Primary Function: CS0_B (Output)
     */
    { PIN_CS0_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CS1_B
     * LJ7.1 Reference Design Signal: CS1_B
     * Selected Primary Function: CS1_B (Output)
     */
    { PIN_CS1_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CS2_B
     * LJ7.1 Reference Design Signal: CS2_B
     * Selected Primary Function: CS2_B (Output)
     */
    { PIN_CS2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: CS3_B
     * LJ7.1 Reference Design Signal: CS3_B
     * Selected Primary Function: CS3_B (Output)
     */
    { PIN_CS3_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DQM1
     * LJ7.1 Reference Design Signal: DQM1
     * Selected Primary Function: DQM1 (Output)
     */
    { PIN_DQM1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DQM2
     * LJ7.1 Reference Design Signal: DQM2
     * Selected Primary Function: DQM2 (Output)
     */
    { PIN_DQM2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DQM3
     * LJ7.1 Reference Design Signal: DQM3
     * Selected Primary Function: DQM3 (Output)
     */
    { PIN_DQM3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: OE_B
     * LJ7.1 Reference Design Signal: OE_B
     * Selected Primary Function: OE_B (Input/Output)
     */
    { PIN_OE_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDQS1
     * LJ7.1 Reference Design Signal: SDQS1
     * Selected Primary Function: SDQS1 (Input/Output)
     */
    { PIN_SDQS1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: EB1_B
     * LJ7.1 Reference Design Signal: TP1001
     * Selected Primary Function: Not ised (Not Specified)
     */
    { PIN_EB1_b, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: EB2_B
     * LJ7.1 Reference Design Signal: TP1000
     * Selected Primary Function: Not used (Not Specified)
     */
    { PIN_EB2_B, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: DQM0
     * LJ7.1 Reference Design Signal: DQM0
     * Selected Primary Function: DQM0 (Output)
     */
    { PIN_DQM0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD29
     * LJ7.1 Reference Design Signal: SD29
     * Selected Primary Function: SD29 (Input/Output)
     */
    { PIN_SD29, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD30
     * LJ7.1 Reference Design Signal: SD30
     * Selected Primary Function: SD30 (Input/Output)
     */
    { PIN_SD30, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD31
     * LJ7.1 Reference Design Signal: SD31
     * Selected Primary Function: SD31 (Input/Output)
     */
    { PIN_SD31, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SDQS0
     * LJ7.1 Reference Design Signal: SDQS0
     * Selected Primary Function: SDQS0 (Input/Output)
     */
    { PIN_SDQS0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD25
     * LJ7.1 Reference Design Signal: SD25
     * Selected Primary Function: SD25 (Input/Output)
     */
    { PIN_SD25, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD26
     * LJ7.1 Reference Design Signal: SD26
     * Selected Primary Function: SD26 (Input/Output)
     */
    { PIN_SD26, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD27
     * LJ7.1 Reference Design Signal: SD27
     * Selected Primary Function: SD27 (Input/Output)
     */
    { PIN_SD27, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD28
     * LJ7.1 Reference Design Signal: SD28
     * Selected Primary Function: SD28 (Input/Output)
     */
    { PIN_SD28, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD21
     * LJ7.1 Reference Design Signal: SD21
     * Selected Primary Function: SD21 (Input/Output)
     */
    { PIN_SD21, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD22
     * LJ7.1 Reference Design Signal: SD22SD26
     * Selected Primary Function: SD22 (Input/Output)
     */
    { PIN_SD22, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD23
     * LJ7.1 Reference Design Signal: SD23
     * Selected Primary Function: SD23 (Input/Output)
     */
    { PIN_SD23, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD24
     * LJ7.1 Reference Design Signal: SD24
     * Selected Primary Function: SD24 (Input/Output)
     */
    { PIN_SD24, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD17
     * LJ7.1 Reference Design Signal: SD17
     * Selected Primary Function: SD17 (Input/Output)
     */
    { PIN_SD17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD18
     * LJ7.1 Reference Design Signal: SD18
     * Selected Primary Function: SD18 (Input/Output)
     */
    { PIN_SD18, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD19
     * LJ7.1 Reference Design Signal: SD19
     * Selected Primary Function: SD19 (Input/Output)
     */
    { PIN_SD19, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD20
     * LJ7.1 Reference Design Signal: SD20
     * Selected Primary Function: SD20 (Input/Output)
     */
    { PIN_SD20, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD13
     * LJ7.1 Reference Design Signal: SD13
     * Selected Primary Function: SD13 (Input/Output)
     */
    { PIN_SD13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD14
     * LJ7.1 Reference Design Signal: SD14
     * Selected Primary Function: SD14 (Input/Output)
     */
    { PIN_SD14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD15
     * LJ7.1 Reference Design Signal: SD15
     * Selected Primary Function: SD15 (Input/Output)
     */
    { PIN_SD15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD16
     * LJ7.1 Reference Design Signal: SD16
     * Selected Primary Function: SD16 (Input/Output)
     */
    { PIN_SD16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD9
     * LJ7.1 Reference Design Signal: SD9
     * Selected Primary Function: SD9 (Input/Output)
     */
    { PIN_SD9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD10
     * LJ7.1 Reference Design Signal: SD10
     * Selected Primary Function: SD10 (Input/Output)
     */
    { PIN_SD10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD11
     * LJ7.1 Reference Design Signal: SD11
     * Selected Primary Function: SD11 (Input/Output)
     */
    { PIN_SD11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD12
     * LJ7.1 Reference Design Signal: SD12
     * Selected Primary Function: SD12 (Input/Output)
     */
    { PIN_SD12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD5
     * LJ7.1 Reference Design Signal: SD5
     * Selected Primary Function: SD5 (Input/Output)
     */
    { PIN_SD5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD6
     * LJ7.1 Reference Design Signal: SD6
     * Selected Primary Function: SD6 (Input/Output)
     */
    { PIN_SD6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD7
     * LJ7.1 Reference Design Signal: SD7
     * Selected Primary Function: SD7 (Input/Output)
     */
    { PIN_SD7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD8
     * LJ7.1 Reference Design Signal: SD8
     * Selected Primary Function: SD8 (Input/Output)
     */
    { PIN_SD8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD1
     * LJ7.1 Reference Design Signal: SD1
     * Selected Primary Function: SD1 (Input/Output)
     */
    { PIN_SD1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD2
     * LJ7.1 Reference Design Signal: SD2
     * Selected Primary Function: SD2 (Input/Output)
     */
    { PIN_SD2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD3
     * LJ7.1 Reference Design Signal: SD3
     * Selected Primary Function: SD3 (Input/Output)
     */
    { PIN_SD3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD4
     * LJ7.1 Reference Design Signal: SD4
     * Selected Primary Function: SD4 (Input/Output)
     */
    { PIN_SD4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A23
     * LJ7.1 Reference Design Signal: ADDR23
     * Selected Primary Function: A23 (Output)
     */
    { PIN_A23, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A24
     * LJ7.1 Reference Design Signal: ADDR24
     * Selected Primary Function: A24 (Output)
     */
    { PIN_A24, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A25
     * LJ7.1 Reference Design Signal: ADDR25
     * Selected Primary Function: A25 (Output)
     */
    { PIN_A25, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: SD0
     * LJ7.1 Reference Design Signal: SD0
     * Selected Primary Function: SD0 (Input/Output)
     */
    { PIN_SD0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A19
     * LJ7.1 Reference Design Signal: ADDR19
     * Selected Primary Function: A19 (Output)
     */
    { PIN_A19, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A20
     * LJ7.1 Reference Design Signal: ADDR20
     * Selected Primary Function: A20 (Output)
     */
    { PIN_A20, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A21
     * LJ7.1 Reference Design Signal: ADDR21
     * Selected Primary Function: A21 (Output)
     */
    { PIN_A21, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A22
     * LJ7.1 Reference Design Signal: ADDR22
     * Selected Primary Function: A22 (Output)
     */
    { PIN_A22, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A15
     * LJ7.1 Reference Design Signal: ADDR15
     * Selected Primary Function: A15 (Output)
     */
    { PIN_A15, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A16
     * LJ7.1 Reference Design Signal: ADDR16
     * Selected Primary Function: A16 (Output)
     */
    { PIN_A16, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A17
     * LJ7.1 Reference Design Signal: ADDR17
     * Selected Primary Function: A17 (Output)
     */
    { PIN_A17, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A18
     * LJ7.1 Reference Design Signal: ADDR18
     * Selected Primary Function: A18 (Output)
     */
    { PIN_A18, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A11_MA11
     * LJ7.1 Reference Design Signal: ADDR11
     * Selected Primary Function: A11_MA11 (Output)
     */
    { PIN_A11_MA11, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A12_MA12
     * LJ7.1 Reference Design Signal: ADDR12
     * Selected Primary Function: A12_MA12 (Output)
     */
    { PIN_A12_MA12, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A13_MA13
     * LJ7.1 Reference Design Signal: ADDR13
     * Selected Primary Function: A13_MA13 (Output)
     */
    { PIN_A13_MA13, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A14
     * LJ7.1 Reference Design Signal: ADDR14
     * Selected Primary Function: A14 (Output)
     */
    { PIN_A14, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A8_M8
     * LJ7.1 Reference Design Signal: ADDR8
     * Selected Primary Function: A8_M8 (Output)
     */
    { PIN_A8_MA8, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A9_MA9
     * LJ7.1 Reference Design Signal: ADDR9
     * Selected Primary Function: A9_MA9 (Output)
     */
    { PIN_A9_MA9, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A10
     * LJ7.1 Reference Design Signal: ADDR10
     * Selected Primary Function: A10 (Output)
     */
    { PIN_A10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: MA10
     * LJ7.1 Reference Design Signal: MA10
     * Selected Primary Function: MA10 (Output)
     */
    { PIN_MA10, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A4_MA4
     * LJ7.1 Reference Design Signal: ADDR4
     * Selected Primary Function: A4_MA4 (Output)
     */
    { PIN_A4_MA4, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A5_MA5
     * LJ7.1 Reference Design Signal: ADDR5
     * Selected Primary Function: A5_MA5 (Output)
     */
    { PIN_A5_MA5, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A6_MA6
     * LJ7.1 Reference Design Signal: ADDR6
     * Selected Primary Function: A6_MA6 (Output)
     */
    { PIN_A6_MA6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A7_MA7
     * LJ7.1 Reference Design Signal: ADDR7
     * Selected Primary Function: A7_MA7 (Output)
     */
    { PIN_A7_MA7, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A0_MA0
     * LJ7.1 Reference Design Signal: ADDR0
     * Selected Primary Function: A0_MA0 (Output)
     */
    { PIN_A0_MA0, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A1_MA1
     * LJ7.1 Reference Design Signal: ADDR1
     * Selected Primary Function: A1_MA1 (Output)
     */
    { PIN_A1_MA1, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A2_MA2
     * LJ7.1 Reference Design Signal: ADDR2
     * Selected Primary Function: A2_MA2 (Output)
     */
    { PIN_A2_MA2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /*
     * ArgonLV Pin: A3_MA3
     * LJ7.1 Reference Design Signal: ADDR3
     * Selected Primary Function: A3_MA3 (Output)
     */
    { PIN_A3_MA3, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC },
    /* List Terminator */
    { IOMUX_INVALID_PIN, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC }
};

/**
 * Global IOMUX settings.
 */
struct iomux_initialization *initial_iomux_settings[] __initdata = {
    uncategorized_iomux_settings,
    /* End of List */
    NULL
};
