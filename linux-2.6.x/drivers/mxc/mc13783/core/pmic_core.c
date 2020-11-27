/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

/*!
 * @file mc13783_core.c
 * @brief This is the main file for the mc13783 driver.
 *
 * It contains devfs file system
 * and initialization of the driver.
 *
 * @ingroup MC13783
 */

/*
 * Includes
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <asm/arch/gpio.h>
#include <asm/arch/pmic_external.h>

//#include <spi/spi.h>
#include "../../spi/spi.h"

#include "pmic_event.h"
#include "pmic_config.h"

/*
 * Global variables
 */
static struct mc13783_status_regs status_regs = { 0, 0 };
static spi_config the_config;
static void *spi_id;
static int mxc_mc13783_ic_version;

/*!extern definitions found on GPIO and PMIC modules
 * We need to call this functions to work with
 * the SPI driver.
 */
extern void gpio_mc13783_active(void);
extern void gpio_mc13783_clear_int(void);
extern int send_frame_to_spi(int, unsigned int *, bool);
extern int gpio_mc13783_get_spi(void);
extern int gpio_mc13783_get_ss(void);

EXPORT_SYMBOL(mxc_pmic_get_ic_version);

/*! This is the bottom half of mc13783 core driver*/
void mc13783_handler(void *param);
DECLARE_WORK(mc13783_ws, mc13783_handler, (void *)&status_regs);

/*!
 * This function configures the SPI access.
 *
 * @return       This function returns 0 if successful.
 */
int spi_init(void)
{
	TRACEMSG(_K_D("\t\tspi configuration"));
	/* Initialize the semaphore */

	the_config.module_number = gpio_mc13783_get_spi();

	the_config.priority = HIGH;
	the_config.master_mode = true;
	the_config.bit_rate = 4000000;
	the_config.bit_count = 32;
	the_config.active_high_polarity = true;
	the_config.active_high_ss_polarity = true;
	the_config.phase = false;
	the_config.ss_low_between_bursts = true;
	the_config.ss_asserted = gpio_mc13783_get_ss();
	the_config.tx_delay = 0;
	spi_id = spi_get_device_id((spi_config *) & the_config);
	return 0;
}

/*!
 * This function is used to send a frame on SPI bus.
 *
 * @param        num_reg    mc13783 register number
 * @param        reg_value  Register value
 * @param        rw       read or write operation
 *
 * @return       This function returns 0 if successful.
 */
int send_frame_to_spi(int num_reg, unsigned int *reg_value, bool rw)
{
	unsigned int send_val = 0;
	unsigned int frame = 0;
	unsigned int frame_ret = 0;
	unsigned int result = 0;

	if (rw == true) {
		frame |= 0x80000000;
	} else {
		frame = 0;
	}

	frame |= (*reg_value & 0x0ffffff);
	frame |= ((unsigned int)num_reg & 0x3f) << 0x19;

	TRACEMSG(_K_D("\t\tspi send frame : value=0x%.8x"), frame & 0xffffffff);

	send_val = (((frame & 0x000000ff) << 0x18) |
		    ((frame & 0x0000ff00) << 0x08) |
		    ((frame & 0x00ff0000) >> 0x08) |
		    ((frame & 0xff000000) >> 0x18));

	result = spi_send_frame((unsigned char *)(&send_val),
				(unsigned long)4, spi_id);

	frame_ret = (((send_val & 0x000000ff) << 0x18) |
		     ((send_val & 0x0000ff00) << 0x08) |
		     ((send_val & 0x00ff0000) >> 0x08) |
		     ((send_val & 0xff000000) >> 0x18));

	*reg_value = frame_ret & 0x00ffffff;

	TRACEMSG(_K_D("\t\tspi received frame : value=0x%.8x"),
		 frame_ret & 0xffffffff);
	return 0;
};

/*!
 * This function is called when mc13783 interrupt occurs on the processor.
 * It is the interrupt handler for the mc13783 module.
 *
 * @param        irq        the irq number
 * @param        dev_id     the pointer on the device
 * @param        regs       the interrupt parameters
 *
 * @return       The function returns IRQ_RETVAL(1) when handled.
 */
static irqreturn_t mc13783_irq_handler(int irq, void *dev_id,
				       struct pt_regs *regs)
{
	/*clear mc13783 interrupt line */
	gpio_mc13783_clear_int();

	/* prepare a task */
	//status_regs.status0 = status0;
	//status_regs.status0 = status1;
	schedule_work(&mc13783_ws);

	TRACEMSG(_K_D("* mc13783 irq handler top half *"));
	return IRQ_RETVAL(1);
}

/*!
 * This function is called to find out the revision of the mc13783 IC.
 *
 * @param        none
 *
 * @return       The function returns a number based on mc13783 IC version .
 */

int mxc_pmic_get_ic_version(void)
{
	return mxc_mc13783_ic_version;
}

/*!
 * This function is the bottom half of the mc13783 Interrupt.
 * It checks the IT and launch client callback.
 *
 */
void mc13783_handler(void *param)
{
	//struct mc13783_status_regs status_regs;
	unsigned int status0 = 0;
	unsigned int status1 = 0;
	unsigned int mask_value = 0;
	int j = 0;

	/* read and clear the status bits of mc13783 events */
	send_frame_to_spi(REG_INTERRUPT_STATUS_0, &status0, false);
	status0 &= ISR_MASK_EVENT_0;
	send_frame_to_spi(REG_INTERRUPT_STATUS_0, &status0, true);

	/*
	   status_regs.status0 = ((struct mc13783_status_regs*)param)->status0;
	   status_regs.status1 = ((struct mc13783_status_regs*)param)->status1;
	 */

	if (status0 != 0) {
		for (j = 0; j < ISR_NB_BITS; j++) {
			mask_value = 1 << j;
			if (mask_value & status0) {
				launch_all_callback_event(0, (type_event) j);
			}
		}
	}

	send_frame_to_spi(REG_INTERRUPT_STATUS_1, &status1, false);
	status1 &= ISR_MASK_EVENT_1;
	send_frame_to_spi(REG_INTERRUPT_STATUS_1, &status1, true);

	TRACEMSG(_K_D("Test second IT regs"));

	if (status1 != 0) {
		for (j = 0; j < ISR_NB_BITS; j++) {
			mask_value = 1 << j;
			if (mask_value & status1) {
				launch_all_callback_event(1, (type_event) j);
			}
		}
	}
}

/*
 * Initialization and Exit
 */

/*!
 * This function implements the init function of the MC13783 device.
 * This function is called when the module is loaded.
 * It init the interrupt handler and write initialization value of mc13783.
 *
 * @return       This function returns 0.
 */
static int __init mc13783_init(void)
{
	int ret = 0;
	int rev_id = 0;
	int rev1 = 0;
	int rev2 = 0;
	int finid = 0;
	unsigned int reg_value;

	spi_init();

	reg_value = 0x00ffffff;
	pmic_write_reg(PRIO_CORE, REG_INTERRUPT_MASK_0, reg_value, reg_value);
	pmic_write_reg(PRIO_CORE, REG_INTERRUPT_MASK_1, reg_value, reg_value);
	pmic_write_reg(PRIO_CORE, REG_INTERRUPT_STATUS_0, reg_value, reg_value);
	pmic_write_reg(PRIO_CORE, REG_INTERRUPT_STATUS_1, reg_value, reg_value);

	/* install our own mc13783 handler */
	mc13783_init_event_and_it_gpio();

	/* initialize GPIO for Interrupt of mc13783 */
	gpio_mc13783_active();

	set_irq_type(MXC_PMIC_INT_LINE, IRQT_RISING);
	/* bind IT */
	ret = request_irq(MXC_PMIC_INT_LINE, mc13783_irq_handler, 0, 0, 0);
	if (ret) {
		TRACEMSG(_K_I("gpio1: irq%d error."), MXC_PMIC_INT_LINE);
		return ret;
	}

	send_frame_to_spi(REG_REVISION, &rev_id, false);

	rev1 = (rev_id & 0x018) >> 3;
	rev2 = (rev_id & 0x007);
	finid = (rev_id & 0x01E00) >> 9;

	TRACEMSG_ALL_TIME(KERN_INFO "mc13783 Rev %d.%d FinVer %X detected\n",
			  rev1, rev2, finid);

	if (rev1 == 0) {
		TRACEMSG_ALL_TIME(KERN_INFO
				  "mc13783 Core: FAILED\t!!!\tAccess failed\t!!!\n");
	} else {
		TRACEMSG_ALL_TIME(KERN_INFO
				  "mc13783 Core: successfully loaded\n");
	}

	mxc_mc13783_ic_version = (rev1 * 10) + rev2;
	TRACEMSG_ALL_TIME(KERN_INFO
			  "Detected mc13783 core IC version number is %d\n",
			  mxc_mc13783_ic_version);

	return 0;
}

/*!
 * This function implements the exit function of the MC13783 device.
 * This function is called when the module is unloaded.
 *
 */
static void __exit mc13783_exit(void)
{
	free_irq(MXC_PMIC_INT_LINE, 0);
	TRACEMSG_ALL_TIME(_K_I("mc13783 device: successfully unloaded\n"));
}

/*
 * Module entry points
 */
subsys_initcall(mc13783_init);
module_exit(mc13783_exit);

MODULE_DESCRIPTION("mc13783 device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
