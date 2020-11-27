/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright 2005-2006 Motorola, Inc.
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
* @file sah_driver_interface.c
*
* @brief Provides a Linux Kernel Module interface to the SAHARA h/w device.
*
* @ingroup MXCSAHARA2
*/

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Add check for SCM-A11 Pass 1 revision.
 */


/* SAHARA Includes */
#include <sah_driver_common.h>
#include <sah_kernel.h>
#include <sah_memory_mapper.h>
#include <sah_queue_manager.h>
#include <sah_status_manager.h>
#include <sah_interrupt_handler.h>
#include <sah_hardware_interface.h>
#include <adaptor.h>
#include <asm/arch/mxc_scc_driver.h>

#ifdef DIAG_DRV_IF
#include <diagnostic.h>
#endif


#if defined(CONFIG_DEVFS_FS) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
#include <linux/devfs_fs_kernel.h>
#else
#include <linux/proc_fs.h>
#endif

#include <asm/arch/spba.h>
#ifdef CONFIG_MACH_SCMA11REF
#include <asm/arch/mxc91231.h>
#endif /* CONFIG_MACH_SCMA11REF */

#ifdef PERF_TEST
#define interruptible_sleep_on(x) sah_Handle_Interrupt()
#endif

#define TEST_MODE_OFF 1
#define TEST_MODE_ON 2


/*! Version register on first deployments */
#define SAHARA_VERSION2  2
/*! Version register on MX27 */
#define SAHARA_VERSION3  3


/******************************************************************************
* Module function declarations
******************************************************************************/

OS_DEV_INIT_DCL(sah_init);
OS_DEV_SHUTDOWN_DCL(sah_cleanup);
OS_DEV_OPEN_DCL(device_open);
OS_DEV_CLOSE_DCL(device_release);
OS_DEV_IOCTL_DCL(device_ioctl);

static void          sah_user_callback(fsl_shw_uco_t* uco);
static os_error_code sah_handle_scc_slot_alloc(uint32_t info);
static os_error_code sah_handle_scc_slot_dealloc(uint32_t info);
static os_error_code sah_handle_scc_slot_load(uint32_t info);
static os_error_code sah_handle_scc_slot_decrypt(uint32_t info);
static os_error_code sah_handle_scc_slot_encrypt(uint32_t info);

/*! Boolean flag for whether interrupt handler needs to be released on exit */
static unsigned interrupt_registered;

static int handle_device_ioctl_dar(
    fsl_shw_uco_t *filp,
    uint32_t user_space_desc);

#if !defined(CONFIG_DEVFS_FS) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
static int device_read_procfs (
    char    *buf,
    char    **start,
    off_t   offset,
    int     count,
    int     *eof,
    void    *data);
#endif

#if defined(CONFIG_DEVFS_FS) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))

/* This is a handle to the sahara DEVFS entry. */
static devfs_handle_t Sahara_devfs_handle;

#else

/* Major number assigned to our device driver */
static int Major;

/* This is a handle to the sahara PROCFS entry */
static struct proc_dir_entry *Sahara_procfs_handle;

#endif

uint32_t sah_hw_version;

/* This is the wait queue to this driver.  Linux declaration. */
DECLARE_WAIT_QUEUE_HEAD(Wait_queue);

/* This is a global variable that is used to track how many times the device
 * has been opened simultaneously. */
#ifdef DIAG_DRV_IF
static int Device_in_use = 0;
#endif

/*!
 * The file_operations structure.  This is used by the kernel to map the four
 * device operations write/open/release/ioctl to functions in this file.
 * For the return values of these functions see page 64 of the Linux
 * Device Drivers book - Alessandro Rubini, Jonathan Corbet, Linux Device
 * Drivers, 2nd Edition, Jun 2001.
 */
static struct file_operations Fops =
{
    .owner =    THIS_MODULE,
/*    .write =    device_write, */
    .open =     OS_DEV_OPEN_REF(device_open),
    .release =  OS_DEV_CLOSE_REF(device_release),
    .ioctl =    OS_DEV_IOCTL_REF(device_ioctl),
    .read =     NULL,
    .llseek =   NULL,
    .readdir =  NULL,
    .poll =     NULL,
    .mmap =     NULL,
    .flush =    NULL,
    .fsync =    NULL,
    .fasync =   NULL,
    .lock =     NULL,
    .readv =    NULL,
    .writev =   NULL,
    .sendpage = NULL,
    .get_unmapped_area = NULL
};


#ifdef DIAG_DRV_IF
/* This is for sprintf() to use when constructing output. */
#define DIAG_MSG_SIZE   1024
static char Diag_msg[DIAG_MSG_SIZE];
#endif


/*!
*******************************************************************************
* This function gets called when the module is inserted (insmod) into  the
* running kernel.
*
* @brief     SAHARA device initialisation function.
*
* @return   0 on success
* @return   -EBUSY if the device or proc file entry cannot be created.
* @return   OS_ERROR_NO_MEMORY_S if kernel memory could not be allocated.
* @return   OS_ERROR_FAIL_S if initialisation of proc entry failed
*/
OS_DEV_INIT(sah_init)
{
    /* Status variable */
    int os_error_code = 0;
#ifdef DIAG_DRV_IF
    char err_string[200];
#endif

#ifdef CONFIG_MACH_SCMA11REF
    if(system_rev < CHIP_REV_2_0) {
        printk(KERN_ERR "SCM-A11 Pass 1 detected. Not loading Sahara.\n");
        os_dev_init_return(OS_ERROR_FAIL_S);
    }
#endif /* CONFIG_MACH_SCMA11REF */


    interrupt_registered = 0;

#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91321)
    /* This needs to be a PLATFORM abstraction */
    if (spba_take_ownership(SPBA_SAHARA, SPBA_MASTER_A)) {
#ifdef DIAG_DRV_IF
        LOG_KDIAG("Sahara driver could not take ownership of Sahara\n");
#endif
        os_error_code = OS_ERROR_FAIL_S;
    }
#endif /* MXC91231 */

    if (os_error_code == 0) {
        sah_hw_version = sah_HW_Read_Version();

        /* verify code and hardware are version compatible */
        if ((sah_hw_version != SAHARA_VERSION2)
            && (sah_hw_version != SAHARA_VERSION3)) {
#ifdef DIAG_DRV_IF
            LOG_KDIAG("Sahara HW Version was not expected value.");
#endif
            os_error_code = OS_ERROR_FAIL_S;
        }
    }

    if (os_error_code == 0) {
#ifdef DIAG_DRV_IF
        LOG_KDIAG("Calling sah_Init_Mem_Map to initialise "
                  "memory subsystem.");
#endif
        /* Do any memory-routine initialization */
        os_error_code = sah_Init_Mem_Map ();
    }

    if (os_error_code == 0) {
#ifdef DIAG_DRV_IF
        LOG_KDIAG("Calling sah_HW_Reset() to Initialise the Hardware.");
#endif
        /* Initialise the hardware */
        os_error_code = sah_HW_Reset();
        if (os_error_code != OS_ERROR_OK_S) {
#ifdef DIAG_DRV_IF
            LOG_KDIAG("sah_HW_Reset() failed to Initialise the Hardware.");
#endif
        }

    }

    if (os_error_code == 0) {
#if defined(CONFIG_DEVFS_FS) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
        /* Register the DEVFS entry */
        Sahara_devfs_handle = devfs_register(
                                             NULL,
                                             SAHARA_DEVICE_SHORT,
                                             DEVFS_FL_AUTO_DEVNUM,
                                             0, 0,
                                             SAHARA_DEVICE_MODE,
                                             &Fops,
                                             NULL);
        if (Sahara_devfs_handle == NULL) {
#ifdef DIAG_DRV_IF
            LOG_KDIAG("Registering the DEVFS character device failed.");
#endif /* DIAG_DRV_IF */
            os_error_code = -EBUSY;
        }
#else   /* CONFIG_DEVFS_FS */
        /* Create the PROCFS entry. This is used to report the assigned device
         * major number back to user-space. */
#if 1
        Sahara_procfs_handle = create_proc_read_entry(
               SAHARA_DEVICE_SHORT,
               0,          /* default mode */
               NULL,       /* parent dir */
               device_read_procfs,
               NULL);      /* client data */
        if (Sahara_procfs_handle == NULL) {
#ifdef DIAG_DRV_IF
            LOG_KDIAG("Registering the PROCFS interface failed.");
#endif /* DIAG_DRV_IF */
            os_error_code = OS_ERROR_FAIL_S;
        }
#endif /* #if 1 */
    }

    if (os_error_code == 0) {
#ifdef DIAG_DRV_IF
        LOG_KDIAG("Calling sah_Queue_Manager_Init() to Initialise the Queue "
                  "Manager.");
#endif
        /* Initialise the Queue Manager */
        if (sah_Queue_Manager_Init() != FSL_RETURN_OK_S) {
            os_error_code =  -ENOMEM;
        }
    }

#ifndef SAHARA_POLL_MODE
    if (os_error_code == 0) {
#ifdef DIAG_DRV_IF
        LOG_KDIAG("Calling sah_Intr_Init() to Initialise the Interrupt "
                  "Handler.");
#endif
        /* Initialise the Interrupt Handler */
        os_error_code = sah_Intr_Init(&Wait_queue);
        if (os_error_code == OS_ERROR_OK_S) {
            interrupt_registered = 1;
        }
    }
#endif /* ifndef SAHARA_POLL_MODE */

#ifdef SAHARA_POWER_MANAGEMENT
    if (os_error_code == 0) {
        /* set up dynamic power management (dmp) */
        os_error_code = sah_dpm_init();
    }
#endif

    if (os_error_code == 0) {
        Major = register_chrdev(0, "sahara", &Fops);

        if (Major < 0) {
#ifdef DIAG_DRV_IF
            snprintf(Diag_msg, DIAG_MSG_SIZE, "Registering the regular "
                     "character device failed with error code: %d\n", Major);
            LOG_KDIAG(Diag_msg);
#endif
            os_error_code = Major;
        }
    }
#endif  /* CONFIG_DEVFS_FS */

    if (os_error_code != 0) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
        cleanup_module();
#else
        sah_cleanup();
#endif
    }
#ifdef DIAG_DRV_IF
    else {
        sprintf(err_string, "Sahara major node is %d\n", Major);
        LOG_KDIAG(err_string);
    }
#endif

    os_dev_init_return(os_error_code);
}


/*!
*******************************************************************************
* This function gets called when the module is removed (rmmod) from the running
* kernel.
*
* @brief     SAHARA device clean-up function.
*
* @return   void
*/
OS_DEV_SHUTDOWN(sah_cleanup)
{
    /* Unregister the device */
#if defined(CONFIG_DEVFS_FS) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    devfs_unregister(Sahara_devfs_handle);
#else
    int ret_val = 0;


    if (Sahara_procfs_handle != NULL) {
        remove_proc_entry(SAHARA_DEVICE_SHORT, NULL);
    }

    if (Major >= 0) {
        ret_val = unregister_chrdev(Major, SAHARA_DEVICE);
    }

#ifdef DIAG_DRV_IF
    if (ret_val < 0) {
        snprintf(Diag_msg, DIAG_MSG_SIZE, "Error while attempting to "
                 "unregister the device: %d\n", ret_val);
        LOG_KDIAG(Diag_msg);
    }
#endif

#endif  /* CONFIG_DEVFS_FS */
    sah_Queue_Manager_Close();
#ifndef SAHARA_POLL_MODE
    if (interrupt_registered) {
        sah_Intr_Release();
        interrupt_registered = 0;
    }
#endif
    sah_Stop_Mem_Map();
#ifdef SAHARA_POWER_MANAGEMENT
    sah_dpm_close();
#endif

    os_dev_shutdown_return(OS_ERROR_OK_S);
}


/*!
*******************************************************************************
* This function simply increments the module usage count.
*
* @brief     SAHARA device open function.
*
* @param    inode Part of the kernel prototype.
* @param    file Part of the kernel prototype.
*
* @return   0 - Always returns 0 since any number of calls to this function are
*               allowed.
*
*/
OS_DEV_OPEN(device_open)
{

#if defined(LINUX_VERSION) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
    MOD_INC_USE_COUNT;
#endif

#ifdef DIAG_DRV_IF
    Device_in_use++;
    snprintf(Diag_msg, DIAG_MSG_SIZE, "Incrementing module use count to: %d ",
             Device_in_use);
    LOG_KDIAG(Diag_msg);
#endif

    os_dev_set_user_private(NULL);

    /* Return 0 to indicate success */
    os_dev_open_return(0);
}


/*!
*******************************************************************************
* This function simply decrements the module usage count.
*
* @brief     SAHARA device release function.
*
* @param    inode   Part of the kernel prototype.
* @param    file    Part of the kernel prototype.
*
* @return   0 - Always returns 0 since this function does not fail.
*/
OS_DEV_CLOSE(device_release)
{
    fsl_shw_uco_t* user_ctx = os_dev_get_user_private();

#if defined(LINUX_VERSION) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
    MOD_DEC_USE_COUNT;
#endif

#ifdef DIAG_DRV_IF
    Device_in_use--;
    snprintf(Diag_msg, DIAG_MSG_SIZE, "Decrementing module use count to: %d ",
             Device_in_use);
    LOG_KDIAG(Diag_msg);
#endif

    if (user_ctx != NULL) {
        sah_handle_deregistration(user_ctx);
        os_free_memory(user_ctx);
        os_dev_set_user_private(NULL);
    }

    /* Return 0 to indicate success */
    os_dev_close_return(OS_ERROR_OK_S);
}


/*!
*******************************************************************************
* This function provides the IO Controls for the SAHARA driver. Three IO
* Controls are supported:
*
*   SAHARA_HWRESET and
*   SAHARA_SET_HA
*   SAHARA_CHK_TEST_MODE
*
* @brief SAHARA device IO Control function.
*
* @param    inode   Part of the kernel prototype.
* @param    filp    Part of the kernel prototype.
* @param    cmd     Part of the kernel prototype.
* @param    arg     Part of the kernel prototype.
*
* @return   0 on success
* @return   -EBUSY if the HA bit could not be set due to busy hardware.
* @return   -ENOTTY if an unsupported IOCTL was attempted on the device.
* @return   -EFAULT if put_user() fails
*/
OS_DEV_IOCTL(device_ioctl)
{
    int status = 0;
    int test_mode;

    switch (os_dev_get_ioctl_op()) {
    case SAHARA_HWRESET:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_HWRESET IOCTL.");
#endif
        /* We need to reset the hardware. */
        sah_HW_Reset();

        /* Mark all the entries in the Queue Manager's queue with state
         * SAH_STATE_RESET.
         */
        sah_Queue_Manager_Reset_Entries();

        /* Wake up all sleeping write() calls. */
        wake_up_interruptible(&Wait_queue);
        break;
#ifdef SAHARA_HA_ENABLED
    case SAHARA_SET_HA:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_SET_HA IOCTL.");
#endif /* DIAG_DRV_IF */
        if (sah_HW_Set_HA() == ERR_INTERNAL) {
            status = -EBUSY;
        }
        break;
#endif /* SAHARA_HA_ENABLED */

    case SAHARA_CHK_TEST_MODE:
        /* load test_mode */
        test_mode = TEST_MODE_OFF;
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_CHECK_TEST_MODE IOCTL.");
        test_mode = TEST_MODE_ON;
#endif /* DIAG_DRV_IF */
#if defined(KERNEL_TEST) || defined(PERF_TEST)
        test_mode = TEST_MODE_ON;
#endif /* KERNEL_TEST || PERF_TEST */
        /* copy test_mode back to user space.  put_user() is Linux fn */
        /* compiler warning `register': no problem found so ignored */
        status = put_user(test_mode, (int *)os_dev_get_ioctl_arg());
        break;

    case SAHARA_DAR:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_DAR IOCTL.");
#endif /* DIAG_DRV_IF */
        {
            fsl_shw_uco_t* user_ctx = os_dev_get_user_private();

            if (user_ctx != NULL) {
                status =
                    handle_device_ioctl_dar(user_ctx, os_dev_get_ioctl_arg());
            } else {
                status = OS_ERROR_FAIL_S;
            }
        }
        break;

    case SAHARA_GET_RESULTS:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_GET_RESULTS IOCTL.");
#endif /* DIAG_DRV_IF */
        {
            fsl_shw_uco_t * user_ctx = os_dev_get_user_private();

            if (user_ctx != NULL) {
                status =
                    sah_get_results_pointers(user_ctx, os_dev_get_ioctl_arg());
            } else {
                status = OS_ERROR_FAIL_S;
            }
        }
        break;

    case SAHARA_REGISTER:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_REGISTER IOCTL.");
#endif /* DIAG_DRV_IF */
        {
            fsl_shw_uco_t* user_ctx = os_dev_get_user_private();

            if (user_ctx != NULL) {
                status = OS_ERROR_FAIL_S; /* already registered */
            } else {
                user_ctx = os_alloc_memory(sizeof(fsl_shw_uco_t), GFP_KERNEL);
                if (user_ctx == NULL) {
                    status = OS_ERROR_NO_MEMORY_S;
                } else {
                    if (os_copy_from_user(user_ctx,
                                          (void *)os_dev_get_ioctl_arg(),
                                          sizeof(fsl_shw_uco_t))) {
                        status = OS_ERROR_FAIL_S;
                    } else {
                        os_dev_set_user_private(user_ctx);
                        status = sah_handle_registration(user_ctx);
                    }
                }
            }
        }
        break;

        /* This ioctl cmd should disappear in favor of a close() routine.  */
    case SAHARA_DEREGISTER:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_DEREGISTER IOCTL.");
#endif /* DIAG_DRV_IF */
        {
            fsl_shw_uco_t* user_ctx = os_dev_get_user_private();

            if (user_ctx == NULL) {
                status = OS_ERROR_FAIL_S;
            } else {
                status = sah_handle_deregistration(user_ctx);
                os_free_memory(user_ctx);
                os_dev_set_user_private(NULL);
            }
        }
        break;

    case SAHARA_SCC_ALLOC:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_SCC_ALLOC IOCTL.");
#endif /* DIAG_DRV_IF */
        status = sah_handle_scc_slot_alloc(os_dev_get_ioctl_arg());
        break;

    case SAHARA_SCC_DEALLOC:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_SCC_DEALLOC IOCTL.");
#endif /* DIAG_DRV_IF */
        status = sah_handle_scc_slot_dealloc(os_dev_get_ioctl_arg());
        break;

    case SAHARA_SCC_LOAD:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("SAHARA_SCC_LOAD IOCTL.");
#endif /* DIAG_DRV_IF */
        status = sah_handle_scc_slot_load(os_dev_get_ioctl_arg());
        break;

    case SAHARA_SCC_SLOT_DEC:
        status = sah_handle_scc_slot_decrypt(os_dev_get_ioctl_arg());
        break;

    case SAHARA_SCC_SLOT_ENC:
        status = sah_handle_scc_slot_encrypt(os_dev_get_ioctl_arg());
        break;

    default:
#ifdef DIAG_DRV_IF
        LOG_KDIAG("Unknown SAHARA IOCTL.");
#endif /* DIAG_DRV_IF */
        status = OS_ERROR_FAIL_S;
    }

    os_dev_ioctl_return(status);
}


/*!
*******************************************************************************
* Allocates a slot in the SCC
*
* @brief Allocates a slot in the SCC
*
* @param    info   slot information
*
* @return   0 if pass; non-zero on error
*/
static os_error_code sah_handle_scc_slot_alloc(uint32_t info)
{
    scc_slot_t slot_info;
    os_error_code os_err;
    scc_return_t scc_ret;

    os_err = os_copy_from_user(&slot_info, (void*)info, sizeof(slot_info));
    if (os_err == OS_ERROR_OK_S) {
        scc_ret = scc_alloc_slot(slot_info.key_length, slot_info.ownerid,
                                 &slot_info.slot);
        if (scc_ret == SCC_RET_OK) {
            slot_info.code = FSL_RETURN_OK_S;
        } else if (scc_ret == SCC_RET_INSUFFICIENT_SPACE) {
            slot_info.code = FSL_RETURN_NO_RESOURCE_S;
        } else {
            slot_info.code = FSL_RETURN_ERROR_S;
        }

        /* Return error code and slot info */
        os_err = os_copy_to_user((void*)info, &slot_info, sizeof(slot_info));

        if (os_err != OS_ERROR_OK_S) {
            (void) scc_dealloc_slot(slot_info.ownerid, slot_info.slot);
        }
    }

    return os_err;
}


/*!
 * Deallocate a slot in the SCC
 *
 * @brief Deallocate a slot in the SCC
 *
 * @param    info   slot information
 *
 * @return   0 if pass; non-zero on error
 */
static os_error_code sah_handle_scc_slot_dealloc(uint32_t info)
{
    fsl_shw_return_t ret = FSL_RETURN_INTERNAL_ERROR_S;
    scc_slot_t slot_info;
    os_error_code os_err;
    scc_return_t scc_ret;

    os_err = os_copy_from_user(&slot_info, (void*)info, sizeof(slot_info));
    if (os_err == OS_ERROR_OK_S) {
        scc_ret = scc_dealloc_slot(slot_info.ownerid, slot_info.slot);

        if (scc_ret == SCC_RET_OK) {
            ret = FSL_RETURN_OK_S;
        } else {
            ret = FSL_RETURN_ERROR_S;
        }
        slot_info.code = ret;
        os_err = os_copy_to_user((void*)info, &slot_info, sizeof(slot_info));
    }

    return os_err;
}


/*!
 * Populate a slot in the SCC
 *
 * @brief Populate a slot in the SCC
 *
 * @param    info   slot information
 *
 * @return   0 if pass; non-zero on error
 */
static os_error_code sah_handle_scc_slot_load(uint32_t info)
{
    fsl_shw_return_t ret = FSL_RETURN_INTERNAL_ERROR_S;
    scc_slot_t slot_info;
    os_error_code os_err;
    scc_return_t scc_ret;
    uint8_t* key = NULL;

    os_err = os_copy_from_user(&slot_info, (void*)info, sizeof(slot_info));

    if (os_err == OS_ERROR_OK_S) {
        	/* Allow slop in alloc in case we are rounding up to word multiple */
            key = os_alloc_memory(slot_info.key_length + 3, GFP_KERNEL);
            if (key == NULL) {
                ret = FSL_RETURN_NO_RESOURCE_S;
                os_err == OS_ERROR_OK_S;
            } else {
                os_err = os_copy_from_user(key, slot_info.key,
                                           slot_info.key_length);
            }
        }

    if (os_err == OS_ERROR_OK_S) {
        unsigned key_length = slot_info.key_length;

        /* Round up if necessary, as SCC call wants a multiple of 32-bit
         * values for the full object being loaded. */
        if ((key_length & 3) != 0) {
            key_length += 4 - (key_length & 3);
        }
        scc_ret = scc_load_slot(slot_info.ownerid, slot_info.slot, key,
                                key_length);
        if (scc_ret == SCC_RET_OK) {
            ret = FSL_RETURN_OK_S;
        } else {
            ret = FSL_RETURN_ERROR_S;
        }

        slot_info.code = ret;
        os_err = os_copy_to_user((void*)info, &slot_info, sizeof(slot_info));
    }

    if (key != NULL) {
		memset(key, 0, slot_info.key_length);
        os_free_memory(key);
    }

    return os_err;
}


/*!
 * Decrypt info from a slot in the SCC
 *
 * @brief Decrypt info from a slot in the SCC
 *
 * @param    info   slot information
 *
 * @return   0 if pass; non-zero on error
 */
static os_error_code sah_handle_scc_slot_decrypt(uint32_t info)
{
    fsl_shw_return_t ret = FSL_RETURN_INTERNAL_ERROR_S;
    scc_slot_t slot_info;
    os_error_code os_err;
    scc_return_t scc_ret;
    uint8_t* key = NULL;

    os_err = os_copy_from_user(&slot_info, (void*)info, sizeof(slot_info));

    if (os_err == OS_ERROR_OK_S) {
		key = os_alloc_memory(slot_info.key_length, GFP_KERNEL);
        if (key == NULL) {
        	ret = FSL_RETURN_NO_RESOURCE_S;
            os_err == OS_ERROR_OK_S;
        } else {
				os_err =
				    os_copy_from_user(key, slot_info.key,
					      slot_info.key_length);
        }
    }

    if (os_err == OS_ERROR_OK_S) {
        scc_ret = scc_decrypt_slot(slot_info.ownerid, slot_info.slot,
                                   slot_info.key_length, key);
        if (scc_ret == SCC_RET_OK) {
            ret = FSL_RETURN_OK_S;
        } else {
            ret = FSL_RETURN_ERROR_S;
        }

        slot_info.code = ret;
        os_err = os_copy_to_user((void*)info, &slot_info, sizeof(slot_info));
    }

    if (key != NULL) {
		memset(key, 0, slot_info.key_length);
        os_free_memory(key);
    }

    return os_err;
}


/*!
 * Encrypt info in a slot in the SCC
 *
 * @brief Encrypt info in a slot in the SCC
 *
 * @param    info   slot information
 *
 * @return   0 if pass; non-zero on error
 */
static os_error_code sah_handle_scc_slot_encrypt(uint32_t info)
{
    fsl_shw_return_t ret = FSL_RETURN_INTERNAL_ERROR_S;
    scc_slot_t slot_info;
    os_error_code os_err;
    scc_return_t scc_ret;
    uint8_t* key = NULL;

    os_err = os_copy_from_user(&slot_info, (void*)info, sizeof(slot_info));

    if (os_err == OS_ERROR_OK_S) {
            key = os_alloc_memory(slot_info.key_length, GFP_KERNEL);
            if (key == NULL) {
                ret = FSL_RETURN_NO_RESOURCE_S;
            }
        }

    if (key != NULL) {
        scc_ret = scc_encrypt_slot(slot_info.ownerid, slot_info.slot,
                                   slot_info.key_length, key);

        if (scc_ret != SCC_RET_OK) {
            ret = FSL_RETURN_ERROR_S;
        } else {
			os_err =
			    os_copy_to_user(slot_info.key, key,
					    slot_info.key_length);
            if (os_err != OS_ERROR_OK_S) {
                ret = FSL_RETURN_INTERNAL_ERROR_S;
            } else {
                ret = FSL_RETURN_OK_S;
            }
        }

        slot_info.code = ret;
        os_err = os_copy_to_user((void*)info, &slot_info, sizeof(slot_info));

		memset(key, 0, slot_info.key_length);
        os_free_memory(key);
    }

    return os_err;
}


/*!
 * Register a user
 *
 * @brief Register a user
 *
 * @param  user_ctx  information about this user
 *
 * @return   status code
 */
fsl_shw_return_t sah_handle_registration(fsl_shw_uco_t *user_ctx)
{
    /* Initialize the user's result pool (like sah_Queue_Construct() */
    user_ctx->result_pool.head = NULL;
    user_ctx->result_pool.tail = NULL;
    user_ctx->result_pool.count = 0;

    return FSL_RETURN_OK_S;
}

/*!
 * Deregister a user
 *
 * @brief Deregister a user
 *
 * @param  user_ctx  information about this user
 *
 * @return   status code
 */
fsl_shw_return_t sah_handle_deregistration(fsl_shw_uco_t *user_ctx)
{
    return FSL_RETURN_OK_S;
}


/*!
 * Sets up memory to extract results from results pool
 *
 * @brief Sets up memory to extract results from results pool
 *
 * @param  user_ctx  information about this user
 * @param[in,out]   arg contains input parameters and fields that the driver
 *                   fills in
 *
 * @return   os error code or 0 on success
 */
int sah_get_results_pointers(fsl_shw_uco_t* user_ctx, uint32_t arg)
{
    sah_results results_arg;  /* kernel mode usable version of 'arg' */
    fsl_shw_result_t *user_results; /* user mode address of results */
    unsigned *user_actual;  /* user mode address of actual number of results */
    unsigned actual;            /* local memory of actual number of results */
    int ret_val = OS_ERROR_FAIL_S;
    sah_Head_Desc *finished_request;
    unsigned int loop;


    /* copy structure from user to kernel space */
    if (!os_copy_from_user(&results_arg, (void*)arg, sizeof(sah_results))) {
        /* save user space pointers */
        user_actual = results_arg.actual; /* where count goes */
        user_results = results_arg.results; /* where results goe */

        /* Set pointer for actual value to temporary kernel memory location */
        results_arg.actual = &actual;

        /* Allocate kernel memory to hold temporary copy of the results */
        results_arg.results =
            os_alloc_memory(sizeof(fsl_shw_result_t) * results_arg.requested,
                    GFP_KERNEL);

        /* if memory allocated, continue */
        if (results_arg.results == NULL) {
            ret_val = OS_ERROR_NO_MEMORY_S;
        } else {
            fsl_shw_return_t get_status;

            /* get the results */
            get_status = sah_get_results_from_pool(user_ctx, &results_arg);

            /* free the copy of the user space descriptor chain */
            for (loop = 0; loop < actual; ++loop) {
                /* get sah_Head_Desc from results and put user address into
                 * the return structure */
                finished_request = results_arg.results[loop].user_desc;
                results_arg.results[loop].user_desc =
                                            finished_request->user_desc;
                /* return the descriptor chain memory to the block free pool */
                sah_Free_Chained_Descriptors(finished_request);
            }

            /* if no errors, copy results and then the actual number of results
             * back to user space
             */
            if (get_status == FSL_RETURN_OK_S) {
                if (os_copy_to_user(user_results, results_arg.results,
                                 actual * sizeof(fsl_shw_result_t)) ||
                    os_copy_to_user(user_actual, &actual,
                                    sizeof(user_actual))) {
                    ret_val = OS_ERROR_FAIL_S;
                } else {
                    ret_val = 0; /* no error */
                }
            }
            /* free the allocated memory */
            os_free_memory(results_arg.results);
        }
    }

    return ret_val;
}


/*!
 * Extracts results from results pool
 *
 * @brief Extract results from results pool
 *
 * @param           user_ctx  information about this user
 * @param[in,out]   arg       contains input parameters and fields that the
 *                            driver fills in
 *
 * @return   status code
 */
fsl_shw_return_t sah_get_results_from_pool(volatile fsl_shw_uco_t *user_ctx,
                                           sah_results *arg)
{
    sah_Head_Desc *finished_request;
    unsigned int  loop = 0;
    os_lock_context_t int_flags;

    /* Get the number of results requested, up to total number of results
     * available
     */
    do{
        /* Protect state of user's result pool until we have retrieved and
         * remove the first entry, or determined that the pool is empty. */
        os_lock_save_context(desc_queue_lock, int_flags);
        finished_request = user_ctx->result_pool.head;

        if (finished_request != NULL) {
            sah_Queue_Remove_Entry((sah_Queue*)&user_ctx->result_pool);
            os_unlock_restore_context(desc_queue_lock, int_flags);

            arg->results[loop].user_ref = finished_request->user_ref;
            arg->results[loop].code = finished_request->result;
            arg->results[loop].detail1 = finished_request->fault_address;
            arg->results[loop].detail2 = finished_request->current_dar;
            arg->results[loop].user_desc = finished_request;

            loop++;
        } else {                /* finished_request is NULL */
            /* pool is empty*/
            os_unlock_restore_context(desc_queue_lock, int_flags);
        }

    } while ( (loop < arg->requested) && (finished_request != NULL) );

    /* record number of results actually obtained */
    *arg->actual = loop;

    return FSL_RETURN_OK_S;
}


/*!
 * Converts descriptor chain to kernel space (from user space) and submits
 * chain to Sahara for processing
 *
 * @brief Submits converted descriptor chain to sahara
 *
 * @param  user_ctx        Pointer to Kernel version of user's ctx
 * @param  user_space_desc user space address of descriptor chain that is
 *                         in user space
 *
 * @return   OS status code
 */
static int handle_device_ioctl_dar(fsl_shw_uco_t* user_ctx,
                                   uint32_t user_space_desc)
{
    int os_error_code = OS_ERROR_FAIL_S;
    sah_Head_Desc *desc_chain_head; /* chain in kernel - virtual address */

    /* This will re-create the linked list so that the SAHARA hardware can
     * DMA on it.
     */
    desc_chain_head = sah_Copy_Descriptors((sah_Head_Desc*)user_space_desc);

    if (desc_chain_head == NULL ) {
        /* We may have failed due to a -EFAULT as well, but we will return
         * OS_ERROR_NO_MEMORY_S since either way it is a memory related
         * failure.
         */
        os_error_code = OS_ERROR_NO_MEMORY_S;
    } else {
        fsl_shw_return_t stat;

        desc_chain_head->user_info = user_ctx;
        desc_chain_head->user_desc = (sah_Head_Desc*)user_space_desc;

        if (desc_chain_head->uco_flags & FSL_UCO_BLOCKING_MODE) {
#ifdef SAHARA_POLL_MODE
            sah_Handle_Poll(desc_chain_head);
#else
            sah_blocking_mode(desc_chain_head);
#endif
            stat = desc_chain_head->result;
            /* return the descriptor chain memory to the block free pool */
            sah_Free_Chained_Descriptors(desc_chain_head);
            /* Tell user how the call turned out */

            /* Copy 'result' back up to the result member.
             *
             * The dereference of the different member will cause correct the
             * arithmetic to occur on the user-space address because of the
             * missing dma/bus locations in the user mode version of the
             * sah_Desc structure. */
            os_error_code =
                os_copy_to_user((void*)(user_space_desc
                                        + offsetof(sah_Head_Desc, uco_flags)),
                                &stat, sizeof(fsl_shw_return_t));

        } else {                /* not blocking mode - queue and forget */

            if (desc_chain_head->uco_flags & FSL_UCO_CALLBACK_MODE) {
                user_ctx->process = os_get_process_handle();
                user_ctx->callback = sah_user_callback;
            }
#ifdef SAHARA_POLL_MODE
            /* will put results in result pool */
            sah_Handle_Poll(desc_chain_head);
#else
            /* just put someting in the DAR */
            sah_Queue_Manager_Append_Entry(desc_chain_head);
#endif
            /* assume all went well */
            os_error_code = OS_ERROR_OK_S;
        }
    }

    return os_error_code;
}


static void sah_user_callback(fsl_shw_uco_t* uco)
{
    os_send_signal(uco->process, SIGUSR2);
}


/*!
 * This function is called when a thread attempts to read from the /proc/sahara
 * file.  Upon read, statistics and information about the state of the driver
 * are returned in nthe supplied buffer.
 *
 * @brief     SAHARA PROCFS read function.
 *
 * @param    buf     Anything written to this buffer will be returned to the
 *                   user-space process that is reading from this proc entry.
 * @param    start   Part of the kernel prototype.
 * @param    offset  Part of the kernel prototype.
 * @param    count   The size of the buf argument.
 * @param    eof     An integer which is set to one to tell the user-space
 *                   process that there is no more data to read.
 * @param    data    Part of the kernel prototype.
 *
 * @return   The number of bytes written to the proc entry.
 */
#if !defined(CONFIG_DEVFS_FS) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
static int device_read_procfs(
    char *buf,
    char **start,
    off_t offset,
    int count,
    int *eof,
    void *data)
{
    int output_bytes = 0;
    int in_queue_count = 0;
    os_lock_context_t lock_context;


    os_lock_save_context(desc_queue_lock, lock_context);
    in_queue_count = sah_Queue_Manager_Count_Entries(TRUE, 0);
    os_unlock_restore_context(desc_queue_lock, lock_context);
    output_bytes += snprintf(buf, count - output_bytes, "queued: %d\n",
                             in_queue_count);
    output_bytes += snprintf(buf+output_bytes, count - output_bytes,
                             "Descriptors: %d, "
                             "Interrupts %d (%d Done1Done2, %d Done1Busy2, "
                             " %d Done1)\n",
                             dar_count, interrupt_count, done1done2_count,
                             done1busy2_count, done1_count);

    /* Signal the end of the file */
    *eof = 1;

    /* To get rid of the unused parameter warnings */
    start = NULL;
    data = NULL;
    offset = 0;

    return output_bytes;
}
#endif


#ifndef SAHARA_POLL_MODE
/*!
 * Block user call until processing is complete.
 *
 * @param entry    The user's request.
 *
 * @return   An OS error code, or 0 if no error
 */
int sah_blocking_mode(sah_Head_Desc *entry)
{
    int os_error_code = 0;
    sah_Queue_Status  status;

    /* queue entry, put someting in the DAR, if nothing is there currently */
    sah_Queue_Manager_Append_Entry(entry);

    /* get this descriptor chain's current status */
    status = ((volatile sah_Head_Desc*)entry)->status;

    while (!SAH_DESC_PROCESSED(status)) {
        extern sah_Queue *main_queue;


        DEFINE_WAIT(sahara_wait); /* create a wait queue entry. Linux */

        /* enter the wait queue entry into the queue */
        prepare_to_wait(&Wait_queue, &sahara_wait, TASK_INTERRUPTIBLE);

        /* check if this entry has been processed */
        status = ((volatile sah_Head_Desc*)entry)->status;

        if (!SAH_DESC_PROCESSED(status)) {
            /* go to sleep - Linux */
            schedule();
        }

        /* un-queue the 'prepare to wait' queue? - Linux */
        finish_wait(&Wait_queue, &sahara_wait);

        /* signal belongs to this thread? */
        if (signal_pending(current)) { /* Linux */
            os_lock_context_t lock_flags;

            /* don't allow access during this check and operation */
            os_lock_save_context(desc_queue_lock, lock_flags);
            status = ((volatile sah_Head_Desc*)entry)->status;
            if (status == SAH_STATE_PENDING) {
                sah_Queue_Remove_Any_Entry(main_queue, entry);
            }
            os_unlock_restore_context(desc_queue_lock, lock_flags);

            /* if the state was Pending, dephysicalize the entry. Otherwise,
             * loop around the while loop again and go to sleep if needed */
            if (status == SAH_STATE_PENDING) {
                entry->result = FSL_RETURN_INTERNAL_ERROR_S;
                entry = sah_DePhysicalise_Descriptors(entry);
                status = SAH_STATE_FAILED; /* exit loop */
            }
        }

        status = ((volatile sah_Head_Desc*)entry)->status;
    } /* while ... */

    return os_error_code;
}


/*!
 * If interrupt doesnt return in a resasonable time, time out, trigger
 * interrupt, and continue with process
 *
 * @param    data   ignored
 */
void sahara_timeout_handler(unsigned long data)
{
    /* Sahara has not issuing an interrupt, so timed out */
#ifdef DIAG_DRV_IF
    LOG_KDIAG("Sahara HW did not respond.  Resetting.\n");
#endif
    /* assume hardware needs resetting */
    sah_Handle_Interrupt(SAH_EXEC_FAULT);
    /* wake up sleeping thread to try again */
    wake_up_interruptible(&Wait_queue);
}

#endif /* ifndef SAHARA_POLL_MODE */

/* End of sah_driver_interface.c */
