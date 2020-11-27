/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef _ASM_ARCH_MXC91131_PINS_H
#define _ASM_ARCH_MXC91131_PINS_H

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
 * 0 = GPIO_AP_A*, 1 = GPIO_AP_B*, 2 = GPIO_AP_C*
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
 * This enumeration is constructed based on the Section
 * "sw_pad_ctl & sw_mux_ctl details" of the MXC91131 IC Spec. Each enumerated
 * value is constructed based on the rules described above.
 */
typedef enum iomux_pins {
	/* IOMUX_AP pins */
	AD1_RXD_PIN = _MXC_BUILD_GPIO_PIN(2, 19, 0, 0, 0, 0),
	AD1_TXC_PIN = _MXC_BUILD_GPIO_PIN(2, 20, 0, 1, 0, 0),
	AD1_TXD_PIN = _MXC_BUILD_GPIO_PIN(2, 18, 0, 2, 0, 0),
	AD1_TXFS_PIN = _MXC_BUILD_GPIO_PIN(2, 21, 0, 3, 0, 0),
	AD2_RXD_PIN = _MXC_BUILD_GPIO_PIN(2, 23, 1, 0, 0, 0),
	AD2_TXC_PIN = _MXC_BUILD_GPIO_PIN(2, 24, 1, 1, 0, 0),
	AD2_TXD_PIN = _MXC_BUILD_GPIO_PIN(2, 22, 1, 2, 0, 0),
	AD2_TXFS_PIN = _MXC_BUILD_GPIO_PIN(2, 25, 1, 3, 0, 0),
	AD3_RXC_PIN = _MXC_BUILD_GPIO_PIN(0, 23, 2, 0, 0, 0),
	AD3_RXD_PIN = _MXC_BUILD_GPIO_PIN(0, 22, 2, 1, 0, 0),
	AD3_RXFS_PIN = _MXC_BUILD_GPIO_PIN(0, 24, 2, 2, 0, 0),
	AD3_TXC_PIN = _MXC_BUILD_GPIO_PIN(1, 24, 3, 3, 0, 0),
	AD3_TXD_PIN = _MXC_BUILD_GPIO_PIN(0, 21, 3, 0, 0, 0),
	AD3_TXFS_PIN = _MXC_BUILD_GPIO_PIN(1, 25, 3, 1, 0, 0),
	ALE_PIN = _MXC_BUILD_GPIO_PIN(0, 10, 3, 2, 0, 0),
	BMOD0_PIN = _MXC_BUILD_GPIO_PIN(0, 0, 3, 3, 0, 0),
	BMOD1_PIN = _MXC_BUILD_GPIO_PIN(0, 1, 4, 0, 0, 0),
	BMOD2_PIN = _MXC_BUILD_GPIO_PIN(0, 2, 4, 1, 0, 0),
	BSY_B_PIN = _MXC_BUILD_GPIO_PIN(0, 8, 4, 2, 0, 0),
	CE_B_PIN = _MXC_BUILD_GPIO_PIN(0, 11, 4, 3, 0, 0),
	CLE_PIN = _MXC_BUILD_GPIO_PIN(0, 9, 5, 0, 0, 0),
	CSI_D0_PIN = _MXC_BUILD_GPIO_PIN(2, 31, 5, 1, 0, 0),
	CSI_D1_PIN = _MXC_BUILD_GPIO_PIN(2, 30, 5, 2, 0, 0),
	CSI_D2_PIN = _MXC_BUILD_GPIO_PIN(2, 29, 5, 3, 0, 0),
	CSI_D3_PIN = _MXC_BUILD_GPIO_PIN(2, 28, 6, 0, 0, 0),
	CSI_D4_PIN = _MXC_BUILD_GPIO_PIN(2, 27, 6, 1, 0, 0),
	CSI_D5_PIN = _MXC_BUILD_GPIO_PIN(2, 26, 6, 2, 0, 0),
	CSI_D6_PIN = _MXC_BUILD_GPIO_PIN(2, 25, 7, 3, 0, 0),
	CSI_D7_PIN = _MXC_BUILD_GPIO_PIN(2, 24, 7, 0, 0, 0),
	CSI_D8_PIN = _MXC_BUILD_GPIO_PIN(2, 23, 7, 1, 0, 0),
	CSI_D9_PIN = _MXC_BUILD_GPIO_PIN(2, 22, 7, 2, 0, 0),
	CSI_HSYNC_PIN = _MXC_BUILD_GPIO_PIN(2, 19, 7, 3, 0, 0),
	CSI_PIXCLK_PIN = _MXC_BUILD_GPIO_PIN(2, 18, 8, 0, 0, 0),
	CSI_VSYNC_PIN = _MXC_BUILD_GPIO_PIN(2, 20, 8, 1, 0, 0),
	ED_INT0_PIN = _MXC_BUILD_GPIO_PIN(0, 28, 8, 2, 0, 0),
	ED_INT1_PIN = _MXC_BUILD_GPIO_PIN(0, 29, 8, 3, 0, 0),
	ED_INT2_PIN = _MXC_BUILD_GPIO_PIN(0, 30, 9, 0, 0, 0),
	ED_INT3_PIN = _MXC_BUILD_GPIO_PIN(0, 31, 9, 1, 0, 0),
	ED_INT4_PIN = _MXC_BUILD_GPIO_PIN(0, 13, 9, 2, 0, 0),
	ED_INT5_PIN = _MXC_BUILD_GPIO_PIN(0, 14, 9, 3, 0, 0),
	ED_INT6_PIN = _MXC_BUILD_GPIO_PIN(1, 2, 10, 0, 0, 0),
	ED_INT7_PIN = _MXC_BUILD_GPIO_PIN(1, 3, 10, 1, 0, 0),
	U1_TXD_PIN = _MXC_BUILD_GPIO_PIN(0, 0, 10, 2, 0, 0),
	U1_RXD_PIN = _MXC_BUILD_GPIO_PIN(0, 1, 11, 3, 0, 0),
	IPU_LD17_D0_VSYNC_PIN = _MXC_BUILD_GPIO_PIN(0, 15, 11, 0, 0, 0),
	IPU_D3_CLK_CONTR_PIN = _MXC_BUILD_GPIO_PIN(0, 16, 11, 1, 0, 0),
	IPU_D3_DRDY_PS_PIN = _MXC_BUILD_GPIO_PIN(0, 17, 11, 2, 0, 0),
	IPU_SER_RS_PIN = _MXC_BUILD_GPIO_PIN(0, 18, 11, 3, 0, 0),
	IPU_D2_CS_PIN = _MXC_BUILD_GPIO_PIN(0, 19, 12, 0, 0, 0),
	U1_RTS_B_PIN = _MXC_BUILD_GPIO_PIN(0, 2, 12, 1, 0, 0),
	IPU_REV_D1_CS_PIN = _MXC_BUILD_GPIO_PIN(0, 20, 12, 2, 0, 0),
	IPU_SD_D_PIN = _MXC_BUILD_GPIO_PIN(0, 21, 12, 3, 0, 0),
	IPU_SD_CLK_PIN = _MXC_BUILD_GPIO_PIN(0, 22, 13, 0, 0, 0),
	U1_CTS_B_PIN = _MXC_BUILD_GPIO_PIN(0, 3, 13, 1, 0, 0),
	U2_TXD_PIN = _MXC_BUILD_GPIO_PIN(0, 4, 13, 2, 0, 0),
	U2_RXD_PIN = _MXC_BUILD_GPIO_PIN(0, 5, 13, 3, 0, 0),
	U2_CTS_B_PIN = _MXC_BUILD_GPIO_PIN(0, 6, 14, 0, 0, 0),
	U2_RTS_B_PIN = _MXC_BUILD_GPIO_PIN(0, 7, 14, 1, 0, 0),
	SPI1_SS1_PIN = _MXC_BUILD_GPIO_PIN(0, 8, 14, 2, 0, 0),
	GP_AP_C0_PIN = _MXC_BUILD_GPIO_PIN(2, 0, 15, 3, 0, 0),
	GP_AP_C1_PIN = _MXC_BUILD_GPIO_PIN(2, 1, 15, 0, 0, 0),
	GP_AP_C10_PIN = _MXC_BUILD_GPIO_PIN(2, 10, 15, 1, 0, 0),
	GP_AP_C11_PIN = _MXC_BUILD_GPIO_PIN(2, 11, 15, 2, 0, 0),
	ICAP1_PIN = _MXC_BUILD_GPIO_PIN(2, 12, 15, 3, 0, 0),
	ICK_AP_PIN = _MXC_BUILD_GPIO_PIN(2, 13, 16, 0, 0, 0),
	OC1_PIN = _MXC_BUILD_GPIO_PIN(2, 14, 16, 1, 0, 0),
	OC2_PIN = _MXC_BUILD_GPIO_PIN(2, 15, 16, 2, 0, 0),
	OC3_PIN = _MXC_BUILD_GPIO_PIN(2, 16, 16, 3, 0, 0),
	ICAP2_PIN = _MXC_BUILD_GPIO_PIN(2, 17, 17, 0, 0, 0),
	GP_AP_C2_PIN = _MXC_BUILD_GPIO_PIN(2, 2, 17, 1, 0, 0),
	CSI_MCLK_PIN = _MXC_BUILD_GPIO_PIN(2, 21, 17, 2, 0, 0),
	IPU_D3_VSYNC_BE0_PIN = _MXC_BUILD_GPIO_PIN(2, 26, 17, 3, 0, 0),
	IPU_D3_HSYNC_BE1_PIN = _MXC_BUILD_GPIO_PIN(2, 27, 18, 0, 0, 0),
	IPU_LD16_D0_CS_PIN = _MXC_BUILD_GPIO_PIN(2, 28, 18, 1, 0, 0),
	IPU_D3_CONTR_PAR_RS_PIN = _MXC_BUILD_GPIO_PIN(2, 29, 18, 2, 0, 0),
	GP_AP_C3_PIN = _MXC_BUILD_GPIO_PIN(2, 3, 19, 3, 0, 0),
	IPU_D3_SPL_RD_PIN = _MXC_BUILD_GPIO_PIN(2, 30, 19, 0, 0, 0),
	IPU_D3_CLS_WR_PIN = _MXC_BUILD_GPIO_PIN(2, 31, 19, 1, 0, 0),
	GP_AP_C4_PIN = _MXC_BUILD_GPIO_PIN(2, 4, 19, 2, 0, 0),
	GP_AP_C5_PIN = _MXC_BUILD_GPIO_PIN(2, 5, 19, 3, 0, 0),
	GP_AP_C6_PIN = _MXC_BUILD_GPIO_PIN(2, 6, 20, 0, 0, 0),
	GP_AP_C7_PIN = _MXC_BUILD_GPIO_PIN(2, 7, 20, 1, 0, 0),
	GP_AP_C8_PIN = _MXC_BUILD_GPIO_PIN(2, 8, 20, 2, 0, 0),
	GP_AP_C9_PIN = _MXC_BUILD_GPIO_PIN(2, 9, 20, 3, 0, 0),
	I2CLK_PIN = _MXC_BUILD_GPIO_PIN(0, 23, 21, 0, 0, 0),
	I2DAT_PIN = _MXC_BUILD_GPIO_PIN(0, 24, 21, 1, 0, 0),
	IPU_LD0_PIN = _MXC_BUILD_GPIO_PIN(1, 10, 21, 2, 0, 0),
	IPU_LD1_PIN = _MXC_BUILD_GPIO_PIN(1, 11, 21, 3, 0, 0),
	IPU_LD10_PIN = _MXC_BUILD_GPIO_PIN(1, 20, 22, 0, 0, 0),
	IPU_LD11_PIN = _MXC_BUILD_GPIO_PIN(1, 21, 22, 1, 0, 0),
	IPU_LD12_PIN = _MXC_BUILD_GPIO_PIN(1, 22, 22, 2, 0, 0),
	IPU_LD13_PIN = _MXC_BUILD_GPIO_PIN(1, 23, 23, 3, 0, 0),
	IPU_LD14_PIN = _MXC_BUILD_GPIO_PIN(1, 24, 23, 0, 0, 0),
	IPU_LD15_PIN = _MXC_BUILD_GPIO_PIN(1, 25, 23, 1, 0, 0),
	IPU_LD2_PIN = _MXC_BUILD_GPIO_PIN(1, 12, 23, 2, 0, 0),
	IPU_LD3_PIN = _MXC_BUILD_GPIO_PIN(1, 13, 23, 3, 0, 0),
	IPU_LD4_PIN = _MXC_BUILD_GPIO_PIN(1, 14, 24, 0, 0, 0),
	IPU_LD5_PIN = _MXC_BUILD_GPIO_PIN(1, 15, 24, 1, 0, 0),
	IPU_LD6_PIN = _MXC_BUILD_GPIO_PIN(1, 16, 24, 2, 0, 0),
	IPU_LD7_PIN = _MXC_BUILD_GPIO_PIN(1, 17, 24, 3, 0, 0),
	IPU_LD8_PIN = _MXC_BUILD_GPIO_PIN(1, 18, 25, 0, 0, 0),
	IPU_LD9_PIN = _MXC_BUILD_GPIO_PIN(1, 19, 25, 1, 0, 0),
	KPCOL0_PIN = _MXC_BUILD_NON_GPIO_PIN(25, 2, 0, 0),
	KPCOL1_PIN = _MXC_BUILD_NON_GPIO_PIN(25, 3, 0, 0),
	KPCOL2_PIN = _MXC_BUILD_NON_GPIO_PIN(26, 0, 0, 0),
	KPCOL3_PIN = _MXC_BUILD_NON_GPIO_PIN(26, 1, 0, 0),
	KPCOL4_PIN = _MXC_BUILD_GPIO_PIN(0, 17, 26, 2, 0, 0),
	KPCOL5_PIN = _MXC_BUILD_GPIO_PIN(0, 18, 27, 3, 0, 0),
	KPCOL6_PIN = _MXC_BUILD_GPIO_PIN(0, 19, 27, 0, 0, 0),
	KPCOL7_PIN = _MXC_BUILD_GPIO_PIN(0, 20, 27, 1, 0, 0),
	KPROW0_PIN = _MXC_BUILD_NON_GPIO_PIN(27, 2, 0, 0),
	KPROW1_PIN = _MXC_BUILD_NON_GPIO_PIN(27, 3, 0, 0),
	KPROW2_PIN = _MXC_BUILD_NON_GPIO_PIN(28, 0, 0, 0),
	KPROW3_PIN = _MXC_BUILD_NON_GPIO_PIN(28, 1, 0, 0),
	KPROW4_PIN = _MXC_BUILD_GPIO_PIN(0, 13, 28, 2, 0, 0),
	KPROW5_PIN = _MXC_BUILD_GPIO_PIN(0, 14, 28, 3, 0, 0),
	KPROW6_PIN = _MXC_BUILD_GPIO_PIN(0, 15, 29, 0, 0, 0),
	KPROW7_PIN = _MXC_BUILD_GPIO_PIN(0, 16, 29, 1, 0, 0),
	OWDAT_PIN = _MXC_BUILD_GPIO_PIN(0, 25, 29, 2, 0, 0),
	RE_B_PIN = _MXC_BUILD_GPIO_PIN(0, 12, 29, 3, 0, 0),
	SPI1_CLK_PIN = _MXC_BUILD_GPIO_PIN(0, 9, 30, 0, 0, 0),
	SPI1_MISO_PIN = _MXC_BUILD_GPIO_PIN(0, 10, 30, 1, 0, 0),
	SPI1_MOSI_PIN = _MXC_BUILD_GPIO_PIN(0, 11, 30, 2, 0, 0),
	SPI1_SS0_PIN = _MXC_BUILD_GPIO_PIN(0, 12, 31, 3, 0, 0),
	WE_B_PIN = _MXC_BUILD_GPIO_PIN(0, 13, 31, 0, 0, 0),
	WP_B_PIN = _MXC_BUILD_GPIO_PIN(0, 14, 31, 1, 0, 0),

	/* IOMUX_COM pins */
	USB_DAT_VP_PIN = _MXC_BUILD_GPIO_PIN(0, 10, 0x80 + 0, 0, 0, 0),
	USB_SE0_VM_PIN = _MXC_BUILD_GPIO_PIN(0, 11, 0x80 + 0, 1, 0, 0),
	UH2_OVR_PIN = _MXC_BUILD_GPIO_PIN(0, 25, 0x80 + 0, 2, 0, 0),
	UH2_PWR_PIN = _MXC_BUILD_GPIO_PIN(0, 26, 0x80 + 0, 3, 0, 0),
	UH1_TXOE_B_PIN = _MXC_BUILD_GPIO_PIN(0, 27, 0x80 + 1, 0, 0, 0),
	UH1_SPEED_PIN = _MXC_BUILD_GPIO_PIN(0, 28, 0x80 + 1, 1, 0, 0),
	UH1_SUSPEND_PIN = _MXC_BUILD_GPIO_PIN(0, 29, 0x80 + 1, 2, 0, 0),
	UH1_TXDP_PIN = _MXC_BUILD_GPIO_PIN(0, 30, 0x80 + 1, 3, 0, 0),
	UH1_TXDM_PIN = _MXC_BUILD_GPIO_PIN(0, 31, 0x80 + 2, 0, 0, 0),
	USB_TXOE_B_PIN = _MXC_BUILD_GPIO_PIN(0, 9, 0x80 + 2, 1, 0, 0),
	UH1_RXD_PIN = _MXC_BUILD_GPIO_PIN(1, 0, 0x80 + 2, 2, 0, 0),
	UH1_RXDP_PIN = _MXC_BUILD_GPIO_PIN(1, 1, 0x80 + 3, 3, 0, 0),
	SD1_CMD_PIN = _MXC_BUILD_GPIO_PIN(1, 14, 0x80 + 3, 0, 0, 0),
	SD1_CLK_PIN = _MXC_BUILD_GPIO_PIN(1, 15, 0x80 + 3, 1, 0, 0),
	SD1_MMC_PU_CTRL_PIN = _MXC_BUILD_GPIO_PIN(1, 16, 0x80 + 3, 2, 0, 0),
	UH1_RXDM_PIN = _MXC_BUILD_GPIO_PIN(1, 2, 0x80 + 3, 3, 0, 0),
	SD2_CMD_PIN = _MXC_BUILD_GPIO_PIN(1, 21, 0x80 + 4, 0, 0, 0),
	SD2_CLK_PIN = _MXC_BUILD_GPIO_PIN(1, 22, 0x80 + 4, 1, 0, 0),
	SD2_MMC_PU_CTRL_PIN = _MXC_BUILD_GPIO_PIN(1, 23, 0x80 + 4, 2, 0, 0),
	SPI2_CLK_PIN = _MXC_BUILD_GPIO_PIN(1, 26, 0x80 + 4, 3, 0, 0),
	SPI2_MOSI_PIN = _MXC_BUILD_GPIO_PIN(1, 27, 0x80 + 5, 0, 0, 0),
	SPI2_MISO_PIN = _MXC_BUILD_GPIO_PIN(1, 28, 0x80 + 5, 1, 0, 0),
	SPI2_SS0_PIN = _MXC_BUILD_GPIO_PIN(1, 29, 0x80 + 5, 2, 0, 0),
	UH1_OVR_PIN = _MXC_BUILD_GPIO_PIN(1, 3, 0x80 + 5, 3, 0, 0),
	SPI2_SS1_PIN = _MXC_BUILD_GPIO_PIN(1, 30, 0x80 + 6, 0, 0, 0),
	UH1_PWR_PIN = _MXC_BUILD_GPIO_PIN(1, 4, 0x80 + 6, 1, 0, 0),
	U3CE_DSR_B_PIN = _MXC_BUILD_GPIO_PIN(1, 6, 0x80 + 6, 2, 0, 0),
	U3CE_RI_B_PIN = _MXC_BUILD_GPIO_PIN(1, 7, 0x80 + 7, 3, 0, 0),
	U3CE_DCD_B_PIN = _MXC_BUILD_GPIO_PIN(1, 8, 0x80 + 7, 0, 0, 0),
	U3CE_DTR_B_PIN = _MXC_BUILD_GPIO_PIN(1, 9, 0x80 + 7, 1, 0, 0),
	MDO0_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 7, 2, 0, 0),
	MDO1_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 7, 3, 0, 0),
	MDO10_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 8, 0, 0, 0),
	MDO11_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 8, 1, 0, 0),
	MDO12_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 8, 2, 0, 0),
	MDO13_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 8, 3, 0, 0),
	MDO14_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 9, 0, 0, 0),
	MDO15_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 9, 1, 0, 0),
	MDO2_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 9, 2, 0, 0),
	MDO3_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 9, 3, 0, 0),
	MDO4_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 10, 0, 0, 0),
	MDO5_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 10, 1, 0, 0),
	MDO6_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 10, 2, 0, 0),
	MDO7_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 10, 3, 0, 0),
	MDO8_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 11, 0, 0, 0),
	MDO9_PIN = _MXC_BUILD_NON_GPIO_PIN(0x80 + 11, 1, 0, 0),
	SD1_DAT0_PIN = _MXC_BUILD_GPIO_PIN(1, 10, 0x80 + 11, 2, 0, 0),
	SD1_DAT1_PIN = _MXC_BUILD_GPIO_PIN(1, 11, 0x80 + 11, 3, 0, 0),
	SD1_DAT2_PIN = _MXC_BUILD_GPIO_PIN(1, 12, 0x80 + 12, 0, 0, 0),
	SD1_DAT3_PIN = _MXC_BUILD_GPIO_PIN(1, 13, 0x80 + 12, 1, 0, 0),
	SD2_DAT0_PIN = _MXC_BUILD_GPIO_PIN(1, 17, 0x80 + 12, 2, 0, 0),
	SD2_DAT1_PIN = _MXC_BUILD_GPIO_PIN(1, 18, 0x80 + 12, 3, 0, 0),
	SD2_DAT2_PIN = _MXC_BUILD_GPIO_PIN(1, 19, 0x80 + 13, 0, 0, 0),
	SD2_DAT3_PIN = _MXC_BUILD_GPIO_PIN(1, 20, 0x80 + 13, 1, 0, 0),
	SIM_CLK_PIN = _MXC_BUILD_GPIO_PIN(1, 28, 0x80 + 13, 2, 0, 0),
	SIM_PD_PIN = _MXC_BUILD_GPIO_PIN(1, 31, 0x80 + 13, 3, 0, 0),
	SIM_RST_B_PIN = _MXC_BUILD_GPIO_PIN(1, 26, 0x80 + 14, 0, 0, 0),
	SIM_SVEN_PIN = _MXC_BUILD_GPIO_PIN(1, 27, 0x80 + 14, 1, 0, 0),
	SIM_TRXD_PIN = _MXC_BUILD_GPIO_PIN(1, 29, 0x80 + 14, 2, 0, 0),
	U3CE_CTS_B_PIN = _MXC_BUILD_GPIO_PIN(1, 5, 0x80 + 15, 3, 0, 0),
	USB_RXD_PIN = _MXC_BUILD_GPIO_PIN(0, 12, 0x80 + 15, 0, 0, 0),

	AP_MAX_PIN = WP_B_PIN,
} iomux_pin_name_t;

#endif
#endif
