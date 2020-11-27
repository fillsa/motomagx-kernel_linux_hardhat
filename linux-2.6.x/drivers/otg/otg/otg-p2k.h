/*
 * otg/otg/otg-p2k.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-p2k.h|20051116205001|30862
 *
 *      Copyright (c) 2004-2005 Belcarra
 */
/*
 *  Copyright 2005-2006, Motorola, All Rights Reserved.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 */
/*!
 * @file otg/otg/otg-p2k.h
 * @brief Linux OS Compatibility defines
 *
 * @ingroup OTGCore
 */

#ifndef _OTG_P2K_H 1
#define _OTG_P2K_H 1

/*! @name Common Motorola includes
 */
/*@{*/

/* UNKNOWN SO FAR */
/*@}*/

/*! @name  Compilation Related
 */

/*@{*/
#define PRAGMAPACK
#define PACKED 
#define INLINE __inline

/*@}*/

/*!@name OTG int typedefs
 */
/*@{*/
typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned long int u32;
typedef __int64 u64;
typedef unsigned char __u8;
typedef unsigned short __u16;
typedef unsigned long int __u32;
typedef __int64 __u64;

/*@}*/

/*!@name  Memory Allocation Primitives
 *
 * CKMALLOC()
 * LSTRDUP()
 * LKFREE()
 * LIST_ENTRY()
 */
/*! @{ */

#define GFP_KERNEL 0
#define GFP_ATOMIC 0

#define CKMALLOC(n,f) _ckmalloc(__FILE__, __LINE__, n, f)
#define LSTRDUP(str) _lstrdup(__FILE__, __LINE__, str)
#define LKFREE(p) _lkfree(__FILE__, __LINE__, p)

#define OTG_MALLOC_TEST
#undef OTG_MALLOC_DEBUG

#ifdef OTG_MALLOC_TEST
    extern int otg_mallocs;
#endif

static INLINE void *_ckmalloc (const char *func, int line, int n, int f)
{

	return 0;
}
static INLINE char *_lstrdup (const char *func, int line, char *str)
{

	return 0;
}
static INLINE void _lkfree (const char *func, int line, void *p)
{
}

// XXX 
#define LIST_ENTRY(p, t, m) \
        ((t *)((char *)(p)-(unsigned long)(&((t *)0)->m)))

// XXX
#define LIST_FOR_EACH(p, h) \
        for (p = (h)->next, (p->next); p != (h); \
                p = p->next, (p->next))

#define LIST_ADD_TAIL(n,h) (0)
#define LIST_ADD_TAIL(n,h) (0)
#define LIST_DEL(h) (0)


/*! @} */


/*! @name Atomic Operations
 *
 * atomic_post_inc()
 * atomic_pre_dec()
 */
/*! @{ */

/* @} */



/*! @name Scheduling Primitives
 *
 * WORK_STRUCT
 * WORK_ITEM
 *
 * SCHEDULE_TIMEOUT()
 * SET_WORK_ARG()
 * SCHEDULE_WORK()
 * SCHEDULE_IMMEDIATE_WORK()
 * NO_WORK_DATA()
 * MOD_DEC_USE_COUNT
 * MOD_INC_USE_COUNT
 */
/*! @{ */
#define HZ      60
#define SCHEDULE_TIMEOUT(seconds)

    #define SCHEDULE_WORK(item) (0)
#define PENDING_WORK_ITEM(item) (0)
#define PREPARE_WORK_ITEM(__item,__routine,__data) (0)

/*! @} */


/*! @name Semaphores
 *
 * up()
 * down()
 */
/*! @{ */
#define UP(s) 
#define DOWN(s) 
/*! @} */


/*!@name Printk
 *
 * PRINTK()
 */
/*! @{ */
//#define PRINTK(s) printk(s)
//          DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s"), _T(msg)));

/*! @} */

/*! @name Little Endian Macros
 * /
 *! @{ 
 */

__inline u16 cpu_to_le16 ( u16 x )
{
        return (x<<8) | (x>>8);
}

__inline u16 le16_to_cpu ( u16 val )
{
        return ((((val) >> 8) & 0xff) | (((val) & 0xff) << 8));
}

__inline u32 le32_to_cpu (u32 val)
{
        return (((val) & 0xff000000) >> 24) | (((val) & 0x00ff0000) >>  8) |
                (((val) & 0x0000ff00) <<  8) | (((val) & 0x000000ff) << 24);
}
/*! @} */

/*! @name little endian macros
 */
/*! @{ */
#define local_irq_save(f) f = 0; __asm cli
#define local_irq_restore(f) __asm sti

/*! @} */

/*! @name Ioctl
 */
/*! @{ */
#define _IOR(m,n,s) ((m)|(n))
#define _IOW(m,n,s) ((m)|(n))
#define _IOCSIZE(a) (sizeof(a))
#define copy_from_user(d,s,n) memcpy(d,s,n)
#define copy_to_user(d,s,n) memcpy(d,s,n)

/*! @} */

/*! @name ERRNO
 */
/*! @{ */
#define EINVAL  WSAEINVAL

/*! @} */



#endif /* _OTG_P2K_H */
