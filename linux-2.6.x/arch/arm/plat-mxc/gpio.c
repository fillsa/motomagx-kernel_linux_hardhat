/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (c) 2007 Motorola, Inc.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Implementation based on omap gpio.c
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 06-Jul-2007  Motorola        Added gpio_free_irq_dev work around.
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch/irqs.h>
#include <asm/mach/irq.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

/*!
 * @file gpio.c
 *
 * @brief This file contains the GPIO and IOMUX implementation details.
 *
 * @ingroup GPIO
 */

#define GPIO_TO_PORT(n)		(n / GPIO_NUM_PIN)
#define GPIO_TO_INDEX(n)	(n % GPIO_NUM_PIN)

/* GPIO related defines */

enum gpio_reg {
	GPIO_DR = 0x00,
	GPIO_GDIR = 0x04,
	GPIO_PSR = 0x08,
	GPIO_ICR1 = 0x0C,
	GPIO_ICR2 = 0x10,
	GPIO_IMR = 0x14,
	GPIO_ISR = 0x18,
};

extern struct mxc_gpio_port mxc_gpio_ports[];

struct gpio_port {
	u32 num;		/*!< gpio port number */
	u32 base;		/*!< gpio port base VA */
	u16 irq;		/*!< irq number to the core */
	u16 virtual_irq_start;	/*!< virtual irq start number */
	u32 reserved_map;	/*!< keep track of which pins are in use */
	u32 irq_is_level_map;	/*!< if a pin's irq is level sensitive. default is edge */
	raw_spinlock_t lock;	/*!< lock when operating on the port */
};
static struct gpio_port gpio_port[GPIO_PORT_NUM];

/*
 * Find the pointer to the gpio_port for a given pin.
 * @param gpio		a gpio pin number
 * @return		pointer to \b struc \b gpio_port
 */
static inline struct gpio_port *get_gpio_port(u32 gpio)
{
	return &gpio_port[GPIO_TO_PORT(gpio)];
}

/*
 * Check if a gpio pin is within [0, MXC_MAX_GPIO_LINES -1].
 * @param gpio		a gpio pin number
 * @return		0 if the pin number is valid; -1 otherwise
 */
static int check_gpio(u32 gpio)
{
	if (gpio >= MXC_MAX_GPIO_LINES) {
		printk(KERN_ERR "mxc-gpio: invalid GPIO %d\n", gpio);
		dump_stack();
		return -1;
	}
	return 0;
}

/*
 * Set a GPIO pin's direction
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param is_input	0 for output; non-zero for input
 */
static void _set_gpio_direction(struct gpio_port *port, u32 index, int is_input)
{
	u32 reg = port->base + GPIO_GDIR;
	u32 l;

	l = __raw_readl(reg);
	if (is_input)
		l &= ~(1 << index);
	else
		l |= 1 << index;
	__raw_writel(l, reg);
}

/*!
 * Exported function to set a GPIO pin's direction
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param is_input	1 (or non-zero) for input; 0 for output
 */
void mxc_set_gpio_direction(iomux_pin_name_t pin, int is_input)
{
	struct gpio_port *port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return;
	port = get_gpio_port(gpio);
	spin_lock(&port->lock);
	_set_gpio_direction(port, GPIO_TO_INDEX(gpio), is_input);
	spin_unlock(&port->lock);
}

/*
 * Set a GPIO pin's data output
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param data		value to be set (only 0 or 1 is valid)
 */
static void _set_gpio_dataout(struct gpio_port *port, u32 index, u32 data)
{
	u32 reg = port->base + GPIO_DR;
	u32 l = 0;

	l = (__raw_readl(reg) & (~(1 << index))) | (data << index);
	__raw_writel(l, reg);
}

/*!
 * Exported function to set a GPIO pin's data output
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param data		value to be set (only 0 or 1 is valid)
 */
void mxc_set_gpio_dataout(iomux_pin_name_t pin, u32 data)
{
	struct gpio_port *port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return;

	port = get_gpio_port(gpio);
	spin_lock(&port->lock);
	_set_gpio_dataout(port, GPIO_TO_INDEX(gpio), (data == 0) ? 0 : 1);
	spin_unlock(&port->lock);
}

/*!
 * Return the data value of a GPIO signal.
 * @param pin	a name defined by \b iomux_pin_name_t
 *
 * @return 	value (0 or 1) of the GPIO signal; -1 if pass in invalid pin
 */
int mxc_get_gpio_datain(iomux_pin_name_t pin)
{
	struct gpio_port *port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return -1;

	port = get_gpio_port(gpio);

	return (__raw_readl(port->base + GPIO_DR) >> GPIO_TO_INDEX(gpio)) & 1;
}

/*
 * Set a GPIO pin's interrupt edge
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param icr		one of the values defined in \b gpio_edge_t
 *                      to indicate how to generate an interrupt
 */
static void _set_gpio_edge_ctrl(struct gpio_port *port, u32 index,
				gpio_edge_t edge)
{
	u32 reg = port->base;
	u32 l, sig;

	reg += (index <= 15) ? GPIO_ICR1 : GPIO_ICR2;
	sig = (index <= 15) ? index : (index - 16);

	l = __raw_readl(reg);
	l = (l & (~(0x3 << (sig * 2)))) | (edge << (sig * 2));
	__raw_writel(l, reg);
}

/*
 * Clear a GPIO signal's interrupt status
 *
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 */
static inline void _clear_gpio_irqstatus(struct gpio_port *port, u32 index)
{
	__raw_writel(1 << index, port->base + GPIO_ISR);
}

/*
 * Enable/disable a GPIO signal's interrupt.
 *
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param enable	\b #true for enabling the interrupt; \b #false otherwise
 */
static inline void _set_gpio_irqenable(struct gpio_port *port, u32 index,
				       bool enable)
{
	u32 reg = port->base + GPIO_IMR;
	u32 mask = (!enable) ? 0 : 1;
	u32 l;

	l = __raw_readl(reg);
	l = (l & (~(1 << index))) | (mask << index);
	__raw_writel(l, reg);
}

static inline int _request_gpio(struct gpio_port *port, u32 index)
{
	spin_lock(&port->lock);
	if (port->reserved_map & (1 << index)) {
		printk(KERN_ERR
		       "GPIO port %d (0-based), pin %d is already reserved!\n",
		       port->num, index);
		dump_stack();
		spin_unlock(&port->lock);
		return -1;
	}
	port->reserved_map |= (1 << index);
	spin_unlock(&port->lock);
	return 0;
}

/*!
 * Request ownership for a GPIO pin. The caller has to check the return value
 * of this function to make sure it returns 0 before make use of that pin.
 * @param pin		a name defined by \b iomux_pin_name_t
 * @return		0 if successful; Non-zero otherwise
 */
int mxc_request_gpio(iomux_pin_name_t pin)
{
	struct gpio_port *port;
	u32 index, gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return -EINVAL;

	port = get_gpio_port(gpio);
	index = GPIO_TO_INDEX(gpio);

	return _request_gpio(port, index);
}

/*!
 * Release ownership for a GPIO pin
 * @param pin		a name defined by \b iomux_pin_name_t
 */
void mxc_free_gpio(iomux_pin_name_t pin)
{
	struct gpio_port *port;
	u32 index, gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return;

	port = get_gpio_port(gpio);
	index = GPIO_TO_INDEX(gpio);

	spin_lock(&port->lock);
	if ((!(port->reserved_map & (1 << index)))) {
		printk(KERN_ERR "GPIO port %d, pin %d wasn't reserved!\n",
		       port->num, index);
		dump_stack();
		spin_unlock(&port->lock);
		return;
	}
	port->reserved_map &= ~(1 << index);
	port->irq_is_level_map &= ~(1 << index);
	_set_gpio_direction(port, index, 1);
	_set_gpio_irqenable(port, index, 0);
	_clear_gpio_irqstatus(port, index);
	spin_unlock(&port->lock);
}

/*
 * We need to unmask the GPIO port interrupt as soon as possible to
 * avoid missing GPIO interrupts for other lines in the port.
 * Then we need to mask-read-clear-unmask the triggered GPIO lines
 * in the port to avoid missing nested interrupts for a GPIO line.
 * If we wait to unmask individual GPIO lines in the port after the
 * line's interrupt handler has been run, we may miss some nested
 * interrupts.
 */
static void mxc_gpio_irq_handler(u32 irq, struct irqdesc *desc,
				 struct pt_regs *regs)
{
	u32 isr_reg = 0, imr_reg = 0, imr_val;
	u32 int_valid;
	u32 gpio_irq;
	struct gpio_port *port;

	port = (struct gpio_port *)desc->data;
	isr_reg = port->base + GPIO_ISR;
	imr_reg = port->base + GPIO_IMR;

	imr_val = __raw_readl(imr_reg);
	int_valid = __raw_readl(isr_reg) & imr_val;

	if (unlikely(!int_valid)) {
		printk(KERN_ERR "\nGPIO port: %d Spurious interrupt:0x%0x\n\n",
		       port->num, int_valid);
		BUG();		/* oops */
	}

	gpio_irq = port->virtual_irq_start;
	for (; int_valid != 0; int_valid >>= 1, gpio_irq++) {
		struct irqdesc *d;

		if ((int_valid & 1) == 0)
			continue;
		d = irq_desc + gpio_irq;
		if (unlikely(!(d->handle))) {
			printk(KERN_ERR "\nGPIO port: %d irq: %d unhandeled\n",
			       port->num, gpio_irq);
			BUG();	/* oops */
		}
		d->handle(gpio_irq, d, regs);
	}
}

/*
 * Disable a gpio pin's interrupt by setting the bit in the imr.
 * @param irq		a gpio virtual irq number
 */
static void gpio_mask_irq(u32 irq)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	struct gpio_port *port = get_gpio_port(gpio);

	_set_gpio_irqenable(port, GPIO_TO_INDEX(gpio), 0);
}

/*
 * Acknowledge a gpio pin's interrupt by clearing the bit in the isr.
 * If the GPIO interrupt is level triggered, it also disables the interrupt.
 * @param irq		a gpio virtual irq number
 */
static void gpio_ack_irq(u32 irq)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	u32 index = GPIO_TO_INDEX(gpio);
	struct gpio_port *port = get_gpio_port(gpio);

	_clear_gpio_irqstatus(port, GPIO_TO_INDEX(gpio));
	if (port->irq_is_level_map & (1 << index)) {
		gpio_mask_irq(irq);
	}
}

/*
 * Enable a gpio pin's interrupt by clearing the bit in the imr.
 * @param irq		a gpio virtual irq number
 */
static void gpio_unmask_irq(u32 irq)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	struct gpio_port *port = get_gpio_port(gpio);

	_set_gpio_irqenable(port, GPIO_TO_INDEX(gpio), 1);
}

/*
 * Enable a gpio pin's interrupt by clearing the bit in the imr.
 * @param irq		a gpio virtual irq number
 */
static int gpio_set_irq_type(u32 irq, u32 type)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	struct gpio_port *port = get_gpio_port(gpio);

	switch (type) {
	case IRQT_RISING:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_RISE_EDGE);
		set_irq_handler(irq, do_edge_IRQ);
		break;
	case IRQT_FALLING:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_FALL_EDGE);
		set_irq_handler(irq, do_edge_IRQ);
		break;
	case IRQT_LOW:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_LOW_LEV);
		set_irq_handler(irq, do_level_IRQ);
		break;
	case IRQT_HIGH:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_HIGH_LEV);
		set_irq_handler(irq, do_level_IRQ);
		break;
	case IRQT_BOTHEDGE:
	default:
		return -EINVAL;
		break;
	}
	return 0;
}

static struct irqchip gpio_irq_chip = {
	.ack = gpio_ack_irq,
	.mask = gpio_mask_irq,
	.unmask = gpio_unmask_irq,
	.type = gpio_set_irq_type,
};

static int initialized = 0;

static int __init _mxc_gpio_init(void)
{
	int i;
	struct gpio_port *port;

	initialized = 1;

	printk(KERN_INFO "MXC GPIO hardware\n");

	for (i = 0; i < GPIO_PORT_NUM; i++) {
		int j, gpio_count = GPIO_NUM_PIN;

		port = &gpio_port[i];
		port->base = mxc_gpio_ports[i].base;
		port->num = mxc_gpio_ports[i].num;
		port->irq = mxc_gpio_ports[i].irq;
		port->virtual_irq_start = mxc_gpio_ports[i].virtual_irq_start;

		port->reserved_map = 0;
		spin_lock_init(&port->lock);

		/* disable the interrupt and clear the status */
		__raw_writel(0, port->base + GPIO_IMR);
		__raw_writel(0xFFFFFFFF, port->base + GPIO_ISR);
		for (j = port->virtual_irq_start;
		     j < port->virtual_irq_start + gpio_count; j++) {
			set_irq_chip(j, &gpio_irq_chip);
			set_irq_handler(j, do_edge_IRQ);
			set_irq_flags(j, IRQF_VALID);
		}
		set_irq_chained_handler(port->irq, mxc_gpio_irq_handler);
		set_irq_data(port->irq, port);
	}

	return 0;
}

/*
 * This may get called early from board specific init
 */
int mxc_gpio_init(void)
{
	if (!initialized)
		return _mxc_gpio_init();
	else
		return 0;
}

/*
 * FIXME: The following functions are for backward-compatible.
 * They will be removed at a future release.
 */
#define PORT_SIG_TO_GPIO(p, s)		(p * 32 + s)

/*!
 * This function configures the GPIO signal to be either input or output. For
 * input signals used for generating interrupts for the ARM core, how the
 * interrupts being triggered is also passed in via \a icr. For output signals,
 * the \a icr value doesn't matter.
 * FIXED ME: for backward compatible. to be removed!
 *
 * @param  port         specified port with 0-GPIO port 1; 1-GPIO port 2
 * @param  sig_no       specified GPIO signal (0 based)
 * @param  out          #true for output, #false for input
 * @param  icr          value defined in \b #gpio_edge_t
 */
void gpio_config(u32 port, u32 sig_no, bool out, gpio_edge_t icr)
{
	u32 gpio = PORT_SIG_TO_GPIO(port, sig_no);
	struct gpio_port *port_p;

	if (check_gpio(gpio) < 0)
		return;

	port_p = get_gpio_port(gpio);

	if (!(port_p->reserved_map & (1 << sig_no)))
		_request_gpio(port_p, sig_no);

	if (!out) {
		/* input */
		_set_gpio_direction(port_p, sig_no, 1);
		_set_gpio_edge_ctrl(port_p, sig_no, icr);
	} else {		/* output */
		_set_gpio_direction(port_p, sig_no, 0);
	}
}

/*!
 * This function sets a GPIO signal value.
 * FIXED ME: for backward compatible. to be removed!
 *
 * @param  port         specified port with 0 for GPIO port 1 and 1 for GPIO port 2
 * @param  sig_no       specified GPIO signal (0 based)
 * @param  data         value to be set (only 0 or 1 is valid)
 */
void gpio_set_data(u32 port, u32 sig_no, u32 data)
{
	u32 gpio = PORT_SIG_TO_GPIO(port, sig_no);
	struct gpio_port *port_p;

	if (check_gpio(gpio) < 0)
		return;

	port_p = get_gpio_port(gpio);

	if (!(port_p->reserved_map & (1 << sig_no)))
		_request_gpio(port_p, sig_no);

	spin_lock(&port_p->lock);
	_set_gpio_dataout(port_p, sig_no, (data == 0) ? 0 : 1);
	spin_unlock(&port_p->lock);
}

/*!
 * This function returns the value of the GPIO signal.
 * FIXED ME: for backward compatible. to be removed!
 *
 * @param  port         specified port with 0 for GPIO port 1 and 1 for GPIO port 2
 * @param  sig_no       specified GPIO signal (0 based)
 *
 * @return Value of the GPIO signal
 */
u32 gpio_get_data(u32 port, u32 sig_no)
{
	u32 gpio = PORT_SIG_TO_GPIO(port, sig_no);
	struct gpio_port *port_p;

	if (check_gpio(gpio) < 0)
		BUG();

	port_p = get_gpio_port(gpio);

	if (!(port_p->reserved_map & (1 << sig_no)))
		_request_gpio(port_p, sig_no);

	return (__raw_readl(port_p->base + GPIO_DR) >> sig_no) & 1;
}

/*!
 * This function clears the GPIO interrupt for a signal on a port.
 * FIXED ME: for backward compatible. to be removed!
 *
 * @param  port         specified port with 0 for GPIO port 1 and 1 for GPIO port 2
 * @param  sig_no       specified GPIO signal (0 based) to clear
 */
void gpio_clear_int(u32 port, u32 sig_no)
{
	u32 gpio = PORT_SIG_TO_GPIO(port, sig_no);
	struct gpio_port *port_p;

	if (check_gpio(gpio) < 0)
		return;

	port_p = get_gpio_port(gpio);

	if (!(port_p->reserved_map & (1 << sig_no)))
		_request_gpio(port_p, sig_no);

	_clear_gpio_irqstatus(port_p, sig_no);
}

/*!
 * This function is responsible for registering a GPIO signal's ISR.
 * FIXED ME: for backward compatible. to be removed!
 *
 * @param  port         specified port with 0 for GPIO port 1 and 1 for GPIO port 2
 * @param  sig_no       specified GPIO signal (0 based)
 * @param  prio         priority as defined in \b enum \b #gpio_prio
 * @param  handler      GPIO ISR function pointer for the GPIO signal
 * @param  irq_flags    irq flags (not used)
 * @param  devname      device name associated with the interrupt
 * @param  dev_id       some unique information for the ISR
 *
 * @return 0 if successful; non-zero otherwise.
 */
int gpio_request_irq(u32 port, u32 sig_no, enum gpio_prio prio,
		     gpio_irq_handler handler, u32 irq_flags,
		     const char *devname, void *dev_id)
{
	u32 gpio = PORT_SIG_TO_GPIO(port, sig_no), ret;
	struct gpio_port *port_p;

	if (check_gpio(gpio) < 0)
		return -1;

	port_p = get_gpio_port(gpio);

	if (!(port_p->reserved_map & (1 << sig_no)))
		_request_gpio(port_p, sig_no);

	ret = request_irq(MXC_GPIO_TO_IRQ(gpio), handler, 0, "gpio", dev_id);
	if (ret) {
		printk(KERN_ERR "gpio: IRQ%d already in use.\n",
		       MXC_GPIO_TO_IRQ(gpio));
		return ret;
	}
	return 0;
}

/*!
 * This function un-registers an ISR with the GPIO interrupt module.
 * FIXED ME: for backward compatible. to be removed!
 *
 * @param  port         specified port with 0 for GPIO port 1 and 1 for GPIO port 2
 * @param  sig_no       specified GPIO signal (0 based)
 * @param  prio         priority as defined in \b enum \b #gpio_prio
 */
void gpio_free_irq(u32 port, u32 sig_no, enum gpio_prio prio)
{
	u32 gpio = PORT_SIG_TO_GPIO(port, sig_no);
	struct gpio_port *port_p;

	if (check_gpio(gpio) < 0)
		return;

	port_p = get_gpio_port(gpio);

	free_irq(MXC_GPIO_TO_IRQ(gpio), NULL);

	spin_lock(&port_p->lock);
	if ((!(port_p->reserved_map & (1 << sig_no)))) {
		printk(KERN_ERR "GPIO port %d, pin %d wasn't reserved!\n",
		       port_p->num, sig_no);
		dump_stack();
		spin_unlock(&port_p->lock);
		return;
	}
	port_p->reserved_map &= ~(1 << sig_no);
	_set_gpio_direction(port_p, sig_no, 1);
	_set_gpio_irqenable(port_p, sig_no, 0);
	_clear_gpio_irqstatus(port_p, sig_no);
	spin_unlock(&port_p->lock);
}

#if defined(CONFIG_MOT_FEAT_GPIO_API)
/*!
 * This function un-registers an ISR with the GPIO interrupt module.
 * FIXED ME: for backward compatible. to be removed!
 *
 * @param  port         specified port with 0 for GPIO port 1 and 1 for GPIO port 2
 * @param  sig_no       specified GPIO signal (0 based)
 * @param  prio         priority as defined in \b enum \b #gpio_prio
 * @param  dev_id       some unique information for the ISR
 */
void gpio_free_irq_dev(u32 port, u32 sig_no, enum gpio_prio prio, void *dev_id)
{
	u32 gpio = PORT_SIG_TO_GPIO(port, sig_no);
	struct gpio_port *port_p;

	if (check_gpio(gpio) < 0)
		return;

	port_p = get_gpio_port(gpio);

	free_irq(MXC_GPIO_TO_IRQ(gpio), dev_id);

	spin_lock(&port_p->lock);
	if ((!(port_p->reserved_map & (1 << sig_no)))) {
		printk(KERN_ERR "GPIO port %d, pin %d wasn't reserved!\n",
		       port_p->num, sig_no);
		dump_stack();
		spin_unlock(&port_p->lock);
		return;
	}
	port_p->reserved_map &= ~(1 << sig_no);
	_set_gpio_direction(port_p, sig_no, 1);
	_set_gpio_irqenable(port_p, sig_no, 0);
	_clear_gpio_irqstatus(port_p, sig_no);
	spin_unlock(&port_p->lock);
}
#endif /* CONFIG_MOT_FEAT_GPIO_API */

EXPORT_SYMBOL(mxc_request_gpio);
EXPORT_SYMBOL(mxc_free_gpio);
EXPORT_SYMBOL(mxc_set_gpio_direction);
EXPORT_SYMBOL(mxc_set_gpio_dataout);
EXPORT_SYMBOL(mxc_get_gpio_datain);

/* For backward compatible */
EXPORT_SYMBOL(gpio_config);
EXPORT_SYMBOL(gpio_set_data);
EXPORT_SYMBOL(gpio_get_data);
EXPORT_SYMBOL(gpio_clear_int);
EXPORT_SYMBOL(gpio_request_irq);
EXPORT_SYMBOL(gpio_free_irq);
#if defined(CONFIG_MOT_FEAT_GPIO_API)
EXPORT_SYMBOL(gpio_free_irq_dev);
#endif /* CONFIG_MOT_FEAT_GPIO_API */

postcore_initcall(mxc_gpio_init);
