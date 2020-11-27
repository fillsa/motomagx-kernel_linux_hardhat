/*
 * otg/otg/otg-linux.h
 * @(#) sl@belcarra.com|otg/otg/otg-linux.h|20060403224914|05355
 *
 *      Copyright (c) 2004-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * Copyright (c) 2005-2008 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 04/25/2006         Motorola         interrupt disabled bug
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 11/23/2007         Motorola         Changes for OSS compliance.
 * 29/09/2008         Motorola         fix AP kernel panic by taking care of kfree pointers 
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
 */

/*!
 * @file otg/otg/otg-linux.h
 * @brief Linux OS Compatibility defines
 *
 * @ingroup OTGCore
 */
#ifndef _OTG_LINUX_H
#define _OTG_LINUX_H 1

#if !defined(_OTG_MODULE_H)
#include <otg/otg-module.h>
#endif


#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <asm/types.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/random.h>
#include <linux/sched.h>

#if !defined(LINUX26) && !defined(LINUX24)
#error "One of LINUX24 and LINUX26 needs to be defined here"
#endif

#if defined(LINUX26)
#include <linux/device.h>
#endif

/*! @name Compilation Related
 */

/*@{*/
#undef PRAGMAPACK
/** Macros to support packed structures **/
#define PACKED1 __attribute__((packed))
#define PACKED2
#define PACKED0

/** Macros to support packed enum's **/
#define PACKED_ENUM enum
#define PACKED_ENUM_EXTRA

#define INLINE __inline__

/*@}*/

#if defined(LINUX26)
/*! @name OTG Bus Type
 */
/*@{*/
extern struct bus_type otg_bus_type;
/*@}*/
#endif /* defined(LINUX26) */

/*! @name Memory Allocation Primitives
 *
 * CKMALLOC()
 * LSTRDUP()
 * LKFREE()
 * LIST_ENTRY()
 * LIST_FOR_EACH()
 */

 /*! @{ */

#if defined(LINUX26)
    #include <linux/gfp.h>
#define GET_KERNEL_PAGE()  __get_free_page(GFP_KERNEL)

#else /* LINUX26 */

    #include <linux/mm.h>
    #define GET_KERNEL_PAGE() get_free_page(GFP_KERNEL)
#endif /* LINUX26 */



// Common to all supported versions of Linux ??

#define CKMALLOC(n,f) _ckmalloc(__FUNCTION__, __LINE__, n, f)
#define LSTRDUP(str) _lstrdup(__FUNCTION__, __LINE__, str)
#define LKFREE(p) _lkfree(__FUNCTION__, __LINE__, p)

#define ckmalloc(n,f) _ckmalloc(__FUNCTION__, __LINE__, n, f)
#define lstrdup(str) _lstrdup(__FUNCTION__, __LINE__, str)
#define lkfree(p) _lkfree(__FUNCTION__, __LINE__, p)

#define OTG_MALLOC_TEST
#undef OTG_MALLOC_DEBUG

#ifdef OTG_MALLOC_TEST
    extern int otg_mallocs;
#endif


static INLINE void *_ckmalloc (const char *func, int line, int n, int f)
{
    void *p;
    if ((p = kmalloc (n, f)) == NULL) {
        return NULL;
    }
    memset (p, 0, n);
    #ifdef OTG_MALLOC_TEST
    ++otg_mallocs;
    #endif
    #ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s: %p %s %d %d\n", __FUNCTION__, p, func, line, otg_mallocs);
    #endif
    return p;
}

static INLINE char *_lstrdup (const char *func, int line, char *str)
{
    int n;
    char *s;
    if (str && (n = strlen (str) + 1) && (s = kmalloc (n, GFP_ATOMIC))) {
#ifdef OTG_MALLOC_TEST
        ++otg_mallocs;
#endif
#ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s: %p %s %d %d\n", __FUNCTION__, s, func, line, otg_mallocs);
#endif
        return strcpy (s, str);
    }
    return NULL;
}

static INLINE void _lkfree (const char *func, int line, void *p)
{
    if (p) {
#ifdef OTG_MALLOC_TEST
        --otg_mallocs;
#endif
#ifdef OTG_MALLOC_DEBUG
        printk(KERN_INFO"%s: %p %s %d %d\n", __FUNCTION__, p, func, line, otg_mallocs);
#endif
        kfree (p);
        //set p to NULL, together with NULL check to avoid the issue of it being able to be kfree'd again
	p = NULL;
#ifdef MALLOC_TEST
        if (otg_mallocs < 0) {
                printk(KERN_INFO"%s: %p %s %d %d otg_mallocs less zero!\n", __FUNCTION__, p, func, line, otg_mallocs);
        }
#endif
#ifdef OTG_MALLOC_DEBUG
        else {
            printk(KERN_INFO"%s: %s %d NULL\n", __FUNCTION__, func, line);
        }
#endif
    }
    else { 
        //display warning message against freeing a NULL pointer
        printk("USB ::%s :: WARNING :: Pointer is NULL, do not free it\n", __FUNCTION__);
    }
}

#if 1
#include <linux/list.h>
#define LIST_NODE struct list_head
#define LIST_NODE_INIT(name) struct list_head name = {&name, &name}
#define INIT_LIST_NODE(ptr) INIT_LIST_HEAD(ptr)
#define LIST_ENTRY(pointer, type, member) list_entry(pointer, type, member)
#define LIST_FOR_EACH(cursor, head) list_for_each(cursor, head)
#define LIST_ADD_TAIL(n,h) list_add_tail(n,h)
#define LIST_DEL(h) list_del(&(h))
#else
#include <otg/otg-list.h>
#endif

/*! @} */


/*! @name Atomic Operations
 *
 * atomic_post_inc()
 * atomic_pre_dec()
 *
 * XXX Need to define otg_atomic_t so these are portable
 */
/*! @{ */
typedef atomic_t otg_atomic_t;
static __inline__ int atomic_post_inc(volatile otg_atomic_t *v)
{
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = (v->counter)++;
        local_irq_restore(flags);
        return(result);
}

static __inline__ int atomic_pre_dec(volatile otg_atomic_t *v)
{
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = --(v->counter);
        local_irq_restore(flags);
        return(result);
}

static __inline__ int atomic_and_read(volatile otg_atomic_t *v, int v_and_flag)
{
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = (v->counter) & v_and_flag;
        local_irq_restore(flags);
        return(result);
}

static __inline__ void atomic_and_set(volatile otg_atomic_t *v, int v_and_flag)
{
        unsigned long flags;
        local_irq_save(flags);
        v->counter = (v->counter) & v_and_flag;
        local_irq_restore(flags);
}

static __inline__ int atomic_or_read(volatile otg_atomic_t *v, int v_and_flag)
{
        unsigned long flags;
        int result;
        local_irq_save(flags);
        result = (v->counter) | v_and_flag;
        local_irq_restore(flags);
        return(result);
}

static __inline__ void atomic_or_set(volatile otg_atomic_t *v, int v_and_flag)
{
        unsigned long flags;
        local_irq_save(flags);
        v->counter = (v->counter) | v_and_flag;
        local_irq_restore(flags);
}

/*! @} */


/*!@name wait
 */
/*! @{ */
/*
*  Cond Wait Interruptible Timeout Irqrestore
*
*  Do spin_unlock_irqrestore and interruptible_sleep_on_timeout
*  so that wake ups are not lost if they occur between the unlock
*  and the sleep.  In other words, spin_unlock_irqrestore and
*  interruptible_sleep_on_timeout are "atomic" with respect to
*  wake ups.  This is used to implement condition variables.
*/

static inline long cond_wait_interruptible_timeout_irqrestore(
        wait_queue_head_t *q, long timeout,
        spinlock_t *lock, unsigned long flags )
{

        wait_queue_t wait;

        init_waitqueue_entry( &wait, current );

        set_current_state( TASK_INTERRUPTIBLE );

        add_wait_queue( q, &wait );

        spin_unlock_irqrestore( lock, flags );

        timeout = schedule_timeout(timeout);

        remove_wait_queue( q, &wait );

        return( timeout );

}



/*! @} */

/*!@name Scheduling Primitives
 *
 * WORK_STRUCT
 * WORK_ITEM
 *
 * SCHEDULE_TIMEOUT()\n
 * SET_WORK_ARG()\n
 * SCHEDULE_WORK()\n
 * SCHEDULE_IMMEDIATE_WORK()\n
 * NO_WORK_DATA()\n
 * MOD_DEC_USE_COUNT\n
 * MOD_INC_USE_COUNT\n
 */

/*! @{ */

static void inline SCHEDULE_TIMEOUT(int seconds){
        schedule_timeout( seconds * HZ );
}


/* Separate Linux 2.4 and 2.6 versions of scheduling primitives */
#if defined(LINUX26)
    #include <linux/workqueue.h>

    #define WORK_STRUCT  work_struct
    #define WORK_ITEM    work_struct
    typedef struct WORK_ITEM WORK_ITEM;
    #if 0
        #define PREPARE_WORK_ITEM(__item,__routine,__data) INIT_WORK((__item),(__routine),(__data))
    #else
        #include <linux/interrupt.h>
        #define PREPARE_WORK_ITEM(__item,__routine,__data) __prepare_work(&(__item),(__routine),(__data))
        static inline void __prepare_work(struct work_struct *_work,
            void (*_routine),
            void * _data){
            INIT_LIST_HEAD(&_work->entry);
            _work->pending = 0;
            _work->func = _routine;
            _work->data = _data;
            init_timer(&_work->timer);
        }
    #endif
    #undef PREPARE_WORK
    typedef void (* WORK_PROC)(void *);

    #define SET_WORK_ARG(__item, __data) (__item).data = __data

//LYN    #define SCHEDULE_DELAYED_WORK(item) schedule_delayed_work(&item, 0)
    #define SCHEDULE_DELAYED_WORK(item)	queue_delayed_work(otg_wqrs,&item,0)

    #define SCHEDULE_WORK(item) schedule_work(&item)
    #define SCHEDULE_IMMEDIATE_WORK(item) SCHEDULE_WORK((item))


    extern struct workqueue_struct * otg_workqueue;
    extern struct workqueue_struct * otg_wqrs;

    #define DEBUS_SCHEDULE_WORK(item) queue_work(otg_workqueue,&item)

    #define PENDING_WORK_ITEM(item) (item.pending != 0)
    #define NO_WORK_DATA(item) (!item.data)
    #define _MOD_DEC_USE_COUNT   //Not used in 2.6
    #define _MOD_INC_USE_COUNT   //Not used in 2.6

    #define MODPARM(a) _##a

    #define MOD_PARM_BOOL(a, d, v) static int _##a = v; module_param_named(a, _##a, bool, 0); MODULE_PARM_DESC(a, d)
    #define MOD_PARM_INT(a, d, v) static int _##a = v; module_param_named(a, _##a, uint, 0); MODULE_PARM_DESC(a, d)
    #define MOD_PARM_STR(a, d, v) static char * _##a = v; module_param_named(a, _##a, charp, 0); MODULE_PARM_DESC(a, d)
    #define OTG_INTERRUPT  0   /* SA_INTERRUPT in some environments */
#else /* LINUX26 */

    #define WORK_STRUCT  tq_struct
    #define WORK_ITEM    tq_struct
    typedef struct WORK_ITEM WORK_ITEM;
    #define PREPARE_WORK_ITEM(item,work_routine,work_data) { item.routine = work_routine; item.data = work_data; }
    #define SET_WORK_ARG(__item, __data) (__item).data = __data
    #define NO_WORK_DATA(item) (!(item).data)

//LYN    #define SCHEDULE_WORK(item) schedule_task(&item)
    #define SCHEDULE_WORK(item)	queue_work(otg_wqrs,&item)

    #define PENDING_WORK_ITEM(item) (item.sync != 0)
    #define _MOD_DEC_USE_COUNT   MOD_DEC_USE_COUNT
    #define _MOD_INC_USE_COUNT   MOD_INC_USE_COUNT

    #define MODPARM(a) a

    #define MOD_PARM_BOOL(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    #define MOD_PARM_INT(a, d, v) static int a = v; MOD_PARM(a, "i"); MOD_PARM_DESC(a, d);
    #define MOD_PARM_STR(a, d, v) static char * a = v; MOD_PARM(a, "s"); MOD_PARM_DESC(a, d);

    typedef void (* WORK_PROC)(void *);

    #if !defined(IRQ_HANDLED)
        // Irq's
        typedef void irqreturn_t;
        #define IRQ_NONE
        #define IRQ_HANDLED
        #define IRQ_RETVAL(x)
    #endif
    #define OTG_INTERRUPT  0   /* SA_INTERRUPT in some environments */
#endif /* LINUX26 */

/*! @} */

/*!@name Semaphores
 *
 * up()
 * down()
 */
/*! @{ */
#define UP(s) up(s)
#define DOWN(s) down(s)
/*! @} */

/*! @name Printk
 *
 * PRINTK()
 */
/*! @{ */
#define PRINTK(s) printk(s)
/*! @} */

/*! @name Cache
 */
/*@{*/
#define CACHE_SYNC_RCV(buf, len) pci_map_single (NULL, (void *) buf, len, PCI_DMA_FROMDEVICE)
#if defined(CONFIG_OTG_NEW_TX_CACHE)
#define CACHE_SYNC_TX(buf, len) consistent_sync (NULL, buf, len, PCI_DMA_TODEVICE)
#else
#define CACHE_SYNC_TX(buf, len) consistent_sync (buf, len, PCI_DMA_TODEVICE)
#endif

/*@}*/
#endif /* _OTG_LINUX_H */
