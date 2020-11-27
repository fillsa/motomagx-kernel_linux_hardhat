/*
 *  include/asm-arm/arch-pnx4008/i2c.h
 *
 * Author: Vitaly Wool <vwool@ru.mvista.com>
 *
 * PNX4008-specific tweaks for I2C IP3204 block
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __ASM_ARCH_I2C_H__
#define __ASM_ARCH_I2C_H__

#include <linux/i2c.h>
#include <linux/i2c-pnx.h>
#include <asm/irq.h>


typedef enum {
	mstatus_tdi	= 0x00000001,
	mstatus_afi	= 0x00000002,
	mstatus_nai	= 0x00000004,
	mstatus_drmi	= 0x00000008,
	mstatus_active	= 0x00000020,
	mstatus_scl	= 0x00000040,
	mstatus_sda	= 0x00000080,
	mstatus_rff	= 0x00000100,
	mstatus_rfe	= 0x00000200,
	mstatus_tff	= 0x00000400,
	mstatus_tfe	= 0x00000800,
} mstatus_bits_t;

typedef enum {
	mcntrl_tdie	= 0x00000001,
	mcntrl_afie	= 0x00000002,
	mcntrl_naie	= 0x00000004,
	mcntrl_drmie	= 0x00000008,
	mcntrl_daie	= 0x00000020,
	mcntrl_rffie	= 0x00000040,
	mcntrl_tffie	= 0x00000080,
	mcntrl_reset	= 0x00000100,
	mcntrl_cdbmode	= 0x00000400,
} mcntrl_bits_t;

typedef enum {
	sstatus_drsi	= 0x00000010,
	sstatus_active	= 0x00000020,
	sstatus_scl	= 0x00000040,
	sstatus_sda	= 0x00000080,
	sstatus_rff	= 0x00000100,
	sstatus_rfe	= 0x00000200,
	sstatus_tff	= 0x00001000,
	sstatus_tte	= 0x00002000,
} sstatus_bits_t;

typedef enum {
	scntrl_drsie	= 0x00000010,
	scntrl_daie	= 0x00000020,
	scntrl_rffie	= 0x00000040,
	scntrl_tffie	= 0x00000400,
	scntrl_reset	= 0x00000100,
} scntrl_bits_t;

typedef enum {
	rw_bit		= 1 << 0,
	start_bit	= 1 << 8,
	stop_bit	= 1 << 9,
} i2c_pnx010x_prot_bits_t;


typedef volatile struct i2c_regs {
  /* LSB */
  union {
    const u32 rx; /* Receive FIFO Register - Read Only */
          u32 tx; /* Transmit FIFO Register - Write Only */
  } fifo;
        u32 sts;  /* Status Register - Read Only */
        u32 ctl;  /* Control Register - Read/Write */
        u32 ckl;  /* Clock divider low  - Read/Write */
        u32 ckh;  /* Clock divider high - Read/Write */
        u32 adr;  /* I2C address (optional) - Read/Write */
  const u32 rfl;  /* Rx FIFO level (optional) - Read Only */
  const u32 _unused;       /* Tx FIFO level (optional) - Read Only */
  const u32 rxb;  /* Number of bytes received (opt.) - Read Only */
  const u32 txb;  /* Number of bytes transmitted (opt.) - Read Only */
        u32 txs;  /* Tx Slave FIFO register - Write Only */
  const u32 stfl; /* Tx Slave FIFO level - Read Only */
        /* MSB */
} i2c_regs;

#include <asm/arch/platform.h>

/* Common defines */
#define LOW_IRQ			0
#define HIGH_IRQ		1
#define DONT_REQUEST_MEMORY	0
#define REQUEST_MEMORY		1
#define CLK_DIV_TYPE1		0	/* 2 reg. CLKHI, CLKLO */
#define CLK_DIV_TYPE2		1	/* 1 reg. CLK_CTRL */
#define I2C_BLOCK_SIZE		0x100

#define HCLK_MHZ		13
#define TIMEOUT			(2*(HZ))
#define I2C_SAA7752_SPEED_KHZ	100

#include <asm/hardware/clock.h>
static inline int i2c_pnx_suspend(struct device *dev, u32 state, u32 level)
{
	int retval = 0;
#ifdef CONFIG_PM
	struct clk *clk;
	struct platform_device *pdev = to_platform_device(dev);
	char name[10];

	switch (level) {
		case SUSPEND_DISABLE:
			break;
		case SUSPEND_SAVE_STATE:
			break;
		case SUSPEND_POWER_DOWN:
			sprintf(name, "i2c%d_ck", pdev->id);
			clk = clk_get(0, name);
			if (!IS_ERR(clk)) {
				clk_set_rate(clk, 0);
				clk_put(clk);
			} else
				retval = -ENOENT;
			break;
	}
#endif
	return retval;
}

static inline int i2c_pnx_resume(struct device *dev, u32 level)
{
	int retval = 0;
#ifdef CONFIG_PM
	struct clk *clk;
	struct platform_device *pdev = to_platform_device(dev);
	char name[10];


	switch (level) {
		case RESUME_POWER_ON:
			sprintf(name, "i2c%d_ck", pdev->id);
			clk = clk_get(0, name);
			if (!IS_ERR(clk)) {
				clk_set_rate(clk, 1);
				clk_put(clk);
			} else
				retval = -ENOENT;
			break;
		case RESUME_RESTORE_STATE:
			break;
		case RESUME_ENABLE:
			break;
	}
#endif
	return retval;
}

static inline u32 calculate_input_freq(struct device *dev)
{
	return 13; /* TODO */
}

static inline int set_clock_run(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct clk *clk;
	char name[10];
	int retval = 0;

	sprintf(name, "i2c%d_ck", pdev->id);
	clk = clk_get(0, name);
	if (!IS_ERR(clk)) {
		clk_set_rate(clk, 1);
		clk_put(clk);
	} else
		retval = -ENOENT;

	return retval;
}

static inline int set_clock_stop(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct clk *clk;
	char name[10];
	int retval = 0;

	sprintf(name, "i2c%d_ck", pdev->id);
	clk = clk_get(0, name);
	if (!IS_ERR(clk)) {
		clk_set_rate(clk, 0);
		clk_put(clk);
	} else
		retval = -ENOENT;

	return retval;
}

static struct i2c_algorithm pnx_algorithm;

static i2c_pnx_algo_data_t pnx_algo_data0 = {
	.base		= PNX4008_I2C1_BASE,
	.irq		= I2C_1_INT,
	.mode		= master,
	.master		= ((i2c_regs *)IO_ADDRESS(PNX4008_I2C1_BASE)),
};

static i2c_pnx_algo_data_t pnx_algo_data1 = {
	.base		= PNX4008_I2C2_BASE,
	.irq		= I2C_2_INT,
	.mode		= master,
	.master		= ((i2c_regs *)IO_ADDRESS(PNX4008_I2C2_BASE)),
};

static i2c_pnx_algo_data_t pnx_algo_data2 = {
	.base		= (PNX4008_USB_CONFIG_BASE + 0x300), /* TODO: #define it */
	.irq		= USB_I2C_INT,
	.mode		= master,
	.master		= ((i2c_regs *)(IO_ADDRESS(PNX4008_USB_CONFIG_BASE) + 0x300)),
};

static struct i2c_adapter pnx_adapter0 = {
	.name		= ADAPTER0_NAME,
	.id		= I2C_ALGO_SMBUS,
	.algo		= &pnx_algorithm,
	.algo_data	= (void *)&pnx_algo_data0,
};
static struct i2c_adapter pnx_adapter1 = {
	.name		= ADAPTER1_NAME,
	.id		= I2C_ALGO_SMBUS,
	.algo		= &pnx_algorithm,
	.algo_data	= (void *)&pnx_algo_data1,
};

static struct i2c_adapter pnx_adapter2 = {
	.name		= "USB-I2C",
	.id		= I2C_ALGO_SMBUS,
	.algo		= &pnx_algorithm,
	.algo_data	= (void *)&pnx_algo_data2,
};

static struct i2c_adapter *pnx_i2c_adapters[] = {
	&pnx_adapter0, 
	&pnx_adapter1,
	&pnx_adapter2,
};

#endif /* __ASM_ARCH_I2C_H___ */

