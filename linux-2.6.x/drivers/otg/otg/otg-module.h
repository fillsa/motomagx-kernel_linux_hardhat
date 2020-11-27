/*
 * otg/otg/otg-module.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-module.h|20051116205001|27906
 *
 *      Copyright (c) 2004 Belcarra
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
 * @file otg/otg/otg-module.h
 * @brief Linux Module OS Compatibility defines
 *
 *
 * @ingroup OTGCore
 */
#ifndef _OTG_MODULE_H
#define _OTG_MODULE_H 1

#if defined(OTG_LINUX)
    #if !defined(_LINUX_MODULE_H)
        #include <linux/module.h>
    #endif /* _LINUX_MODULE_H */
    
    #if defined(MODULE)

        #define MOD_EXIT(exit_routine) module_exit(exit_routine)
        #define MOD_INIT(init_routine) module_init(init_routine)
        #define MOD_PROC(proc) (proc)
        #define MOD_AUTHOR(string) MODULE_AUTHOR(string)
        #define MOD_PARM(param, type) MODULE_PARM(param, type)
        #define MOD_PARM_DESC(param,desc) MODULE_PARM_DESC(param, desc)
        #define MOD_DESCRIPTION(description) MODULE_DESCRIPTION(description)
        #define OTG_EXPORT_SYMBOL(symbol) EXPORT_SYMBOL(symbol)
        #define OTG_EPILOGUE  1 /* EPILOGUE ROUTINE NEEDED */
    
    #else /* defined(MODULE) */

        #define MOD_EXIT(exit_routine) 
        #define MOD_INIT(init_routine) module_init(init_routine)
        #define MOD_PROC(proc) NULL
        #define MOD_AUTHOR(string) 
        #define MOD_PARM(param, type)
        #define MOD_PARM_DESC(param,desc)
        #define MOD_DESCRIPTION(description) 
        #define OTG_EXPORT_SYMBOL(symbol) //EXPORT_SYMBOL(symbol)
        #undef EXPORT_SYMBOL
        #define OTG_EPILOGUE 0  /* EPILOGUE ROUTINE NOT NEEDED */

    #endif /* defined(MODULE) */
    
    //#undef MODULE_AUTHOR
    //#undef MODULE_DESCRIPTION
    //#undef MODULE_PARM_DESC
    //#undef MODULE_PARM
    #if defined(LINUX24) || defined(LINUX26)
        #include <linux/version.h>
        #if defined(MODULE) && (LINUX_VERSION_CODE >= KERNEL_VERSION (2,4,17))
        //"GPL License applies under certain cirumstances; consult your vendor for details"
            #define EMBED_LICENSE() MODULE_LICENSE ("GPL")
        #else 
            #define EMBED_LICENSE()   //Operation not supported for earlier Linux kernels
        #endif

    #else /* defined(LINUX24) || defined(LINUX26) */
        #error "Need to define EMBED_LICENSE for the current operating system"
    #endif /* defined(LINUX24) || defined(LINUX26) */
    #define EMBED_MODULE_INFO(section,moduleinfo) static char __##section##_module_info[] = moduleinfo "balden@seth2.belcarratech.com"
    #define EMBED_USBD_INFO(moduleinfo) EMBED_MODULE_INFO(usbd,moduleinfo)
    #define GET_MODULE_INFO(section) __##section##_module_info
#endif /* defined(OTG_LINUX) */
    
    
#endif
