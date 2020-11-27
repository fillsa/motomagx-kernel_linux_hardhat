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
#ifndef PORTABLE_OS_H
#define PORTABLE_OS_H


/***************************************************************************/

/*
 * Add support for your target OS by checking appropriate flags and then
 * including the appropriate file.  Don't forget to document the conditions
 * in the later documentation section at the beginning of the
 * DOXYGEN_PORTABLE_OS_DOC.
 */

#if defined(LINUX_KERNEL)

#include "linux_port.h"

#elif defined(PORTABLE_OS)

#include "check_portability.h"

#else

#error Target OS unsupported or unspecified

#endif


/***************************************************************************/

/*!
 * @file portable_os.h
 *
 * @brief This file should be included by portable driver code in order to gain access
 * to the OS-specific header files.  It is the only OS-related header file that
 * the writer of a portable driver should need.
 *
 * This file also contains the documentation for the common API.
 *
 * @section intro_sec Introduction
 *
 * This defines the API / kernel programming environment for portable drivers.
 *
 * This API is broken up into several functional areas.  It greatly limits the
 * choices of a device-driver author, but at the same time should allow for
 * greater portability of the resulting code.
 *
 * Each kernel-to-driver function (initialization function, interrupt service
 * routine, etc.) has a 'portable signature' which must be used, and a specific
 * function which must be called to generate the return statement.  There is
 * one exception, a background task or "bottom half" routine, which instead has
 * a specific structure which must be followed.  These signatures and function
 * definitions are found in "Driver Function Signatures" section.
 *
 * None of these kernel-to-driver functions seem to get any arguments passed to
 * them.  Instead, there are Functions to access arguments from Driver Signature routines 
 * which allow one of these functions to get at fairly generic parts of its calling 
 * arguments, if there are any.
 *
 * Almost every driver will have some need to call the operating system.
 * Driver-to-Kernel Operations is the list of services which are available 
 * to the driver.
 *
 *
 * @subsection warn_sec Warning
 *
 * The specifics of the types, values of the various enumerations
 * (unless specifically stated, like = 0), etc., are only here for illustrative
 * purposes.  No attempts should be made to make use of any specific knowledge
 * gleaned from this documentation.  These types are only meant to be passed in
 * and out of the API, and their contents are to be handled only by the
 * provided OS-specific functions.
 *
 * Also, note that the function may be provided as macros in some
 * implementations, or vice versa.
 *
 *
 * @section dev_sec Writing a Portable Driver
 *
 * First off, writing a portable driver means calling no function in an OS
 * except what is available through this header file.
 *
 * Secondly, all OS-called functions in your driver must be invoked and
 * referenced through the signature routines.
 *
 * Thirdly, there might be some rules which you can get away with ignoring or
 * violating on one OS, yet will cause your code not to be portable to a
 * different OS.
 *
 *
 * @section limit_sec Limitations
 *
 * This API is not expected to handle every type of driver which may be found
 * in an operating system.  For example, it will not be able to handle the
 * usual design for something like a UART driver, where there are multiple
 * logical devices to access, because the design usually calls for some sort of
 * indication to the #OS_DEV_TASK() function to indicate which channel is to be
 * serviced by that instance of the task (which would be an argument, missing
 * in this API, to #os_create_task().
 *
 *
 * @section port_guide Porting Guidelines
 *
 * This section is intended for a developer who needs to port the header file
 * to an operating system which is not yet supported.
 *
 * This interface allows for a lot of flexibility when it comes to porting to
 * an operating systems device driver interface.  There are three main areas to
 * examine:  The use of Driver Routine Signatures, the use of Driver Argument
 * Access functions, the Calls to Kernel Functions, and Data Types.
 *
 *
 * @subsection port_sig Porting Driver Routine Signatures
 *
 * The three macros for each function (e.g. #OS_DEV_INIT(), #OS_DEV_INIT_DCL(),
 * and #OS_DEV_INIT_REF()) allow the flexibility of having a 'wrapper' function
 * with the OS-required signature, which would then call the user's function
 * with some different signature.
 *
 * The first form would lay down the wrapper function, followed by the
 * signature for the user function.  The second form would lay down just the
 * signatures for both functions, and the last function would reference the
 * wrapper function, since that is the interface function called by the OS.
 *
 * Note that the driver author has no visibility at all to the signature of the
 * routines.  The author can access arguments only through a limited set of
 * functions, and must return via another function.
 *
 * The Return Functions allow a lot of flexibility in converting the return
 * value, or not returning a value at all.  These will likely be implemented as
 * macros.
 *
 *
 * @subsection port_arg Porting Driver Argument Access Functions
 *
 * The signatures defined by the guide will usually be replaced with macro
 * definitions.
 *
 *
 * @subsection port_dki Porting Calls to Kernel Functions
 *
 * The signatures defined by the guide may be replaced with macro definitions,
 * if that makes more sense.
 *
 * Implementors are free to ignore arguments which are not applicable to their
 * OS.
 *
 * @subsection port_datatypes Porting Data Types
 *
 * @ingroup MXCSAHARA2
 */


/***************************************************************************
 * Compile flags
 **************************************************************************/

/*
 * This compile flag should only be turned on when running doxygen to generate
 * the API documentation.
 */
#ifdef DOXYGEN_PORTABLE_OS_DOC

/*!
 * @todo module_init()/module_cleanup() for Linux need to be added to OS
 * abstractions.  Also need EXPORT_SYMBOL() equivalent??
 *
 */

/* Drop OS differentation documentation here */


/*!
 * \#define this flag to build your driver as a Linux driver
 */
#define LINUX

/* end OS differentation documentation */


/*!
 * \#define this file to do some checking of your driver's portability
 */
#define PORTABLE_OS

/*
 * NOTICE: The following definitions (the rest of the file) are not meant ever
 * to be compiled.  Instead, they are the documentation for the portable OS
 * API, to be used by driver writers.
 *
 * Each individual OS port will define each of these types, functions, or
 * macros as appropriate to the target OS. This is why they are under the
 * DOXYGEN_PORTABLE_OS_DOC flag.
 */


/***************************************************************************
 * Type definitions
 **************************************************************************/

/*!
 * Type used for registering and deregistering interrupts.
 *
 * This is typically an interrupt channel number.
 */
typedef int os_interrupt_id_t;

/*!
 * Type used as handle for a process
 *
 * See #os_get_process_handle() and #os_send_signal().
 */
typedef int os_process_handle_t;


/*!
 * Generic return code for functions which need such a thing.
 *
 * No knowledge should be assumed of the value of any of these symbols except
 * that @c OS_ERROR_OK_S is guaranteed to be zero.
 *
 * @todo Any other named values? What about (-EAGAIN?  -ERESTARTSYS? Are they
 * too Linux/Unix-specific read()/write() return values) ?
 */
typedef enum {
    OS_ERROR_OK_S = 0,          /*!< Success  */
    OS_ERROR_FAIL_S,            /*!< Generic driver failure */
    OS_ERROR_NO_MEMORY_S        /*!< Failure to acquire/use memory  */
} os_error_code;


/*!
 * Handle to a lock.
 *
 * @todo Need to define OS generic locks (#os_lock_t), and ways to create them,
 * initialize them, tickle them, etc.  (Dynamic or static or both?)
 *
 */
typedef int* os_lock_t;


typedef spinlock_t* os_lock_t;


/*!
 * Context while locking.
 */
typedef int os_lock_context_t;


/*!
 * Handle to a queue
 *
 * @todo Need to define OS generic queues (#os_queue_t), and ways to create
 * them, initialize them, add things to them, etc.
 */
typedef void* os_queue_t;


/*!
 * Driver registration handle
 */
typedef void* os_driver_reg_t;


/*!
 * Function signature for an #OS_DEV_INIT() function.
 *
 * @return  A call to os_dev_init_return() function.
 */
typedef void (*os_init_function_t)(void);


/*!
 * Function signature for an #OS_DEV_SHUTDOWN() function.
 *
 * @return  A call to os_dev_shutdown_return() function.
 */
typedef void (*os_shutdown_function_t)(void);


/*!
 * Function signature for a user-driver function.
 *
 * @return  A call to the appropriate os_dev_xxx_return() function.
 */
typedef void (*os_user_function_t)(void);


/*!
 * Function signature for the portable interrupt handler
 *
 * While it would be nice to know which interrupt is being serviced, the
 * Least Common Denominator rule says that no arguments get passed in.
 *
 * @return  A call to #os_dev_isr_return()
 */
typedef void (*os_interrupt_handler_t)(void);


/*!
 * Function signature for a task function
 *
 * Many task function definitions get some sort of generic argument so that the
 * same function can run across many (queues, channels, ...) as separate task
 * instances.  This has been left out of this API.
 *
 * This function must be structured as documented by #OS_DEV_TASK().
 *
 */
typedef void (*os_task_fn_t)(void);


/*!
 * A handle to a created task.  Returned by #os_create_task() and used for
 * scheduling by calling os_schedule_task;
 */
typedef void* os_task_handle_t;


/*!
 *  Function types which can be associated with driver entry points.
 *
 *  Note that init and shutdown are absent.
 */
typedef enum {
    OS_FN_OPEN,
    OS_FN_CLOSE,
    OS_FN_READ,
    OS_FN_WRITE,
    OS_FN_IOCTL,
    OS_FN_MMAP
} os_driver_fn_t;


/***************************************************************************
 * Driver-to-Kernel Operations
 **************************************************************************/

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
 * @return #os_error_code
 */
os_error_code os_register_interrupt(char* driver_name,
                                    os_interrupt_id_t interrupt_id,
                                    os_interrupt_handler_t function);


/*!
 * Deregister an interrupt handler.
 *
 * @param interrupt_id   The interrupt line to stop monitoring
 *
 * @return #os_error_code
 */
os_error_code os_deregister_interrupt(os_interrupt_id_t interrupt_id);


/*!
 * Initialize driver registration.
 *
 * If the driver handles open(), close(), ioctl(), read(), write(), or mmap()
 * calls, then it needs to register their location with the kernel so that they
 * get associated with the device.
 *
 * @return A handle for further driver registration, or NULL if failed.
 */
os_driver_reg_t os_driver_init_registration(void);


/*!
 * Add a function registration to driver registration.
 *
 * @param handle    A handle returned by #os_driver_init_registration().
 * @param name      Which function is being supported.
 * @param function  The result of a call to a @c _REF version of one of the
 *                  driver function signature macros
 */
void os_driver_add_registration(os_driver_reg_t handle, os_driver_fn_t name,
                                void* function);


/*!
 * Finalize the driver registration with the kernel.  The handle may not be
 * used after this call.
 *
 * Upon return from this call, the driver may begin receiving calls at the
 * defined entry points.
 *
 * @param handle     The driver registration information from
 *                   #os_driver_init_registration() and modified by
 * @param major       The major device number to be associated with the driver.
 * @param driver_name The driver name
 *
 * @return  On error, OS_ERROR_FAIL_S.  On success, either 0, or a major number
 *          (if @a major was 0).
 */
os_error_code os_driver_complete_registration(os_driver_reg_t handle,
                                              int major,
                                              char* driver_name);


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
os_error_code os_driver_remove_registration(int major, char* driver_name);


/*!
 * This function causes the current task to sleep on a queue until awoken.  It
 * takes as an argument a condition to test before going to sleep.  This must
 * be used to prevent deadlocks in certain situations.
 *
 * Example:  os_task_queue(my_queue, available_count == 0);
 *
 * @param queue      The #os_queue_t to sleep on.
 * @param condition  Some expression to be tested before going to sleep
 *
 * @todo Need to define a way wake up tasks sleeping on queues (singly or
 * simultaneously?).
 */
void os_task_queue_sleep(os_queue_t queue, void condition);


/*!
 * Print a message to console / into log file.  After the @c msg argument a
 * number of printf-style arguments may be added.  Types should be limited to
 * printf string, char, octal, decimal, and hexadecimal types.  (This excludes
 * pointers, and floating point).
 *
 * @param  msg  The message to print to console / system log
 *
 * @return (void)
 */
void os_printk(char* msg, ...);


/*!
 * Allocate some kernel memory
 *
 * @param amount   Number of 8-bit bytes to allocate
 * @param flags    Some indication of purpose of memory (needs definition)
 *
 * @return  Pointer to allocated memory, or NULL if failed.
 */
void* os_alloc_memory(unsigned amount, int flags);


/*!
 * Free some kernel memory
 *
 * @param location  The beginning of the region to be freed.
 *
 * @todo Do some OSes have separate free() functions which should be
 * distinguished by passing in @c flags here, too? Don't some also require the
 * size of the buffer being freed?  Perhaps separate routines for each
 * alloc/free pair (DMAable, etc.)?
 */
void os_free_memory(void* location);


/*!
 * Map an I/O space into kernel memory space
 *
 * @param start       The starting address of the (physical / io space) region
 * @param range_bytes The number of bytes to map
 *
 * @return A pointer to the mapped area, or NULL on failure
 */
void* os_map_device(void* start, unsigned range_bytes);


/*!
 * Copy data from Kernel space to User space
 *
 * @param to   The target location in user memory
 * @param from The source location in kernel memory
 * @param size The number of bytes to be copied
 *
 * @return #os_error_code
 */
os_error_code os_copy_to_user(void* to, void* from, unsigned size);


/*!
 * Copy data from User space to Kernel space
 *
 * @param to   The target location in kernel memory
 * @param from The source location in user memory
 * @param size The number of bytes to be copied
 *
 * @return #os_error_code
 */
os_error_code os_copy_from_user(void* to, void* from, unsigned size);


/*!
 * Read an 8-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
uint8_t os_read8(uint8_t* register_address);


/*!
 * Write an 8-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
void os_write8(uint8_t* register_address, uint8_t value);


/*!
 * Read a 16-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
uint16_t os_read16(uint16_t* register_address);


/*!
 * Write a 16-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
void os_write16(uint16_t* register_address, uint16_t value);

/*!
 * Read a 32-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
uint32_t os_read32(uint32_t* register_address);


/*!
 * Write a 32-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
void os_write32(uint32_t* register_address, uint32_t value);


/*!
 * Read a 64-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @return                  The value in the register
 */
uint64_t os_read64(uint64_t* register_address);


/*!
 * Write a 64-bit device register
 *
 * @param register_address  The (bus) address of the register to write to
 * @param value             The value to write into the register
 */
void os_write64(uint64_t* register_address, uint64_t value);


/*!
 * Create a task from an #OS_DEV_TASK() function
 *
 * The task will begin execution sometime after or during this call.
 *
 * @param name     A name to be assigned to this task
 * @param priority An execution priority
 * @param task_fn  A #OS_DEV_TASK_REF() to the task function to execute in the
 *                 task
 *
 * @todo Need to define OS generic interface mechanisms to delete a task.
 *
 * @return
 */
os_task_handle_t os_create_task(char* name, int priority,
                                os_task_fn_t* task_fn);

/*!
 * Delay some number of microseconds
 *
 * Note that this is a busy-loop, not a suspension of the task/process.
 *
 * @param  msecs   The number of microseconds to delay
 *
 * @return void
 */
void os_mdelay(unsigned long msecs);


/*!
 * Calculate virtual address from physical address
 *
 * @param pa    Physical address
 *
 * @return virtual address
 *
 * @note this assumes that addresses are 32 bits wide
 */
void *os_va(uint32_t pa);


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
uint32_t os_pa(void* va);


/*!
 * Allocate and initialize a lock, returning a lock handle.
 *
 * The lock state will be initialized to 'unlocked'.
 *
 * @return A lock handle, or NULL if an error occurred.
 */
os_lock_t lockp os_lock_alloc_init(void);


/*!
 * Acquire a lock.
 *
 * This function should only be called from an interrupt service routine.
 *
 * @param   lock_handle  A handle to the lock to acquire.
 *
 * @return void
 */
void os_lock(os_lock_t lock_handle);


/*!
 * Unlock a lock.  Lock must have been acquired by #os_lock().
 *
 * @param   lock_handle  A handle to the lock to unlock.
 *
 * @return void
 */
void os_unlock(os_lock_t lock_handle);

/*!
 * Acquire a lock in non-ISR context
 *
 * This function will spin until the lock is available.
 *
 * @param lock_handle  A handle of the lock to acquire.
 * @param context      Place to save the before-lock context
 *
 * @return void
 */
void os_lock_save_context(os_lock_t lock_handle, os_lock_context_t context);

/*!
 * Release a lock in non-ISR context
 *
 * @param lock_handle  A handle of the lock to release.
 * @param context      Place where before-lock context was saved.
 *
 * @return void
 */
void os_unlock_restore_context(os_lock_t lock_handle,
                               os_lock_context_t context);

/*!
 * Deallocate a lock handle.
 *
 * @param lock_handle   An #os_lock_t that has been allocated.
 *
 * @return void
 */
void os_lock_deallocate(os_lock_t lock_handle);


/*!
 * Determine process handle
 *
 * The process handle of the current user is returned.
 *
 * @return   A handle on the current process.
 */
os_process_handle_t os_get_process_handle();


/*!
 * Send a signal to a process
 *
 * @param  proc   A handle to the target process.
 * @param  sig    The POSIX signal to send to that process.
 */
void os_send_signal(os_process_handle_t proc, int sig);


/*!
 * Get some random bytes
 *
 * @param buf    The location to store the random data.
 * @param count  The number of bytes to store.
 *
 * @return  void
 */
void os_get_random_bytes(void* buf, unsigned count);

/** @} */ /* end Driver-to-Kernel Operations */



/*****************************************************************************
 *  Function-signature-generating macros
 *****************************************************************************/

/*!
 * @name Driver Function Signatures
 *
 * These macros will define the entry point signatures for interrupt handlers;
 * driver initialization and shutdown; device open/close; etc.  They are to be
 * used whenever the Kernel will call into the Driver.  They are not
 * appropriate for driver calls to other routines in the driver.
 *
 * There are three versions of each macro for a given Driver Entry Point.  The
 * first version is used to define a function and its implementation in the
 * driver.c file, e.g. #OS_DEV_INIT().
 *
 * The second form is used whenever a forward declaration (prototype) is
 * needed.  It has the letters @c _DCL appended to the name of the definition
 * function.  These are not otherwise mentioned in this documenation.
 *
 * There is a third form used when a reference to a function is required, for
 * instance when passing the routine as a pointer to a function.  It has the
 * letters @c _REF appended to the name of the definition function
 * (e.g. DEV_IOCTL_REF).
 *
 * Note that these two extra forms are required because of the possibility of
 * having an invisible 'wrapper function' created by the os-specific header
 * file which would need to be invoked by the operating system, and which in
 * turn would invoke the generic function.
 *
 * Example:
 *
 * (in a header file)
 * @code
 * OS_DEV_INIT_DCL(widget, widget_init);
 * OS_DEV_ISR_DCL(widget_isr);
 * @endcode
 *
 * (in an implementation file)
 * @code
 * OS_DEV_INIT(widget, widget_init)
 * {
 *
 *     os_register_interrupt("widget", WIDGET_IRQ, OS_DEV_ISR_REF(widget_isr);
 *
 *     os_dev_init_return(TRUE);
 * }
 *
 * OS_DEV_ISR(widget_isr)
 * {
 *     os_dev_isr_return(TRUE);
 * }
 * @endcode
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
#define OS_DEV_INIT(function_name)


/*!
 * Define a function which will handle device shutdown
 *
 * This is the reverse of the #OS_DEV_INIT() routine.
 *
 * @param function_name   The name of the portable driver shutdown routine.
 *
 * @return  A call to #os_dev_shutdown_return()
 */
#define OS_DEV_SHUTDOWN(function_name)


/*!
 * Define a function which will open the device for a user.
 *
 * @param function_name The name of the driver open() function
 *
 * @return A call to #os_dev_open_return()
 */
#define OS_DEV_OPEN(function_name)


/*!
 * Define a function which will handle a user's ioctl() request
 *
 * @param function_name The name of the driver ioctl() function
 *
 * @return A call to #os_dev_ioctl_return()
 */
#define OS_DEV_IOCTL(function_name)


/*!
 * Define a function which will handle a user's read() request
 *
 * @param function_name The name of the driver read() function
 *
 * @return A call to #os_dev_read_return()
 */
#define OS_DEV_READ(function_name)


/*!
 * Define a function which will handle a user's write() request
 *
 * @param function_name The name of the driver write() function
 *
 * @return A call to #os_dev_write_return()
 */
#define OS_DEV_WRITE(function_name)


/*!
 * Define a function which will handle a user's mmap() request
 *
 * The mmap() function requests the driver to map some memory into user space.
 *
 * @todo Determine what support functions are needed for mmap() handling.
 *
 * @param function_name The name of the driver mmap() function
 *
 * @return A call to #os_dev_mmap_return()
 */
#define OS_DEV_MMAP(function_name)


/*!
 * Define a function which will close the device - opposite of OS_DEV_OPEN()
 *
 * @param function_name The name of the driver close() function
 *
 * @return A call to #os_dev_close_return()
 */
#define OS_DEV_CLOSE(function_name)


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
#define OS_DEV_ISR(function_name)


/*!
 * Define a function which will operate as a background task / bottom half.
 *
 * The function implementation must be structured in the following manner:
 * @code
 * OS_DEV_TASK(widget_task)
 * {
 *     OS_DEV_TASK_SETUP(widget_task);
 *
 *     while OS_DEV_TASK_CONDITION(widget_task) }
 *
 *     };
 * }
 * @endcode
 *
 * @todo In some systems the OS_DEV_TASK_CONDITION() will be an action which
 * will cause the task to sleep on some event triggered by os_run_task().  In
 * others, the macro will reference a variable laid down by
 * OS_DEV_TASK_SETUP() to make sure that the loop is only performed once.
 *
 * @param function_name The name of this background task function
 */
#define OS_DEV_TASK(function_name)

/*! @} */ /* end Driver Function Signatures */


/*! @name Routines to declare Driver Signature routines
 *
 * These macros drop prototypes suitable for forward-declartion of Driver 
 * Function Signatures
 */
/*! @{ */

/*!
 * Declare prototype for the device initialization function
 *
 * @param function_name   The name of the portable initialization function.
 */
#define OS_DEV_INIT_DCL(function_name)


/*!
 * Declare prototype for the device shutdown function
 *
 * @param function_name   The name of the portable driver shutdown routine.
 *
 * @return  A call to #os_dev_shutdown_return()
 */
#define OS_DEV_SHUTDOWN_DCL(function_name)


/*!
 * Declare prototype for the open() function.
 *
 * @param function_name The name of the driver open() function
 *
 * @return A call to #os_dev_open_return()
 */
#define OS_DEV_OPEN_DCL(function_name)


/*!
 * Declare prototype for the user's ioctl() request function
 *
 * @param function_name The name of the driver ioctl() function
 *
 * @return A call to #os_dev_ioctl_return()
 */
#define OS_DEV_IOCTL_DCL(function_name)


/*!
 * Declare prototype for the function a user's read() request
 *
 * @param function_name The name of the driver read() function
 */
#define OS_DEV_READ_DCL(function_name)


/*!
 * Declare prototype for the user's write() request function
 *
 * @param function_name The name of the driver write() function
 */
#define OS_DEV_WRITE_DCL(function_name)


/*!
 * Declare prototype for the user's mmap() request function
 *
 * @param function_name The name of the driver mmap() function
 */
#define OS_DEV_MMAP_DCL(function_name)


/*!
 * Declare prototype for the close function
 *
 * @param function_name The name of the driver close() function
 *
 * @return A call to #os_dev_close_return()
 */
#define OS_DEV_CLOSE_DCL(function_name)


/*!
 * Declare prototype for the interrupt handling function
 *
 * @param function_name The name of the driver ISR function
 */
#define OS_DEV_ISR_DCL(function_name)


/*!
 * Declare prototype for a background task / bottom half function
 *
 * @param function_name The name of this background task function
 */
#define OS_DEV_TASK_DCL(function_name)

/*! @} */ /* end Routines to declare Driver Signature routines */


/*****************************************************************************
 *  Functions for Returning Values from Driver Signature routines
 *****************************************************************************/

/*!
 * @name Functions to Return Values from Driver Signature routines
 */
/*! @{ */

/*!
 * Return from the #OS_DEV_INIT() function
 *
 * @param code    An error code to report success or failure.
 *
 */
void os_dev_init_return(os_error_code code);


/*!
 * Return from the #OS_DEV_SHUTDOWN() function
 *
 * @param code    An error code to report success or failure.
 *
 */
void os_dev_shutdown_return(os_error_code code);


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
void os_dev_isr_return(int code);


/*!
 * Return from the #OS_DEV_OPEN() function
 *
 * @param code    An error code to report success or failure.
 *
 */
void os_dev_open_return(os_error_code code);


/*!
 * Return from the #OS_DEV_IOCTL() function
 *
 * @param code    An error code to report success or failure.
 *
 */
void os_dev_ioctl_return(os_error_code code);


/*!
 * Return from the #OS_DEV_READ() function
 *
 * @param code    Number of bytes read, or an error code to report failure.
 *
 */
void os_dev_read_return(os_error_code code);


/*!
 * Return from the #OS_DEV_WRITE() function
 *
 * @param code    Number of bytes written, or an error code to report failure.
 *
 */
void os_dev_write_return(os_error_code code);


/*!
 * Return from the #OS_DEV_MMAP() function
 *
 * @param code    Number of bytes written, or an error code to report failure.
 *
 */
void os_dev_mmap_return(os_error_code code);


/*!
 * Return from the #OS_DEV_CLOSE() function
 *
 * @param code    An error code to report success or failure.
 *
 */
void os_dev_close_return(os_error_code code);

/*! @} */ /* end Functions to Return Values from Driver Signature routines */


/*****************************************************************************
 *  Functions/Macros for accessing arguments from Driver Signature routines
 *****************************************************************************/

/*!
 * @name Functions to access arguments from Driver Signature routines
 */

/*! @{ */

/*!
 * Usable in #OS_DEV_OPEN(), #OS_DEV_CLOSE(), #OS_DEV_IOCTL(), #OS_DEV_READ()
 * and #OS_DEV_WRITE() routines to check whether user is requesting read
 * (permission)
 */
int os_dev_is_flag_read(void);


/*!
 * Usable in #OS_DEV_OPEN(), #OS_DEV_CLOSE(), #OS_DEV_IOCTL(), #OS_DEV_READ()
 * and #OS_DEV_WRITE() routines to check whether user is requesting write
 * (permission)
 */
int os_dev_is_flag_write(void);


/*!
 * Usable in #OS_DEV_OPEN(), #OS_DEV_CLOSE(), #OS_DEV_IOCTL(), #OS_DEV_READ()
 * and #OS_DEV_WRITE() routines to check whether user is requesting
 * non-blocking I/O.
 *
 * @todo Specify required behavior when nonblock is requested and (sufficient?)
 * data are not available to fulfill the request.
 *
 */
int os_dev_is_flag_nonblock(void);


/*!
 * Usable in #OS_DEV_OPEN() and #OS_DEV_CLOSE() to determine major device being
 * accessed.
 */
int os_dev_get_major(void);


/*!
 * Usable in #OS_DEV_OPEN() and #OS_DEV_CLOSE() to determine minor device being
 * accessed.
 */
int os_dev_get_minor(void);


/*!
 * Usable in #OS_DEV_IOCTL() to determine which operation the user wants
 * performed.
 *
 * @return  Value of the operation.
 *
 * @todo Define some generic way to define the individual operations.
 */
unsigned os_dev_get_ioctl_op(void);


/*!
 * Usable in #OS_DEV_IOCTL() to return the associated argument for the desired
 * operation.
 *
 * @return    A value which can be cast to a struct pointer or used as
 *            int/long.
 */
os_dev_ioctl_arg_t os_dev_get_ioctl_arg(void);


/*!
 * Usable in OS_DEV_READ() and OS_DEV_WRITE() routines to access the requested
 * byte count.
 *
 * @return   A count of bytes
 */
unsigned os_dev_get_count(void);


/*!
 * Usable in OS_DEV_READ(), OS_DEV_WRITE(), and OS_DEV_MMAP() routines to
 * return the pointer byte count.
 *
 * @return   pointer to user buffer
 */
void* os_dev_get_user_buffer(void);


/*!
 * Set the driver's private structure associated with this file/open.
 *
 * Generally used during #OS_DEV_OPEN().  See #os_dev_get_user_private().
 *
 * @param  struct_p   The driver data structure to associate with this user.
 */
void os_dev_set_user_private(void* struct_p);


/*!
 * Get the driver's private structure associated with this file.
 *
 * May be used during #OS_DEV_OPEN(), #OS_DEV_READ(), #OS_DEV_WRITE(),
 * #OS_DEV_IOCTL(), and #OS_DEV_CLOSE().  See #os_dev_set_user_private().
 *
 * @return   The driver data structure to associate with this user.
 */
void* os_dev_get_user_private(void);

/*! @} */ /* end Functions to access arguments from Driver Signature routines */


/*****************************************************************************
 *  Functions for Generating References to Driver Routines
 *****************************************************************************/

/*!
 * @name Functions for Generating References to Driver Routines
 *
 * These functions will most likely be implemented as macros.  They are a
 * necessary part of the portable API to guarantee portability.  The @c symbol
 * type in here is the same symbol passed to the associated
 * signature-generating macro.
 *
 * These macros must be used whenever referring to a
 * driver signature function, for instance when storing or
 * passing a pointer to the function.
 */

/*! @{ */

/*!
 * Generate a reference to an #OS_DEV_INIT() function
 *
 * @param function_name   The name of the init function being referenced.
 *
 * @return A reference to the function
 */
os_init_function_t OS_DEV_INIT_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_SHUTDOWN() function
 *
 * @param function_name   The name of the shutdown function being referenced.
 *
 * @return A reference to the function
 */
os_shutdown_function_t OS_DEV_SHUTDOWN_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_OPEN() function
 *
 * @param function_name   The name of the open function being referenced.
 *
 * @return A reference to the function
 */
os_user_function_t OS_DEV_OPEN_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_CLOSE() function
 *
 * @param function_name   The name of the close function being referenced.
 *
 * @return A reference to the function
 */
os_user_function_t OS_DEV_CLOSE_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_READ() function
 *
 * @param function_name   The name of the read function being referenced.
 *
 * @return A reference to the function
 */
os_user_function_t OS_DEV_READ_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_WRITE() function
 *
 * @param function_name   The name of the write function being referenced.
 *
 * @return A reference to the function
 */
os_user_function_t OS_DEV_WRITE_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_IOCTL() function
 *
 * @param function_name   The name of the ioctl function being referenced.
 *
 * @return A reference to the function
 */
os_user_function_t OS_DEV_IOCTL_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_MMAP() function
 *
 * @param function_name   The name of the mmap function being referenced.
 *
 * @return A reference to the function
 */
os_user_function_t OS_DEV_MMAP_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_ISR() function
 *
 * @param function_name   The name of the isr being referenced.
 *
 * @return a reference to the function
 */
os_interrupt_handler_t OS_DEV_ISR_REF(symbol function_name);


/*!
 * Generate a reference to an #OS_DEV_TASK() function
 *
 * @param function_name   The name of the task being referenced.
 */
os_task_handle_t OS_DEV_TASK_REF(symbol function_name);

/*! @} */ /* end Functions for Generating References to Driver Routines */


/*!
 * Flush and invalidate all cache lines.
 */
void os_flush_cache_all(void);


/*!
 * Flush a range of addresses from the cache
 *
 * @param  start    Starting virtual address
 * @param  len      Number of bytes to flush
 */
void os_cache_flush_range(void* start, uint32_t len);


/*!
 * Invalidate a range of addresses in the cache
 *
 * @param  start    Starting virtual address
 * @param  len      Number of bytes to flush
 */
void os_cache_inv_range(void* start, uint32_t len);


/*!
 * Clean a range of addresses from the cache
 *
 * @param  start    Starting virtual address
 * @param  len      Number of bytes to flush
 */
void os_cache_clean_range(void* start, uint32_t len);

#endif /* DOXYGEN_PORTABLE_OS_DOC */

#endif /* PORTABLE_OS_H */
