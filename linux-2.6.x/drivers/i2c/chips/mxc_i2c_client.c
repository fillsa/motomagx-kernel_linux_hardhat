/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006 Motorola, Inc.
 */

/* 
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 * 
 * http://www.opensource.org/licenses/gpl-license.html 
 * http://www.gnu.org/copyleft/gpl.html
 *
 *
 * Date        Author            Comment
 * ==========  ================  ========================
 * 11/04/2006  Motorola          Added RAW_I2C_API
 */

/*!
 * @file mxc_i2c_client.c
 * 
 * @brief I2C Chip Driver for the Freescale Semiconductor MXC platform. 
 * 
 * @ingroup MXCI2C
 */

/*
 * Include Files
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <asm/arch/mxc_i2c.h>

/*!
 * Array that holds pointers to MXC I2C Bus adapter structures. 
 */
static struct i2c_adapter *adap_list[I2C_NR];

/*!
 * Calls the adapters transfer method to exchange data with the i2c device.
 *
 * @param   bus_id       the MXC I2C bus that the slave device is connected to
 *                       (0 based)
 * @param   addr         slave address of the device we wish to exchange data with
 * @param   *reg         register in the device we wish to access
 * @param   reg_len      number of bytes in the register address
 * @param   *buf         data buffer 
 * @param   num          number of data bytes to transfer
 * @param   tran_flag    transfer flag
 *
 * @return  Function returns the number of messages sent to the device 
 *          or a negative number on failure
 */
static int mxc_i2c_client_xfer(int bus_id, unsigned int addr, char *reg,
			       int reg_len, char *buf, int num, int tran_flag)
{
	struct i2c_msg msg[2];
	int ret;

	msg[0].addr = addr;
	msg[0].len = reg_len;
	msg[0].buf = reg;
	msg[0].flags = tran_flag;
	msg[0].flags &= ~I2C_M_RD;

	msg[1].addr = addr;
	msg[1].len = num;
	msg[1].buf = buf;

	msg[1].flags = tran_flag;
	if (tran_flag & MXC_I2C_FLAG_READ) {
		msg[1].flags |= I2C_M_RD;
	} else {
		msg[1].flags &= ~I2C_M_RD;
	}

	ret = i2c_transfer(adap_list[bus_id], msg, 2);
	return ret;
}

#if defined(CONFIG_MOT_FEAT_RAW_I2C_API)
/*!
 * Calls the adapters transfer method to exchange data with the i2c device.
 *
 * @param   bus_id       the MXC I2C bus that the slave device is connected to
 *                       (0 based)
 * @param   addr         slave address of the device we wish to exchange data with
 * @param   len         number of bytes in the register address
 * @param   *buf         data buffer 
 * @param   tran_flag    flag indicates if it is a read or write
 *
 * @return  Function returns the number of messages sent to the device 
 *          or a negative number on failure
 */
static int mxc_i2c_raw_xfer(int bus_id, unsigned int addr, unsigned int len, 
                               char *buf, int tran_flag)
{
        struct i2c_msg msg[1];
        int ret;
      
        msg[0].addr = addr;
        msg[0].len = len;
        msg[0].buf = buf;

        msg[0].flags = tran_flag;
        if (tran_flag & MXC_I2C_FLAG_READ) {
                msg[0].flags |= I2C_M_RD;
        } else {
                msg[0].flags &= ~I2C_M_RD;
        }

        ret = i2c_transfer(adap_list[bus_id], msg, 1);
        return ret;
}

/*!
 * Function used to read the register of the i2c slave device. This function
 * is a kernel level API that can be called by camera misc device drivers.
 *
 * @param   bus_id       the MXC I2C bus that the slave device is connected to
 *                       (0 based)
 * @param   addr         slave address of the device we wish to read data from
 * @param   len         number of bytes in the register address
 * @param   *buf         data buffer 
 *
 * @return  Function returns the number of messages transferred to/from the device or 
 *          a negative number on failure
 */
int mxc_i2c_raw_read(int bus_id, unsigned int addr, int len, char *buf) 
{
        return mxc_i2c_raw_xfer(bus_id, addr, len, buf, MXC_I2C_FLAG_READ); 
}

/*!
 * Function used to write to the register of the i2c slave device. This function
 * is a kernel level API that can be called by camera misc device drivers.
 *
 * @param   bus_id       the MXC I2C bus that the slave device is connected to
 *                       (0 based)
 * @param   addr         slave address of the device we wish to write data to
 * @param   len         number of bytes in the register address
 * @param   *buf         data buffer 
 *
 * @return  Function returns the number of messages transferred to/from the device or 
 *          a negative number on failure
 */
int mxc_i2c_raw_write(int bus_id, unsigned int addr, int len, char *buf) 
{
        return mxc_i2c_raw_xfer(bus_id, addr, len, buf, 0); 
}
#endif /* CONFIG_MOT_FEAT_RAW_I2C_API */

/*!
 * Function used to read the register of the i2c slave device. This function
 * is a kernel level API that can be called by other device drivers.
 *
 * @param   bus_id       the MXC I2C bus that the slave device is connected to
 *                       (0 based)
 * @param   addr         slave address of the device we wish to read data from
 * @param   *reg         register in the device we wish to access
 * @param   reg_len      number of bytes in the register address
 * @param   *buf         data buffer 
 * @param   num          number of data bytes to transfer
 *
 * @return  Function returns the number of messages transferred to/from the device or 
 *          a negative number on failure
 */
int mxc_i2c_read(int bus_id, unsigned int addr, char *reg, int reg_len,
		 char *buf, int num)
{
	return mxc_i2c_client_xfer(bus_id, addr, reg, reg_len, buf, num,
				   MXC_I2C_FLAG_READ);
}

/*!
 * Function used to write to the register of the i2c slave device. This function
 * is a kernel level API that can be called by other device drivers.
 *
 * @param   bus_id       the MXC I2C bus that the slave device is connected to
 *                       (0 based)
 * @param   addr         slave address of the device we wish to write data to
 * @param   *reg         register in the device we wish to access
 * @param   reg_len      number of bytes in the register address
 * @param   *buf         data buffer 
 * @param   num          number of data bytes to transfer
 *
 * @return  Function returns the number of messages transferred to/from the device or 
 *          a negative number on failure
 */
int mxc_i2c_write(int bus_id, unsigned int addr, char *reg, int reg_len,
		  char *buf, int num)
{
	return mxc_i2c_client_xfer(bus_id, addr, reg, reg_len, buf, num, 0);
}

int mxc_i2c_polling_read(int bus_id, unsigned int addr, char *reg, int reg_len,
			 char *buf, int num)
{
	return mxc_i2c_client_xfer(bus_id, addr, reg, reg_len, buf, num,
				   MXC_I2C_FLAG_READ | MXC_I2C_FLAG_POLLING);
}

int mxc_i2c_polling_write(int bus_id, unsigned int addr, char *reg,
			  int reg_len, char *buf, int num)
{
	return mxc_i2c_client_xfer(bus_id, addr, reg, reg_len, buf, num,
				   MXC_I2C_FLAG_POLLING);
}

/*!
 * This callback function is called by the i2c core when an i2c bus driver is
 * loaded. This function checks to see if any of its i2c device are on this 
 * bus.
 *
 * @param  adapter the i2c adapter structure of the bus driver
 *
 * @return The function returns 0 on success and a non-zero value of failure
 */
static int mxc_i2c_client_attach(struct i2c_adapter *adapter)
{
	if (strcmp(adapter->name, MXC_ADAPTER_NAME) != 0) {
		return -1;
	}
	adap_list[adapter->id] = adapter;
	return 0;
}

/*!
 * This structure is used to hold the attach callback function name.
 */
static struct i2c_driver mxc_i2c_client_driver = {
	.owner = THIS_MODULE,
	.name = "MXC I2C Client",
	.flags = I2C_DF_NOTIFY,
	.attach_adapter = mxc_i2c_client_attach,
};

/*!
 * This function registers the i2c_driver structure with the i2c core.
 *
 * @return The function returns 0 on success and a non-zero value of failure
 */
static int __init mxc_i2c_client_init(void)
{
	return i2c_add_driver(&mxc_i2c_client_driver);
}

/*!
 * This function unregisters the chip driver from the i2c core.
 */
static void __exit mxc_i2c_client_exit(void)
{
	i2c_del_driver(&mxc_i2c_client_driver);
}

subsys_initcall(mxc_i2c_client_init);
module_exit(mxc_i2c_client_exit);

#if defined(CONFIG_MOT_FEAT_RAW_I2C_API)
EXPORT_SYMBOL(mxc_i2c_raw_read);
EXPORT_SYMBOL(mxc_i2c_raw_write);
#endif

EXPORT_SYMBOL(mxc_i2c_read);
EXPORT_SYMBOL(mxc_i2c_write);

EXPORT_SYMBOL(mxc_i2c_polling_read);
EXPORT_SYMBOL(mxc_i2c_polling_write);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC I2C Client driver");
MODULE_LICENSE("GPL");
