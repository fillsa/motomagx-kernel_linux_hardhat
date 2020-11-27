/*
 * include/asm-arm/arch-pnx4008/pm.h
 *
 * PNX4008 Power Management Routiness - header file
 *
 * Authors: Vitaly Wool, Dmitry Chigirev <source@mvista.com>
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ASM_ARCH_PNX4008_PM_H
#define __ASM_ARCH_PNX4008_PM_H

#ifndef __ASSEMBLER__
#include <asm/hardware/clock.h>

extern void pnx4008_pm_idle(void);
extern void pnx4008_pm_suspend(void);
extern unsigned int pnx4008_cpu_suspend_sz;
extern void pnx4008_cpu_suspend(void);
extern unsigned int pnx4008_cpu_standby_sz;
extern void pnx4008_cpu_standby(void);

extern int pnx4008_startup_pll(struct clk *);
extern int pnx4008_shutdown_pll(struct clk *);

/* Start Enable Pin Interrupts - table 58 page 66 */

#define SE_PIN_BASE_INT   32

#define SE_U7_RX_INT            63
#define SE_U7_HCTS_INT          62
#define SE_BT_CLKREQ_INT        61
#define SE_U6_IRRX_INT          60
/*59 unused*/
#define SE_U5_RX_INT            58
#define SE_GPI_11_INT           57
#define SE_U3_RX_INT            56
#define SE_U2_HCTS_INT          55
#define SE_U2_RX_INT            54
#define SE_U1_RX_INT            53
#define SE_DISP_SYNC_INT        52
/*51 unused*/
#define SE_SDIO_INT_N           50
#define SE_MSDIO_START_INT      49
#define SE_GPI_06_INT           48
#define SE_GPI_05_INT           47
#define SE_GPI_04_INT           46
#define SE_GPI_03_INT           45
#define SE_GPI_02_INT           44
#define SE_GPI_01_INT           43
#define SE_GPI_00_INT           42
#define SE_SYSCLKEN_PIN_INT     41
#define SE_SPI1_DATAIN_INT      40
#define SE_GPI_07_INT           39
#define SE_SPI2_DATAIN_INT      38
#define SE_GPI_10_INT           37
#define SE_GPI_09_INT           36
#define SE_GPI_08_INT           35
/*34-32 unused*/

/* Start Enable Internal Interrupts - table 57 page 65 */

#define SE_INT_BASE_INT   0

#define SE_TS_IRQ               31
#define SE_TS_P_INT             30
#define SE_TS_AUX_INT           29
/*27-28 unused*/
#define SE_USB_AHB_NEED_CLK_INT 26
#define SE_MSTIMER_INT          25
#define SE_RTC_INT              24
#define SE_USB_NEED_CLK_INT     23
#define SE_USB_INT              22
#define SE_USB_I2C_INT          21
#define SE_USB_OTG_TIMER_INT    20
#define SE_USB_OTG_ATX_INT_N    19
/*18 unused*/
#define SE_DSP_GPIO4_INT        17
#define SE_KEY_IRQ              16
#define SE_DSP_SLAVEPORT_INT    15
#define SE_DSP_GPIO1_INT        14
#define SE_DSP_GPIO0_INT        13
#define SE_DSP_AHB_INT          12
/*11-6 unused*/
#define SE_GPIO_05_INT          5
#define SE_GPIO_04_INT          4
#define SE_GPIO_03_INT          3
#define SE_GPIO_02_INT          2
#define SE_GPIO_01_INT          1
#define SE_GPIO_00_INT          0

#define START_INT_REG_BIT(irq) (1<<((irq)&0x1F))

#define START_INT_ER_REG(irq)     IO_ADDRESS((PNX4008_PWRMAN_BASE + 0x20 + (((irq)&(0x1<<5))>>1)))
#define START_INT_RSR_REG(irq)    IO_ADDRESS((PNX4008_PWRMAN_BASE + 0x24 + (((irq)&(0x1<<5))>>1)))
#define START_INT_SR_REG(irq)     IO_ADDRESS((PNX4008_PWRMAN_BASE + 0x28 + (((irq)&(0x1<<5))>>1)))
#define START_INT_APR_REG(irq)    IO_ADDRESS((PNX4008_PWRMAN_BASE + 0x2C + (((irq)&(0x1<<5))>>1)))

static inline void start_int_umask(u8 irq)
{
	__raw_writel(__raw_readl(START_INT_ER_REG(irq)) |
		     START_INT_REG_BIT(irq), START_INT_ER_REG(irq));
}

static inline void start_int_mask(u8 irq)
{
	__raw_writel(__raw_readl(START_INT_ER_REG(irq)) &
		     ~START_INT_REG_BIT(irq), START_INT_ER_REG(irq));
}

static inline void start_int_ack(u8 irq)
{
	__raw_writel(START_INT_REG_BIT(irq), START_INT_RSR_REG(irq));
}

static inline void start_int_set_falling_edge(u8 irq)
{
	__raw_writel(__raw_readl(START_INT_APR_REG(irq)) &
		     ~START_INT_REG_BIT(irq), START_INT_APR_REG(irq));
}

static inline void start_int_set_rising_edge(u8 irq)
{
	__raw_writel(__raw_readl(START_INT_APR_REG(irq)) |
		     START_INT_REG_BIT(irq), START_INT_APR_REG(irq));
}

#endif				/* ASSEMBLER */
#endif				/* __ASM_ARCH_PNX4008_PM_H */
