/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
#ifndef _ASM_ARCH_MXC91231_PINS_H
#define _ASM_ARCH_MXC91231_PINS_H

#ifndef __ASSEMBLY__

/*!
 * @name IOMUX/PAD Bit field definitions
 */

/*! @{ */

/*!
 * In order to identify pins more effectively, each mux-controlled pin's
 * enumerated value is constructed in the following way:
 *
 * -------------------------------------------------------------------
 * 31-29 | 28 - 24 |23 - 21| 20  | 19 - 18 | 17 - 10| 9 - 8 | 7 - 0
 * -------------------------------------------------------------------
 * IO_P  |  IO_I   |     RSVD    |  PAD_F  |  PAD_I | MUX_F | MUX_I
 * -------------------------------------------------------------------
 *
 * Bit 0 to 7 contains MUX_I used to identify the register
 * offset (0-based. base is IOMUX_module_base + 0xC) defined in the Section
 * "sw_pad_ctl & sw_mux_ctl details" of the IC Spec. Bit 8 to 9 is MUX_F which
 * contains the offset value defined WITHIN the same register (each IOMUX
 * control register contains four 8-bit fields for four different pins). The
 * similar field definitions are used for the pad control register.
 * For example, the MX31_PIN_A0 is defined in the enumeration:
 *    ( 73 << MUX_I) | (0 << MUX_F)|( 98 << PAD_I) | (0 << PAD_F)
 * It means the mux control register is at register offset 73. So the absolute
 * address is: 0xC+73*4=0x130   0 << MUX_F means the control bits are at the
 * least significant bits within the register. The pad control register offset
 * is: 0x154+98*4=0x2DC and also occupy the least significant bits within the
 * register.
 */

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * gpio port number (0-based) for that pin. For non-gpio pins, the bits will
 * be all 1's for error checking in the functions. (gpio port 7 is invalid)
 *
 * 0 = GPIO_AP_A*, 1 = GPIO_AP_B*, 2 = GPIO_AP_C*, 3 = GPIO_SP_A*
 */
#define MUX_IO_P	29

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * gpio offset bit (0-based) for that pin. For non-gpio pins, the bits will
 * be all 0's since they are don't cares. So for port 2 pin 21, bit 31-24
 * will be (1 << MUX_IO_P) | (21 << MUX_IO_I).
 */
#define MUX_IO_I	24

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * MUX control register index (0-based)
 */
#define MUX_I		0

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * field within IOMUX control register for control bits
 * (legal values are 0, 1, 2, 3)
 */
#define MUX_F		8

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * PAD control register index (0-based)
 */
#define PAD_I		10

/*!
 * Starting bit position within each entry of \b iomux_pins to represent the
 * field within PAD control register for control bits
 * (legal values are 0, 1)
 */
#define PAD_F		18

#define _MXC_BUILD_PIN(gp,gi,mi,mf,pi,pf) \
	((gp) << MUX_IO_P) | ((gi) << MUX_IO_I) | ((mi) << MUX_I) | \
	((mf) << MUX_F) | ((pi) << PAD_I) | ((pf) << PAD_F)

#define _MXC_BUILD_GPIO_PIN(gp,gi,mi,mf,pi,pf) \
		_MXC_BUILD_PIN(gp,gi,mi,mf,pi,pf)
#define _MXC_BUILD_NON_GPIO_PIN(mi,mf,pi,pf) \
		_MXC_BUILD_PIN(7,0,mi,mf,pi,pf)

/*! @} End IOMUX/PAD Bit field definitions */

/*!
 * This enumeration is constructed based on the section 3 "IOMUX Control"
 * of the MXC91231 IC Spec. Each enumerated value is constructed based
 * on the rules described above.
 */
typedef enum iomux_pins {
	/* IOMUX for AP side */
	AP_CLE = _MXC_BUILD_GPIO_PIN(0, 0, 0, 0, 12, 0),
	AP_ALE = _MXC_BUILD_GPIO_PIN(0, 1, 0, 1, 12, 0),
	AP_CE_B = _MXC_BUILD_GPIO_PIN(0, 2, 0, 2, 12, 0),
	AP_RE_B = _MXC_BUILD_GPIO_PIN(0, 3, 0, 3, 12, 0),
	AP_WE_B = _MXC_BUILD_GPIO_PIN(0, 4, 1, 0, 12, 0),
	AP_WP_B = _MXC_BUILD_GPIO_PIN(0, 5, 1, 1, 12, 0),
	AP_BSY_B = _MXC_BUILD_GPIO_PIN(0, 6, 1, 2, 12, 0),
	AP_U1_TXD = _MXC_BUILD_GPIO_PIN(0, 7, 1, 3, 14, 0),
	AP_U1_RXD = _MXC_BUILD_GPIO_PIN(0, 8, 2, 0, 14, 0),
	AP_U1_RTS_B = _MXC_BUILD_GPIO_PIN(0, 9, 2, 1, 14, 0),
	AP_U1_CTS_B = _MXC_BUILD_GPIO_PIN(0, 10, 2, 2, 14, 0),
	AP_AD1_TXD = _MXC_BUILD_GPIO_PIN(0, 11, 2, 3, 4, 1),
	AP_AD1_RXD = _MXC_BUILD_GPIO_PIN(0, 12, 3, 0, 4, 1),
	AP_AD1_TXC = _MXC_BUILD_GPIO_PIN(0, 13, 3, 1, 4, 1),
	AP_AD1_TXFS = _MXC_BUILD_GPIO_PIN(0, 14, 3, 2, 4, 1),
	AP_AD2_TXD = _MXC_BUILD_GPIO_PIN(0, 15, 3, 3, 4, 1),
	AP_AD2_RXD = _MXC_BUILD_GPIO_PIN(0, 16, 4, 0, 4, 1),
	AP_AD2_TXC = _MXC_BUILD_GPIO_PIN(0, 17, 4, 1, 4, 1),
	AP_AD2_TXFS = _MXC_BUILD_GPIO_PIN(0, 18, 4, 2, 4, 1),
	AP_OWDAT = _MXC_BUILD_GPIO_PIN(0, 19, 4, 3, 14, 0),
	AP_IPU_LD17 = _MXC_BUILD_GPIO_PIN(0, 20, 5, 0, 14, 0),
	AP_IPU_D3_VSYNC = _MXC_BUILD_GPIO_PIN(0, 21, 5, 1, 14, 0),
	AP_IPU_D3_HSYNC = _MXC_BUILD_GPIO_PIN(0, 22, 5, 2, 14, 0),
	AP_IPU_D3_CLK = _MXC_BUILD_GPIO_PIN(0, 23, 5, 3, 14, 0),
	AP_IPU_D3_DRDY = _MXC_BUILD_GPIO_PIN(0, 24, 6, 0, 14, 0),
	AP_IPU_D3_CONTR = _MXC_BUILD_GPIO_PIN(0, 25, 6, 1, 14, 0),
	AP_IPU_D0_CS = _MXC_BUILD_GPIO_PIN(0, 26, 6, 2, 14, 0),
	AP_IPU_LD16 = _MXC_BUILD_GPIO_PIN(0, 27, 6, 3, 14, 0),
	AP_IPU_D2_CS = _MXC_BUILD_GPIO_PIN(0, 28, 7, 0, 14, 0),
	AP_IPU_PAR_RS = _MXC_BUILD_GPIO_PIN(0, 29, 7, 1, 14, 0),
	AP_IPU_D3_PS = _MXC_BUILD_GPIO_PIN(0, 30, 7, 2, 14, 0),
	AP_IPU_D3_CLS = _MXC_BUILD_GPIO_PIN(0, 31, 7, 3, 14, 0),
	AP_IPU_RD = _MXC_BUILD_GPIO_PIN(1, 0, 8, 0, 14, 0),
	AP_IPU_WR = _MXC_BUILD_GPIO_PIN(1, 1, 8, 1, 14, 0),
	AP_IPU_LD0 = _MXC_BUILD_NON_GPIO_PIN(8, 2, 14, 0),
	AP_IPU_LD1 = _MXC_BUILD_NON_GPIO_PIN(8, 3, 14, 0),
	AP_IPU_LD2 = _MXC_BUILD_NON_GPIO_PIN(9, 0, 14, 0),
	AP_IPU_LD3 = _MXC_BUILD_GPIO_PIN(1, 2, 9, 1, 14, 0),
	AP_IPU_LD4 = _MXC_BUILD_GPIO_PIN(1, 3, 9, 2, 14, 0),
	AP_IPU_LD5 = _MXC_BUILD_GPIO_PIN(1, 4, 9, 3, 14, 0),
	AP_IPU_LD6 = _MXC_BUILD_GPIO_PIN(1, 5, 10, 0, 14, 0),
	AP_IPU_LD7 = _MXC_BUILD_GPIO_PIN(1, 6, 10, 1, 14, 0),
	AP_IPU_LD8 = _MXC_BUILD_GPIO_PIN(1, 7, 10, 2, 14, 0),
	AP_IPU_LD9 = _MXC_BUILD_GPIO_PIN(1, 8, 10, 3, 14, 0),
	AP_IPU_LD10 = _MXC_BUILD_GPIO_PIN(1, 9, 11, 0, 14, 0),
	AP_IPU_LD11 = _MXC_BUILD_GPIO_PIN(1, 10, 11, 1, 14, 0),
	AP_IPU_LD12 = _MXC_BUILD_GPIO_PIN(1, 11, 11, 2, 14, 0),
	AP_IPU_LD13 = _MXC_BUILD_GPIO_PIN(1, 12, 11, 3, 14, 0),
	AP_IPU_LD14 = _MXC_BUILD_GPIO_PIN(1, 13, 12, 0, 14, 0),
	AP_IPU_LD15 = _MXC_BUILD_GPIO_PIN(1, 14, 12, 1, 14, 0),
	AP_KPROW4 = _MXC_BUILD_NON_GPIO_PIN(12, 2, 5, 0),
	AP_KPROW5 = _MXC_BUILD_GPIO_PIN(1, 16, 12, 3, 5, 0),
	AP_GPIO_AP_B17 = _MXC_BUILD_GPIO_PIN(1, 17, 13, 0, 5, 0),
	AP_GPIO_AP_B18 = _MXC_BUILD_GPIO_PIN(1, 18, 13, 1, 5, 0),
	AP_KPCOL3 = _MXC_BUILD_GPIO_PIN(1, 19, 13, 2, 5, 1),
	AP_KPCOL4 = _MXC_BUILD_GPIO_PIN(1, 20, 13, 3, 5, 1),
	AP_KPCOL5 = _MXC_BUILD_GPIO_PIN(1, 21, 14, 0, 5, 1),
	AP_GPIO_AP_B22 = _MXC_BUILD_GPIO_PIN(1, 22, 14, 1, 5, 1),
	AP_GPIO_AP_B23 = _MXC_BUILD_GPIO_PIN(1, 23, 14, 2, 5, 1),
	AP_CSI_D0 = _MXC_BUILD_GPIO_PIN(1, 24, 14, 3, 10, 1),
	AP_CSI_D1 = _MXC_BUILD_GPIO_PIN(1, 25, 15, 0, 10, 1),
	AP_CSI_D2 = _MXC_BUILD_GPIO_PIN(1, 26, 15, 1, 10, 1),
	AP_CSI_D3 = _MXC_BUILD_GPIO_PIN(1, 27, 15, 2, 10, 1),
	AP_CSI_D4 = _MXC_BUILD_GPIO_PIN(1, 28, 15, 3, 10, 1),
	AP_CSI_D5 = _MXC_BUILD_GPIO_PIN(1, 29, 16, 0, 10, 1),
	AP_CSI_D6 = _MXC_BUILD_GPIO_PIN(1, 30, 16, 1, 10, 1),
	AP_CSI_D7 = _MXC_BUILD_GPIO_PIN(1, 31, 16, 2, 10, 1),
	AP_CSI_D8 = _MXC_BUILD_GPIO_PIN(2, 0, 16, 3, 10, 1),
	AP_CSI_D9 = _MXC_BUILD_GPIO_PIN(2, 1, 17, 0, 10, 1),
	AP_CSI_MCLK = _MXC_BUILD_GPIO_PIN(2, 2, 17, 1, 10, 1),
	AP_CSI_VSYNC = _MXC_BUILD_GPIO_PIN(2, 3, 17, 2, 10, 1),
	AP_CSI_HSYNC = _MXC_BUILD_GPIO_PIN(2, 4, 17, 3, 10, 1),
	AP_CSI_PIXCLK = _MXC_BUILD_GPIO_PIN(2, 5, 18, 0, 10, 1),
	AP_I2CLK = _MXC_BUILD_GPIO_PIN(2, 6, 18, 1, 6, 0),
	AP_I2DAT = _MXC_BUILD_GPIO_PIN(2, 7, 18, 2, 6, 0),
	AP_GPIO_AP_C8 = _MXC_BUILD_GPIO_PIN(2, 8, 18, 3, 4, 1),
	AP_GPIO_AP_C9 = _MXC_BUILD_GPIO_PIN(2, 9, 19, 0, 4, 1),
	AP_GPIO_AP_C10 = _MXC_BUILD_GPIO_PIN(2, 10, 19, 1, 4, 1),
	AP_GPIO_AP_C11 = _MXC_BUILD_GPIO_PIN(2, 11, 19, 2, 4, 1),
	AP_GPIO_AP_C12 = _MXC_BUILD_GPIO_PIN(2, 12, 19, 3, 4, 1),
	AP_GPIO_AP_C13 = _MXC_BUILD_GPIO_PIN(2, 13, 20, 0, 14, 0),
	AP_GPIO_AP_C14 = _MXC_BUILD_GPIO_PIN(2, 14, 20, 1, 14, 0),
	AP_GPIO_AP_C15 = _MXC_BUILD_GPIO_PIN(2, 15, 20, 2, 4, 1),
	AP_GPIO_AP_C16 = _MXC_BUILD_GPIO_PIN(2, 16, 20, 3, 4, 1),
	AP_GPIO_AP_C17 = _MXC_BUILD_GPIO_PIN(2, 17, 21, 0, 4, 1),
	AP_ED_INT0 = _MXC_BUILD_GPIO_PIN(2, 18, 21, 1, 11, 0),
	AP_ED_INT1 = _MXC_BUILD_GPIO_PIN(2, 19, 21, 2, 11, 0),
	AP_ED_INT2 = _MXC_BUILD_GPIO_PIN(2, 20, 21, 3, 11, 0),
	AP_ED_INT3 = _MXC_BUILD_GPIO_PIN(2, 21, 22, 0, 11, 0),
	AP_ED_INT4 = _MXC_BUILD_GPIO_PIN(2, 22, 22, 1, 11, 1),
	AP_ED_INT5 = _MXC_BUILD_GPIO_PIN(2, 23, 22, 2, 11, 1),
	AP_ED_INT6 = _MXC_BUILD_GPIO_PIN(2, 24, 22, 3, 11, 1),
	AP_ED_INT7 = _MXC_BUILD_GPIO_PIN(2, 25, 23, 0, 11, 1),
	AP_U2_DSR_B = _MXC_BUILD_GPIO_PIN(2, 26, 23, 1, 14, 0),
	AP_U2_RI_B = _MXC_BUILD_GPIO_PIN(2, 27, 23, 2, 14, 0),
	AP_U2_CTS_B = _MXC_BUILD_GPIO_PIN(2, 28, 23, 3, 14, 0),
	AP_U2_DTR_B = _MXC_BUILD_GPIO_PIN(2, 29, 24, 0, 14, 0),
	AP_KPROW0 = _MXC_BUILD_NON_GPIO_PIN(24, 1, 5, 0),
	AP_KPROW1 = _MXC_BUILD_GPIO_PIN(1, 15, 24, 2, 5, 0),
	AP_KPROW2 = _MXC_BUILD_NON_GPIO_PIN(24, 3, 5, 0),
	AP_KPROW3 = _MXC_BUILD_NON_GPIO_PIN(25, 0, 5, 0),
	AP_KPCOL0 = _MXC_BUILD_NON_GPIO_PIN(25, 1, 5, 1),
	AP_KPCOL1 = _MXC_BUILD_NON_GPIO_PIN(25, 2, 5, 1),
	AP_KPCOL2 = _MXC_BUILD_NON_GPIO_PIN(25, 3, 5, 1),

	/* IOMUX shared between AP and BP */
	SP_U3_TXD = _MXC_BUILD_GPIO_PIN(3, 0, 0x80 + 0, 0, 14, 0),
	SP_U3_RXD = _MXC_BUILD_GPIO_PIN(3, 1, 0x80 + 0, 1, 14, 0),
	SP_U3_RTS_B = _MXC_BUILD_GPIO_PIN(3, 2, 0x80 + 0, 2, 14, 0),
	SP_U3_CTS_B = _MXC_BUILD_GPIO_PIN(3, 3, 0x80 + 0, 3, 14, 0),
	SP_USB_TXOE_B = _MXC_BUILD_GPIO_PIN(3, 4, 0x80 + 1, 0, 14, 0),
	SP_USB_DAT_VP = _MXC_BUILD_GPIO_PIN(3, 5, 0x80 + 1, 1, 14, 0),
	SP_USB_SE0_VM = _MXC_BUILD_GPIO_PIN(3, 6, 0x80 + 1, 2, 14, 0),
	SP_USB_RXD = _MXC_BUILD_GPIO_PIN(3, 7, 0x80 + 1, 3, 14, 0),
	SP_UH2_TXOE_B = _MXC_BUILD_GPIO_PIN(3, 8, 0x80 + 2, 0, 14, 0),
	SP_UH2_SPEED = _MXC_BUILD_GPIO_PIN(3, 9, 0x80 + 2, 1, 14, 0),
	SP_UH2_SUSPEND = _MXC_BUILD_GPIO_PIN(3, 10, 0x80 + 2, 2, 14, 0),
	SP_UH2_TXDP = _MXC_BUILD_GPIO_PIN(3, 11, 0x80 + 2, 3, 14, 0),
	SP_UH2_RXDP = _MXC_BUILD_GPIO_PIN(3, 12, 0x80 + 3, 0, 14, 0),
	SP_UH2_RXDM = _MXC_BUILD_GPIO_PIN(3, 13, 0x80 + 3, 1, 14, 0),
	SP_UH2_OVR = _MXC_BUILD_GPIO_PIN(3, 14, 0x80 + 3, 2, 14, 0),
	SP_UH2_PWR = _MXC_BUILD_GPIO_PIN(3, 15, 0x80 + 3, 3, 14, 0),
	SP_SD1_DAT0 = _MXC_BUILD_GPIO_PIN(3, 16, 0x80 + 4, 0, 12, 1),
	SP_SD1_DAT1 = _MXC_BUILD_GPIO_PIN(3, 17, 0x80 + 4, 1, 12, 1),
	SP_SD1_DAT2 = _MXC_BUILD_GPIO_PIN(3, 18, 0x80 + 4, 2, 12, 1),
	SP_SD1_DAT3 = _MXC_BUILD_GPIO_PIN(3, 19, 0x80 + 4, 3, 12, 1),
	SP_SD1_CMD = _MXC_BUILD_GPIO_PIN(3, 20, 0x80 + 5, 0, 12, 1),
	SP_SD1_CLK = _MXC_BUILD_GPIO_PIN(3, 21, 0x80 + 5, 1, 12, 1),
	SP_SD2_DAT0 = _MXC_BUILD_GPIO_PIN(3, 22, 0x80 + 5, 2, 13, 0),
	SP_SD2_DAT1 = _MXC_BUILD_GPIO_PIN(3, 23, 0x80 + 5, 3, 13, 0),
	SP_SD2_DAT2 = _MXC_BUILD_GPIO_PIN(3, 24, 0x80 + 6, 0, 13, 0),
	SP_SD2_DAT3 = _MXC_BUILD_GPIO_PIN(3, 25, 0x80 + 6, 1, 13, 0),
	SP_GPIO_Shared26 = _MXC_BUILD_GPIO_PIN(3, 26, 0x80 + 6, 2, 14, 0),
	SP_SPI1_CLK = _MXC_BUILD_GPIO_PIN(3, 27, 0x80 + 6, 3, 6, 1),
	SP_SPI1_MOSI = _MXC_BUILD_GPIO_PIN(3, 28, 0x80 + 7, 0, 6, 1),
	SP_SPI1_MISO = _MXC_BUILD_GPIO_PIN(3, 29, 0x80 + 7, 1, 6, 1),
	SP_SPI1_SS0 = _MXC_BUILD_GPIO_PIN(3, 30, 0x80 + 7, 2, 6, 1),
	SP_SPI1_SS1 = _MXC_BUILD_GPIO_PIN(3, 31, 0x80 + 7, 3, 6, 1),
	SP_SD2_CMD = _MXC_BUILD_NON_GPIO_PIN(0x80 + 8, 0, 13, 0),
	SP_SD2_CLK = _MXC_BUILD_NON_GPIO_PIN(0x80 + 8, 1, 13, 0),
	SP_SIM1_RST_B = _MXC_BUILD_GPIO_PIN(2, 30, 0x80 + 8, 2, 14, 0),
	SP_SIM1_SVEN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 8, 3, 14, 0),
	SP_SIM1_CLK = _MXC_BUILD_NON_GPIO_PIN(0x80 + 9, 0, 14, 0),
	SP_SIM1_TRXD = _MXC_BUILD_NON_GPIO_PIN(0x80 + 9, 1, 14, 0),
	SP_SIM1_PD = _MXC_BUILD_GPIO_PIN(2, 31, 0x80 + 9, 2, 14, 0),
	SP_UH2_TXDM = _MXC_BUILD_NON_GPIO_PIN(0x80 + 9, 3, 14, 0),
	SP_UH2_RXD = _MXC_BUILD_NON_GPIO_PIN(0x80 + 10, 0, 14, 0),

	AP_MAX_PIN = AP_KPCOL2,
} iomux_pin_name_t;

#endif
#endif
