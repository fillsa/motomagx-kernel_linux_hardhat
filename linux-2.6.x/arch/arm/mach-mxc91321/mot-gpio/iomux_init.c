/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/iomux_init.c
 *
 * Configure IOMUX mux control registers.
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


#if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
/**
 * Number of entries in the hwcfg_pins array.
 */
#define HWCFG_IOMUX_PIN_COUNT   438


/**
 * Used to map hardware configuration tree IOMUX pin identifiers to the
 * identifier use by Freescale.
 */
static enum iomux_pins __initdata hwcfg_pins[HWCFG_IOMUX_PIN_COUNT] = {
    PIN_MSE0,                    /* 0x0000 */
    PIN_PAD1,                    /* 0x0001 */
    PIN_PAD2,                    /* 0x0002 */
    PIN_PAD3,                    /* 0x0003 */
    PIN_MSE1,                    /* 0x0004 */
    PIN_EVTO,                    /* 0x0005 */
    PIN_MRDY,                    /* 0x0006 */
    PIN_MCKO,                    /* 0x0007 */
    PIN_MDO28,                   /* 0x0008 */
    PIN_MDO29,                   /* 0x0009 */
    PIN_MDO30,                   /* 0x000A */
    PIN_MDO31,                   /* 0x000B */
    PIN_MDO24,                   /* 0x000C */
    PIN_MDO25,                   /* 0x000D */
    PIN_MDO26,                   /* 0x000E */
    PIN_MDO27,                   /* 0x000F */
    PIN_MDO20,                   /* 0x0010 */
    PIN_MDO21,                   /* 0x0011 */
    PIN_MDO22,                   /* 0x0012 */
    PIN_MDO23,                   /* 0x0013 */
    PIN_MDO16,                   /* 0x0014 */
    PIN_MDO17,                   /* 0x0015 */
    PIN_MDO18,                   /* 0x0016 */
    PIN_MDO19,                   /* 0x0017 */
    PIN_MDO12,                   /* 0x0018 */
    PIN_MDO13,                   /* 0x0019 */
    PIN_MDO14,                   /* 0x001A */
    PIN_MDO15,                   /* 0x001B */
    PIN_MDO8,                    /* 0x001C */
    PIN_MDO9,                    /* 0x001D */
    PIN_MDO10,                   /* 0x001E */
    PIN_MDO11,                   /* 0x001F */
    PIN_MDO4,                    /* 0x0020 */
    PIN_MDO5,                    /* 0x0021 */
    PIN_MDO6,                    /* 0x0022 */
    PIN_MDO7,                    /* 0x0023 */
    PIN_MDO0,                    /* 0x0024 */
    PIN_MDO1,                    /* 0x0025 */
    PIN_MDO2,                    /* 0x0026 */
    PIN_MDO3,                    /* 0x0027 */
    PIN_TRCLK,                   /* 0x0028 */
    PIN_SRST,                    /* 0x0029 */
    PIN_DBGRQ,                   /* 0x002A */
    PIN_EMD_SDCLK,               /* 0x002B */
    PIN_EXTRIG,                  /* 0x002C */
    PIN_TRACE29,                 /* 0x002D */
    PIN_TRACE30,                 /* 0x002E */
    PIN_TRACE31,                 /* 0x002F */
    PIN_DBGACK,                  /* 0x0030 */
    PIN_TRACE25,                 /* 0x0031 */
    PIN_TRACE26,                 /* 0x0032 */
    PIN_TRACE27,                 /* 0x0033 */
    PIN_TRACE28,                 /* 0x0034 */
    PIN_TRACE21,                 /* 0x0035 */
    PIN_TRACE22,                 /* 0x0036 */
    PIN_TRACE23,                 /* 0x0037 */
    PIN_TRACE24,                 /* 0x0038 */
    PIN_TRACE17,                 /* 0x0039 */
    PIN_TRACE18,                 /* 0x003A */
    PIN_TRACE19,                 /* 0x003B */
    PIN_TRACE20,                 /* 0x003C */
    PIN_TRACE13,                 /* 0x003D */
    PIN_TRACE14,                 /* 0x003E */
    PIN_TRACE15,                 /* 0x003F */
    PIN_TRACE16,                 /* 0x0040 */
    PIN_TRACE9,                  /* 0x0041 */
    PIN_TRACE10,                 /* 0x0042 */
    PIN_TRACE11,                 /* 0x0043 */
    PIN_TRACE12,                 /* 0x0044 */
    PIN_TRACE5,                  /* 0x0045 */
    PIN_TRACE6,                  /* 0x0046 */
    PIN_TRACE7,                  /* 0x0047 */
    PIN_TRACE8,                  /* 0x0048 */
    PIN_TRACE1,                  /* 0x0049 */
    PIN_TRACE2,                  /* 0x004A */
    PIN_TRACE3,                  /* 0x004B */
    PIN_TRACE4,                  /* 0x004C */
    PIN_MMC2_DATA_2,             /* 0x004D */
    PIN_MMC2_DATA_3,             /* 0x004E */
    PIN_MMC2_CLK,                /* 0x004F */
    PIN_TRACE0,                  /* 0x0050 */
    PIN_MMC1_SDDET,              /* 0x0051 */
    PIN_GPIO38,                  /* 0x0052 */
    PIN_MMC2_CMD,                /* 0x0053 */
    PIN_MMC2_DATA_0,             /* 0x0054 */
    PIN_MMC2_DATA_1,             /* 0x0055 */
    PIN_MMC1_DATA_1,             /* 0x0056 */
    PIN_MMC1_DATA_2,             /* 0x0057 */
    PIN_MMC1_DATA_3,             /* 0x0058 */
    PIN_MMC1_CLK,                /* 0x0059 */
    PIN_KEY_COL6,                /* 0x005A */
    PIN_KEY_COL7,                /* 0x005B */
    PIN_MMC1_CMD,                /* 0x005C */
    PIN_MMC1_DATA_0,             /* 0x005D */
    PIN_KEY_COL2,                /* 0x005E */
    PIN_KEY_COL3,                /* 0x005F */
    PIN_KEY_COL4,                /* 0x0060 */
    PIN_KEY_COL5,                /* 0x0061 */
    PIN_KEY_ROW6,                /* 0x0062 */
    PIN_KEY_ROW7,                /* 0x0063 */
    PIN_KEY_COL0,                /* 0x0064 */
    PIN_KEY_COL1,                /* 0x0065 */
    PIN_KEY_ROW2,                /* 0x0066 */
    PIN_KEY_ROW3,                /* 0x0067 */
    PIN_KEY_ROW4,                /* 0x0068 */
    PIN_KEY_ROW5,                /* 0x0069 */
    PIN_I2C_CLK,                 /* 0x006A */
    PIN_I2C_DAT,                 /* 0x006B */
    PIN_KEY_ROW0,                /* 0x006C */
    PIN_KEY_ROW1,                /* 0x006D */
    PIN_IPU_CSI_DATA_7,          /* 0x006E */
    PIN_IPU_CSI_DATA_8,          /* 0x006F */
    PIN_IPU_CSI_DATA_9,          /* 0x0070 */
    PIN_IPU_CSI_PIXCLK,          /* 0x0071 */
    PIN_IPU_CSI_DATA_3,          /* 0x0072 */
    PIN_IPU_CSI_DATA_4,          /* 0x0073 */
    PIN_IPU_CSI_DATA_5,          /* 0x0074 */
    PIN_IPU_CSI_DATA_6,          /* 0x0075 */
    PIN_IPU_CSI_VSYNC,           /* 0x0076 */
    PIN_IPU_CSI_DATA_0,          /* 0x0077 */
    PIN_IPU_CSI_DATA_1,          /* 0x0078 */
    PIN_IPU_CSI_DATA_2,          /* 0x0079 */
    PIN_IPU_D3_SPL,              /* 0x007A */
    PIN_IPU_D3_REV,              /* 0x007B */
    PIN_IPU_CSI_MCLK,            /* 0x007C */
    PIN_IPU_CSI_HSYNC,           /* 0x007D */
    PIN_IPU_PAR_RS,              /* 0x007E */
    PIN_IPU_WRITE,               /* 0x007F */
    PIN_IPU_READ,                /* 0x0080 */
    PIN_IPU_D3_CLS,              /* 0x0081 */
    PIN_IPU_SD_D_IO,             /* 0x0082 */
    PIN_IPU_SD_D_O,              /* 0x0083 */
    PIN_IPU_SD_D_CLK,            /* 0x0084 */
    PIN_IPU_SER_RS,              /* 0x0085 */
    PIN_IPU_FPSHIFT,             /* 0x0086 */
    PIN_IPU_DRDY0,               /* 0x0087 */
    PIN_IPU_CONTRAST1,           /* 0x0088 */
    PIN_IPU_LCS1,                /* 0x0089 */
    PIN_IPU_VSYNC0,              /* 0x008A */
    PIN_IPU_HSYNC,               /* 0x008B */
    PIN_IPU_VSYNC3,              /* 0x008C */
    PIN_IPU_LCS0,                /* 0x008D */
    PIN_IPU_FDAT_14,             /* 0x008E */
    PIN_IPU_FDAT_15,             /* 0x008F */
    PIN_IPU_FDAT_16,             /* 0x0090 */
    PIN_IPU_FDAT_17,             /* 0x0091 */
    PIN_IPU_FDAT_10,             /* 0x0092 */
    PIN_IPU_FDAT_11,             /* 0x0093 */
    PIN_IPU_FDAT_12,             /* 0x0094 */
    PIN_IPU_FDAT_13,             /* 0x0095 */
    PIN_IPU_FDAT_6,              /* 0x0096 */
    PIN_IPU_FDAT_7,              /* 0x0097 */
    PIN_IPU_FDAT_8,              /* 0x0098 */
    PIN_IPU_FDAT_9,              /* 0x0099 */
    PIN_IPU_FDAT_2,              /* 0x009A */
    PIN_IPU_FDAT_3,              /* 0x009B */
    PIN_IPU_FDAT_4,              /* 0x009C */
    PIN_IPU_FDAT_5,              /* 0x009D */
    PIN_CSPI1_CS_0,              /* 0x009E */
    PIN_CSPI1_CS_1,              /* 0x009F */
    PIN_IPU_FDAT_0,              /* 0x00A0 */
    PIN_IPU_FDAT_1,              /* 0x00A1 */
    PIN_CSPI2_CS_1,              /* 0x00A2 */
    PIN_CSPI1_CK,                /* 0x00A3 */
    PIN_CSPI1_DO,                /* 0x00A4 */
    PIN_CSPI1_DI,                /* 0x00A5 */
    PIN_CSPI2_CK,                /* 0x00A6 */
    PIN_CSPI2_DO,                /* 0x00A7 */
    PIN_CSPI2_DI,                /* 0x00A8 */
    PIN_CSPI2_CS_0,              /* 0x00A9 */
    PIN_DAM3_R_CLK,              /* 0x00AA */
    PIN_DAM3_R_FS,               /* 0x00AB */
    PIN_BATT_LINE,               /* 0x00AC */
    PIN_PWM,                     /* 0x00AD */
    PIN_DAM3_TXD,                /* 0x00AE */
    PIN_DAM3_RXD,                /* 0x00AF */
    PIN_DAM3_T_CLK,              /* 0x00B0 */
    PIN_DAM3_T_FS,               /* 0x00B1 */
    PIN_DAM2_TXD,                /* 0x00B2 */
    PIN_DAM2_RXD,                /* 0x00B3 */
    PIN_DAM2_T_CLK,              /* 0x00B4 */
    PIN_DAM2_T_FS,               /* 0x00B5 */
    PIN_DAM1_TXD,                /* 0x00B6 */
    PIN_DAM1_RXD,                /* 0x00B7 */
    PIN_DAM1_T_CLK,              /* 0x00B8 */
    PIN_DAM1_T_FS,               /* 0x00B9 */
    PIN_UART_DSR2_B,             /* 0x00BA */
    PIN_UART_MUXCTL,             /* 0x00BB */
    PIN_IRDA_TX4,                /* 0x00BC */
    PIN_IRDA_RX4,                /* 0x00BD */
    PIN_UART_CTS2_B,             /* 0x00BE */
    PIN_UART_DTR2_B,             /* 0x00BF */
    PIN_UART_RI2_B,              /* 0x00C0 */
    PIN_UART_DCD2_B,             /* 0x00C1 */
    PIN_UART_TXD1,               /* 0x00C2 */
    PIN_UART_RXD1,               /* 0x00C3 */
    PIN_UART_RTS1_B,             /* 0x00C4 */
    PIN_UART_CTS1_B,             /* 0x00C5 */
    PIN_GPIO24,                  /* 0x00C6 */
    PIN_GPIO25,                  /* 0x00C7 */
    PIN_GPIO26,                  /* 0x00C8 */
    PIN_GPIO27,                  /* 0x00C9 */
    PIN_GPIO20,                  /* 0x00CA */
    PIN_GPIO21,                  /* 0x00CB */
    PIN_GPIO22,                  /* 0x00CC */
    PIN_GPIO23,                  /* 0x00CD */
    PIN_GPIO36,                  /* 0x00CE */
    PIN_GPIO37,                  /* 0x00CF */
    PIN_GPIO18,                  /* 0x00D0 */
    PIN_GPIO19,                  /* 0x00D1 */
    PIN_GPIO14,                  /* 0x00D2 */
    PIN_GPIO15,                  /* 0x00D3 */
    PIN_GPIO16,                  /* 0x00D4 */
    PIN_GPIO17,                  /* 0x00D5 */
    PIN_GPIO10,                  /* 0x00D6 */
    PIN_GPIO11,                  /* 0x00D7 */
    PIN_GPIO12,                  /* 0x00D8 */
    PIN_GPIO13,                  /* 0x00D9 */
    PIN_GPIO6,                   /* 0x00DA */
    PIN_GPIO7,                   /* 0x00DB */
    PIN_GPIO8,                   /* 0x00DC */
    PIN_GPIO9,                   /* 0x00DD */
    PIN_GPIO2,                   /* 0x00DE */
    PIN_GPIO3,                   /* 0x00DF */
    PIN_GPIO4,                   /* 0x00E0 */
    PIN_GPIO5,                   /* 0x00E1 */
    PIN_UART_RTS3_B,             /* 0x00E2 */
    PIN_UART_CTS3_B,             /* 0x00E3 */
    PIN_GPIO0,                   /* 0x00E4 */
    PIN_GPIO1,                   /* 0x00E5 */
    PIN_MQSPI2_CS0,              /* 0x00E6 */
    PIN_MQSPI2_CS1,              /* 0x00E7 */
    PIN_UART_TXD3,               /* 0x00E8 */
    PIN_UART_RXD3,               /* 0x00E9 */
    PIN_MQSPI1_CS2,              /* 0x00EA */
    PIN_MQSPI2_CK,               /* 0x00EB */
    PIN_MQSPI2_DI,               /* 0x00EC */
    PIN_MQSPI2_DO,               /* 0x00ED */
    PIN_MQSPI1_DI,               /* 0x00EE */
    PIN_MQSPI1_DO,               /* 0x00EF */
    PIN_MQSPI1_CS0,              /* 0x00F0 */
    PIN_MQSPI1_CS1,              /* 0x00F1 */
    PIN_L1T2OUT1,                /* 0x00F2 */
    PIN_L1T2OUT2,                /* 0x00F3 */
    PIN_L1T2OUT3,                /* 0x00F4 */
    PIN_MQSPI1_CK,               /* 0x00F5 */
    PIN_L1T1OUT3,                /* 0x00F6 */
    PIN_L1T1OUT4,                /* 0x00F7 */
    PIN_L1T1OUT7,                /* 0x00F8 */
    PIN_L1T1OUT5,                /* 0x00F9 */
    PIN_L1T1OUT8,                /* 0x00FA */
    PIN_L1T2OUT0,                /* 0x00FB */
    PIN_DSM2_VCO_EN,             /* 0x00FC */
    PIN_L1T1OUT0,                /* 0x00FD */
    PIN_L1T1OUT1,                /* 0x00FE */
    PIN_L1T1OUT2,                /* 0x00FF */
    PIN_WAMMO_DIV_RX_FRAME,      /* 0x0100 */
    PIN_DSM1_STBY,               /* 0x0101 */
    PIN_DSM1_RX_ON,              /* 0x0102 */
    PIN_DSM2_STBY,               /* 0x0103 */
    PIN_WAMMO_TX_FRAME,          /* 0x0104 */
    PIN_WAMMO_RX_FRAME,          /* 0x0105 */
    PIN_WAMMO_DIV_RX_I_CH,       /* 0x0106 */
    PIN_WAMMO_DIV_RX_Q_CH,       /* 0x0107 */
    PIN_WAMMO_TX_Q_CH,           /* 0x0108 */
    PIN_WAMMO_TX_I_CH,           /* 0x0109 */
    PIN_WAMMO_RX_Q_CH,           /* 0x010A */
    PIN_WAMMO_RX_I_CH,           /* 0x010B */
    PIN_GPIO34,                  /* 0x010C */
    PIN_GPIO35,                  /* 0x010D */
    PIN_SDMA_EVNT0,              /* 0x010E */
    PIN_SDMA_EVNT1,              /* 0x010F */
    PIN_GPIO30,                  /* 0x0110 */
    PIN_GPIO31,                  /* 0x0111 */
    PIN_GPIO32,                  /* 0x0112 */
    PIN_GPIO33,                  /* 0x0113 */
    PIN_BBP_RX_DATA,             /* 0x0114 */
    PIN_BBP_TX_DATA,             /* 0x0115 */
    PIN_GPIO28,                  /* 0x0116 */
    PIN_GPIO29,                  /* 0x0117 */
    PIN_BBP_RX_CLK,              /* 0x0118 */
    PIN_BBP_TX_CLK,              /* 0x0119 */
    PIN_BBP_RX_FRAME,            /* 0x011A */
    PIN_BBP_TX_FRAME,            /* 0x011B */
    PIN_USB_VPIN,                /* 0x011C */
    PIN_USB_VPOUT,               /* 0x011D */
    PIN_USB_VMIN,                /* 0x011E */
    PIN_USB_XRXD,                /* 0x011F */
    PIN_USB1_VMIN,               /* 0x0120 */
    PIN_USB1_XRXD,               /* 0x0121 */
    PIN_USB_VMOUT,               /* 0x0122 */
    PIN_USB_TXENB,               /* 0x0123 */
    PIN_USB1_VMOUT,              /* 0x0124 */
    PIN_USB1_TXENB,              /* 0x0125 */
    PIN_USB1_VPIN,               /* 0x0126 */
    PIN_USB1_VPOUT,              /* 0x0127 */
    PIN_SIM1_RST0,               /* 0x0128 */
    PIN_SIM1_DATA0_RX_TX,        /* 0x0129 */
    PIN_SIM1_SVEN0,              /* 0x012A */
    PIN_SIM1_SIMPD0,             /* 0x012B */
    PIN_SJC_MOD,                 /* 0x012C */
    PIN_DE_B,                    /* 0x012D */
    PIN_EVTI_B,                  /* 0x012E */
    PIN_SIM1_CLK0,               /* 0x012F */
    PIN_TDI,                     /* 0x0130 */
    PIN_RTCK,                    /* 0x0131 */
    PIN_TCK,                     /* 0x0132 */
    PIN_TMS,                     /* 0x0133 */
    PIN_USER_OFF,                /* 0x0134 */
    PIN_WDOG_B,                  /* 0x0135 */
    PIN_TRSTB,                   /* 0x0136 */
    PIN_TDO,                     /* 0x0137 */
    PIN_BOOT_MODE1,              /* 0x0138 */
    PIN_REF_EN_B,                /* 0x0139 */
    PIN_PM_INT,                  /* 0x013A */
    PIN_POWER_FAIL,              /* 0x013B */
    PIN_RESET_IN_B,              /* 0x013C */
    PIN_RESET_POR_IN_B,          /* 0x013D */
    PIN_TEST_MODE,               /* 0x013E */
    PIN_BOOT_MODE0,              /* 0x013F */
    PIN_MB_REF_CLK_IN,           /* 0x0140 */
    PIN_CKIH_REF_CLK_IN,         /* 0x0141 */
    PIN_CKO1,                    /* 0x0142 */
    PIN_CKO2,                    /* 0x0143 */
    PIN_D14,                     /* 0x0144 */
    PIN_D15,                     /* 0x0145 */
    PIN_CKIL,                    /* 0x0146 */
    PIN_WB_REF_CLK_IN,           /* 0x0147 */
    PIN_D10,                     /* 0x0148 */
    PIN_D11,                     /* 0x0149 */
    PIN_D12,                     /* 0x014A */
    PIN_D13,                     /* 0x014B */
    PIN_D6,                      /* 0x014C */
    PIN_D7,                      /* 0x014D */
    PIN_D8,                      /* 0x014E */
    PIN_D9,                      /* 0x014F */
    PIN_D2,                      /* 0x0150 */
    PIN_D3,                      /* 0x0151 */
    PIN_D4,                      /* 0x0152 */
    PIN_D5,                      /* 0x0153 */
    PIN_WE,                      /* 0x0154 */
    PIN_R_B,                     /* 0x0155 */
    PIN_D0,                      /* 0x0156 */
    PIN_D1,                      /* 0x0157 */
    PIN_CLE,                     /* 0x0158 */
    PIN_WP,                      /* 0x0159 */
    PIN_ALE,                     /* 0x015A */
    PIN_RE,                      /* 0x015B */
    PIN_SDCKE0,                  /* 0x015C */
    PIN_SDCKE1,                  /* 0x015D */
    PIN_SDCLK,                   /* 0x015E */
    PIN_SDCLK_B,                 /* 0x015F */
    PIN_EB3_B,                   /* 0x0160 */
    PIN_RAS,                     /* 0x0161 */
    PIN_CAS,                     /* 0x0162 */
    PIN_SDWE,                    /* 0x0163 */
    PIN_LBA_B,                   /* 0x0164 */
    PIN_BCLK,                    /* 0x0165 */
    PIN_RW_B,                    /* 0x0166 */
    PIN_EB0_B,                   /* 0x0167 */
    PIN_CS4_B,                   /* 0x0168 */
    PIN_SDBA0,                   /* 0x0169 */
    PIN_SDBA1,                   /* 0x016A */
    PIN_ECB_B,                   /* 0x016B */
    PIN_CS0_B,                   /* 0x016C */
    PIN_CS1_B,                   /* 0x016D */
    PIN_CS2_B,                   /* 0x016E */
    PIN_CS3_B,                   /* 0x016F */
    PIN_DQM1,                    /* 0x0170 */
    PIN_DQM2,                    /* 0x0171 */
    PIN_DQM3,                    /* 0x0172 */
    PIN_OE_B,                    /* 0x0173 */
    PIN_SDQS1,                   /* 0x0174 */
    PIN_MGRNT,                   /* 0x0175 */
    PIN_EB1_b,                   /* 0x0176 */
    PIN_MRQST,                   /* 0x0177 */
    PIN_EB2_B,                   /* 0x0178 */
    PIN_DQM0,                    /* 0x0179 */
    PIN_SD29,                    /* 0x017A */
    PIN_SD30,                    /* 0x017B */
    PIN_SD31,                    /* 0x017C */
    PIN_SDQS0,                   /* 0x017D */
    PIN_SD25,                    /* 0x017E */
    PIN_SD26,                    /* 0x017F */
    PIN_SD27,                    /* 0x0180 */
    PIN_SD28,                    /* 0x0181 */
    PIN_SD21,                    /* 0x0182 */
    PIN_SD22,                    /* 0x0183 */
    PIN_SD23,                    /* 0x0184 */
    PIN_SD24,                    /* 0x0185 */
    PIN_SD17,                    /* 0x0186 */
    PIN_SD18,                    /* 0x0187 */
    PIN_SD19,                    /* 0x0188 */
    PIN_SD20,                    /* 0x0189 */
    PIN_SD13,                    /* 0x018A */
    PIN_SD14,                    /* 0x018B */
    PIN_SD15,                    /* 0x018C */
    PIN_SD16,                    /* 0x018D */
    PIN_SD9,                     /* 0x018E */
    PIN_SD10,                    /* 0x018F */
    PIN_SD11,                    /* 0x0190 */
    PIN_SD12,                    /* 0x0191 */
    PIN_SD5,                     /* 0x0192 */
    PIN_SD6,                     /* 0x0193 */
    PIN_SD7,                     /* 0x0194 */
    PIN_SD8,                     /* 0x0195 */
    PIN_SD1,                     /* 0x0196 */
    PIN_SD2,                     /* 0x0197 */
    PIN_SD3,                     /* 0x0198 */
    PIN_SD4,                     /* 0x0199 */
    PIN_A23,                     /* 0x019A */
    PIN_A24,                     /* 0x019B */
    PIN_A25,                     /* 0x019C */
    PIN_SD0,                     /* 0x019D */
    PIN_A19,                     /* 0x019E */
    PIN_A20,                     /* 0x019F */
    PIN_A21,                     /* 0x01A0 */
    PIN_A22,                     /* 0x01A1 */
    PIN_A15,                     /* 0x01A2 */
    PIN_A16,                     /* 0x01A3 */
    PIN_A17,                     /* 0x01A4 */
    PIN_A18,                     /* 0x01A5 */
    PIN_A11_MA11,                /* 0x01A6 */
    PIN_A12_MA12,                /* 0x01A7 */
    PIN_A13_MA13,                /* 0x01A8 */
    PIN_A14,                     /* 0x01A9 */
    PIN_A8_MA8,                  /* 0x01AA */
    PIN_A9_MA9,                  /* 0x01AB */
    PIN_A10,                     /* 0x01AC */
    PIN_MA10,                    /* 0x01AD */
    PIN_A4_MA4,                  /* 0x01AE */
    PIN_A5_MA5,                  /* 0x01AF */
    PIN_A6_MA6,                  /* 0x01B0 */
    PIN_A7_MA7,                  /* 0x01B1 */
    PIN_A0_MA0,                  /* 0x01B2 */
    PIN_A1_MA1,                  /* 0x01B3 */
    PIN_A2_MA2,                  /* 0x01B4 */
    PIN_A3_MA3,                  /* 0x01B5 */
};


/**
 * Convert the value defined in the hardware config schema for an IOMUX
 * pin to the Freescale defined value for the same pin.
 *
 * @param   schemavalue Value assocated with a pin name in hwcfg.
 * 
 * @return  Freescale pin name for the pin.
 */
enum iomux_pins __init hwcfg_pin_to_fsl_pin(u16 schemavalue)
{
    if( schemavalue < HWCFG_IOMUX_PIN_COUNT ) {
        return hwcfg_pins[schemavalue];
    } else {
        return IOMUX_INVALID_PIN;
    }
}


/**
 * Set an IOMUX pad group register based on data passed in from HWCFG.
 *
 * @param   grp     HWCFG identifier for pad group.
 * @param   config  HWCFG identifier for pad setting.
 */
void __init arch_mot_iomux_pad_init(u16 grp, u16 config)
{
    /* Argon pad groups are the same as pins */
    enum iomux_pins fsl_pin = hwcfg_pin_to_fsl_pin(grp);

    if(fsl_pin == IOMUX_INVALID_PIN) {
        printk("%s: Unknown IOMUX pad group 0x%04X\n",
                __FUNCTION__, grp);
        return;
    }
    
    gpio_tracemsg("Setting pad register 0x%08x to: 0x%04x", fsl_pin, config);

    iomux_config_pad(fsl_pin, config);
}
#endif  // #if defined(CONFIG_MOT_FEAT_MOTHWCFG_DEVICE_TREE)
