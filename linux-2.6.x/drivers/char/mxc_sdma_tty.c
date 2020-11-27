/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_sdma_tty.c
 * @brief This file contains functions for SDMA TTY driver
 *
 * SDMA TTY driver is used for moving data between MCU and DSP using line discipline API
 *
 * @ingroup SDMA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <asm/dma.h>
#include <asm/mach/dma.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/system.h>		/* save/local_irq_restore */
#include <asm/uaccess.h>	/* For copy_from_user */
#include <linux/sched.h>	/* For schedule */
#include <linux/devfs_fs_kernel.h>	/* For devfs_mk_cdev */
#include <linux/device.h>

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/circ_buf.h>

#include <asm/arch/mxc_sdma_tty.h>

#define DEBUG 0

#if DEBUG
#define DPRINTK(fmt, args...) printk("%s: " fmt, __FUNCTION__ , ## args)
#else
#define DPRINTK(fmt, args...)
#endif

#define SDMA_TTY_NORMAL_MAJOR 0

#define WRITE_ROOM 512

/*!
 * This define returns the number of read SDMA channel
 */
#define READ_CHANNEL(line) (line < IPC_NB_CH_BIDIR) ? 2 * line + 1 : \
                           (line < (IPC_NB_CH_BIDIR + IPC_NB_CH_DSPMCU)) ? \
                           (line + 3) : -1

/*!
 * This define returns the number of write SDMA channel
 */
#define WRITE_CHANNEL(line) (line < IPC_NB_CH_BIDIR) ? 2 * line + 2 : -1

/*!
 * This define returns 1 if the device is unidirectional
 * DSP to MCU
 */
#define DSPMCU_DEVICE(line) line < (IPC_NB_CH_BIDIR + IPC_NB_CH_DSPMCU) && \
                            line >= IPC_NB_CH_BIDIR

/*!
 * This holds tty driver definitions
 */
static struct tty_driver *sdma_tty_driver;

/*!
 * This holds the transmit tasklet
 */
static struct tasklet_struct sdma_tty_tasklet;

static struct class_simple *sdma_tty_class;

/*!
 * Structure containing sdma tty line status
 */
typedef struct {
	struct tty_struct *tty;	/*!< pointer to tty struct of the line */
	int loopback_mode;	/*!< Loopback mode: 0 - disable, 1 - enable */
	struct circ_buf write_buf;	/*!< Write buffer */
	char *write_buf_phys;	/*!< Write buffer dma pointer */
	char *read_buf;		/*!< Read buffer */
	char *read_buf_phys;	/*!< Read buffer dma pointer */
	int chars_in_buffer;	/*!< Number of characters in write buffer */
	int sending;		/*!< Sending flag */
	int sending_count;	/*!< Number of characters sent
				   in last write operation */
	int f_mode;		/*!< File mode (read/write) */
} sdma_tty_struct;

static sdma_tty_struct sdma_tty_data[IPC_NB_CH_BIDIR + IPC_NB_CH_DSPMCU];

static void sdma_tty_write_tasklet(unsigned long arg)
{
	int line, tx_num;
	struct tty_struct *tty;
	dma_request_t writechnl_request;
	struct circ_buf *circ;

	tty = (struct tty_struct *)arg;
	line = tty->index;
	DPRINTK("SDMA %s on line %d\n", __FUNCTION__, line);

	if (sdma_tty_data[line].chars_in_buffer > 0) {
		circ = &sdma_tty_data[line].write_buf;
		tx_num = CIRC_CNT_TO_END(circ->head, circ->tail, WRITE_ROOM);
		writechnl_request.sourceAddr =
		    sdma_tty_data[line].write_buf_phys + circ->tail;
		writechnl_request.count = tx_num;
		mxc_dma_set_config(WRITE_CHANNEL(line), &writechnl_request, 0);
		mxc_dma_start(WRITE_CHANNEL(line));
	} else {
		sdma_tty_data[line].sending = 0;
	}
}

/*!
 * This function called on SDMA write channel interrupt.
 *
 * @param       arg   number of SDMA channel
 */
static void sdma_tty_write_callback(void *arg)
{
	dma_request_t sdma_read_request, sdma_write_request;
	int line, count = 0;
	struct tty_struct *tty;
	struct circ_buf *circ;

	tty = (struct tty_struct *)arg;
	line = tty->index;

	mxc_dma_get_config(WRITE_CHANNEL(line), &sdma_write_request, 0);

	count = sdma_write_request.count;
	circ = &sdma_tty_data[line].write_buf;

	DPRINTK("SDMA %s on line %d count %d\n", __FUNCTION__, line, count);

	if (sdma_tty_data[line].loopback_mode) {
		sdma_tty_data[line].sending_count = count;

		mxc_dma_get_config(READ_CHANNEL(line), &sdma_read_request, 0);
		sdma_read_request.count = count;
		sdma_read_request.destAddr = sdma_tty_data[line].read_buf_phys;

		if (sdma_tty_data[line].tty->flip.count +
		    sdma_read_request.count >= TTY_FLIPBUF_SIZE) {
			tty_flip_buffer_push(sdma_tty_data[line].tty);
		}

		mxc_dma_set_config(READ_CHANNEL(line), &sdma_read_request, 0);
		mxc_dma_start(READ_CHANNEL(line));
	}
	sdma_tty_data[line].chars_in_buffer -= count;
	circ->tail = (circ->tail + count) & (WRITE_ROOM - 1);
	DPRINTK("SDMA %s on head %x tail %x\n", __FUNCTION__, circ->head,
		circ->tail);
	wake_up_interruptible(&sdma_tty_data[line].tty->write_wait);

	if (sdma_tty_data[line].chars_in_buffer > 0) {
		tasklet_schedule(&sdma_tty_tasklet);
	} else {
		sdma_tty_data[line].sending = 0;
	}
}

/*!
 * This function called on SDMA read channel interrupt.
 *
 * @param       arg   number of SDMA channel
 */
static void sdma_tty_read_callback(void *arg)
{
	dma_request_t read_request;
	struct tty_struct *tty;
	int line;
	int count, flip_cnt;

	tty = (struct tty_struct *)arg;
	line = tty->index;

	if (sdma_tty_data[line].loopback_mode &&
	    sdma_tty_data[line].sending_count <= 0) {
		return;
	}

	mxc_dma_get_config(READ_CHANNEL(line), &read_request, 0);

	if (read_request.bd_done == 1) {
		/* BD is not done yet */
		return;
	}

	flip_cnt = TTY_FLIPBUF_SIZE - tty->flip.count;
	/* Check for space availability in the TTY Flip buffer */
	if (flip_cnt <= 0) {
		goto drop_data;
	}
	/* Copy as much data as possible */
	if (read_request.count < flip_cnt) {
		count = read_request.count;
	} else {
		count = flip_cnt;
	}

	DPRINTK("SDMA %s on line %d mode = %d count = %d \
buff[0] = %x buff = %p dstAddr = %p\n", __FUNCTION__, line, sdma_tty_data[line].loopback_mode, count, tty->flip.char_buf_ptr[0], tty->flip.char_buf_ptr, read_request.destAddr);

	memcpy(tty->flip.char_buf_ptr, sdma_tty_data[line].read_buf, count);
	memset(tty->flip.flag_buf_ptr, TTY_NORMAL, count);
	tty->flip.flag_buf_ptr += count;
	tty->flip.char_buf_ptr += count;
	tty->flip.count += count;
	tty->real_raw = 1;
	tty_flip_buffer_push(tty);

	wake_up_interruptible(&tty->read_wait);

      drop_data:
	if (!sdma_tty_data[line].loopback_mode) {
		read_request.count = WRITE_ROOM;
		read_request.destAddr = sdma_tty_data[line].read_buf_phys;

		mxc_dma_set_config(READ_CHANNEL(line), &read_request, 0);
		mxc_dma_start(READ_CHANNEL(line));
	} else {
		sdma_tty_data[line].sending_count = 0;
	}

	DPRINTK("Exit\n");
}

/*!
 * This function changes loopback mode
 *
 * @param       tty    pointer to current tty line structure
 * @param       mode   loopback mode: 0 - disable, 1 enable
 * @param       line   tty line
 * @return      0 on success, error code on fail
 */
static int sdma_tty_change_mode(struct tty_struct *tty, int mode, int line)
{
	dma_channel_params read_sdma_params, write_sdma_params;
	dma_request_t sdma_read_request;
	int res = 0;

	if (!(mode == 0 || mode == 1)) {
		printk(KERN_WARNING "Illegal loopback mode value\n");
		return -EINVAL;
	}

	if ((mode == 1) && (!((sdma_tty_data[line].f_mode & FMODE_READ) &&
			      (sdma_tty_data[line].f_mode & FMODE_WRITE)))) {
		printk(KERN_WARNING
		       "Loopback mode requires channel to be have read and write modes\n");
		return -EINVAL;
	}

	/* Mode not changed */
	if (sdma_tty_data[line].loopback_mode == mode) {
		return 0;
	}

	sdma_tty_data[line].loopback_mode = mode;

	if (sdma_tty_data[line].f_mode & FMODE_READ) {
		mxc_dma_stop(READ_CHANNEL(line));
	}
	if (sdma_tty_data[line].f_mode & FMODE_WRITE) {
		mxc_dma_stop(WRITE_CHANNEL(line));
	}

	if (sdma_tty_data[line].f_mode & FMODE_READ) {
		/* SDMA read channel setup */
		memset(&read_sdma_params, 0, sizeof(dma_channel_params));
		read_sdma_params.peripheral_type = DSP;
		read_sdma_params.transfer_type =
		    (mode == 0) ? dsp_2_emi : dsp_2_emi_loop;
		read_sdma_params.event_id = 0;
		read_sdma_params.callback = sdma_tty_read_callback;
		read_sdma_params.arg = tty;
		res =
		    mxc_dma_setup_channel(READ_CHANNEL(line),
					  &read_sdma_params);
		if (res < 0) {
			return res;
		}
		tty_flip_buffer_push(sdma_tty_data[line].tty);

		/* SDMA read request setup */
		memset(&sdma_read_request, 0, sizeof(dma_request_t));
		sdma_read_request.destAddr = sdma_tty_data[line].read_buf_phys;

		mxc_dma_set_config(READ_CHANNEL(line), &sdma_read_request, 0);

		if (!sdma_tty_data[line].loopback_mode) {
			mxc_dma_start(READ_CHANNEL(line));
		}

	}

	if (sdma_tty_data[line].f_mode & FMODE_WRITE) {
		/* Write SDMA channel setup */
		memset(&write_sdma_params, 0, sizeof(dma_channel_params));
		write_sdma_params.peripheral_type = DSP;
		write_sdma_params.transfer_type =
		    (mode == 0) ? emi_2_dsp : emi_2_dsp_loop;
		write_sdma_params.event_id = 0;
		write_sdma_params.callback = sdma_tty_write_callback;
		write_sdma_params.arg = tty;
		res =
		    mxc_dma_setup_channel(WRITE_CHANNEL(line),
					  &write_sdma_params);
		if (res < 0) {
			return res;
		}
	}

	if (mode) {
		/* Echo off to avoid infinite loop */
		sdma_tty_data[line].tty->termios->c_lflag = 0;
	}

	return 0;
}

/*!
 * This function opens tty line
 *
 * @param       tty   pointer to current tty line structure
 * @param       filp  pointer to file structure
 * @return      0 on success, error code on fail
 */
static int sdma_tty_open(struct tty_struct *tty, struct file *filp)
{
	int channels[2];
	int line;
	int res;

	line = tty->index;

	if (DSPMCU_DEVICE(line) && (filp->f_mode & FMODE_WRITE)) {
		printk(KERN_WARNING
		       "Error: Cannot open this channel for writing.\n");
		return -EINVAL;
	}

	memset(&sdma_tty_data[line], 0, sizeof(sdma_tty_struct));

	channels[0] = READ_CHANNEL(line);
	channels[1] = WRITE_CHANNEL(line);

	sdma_tty_data[line].tty = tty;
	sdma_tty_data[line].f_mode = filp->f_mode;

	if (sdma_tty_data[line].f_mode & FMODE_READ) {
		res = mxc_request_dma(channels, "SDMA TTY");
		if (res < 0) {	// For read
			printk(KERN_WARNING
			       "Error: SDMA DSP read channel busy\n");
			goto read_dma_req_failed;
		}
	}
	if (sdma_tty_data[line].f_mode & FMODE_WRITE) {
		res = mxc_request_dma(channels + 1, "SDMA TTY");
		if (res < 0) {	// For write
			printk(KERN_WARNING
			       "Error: SDMA DSP write channel busy\n");
			goto write_dma_req_failed;
		}
	}

	sdma_tty_data[line].chars_in_buffer = 0;
	sdma_tty_data[line].sending = 0;
	sdma_tty_data[line].loopback_mode = -1;

	if (sdma_tty_data[line].f_mode & FMODE_WRITE) {
		sdma_tty_data[line].write_buf.buf = sdma_malloc(WRITE_ROOM);
		sdma_tty_data[line].write_buf.head = 0;
		sdma_tty_data[line].write_buf.tail = 0;
		sdma_tty_data[line].write_buf_phys =
		    (char *)sdma_virt_to_phys(sdma_tty_data[line].write_buf.
					      buf);
	}
	if (sdma_tty_data[line].f_mode & FMODE_READ) {
		sdma_tty_data[line].read_buf = sdma_malloc(WRITE_ROOM);
		sdma_tty_data[line].read_buf_phys =
		    (char *)sdma_virt_to_phys(sdma_tty_data[line].read_buf);
	}

	tty->low_latency = 1;	// High rate

	res = sdma_tty_change_mode(tty, DEFAULT_LOOPBACK_MODE, line);
	if (res < 0) {
		goto change_mode_failed;
	}
	tasklet_init(&sdma_tty_tasklet, sdma_tty_write_tasklet,
		     (unsigned long)tty);
	DPRINTK("SDMA %s raw = %d real_raw = %d\n", __FUNCTION__, tty->raw,
		tty->real_raw);

	return 0;

      change_mode_failed:
	if (sdma_tty_data[line].f_mode & FMODE_WRITE) {
		sdma_free(sdma_tty_data[line].write_buf.buf);
	}
	if (sdma_tty_data[line].f_mode & FMODE_READ) {
		sdma_free(sdma_tty_data[line].read_buf);
	}
      write_dma_req_failed:
	if (sdma_tty_data[line].f_mode & FMODE_READ) {
		mxc_free_dma(READ_CHANNEL(line));
	}
      read_dma_req_failed:
	return res;
}

/*!
 * This function closes tty line
 *
 * @param       tty   pointer to current tty line structure
 * @param       filp  pointer to file structure
 */
static void sdma_tty_close(struct tty_struct *tty, struct file *filp)
{
	int line;

	line = tty->index;

	if (sdma_tty_data[line].f_mode & FMODE_READ) {
		mxc_free_dma(READ_CHANNEL(line));
		sdma_free(sdma_tty_data[line].read_buf);
	}

	if (sdma_tty_data[line].f_mode & FMODE_WRITE) {
		mxc_free_dma(WRITE_CHANNEL(line));
		sdma_free(sdma_tty_data[line].write_buf.buf);
		sdma_tty_data[line].write_buf.buf = NULL;
	}
}

/*!
 * This function performs ioctl commands
 *
 * The driver supports 2 ioctls: TIOCLPBACK and TCGETS
 * Other ioctls are supported by upper tty layers.
 *
 * @param       tty   pointer to current tty line structure
 * @param       file  pointer to file structure
 * @param       cmd   command
 * @param       arg   pointer to argument
 * @return      0 on success, error code on fail
 */
static int sdma_tty_ioctl(struct tty_struct *tty, struct file *file,
			  unsigned int cmd, unsigned long arg)
{
	int res;
	int line;

	line = tty->index;

	switch (cmd) {
	case TIOCLPBACK:
		res = sdma_tty_change_mode(tty, *((int *)arg), line);

		return res;
		break;
	case TCGETS:
		if (copy_to_user((struct termios *)arg,
				 tty->termios, sizeof(struct termios))) {
			return -EFAULT;
		}
		return (0);
		break;
	default:
		/* printk(KERN_WARNING "Unsupported %d ioctl\n", cmd); */
		break;
	}

	return -ENOIOCTLCMD;
}

/*!
 * This function starts SDMA for sending data from write buffer
 *
 * @param       tty       pointer to current tty line structure
 * @param       count     number of characters
 * @return      number of characters copied to write buffer
 */
static ssize_t sdma_tty_write_to_device(struct tty_struct *tty, int count)
{
	dma_request_t sdma_write_request;
	int line;

	line = tty->index;

	DPRINTK("SDMA %s on line %d %d\n", __FUNCTION__, line, count);

	memset(&sdma_write_request, 0, sizeof(dma_request_t));

	sdma_tty_data[line].sending_count = count;

	sdma_write_request.sourceAddr = sdma_tty_data[line].write_buf_phys +
	    sdma_tty_data[line].write_buf.tail;
	sdma_write_request.count = count;

	mxc_dma_set_config(WRITE_CHANNEL(line), &sdma_write_request, 0);

	mxc_dma_start(WRITE_CHANNEL(line));

	return count;
}

/*!
 * This function flushes chars from write buffer
 *
 * @param       tty   pointer to current tty line structure
 */
static void sdma_tty_flush_chars(struct tty_struct *tty)
{
	int line, count = 0;
	struct circ_buf *circ;

	line = tty->index;
	circ = &sdma_tty_data[line].write_buf;

	if (sdma_tty_data[line].sending == 1) {
		return;
	}

	if (sdma_tty_data[line].chars_in_buffer == 0) {
		return;
	}

	sdma_tty_data[line].sending = 1;
	count = CIRC_CNT_TO_END(circ->head, circ->tail, WRITE_ROOM);
	sdma_tty_write_to_device(tty, count);
}

/*!
 * This function writes characters to write buffer
 *
 * @param       tty       pointer to current tty line structure
 * @param       buf       pointer to buffer
 * @param       count     number of characters
 * @return      number of characters copied to write buffer
 */
static ssize_t sdma_tty_write(struct tty_struct *tty,
			      const unsigned char *buf, int count)
{
	int line;
	int write_room, ret = 0;
	struct circ_buf *circ;
	line = tty->index;

	DPRINTK("SDMA %s on line %d %d\n", __FUNCTION__, line, count);
	circ = &sdma_tty_data[line].write_buf;

	if (circ->head == circ->tail) {
		circ->head = 0;
		circ->tail = 0;
	}

	while (1) {
		write_room =
		    CIRC_SPACE_TO_END(circ->head, circ->tail, WRITE_ROOM);
		if (count < write_room) {
			write_room = count;
		}
		if (write_room <= 0) {
			break;
		}

		memcpy(circ->buf + circ->head, buf, write_room);
		circ->head = (circ->head + write_room) & (WRITE_ROOM - 1);
		buf += write_room;
		count -= write_room;
		ret += write_room;
	}
	DPRINTK("SDMA %s on head %x tail %x\n", __FUNCTION__, circ->head,
		circ->tail);
	sdma_tty_data[line].chars_in_buffer += ret;
	sdma_tty_flush_chars(tty);

	return ret;
}

/*!
 * This function returns the number of characters in write buffer
 *
 * @param       tty       pointer to current tty line structure
 * @return      number of characters in write buffer
 */
static int sdma_tty_chars_in_buffer(struct tty_struct *tty)
{
	int res;
	int line;

	line = tty->index;

	res = sdma_tty_data[line].chars_in_buffer;

	return res;
}

/*!
 * This function returns how much room the tty driver has available in the write buffer
 *
 * @param       tty       pointer to current tty line structure
 * @return      how much room the tty driver has available in the write buffer
 */
static int sdma_tty_write_room(struct tty_struct *tty)
{
	int res;
	int line;
	struct circ_buf *circ;

	line = tty->index;
	circ = &sdma_tty_data[line].write_buf;

	res = CIRC_SPACE(circ->head, circ->tail, WRITE_ROOM);

	return res;
}

/*!
 * TTY operation structure
 */
static struct tty_operations sdma_tty_ops = {
	.open = sdma_tty_open,
	.close = sdma_tty_close,
	.write_room = sdma_tty_write_room,
	.write = sdma_tty_write,
	.flush_chars = sdma_tty_flush_chars,
	.ioctl = sdma_tty_ioctl,
	.chars_in_buffer = sdma_tty_chars_in_buffer,
};

/*!
 * This function registers the tty driver
 */
static int __init sdma_tty_init(void)
{
	int error;
	int dev_id, dev_number;
	int dev_mode;
	struct class_device *temp_class;

	dev_number = IPC_NB_CH_BIDIR + IPC_NB_CH_DSPMCU;

	sdma_tty_driver = alloc_tty_driver(dev_number);

	if (!sdma_tty_driver) {
		return -ENOMEM;
	}

	sdma_tty_driver->owner = THIS_MODULE;
	sdma_tty_driver->devfs_name = "sdma";
	sdma_tty_driver->name = "mxc_sdma_tty";
	sdma_tty_driver->name_base = 0;
	sdma_tty_driver->major = SDMA_TTY_NORMAL_MAJOR;
	sdma_tty_driver->minor_start = 0;
	sdma_tty_driver->num = dev_number;
	sdma_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
	sdma_tty_driver->subtype = SERIAL_TYPE_NORMAL;
	sdma_tty_driver->init_termios = tty_std_termios;
	sdma_tty_driver->flags = TTY_DRIVER_NO_DEVFS;	//TTY_DRIVER_REAL_RAW;
	sdma_tty_driver->flags |= TTY_DRIVER_REAL_RAW;	/* lpj */
	tty_set_operations(sdma_tty_driver, &sdma_tty_ops);

	if ((error = tty_register_driver(sdma_tty_driver))) {
		printk(KERN_ERR "SDMA_TTY: Couldn't register SDMA_TTY driver,\
 error = %d\n", error);
		put_tty_driver(sdma_tty_driver);

		return error;
	}

	sdma_tty_class = class_simple_create(THIS_MODULE, "sdma");
	if (IS_ERR(sdma_tty_class)) {
		printk(KERN_ERR "Error creating sdma tty class.\n");
		return PTR_ERR(sdma_tty_class);
	}

	dev_mode = S_IFCHR | S_IRUGO | S_IWUGO;
	for (dev_id = 0; dev_id < dev_number; dev_id++) {
		temp_class =
		    class_simple_device_add(sdma_tty_class,
					    MKDEV(sdma_tty_driver->major,
						  dev_id), NULL, "sdma%u",
					    dev_id);
		if (IS_ERR(temp_class))
			goto err_out;

		if (dev_id == IPC_NB_CH_BIDIR) {
			dev_mode = S_IFCHR | S_IRUGO;
		}
		if (dev_id == IPC_NB_CH_BIDIR + IPC_NB_CH_DSPMCU) {
			dev_mode = S_IFCHR | S_IWUGO;
		}

		if (devfs_mk_cdev(MKDEV(sdma_tty_driver->major, dev_id),
				  dev_mode, "sdma%u", dev_id))
			goto err_out;
	}

	printk("SDMA TTY Driver initialized\n");
	return error;

      err_out:
	printk(KERN_ERR "Error creating sdma class or class device.\n");
	for (dev_id = 0; dev_id < dev_number; dev_id++) {
		class_simple_device_remove(MKDEV
					   (sdma_tty_driver->major, dev_id));
		devfs_remove("sdma%u", dev_id);
	}
	class_simple_destroy(sdma_tty_class);
	devfs_remove("sdma");
	return -1;
}

/*!
 * This function unregisters the tty driver
 */
static void __exit sdma_tty_exit(void)
{

	int dev_id, dev_number;

	dev_number = IPC_NB_CH_BIDIR + IPC_NB_CH_DSPMCU;

	if (!sdma_tty_driver) {
		for (dev_id = 0; dev_id < dev_number; dev_id++) {
			class_simple_device_remove(MKDEV
						   (sdma_tty_driver->major,
						    dev_id));
			devfs_remove("sdma%u", dev_id);
		}
		devfs_remove("sdma");
		class_simple_destroy(sdma_tty_class);
	}

	tty_unregister_driver(sdma_tty_driver);
	put_tty_driver(sdma_tty_driver);
}

module_init(sdma_tty_init);
module_exit(sdma_tty_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MXC SDMA TTY driver");
MODULE_LICENSE("GPL");
