/*
 * linux/kernel/gpio.c
 *
 * (C) 2004 Robert Schwebel, Pengutronix
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>

#define DRIVER_NAME "gpio"

static char mapping[255] = "";

MODULE_AUTHOR("John Lenz, Robert Schwebel");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Generic GPIO Infrastructure");
module_param_string(mapping, mapping, sizeof(mapping), 0);
MODULE_PARM_DESC(mapping,
"period delimited options string to map GPIO pins to userland:\n"
"\n"
"	<pin>:[out|in][hi|lo]\n"
"\n"
"	example: mapping=5:out:hi.8:in\n"	
);

static ssize_t gpio_show_level(struct class_device *dev, char *buf);
static ssize_t gpio_store_level(struct class_device *dev, const char *buf, size_t size);
static ssize_t gpio_show_policy(struct class_device *dev, char *buf);

struct gpio_properties {
	unsigned int       pin_nr;
	unsigned char      policy;	/* GPIO_xxx */
	char               pin_level;	/* -1=tristate, 0, 1 */
	char               owner[20];
	struct gpio_device *gpio_dev;
};

struct gpio_device {
	spinlock_t lock; 		/* protects the props field */
	struct gpio_properties props;
	struct class_device class_dev;
	struct list_head list;
};
#define to_gpio_device(d) container_of(d, struct gpio_device, class_dev)

static LIST_HEAD(gpio_list);
static rwlock_t gpio_list_lock = RW_LOCK_UNLOCKED;

/* gpio_device is static, so we don't have to free it here */
static void gpio_class_release(struct class_device *dev)
{
	return;
}

static struct class gpio_class = {
	.name		= "gpio",
	.release	= gpio_class_release,
};


/* 
 * Attribute: /sys/class/gpio/gpioX/level 
 */
static struct class_device_attribute attr_gpio_level = {
	.attr = { .name = "level", .mode = 0644, .owner = THIS_MODULE },
	.show = gpio_show_level,
	.store = gpio_store_level,
};

static ssize_t gpio_show_level(struct class_device *dev, char *buf)
{
	struct gpio_device *gpio_dev = to_gpio_device(dev);
	ssize_t ret_size = 0;

	if (gpio_dev->props.policy & GPIO_INPUT)
		gpio_dev->props.pin_level = gpio_get_pin(gpio_dev->props.pin_nr);
	
	spin_lock(&gpio_dev->lock);
	ret_size += sprintf(buf, "%i\n", gpio_dev->props.pin_level);
	spin_unlock(&gpio_dev->lock);

	return ret_size;
}

static ssize_t gpio_store_level(struct class_device *dev, const char *buf, size_t size)
{
	struct gpio_device *gpio_dev = to_gpio_device(dev);
	long value;

	if (gpio_dev->props.policy & GPIO_INPUT) 
		return -EINVAL;
	
	value = simple_strtol(buf, NULL, 10);
	if ((value != 0) && (value != 1)) 
		return -EINVAL;
	gpio_dev->props.pin_level = value;

	/* set real hardware */
	switch (value) {
		case 0:  gpio_clear_pin(gpio_dev->props.pin_nr); 
			 gpio_dir_output(gpio_dev->props.pin_nr); 
			 break;
		case 1:  gpio_set_pin(gpio_dev->props.pin_nr); 
			 gpio_dir_output(gpio_dev->props.pin_nr);
			 break;
		default: break;
	}
	return size;
}

/* 
 * Attribute: /sys/class/gpio/gpioX/policy 
 */
static struct class_device_attribute attr_gpio_policy = {
	.attr = { .name = "policy", .mode = 0444, .owner = THIS_MODULE },
	.show = gpio_show_policy,
	.store = NULL, 
};

static ssize_t gpio_show_policy(struct class_device *dev, char *buf)
{
	struct gpio_device *gpio_dev = to_gpio_device(dev);
	ssize_t ret_size = 0;
	
	spin_lock(&gpio_dev->lock);
	if (gpio_dev->props.policy & GPIO_USER)
		ret_size += sprintf(buf,"userspace\n");
	if (gpio_dev->props.policy & GPIO_KERNEL)
		ret_size += sprintf(buf,"kernel\n");
	spin_unlock(&gpio_dev->lock);

	return ret_size;
}

#ifdef CONFIG_PM
/* 
 * Attribute: /sys/class/gpio/gpioX/wakeup
 */

static ssize_t gpio_show_wakeup(struct class_device *dev, char *buf);
static ssize_t gpio_store_wakeup(struct class_device *dev, const char *buf, size_t size);

static struct class_device_attribute attr_gpio_wakeup = {
	.attr = { .name = "wakeup", .mode = 0644, .owner = THIS_MODULE },
	.show = gpio_show_wakeup,
	.store = gpio_store_wakeup,
};

static const char enabled[] = "enabled";
static const char disabled[] = "disabled";

static ssize_t gpio_show_wakeup(struct class_device *dev, char *buf)
{
	struct gpio_device *gpio_dev = to_gpio_device(dev);
	ssize_t ret_size = 0;

	ret_size +=  sprintf(buf, "%s\n", 
			     gpio_can_wakeup(gpio_dev->props.pin_nr) ?
			     (gpio_get_wakeup_enable(gpio_dev->props.pin_nr)
			      ? enabled : disabled)
			     : "");
	return ret_size;
}

static ssize_t gpio_store_wakeup(struct class_device *dev, const char *buf, size_t size)
{
	struct gpio_device *gpio_dev = to_gpio_device(dev);
	char *cp;
	int len = size;

	if (!gpio_can_wakeup(gpio_dev->props.pin_nr))
		return -EINVAL;

	cp = memchr(buf, '\n', len);
	if (cp)
		len = cp - buf;
	if (len == sizeof enabled - 1
			&& strncmp(buf, enabled, sizeof enabled - 1) == 0)
		gpio_set_wakeup_enable(gpio_dev->props.pin_nr, 1);
	else if (len == sizeof disabled - 1
			&& strncmp(buf, disabled, sizeof disabled - 1) == 0)
		gpio_set_wakeup_enable(gpio_dev->props.pin_nr, 0);
	else
		return -EINVAL;

	return size;
}
#endif

static int gpio_read_proc(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	char *p = page;
	int size = 0;
	struct gpio_device *gpio_dev;
	struct list_head *lact, *ltmp;

	if (off != 0)
		goto end;

	p += sprintf(p, "GPIO   POLICY       DIRECTION    OWNER\n");
	list_for_each_safe(lact, ltmp, &gpio_list) {
		gpio_dev = list_entry(lact, struct gpio_device, list);
		p += sprintf(p, "%3i:   ", gpio_dev->props.pin_nr);
		if (gpio_dev->props.policy & GPIO_KERNEL)
			p += sprintf(p, "kernel       ");
		if (gpio_dev->props.policy & GPIO_USER)
			p += sprintf(p, "user space   ");
		if (gpio_dev->props.policy & GPIO_INPUT)
			p += sprintf(p, "input        ");
		if (gpio_dev->props.policy & GPIO_OUTPUT)
			p += sprintf(p, "output       ");
		p += sprintf(p, "%s\n", gpio_dev->props.owner);
	}
end:
	size = (p - page);
	if (size <= off + count)
		*eof = 1;
	*start = page + off;
	size -= off;
	if (size > count)
		size = count;
	if (size < 0)
		size = 0;

	return size;
}

/** 
 * request_gpio - register a new object of gpio_device class.  
 *
 * @pin_nr:     GPIO pin which is registered
 * @owner:      name of the driver that owns this pin
 * @policy:     set policy for this pin, which is one of these: 
 * 		- GPIO_USER or GPIO_KERNEL
 * 		- GPIO_INPUT or GPIO_OUTPUT
 * 		For user space registered pins a sysfs entry is added. 
 * @init_level: initially configured pin level
 */
int request_gpio(unsigned int pin_nr, const char *owner,
		 unsigned char policy, unsigned char init_level)
{
	int rc;
	struct gpio_device *gpio_dev;
	struct list_head *lact, *ltmp;

	list_for_each_safe(lact, ltmp, &gpio_list) {
		gpio_dev = list_entry(lact, struct gpio_device, list);
		if (pin_nr == gpio_dev->props.pin_nr) {
			printk(KERN_ERR "gpio pin %i is already used by %s\n",
			       pin_nr, gpio_dev->props.owner);
			return -EBUSY;
		}
	}

	/* value checks - FIXME: is there a logical xor in C? */
	if ( ( (policy & GPIO_USER) &&  (policy & GPIO_KERNEL)) ||
	     (!(policy & GPIO_USER) && !(policy & GPIO_KERNEL))) {
		printk(KERN_ERR "%s: policy has to be one of GPIO_KERNEL, GPIO_USER\n", DRIVER_NAME); 
		return -EINVAL;
	}
	if ( ( (policy & GPIO_INPUT) &&  (policy & GPIO_OUTPUT)) ||
	     (!(policy & GPIO_INPUT) && !(policy & GPIO_OUTPUT))) {
		printk(KERN_ERR "%s: policy has to be one of GPIO_INPUT, GPIO_OUTPUT\n", DRIVER_NAME); 
		return -EINVAL;
	}

	gpio_dev = kmalloc(sizeof(struct gpio_device), GFP_KERNEL);
	
	if (unlikely(!gpio_dev)) {
		printk(KERN_ERR "%s: couldn't allocate memory\n", DRIVER_NAME);
		return -ENOMEM;
	}

	spin_lock_init(&gpio_dev->lock);
	gpio_dev->props.pin_nr = pin_nr;
	gpio_dev->props.policy = policy;
	gpio_dev->props.pin_level = init_level;
	gpio_dev->props.gpio_dev = gpio_dev;
	strncpy(gpio_dev->props.owner, owner, 20);

	memset(&gpio_dev->class_dev, 0, sizeof(gpio_dev->class_dev));
	gpio_dev->class_dev.class = &gpio_class;
	snprintf(gpio_dev->class_dev.class_id, BUS_ID_SIZE, "gpio%i", pin_nr);

	rc = class_device_register(&gpio_dev->class_dev);
	if (unlikely(rc)) {
		printk(KERN_ERR "%s: class registering failed\n", DRIVER_NAME);
		kfree(gpio_dev);
		return rc;
	}

	INIT_LIST_HEAD(&gpio_dev->list);

	/* register the attributes */
	if (policy & GPIO_USER)
		class_device_create_file(&gpio_dev->class_dev, &attr_gpio_level);
	
	class_device_create_file(&gpio_dev->class_dev, &attr_gpio_policy);
#ifdef CONFIG_PM
	class_device_create_file(&gpio_dev->class_dev, &attr_gpio_wakeup);
#endif

	/* set real hardware */
	if (policy & GPIO_OUTPUT) {
		switch (init_level) {
			case 0: gpio_clear_pin(pin_nr); break;
			case 1: gpio_set_pin(pin_nr); break;
			default: break; 
		}
		gpio_dir_output(pin_nr); 
	}

	write_lock(&gpio_list_lock);
	list_add_tail(&gpio_dev->list, &gpio_list);
	write_unlock(&gpio_list_lock);
	
	printk(KERN_INFO "registered gpio%i\n", pin_nr);

	return 0;
}
EXPORT_SYMBOL(request_gpio);

/**
 * free_gpio - unregisters a object of gpio_properties class.
 *
 * @pin_nr: pin number to free. 
 *
 * Unregisters a previously registered via request_gpio object.
 */
void free_gpio(unsigned int pin_nr)
{
	struct gpio_device *gpio_dev;
	struct list_head *lact, *ltmp;

	list_for_each_safe(lact, ltmp, &gpio_list) {
		gpio_dev = list_entry(lact, struct gpio_device, list);
		if (pin_nr == gpio_dev->props.pin_nr) {

			printk(KERN_INFO "unregistering gpio pin %i\n", pin_nr);

			/* unregister attributes */
			if (gpio_dev->props.policy & GPIO_USER)
				class_device_remove_file(&gpio_dev->class_dev,
							 &attr_gpio_level);

			class_device_remove_file(&gpio_dev->class_dev, &attr_gpio_level);
#ifdef CONFIG_PM
			class_device_remove_file(&gpio_dev->class_dev, 
						 &attr_gpio_wakeup);
#endif

			class_device_unregister(&gpio_dev->class_dev);
			write_lock(&gpio_list_lock);
			list_del(&gpio_dev->list);
			write_unlock(&gpio_list_lock);
			kfree(gpio_dev);
			return;
		}
	}
}
EXPORT_SYMBOL(free_gpio);

void free_all_gpios(void)
{
	struct gpio_device *gpio_dev;
	struct list_head *lact, *ltmp;

	list_for_each_safe(lact, ltmp, &gpio_list) {
		gpio_dev = list_entry(lact, struct gpio_device, list);

		printk(KERN_INFO "unregistering gpio pin %i\n", gpio_dev->props.pin_nr);

		/* unregister attributes */
		if (gpio_dev->props.policy & GPIO_USER)
			class_device_remove_file(&gpio_dev->class_dev,
						 &attr_gpio_level);

		class_device_remove_file(&gpio_dev->class_dev, &attr_gpio_level);

		class_device_unregister(&gpio_dev->class_dev);
		write_lock(&gpio_list_lock);
		list_del(&gpio_dev->list);
		write_unlock(&gpio_list_lock);
		kfree(gpio_dev);
		return;
	}
}
EXPORT_SYMBOL(free_all_gpios);

#ifdef CONFIG_PM
static int gpio_resume(struct sys_device *dev)
{
	int pin = gpio_waker();

	if (pin >= 0) {
		struct gpio_device *gpio_dev;
		struct list_head *lact, *ltmp;

		list_for_each_safe(lact, ltmp, &gpio_list) {
			gpio_dev = list_entry(lact, struct gpio_device, list);

			if (gpio_dev->props.pin_nr == pin) {
				pm_set_class_waker(&gpio_dev->class_dev);
				goto out;
			}
		}
	}

out:
	return 0;
}
#else
static int gpio_resume(struct sys_device *dev)
{
	return 0;
}
#endif

static struct sysdev_class gpio_sysclass = {
	set_kset_name("gpio"),
	.resume = gpio_resume,
};

static struct sys_device gpio_sys_device = {
	.id		= 0,
	.cls		= &gpio_sysclass,
};

static int gpio_setup(char *s)
{
	char *p, *q = NULL;
	int pin_nr;
	unsigned char policy;
	unsigned char init_level = 0;

	while ((p = strsep(&s, ".,")) != NULL) {

		/* end of command reached? -> next command */
		if (*p == '\0')
			continue;

		/* process one command */
		pin_nr = -1;
		policy = 0;
		q = strsep(&p, ":");
		if (*q == '\0') {
			printk("%s: invalid token (scanning pin_nr): %s\n", DRIVER_NAME, p); 
			continue;
		}
		pin_nr = simple_strtoul(q, NULL, 0);	/* FIXME: this doesn't detect if no number is given! */
		q = strsep(&p, ":");
		if (*q == '\0') {
			printk("%s: invalid token (scanning direction): %s\n", DRIVER_NAME, p); 
			continue;
		}
		policy |= strncmp(q, "in", strlen(q)) == 0 ? GPIO_INPUT : 0;
		policy |= strncmp(q, "out", strlen(q)) == 0 ? GPIO_OUTPUT : 0;
		if (policy == 0) {
			printk("%s: invalid token (scanning policy): %s\n", DRIVER_NAME, p); 
			continue;
		}
		if (policy & GPIO_OUTPUT) {
			q = strsep(&p, ":");
			if (*q == '\0') {
				printk("%s: invalid token (scanning init_level): %s\n", DRIVER_NAME, p); 
				continue;
			}
			init_level = 2;
			if (strncmp(q, "hi", strlen(q)) == 0)
				init_level = 1;
			if (strncmp(q, "lo", strlen(q)) == 0)
				init_level = 0;
			if (init_level == 2) {
				printk("%s: invalid token (scanning level_value): %s\n", DRIVER_NAME, p); 
				continue;
			}
		}
		policy |= GPIO_USER;
		if (request_gpio(pin_nr, "kernel", policy, init_level) != 0) {
			printk(KERN_ERR
			       "%s: could not register GPIO pins from commandline!\n",
			       DRIVER_NAME);
			return -EIO;
		}
	}
	return 0;
}

static int __init gpio_init(void)
{
	int ret;

	printk(KERN_INFO "Initialising gpio device class.\n");

	class_register(&gpio_class);

	ret = sysdev_class_register(&gpio_sysclass);
	if (ret) {
		printk(KERN_ERR "%s: couldn't register sysdev class, exiting\n", DRIVER_NAME);
		goto out_sysdev_class;
	}

	ret = sysdev_register(&gpio_sys_device);
	if (ret) {
		printk(KERN_ERR "%s: couldn't register sysdev, exiting\n", DRIVER_NAME);
		goto out_sysdev_register;
	}

        if (!create_proc_read_entry ("gpio", 0, 0, gpio_read_proc, NULL)) {
		printk(KERN_ERR "%s: couldn't register proc entry, exiting\n", DRIVER_NAME);
		goto out_proc;
	}

	/* parse command line parameters */
	printk(KERN_INFO "%s: mapping=", DRIVER_NAME);
	if (strcmp(mapping,"")) {
		printk("%s\n", mapping);
		if (gpio_setup(mapping) != 0) {
			printk(KERN_ERR "%s: could not register ('mapping=...'), exiting\n", DRIVER_NAME);
		}
	} else {
		printk("EMPTY\n"); 
	};

	return ret;

out_proc:
	sysdev_unregister(&gpio_sys_device);
out_sysdev_register:
	sysdev_class_unregister(&gpio_sysclass);
out_sysdev_class:
	class_unregister(&gpio_class);
	return ret;
}

module_init(gpio_init);
