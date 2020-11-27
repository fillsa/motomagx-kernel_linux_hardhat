/*
 * include/linux/i2c-pnx.h
 *
 * Header file for i2c support for PNX010x/4008.
 *
 * Author: Dennis Kovalev <dkovalev@ru.mvista.com>
 *
 * 2004 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef _I2C_PNX_H
#define _I2C_PNX_H

typedef enum {
	xmit,
	rcv,
	nodata,
} i2c_pnx_mode_t;

typedef enum {
	master,
	slave,
} i2c_pnx_adap_mode_t;

#define I2C_BUFFER_SIZE   0x100

typedef struct {
	int			ret;		/* Return value */
	i2c_pnx_mode_t	mode;		/* Interface mode */
	struct semaphore	sem;		/* Mutex for this interface */
	struct completion 	complete;	/* I/O completion */
	struct timer_list	timer;		/* Timeout */
	char *			buf;		/* Data buffer */
	int			len;		/* Length of data buffer */
} i2c_pnx_mif_t;

typedef struct {
	u32	base;
	int	irq;
	int	clock_id;
	i2c_pnx_adap_mode_t	mode;
	int	slave_addr;
	void	(*slave_recv_cb)(void *);
	void	(*slave_send_cb)(void *);
	volatile struct i2c_regs *	master;
	volatile struct i2c_regs *	slave;
	char	buffer [I2C_BUFFER_SIZE];	/* contains data received from I2C bus */
	int	buf_index;			/* index within I2C buffer (see above) */
	i2c_pnx_mif_t	mif;
} i2c_pnx_algo_data_t;

#include <asm/arch/platform.h>

#define TIMEOUT			(2*(HZ))
#define I2C_PNX010X_SPEED_KHZ	100

#define I2C_BLOCK_SIZE		0x100

#define CHIP_NAME		"PNX010x I2C"
#define ADAPTER0_NAME		"PNX010x I2C-0"
#define ADAPTER1_NAME		"PNX010x I2C-1"

#if defined (CONFIG_I2C_PNX_SLAVE_SUPPORT)
extern int i2c_slave_set_recv_cb (void(*i2c_slave_recv_cb)(void *), unsigned int adap);
extern int i2c_slave_set_send_cb (void(*i2c_slave_send_cb)(void *), unsigned int adap);
extern int i2c_slave_send_data(char *buf, int len, void *data);
extern int i2c_slave_recv_data(char *buf, int len, void *data);
#endif

#endif
