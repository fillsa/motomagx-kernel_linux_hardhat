/*
 * Copyright 2006 Motorola, Inc.
 *
 * Date         Author          Comment
 * 10/2006      Motorola        Added minor device numbers for camera related 
 *              		devices CAM_MINOR 
 * 31-Oct-2006  Motorola        Added inotify
 *
 */

#ifndef _LINUX_MISCDEVICE_H
#define _LINUX_MISCDEVICE_H
#include <linux/module.h>
#include <linux/major.h>
#ifdef CONFIG_MOT_FEAT_INOTIFY
#include <linux/device.h>
#endif

#define PSMOUSE_MINOR  1
#define MS_BUSMOUSE_MINOR 2
#define ATIXL_BUSMOUSE_MINOR 3
/*#define AMIGAMOUSE_MINOR 4	FIXME OBSOLETE */
#define ATARIMOUSE_MINOR 5
#define SUN_MOUSE_MINOR 6
#define APOLLO_MOUSE_MINOR 7
#define PC110PAD_MINOR 9
/*#define ADB_MOUSE_MINOR 10	FIXME OBSOLETE */
#define WATCHDOG_MINOR		130	/* Watchdog timer     */
#define TEMP_MINOR		131	/* Temperature Sensor */
#define RTC_MINOR 135
#define EFI_RTC_MINOR		136	/* EFI Time services */
#define SUN_OPENPROM_MINOR 139
#define DMAPI_MINOR		140	/* DMAPI */
#define NVRAM_MINOR 144
#define SGI_MMTIMER        153
#define STORE_QUEUE_MINOR	155
#define I2O_MINOR 166
#define MICROCODE_MINOR		184
#define MWAVE_MINOR	219		/* ACP/Mwave Modem */
#define MPT_MINOR	220
#define CAMERA0_MINOR           240
#define CAMERA1_MINOR           241
#define CAM_MINOR               244     /* camera i2c */
#define LIGHT_SENSOR_MINOR      246     /* Light sensor */
#define MISC_DYNAMIC_MINOR 255

#define TUN_MINOR	     200
#define	HPET_MINOR	     228

struct device;

struct miscdevice 
{
	int minor;
	const char *name;
	struct file_operations *fops;
	struct list_head list;
	struct device *dev;
#ifdef CONFIG_MOT_FEAT_INOTIFY
	struct class_device *class;
#endif
	char devfs_name[64];
};

extern int misc_register(struct miscdevice * misc);
extern int misc_deregister(struct miscdevice * misc);

#define MODULE_ALIAS_MISCDEV(minor)				\
	MODULE_ALIAS("char-major-" __stringify(MISC_MAJOR)	\
	"-" __stringify(minor))
#endif
