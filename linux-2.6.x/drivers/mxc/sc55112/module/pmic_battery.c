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
 * @file pmic_battery.c
 * @brief This is the main file of sc55112 Battery driver.
 *
 * @ingroup PMIC_SC55112_BATTERY
 */

/*
 * Includes
 */
#include <linux/delay.h>
#include <asm/ioctl.h>
#include <linux/device.h>

#include "../core/sc55112_config.h"
#include "sc55112_battery_regs.h"
#include "asm/arch/pmic_status.h"
#include "asm/arch/pmic_external.h"
#include "asm/arch/pmic_battery.h"
#include "asm/arch/pmic_adc.h"

static int pmic_battery_major;

/*
 * PMIC Battery API
 */

/* EXPORTED FUNCTIONS */
EXPORT_SYMBOL(pmic_batt_enable_charger);
EXPORT_SYMBOL(pmic_batt_disable_charger);
EXPORT_SYMBOL(pmic_batt_set_charger);
EXPORT_SYMBOL(pmic_batt_get_charger_setting);
EXPORT_SYMBOL(pmic_batt_get_charge_current);
EXPORT_SYMBOL(pmic_batt_enable_eol);
EXPORT_SYMBOL(pmic_batt_disable_eol);
EXPORT_SYMBOL(pmic_batt_led_control);
EXPORT_SYMBOL(pmic_batt_set_reverse_supply);
EXPORT_SYMBOL(pmic_batt_set_unregulated);

/*!
 * This is the suspend of power management for the sc55112 Battery API.
 *
 * @param        dev            the device
 * @param        state          the state
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_battery_suspend(struct device *dev, u32 state, u32 level)
{
	/* not supported */
	return -1;
};

/*!
 * This is the resume of power management for the sc55112 adc API.
 * It suports RESTORE state.
 *
 * @param        dev            the device
 * @param        level          the level
 *
 * @return       This function returns 0 if successful.
 */
static int pmic_battery_resume(struct device *dev, u32 level)
{
	/* not supported */
	return -1;
};

static struct device_driver pmic_battery_driver_ldm = {
	.name = "PMIC_Battery",
	.bus = &platform_bus_type,
	.probe = NULL,
	.remove = NULL,
	.suspend = pmic_battery_suspend,
	.resume = pmic_battery_resume,
};

/*!
 * This function is used to start charging a battery. For different charger,
 * different voltage and current range are supported. \n
 *
 *
 * @param      chgr        Charger as defined in /b t_batt_charger.
 * @param      c_voltage   Charging voltage.
 * @param      c_current   Charging current.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_enable_charger(t_batt_charger chgr,
				     unsigned char c_voltage,
				     unsigned char c_current)
{
	unsigned int val, mask;

	val = 0;
	mask = 0;

	switch (chgr) {
	case BATT_MAIN_CHGR:
		val = BITFVAL(sc55112_BATT_DAC_DAC, c_current);
		mask = BITFMASK(sc55112_BATT_DAC_DAC);
		break;

	case BATT_CELL_CHGR:
		val = BITFVAL(sc55112_BATT_DAC_V_COIN, c_voltage) |
		    BITFVAL(sc55112_BATT_DAC_I_COIN, c_current) |
		    BITFVAL(sc55112_BATT_DAC_COIN_CH_EN,
			    sc55112_BATT_DAC_COIN_CH_EN_ENABLED);
		mask =
		    BITFMASK(sc55112_BATT_DAC_V_COIN) |
		    BITFMASK(sc55112_BATT_DAC_I_COIN) |
		    BITFMASK(sc55112_BATT_DAC_COIN_CH_EN);
		break;

	case BATT_TRCKLE_CHGR:
		return PMIC_NOT_SUPPORTED;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_BATT_DAC, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function turns off a charger.
 *
 * @param      chgr        Charger as defined in /b t_batt_charger.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_disable_charger(t_batt_charger chgr)
{
	unsigned int val, mask;

	val = 0;
	mask = 0;

	switch (chgr) {
	case BATT_MAIN_CHGR:
		val = BITFVAL(sc55112_BATT_DAC_DAC, 0);
		mask = BITFMASK(sc55112_BATT_DAC_DAC);
		break;

	case BATT_CELL_CHGR:
		val =
		    BITFVAL(sc55112_BATT_DAC_COIN_CH_EN,
			    sc55112_BATT_DAC_COIN_CH_EN_DISABLED);
		mask = BITFMASK(sc55112_BATT_DAC_COIN_CH_EN);
		break;

	case BATT_TRCKLE_CHGR:
		return PMIC_NOT_SUPPORTED;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_BATT_DAC, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function is used to change the charger setting.
 *
 * @param      chgr        Charger as defined in /b t_batt_charger.
 * @param      c_voltage   Charging voltage.
 * @param      c_current   Charging current.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_charger(t_batt_charger chgr,
				  unsigned char c_voltage,
				  unsigned char c_current)
{
	unsigned int val, mask;

	val = 0;
	mask = 0;

	switch (chgr) {
	case BATT_MAIN_CHGR:
		val = BITFVAL(sc55112_BATT_DAC_DAC, c_current);
		mask = BITFMASK(sc55112_BATT_DAC_DAC);
		break;

	case BATT_CELL_CHGR:
		val = BITFVAL(sc55112_BATT_DAC_V_COIN, c_voltage) |
		    BITFVAL(sc55112_BATT_DAC_I_COIN, c_current);
		mask = BITFMASK(sc55112_BATT_DAC_V_COIN) |
		    BITFMASK(sc55112_BATT_DAC_I_COIN);
		break;

	case BATT_TRCKLE_CHGR:
		return PMIC_NOT_SUPPORTED;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_BATT_DAC, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function is used to retrive the charger setting.
 *
 * @param      chgr        Charger as defined in /b t_batt_charger.
 * @param      c_voltage   Output parameter for charging voltage setting.
 * @param      c_current   Output parameter for charging current setting.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_get_charger_setting(t_batt_charger chgr,
					  unsigned char *c_voltage,
					  unsigned char *c_current)
{
	unsigned int val;

	CHECK_ERROR(pmic_read_reg(PRIO_BATTERY, REG_BATT_DAC, &val));

	switch (chgr) {
	case BATT_MAIN_CHGR:
		*c_voltage = 0;
		*c_current = BITFEXT(val, sc55112_BATT_DAC_DAC);
		break;

	case BATT_CELL_CHGR:
		*c_voltage = BITFEXT(val, sc55112_BATT_DAC_V_COIN);
		*c_current = BITFEXT(val, sc55112_BATT_DAC_I_COIN);
		break;

	case BATT_TRCKLE_CHGR:
		*c_voltage = 0;
		*c_current = 0;
		return PMIC_NOT_SUPPORTED;
		break;

	default:
		return PMIC_PARAMETER_ERROR;
		break;
	}

	return PMIC_SUCCESS;
}

/*!
 * This function is retrives the main battery charging current.
 *
 * @param      c_current   Output parameter for charging current setting.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_get_charge_current(unsigned char *c_current)
{

	unsigned int val, mask, i, max_attempt = 3;
	PMIC_STATUS ret, ret1;

	val =
	    BITFVAL(sc55112_BATT_DAC_EXT_ISENSE,
		    sc55112_BATT_DAC_EXT_ISENSE_ENABLE);
	mask = BITFMASK(sc55112_BATT_DAC_EXT_ISENSE);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_BATT_DAC, val, mask));

	ret = pmic_adc_convert(AD5_CHG_ISENSE, (unsigned short *)c_current);

	val =
	    BITFVAL(sc55112_BATT_DAC_EXT_ISENSE,
		    sc55112_BATT_DAC_EXT_ISENSE_DISABLE);
	mask = BITFMASK(sc55112_BATT_DAC_EXT_ISENSE);

	for (i = 0; i < max_attempt; i++) {
		ret1 = pmic_write_reg(PRIO_BATTERY, REG_BATT_DAC, val, mask);
		if (ret1 != PMIC_SUCCESS) {
			msleep(10);
		} else {
			break;
		}
	}
	return ret;
}

/*!
 * This function enables End-of-Life comparator.
 *
 * @param      threshold  End-of-Life threshold.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_enable_eol(unsigned char threshold)
{
	unsigned val, mask;

	val =
	    BITFVAL(sc55112_BATT_DAC_EOL_CMP_EN,
		    sc55112_BATT_DAC_EOL_CMP_EN_ENABLE) |
	    BITFVAL(sc55112_BATT_DAC_EOL_SEL, threshold);
	mask =
	    BITFMASK(sc55112_BATT_DAC_EOL_CMP_EN) |
	    BITFMASK(sc55112_BATT_DAC_EOL_SEL);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_BATT_DAC, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function disables End-of-Life comparator.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_disable_eol(void)
{
	unsigned val, mask;

	val =
	    BITFVAL(sc55112_BATT_DAC_EOL_CMP_EN,
		    sc55112_BATT_DAC_EOL_CMP_EN_DISABLE);
	mask = BITFMASK(sc55112_BATT_DAC_EOL_CMP_EN);

	CHECK_ERROR(pmic_write_reg(PRIO_BATTERY, REG_BATT_DAC, val, mask));

	return PMIC_SUCCESS;
}

/*!
 * This function controls charge LED. This function is not supported by
 * sc55112.
 *
 * @param      on   If on is ture, LED will be turned on,
 *                  or otherwise, LED will be turned off.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_led_control(bool on)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function sets reverse supply mode. This function is not applicable
 * to sc55112.
 *
 * @param      enable     If enable is ture, reverse supply mode is enable,
 *                        or otherwise, reverse supply mode is disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_reverse_supply(bool enable)
{
	return PMIC_NOT_SUPPORTED;
}

/*!
 * This function sets unregulatored charging mode on main battery. This
 * function is not aoolicable to sc55112.
 *
 * @param      enable     If enable is ture, unregulated charging mode is
 *                        enable, or otherwise, disabled.
 *
 * @return     This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_batt_set_unregulated(bool enable)
{
	return PMIC_NOT_SUPPORTED;

}

/*!
 * This function implements IOCTL controls on a PMIC Battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @param        cmd         the command
 * @param        arg         the parameter
 * @return       This function returns 0 if successful.
 */
static int pmic_battery_ioctl(struct inode *inode, struct file *file,
			      unsigned int cmd, unsigned long arg)
{
	t_charger_setting *chgr_setting;
	unsigned char c_current;
	t_eol_setting *eol_setting;

	if (_IOC_TYPE(cmd) != 'p')
		return -ENOTTY;

	switch (cmd) {
	case PMIC_BATT_CHARGER_CONTROL:
		if ((chgr_setting =
		     kmalloc(sizeof(t_charger_setting), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(chgr_setting, (t_charger_setting *) arg,
				   sizeof(t_charger_setting))) {
			kfree(chgr_setting);
			return -EFAULT;
		}

		if (chgr_setting->on != false) {
			CHECK_ERROR_KFREE(pmic_batt_enable_charger
					  (chgr_setting->chgr,
					   chgr_setting->c_voltage,
					   chgr_setting->c_current),
					  (kfree(chgr_setting)));

		} else {
			CHECK_ERROR_KFREE(pmic_batt_disable_charger
					  (chgr_setting->chgr),
					  (kfree(chgr_setting)));

		}

		kfree(chgr_setting);
		break;

	case PMIC_BATT_SET_CHARGER:
		if ((chgr_setting =
		     kmalloc(sizeof(t_charger_setting), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(chgr_setting, (t_charger_setting *) arg,
				   sizeof(t_charger_setting))) {
			kfree(chgr_setting);
			return -EFAULT;
		}
		CHECK_ERROR_KFREE(pmic_batt_set_charger
				  (chgr_setting->chgr, chgr_setting->c_voltage,
				   chgr_setting->c_current),
				  (kfree(chgr_setting)));

		kfree(chgr_setting);
		break;

	case PMIC_BATT_GET_CHARGER:
		if ((chgr_setting =
		     kmalloc(sizeof(t_charger_setting), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(chgr_setting, (t_charger_setting *) arg,
				   sizeof(t_charger_setting))) {
			kfree(chgr_setting);
			return -EFAULT;
		}
		CHECK_ERROR_KFREE(pmic_batt_get_charger_setting
				  (chgr_setting->chgr, &chgr_setting->c_voltage,
				   &chgr_setting->c_current),
				  (kfree(chgr_setting)));

		if (copy_to_user((t_charger_setting *) arg, chgr_setting,
				 sizeof(t_charger_setting))) {
			kfree(chgr_setting);
			return -EFAULT;
		}

		kfree(chgr_setting);
		break;

	case PMIC_BATT_GET_CHARGER_CURRENT:

		CHECK_ERROR(pmic_batt_get_charge_current(&c_current));
		if (copy_to_user((unsigned char *)arg, &c_current,
				 sizeof(unsigned char *))) {
			return -EFAULT;
		}

		break;

	case PMIC_BATT_EOL_CONTROL:
		if ((eol_setting = kmalloc(sizeof(t_eol_setting), GFP_KERNEL))
		    == NULL) {
			return -ENOMEM;
		}
		if (copy_from_user(eol_setting, (t_eol_setting *) arg,
				   sizeof(t_eol_setting))) {
			kfree(eol_setting);
			return -EFAULT;
		}

		if (eol_setting->enable != false) {
			CHECK_ERROR_KFREE(pmic_batt_enable_eol
					  (eol_setting->threshold),
					  (kfree(eol_setting)));
		} else {
			CHECK_ERROR_KFREE(pmic_batt_disable_eol(),
					  (kfree(eol_setting)));
		}

		kfree(eol_setting);
		break;

	case PMIC_BATT_LED_CONTROL:
		return PMIC_NOT_SUPPORTED;
		break;

	case PMIC_BATT_REV_SUPP_CONTROL:
		return PMIC_NOT_SUPPORTED;
		break;

	case PMIC_BATT_UNREG_CONTROL:
		return PMIC_NOT_SUPPORTED;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}

/*!
 * This function implements the open method on a PMIC battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_battery_open(struct inode *inode, struct file *file)
{
	return 0;
}

/*!
 * This function implements the release method on a PMIC battery device.
 *
 * @param        inode       pointer on the node
 * @param        file        pointer on the file
 * @return       This function returns 0.
 */
static int pmic_battery_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct platform_device pmic_battery_ldm = {
	.name = "PMIC_BATTERY",
	.id = 1,
};

static struct file_operations pmic_battery_fops = {
	.owner = THIS_MODULE,
	.ioctl = pmic_battery_ioctl,
	.open = pmic_battery_open,
	.release = pmic_battery_release,
};

/*
 * Initialization and Exit
 */
static int __init pmic_battery_init(void)
{
	unsigned int ret;
	pmic_battery_major =
	    register_chrdev(0, "pmic_battery", &pmic_battery_fops);

	if (pmic_battery_major < 0) {
		/* TRACEMSG_ADC(_K_D("Unable to get a major for pmic_battery")); */
		return pmic_battery_major;
	}

	devfs_mk_cdev(MKDEV(pmic_battery_major, 0), S_IFCHR | S_IRUGO | S_IWUSR,
		      "pmic_battery");
	ret = driver_register(&pmic_battery_driver_ldm);
	if (ret == 0) {

		ret = platform_device_register(&pmic_battery_ldm);

		if (ret != 0) {
			driver_unregister(&pmic_battery_driver_ldm);
		}

		printk(KERN_INFO "PMIC Battery loaded\n");
	}

	return 0;
}

static void __exit pmic_battery_exit(void)
{
	devfs_remove("pmic_battery");
	unregister_chrdev(pmic_battery_major, "pmic_battery");
	driver_unregister(&pmic_battery_driver_ldm);
	platform_device_unregister(&pmic_battery_ldm);
	printk(KERN_INFO "PMIC Battery unloaded\n");
}

/*
 * Module entry points
 */

module_init(pmic_battery_init);
module_exit(pmic_battery_exit);

MODULE_DESCRIPTION("PMIC Battery device driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
