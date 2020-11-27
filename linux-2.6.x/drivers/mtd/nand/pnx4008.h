
#ifndef __MTD_NAND_PNX4008_H
#define __MTD_NAND_PNX4008_H

#define MLC_CMD 		0x00
#define MLC_ADDR 		0x04
#define MLC_DATA		0x04
#define MLC_ECC_ENC		0x08
#define MLC_ECC_DEC		0x0C
#define MLC_ECC_AUTO_ENC	0x10
#define MLC_ECC_AUTO_DEC	0x14
#define MLC_RPR			0x18
#define MLC_WPR			0x1C
#define MLC_RUBP		0x20
#define MLC_ROBP		0x24
#define MLC_SW_WP_ADD_LOW	0x28
#define MLC_SW_WP_ADD_HIGH	0x2C
#define MLC_ICR			0x30
#define MLC_TIME		0x34
#define MLC_IRQ_MR		0x38
#define MLC_IRQ_SR		0x3C
#define MLC_WP			0x40
#define MLC_LOCK_PR		0x44
#define MLC_UNLOCK_MAGIC	0xA25E
#define MLC_ISR			0x48
#define MLC_CEH			0x4C

#define MLC_ICR_WIDTH8		0
#define MLC_ICR_WP_ENABLED	(1<<3)
#define MLC_ICR_WP_DISABLED	0
#define MLC_ICR_SMALLBLK	0
#define MLC_ICR_LARGEBLK	(1<<2)
#define MLC_ICR_AWC4		(1<<1)
#define MLC_ICR_AWC3		0

#define MLC_ISR_ECCFAILURE	0x40
#define MLC_ISR_NUM_RS_ERRORS_MASK 0x30
#define MLC_ISR_NUM_RS_ERRORS_POS 4
#define MLC_ISR_ERRORS		0x08
#define MLC_ISR_ECCREADY	0x04
#define MLC_ISR_IFREADY		0x02
#define MLC_ISR_READY		0x01

#define GPIO_FLASH_WP 		GPO1_GPO_01
#define GPIO_OUTP_SET       	0x04

#define FLASHCLK_NDF_CLOCKS	0x01
#define FLASHCLK_MLC_CLOCKS	0x02
#define FLASHCLK_NDF_SELECT	(1<<2)
#define FLASHCLK_MLC_SELECT	(0<<2)
#define FLASHCLK_NAND_INT_REQ_E	0x08
#define FLASHCLK_NAND_RnB_REQ_E	0x10
#define FLASHCLK_NAND_INT_E	0x20

#define FLASHCLK_CTRL 0xC8
#define PNX4008_FLASHCLK_CTRL_REG IO_ADDRESS(PNX4008_PWRMAN_BASE + FLASHCLK_CTRL)
#define FLASHCLK_CTRL_INIT          (FLASHCLK_MLC_CLOCKS | FLASHCLK_MLC_SELECT)

#define FLASH_MLC_TIME_tWP(x)  (((x) & 0x0F ) <<  0 )
#define FLASH_MLC_TIME_tWH(x)  (((x) & 0x0F ) <<  4 )
#define FLASH_MLC_TIME_tRP(x)  (((x) & 0x0F ) <<  8 )
#define FLASH_MLC_TIME_tREH(x) (((x) & 0x0F ) << 12 )
#define FLASH_MLC_TIME_tRHZ(x) (((x) & 0x07 ) << 16 )
#define FLASH_MLC_TIME_tRWB(x) (((x) & 0x1F ) << 19 )
#define FLASH_MLC_TIME_tCEA(x) (((x) & 0x03 ) << 24 )

#endif /* PNX4008 */

