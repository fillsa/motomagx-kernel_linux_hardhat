/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file linux_port.h
 *
 * @brief OS_PORT ported to Linux (2.6.9+ for now)
 *
 * ==Linux version of== Generic OS API for STC Drivers
 *
 * @section intro_sec Introduction
 *
 * This API / kernel programming environment blah blah.
 *
 * See "Driver-to-Kernel Operations" as a good place to start.
 *
 * @todo  Rearrange so that Doxygen comments for all of this comes from
 * a common file and can be maintained in one place, instead of with each OS?
 *
 * @ingroup MXCSAHARA2
 */

#ifndef LINUX_PORT_H
#define LINUX_PORT_H

/* Linux Kernel Includes */
#if defined(CONFIG_MODVERSIONS) && ! defined(MODVERSIONS)
#include <linux/modversions.h>
#define MODVERSIONS
#endif
/* __NO_VERSION__ defined due to Kernel module spanning multiple files */
#define __NO_VERSION__

#include <linux/version.h>          /* Current version Linux kernel */
#include <linux/module.h>           /* Basic support for loadable modules,
                                       printk */
#include <linux/init.h>             /* module_init, module_exit */
#include <linux/kernel.h>           /* General kernel system calls */
#include <linux/sched.h>            /* for interrupt.h */
#include <linux/fs.h>               /* for inode */
#include <linux/random.h>

#include <asm/io.h>                 /* ioremap() */
#include <asm/irq.h>

#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <linux/vmalloc.h>          /* vmalloc */

#include <asm/uaccess.h>            /* copy_to_user(), copy_from_user() */

#include <stdarg.h>

#include <linux/device.h>           /* used in dynamic power management */

#include <asm/arch/clock.h>         /* clock en/disable for DPM */

#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


/* These symbols are defined in Linux 2.6 and later.  Include here for minimal
 * support of 2.4 kernel.
 **/
#if !defined(LINUX_VERSION_CODE) || LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
#define irqreturn_t void
#define IRQ_HANDLED
#define IRQ_RETVAL(x)
#endif


/*!
 * Type used for registering and deregistering interrupts.
 */
typedef int os_interrupt_id_t;


/*!
 * Type used as handle for a process
 *
 * See #os_get_process_handle() and #os_send_signal().
 */
/*
 * The following should be defined this way, but it gets compiler errors
 * on the current tool chain.
 *
 * typedef task_t *os_process_handle_t;
 */

typedef task_t *os_process_handle_t;



/*!
 * Generic return code for functions which need such a thing.
 *
 * No knowledge should be assumed of the value of any of these symbols except
 * that @c OS_ERROR_OK_S is guaranteed to be zero.
 *
 * (-EAGAIN?  -ERESTARTSYS?  Very Linux/Unix specific read()/write() values)
 */
typedef enum {
    OS_ERROR_OK_S = 0,         /*!< Success  */
    OS_ERROR_FAIL_S = -EIO,    /*!< Generic driver failure */
    OS_ERROR_NO_MEMORY_S = -ENOMEM /*!< Failure to acquire/use memory  */
} os_error_code;


/*!
 * Handle to a lock.
 */
typedef raw_spinlock_t* os_lock_t;

/*!
 * Context while locking.
 */
typedef unsigned long os_lock_context_t;

/*!
 * Handle to a queue
 */
typedef void* os_queue_t;

/*!
 * Driver registration handle
 *
 * Used with #os_driver_init_registration(), #os_driver_add_registration(),
 * and #os_driver_complete_registration().
 */
typedef struct file_operations os_driver_reg_t;


/*
 *  Function types which can be associated with driver entry points.
 *
 *  Note that init and shutdown are absent.
 */
#define OS_FN_OPEN open
#define OS_FN_CLOSE release
#define OS_FN_READ read
#define OS_FN_WRITE write
#define OS_FN_IOCTL ioctl
#define OS_FN_MMAP mmap

/*!
 * Function signature for the portable interrupt handler
 *
 * While it would be nice to know which interrupt is being serviced, the
 * Least Common Denominator rule says that no arguments get passed in.
 *
 * @return  Zero if not handled, non-zero if handled.
 */
typedef int (*os_interrupt_handler_t)(int, void*, struct pt_regs*);


/*!
 * @name Driver-to-Kernel Operations
 *
 * These are the operations which drivers should call to get the OS to perform
 * services.
 */
/*! @{ */
/*!
 * Register an interrupt handler.
 *
 * @param driver_name    The name of the driver
 * @param interrupt_id   The interrupt line to monitor (type
 *                       #os_interrupt_id_t)
 * @param function       The function to be called to handle an interrupt
 *
 * @todo Specify the signature of the generic interrupt handler, so that it
 * can be wrapped, if necessary, by the OS port.
 *
 * @return #os_error_code
 */
#define os_register_interrupt(driver_name, interrupt_id, function)            \
     request_irq(interrupt_id, function, 0, driver_name, NULL)



/*!
 * Deregister an interrupt handler.
 *
 * @param interrupt_id   The interrupt line to stop monitoring
 *
 * @return #os_error_code
 */
#define os_deregister_interrupt(interrupt_id)                                 \
     free_irq(interrupt_id, NULL)

/*!
 * Initialize driver registration.
 *
 * If the driver handles open(), close(), ioctl(), read(), write(), or mmap()
 * calls, then it needs to register their location with the kernel so that they
 * get associated with the device.
 *
 * @return A handle for further driver registration, or NULL if failed.
 */
#define os_driver_init_registration(handle)                                   \
({                                                                            \
    memset(&handle, sizeof(struct file_operations), 0);                       \
    handle.owner = THIS_MODULE;                                               \
})

/*!
 * Add a function registration to driver registration.
 *
 * @param handle    A handle returned by #os_driver_init_registration().
 * @param name      Which function is being supported.
 * @param function  The result of a call to a @c _REF version of one of the
 *                  driver function signature macros
 */
#define os_driver_add_registration(handle, name, function)                    \
    handle.name = (void*)(function)


/*!
 * Finalize the driver registration with the kernel.  The handle may not be
 * used after this call.
 *
 * @param handle   The driver registration information from
 *                 #os_driver_init_registration() and modified by
 *                 #os_driver_add_registration().
 * @param major       The major device number to be associated with the driver.
 * @param driver_name The driver name
 *
 * @return  An error code.
 */
#define os_driver_complete_registration(handle, major, driver_name)           \
({                                                                            \
    int code;                                                                 \
    code = register_chrdev(major, driver_name, &handle);                      \
    if (code < 0) {                                                           \
        code = OS_ERROR_FAIL_S;                                               \
    }                                                                         \
    code;                                                                     \
})

/*!
 * Remove the driver's registration with the kernel.
 *
 * Upon return from this call, the driver not receive any more calls at the
 * defined entry points (other than ISR and shutdown).
 *
 * @param major       The major device number to be associated with the driver.
 * @param driver_name The driver name
 *
 * @return  An error code.
 */
#define os_driver_remove_registration(major, driver_name)                     \
    unregister_chrdev(major, driver_name);

/*!
 * register to a driver
 *
 * this routine registers to the Linux Device Model
 *
 * @param   driver_information  The device_driver structure information
 *
 * @return  An error code.
 */
#define os_register_to_driver(driver_information)                             \
              driver_register(driver_information)

/*!
 * unregister from a driver
 *
 * this routine unregisters from the Linux Device Model
 *
 * @param   driver_information  The device_driver structure information
 *
 * @return  An error code.
 */
#define os_unregister_from_driver(driver_information)                         \
                driver_unregister(driver_information)

/*!
 * register a device to a driver
 *
 * this routine registers a drivers devices to the Linux Device Model
 *
 * @param   driver_information  The platform_device structure information
 *
 * @return  An error code.
 */
#define os_register_a_device(device_information)                              \
    platform_device_register(device_information)

/*!
 * unregister a device from a driver
 *
 * this routine unregisters a drivers devices from the Linux Device Model
 *
 * @param   driver_information  The platform_device structure information
 *
 * @return  An error code.
 */
#define os_unregister_a_device(device_information)                            \
    platform_device_unregister(device_information)

/*!
 * This function causes the current task to sleep on a queue until awoken.  It
 * takes as an argument a condition to test before going to sleep.  This must
 * be used to prevent deadlocks in certain situations.
 *
 * Example:  os_task_queue(my_queue, available_count == 0);
 *
 * @param queue      An #os_queue_t to sleep on.
 * @param condition  Some expression to be tested before going to sleep
 *
 * @return (void)
 */
#define os_task_queue_sleep(queue, condition)


/*!
 * Print a message to console / into log file.  After the @c msg argument a
 * number of printf-style arguments may be added.  Types should be limited to
 * printf string, char, octal, decimal, and hexadecimal types.  (This excludes
 * pointers, and floating point).
 *
 * @param  msg    The main text of the message to be logged
 * @param  ...    The printf-style arguments which go with msg, if any
 *
 * @return (void)
 */
/* This may be a GCC-ism which needs to be ported to ANSI */
#define os_printk(msg, s...)                                                  \
    (void) printk(msg, ## s)


/*!
 * @todo Need way to define, bind, and unbind the driver to inode / file
 * operations.
 */

/*!
 * @todo Need to define OS generic interface to tasklet/background task/bottom
 * half, as well as mechanisms to register them, start them, etc.
 */

/*!
 * @todo Need to define OS generic locks (#os_lock_t), and ways to create them,
 * initialize them, tickle them, etc.  (Dynamic or static or both?)
 */

/*!
 * @todo Need to define OS generic queues (#os_queue_t), and ways to create
 * them, initialize them, tickle them, etc.  (Dynamic or static or both?)
 */

/*!
 * @todo Need to define a way to put processes/tasks to sleep, and a way to
 * wake them back up again (singly or simultaneously?).
 */

/*!
 * Allocate some kernel memory
 *
 * @param amount   Number of 8-bit bytes to allocate
 * @param flags    Some indication of purpose of memory (needs definition)
 *
 * @return  Pointer to allocated memory, or NULL if failed.
 */
#define os_alloc_memory(amount, flags)                                        \
    (void*)kmalloc(amount, flags)


/*!
 * Free some kernel memory
 *
 * @param location  The beginning of the region to be freed.
 *
 * @todo Do some OSes have separate free() functions which should be
 * distinguished by passing in @c flags here, too? Don't some also require the
 * size of the buffer being freed?
 */
#define os_free_memory(location)                                              \
    kfree(location)


/*!
 * Map an I/O space into kernel memory space
 *
 * @param start       The starting address of the (physical / io space) region
 * @param range_bytes The number of bytes to map
 *
 * @return A return code indicating success or failure
 */
#define os_map_device(start, range_bytes)                                     \
    (void*)io_remap_nocache((void*)(start), range_bytes)


/*!
 * Copy data from Kernel space to User space
 *
 * @param to   The target location in user memory
 * @param from The source location in kernel memory
 * @param size The number of bytes to be copied
 *
 * @return #os_error_code
 */
#define os_copy_to_user(to, from, size)                                       \
    copy_to_user(to, from, size)


/*!
 * Copy data from User space to Kernel space
 *
 * @param to   The target location in kernel memory
 * @param from The source location in user memory
 * @param size The number of bytes to be copied
 *
 * @return #os_error_code
 */
#define os_copy_from_user(to, from, size)                                     \
    copy_from_user(to, from, size)


/*!
 * Read a 8-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
#define os_read8(register_address)                                            \
    __raw_readb(register_address)


/*!
 * Write a 8-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
#define os_write8(register_address, value)                                    \
    __raw_writeb(value, register_address)

/*!
 * Read a 16-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
#define os_read16(register_address)                                           \
    __raw_readw(register_address)


/*!
 * Write a 16-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
#define os_write16(register_address, value)                                   \
    __raw_writew(value, (uint32_t*)(register_address))

/*!
 * Read a 32-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
#define os_read32(register_address)                                           \
    __raw_readl((uint32_t*)(register_address))


/*!
 * Write a 32-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
#define os_write32(register_address, value)                                   \
    __raw_writel(value, register_address)

/*!
 * Read a 64-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
#define os_read64(register_address)                                           \
     ERROR_UNIMPLEMENTED


/*!
 * Write a 64-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
#define os_write64(register_address, value)                                   \
    ERROR_UNIMPLEMENTED

/*!
 * Delay some number of microseconds
 *
 * Note that this is a busy-loop, not a suspension of the task/process.
 *
 * @param  msecs   The number of microseconds to delay
 *
 * @return void
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/iobuf.h>
#define os_mdelay(msec)                                                       \
{                                                                             \
    unsigned long j = jiffies + (HZ * (msec) / 1000);                         \
    while (jiffies < j);                                                      \
}
#else
#define os_mdelay mdelay
#endif


/*!
 * Calculate virtual address from physical address
 *
 * @param pa    Physical address
 *
 * @return virtual address
 *
 * @note this assumes that addresses are 32 bits wide
 */
#if defined(LINUX_VERSION) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
#define os_va bus_to_virt
#else
#define os_va __va
#endif


/*!
 * Calculate physical address from virtual address
 *
 *
 * @param va    Virtual address
 *
 * @return physical address
 *
 * @note this assumes that addresses are 32 bits wide
 */
#if defined(LINUX_VERSION) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
#define os_pa virt_to_bus
#else
#define os_pa __pa
#endif


/*!
 * Allocate and initialize a lock, returning a lock handle.
 *
 * The lock state will be initialized to 'unlocked'.
 *
 * @return A lock handle, or NULL if an error occurred.
 */
#define os_lock_alloc_init()                                              \
({                                                                        \
    raw_spinlock_t* lockp;                                                \
    lockp = (raw_spinlock_t*)kmalloc(sizeof(raw_spinlock_t), 0);          \
    if (lockp) {                                                          \
        _raw_spin_lock_init(lockp);                                       \
    } else {                                                              \
        printk("lock init failed\n");                                     \
    }                                                                     \
    lockp;                                                                \
})

/*!
 * Acquire a lock.
 *
 * This function should only be called from an interrupt service routine.
 *
 * @param   lock_handle  A handle to the lock to acquire.
 *
 * @return void
 */
#define os_lock(lock_handle)                                              \
   spin_lock(lock_handle)


/*!
 * Unlock a lock.  Lock must have been acquired by #os_lock().
 *
 * @param   lock_handle  A handle to the lock to unlock.
 *
 * @return void
 */
#define os_unlock(lock_handle)                                            \
   spin_unlock(lock_handle)


/*!
 * Acquire a lock in non-ISR context
 *
 * This function will spin until the lock is available.
 *
 * @param lock_handle  A handle of the lock to acquire.
 * @param context      Place to save the before-lock context
 *
 * @returnn void
 */
#define os_lock_save_context(lock, context)                              \
    spin_lock_irqsave(lock, context)

/*!
 * Release a lock in non-ISR context
 *
 * @param lock_handle  A handle of the lock to release.
 * @param context      Place where before-lock context was saved.
 *
 * @returnn void
 */
#define os_unlock_restore_context(lock_handle, context)                  \
    spin_unlock_irqrestore(lock_handle, context)


/*!
 * Deallocate a lock handle.
 *
 * @param lock_handle   An #os_lock_t that has been allocated.
 *
 * @return void
 */
#define os_lock_deallocate(lock_handle)                                   \
    kfree(lock_handle)


/*!
 * Determine process handle
 *
 * The process handle of the current user is returned.
 *
 * @return   A handle on the current process.
 */
#define os_get_process_handle()                                           \
    current


/*!
 * Send a signal to a process
 *
 * @param  proc   A handle to the target process.
 * @param  sig    The POSIX signal to send to that process.
 */
#define os_send_signal(proc, sig)                                         \
    send_sig(sig, proc, 0);


/*!
 * Get some random bytes
 *
 * @param buf    The location to store the random data.
 * @param count  The number of bytes to store.
 *
 * @return  void
 */
#define os_get_random_bytes(buf, count)                                   \
    get_random_bytes(buf, count)

/** @} */ /* end Driver-to-Kernel Operations */


/******************************************************************************
 *  Function signature-generating macros
 *****************************************************************************/

/*!
 * @name Driver Signatures
 *
 * These macros will define the entry point signatures for interrupt handlers;
 * driver initialization and shutdown; device open/close; etc.
 *
 * There are two versions of each macro for a given Driver Entry Point.  The
 * first version is used to define a function and its implementation in the
 * driver.c file, e.g. #OS_DEV_INIT().
 *
 * The second form is used whenever a forward declaration (prototype) is
 * needed.  It has the letters @c _DCL appended to the name of the defintion
 * function, and takes only the first two arguments (driver_name and
 * function_name).  These are not otherwise mentioned in this documenation.
 *
 * There is a third form used when a reference to a function is required, for
 * instance when passing the routine as a pointer to a function.  It has the
 * letters @c _REF appended to it, and takes only the first two arguments
 * (driver_name and function_name).  These functions are not otherwise
 * mentioned in this documentation.
 *
 * (Note that these two extra forms are required because of the
 * possibility/likelihood of having a 'wrapper function' which invokes the
 * generic function with expected arguments.  An alternative would be to have a
 * generic function which isn't able to get at any arguments directly, but
 * would be equipped with macros which could get at information passed in.
 *
 * Example:
 *
 * (in a header file)
 * @code
 * OS_DEV_INIT_DCL(widget, widget_init);
 * @endcode
 *
 * (in an implementation file)
 * @code
 * OS_DEV_INIT(widget, widget_init)
 * {
 *     os_dev_init_return(TRUE);
 * }
 * @endcode
 *
 * @todo mmap() and 'bottom half' signatures.
 */
/*! @{ */
/*!
 * Define a function which will handle device initialization
 *
 * This is tne driver initialization routine.  This is normally where the
 * part would be initialized; queues, locks, interrupts handlers defined;
 * long-term dynamic memory allocated for driver use; etc.
 *
 * @param function_name   The name of the portable initialization function.
 *
 * @return  A call to #os_dev_init_return()
 *
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#define OS_DEV_INIT(function_name)                                            \
int __init init_module(void)

#else

#define OS_DEV_INIT(function_name)                                            \
module_init(function_name);                                                   \
static int __init function_name (void)
#endif

/*! asdfasdfsdf
 * @param function_name foo
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
int init_module(void);

#else

#define OS_DEV_INIT_DCL(function_name)                                        \
static int __init function_name (void);
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#define OS_DEV_INIT_REF(function_name)                                        \
init_module

#else

#define OS_DEV_INIT_REF(function_name)                                        \
function_name
#endif

/*!
 * Define a function which will handle device shutdown
 *
 * This is the inverse of the #OS_DEV_INIT() routine.
 *
 * @param function_name   The name of the portable driver shutdown routine.
 *
 * @return  A call to #os_dev_shutdown_return()
 *
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0))
void cleanup_module (void);

#else

#define OS_DEV_SHUTDOWN(function_name)                                        \
module_exit(function_name);                                                   \
static void function_name(void)
#endif


/*! asdfasdfsdf
 * @param function_name foo
 */
#define OS_DEV_SHUTDOWN_DCL(function_name)                                    \
static void function_name(void);

#define OS_DEV_SHUTDOWN_REF(function_name)                                    \
function_name


/*!
 * Define a function which will open the device for a user.
 *
 * @param function_name The name of the driver open() function
 *
 * @return  A call to #os_dev_open_return()
 */
#define OS_DEV_OPEN(function_name)                                            \
static int function_name(struct inode* inode_p_, struct file* file_p_)

#define OS_DEV_OPEN_DCL(function_name)                                        \
OS_DEV_OPEN(function_name);

#define OS_DEV_OPEN_REF(function_name)                                        \
function_name


/*!
 * Define a function which will handle a user's ioctl() request
 *
 * @param function_name The name of the driver ioctl() function
 *
 * @return A call to #os_dev_ioctl_return()
 */
#define OS_DEV_IOCTL(function_name)                                           \
static int function_name(struct inode* inode_p_, struct file* file_p_,        \
                     unsigned int cmd_, unsigned long data_)

/*! Boo. */
#define OS_DEV_IOCTL_DCL(function_name)                                       \
OS_DEV_IOCTL(function_name);

#define OS_DEV_IOCTL_REF(function_name)                                       \
function_name


/*!
 * Define a function which will handle a user's read() request
 *
 * @param function_name The name of the driver read() function
 *
 * @return A call to #os_dev_read_return()
 */
#define OS_DEV_READ(function_name)                                            \
static ssize_t function_name(struct file* file_p_, char* user_buffer_,        \
                     size_t count_bytes_, loff_t* file_position_)

#define OS_DEV_READ_DCL(function_name)                                        \
OS_DEV_READ(function_name);

#define OS_DEV_READ_REF(function_name)                                        \
function_name


/*!
 * Define a function which will handle a user's write() request
 *
 * @param function_name The name of the driver write() function
 *
 * @return A call to #os_dev_write_return()
 */
#define OS_DEV_WRITE(function_name)                                           \
static ssize_t function_name(struct file* file_p_, char* user_buffer_,        \
                     size_t count_bytes_, loff_t* file_position_)

#define OS_DEV_WRITE_DCL(function_name)                                       \
OS_DEV_WRITE(function_name);

#define OS_DEV_WRITE_REF(function_name)                                       \
function_name


/*!
 * Define a function which will close the device - opposite of OS_DEV_OPEN()
 *
 * @param function_name The name of the driver close() function
 *
 * @return A call to #os_dev_close_return()
 */
#define OS_DEV_CLOSE(function_name)                                           \
static int function_name(struct inode* inode_p_, struct file* file_p_)

/*! Boo. */
#define OS_DEV_CLOSE_DCL(function_name)                                       \
OS_DEV_CLOSE(function_name);

#define OS_DEV_CLOSE_REF(function_name)                                       \
function_name


/*!
 * Define a function which will handle an interrupt
 *
 * No arguments are available to the generic function.  It must not invoke any
 * OS functions which are illegal in a ISR.  It gets no parameters, and must
 * have a call to #os_dev_isr_return() instead of any/all return statements.
 *
 * Example:
 * @code
 * OS_DEV_ISR(widget, widget_isr, WIDGET_IRQ_NUMBER)
 * {
 *     os_dev_isr_return(1);
 * }
 * @endcode
 *
 * @param function_name The name of the driver ISR function
 *
 * @return   A call to #os_dev_isr_return()
 */
#define OS_DEV_ISR(function_name)                                             \
static irqreturn_t function_name(int N1_, void* N2_, struct pt_regs* N3_)

#define OS_DEV_ISR_DCL(function_name)                                         \
OS_DEV_ISR(function_name);

#define OS_DEV_ISR_REF(function_name)                                         \
function_name


/*!
 * Define a function which will operate as a background task / bottom half.
 *
 * @param function_name The name of this background task function
 *
 * @return A call to #os_dev_task_return()
 */
#define OS_DEV_TASK(function_name)                                            \
static void function_name(unsigned long data_)

#define OS_DEV_TASK_DCL(function_name)                                        \
OS_DEV_TASK(function_name);

#define OS_DEV_TASK_REF(function_name)                                        \
function_name


/*! @} */ /* end Driver Signatures */


/*****************************************************************************
 *  Functions/Macros for returning values from Driver Signature routines
 *****************************************************************************/

/*!
 * Return from the #OS_DEV_INIT() function
 *
 * @param code    An error code to report success or failure.
 *
 */
#define os_dev_init_return(code)                                             \
    return code


/*!
 * Return from the #OS_DEV_SHUTDOWN() function
 *
 * @param code    An error code to report success or failure.
 *
 */
#define os_dev_shutdown_return(code)                                         \
    return


/*!
 * Return from the #OS_DEV_ISR() function
 *
 * The function should verify that it really was supposed to be called,
 * and that its device needed attention, in order to properly set the
 * return code.
 *
 * @param code    non-zero if interrupt handled, zero otherwise.
 *
 */
#define os_dev_isr_return(code)                                              \
    return IRQ_RETVAL(code)


/*!
 * Return from the #OS_DEV_OPEN() function
 *
 * @param code    An error code to report success or failure.
 *
 */
#define os_dev_open_return(code)                                             \
    return (code)


/*!
 * Return from the #OS_DEV_IOCTL() function
 *
 * @param code    An error code to report success or failure.
 *
 */
#define os_dev_ioctl_return(code)                                            \
    return (code)


/*!
 * Return from the #OS_DEV_READ() function
 *
 * @param code    Number of bytes read, or an error code to report failure.
 *
 */
#define os_dev_read_return(code)                                             \
    return (code)


/*!
 * Return from the #OS_DEV_WRITE() function
 *
 * @param code    Number of bytes written, or an error code to report failure.
 *
 */
#define os_dev_write_return(code)                                            \
{                                                                            \
    ssize_t retcode = code;                                                  \
    /* get rid of 'unused parameter' warnings */                             \
    user_buffer_ = 0;                                                        \
    count_bytes_ = 0;                                                        \
    file_p_ = 0;                                                             \
    file_position_ = 0;                                                      \
    return retcode;                                                          \
}

/*!
 * Return from the #OS_DEV_CLOSE() function
 *
 * @param code    An error code to report success or failure.
 *
 */
#define os_dev_close_return(code)                                            \
    return (code)


/*****************************************************************************
 *  Functions/Macros for accessing arguments from Driver Signature routines
 *****************************************************************************/

/*!
 * Used in #OS_DEV_OPEN(), #OS_DEV_CLOSE(), #OS_DEV_IOCTL(), #OS_DEV_READ() and
 * #OS_DEV_WRITE() routines to check whether user is requesting read
 * (permission)
 */
#define os_dev_is_flag_read()                                                 \
   (file_p_->f_mode & FMODE_READ)

/*!
 * Used in #OS_DEV_OPEN(), #OS_DEV_CLOSE(), #OS_DEV_IOCTL(), #OS_DEV_READ() and
 * #OS_DEV_WRITE() routines to check whether user is requesting write
 * (permission)
 */
#define os_dev_is_flag_write()                                                \
   (file_p_->f_mode & FMODE_WRITE)


/*!
 * Used in #OS_DEV_OPEN(), #OS_DEV_CLOSE(), #OS_DEV_IOCTL(), #OS_DEV_READ() and
 * #OS_DEV_WRITE() routines to check whether user is requesting non-blocking
 * I/O.
 */
#define os_dev_is_flag_nonblock()                                             \
   (file_p_->f_flags & (O_NONBLOCK | O_NDELAY))


/*!
 * Used in #OS_DEV_OPEN() and #OS_DEV_CLOSE() to determine major device being
 * accessed.
 */
#define os_dev_get_major()                                                    \
    (imajor(inode_p_))


/*!
 * Used in #OS_DEV_OPEN() and #OS_DEV_CLOSE() to determine minor device being
 * accessed.
 */
#define os_dev_get_minor()                                                    \
    (iminor(inode_p_))

/*!
 * Used in #OS_DEV_IOCTL() to determine which operation the user wants
 * performed.
 *
 * @return Value of the operation.
 */
#define os_dev_get_ioctl_op()                                                 \
    (cmd_)

/*!
 * Used in #OS_DEV_IOCTL() to return the associated argument for the desired
 * operation.
 *
 * @return    A value which can be cast to a struct pointer or used as
 *            int/long.
 */
#define os_dev_get_ioctl_arg()                                                \
    (data_)

/*!
 * Used in OS_DEV_READ() and OS_DEV_WRITE() routines to access the requested
 * byte count.
 *
 * @return  (unsigned) a count of bytes
 */
#define os_dev_get_count()                                                    \
    ((unsigned)count_bytes_)

/*!
 * Used in OS_DEV_READ() and OS_DEV_WRITE() routines to return the pointer
 * byte count.
 *
 * @return   char* pointer to user buffer
 */
#define os_dev_get_user_buffer()                                              \
    ((void*)user_buffer_)

/*!
 * Set the driver's private structure associated with this file/open.
 *
 * Generally used during #OS_DEV_OPEN().  See #os_dev_get_user_private().
 *
 * @param  struct_p   The driver data structure to associate with this user.
 */
#define os_dev_set_user_private(struct_p)                                     \
    file_p_->private_data = (void*)(struct_p)

/*!
 * Get the driver's private structure associated with this file.
 *
 * May be used during #OS_DEV_OPEN(), #OS_DEV_READ(), #OS_DEV_WRITE(),
 * #OS_DEV_IOCTL(), and #OS_DEV_CLOSE().  See #os_dev_set_user_private().
 *
 * @return   The driver data structure to associate with this user.
 */
#define os_dev_get_user_private()                                             \
    ((void*)file_p_->private_data)

/*!
 * Flush and invalidate all cache lines.
 */
#if 0
#define os_flush_cache_all()                                              \
    flush_cache_all()
#else
/* Call ARM fn directly, in case L2cache=on3 not set */
#define os_flush_cache_all()                                              \
    v6_flush_kern_cache_all_L2();

/* and define it, because it exists in no easy-access header file */
extern void v6_flush_kern_cache_all_L2(void);
#endif

/*
 *  These macros are using part of the Linux DMA API.  They rely on the
 *  map function to do nothing more than the equivalent clean/inv/flush
 *  operation at the time of the mapping, and do nothing at an unmapping
 *  call, which the Sahara driver code will never invoke.
 */

/*!
 * Clean a range of addresses from the cache.  That is, write updates back
 * to (RAM, next layer).
 *
 * @param  start    Starting virtual address
 * @param  len      Number of bytes to flush
 *
 * @return void
 */
#define os_cache_clean_range(start,len)                                   \
    dma_map_single(NULL, (void*)start, len, DMA_TO_DEVICE)

/*!
 * Invalidate a range of addresses in the cache
 *
 * @param  start    Starting virtual address
 * @param  len      Number of bytes to flush
 *
 * @return void
 */
#define os_cache_inv_range(start,len)                                     \
    dma_map_single(NULL, (void*)start, len, DMA_FROM_DEVICE)

/*!
 * Flush a range of addresses from the cache.  That is, perform clean
 * and invalidate
 *
 * @param  start    Starting virtual address
 * @param  len      Number of bytes to flush
 *
 * @return void
 */
#define os_cache_flush_range(start,len)                                   \
    dma_map_single(NULL, (void*)start, len, DMA_BIDIRECTIONAL)

#endif /* LINUX_PORT_H */
