/*
 * otg/otgcore/otg-compat.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-compat.h|20051116205001|17130
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
/*!
 * @file otg/otg/otg-compat.h
 * @brief Common include file to determine and include appropriate OS compatibility file.
 *
 *
 * @ingroup OTGCore
 */
#ifndef _OTG_COMPAT_H
#define _OTG_COMPAT_H 1

#if defined(_WIN32_WCE)
#define OTG_WINCE       _WIN32_WCE
#endif /* defined(_WIN32_WCE) */

        /* What operating system are we running under? */

        /* Recursively include enough information to determine which release */

        #if (__GNUC__ >=3)
        #define GCC3
        #else
        #define GCC2
        #endif
        #include <linux/config.h>
        #include <linux/kernel.h>
        #include <linux/version.h>

#if defined(__GNUC__)
        #define OTG_LINUX
        #ifndef CONFIG_OTG_NOC99
        #define OTG_C99
        #else
        #endif

        #if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,2)
        #define LINUX26
        #elif LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5)
        #define LINUX24
        #else /* LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5) */
        #define LINUX24
        #define LINUX_OLD
        #warning "Early unsupported release of Linux kernel"
        #endif /* LINUX_VERSION_CODE > KERNEL_VERSION(2,4,5) */

        /* We are running under a supported version of Linux */
        #include <otg/otg-linux.h>

#elif defined(OTG_WINCE)

        /* We are running under a supported version of WinCE */
        #include <otg/otg-wince.h>
	#include <otg/otg-wince-ex.h>

#else /* defined(OTG_WINCE) */

        #error "Operating system not recognized"

#endif /* defined(OTG_WINCE) */

#include <otg/otg-utils.h>

#if !defined(OTG_C99)
#else /* !defined(OTG_C99) */
#endif /* !defined(OTG_C99) */


#endif /* _OTG_COMPAT_H */
