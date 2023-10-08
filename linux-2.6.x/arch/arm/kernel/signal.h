/*
 *  linux/arch/arm/kernel/signal.h
 *
 *  Copyright (C) 2005 Russell King.
 *  Copyright 2007 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Date     Author     Comment
 * 02/2007  Motorola   Created file from patch from 2.6.13-rc1 to move
 *                     sigreturn code to exception vector page
 * 02/2007  Motorola   Added definitions for moving sys_restart to 
 *                     exception vector page
 *
 */
#define KERN_SIGRETURN_CODE	0xffff0500
#define KERN_SYSRESTART_CODE	0xffff0550

extern const unsigned long sigreturn_codes[7];
extern const unsigned long sysrestart_codes[2];
