/*
 *  linux/init/version.c
 *
 *  Copyright (C) 1992  Theodore Ts'o
 *  Copyright 2006 Motorola, Inc.
 *
 *  May be freely distributed as part of Linux.
 *
 * Date         Author          Comment
 * 10/2006      Motorola        Added constants to make checksumming the
 *				zImage comparable between different builds
 */

#include <linux/compile.h>
#include <linux/module.h>
#include <linux/uts.h>
#include <linux/utsname.h>
#include <linux/version.h>

#define version(a) Version_ ## a
#define version_string(a) version(a)

int version_string(LINUX_VERSION_CODE);

struct new_utsname system_utsname = {
	.sysname	= UTS_SYSNAME,
	.nodename	= UTS_NODENAME,
	.release	= UTS_RELEASE,
#ifdef CONFIG_MOT_FEAT_CHKSUM
	.version	= "",
#else
	.version	= UTS_VERSION,
#endif
	.machine	= UTS_MACHINE,
	.domainname	= UTS_DOMAINNAME,
};

EXPORT_SYMBOL(system_utsname);

#ifdef CONFIG_MOT_FEAT_CHKSUM
const char *linux_banner = 
	"Linux version " UTS_RELEASE  
	"(" LINUX_COMPILER ")\n";
#else
const char *linux_banner = 
	"Linux version " UTS_RELEASE " (" LINUX_COMPILE_BY "@"
	LINUX_COMPILE_HOST ") (" LINUX_COMPILER ") " UTS_VERSION "\n";
#endif
