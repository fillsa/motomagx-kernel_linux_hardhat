/*
 * Copyright 2006 Freescale Semiconductor, Inc.
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under the	terms of the GNU General Public	License	as published by
 * the Free Software Foundation; either	version	2 of the License, or
 * (at your option) any	later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59	Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */
#ifndef __ARCH_ARM_MACH_MXC91131_CRM_REGS_H__
#define __ARCH_ARM_MACH_MXC91131_CRM_REGS_H__

#define MXC_CRM_AP_BASE				(IO_ADDRESS(CRM_AP_BASE_ADDR))

#define MXC_CRM_COM_BASE			(IO_ADDRESS(CRM_COM_BASE_ADDR))

/* CRM_AP Register Offsets */
#define MXC_CRMAP_ASCSR				(MXC_CRM_AP_BASE + 0x00)
#define MXC_CRMAP_ACDR				(MXC_CRM_AP_BASE + 0x04)
#define MXC_CRMAP_ACDER1			(MXC_CRM_AP_BASE + 0x08)
#define MXC_CRMAP_ACDER2			(MXC_CRM_AP_BASE + 0x0C)
#define MXC_CRMAP_ACGCR				(MXC_CRM_AP_BASE + 0x10)
#define MXC_CRMAP_ACCGCR			(MXC_CRM_AP_BASE + 0x14)
#define MXC_CRMAP_AMLPMRA			(MXC_CRM_AP_BASE + 0x18)
#define MXC_CRMAP_AMLPMRB			(MXC_CRM_AP_BASE + 0x1C)
#define MXC_CRMAP_AMLPMRC			(MXC_CRM_AP_BASE + 0x20)
#define MXC_CRMAP_AMLPMRD			(MXC_CRM_AP_BASE + 0x24)
#define MXC_CRMAP_AMLPMRE1			(MXC_CRM_AP_BASE + 0x28)
#define MXC_CRMAP_AMLPMRE2			(MXC_CRM_AP_BASE + 0x2C)
#define MXC_CRMAP_AMLPMRF			(MXC_CRM_AP_BASE + 0x30)
#define MXC_CRMAP_AMLPMRG			(MXC_CRM_AP_BASE + 0x34)
#define MXC_CRMAP_APGCR				(MXC_CRM_AP_BASE + 0x38)
#define MXC_CRMAP_ACSR				(MXC_CRM_AP_BASE + 0x3C)
#define MXC_CRMAP_ADCR				(MXC_CRM_AP_BASE + 0x40)
#define MXC_CRMAP_ACR				(MXC_CRM_AP_BASE + 0x44)
#define MXC_CRMAP_AMCR				(MXC_CRM_AP_BASE + 0x48)
#define MXC_CRMAP_APCR				(MXC_CRM_AP_BASE + 0x4C)
#define MXC_CRMAP_AMORA				(MXC_CRM_AP_BASE + 0x50)
#define MXC_CRMAP_AMORB				(MXC_CRM_AP_BASE + 0x54)
#define MXC_CRMAP_AGPR				(MXC_CRM_AP_BASE + 0x58)
#define MXC_CRMAP_APRA				(MXC_CRM_AP_BASE + 0x5C)
#define MXC_CRMAP_APRB				(MXC_CRM_AP_BASE + 0x60)
#define MXC_CRMAP_APOR				(MXC_CRM_AP_BASE + 0x64)

/* CRM_COM Register Offsets */
#define MXC_CRMCOM_CBMR				(MXC_CRM_COM_BASE + 0x00)
#define MXC_CRMCOM_CRSRBP			(MXC_CRM_COM_BASE + 0x04)
#define MXC_CRMCOM_CCRCR			(MXC_CRM_COM_BASE + 0x08)
#define MXC_CRMCOM_CSCR				(MXC_CRM_COM_BASE + 0x0C)
#define MXC_CRMCOM_CCCR				(MXC_CRM_COM_BASE + 0x10)
#define MXC_CRMCOM_CRSRAP			(MXC_CRM_COM_BASE + 0x14)

/* DPLL	Register Offsets */
#define DPLL_DP_CTL				0x00
#define DPLL_DP_CONFIG				0x04
#define DPLL_DP_OP				0x08
#define DPLL_DP_MFD				0x0C
#define DPLL_DP_MFN				0x10
#define DPLL_DP_MFNMINUS			0x14
#define DPLL_DP_MFNPLUS				0x18
#define DPLL_DP_HFS_OP				0x1C
#define DPLL_DP_HFS_MFD				0x20
#define DPLL_DP_HFS_MFN				0x24
#define DPLL_DP_TOGC				0x28
#define DPLL_DP_DESTAT				0x2C

/*
 * Bit definitions of ASCSR, ACDR in CRM_AP and	CSCR in	CRM_COM
 */
#define MXC_CRMAP_ASCSR_APSEL			0x00000018
#define MXC_CRMAP_ASCSR_APSEL0			0x00000008
#define MXC_CRMAP_ASCSR_APSEL1			0x00000010
#define MXC_CRMAP_CSCR_CKOH_SEL			0x00040000
#define MXC_CRMAP_CSCR_CKO_SEL			0x00020000
#define MXC_CRMAP_ACDR_ARM_DIV_MASK		0x00000F00
#define MXC_CRMAP_ACDR_ARM_DIV_OFFSET		8
#define MXC_CRMAP_ACDR_AHB_DIV_MASK		0x000000F0
#define MXC_CRMAP_ACDR_AHB_DIV_OFFSET		4
#define MXC_CRMAP_ACDR_IP_DIV_MASK		0x0000000F
#define MXC_CRMAP_ACDR_IP_DIV_OFFSET		0

/*
 * Bit definitions of ADCR in CRM_AP
 */
#define MXC_CRMAP_ADCR_ALT_PLL			0x00000080
#define MXC_CRMAP_ADCR_DIV_BYP			0x00000002
#define MXC_CRMAP_ADCR_DFS_DIV_EN		0x00000020
#define MXC_CRMAP_ADCR_LFDF_LSB			0x00000100
#define MXC_CRMAP_ADCR_LFDF_MSB			0x00000200
#define MXC_CRMAP_ADCR_LFDF_SHIFT		8
#define MXC_CRMAP_ADCR_LFDF_MASK		0x00000300
#define MXC_CRMAP_ADCR_AP_DELAY			0x00000000
#define MXC_CRMAP_ADCR_VSTAT			0x00000008
#define MXC_CRMAP_ADCR_TSTAT			0x00000010
#define MXC_CRMAP_ADCR_CLK_ON			0x00000040

/*
 * Bit definitions of ACR in CRM_AP
 */
#define MXC_CRMAP_ACR_CKOHD			(1<<15)
#define MXC_CRMAP_ACR_CKOHS2			(1<<14)
#define MXC_CRMAP_ACR_CKOHS1			(1<<13)
#define MXC_CRMAP_ACR_CKOHS0			(1<<12)
#define MXC_CRMAP_ACR_CKOHS_MASK		(ACR_CKOHS2|ACR_CKOHS1|ACR_CKOHS0)

#define MXC_CRMAP_ACR_CKOHDIV3			(1<<11)
#define MXC_CRMAP_ACR_CKOHDIV2			(1<<10)
#define MXC_CRMAP_ACR_CKOHDIV1			(1<<9)
#define MXC_CRMAP_ACR_CKOHDIV0			(1<<8)
#define MXC_CRMAP_ACR_CKOHDIV_MASK		(0xF<<8)

#define MXC_CRMAP_ACR_CKOD			(1<<7)
#define MXC_CRMAP_ACR_CKOS2			(1<<6)
#define MXC_CRMAP_ACR_CKOS1			(1<<5)
#define MXC_CRMAP_ACR_CKOS0			(1<<4)

/*
 * Bit definitions of ACGCR in CRM_AP
 */
#define ACG0_STOP_WAIT				0x00000001
#define ACG0_STOP				0x00000003
#define ACG0_RUN				0x00000007
#define ACG0_DISABLED				0x00000000

#define ACG1_STOP_WAIT				0x00000008
#define ACG1_STOP				0x00000018
#define ACG1_RUN				0x00000038
#define ACG1_DISABLED				0x00000000

#define ACG2_STOP_WAIT				0x00000040
#define ACG2_STOP				0x000000C0
#define ACG2_RUN				0x000001C0
#define ACG2_DISABLED				0x00000000

#define ACG3_STOP_WAIT				0x00000200
#define ACG3_STOP				0x00000600
#define ACG3_RUN				0x00000E00
#define ACG3_DISABLED				0x00000000

#define ACG4_STOP_WAIT				0x00001000
#define ACG4_STOP				0x00003000
#define ACG4_RUN				0x00007000
#define ACG4_DISABLED				0x00000000

#define ACG5_STOP_WAIT				0x00010000
#define ACG5_STOP				0x00030000
#define ACG5_RUN				0x00070000
#define ACG5_DISABLED				0x00000000

#define ACG6_STOP_WAIT				0x00080000
#define ACG6_STOP				0x00180000
#define ACG6_RUN				0x00380000
#define ACG6_DISABLED				0x00000000

#define MXC_CRMCOM_CSCR_NF_WIDTH		0x00080000UL

/*
 * CRM AP Register Bit Field Positions (left shift)
 */
#define ASCSR_AP_ISEL_BFS			0
#define ASCSR_APSEL_BFS				3
#define ASCSR_SSI1SEL_BFS			5
#define ASCSR_SSI2SEL_BFS			7
#define ASCSR_FIRISEL_BFS			9
#define ASCSR_CSSEL_BFS				11
#define ASCSR_USBSEL_BFS			13
#define ASCSR_AP_PAT_REF_DIV_BFS		15
#define ASCSR_CRS_BFS				16

#define ACDR_IP_DIV_BFS				0
#define ACDR_AHB_DIV_BFS			4
#define ACDR_ARM_DIV_BFS			8

#define ACDER1_SSI1_DIV_BFS			0
#define ACDER1_SSI1_EN_BFS			6
#define ACDER1_SSI2_DIV_BFS			8
#define ACDER1_SSI2_EN_BFS			14
#define ACDER1_FIRI_DIV_BFS			16
#define ACDER1_FIRI_EN_BFS			22
#define ACDER1_CS_DIV_BFS			24
#define ACDER1_CS_EN_BFS			30

#define ACDER2_BAUD_DIV_BFS			0
#define ACDER2_BAUD_ISEL_BFS			5
#define ACDER2_USB_DIV_BFS			8
#define ACDER2_USB_EN_BFS			12
#define ACDER2_NFC_DIV_BFS			16
#define ACDER2_NFC_EN_BFS			20

#define ACGCR_ACG0_BFS				0
#define ACGCR_ACG1_BFS				3
#define ACGCR_ACG2_BFS				6
#define ACGCR_ACG3_BFS				9
#define ACGCR_ACG4_BFS				12

#define ACCGCR_ACCG0_BFS			0
#define ACCGCR_ACCG1_BFS			3
#define ACCGCR_ACCG2_BFS			6

/* Field definitions apply to all AMLPRMx */
#define AMLPMR_MLPM0_BFS			0
#define AMLPMR_MLPM1_BFS			3
#define AMLPMR_MLPM2_BFS			6
#define AMLPMR_MLPM4_BFS			12
#define AMLPMR_MLPM5_BFS			16
#define AMLPMR_MLPM6_BFS			19
#define AMLPMR_MLPM7_BFS			22

#define APGCR_PG0_BFS				0
#define APGCR_PG1_BFS				2

#define ACSR_ACS_BFS				0
#define ACSR_WPS_BFS				1
#define ACSR_PDS_BFS				2
#define ACSR_SMD_BFS				3
#define ACSR_DI_BFS				7
#define ACSR_ADS_BFS				8

#define ADCR_DIV_BYP_BFS			1
#define ADCR_VSTAT_BFS				3
#define ADCR_TSTAT_BFS				4
#define ADCR_DFS_DIV_EN_BFS			5
#define ADCR_CLK_ON_BFS				6
#define ADCR_ALT_PLL_BFS			7
#define ADCR_LFDF_BFS				8
#define ADCR_AP_DELAY_BFS			16

#define ACR_CKOS_BFS				4
#define ACR_CKOD_BFS				7
#define ACR_CKOHDIV_BFS				8
#define ACR_CKOHS_BFS				12
#define ACR_CKOHD_BFS				15

#define AMCR_WB_EN_BFS				0
#define AMCR_FA_BFS				3
#define AMCR_SPA_BFS				4
#define AMCR_L2C_SAFE_EN_BFS			8
#define AMCR_L2C_PG_BFS				9
#define AMCR_SW_AP_BFS				14
#define AMCR_CSB_BFS				15
#define AMCR_SDMA_ACK_BYP_BFS			16
#define AMCR_IPU_ACK_BYP_BFS			17
#define AMCR_MAX_ACK_BYP_BFS			18
#define AMCR_USBOTG_ACK_BYP_BFS			20
#define AMCR_IPU_REQ_DIS_BFS			22

#define APCR_PC_BFS				0

#define AMORA_MOA_BFS				0

#define AMORB_MOB_BFS				0

#define AGPR_GPB_BFS				0

#define APRA_UART1_EN_BFS			0
#define APRA_UART2_EN_BFS			8
#define APRA_UART3_EN_BFS			16
#define APRA_UART3_DIV_BFS			17
#define APRA_SIM_EN_BFS				18

#define APRB_SDHC1_EN_BFS			0
#define APRB_SDHC1_DIV_BFS			1
#define APRB_SDHC1_ISEL_BFS			5
#define APRB_SDHC2_EN_BFS			8
#define APRB_SDHC2_DIV_BFS			9
#define APRB_SDHC2_ISEL_BFS			13
#define APRB_ECT_EN_BFS				16

#define APOR_POB_BFS				0

/*
 * CRM AP Register Bit Field Widths
 */
#define ASCSR_AP_ISEL_BFW			2
#define ASCSR_APSEL_BFW				2
#define ASCSR_SSI1SEL_BFW			2
#define ASCSR_SSI2SEL_BFW			2
#define ASCSR_FIRISEL_BFW			2
#define ASCSR_CSSEL_BFW				2
#define ASCSR_USBSEL_BFW			2
#define ASCSR_AP_PAT_REF_DIV_BFW		1
#define ASCSR_CRS_BFW				1

#define ACDR_IP_DIV_BFW				4
#define ACDR_AHB_DIV_BFW			4
#define ACDR_ARM_DIV_BFW			4

#define ACDER1_SSI1_DIV_BFW			6
#define ACDER1_SSI1_EN_BFW			1
#define ACDER1_SSI2_DIV_BFW			6
#define ACDER1_SSI2_EN_BFW			1
#define ACDER1_FIRI_DIV_BFW			5
#define ACDER1_FIRI_EN_BFW			1
#define ACDER1_CS_DIV_BFW			6
#define ACDER1_CS_EN_BFW			1

#define ACDER2_BAUD_DIV_BFW			4
#define ACDER2_BAUD_ISEL_BFW			2
#define ACDER2_USB_DIV_BFW			4
#define ACDER2_USB_EN_BFW			1
#define ACDER2_NFC_DIV_BFW			4
#define ACDER2_NFC_EN_BFW			1

#define ACGCR_ACG0_BFW				3
#define ACGCR_ACG1_BFW				3
#define ACGCR_ACG2_BFW				3
#define ACGCR_ACG3_BFW				3
#define ACGCR_ACG4_BFW				3

#define ACCGCR_ACCG0_BFW			3
#define ACCGCR_ACCG1_BFW			3
#define ACCGCR_ACCG2_BFW			3

/* Field definitions apply to all AMLPRMx */
#define AMLPMR_MLPM0_BFW			3
#define AMLPMR_MLPM1_BFW			3
#define AMLPMR_MLPM2_BFW			3
#define AMLPMR_MLPM3_BFW			3
#define AMLPMR_MLPM4_BFW			3
#define AMLPMR_MLPM5_BFW			3
#define AMLPMR_MLPM6_BFW			3
#define AMLPMR_MLPM7_BFW			3

#define APGCR_PG0_BFW				2
#define APGCR_PG1_BFW				2

#define ACSR_ACS_BFW				1
#define ACSR_WPS_BFW				1
#define ACSR_PDS_BFW				1
#define ACSR_SMD_BFW				1
#define ACSR_DI_BFW				1
#define ACSR_ADS_BFW				1

#define ADCR_DIV_BYP_BFW			1
#define ADCR_VSTAT_BFW				1
#define ADCR_TSTAT_BFW				1
#define ADCR_DFS_DIV_EN_BFW			1
#define ADCR_CLK_ON_BFW				1
#define ADCR_ALT_PLL_BFW			1
#define ADCR_LFDF_BFW				2
#define ADCR_AP_DELAY_BFW			16

#define ACR_CKOS_BFW				3
#define ACR_CKOD_BFW				1
#define ACR_CKODIV_BFW				4
#define ACR_CKOHS_BFW				3
#define ACR_CKOHD_BFW				1

#define AMCR_WB_EN_BFW				1
#define AMCR_FA_BFW				1
#define AMCR_SPA_BFW				2
#define AMCR_L2C_SAFE_EN_BFW			1
#define AMCR_L2C_PG_BFW				1
#define AMCR_SW_AP_BFW				1
#define AMCR_CSB_BFW				1
#define AMCR_SDMA_ACK_BYP_BFW			1
#define AMCR_IPU_ACK_BYP_BFW			1
#define AMCR_MAX_ACK_BYP_BFW			1
#define AMCR_USBOTG_ACK_BYP_BFW			1
#define AMCR_IPU_REQ_DIS_BFW			1

#define APCR_PC_BFW				32

#define AMORA_MOA_BFW				32

#define AMORB_MOB_BFW				28

#define AGPR_GPB_BFW				32

#define APRA_UART1_EN_BFW			1
#define APRA_UART2_EN_BFW			1
#define APRA_UART3_EN_BFW			1
#define APRA_UART3_DIV_BFW			4
#define APRA_SIM_EN_BFW				1

#define APRB_SDHC1_EN_BFW			1
#define APRB_SDHC1_DIV_BFW			4
#define APRB_SDHC1_ISEL_BFW			3
#define APRB_SDHC2_EN_BFW			1
#define APRB_SDHC2_DIV_BFW			4
#define APRB_SDHC2_ISEL_BFW			3
#define APRB_ECT_EN_BFW				1

#define APOR_POB_BFW				12

/*
 * CRM AP Register Bit Field Values
 */
#define ASCSR_AP_ISEL_CKIH			0
#define ASCSR_AP_ISEL_DIGRF			1

#define ASCSR_APSEL_ADPLL			0
#define ASCSR_APSEL_BDPLL			1
#define ASCSR_APSEL_UDPLL			2
#define ASCSR_APSEL_MRCG0			3

#define ASCSR_SSI1SEL_ADPLL			0
#define ASCSR_SSI1SEL_BDPLL			1
#define ASCSR_SSI1SEL_UDPLL			2
#define ASCSR_SSI1SEL_MRCG2			3

#define ASCSR_SSI2SEL_ADPLL			0
#define ASCSR_SSI2SEL_BDPLL			1
#define ASCSR_SSI2SEL_UDPLL			2
#define ASCSR_SSI2SEL_MRCG2			3

#define ASCSR_FIRISEL_ADPLL			0
#define ASCSR_FIRISEL_BDPLL			1
#define ASCSR_FIRISEL_UDPLL			2
#define ASCSR_FIRISEL_MRCG2			3

#define ASCSR_CSSEL_ADPLL			0
#define ASCSR_CSSEL_BDPLL			1
#define ASCSR_CSSEL_UDPLL			2
#define ASCSR_CSSEL_MRCG0			3

#define ASCSR_USBSEL_ADPLL			0
#define ASCSR_USBSEL_BDPLL			1
#define ASCSR_USBSEL_UDPLL			2
#define ASCSR_USBSEL_MRCG2			3

#define ASCSR_AP_PAT_REF_DIV_1			0
#define ASCSR_AP_PAT_REF_DIV_2			1

#define ASCSR_CRS_UNCORRECTED			0
#define ASCSR_CRS_CORRECTED			1

#define ACDR_ARM_DIV_1				0x8
#define ACDR_ARM_DIV_2				0x0
#define ACDR_ARM_DIV_3				0x1
#define ACDR_ARM_DIV_4				0x2
#define ACDR_ARM_DIV_5				0x3
#define ACDR_ARM_DIV_6				0x4
#define ACDR_ARM_DIV_8				0x5
#define ACDR_ARM_DIV_10				0x6
#define ACDR_ARM_DIV_12				0x7

#define ACDER2_BAUD_ISEL_CKIH			0x0
#define ACDER2_BAUD_ISEL_CKIH_X2		0x1
#define ACDER2_BAUD_ISEL_DIGRF			0x2
#define ACDER2_BAUD_ISEL_DIGRF_X2		0x3

#define ACDER2_BAUD_DIV_1			0x8
#define ACDER2_BAUD_DIV_2			0x0
#define ACDER2_BAUD_DIV_3			0x1
#define ACDER2_BAUD_DIV_4			0x2
#define ACDER2_BAUD_DIV_5			0x3
#define ACDER2_BAUD_DIV_6			0x4
#define ACDER2_BAUD_DIV_8			0x5
#define ACDER2_BAUD_DIV_10			0x6
#define ACDER2_BAUD_DIV_12			0x7

#define ACGCR_ACG_ALWAYS_ON			0x0
#define ACGCR_ACG_ON_RUN			0x1
#define ACGCR_ACG_ON_RUN_8			0x2
#define ACGCR_ACG_ON_RUN_WAIT			0x3
#define ACGCR_ACG_ALWAYS_OFF			0x4

#define APGCR_PGC_ALWAYS_OFF			0x0
#define APGCR_PGC_ON_RUN			0x1
#define APGCR_PGC_ON_RUN_WAIT			0x2
#define APGCR_PGC_ALWAYS_ON			0x3

#define AMLPMR_MLPM_ALWAYS_OFF			0x0
#define AMLPMR_MLPM_ON_RUN			0x1
#define AMLPMR_MLPM_ON_RUN_8			0x2
#define AMLPMR_MLPM_ON_RUN_WAIT			0x3
#define AMLPMR_MLPM_ALWAYS_ON			0x4

#define ACSR_ACS_REF_CORE_CLK			0x0
#define ACSR_ACS_PLL_CLK			0x1

#define ACSR_WPS_PLL_CLK			0x0
#define ACSR_WPS_PAT_REF			0x1

#define ACSR_PDS_PLL_ON_IN_STOP			0x0
#define ACSR_PDS_PLL_OFF_IN_STOP		0x1

#define ACSR_SMD_SYNC_MUX_ENABLE		0x0
#define ACSR_SMD_SYNC_MUX_DISABLE		0x1

#define ACSR_DI_IGNORE_DSM_INT			0x0
#define ACSR_DI_USE_DSM_INT			0x1

#define ACSR_ADS_NO_DOUBLER			0x0
#define ACSR_ADS_DOUBLER			0x1

#define ADCR_DIV_BYP_USED			0x0
#define ADCR_DIV_BYP_NOT_USED			0x1

#define ADCR_VSTAT_HIGH_VOLTAGE			0x0
#define ADCR_VSTAT_LOW_VOLTAGE			0x1

#define ADCR_TSTAT_TIMED_OUT			0x0
#define ADCR_TSTAT_RUNNING			0x1

#define ADCR_DFS_DIV_EN_NON_INT_DIV		0x0
#define ADCR_DFS_DIV_EN_INT_DIV			0x1

#define ADCR_CLK_ON_DVFS_NO_CLOCK		0x0
#define ADCR_CLK_ON_DVFS_PAT_REF		0x1

#define ADCR_ALT_PLL_NO_ALT			0x0
#define ADCR_ALT_PLL_USE_ALT			0x1

#define ADCR_LFDF_NO_DIV			0x0
#define ADCR_LFDF_DIV2				0x1
#define ADCR_LFDF_DIV4				0x2
#define ADCR_LFDF_DIV8				0x8

#define ACR_CKOS_CKIL				0x0
#define ACR_CKOS_AP_PAT_REF_CLK			0x1
#define ACR_CKOS_AP_REF_X2_CLK			0x2
#define ACR_CKOS_SSI1_CLK			0x3
#define ACR_CKOS_SSI2_CLK			0x4
#define ACR_CKOS_CS_CLK				0x5
#define ACR_CKOS_FIRI_CLK			0x6
#define ACR_CKOS_AP_UC_PAT_REF_CLK		0x7

#define ACR_CKOD_CKO_ENABLE			0x0
#define ACR_CKOD_CKO_DISABLE			0x1

#define ACR_CKOHDIV_1				0x8
#define ACR_CKOHDIV_2				0x0
#define ACR_CKOHDIV_3				0x1
#define ACR_CKOHDIV_4				0x2
#define ACR_CKOHDIV_5				0x3
#define ACR_CKOHDIV_6				0x4
#define ACR_CKOHDIV_8				0x5
#define ACR_CKOHDIV_10				0x6
#define ACR_CKOHDIV_12				0x7
#define ACR_CKOHDIV_OFFSET			8

#define ACR_CKOHS_AP_UC_PAT_REF_CLK		0x0
#define ACR_CKOHS_AP_CLK			0x1
#define ACR_CKOHS_AP_AHB_CLK			0x2
#define ACR_CKOHS_AP_PCLK			0x3
#define ACR_CKOHS_USB_CLK			0x4
#define ACR_CKOHS_AP_PERCLK			0x5
#define ACR_CKOHS_AP_CKIL_CLK			0x6
#define ACR_CKOHS_AP_PAT_REF_CLK		0x7
#define ACR_CKOHS_OFFSET			12

#define ACR_CKOHD_CKOH_ENABLED			0x0
#define ACR_CKOHD_CKOH_DISABLED			0x1

#define ACR_CKOHD				(1<<15)
#define ACR_CKOHS2				(1<<14)
#define ACR_CKOHS1				(1<<13)
#define ACR_CKOHS0				(1<<12)
#define ACR_CKOHS_MASK				(ACR_CKOHS2|ACR_CKOHS1|ACR_CKOHS0)

#define ACR_CKOHDIV3				(1<<11)
#define ACR_CKOHDIV2				(1<<10)
#define ACR_CKOHDIV1				(1<<9)
#define ACR_CKOHDIV0				(1<<8)
#define ACR_CKOHDIV_MASK			(0xF<<8)

#define ACR_CKOD				(1<<7)
#define ACR_CKOS2				(1<<6)
#define ACR_CKOS1				(1<<5)
#define ACR_CKOS0				(1<<4)
#define ACR_CKOS_MASK				(ACR_CKOS2|ACR_CKOS1|ACR_CKOS0)

#define AMCR_WB_EN_WELL_BIAS_OFF		0x0
#define AMCR_WB_EN_WELL_BIAS_ON			0x1

#define AMCR_FA_SLOW				0x0
#define AMCR_FA_NOMINAL				0x1

#define AMCR_L2C_SAFE_EN_UNSAFE			0
#define AMCR_L2C_SAFE_EN_SAFE			1

#define AMCR_L2C_PG_NO_PWR_GATE			0x0
#define AMCR_L2C_PG_PWR_GATE			0x1

#define AMCR_SW_AP_WARM_RESET			0x0
#define AMCR_SW_AP_NO_RESET			0x1

#define AMCR_CSB_CKIL_SYNC			0x0
#define AMCR_CSB_NO_CKIL_SYNC			0x1

#define AMCR_SDMA_ACK_BYP_WAIT			0
#define AMCR_SDMA_ACK_BYP_NO_WAIT		1

#define AMCR_IPU_ACK_BYP_WAIT			0
#define AMCR_IPU_ACK_BYP_NO_WAIT		1

#define AMCR_MAX_ACK_BYP_WAIT			0
#define AMCR_MAX_ACK_BYP_NO_WAIT		1

#define AMCR_USBOTG_ACK_BYP_WAIT		0
#define AMCR_USBOTG_ACK_BYP_NO_WAIT		1

#define AMCR_IPU_REQ_DIS_REQUEST		0
#define AMCR_IPU_REQ_DIS_NO_REQUEST		1

#define APRB_SDHC_ISEL_CKIH			0
#define APRB_SDHC_ISEL_CKIH_X2			1
#define APRB_SDHC_ISEL_DIGRF			2
#define APRB_SDHC_ISEL_DIGRF_X2			3
#define APRB_SDHC_ISEL_USB_CLK			4
#define APRB_SDHC_ISEL_MRCG2_CLK		5

#define CSCR_CKOHSEL				(1<<18)
#define CSCR_CKOSEL				(1<<17)

/*
 * DPLL	Register Bit Field Positions (left shift)
 */
#define DP_CTL_LRF_BFS				0
#define DP_CTL_BRMO_BFS				1
#define DP_CTL_PLM_BFS				2
#define DP_CTL_RCP_BFS				3
#define DP_CTL_RST_BFS				4
#define DP_CTL_UPEN_BFS				5
#define DP_CTL_PRE_BFS				6
#define DP_CTL_HFSM_BFS				7
#define DP_CTL_REF_CLK_SEL_BFS			8
#define DP_CTL_REF_CLK_DIV_BFS			10
#define DP_CTL_ADE_BFS				11
#define DP_CTL_DPDCK0_2_EN_BFS			12

#define DP_CONFIG_LDREQ_BFS			0
#define DP_CONFIG_AREN_BFS			1
#define DP_CONFIG_BIST_CE_BFS			3

#define DP_OP_PDF_BFS				0
#define DP_OP_MFI_BFS				4

#define DP_MFD_MFD_BFS				0

#define DP_MFN_MFN_BFS				0

#define DP_TOGC_TOF_MFN_CNT_BFS			0
#define DP_TOGC_TOG_EN_BFS			16
#define DP_TOGC_TOG_DIS_BFS			17

#define DP_DESTAT_TOG_MFN_BFS			0
#define DP_DESTAT_TOG_SEL_BFS			31

/*
 * REGISTER BIT	FIELD WIDTHS
 */
#define DP_CTL_LRF_BFW				1
#define DP_CTL_BRMO_BFW				1
#define DP_CTL_PLM_BFW				1
#define DP_CTL_RCP_BFW				1
#define DP_CTL_RST_BFW				1
#define DP_CTL_UPEN_BFW				1
#define DP_CTL_PRE_BFW				1
#define DP_CTL_HFSM_BFW				1
#define DP_CTL_REF_CLK_SEL_BFW			2
#define DP_CTL_REF_CLK_DIV_BFW			1
#define DP_CTL_ADE_BFW				1
#define DP_CTL_DPDCK0_2_EN_BFW			1

#define DP_CONFIG_LDREQ_BFW			1
#define DP_CONFIG_AREN_BFW			1
#define DP_CONFIG_BIST_CE_BFW			1

#define DP_OP_PDF_BFW				4
#define DP_OP_MFI_BFW				4

#define DP_MFD_MFD_BFW				27

#define DP_MFN_MFN_BFW				27

#define DP_TOGC_TOF_MFN_CNT_BFW			16
#define DP_TOGC_TOG_EN_BFW			1
#define DP_TOGC_TOG_DIS_BFW			1

#define DP_DESTAT_TOG_MFN_BFW			27
#define DP_DESTAT_TOG_SEL_BFW			1

/*
 * REGISTER BIT	WRITE VALUES
 */
#define DP_CTL_LRF_LOCKED			0x0
#define DP_CTL_LRF_UNLOCKED			0x1

#define DP_CTL_BRMO_FIRST_ORDER			0x0
#define DP_CTL_BRMO_SECOND_ORDER		0x1

#define DP_CTL_PLM_FREQ_ONLY			0x0
#define DP_CTL_PLM_FREQ_PHASE			0x1

#define DP_CTL_RCP_POS_EDGE			0x0
#define DP_CTL_RCP_NEG_EDGE			0x1

#define DP_CTL_RST_NO_RESTART			0x0
#define DP_CTL_RST_RESTART			0x1

#define DP_CTL_UPEN_OFF				0x0
#define DP_CTL_UPEN_ON				0x1

#define DP_CTL_PRE_NO_RESET			0x0
#define DP_CTL_PRE_RESET			0x1

#define DP_CTL_HFSM_NORMAL_MODE			0x0
#define DP_CTL_HFSM_HFS_MODE			0x1

#define DP_CTL_REF_CLK_SEL_CKIH			0x0
#define DP_CTL_REF_CLK_SEL_DIGRF		0x1
#define DP_CTL_REF_CLK_SEL_CKIH_X2		0x2
#define DP_CTL_REF_CLK_SEL_DIGRF_X2		0x3

#define DP_CTL_REF_CLK_DIV_1			0x0
#define DP_CTL_REF_CLK_DIV_2			0x1

#define DP_CONFIG_LDREQ_DONE			0x0
#define DP_CONFIG_LDREQ_UPDATE			0x1

#define DP_CONFIG_AREN_NO_AUTO_RESTART		0x0
#define DP_CONFIG_AREN_AUTO_RESTART		0x1

#define DP_CONFIG_BIST_CE_NORMAL		0x0

#define DP_TOGC_TOG_DIS_ENABLED			0x0
#define DP_TOGC_TOD_DIS_DISABLED		0x1

#define DP_TOGC_TOD_EN_DISABLED			0x0
#define DP_TOGC_TOD_EN_ENABLED			0x1

#define DP_DESTAT_TOG_SEL_INACTIVE		0x0
#define DP_DESTAT_TOG_SEL_ACTIVE		0x1

#endif				/* __ARCH_ARM_MACH_MXC91131_CRM_REGS_H__ */
