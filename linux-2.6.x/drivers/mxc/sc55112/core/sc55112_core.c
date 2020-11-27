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
 * @file sc55112_core.c
 * @brief This is the main file for the sc55112 driver.
 *
 * It contains devfs file system and initialization of the driver.
 *
 * @ingroup PMIC_SC55112_CORE
 */

/*
 * Includes
 */

#include "sc55112_config.h"
#include "sc55112_register.h"
#include "sc55112_event.h"
#include <asm/arch/pmic_external.h>

/*
 * For test only
 */
#define SC55112_DEBUG

#ifdef SC55112_DEBUG
#define STATIC
#else
#define STATIC    static
#endif

/*
 * End of For test only
 */

static int sc55112_major;

extern void gpio_sc55112_active(void);

/*!
 * This structure contains initialized value of registers.
 */
static unsigned int tab_init_reg[][2] = {
	{-1, -1},
};

#define IT_SC55112_PORT   0
#define IT_SC55112_BIT    5

/*
 * Global variables
 */

DECLARE_WORK(sc55112_wq_task, (void *)sc55112_wq_handler, (unsigned long)0);

/*!
 * This function is called when sc55112 interrupt occurs on the processor.
 * It is the interrupt handler for the sc55112 module.
 *
 * @param        irq        the irq number
 * @param        dev_id     the pointer on the device
 * @param        regs       the interrupt parameters
 *
 * @return       The function returns IRQ_RETVAL(1) when handled.
 */

STATIC irqreturn_t sc55112_irq_handler(int irq, void *dev_id,
				       struct pt_regs *regs)
{
	/* prepare a task */
	TRACEMSG(_K_D("* sc55112 irq handler top half *"));
	schedule_work(&sc55112_wq_task);
	return IRQ_RETVAL(1);
}

/*!
 * This function implements IOCTL controls on a MC13783 device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
STATIC int sc55112_ioctl(struct inode *inode, struct file *file,
			 unsigned int cmd, unsigned long arg)
{
#ifdef __TEST_CODE_ENABLE__
	register_info reg_info;
	type_event_notification event_sub;
#endif				/* __TEST_CODE_ENABLE__ */

	if (_IOC_TYPE(cmd) != 'P')
		return -ENOTTY;

	if (copy_from_user(&reg_info, (register_info *) arg,
			   sizeof(register_info))) {
		return -EFAULT;
	}

	switch (cmd) {
#ifdef __TEST_CODE_ENABLE__
	case PMIC_READ_REG:
		TRACEMSG(_K_D("read reg"));
		CHECK_ERROR(sc55112_read_reg(0,
					     reg_info.reg,
					     &(reg_info.reg_value)));
		break;
	case PMIC_WRITE_REG:
		TRACEMSG(_K_D("write reg %d %x %x"), reg_info.reg,
			 reg_info.reg_value, reg_info.reg_mask);
		CHECK_ERROR(sc55112_write_reg(0,
					      reg_info.reg, reg_info.reg_value,
					      reg_info.reg_mask));
		break;
	case PMIC_SUBSCRIBE:
		TRACEMSG(_K_I(" *** event sub %d"), event_sub.event);
		CHECK_ERROR(sc55112_event_subscribe_internal(event_sub));
		TRACEMSG(_K_I("subscribe done"));
		break;
	case PMIC_UNSUBSCRIBE:
		TRACEMSG(_K_I(" *** event unsub %d"), event_sub.event);
		CHECK_ERROR(sc55112_event_unsubscribe_internal(event_sub));
		break;
#endif				/* __TEST_CODE_ENABLE__ */
	default:
		TRACEMSG(_K_D("%d unsupported ioctl command"), (int)cmd);
		return -EINVAL;
	}

	if (copy_to_user((register_info *) arg, &reg_info,
			 sizeof(register_info))) {
		return -EFAULT;
	}

	return 0;
}

/*!
 * This function implements the open method on a MC13783 device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
STATIC int sc55112_open(struct inode *inode, struct file *file)
{
	TRACEMSG(_K_D("sc55112 : sc55112_open()"));
	return 0;
}

/*!
 * This function implements the release method on a MC13783 device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */

STATIC int sc55112_free(struct inode *inode, struct file *file)
{
	TRACEMSG(_K_D("sc55112 : sc55112_free()"));
	return 0;
}

/*!
 * This structure defines file operations for a MC13783 device.
 */
STATIC struct file_operations sc55112_fops = {
	/*!
	 * the owner
	 */
	.owner = THIS_MODULE,
	/*!
	 * the ioctl operation
	 */
	.ioctl = sc55112_ioctl,
	/*!
	 * the open operation
	 */
	.open = sc55112_open,
	/*!
	 * the release operation
	 */
	.release = sc55112_free,
};

/*
 * Initialization and Exit
 */

/*!
 * This function implements the init function of the MC13783 device.
 * This function is called when the module is loaded.
 * It init the interrupt handler and write initialization value of sc55112.
 *
 * @return       This function returns 0.
 */
STATIC int __init sc55112_init(void)
{
#if CONFIG_SC55112_PMIC_FIXARB == y
	extern PMIC_STATUS sc55112_fix_arbitration(void);
#endif

	int ret = 0, i = 0;

	sc55112_major = register_chrdev(0, "sc55112", &sc55112_fops);
	if (sc55112_major < 0) {
		TRACEMSG(_K_D("unable to get a major for sc55112"));
		return sc55112_major;
	}

	devfs_mk_cdev(MKDEV(sc55112_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "sc55112");

#if CONFIG_SC55112_PMIC_FIXARB == y
	if (PMIC_SUCCESS != sc55112_fix_arbitration()) {
		TRACEMSG(_K_D("unable to successfully fix arbitration\n"));
		return -1;
	}
#endif

	sc55112_init_register_access();

	/* init all register */
	TRACEMSG(_K_D("init all register value"));
	for (i = 0; tab_init_reg[i][0] != -1; i++) {
		TRACEMSG(_K_D("reg %d, value %x"), tab_init_reg[i][0],
			 tab_init_reg[i][1]);
		CHECK_ERROR(sc55112_write_reg(0, tab_init_reg[i][0],
					      tab_init_reg[i][1],
					      PMIC_ALL_BITS));
	}

	/* install our own sc55112 handler */
	sc55112_init_event();

	/* initialize GPIO for Interrupt of sc55112 */
	gpio_sc55112_active();

	/* bind IT */
	set_irq_type(MXC_PMIC_INT_LINE, IRQT_RISING);
	ret = request_irq(MXC_PMIC_INT_LINE, sc55112_irq_handler, 0, 0, 0);

	if (ret) {
		TRACEMSG(_K_I("irq%d error."), MXC_PMIC_INT_LINE);
		return ret;
	}

	TRACEMSG_ALL_TIME(KERN_INFO "sc55112 Core loaded\n");

	return 0;
}

/*!
 * This function implements the exit function of the MC13783 device.
 * This function is called when the module is unloaded.
 *
 */
static void __exit sc55112_exit(void)
{
	devfs_remove("sc55112");
	gpio_free_irq(IT_SC55112_PORT, IT_SC55112_BIT, GPIO_HIGH_PRIO);
	unregister_chrdev(sc55112_major, "sc55112");

	TRACEMSG_ALL_TIME(_K_I("sc55112 device: successfully unloaded\n"));
}

/*
 * Module entry points
 */

subsys_initcall(sc55112_init);
module_exit(sc55112_exit);

MODULE_DESCRIPTION("sc55112 char device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
