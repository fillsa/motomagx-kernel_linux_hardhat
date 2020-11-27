/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
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
 * @file mxc_scc.c
 *
 * @brief This is the driver code for the Security Controller (SCC).  It has no device
 * driver interface, so no user programs may access it.  Its interaction with
 * the Linux kernel is from calls to #scc_init() when the driver is loaded, and
 * #scc_cleanup() should the driver be unloaded.  The driver uses locking and
 * (task-sleep/task-wakeup) functions of the kernel.  It also registers itself
 * to handle the interrupt line(s) from the SCC.
 *
 * Other drivers in the kernel may use the remaining API functions to get at
 * the services of the SCC.  The main service provided is the Secure Memory,
 * which allows encoding and decoding of secrets with a per-chip secret key.
 *
 * The SCC is single-threaded, and so is this module.  When the scc_crypt()
 * routine is called, it will lock out other accesses to the function.  If
 * another task is already in the module, the subsequent caller will spin on a
 * lock waiting for the other access to finish.
 *
 * Note that long crypto operations could cause a task to spin for a while,
 * preventing other kernel work (other than interrupt processing) to get done.
 *
 * The external (kernel module) interface is through the following functions:
 * @li scc_get_configuration()
 * @li scc_crypt()
 * @li scc_zeroize_memories()
 * @li scc_monitor_security_failure()
 * @li scc_stop_monitoring_security_failure()
 * @li scc_set_sw_alarm()
 * @li scc_read_register()
 * @li scc_write_register()
 *
 * All other functions are internal to the driver.
 *
 * @ingroup MXCSCC
 */


#include "mxc_scc_internals.h"
#include <asm/arch/clock.h>
#include <linux/device.h>

/**
 * This is the set of errors which signal that access to the SCM RAM has
 * failed or will fail.
 */
#define SCM_ACCESS_ERRORS                                                  \
       (SCM_ERR_USER_ACCESS | SCM_ERR_ILLEGAL_ADDRESS |                    \
        SCM_ERR_ILLEGAL_MASTER | SCM_ERR_CACHEABLE_ACCESS |                \
        SCM_ERR_UNALIGNED_ACCESS | SCM_ERR_BYTE_ACCESS |                   \
        SCM_ERR_INTERNAL_ERROR | SCM_ERR_SMN_BLOCKING_ACCESS |             \
        SCM_ERR_CIPHERING | SCM_ERR_ZEROIZING | SCM_ERR_BUSY)

/******************************************************************************
 *
 *  Global / Static Variables
 *
 *****************************************************************************/

/*!
 * This is type void* so that a) it cannot directly be dereferenced,
 * and b) pointer arithmetic on it will function in a 'normal way' for
 * the offsets in scc_defines.h
 *
 * scc_base is the location in the iomap where the SCC's registers
 * (and memory) start.
 *
 * The referenced data is declared volatile so that the compiler will
 * not make any assumptions about the value of registers in the SCC,
 * and thus will always reload the register into CPU memory before
 * using it (i.e. wherever it is referenced in the driver).
 *
 * This value should only be referenced by the #SCC_READ_REGISTER and
 * #SCC_WRITE_REGISTER macros and their ilk.  All dereferences must be
 * 32 bits wide.
 */
#define static
static volatile void *scc_base;

/*! Array to hold function pointers registered by
    #scc_monitor_security_failure() and processed by
    #scc_perform_callbacks() */
static void (*scc_callbacks[SCC_CALLBACK_SIZE])(void);


/*! Structure returned by #scc_get_configuration() */
static scc_config_t scc_configuration =
{
    .driver_major_version  =  SCC_DRIVER_MAJOR_VERSION_1,
    .driver_minor_version  =  SCC_DRIVER_MINOR_VERSION_5,
    .scm_version           = -1,
    .smn_version           = -1,
    .block_size_bytes      = -1,
    .black_ram_size_blocks = -1,
    .red_ram_size_blocks   = -1
};

/*! Key Control Information.  Integrity is controlled by use of
    #scc_crypto_lock. */
static struct scc_key_slot scc_key_info[SCC_KEY_SLOTS];

/*! Internal flag to know whether SCC is in Failed state (and thus many
 *  registers are unavailable).  Once it goes failed, it never leaves it. */
static volatile enum scc_status scc_availability = SCC_STATUS_INITIAL;


/*! Flag to say whether interrupt handler has been registered for
 * SMN interrupt */
static int smn_irq_set = 0;


/*! Flag to say whether interrupt handler has been registered for
 * SCM interrupt */
static int scm_irq_set = 0;


/*! This lock protects the #scc_callbacks list as well as the @c
 * callbacks_performed flag in #scc_perform_callbacks.  Since the data this
 * protects may be read or written from either interrupt or base level, all
 * operations should use the irqsave/irqrestore or similar to make sure that
 * interrupts are inhibited when locking from base level.
 */
static spinlock_t scc_callbacks_lock = SPIN_LOCK_UNLOCKED;


/*!
 * Ownership of this lock prevents conflicts on the crypto operation in the SCC
 * and the integrity of the #scc_key_info.
 */
static spinlock_t scc_crypto_lock = SPIN_LOCK_UNLOCKED;


/*! Calculated once for quick reference to size of the unreserved space in one
 *  RAM in SCM.
 */
static uint32_t scc_memory_size_bytes;


/*! Calculated once for quick reference to size of SCM address space */
static uint32_t scm_highest_memory_address;


/*! The lookup table for an 8-bit value.  Calculated once
 * by #scc_init_ccitt_crc().
 */
static uint16_t  scc_crc_lookup_table[256];


/*! Fixed padding for appending to plaintext to fill out a block */
static uint8_t scc_block_padding[8] =
    {SCC_DRIVER_PAD_CHAR, 0, 0, 0, 0, 0, 0, 0};

/*!
 * This function is called to put the SCC in a low power state.
 *
 * @param   dev   the device structure used to give information on which SCC
 *                device (0 through 3 channels) to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_scc_suspend(struct device *dev, u32 state, u32 level)
{
#ifdef SCC_DEBUG
    printk(" MXC SCC driver suspend function\n");
#endif
    switch(level) {
    case SUSPEND_DISABLE:
        /* TBD */
        break;
    case SUSPEND_SAVE_STATE:
        /* TBD */
        break;
    case SUSPEND_POWER_DOWN:
        /* Turn off clock */
        mxc_clks_disable(SCC_CLK);
        break;
    }
    return 0;
}

/*!
 * This function is called to resume the SCC from a low power state.
 *
 * @param   dev   the device structure used to give information on which SCC
 *                device (0 through 3 channels) to suspend
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_scc_resume(struct device *dev, u32 level)
{
#ifdef SCC_DEBUG
    printk("MXC SCC driver resume function\n");
#endif
    switch(level) {
    case RESUME_POWER_ON:
        /* Turn on clock */
        mxc_clks_enable(SCC_CLK);
        break;
    case RESUME_RESTORE_STATE:
        /* TBD */
        break;
    case RESUME_ENABLE:
        /* TBD */
        break;
    }
    return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxc_scc_driver = {
    .name           = "mxc_scc",
    .bus            = &platform_bus_type,
    .suspend        = mxc_scc_suspend,
    .resume         = mxc_scc_resume,
};
#undef static

/******************************************************************************
 *
 *  Function Implementations - Externally Accessible
 *
 *****************************************************************************/


/*****************************************************************************/
/* fn scc_init()                                                             */
/*****************************************************************************/
/*!
 *  Initialize the driver at boot time or module load time.
 *
 *  Register with the kernel as the interrupt handler for the SCC interrupt
 *  line(s).
 *
 *  Map the SCC's register space into the driver's memory space.
 *
 *  Query the SCC for its configuration and status.  Save the configuration in
 *  #scc_configuration and save the status in #scc_availability.  Called by the
 *  kernel.
 *
 *  Do any locking/wait queue initialization which may be necessary.
 *
 *  The availability fuse may be checked, depending on platform.
 */
static int
scc_init(void)
{
    uint32_t smn_status;
    int i, ret;
    int return_value = -EIO;       /* assume error */

    ret = driver_register(&mxc_scc_driver);
    if (ret != 0) {
        return return_value;
    }

    if ( scc_availability == SCC_STATUS_INITIAL ) {

        /* Set this until we get an initial reading */
        scc_availability = SCC_STATUS_CHECKING;

        /* Initialize the constant for the CRC function */
        scc_init_ccitt_crc();

        /* initialize the callback table */
        for (i = 0; i < SCC_CALLBACK_SIZE; i++) {
            scc_callbacks[i] = 0;
        }

        /* Initialize key slots */
        for (i = 0; i < SCC_KEY_SLOTS; i++) {
            scc_key_info[i].offset = i * SCC_KEY_SLOT_SIZE;
            scc_key_info[i].status = 0; /* unassigned */
        }

        /* See whether there is an SCC available */
        if ( 0 && !SCC_ENABLED() ) {
#ifdef SCC_DEBUG
            printk("SCC: Fuse for SCC is set to disabled.  Exiting.\n");
#endif
        } else {
            /* Map the SCC (SCM and SMN) memory on the internal bus into
               kernel address space */
            scc_base = ioremap_nocache(SCC_BASE, SCC_ADDRESS_RANGE);

            /* If that worked, we can try to use the SCC */
            if (scc_base == NULL) {
#ifdef SCC_DEBUG
                printk("SCC: Register mapping failed.  Exiting.\n");
#endif
            } else {
                /* Get SCM into 'clean' condition w/interrupts cleared &
                   disabled */
                SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                                   SCM_INTERRUPT_CTRL_CLEAR_INTERRUPT
                                   | SCM_INTERRUPT_CTRL_MASK_INTERRUPTS);

                /* Clear error status register (any write will do it) */
                SCC_WRITE_REGISTER(SCM_ERROR_STATUS, 0);

                /*
                 * There is an SCC.  Determine its current state.  Side effect
                 * is to populate scc_config and scc_availability
                 */
                smn_status = scc_grab_config_values();

                /* Try to set up interrupt handler(s) */
                if (scc_availability == SCC_STATUS_OK) {
                    if (setup_interrupt_handling() != 0) {
                        /**
                         * The error could be only that the SCM interrupt was
                         * not set up.  This interrupt is always masked, so
                         * that is not an issue.
                         *
                         * The SMN's interrupt may be shared on that line, it
                         * may be separate, or it may not be wired.  Do what
                         * is necessary to check its status.
                         *
                         * Although the driver is coded for possibility of not
                         * having SMN interrupt, the fact that there is one
                         * means it should be available and used.
                         */
#ifdef USE_SMN_INTERRUPT
                        if (!smn_irq_set) { /* Separate. Check SMN binding */
#elif !defined(NO_SMN_INTERRUPT)
                        if (!scm_irq_set) { /* Shared. Check SCM binding */
#else
                        if ( FALSE ) { /*  SMN not wired at all.  Ignore. */
#endif
                            /* setup was not able to set up SMN interrupt */
                            scc_availability = SCC_STATUS_UNIMPLEMENTED;
                        }
                    } /* interrupt handling returned non-zero */
                } /* availability is OK */
                if (scc_availability == SCC_STATUS_OK) {
                    /* Get SMN into 'clean' condition w/interrupts cleared &
                       enabled */
                    SCC_WRITE_REGISTER(SMN_COMMAND,
                                       SMN_COMMAND_CLEAR_INTERRUPT
                                       | SMN_COMMAND_ENABLE_INTERRUPT);
                } /* availability is still OK */

                } /* if scc_base != NULL */

        } /* if SCC_ENABLED() */

        /*
         * If status is SCC_STATUS_UNIMPLEMENTED or is still
         * SCC_STATUS_CHECKING, could be leaving here with the driver partially
         * initialized.  In either case, cleanup (which will mark the SCC as
         * UNIMPLEMENTED).
         */
        if (scc_availability == SCC_STATUS_CHECKING ||
            scc_availability == SCC_STATUS_UNIMPLEMENTED) {
            scc_cleanup();
       }
        else {
            return_value = 0;   /* All is well */
        }
    } /* ! STATUS_INITIAL */

#ifdef SCC_DEBUG
    printk("SCC: Driver Status is %s\n",
           (scc_availability==SCC_STATUS_INITIAL) ? "INITIAL" :
           (scc_availability==SCC_STATUS_CHECKING) ? "CHECKING" :
           (scc_availability==SCC_STATUS_UNIMPLEMENTED) ? "UNIMPLEMENTED" :
           (scc_availability==SCC_STATUS_OK) ? "OK" :
           (scc_availability==SCC_STATUS_FAILED) ? "FAILED" :
           "UNKNOWN");
#endif

    return return_value;
} /* scc_init */


/*****************************************************************************/
/* fn scc_cleanup()                                                          */
/*****************************************************************************/
/*!
 * Perform cleanup before driver/module is unloaded by setting the machine
 * state close to what it was when the driver was loaded.  This function is
 * called when the kernel is shutting down or when this driver is being
 * unloaded.
 *
 * A driver like this should probably never be unloaded, especially if there
 * are other module relying upon the callback feature for monitoring the SCC
 * status.
 *
 * In any case, cleanup the callback table (by clearing out all of the
 * pointers).  Deregister the interrupt handler(s).  Unmap SCC registers.
 *
 */
static void
scc_cleanup(void)
{
    int i;

    driver_unregister(&mxc_scc_driver);
    /* Mark the driver / SCC as unusable. */
    scc_availability = SCC_STATUS_UNIMPLEMENTED;

    /* Clear out callback table */
    for (i = 0; i < SCC_CALLBACK_SIZE; i++) {
        scc_callbacks[i] = 0;
    }

    /* If SCC has been mapped in, clean it up and unmap it */
    if (scc_base) {
        /* For the SCM, disable interrupts, zeroize RAMs.  Interrupt
         * status will appear because zeroize will complete. */
        SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                           SCM_INTERRUPT_CTRL_MASK_INTERRUPTS |
                           SCM_INTERRUPT_CTRL_ZEROIZE_MEMORY);

        /* For the SMN, clear and disable interrupts */
        SCC_WRITE_REGISTER(SMN_COMMAND, SMN_COMMAND_CLEAR_INTERRUPT);

        /* remove virtual mapping */
        iounmap((void*)scc_base);
    }

    /* Now that interrupts cannot occur, disassociate driver from the interrupt
     * lines.
     */

    /* Deregister SCM interrupt handler */
    if (scm_irq_set) {
        free_irq (INT_SCC_SCM, NULL);
    }

    /* Deregister SMN interrupt handler */
    if (smn_irq_set) {
#ifdef USE_SMN_INTERRUPT
        free_irq (INT_SCC_SMN, NULL);
#endif
    }

#ifdef SCC_DEBUG
    printk("SCC driver cleaned up.\n");
#endif

} /* scc_cleanup */


/*****************************************************************************/
/* fn scc_get_configuration()                                                */
/*****************************************************************************/
scc_config_t*
scc_get_configuration(void)
{
    /*
     * If some other driver calls scc before the kernel does, make sure that
     * this driver's initialization is performed.
     */
    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    /**
     * If there is no SCC, yet the driver exists, the value -1 will be in
     * the #scc_config_t fields for other than the driver versions.
     */
    return &scc_configuration;
} /* scc_get_configuration */


/*****************************************************************************/
/* fn scc_zeroize_memories()                                                 */
/*****************************************************************************/
scc_return_t
scc_zeroize_memories(void)
{
    scc_return_t return_status = SCC_RET_FAIL;
    uint32_t status;

    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    if (scc_availability == SCC_STATUS_OK) {
        unsigned long irq_flags;    /* for IRQ save/restore */

        /* Lock access to crypto memory of the SCC */
        spin_lock_irqsave(&scc_crypto_lock, irq_flags);

        /* Start the Zeroize by setting a bit in the SCM_INTERRUPT_CTRL
         * register */
        SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                           SCM_INTERRUPT_CTRL_MASK_INTERRUPTS
                           | SCM_INTERRUPT_CTRL_ZEROIZE_MEMORY);

        scc_wait_completion();

        /* Get any error info */
        status = SCC_READ_REGISTER(SCM_ERROR_STATUS);

        /* unlock the SCC */
        spin_unlock_irqrestore(&scc_crypto_lock, irq_flags);

        if (! (status & SCM_ERR_ZEROIZE_FAILED)) {
            return_status = SCC_RET_OK;
        }
#ifdef SCC_DEBUG
        else {
            printk("SCC: Zeroize failed.  SCM Error Status is 0x%08x\n",
                   status);
        }
#endif

        /* Clear out any status.*/
        SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                           SCM_INTERRUPT_CTRL_CLEAR_INTERRUPT
                           | SCM_INTERRUPT_CTRL_MASK_INTERRUPTS);

        /* and any error status */
        SCC_WRITE_REGISTER(SCM_ERROR_STATUS, 0);
    }

    return return_status;
} /* scc_zeroize_memories */


/*****************************************************************************/
/* fn scc_crypt()                                                            */
/*****************************************************************************/
scc_return_t
scc_crypt(unsigned long count_in_bytes, uint8_t *data_in, uint8_t* init_vector,
          scc_enc_dec_t direction, scc_crypto_mode_t crypto_mode,
          scc_verify_t check_mode, uint8_t *data_out,
          unsigned long* count_out_bytes)
{
    scc_return_t return_code = SCC_RET_FAIL;

    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    (void) scc_update_state();  /* in case no interrupt line from SMN */

    /* make initial error checks */
    if (scc_availability != SCC_STATUS_OK
        || count_in_bytes == 0
        || data_in == 0
        || data_out == 0
        || (crypto_mode != SCC_CBC_MODE && crypto_mode != SCC_ECB_MODE)
        || (crypto_mode == SCC_CBC_MODE && init_vector == NULL)
        || (direction != SCC_ENCRYPT && direction != SCC_DECRYPT)
        || (check_mode == SCC_VERIFY_MODE_NONE &&
            count_in_bytes % SCC_BLOCK_SIZE_BYTES() != 0)
        || (direction == SCC_DECRYPT &&
            count_in_bytes % SCC_BLOCK_SIZE_BYTES() != 0)
        || (check_mode != SCC_VERIFY_MODE_NONE &&
            check_mode != SCC_VERIFY_MODE_CCITT_CRC)) {
#ifdef SCC_DEBUG
        printk("SCC: scc_crypt() detected bad argument\n");
#endif
    }
    else {
        /* Start settings for write to SCM_CONTROL register  */
        uint32_t scc_control = SCM_CONTROL_START_CIPHER;
        unsigned long irq_flags;    /* for IRQ save/restore */

        /* Lock access to crypto memory of the SCC */
        spin_lock_irqsave(&scc_crypto_lock, irq_flags);


        /* Special needs for CBC Mode */
        if (crypto_mode == SCC_CBC_MODE) {
            scc_control |= SCM_CBC_MODE; /* change default of ECB */
            /* Put in Initial Context.  Vector registers are contiguous */
            copy_to_scc(init_vector, SCM_INIT_VECTOR_0,
                        SCC_BLOCK_SIZE_BYTES(), NULL);
        }

        /* Fill the RED_START register */
        SCC_WRITE_REGISTER(SCM_RED_START,
                           SCM_NON_RESERVED_OFFSET / SCC_BLOCK_SIZE_BYTES());

        /* Fill the BLACK_START register */
        SCC_WRITE_REGISTER(SCM_BLACK_START,
                           SCM_NON_RESERVED_OFFSET / SCC_BLOCK_SIZE_BYTES());

        if (direction == SCC_ENCRYPT) {
            /* Check for sufficient space in data_out */
            if (check_mode == SCC_VERIFY_MODE_NONE) {
                if (*count_out_bytes < count_in_bytes) {
                    return_code = SCC_RET_INSUFFICIENT_SPACE;
                }
            }
            else {                  /* SCC_VERIFY_MODE_CCITT_CRC */
                /* Calculate extra bytes needed for crc (2) and block
                   padding */
                int padding_needed = CRC_SIZE_BYTES + SCC_BLOCK_SIZE_BYTES() -
                    ((count_in_bytes + CRC_SIZE_BYTES)
                     % SCC_BLOCK_SIZE_BYTES());

                /* Verify space is available */
                if ( *count_out_bytes < count_in_bytes + padding_needed ) {
                    return_code = SCC_RET_INSUFFICIENT_SPACE;
                }
            }
            /* If did not detect space error, do the encryption */
            if (return_code != SCC_RET_INSUFFICIENT_SPACE) {
                return_code =
                    scc_encrypt(count_in_bytes, data_in, scc_control, data_out,
                                check_mode == SCC_VERIFY_MODE_CCITT_CRC,
                                count_out_bytes);
            }

        } /* direction == SCC_ENCRYPT */

        else {                  /* SCC_DECRYPT */
            /* Check for sufficient space in data_out */
            if (check_mode == SCC_VERIFY_MODE_NONE) {
                if (*count_out_bytes < count_in_bytes) {
                    return_code = SCC_RET_INSUFFICIENT_SPACE;
                }
            }
            else {                  /* SCC_VERIFY_MODE_CCITT_CRC */
                /* Do initial check.  Assume last block (of padding) and CRC
                 * will get stripped.  After decipher is done and padding is
                 * removed, will know exact value.
                 */
                int possible_size = (int)count_in_bytes - CRC_SIZE_BYTES
                    - SCC_BLOCK_SIZE_BYTES();
                if ((int)*count_out_bytes < possible_size) {
#ifdef SCC_DEBUG
                    printk("SCC: insufficient decrypt space %ld/%d.\n",
                           *count_out_bytes, possible_size);
#endif
                    return_code = SCC_RET_INSUFFICIENT_SPACE;
                }
            }

            /* If did not detect space error, do the decryption */
            if (return_code != SCC_RET_INSUFFICIENT_SPACE) {
                return_code =
                    scc_decrypt(count_in_bytes, data_in, scc_control, data_out,
                                check_mode == SCC_VERIFY_MODE_CCITT_CRC,
                                count_out_bytes);
            }

        } /* SCC_DECRYPT */

        /* unlock the SCC */
        spin_unlock_irqrestore(&scc_crypto_lock, irq_flags);

    } /* else no initial error */

    return return_code;
} /* scc_crypt */


/*****************************************************************************/
/* fn scc_set_sw_alarm()                                                     */
/*****************************************************************************/
void
scc_set_sw_alarm(void)
{

    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    /* Update scc_availability based on current SMN status.  This might
     * perform callbacks.
     */
    (void) scc_update_state();

    /* if everything is OK, make it fail */
    if (scc_availability == SCC_STATUS_OK) {

        /* sound the alarm (and disable SMN interrupts */
        SCC_WRITE_REGISTER(SMN_COMMAND, SMN_COMMAND_SET_SOFTWARE_ALARM);

        scc_availability = SCC_STATUS_FAILED; /* Remember what we've done */

        /* In case SMN interrupt is not available, tell the world */
        scc_perform_callbacks();
    }

    return;
} /* scc_set_sw_alarm */


/*****************************************************************************/
/* fn scc_monitor_security_failure()                                         */
/*****************************************************************************/
scc_return_t
scc_monitor_security_failure(void callback_func(void))
{
    int i;
    unsigned long irq_flags;    /* for IRQ save/restore */
    scc_return_t return_status = SCC_RET_TOO_MANY_FUNCTIONS;
    int function_stored = FALSE;

    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    /* Acquire lock of callbacks table.  Could be spin_lock_irq() if this
     * routine were just called from base (not interrupt) level
     */
    spin_lock_irqsave(&scc_callbacks_lock, irq_flags);

    /* Search through table looking for empty slot */
    for (i = 0; i < SCC_CALLBACK_SIZE; i++) {
        if (scc_callbacks[i] == callback_func) {
            if (function_stored) {
                /* Saved duplicate earlier.  Clear this later one. */
                scc_callbacks[i] = NULL;
            }
            /* Exactly one copy is now stored */
            return_status = SCC_RET_OK;
            break;
        }
        else if (scc_callbacks[i] == NULL && !function_stored) {
            /* Found open slot.  Save it and remember */
            scc_callbacks[i] = callback_func;
            return_status = SCC_RET_OK;
            function_stored = TRUE;
        }
    }

    /* Free the lock */
    spin_unlock_irqrestore(&scc_callbacks_lock, irq_flags);

    return return_status;
} /* scc_monitor_security_failure */


/*****************************************************************************/
/* fn scc_stop_monitoring_security_failure()                                 */
/*****************************************************************************/
void
scc_stop_monitoring_security_failure(void callback_func(void))
{
    unsigned long irq_flags;    /* for IRQ save/restore */
    int i;

    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    /* Acquire lock of callbacks table.  Could be spin_lock_irq() if this
     * routine were just called from base (not interrupt) level
     */
    spin_lock_irqsave(&scc_callbacks_lock, irq_flags);

    /* Search every entry of the table for this function */
    for (i = 0; i < SCC_CALLBACK_SIZE; i++) {
        if (scc_callbacks[i] == callback_func) {
            scc_callbacks[i] = NULL; /* found instance - clear it out */
            break;
        }
    }

    /* Free the lock */
    spin_unlock_irqrestore(&scc_callbacks_lock, irq_flags);

    return;
} /* scc_stop_monitoring_security_failure */


/*****************************************************************************/
/* fn scc_read_register()                                                    */
/*****************************************************************************/
scc_return_t
scc_read_register(int register_offset, uint32_t *value)
{
    scc_return_t return_status = SCC_RET_FAIL;
    uint32_t smn_status;
    uint32_t scm_status;

    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    /* First layer of protection -- completely unaccessible SCC */
    if (scc_availability != SCC_STATUS_UNIMPLEMENTED) {

        /* Second layer -- that offset is valid */
        if (register_offset != SMN_BITBANK_DECREMENT && /* write only! */
            check_register_offset(register_offset) == SCC_RET_OK) {

            /* Get current status / update local state */
            smn_status = scc_update_state();
            scm_status = SCC_READ_REGISTER(SCM_STATUS);

            /*
             * Third layer - verify that the register being requested is
             * available in the current state of the SCC.
             */
            if ((return_status =
                 check_register_accessible(register_offset, smn_status,
                                           scm_status)) == SCC_RET_OK) {
                *value = SCC_READ_REGISTER(register_offset);
            }
        }
    }

    return return_status;
} /* scc_read_register */


/*****************************************************************************/
/* fn scc_write_register()                                                   */
/*****************************************************************************/
scc_return_t
scc_write_register(int register_offset, uint32_t value)
{
    scc_return_t return_status = SCC_RET_FAIL;
    uint32_t smn_status;
    uint32_t scm_status;

    if (scc_availability == SCC_STATUS_INITIAL) {
        scc_init();
    }

    /* First layer of protection -- completely unaccessible SCC */
    if (scc_availability != SCC_STATUS_UNIMPLEMENTED) {

        /* Second layer -- that offset is valid */
        if (! (register_offset == SCM_STATUS || /* These registers are */
               register_offset == SCM_CONFIGURATION || /*  Read Only */
               register_offset == SMN_BIT_COUNT ||
               register_offset == SMN_TIMER) &&
            check_register_offset(register_offset) == SCC_RET_OK) {

            /* Get current status / update local state */
            smn_status = scc_update_state();
            scm_status = SCC_READ_REGISTER(SCM_STATUS);

            /*
             * Third layer - verify that the register being requested is
             * available in the current state of the SCC.
             */
            if (check_register_accessible(register_offset, smn_status,
                                          scm_status) == 0) {
                SCC_WRITE_REGISTER(register_offset, value);
                return_status = SCC_RET_OK;
            }
        }
    }

    return return_status;
} /* scc_write_register() */


/******************************************************************************
 *
 *  Function Implementations - Internal
 *
 *****************************************************************************/


/*****************************************************************************/
/* fn scc_irq()                                                              */
/*****************************************************************************/
/*!
 * This is the interrupt handler for the SCC.
 *
 * This function checks the SMN Status register to see whether it
 * generated the interrupt, then it checks the SCM Status register to
 * see whether it needs attention.
 *
 * If an SMN Interrupt is active, then the SCC state set to failure, and
 * #scc_perform_callbacks() is invoked to notify any interested parties.
 *
 * The SCM Interrupt should be masked, as this driver uses polling to determine
 * when the SCM has completed a crypto or zeroing operation.  Therefore, if the
 * interrupt is active, the driver will just clear the interrupt and (re)mask.
 *
 * @param irq Channel number for the IRQ. (@c SCC_INT_SMN or @c SCC_INT_SCM).
 * @param dev_id Pointer to the identification of the device.  Ignored.
 * @param regs Holds the snapshot of the processor's context before the
 *        processor entered the interrupt.  Ignored.
 */
static irqreturn_t
scc_irq(int irq, void *dev_id, struct pt_regs *regs)
{
    uint32_t smn_status;
    uint32_t scm_status;
    int handled = 0;            /* assume interrupt isn't from SMN */
#if defined(USE_SMN_INTERRUPT)
    int smn_irq = INT_SCC_SMN;      /* SMN interrupt is on a line by itself */
#elif defined (NO_SMN_INTERRUPT)
    int smn_irq = -1;           /* not wired to CPU at all */
#else
    int smn_irq = INT_SCC_SCM;      /* SMN interrupt shares a line with SCM */
#endif

    /* Update current state... This will perform callbacks... */
    smn_status = scc_update_state();

    /* SMN is on its own interrupt line.  Verify the IRQ was triggered
     * before clearing the interrupt and marking it handled.  */
    if (irq == smn_irq && smn_status & SMN_STATUS_SMN_STATUS_IRQ) {
        SCC_WRITE_REGISTER(SMN_COMMAND, SMN_COMMAND_CLEAR_INTERRUPT);
        handled++;              /* tell kernel that interrupt was handled */
    }

    /* Check on the health of the SCM */
    scm_status = SCC_READ_REGISTER(SCM_STATUS);

    /* The driver masks interrupts, so this should never happen. */
    if (irq == INT_SCC_SCM && scm_status & SCM_STATUS_INTERRUPT_STATUS) {
        /* but if it does, try to prevent it in the future */
        SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                           SCM_INTERRUPT_CTRL_CLEAR_INTERRUPT
                           | SCM_INTERRUPT_CTRL_MASK_INTERRUPTS);
        handled++;
    }

    /* Any non-zero value of handled lets kernel know we got something */
    return IRQ_RETVAL(handled);
}


/*****************************************************************************/
/* fn scc_perform_callbacks()                                                */
/*****************************************************************************/
/*! Perform callbacks registered by #scc_monitor_security_failure().
 *
 *  Make sure callbacks only happen once...  Since there may be some reason why
 *  the interrupt isn't generated, this routine could be called from base(task)
 *  level.
 *
 *  One at a time, go through #scc_callbacks[] and call any non-null pointers.
 */
static void
scc_perform_callbacks(void)
{
    static int callbacks_performed = 0;
    unsigned long irq_flags;    /* for IRQ save/restore */
    int i;

    /* Acquire lock of callbacks table and callbacks_performed flag */
    spin_lock_irqsave(&scc_callbacks_lock, irq_flags);

    if (!callbacks_performed) {
        callbacks_performed = 1;

        /* Loop over all of the entries in the table */
        for (i = 0; i < SCC_CALLBACK_SIZE; i++) {
            /* If not null, ... */
            if (scc_callbacks[i]) {
                scc_callbacks[i](); /* invoke the callback routine */
            }
        }
    }

    spin_unlock_irqrestore(&scc_callbacks_lock, irq_flags);

    return;
}


/*****************************************************************************/
/* fn copy_to_scc()                                                          */
/*****************************************************************************/
/*!
 *  Move data from possibly unaligned source and realign for SCC, possibly
 *  while calculating CRC.
 *
 *  Multiple calls can be made to this routine (without intervening calls to
 *  #copy_from_scc(), as long as the sum total of bytes copied is a multiple of
 *  four (SCC native word size).
 *
 *  @param[in]     from        Location in memory
 *  @param[out]    to          Location in SCC
 *  @param[in]     count_bytes Number of bytes to copy
 *  @param[in,out] crc         Pointer to CRC.  Initial value must be
 *                             #CRC_CCITT_START if this is the start of
 *                             message.  Output is the resulting (maybe
 *                             partial) CRC.  If NULL, no crc is calculated.
 *
 * @return  Zero - success.  Non-zero - SCM status bits defining failure.
 */
static uint32_t
copy_to_scc(const uint8_t* from, uint32_t to, unsigned long count_bytes,
            uint16_t* crc)
{
    int i;
    uint32_t scm_word;
    uint16_t current_crc = 0;       /* local copy for fast access */
    uint32_t status;

#ifdef SCC_DEBUG
    printk("SCC: copying %ld bytes to 0x%0x.\n", count_bytes, to);
#endif

    status = SCC_READ_REGISTER(SCM_ERROR_STATUS) & SCM_ACCESS_ERRORS;
    if (status != 0) {
#ifdef SCC_DEBUG
        printk("SCC copy_to_scc(): Error status detected (before copy):"
               " %08x\n", status);
#endif
        /* clear out errors left behind by somebody else */
        SCC_WRITE_REGISTER(SCM_ERROR_STATUS, status);
    }

    if (crc) {
        current_crc = *crc;
    }

    /* Initialize value being built for SCM.  If we are starting 'clean',
     * set it to zero.  Otherwise pick up partial value which had been saved
     * earlier. */
    if (SCC_BYTE_OFFSET(to) == 0) {
        scm_word = 0;
    }
    else {
        scm_word = SCC_READ_REGISTER(SCC_WORD_PTR(to)); /* recover */
    }

    /* Now build up SCM words and write them out when each is full */
    for (i = 0; i < count_bytes; i++) {
        uint8_t byte = *from++; /* value from plaintext */

#ifdef __BIG_ENDIAN
        scm_word = (scm_word<<8) | byte; /* add byte to SCM word */
#else
        scm_word = (byte<<24) | (scm_word>>8);
#endif
        /* now calculate CCITT CRC */
        if (crc) {
            CALC_CRC(byte, current_crc);
        }

        to++;                   /* bump location in SCM */

        /* check for full word */
        if (SCC_BYTE_OFFSET(to) == 0) {
            SCC_WRITE_REGISTER((uint32_t)(to-4), scm_word); /* write it out */
        }
    }

    /* If at partial word after previous loop, save it in SCM memory for
       next time. */
    if (SCC_BYTE_OFFSET(to) != 0) {
        SCC_WRITE_REGISTER(SCC_WORD_PTR(to), scm_word); /* save */
    }

    /* Copy CRC back */
    if (crc) {
        *crc = current_crc;
    }

    status = SCC_READ_REGISTER(SCM_ERROR_STATUS) & SCM_ACCESS_ERRORS;
    if (status != 0) {
#ifdef SCC_DEBUG
        printk("SCC copy_to_scc(): Error status detected: %08x\n", status);
#endif
        /* Clear any/all bits. */
        SCC_WRITE_REGISTER(SCM_ERROR_STATUS, status);
    }

    return status;
}


/*****************************************************************************/
/* fn copy_from_scc()                                                        */
/*****************************************************************************/
/*!
 *  Move data from aligned 32-bit source and place in (possibly unaligned)
 *  target, and maybe calculate CRC at the same time.
 *
 *  Multiple calls can be made to this routine (without intervening calls to
 *  #copy_to_scc(), as long as the sum total of bytes copied is be a multiple
 *  of four.
 *
 *  @param[in]     from        Location in SCC
 *  @param[out]    to          Location in memory
 *  @param[in]     count_bytes Number of bytes to copy
 *  @param[in,out] crc         Pointer to CRC.  Initial value must be
 *                             #CRC_CCITT_START if this is the start of
 *                             message.  Output is the resulting (maybe
 *                             partial) CRC.  If NULL, crc is not calculated.
 *
 * @return  Zero - success.  Non-zero - SCM status bits defining failure.
 */
static uint32_t
copy_from_scc(const uint32_t from, uint8_t* to, unsigned long count_bytes,
              uint16_t* crc)
{
    uint32_t running_from = from;
    uint32_t scm_word;
    uint16_t current_crc = 0; /* local copy for fast access */
    uint32_t status;

#ifdef SCC_DEBUG
    printk("SCC: copying %ld bytes from 0x%x.\n", count_bytes, from);
#endif

    status = SCC_READ_REGISTER(SCM_ERROR_STATUS) & SCM_ACCESS_ERRORS;
    if (status != 0) {
#ifdef SCC_DEBUG
        printk("SCC copy_from_scc(): Error status detected (before copy):"
               " %08x\n", status);
#endif
        /* clear out errors left behind by somebody else */
        SCC_WRITE_REGISTER(SCM_ERROR_STATUS, status);
    }

    if (crc) {
        current_crc = *crc;
    }

    /* Read word which is sitting in SCM memory.  Ignore byte offset */
    scm_word = SCC_READ_REGISTER(SCC_WORD_PTR(running_from));

    /* If necessary, move the 'first' byte into place */
    if (SCC_BYTE_OFFSET(running_from) != 0) {
#ifdef __BIG_ENDIAN
        scm_word <<= 8*SCC_BYTE_OFFSET(running_from);
#else
        scm_word >>= 8*SCC_BYTE_OFFSET(running_from);
#endif
    }

    /* Now build up SCM words and write them out when each is full */
    while (count_bytes--) {
        uint8_t byte;           /* value from plaintext */

#ifdef __BIG_ENDIAN
        byte = (scm_word & 0xff000000)>>24; /* pull byte out of SCM word */
        scm_word <<= 8;         /* shift over to remove the just-pulled byte */
#else
        byte = (scm_word & 0xff);
        scm_word >>= 8;         /* shift over to remove the just-pulled byte */
#endif
        *to++ = byte;           /* send byte to memory */

        /* now calculate CRC */
        if (crc) {
            CALC_CRC(byte, current_crc);
        }

        running_from++;
        /* check for empty word */
        if (count_bytes && SCC_BYTE_OFFSET(running_from) == 0) {
            /* read one in */
            scm_word = SCC_READ_REGISTER((uint32_t)running_from);
        }
    }

    if (crc) {
        *crc = current_crc;
    }

    status = SCC_READ_REGISTER(SCM_ERROR_STATUS) & SCM_ACCESS_ERRORS;
    if (status != 0) {
#ifdef SCC_DEBUG
        printk("SCC copy_from_scc(): Error status detected: %08x\n", status);
#endif
        /* Clear any/all bits. */
        SCC_WRITE_REGISTER(SCM_ERROR_STATUS, status);
    }

    return status;
}


/*****************************************************************************/
/* fn scc_strip_padding()                                                    */
/*****************************************************************************/
/*!
 *  Remove padding from plaintext.  Search backwards for #SCC_DRIVER_PAD_CHAR,
 *  verifying that each byte passed over is zero (0).  Maximum number of bytes
 *  to examine is 8.
 *
 *  @param[in]     from           Pointer to byte after end of message
 *  @param[out]    count_bytes_stripped Number of padding bytes removed by this
 *                                      function.
 *
 *  @return   #SCC_RET_OK if all goes, well, #SCC_RET_FAIL if padding was
 *            not present.
*/
static scc_return_t
scc_strip_padding(uint8_t* from, unsigned* count_bytes_stripped)
{
    int i = SCC_BLOCK_SIZE_BYTES();
    scc_return_t return_code = SCC_RET_VERIFICATION_FAILED;

    /*
     * Search backwards looking for the magic marker.  If it isn't found,
     * make sure that a 0 byte is there in its place.  Stop after the maximum
     * amount of padding (8 bytes) has been searched);
    */
    while (i-- > 0) {
        if (*--from == SCC_DRIVER_PAD_CHAR) {
            *count_bytes_stripped = SCC_BLOCK_SIZE_BYTES()-i;
            return_code = SCC_RET_OK;
            break;
        }
        else if (*from != 0) {  /* if not marker, check for 0 */
#ifdef SCC_DEBUG
            printk("SCC: Found non-zero interim pad: 0x%x\n",
                   *from);
#endif
            break;
        }
    }

    return return_code;
}


/*****************************************************************************/
/* fn scc_update_state()                                                     */
/*****************************************************************************/
/*!
 * Make certain SCC is still running.
 *
 * Side effect is to update #scc_availability and, if the state goes to failed,
 * run #scc_perform_callbacks().
 *
 * (If #SCC_BRINGUP is defined, bring SCC to secure state if it is found to be
 * in health check state)
 *
 * @return Current value of #SMN_STATUS register.
 */
static uint32_t
scc_update_state(void)
{
    uint32_t smn_status_register = SMN_STATE_FAIL;
    int smn_state;

    /* if FAIL or UNIMPLEMENTED, don't bother */
    if (scc_availability == SCC_STATUS_CHECKING ||
        scc_availability == SCC_STATUS_OK) {

        smn_status_register = SCC_READ_REGISTER(SMN_STATUS);
        smn_state = smn_status_register & SMN_STATUS_STATE_MASK;

#ifdef SCC_BRINGUP
        /* If in Health Check while booting, try to 'bringup' to Secure mode */
        if (scc_availability == SCC_STATUS_CHECKING &&
            smn_state == SMN_STATE_HEALTH_CHECK) {
            /* Code up a simple algorithm for the ASC */
            SCC_WRITE_REGISTER(SMN_SEQUENCE_START, 0xaaaa);
            SCC_WRITE_REGISTER(SMN_SEQUENCE_END, 0x5555);
            SCC_WRITE_REGISTER(SMN_SEQUENCE_CHECK, 0x5555);
            /* State should be SECURE now */
            smn_status_register = SCC_READ_REGISTER(SMN_STATUS);
            smn_state = smn_status_register & SMN_STATUS_STATE_MASK;
        }
#endif

        /*
         * State should be SECURE or NON_SECURE for operation of the part.  If
         * FAIL, mark failed (i.e. limited access to registers).  Any other
         * state, mark unimplemented, as the SCC is unuseable.
         */
        if (smn_state == SMN_STATE_SECURE
            || smn_state == SMN_STATE_NON_SECURE) {
            /* Healthy */
            scc_availability = SCC_STATUS_OK;
        }
        else if (smn_state == SMN_STATE_FAIL) {
            scc_availability = SCC_STATUS_FAILED; /* uh oh - unhealthy */
            scc_perform_callbacks();
#ifdef SCC_DEBUG
            printk("SCC: SCC went into FAILED mode\n");
#endif
        }
        else {
            /* START, ZEROIZE RAM, HEALTH CHECK, or unknown */
            scc_availability = SCC_STATUS_UNIMPLEMENTED; /* unuseable */
#ifdef SCC_DEBUG
            printk("SCC: SCC declared UNIMPLEMENTED\n");
#endif
        }
    } /* if availability is initial or ok */

    return smn_status_register;
}


/*****************************************************************************/
/* fn scc_init_ccitt_crc()                                                   */
/*****************************************************************************/
/*!
 * Populate the partial CRC lookup table.
 *
 * @return   none
 *
 */
void
scc_init_ccitt_crc(void)
{
    int          dividend;      /* index for lookup table */
    uint16_t     remainder;     /* partial value for a given dividend */
    int          bit;           /* index into bits of a byte */


    /*
     * Compute the remainder of each possible dividend.
     */
    for (dividend = 0; dividend < 256; ++dividend) {
        /*
         * Start with the dividend followed by zeros.
         */
        remainder = dividend << (8);

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (bit = 8; bit > 0; --bit) {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & 0x8000) {
                remainder = (remainder << 1) ^ CRC_POLYNOMIAL;
            }
            else {
                remainder = (remainder << 1);
            }
        }

        /*
         * Store the result into the table.
         */
        scc_crc_lookup_table[dividend] = remainder;
    }

}   /* scc_init_ccitt_crc() */


/*****************************************************************************/
/* fn grab_config_values()                                                   */
/*****************************************************************************/
/*!
 * grab_config_values() will read the SCM Configuration and SMN Status
 * registers and store away version and size information for later use.
 *
 * @return The current value of the SMN Status register.
 */
static uint32_t
scc_grab_config_values(void) {
    uint32_t config_register;
    uint32_t smn_status_register = SMN_STATE_FAIL;

    if (scc_availability != SCC_STATUS_UNIMPLEMENTED) {
        /* access the SCC - these are 'safe' registers */
        config_register = SCC_READ_REGISTER(SCM_CONFIGURATION);
#ifdef SCC_DEBUG
        printk("SCC Driver: SCM config is 0x%08x\n", config_register);
#endif

        /* Get SMN status and update scc_availability */
        smn_status_register = scc_update_state();
#ifdef SCC_DEBUG
        printk("SCC Driver: SMN status is 0x%08x\n", smn_status_register);
#endif

        /* save sizes and versions information for later use */
        scc_configuration.block_size_bytes = (config_register &
                                              SCM_CFG_BLOCK_SIZE_MASK)
                                                  >> SCM_CFG_BLOCK_SIZE_SHIFT;

        scc_configuration.red_ram_size_blocks = (config_register &
                                                 SCM_CFG_RED_SIZE_MASK)
                                                     >> SCM_CFG_RED_SIZE_SHIFT;
#ifdef CONFIG_VIRTIO_SUPPORT
        scc_configuration.red_ram_size_blocks = 128;
#endif

        scc_configuration.black_ram_size_blocks = (config_register &
                                                   SCM_CFG_BLACK_SIZE_MASK)
                                                   >> SCM_CFG_BLACK_SIZE_SHIFT;
#ifdef CONFIG_VIRTIO_SUPPORT
        scc_configuration.black_ram_size_blocks = 128;
#endif

        scc_configuration.scm_version = (config_register
                                         & SCM_CFG_VERSION_ID_MASK)
                                             >> SCM_CFG_VERSION_ID_SHIFT;

        scc_configuration.smn_version = (smn_status_register &
                                         SMN_STATUS_VERSION_ID_MASK)
                                             >> SMN_STATUS_VERSION_ID_SHIFT;

        if (scc_configuration.scm_version != SCM_VERSION_1) {
            scc_availability = SCC_STATUS_UNIMPLEMENTED; /* Unknown version */
        }

        scc_memory_size_bytes = (SCC_BLOCK_SIZE_BYTES() *
                                 scc_configuration.black_ram_size_blocks)
                                - SCM_NON_RESERVED_OFFSET;

        /* This last is for driver consumption only */
        scm_highest_memory_address = SCM_BLACK_MEMORY +
                                     (SCC_BLOCK_SIZE_BYTES() *
                                      scc_configuration.black_ram_size_blocks);
    }

    return smn_status_register;
} /* grab_config_values */


/*****************************************************************************/
/* fn setup_interrupt_handling()                                             */
/*****************************************************************************/
/*!
 * Register the SCM and SMN interrupt handlers.
 *
 * Called from #scc_init()
 *
 * @return 0 on success
 */
static int
setup_interrupt_handling(void) {
    int smn_error_code = -1;
    int scm_error_code = -1;

    /* Disnable SCM interrupts */
    SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                       SCM_INTERRUPT_CTRL_CLEAR_INTERRUPT
                       | SCM_INTERRUPT_CTRL_MASK_INTERRUPTS);

#ifdef USE_SMN_INTERRUPT
    /* Install interrupt service routine for SMN. */
    smn_error_code = request_irq(INT_SCC_SMN, scc_irq, 0,
                                 SCC_DRIVER_NAME, NULL);
    if (smn_error_code != 0) {
        printk("SCC Driver: Error installing SMN Interrupt Handler: %d\n",
               smn_error_code);
    }
    else {
        smn_irq_set = 1;              /* remember this for cleanup */
        /* Enable SMN interrupts */
        SCC_WRITE_REGISTER(SMN_COMMAND,
                           SMN_COMMAND_CLEAR_INTERRUPT |
                           SMN_COMMAND_ENABLE_INTERRUPT);
    }
#else
    smn_error_code = 0;         /* no problems... will handle later */
#endif

    /*
     * Install interrupt service routine for SCM (or both together).
     */
    scm_error_code = request_irq(INT_SCC_SCM, scc_irq, 0,
                                 SCC_DRIVER_NAME, NULL);
    if (scm_error_code != 0) {
#ifndef MXC
        printk("SCC Driver: Error installing SCM Interrupt Handler: %d\n",
               scm_error_code);
#else
        printk("SCC Driver: Error installing SCC Interrupt Handler: %d\n",
               scm_error_code);
#endif
    }
    else {
        scm_irq_set = 1;          /* remember this for cleanup */
#if defined(USE_SMN_INTERRUPT) && !defined(NO_SMN_INTERRUPT)
        /* Enable SMN interrupts */
        SCC_WRITE_REGISTER(SMN_COMMAND,
                           SMN_COMMAND_CLEAR_INTERRUPT |
                           SMN_COMMAND_ENABLE_INTERRUPT);
#endif
    }

    /* Return an error if one was encountered */
    return scm_error_code ? scm_error_code : smn_error_code;
} /* setup_interrupt_handling */


/*****************************************************************************/
/* fn scc_do_crypto()                                                        */
/*****************************************************************************/
/*! Have the SCM perform the crypto function.
 *
 * Set up length register, and the store @c scm_control into control register
 * to kick off the operation.  Wait for completion, gather status, clear
 * interrupt / status.
 *
 * @param byte_count  number of bytes to perform in this operation
 * @param scm_control Bit values to be set in @c SCM_CONTROL register
 *
 * @return 0 on success, value of #SCM_ERROR_STATUS on failure
 */
static uint32_t
scc_do_crypto (int byte_count, uint32_t scm_control)
{
    int block_count = byte_count / SCC_BLOCK_SIZE_BYTES();
    uint32_t crypto_status;


    /* clear any outstanding flags */
    SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                       SCM_INTERRUPT_CTRL_CLEAR_INTERRUPT
                       | SCM_INTERRUPT_CTRL_MASK_INTERRUPTS);

    /* In length register, 0 means 1, etc. */
    SCC_WRITE_REGISTER(SCM_LENGTH, block_count-1);

    /* set modes and kick off the operation */
    SCC_WRITE_REGISTER(SCM_CONTROL, scm_control);

    scc_wait_completion();

    /* Mask for done and error bits */
    crypto_status = SCC_READ_REGISTER(SCM_STATUS)
        & (SCM_STATUS_CIPHERING_DONE
           | SCM_STATUS_LENGTH_ERROR
           | SCM_STATUS_INTERNAL_ERROR);

    /* Only done bit should be on */
    if (crypto_status != SCM_STATUS_CIPHERING_DONE) {
        /* Replace with error status instead */
        crypto_status = SCC_READ_REGISTER(SCM_ERROR_STATUS);
#ifdef SCC_DEBUG
        printk("SCM Failure: 0x%x\n", crypto_status);
#endif
        if (crypto_status == 0) {
            /* That came up 0.  Turn on arbitrary bit to signal error. */
            crypto_status = SCM_ERR_INTERNAL_ERROR;
        }
    }
    else {
        crypto_status = 0;
    }

#ifdef SCC_DEBUG
    printk("SCC: Done waiting.\n");
#endif

    /* Clear out any status.*/
    SCC_WRITE_REGISTER(SCM_INTERRUPT_CTRL,
                       SCM_INTERRUPT_CTRL_CLEAR_INTERRUPT
                       | SCM_INTERRUPT_CTRL_MASK_INTERRUPTS);

    /* And clear any error status */
    SCC_WRITE_REGISTER(SCM_ERROR_STATUS, 0);

    return crypto_status;
}


/*****************************************************************************/
/* fn scc_encrypt()                                                          */
/*****************************************************************************/
/*!
 * Perform an encryption on the input.  If @c verify_crc is true, a CRC must be
 * calculated on the plaintext, and appended, with padding, before computing
 * the ciphertext.
 *
 * @param[in]     count_in_bytes  Count of bytes of plaintext
 * @param[in]     data_in         Pointer to the plaintext
 * @param[in]     scm_control     Bit values for the SCM_CONTROL register
 * @param[in,out] data_out        Pointer for storing ciphertext
 * @param[in]     add_crc         Flag for computing CRC - 0 no, else yes
 * @param[in,out] count_out_bytes Number of bytes available at @c data_out
 */
static scc_return_t
scc_encrypt(uint32_t count_in_bytes, uint8_t* data_in, uint32_t scm_control,
            uint8_t* data_out, int add_crc, unsigned long* count_out_bytes)
{
    scc_return_t return_code = SCC_RET_FAIL; /* initialised for failure */
    uint32_t input_bytes_left = count_in_bytes; /* local copy */
    uint32_t output_bytes_copied = 0;   /* running total */
    uint32_t bytes_to_process;          /* multi-purpose byte counter */
    uint16_t crc = CRC_CCITT_START;     /* running CRC value */
    crc_t* crc_ptr = NULL;              /* Reset if CRC required */
    uint32_t scm_location = SCM_RED_MEMORY + SCM_NON_RESERVED_OFFSET; /* byte address  into SCM RAM */
    uint32_t scm_bytes_remaining = scc_memory_size_bytes; /* free RED RAM */
    uint8_t  padding_buffer[PADDING_BUFFER_MAX_BYTES]; /* CRC+padding holder */
    unsigned padding_byte_count = 0;    /* Reset if padding required */
    uint32_t scm_error_status = 0;      /* No known SCM error initially */

    /* Set location of CRC and prepare padding bytes if required */
    if (add_crc != 0) {
        crc_ptr = &crc;
        padding_byte_count = SCC_BLOCK_SIZE_BYTES()
            - (count_in_bytes + CRC_SIZE_BYTES) % SCC_BLOCK_SIZE_BYTES();
        memcpy(padding_buffer+CRC_SIZE_BYTES, scc_block_padding,
               padding_byte_count);
    }

    /* Process remaining input or padding data */
    while (input_bytes_left > 0) {

        /* Determine how much work to do this pass */
        bytes_to_process = (input_bytes_left > scm_bytes_remaining) ?
            scm_bytes_remaining : input_bytes_left;

        /* Copy plaintext into SCM RAM, calculating CRC if required */
        copy_to_scc(data_in, scm_location, bytes_to_process, crc_ptr);

        /* Adjust pointers & counters */
        input_bytes_left -= bytes_to_process;
        data_in += bytes_to_process;
        scm_location += bytes_to_process;
        scm_bytes_remaining -= bytes_to_process;

        /* Add CRC and padding after the last byte is copied if required */
        if ((input_bytes_left == 0) && (crc_ptr != NULL)) {

            /* Copy CRC into padding buffer MSB first */
            padding_buffer[0] = (crc >> 8) & 0xFF;
            padding_buffer[1] = crc & 0xFF;

            /* Reset pointers and counter */
            data_in = padding_buffer;
            input_bytes_left = CRC_SIZE_BYTES + padding_byte_count;
            crc_ptr = NULL;             /* CRC no longer required */

            /* Go round loop again to copy CRC and padding to SCM */
            continue;
        } /* if no input and crc_ptr */

        /* Now have block-sized plaintext in SCM to encrypt */

        /* Encrypt plaintext; exit loop on error */
        bytes_to_process = scm_location -
                           (SCM_RED_MEMORY + SCM_NON_RESERVED_OFFSET);

        if (output_bytes_copied + bytes_to_process > *count_out_bytes) {
            return_code = SCC_RET_INSUFFICIENT_SPACE;
            scm_error_status = -1; /* error signal */
#ifdef SCC_DEBUG
            printk("SCC: too many ciphertext bytes for space available\n");
#endif
            break;
        }

#ifdef SCC_DEBUG
        printk("SCC: Starting encryption. %x for %d bytes (%p/%p)\n",
               scm_control, bytes_to_process,
               (void*)SCC_READ_REGISTER(SCM_RED_START),
               (void*)SCC_READ_REGISTER(SCM_BLACK_START));
#endif
        scm_error_status = scc_do_crypto(bytes_to_process, scm_control);
        if (scm_error_status != 0) {
            break;
        }

        /* Copy out ciphertext */
        copy_from_scc(SCM_BLACK_MEMORY + SCM_NON_RESERVED_OFFSET, data_out,
                      bytes_to_process, NULL);

        /* Adjust pointers and counters for next loop */
        output_bytes_copied += bytes_to_process;
        data_out += bytes_to_process;
        scm_location = SCM_RED_MEMORY + SCM_NON_RESERVED_OFFSET;
        scm_bytes_remaining = scc_memory_size_bytes;


    } /* input_bytes_left > 0 */

    /* If no SCM error, set OK status and save ouput byte count */
    if (scm_error_status == 0) {
        return_code = SCC_RET_OK;
        *count_out_bytes = output_bytes_copied;
    }

    return return_code;
} /* scc_encrypt */


/*****************************************************************************/
/* fn scc_decrypt()                                                          */
/*****************************************************************************/
/*!
 * Perform a decryption on the input.  If @c verify_crc is true, the last block
 * (maybe the two last blocks) is special - it should contain a CRC and
 * padding.  These must be stripped and verified.
 *
 * @param[in]     count_in_bytes  Count of bytes of ciphertext
 * @param[in]     data_in         Pointer to the ciphertext
 * @param[in]     scm_control     Bit values for the SCM_CONTROL register
 * @param[in,out] data_out        Pointer for storing plaintext
 * @param[in]     verify_crc      Flag for running CRC - 0 no, else yes
 * @param[in,out] count_out_bytes Number of bytes available at @c data_out

 */
static scc_return_t
scc_decrypt(uint32_t count_in_bytes, uint8_t* data_in, uint32_t scm_control,
            uint8_t* data_out, int verify_crc,
            unsigned long* count_out_bytes)
{
    scc_return_t return_code = SCC_RET_FAIL;
    uint32_t bytes_left = count_in_bytes; /* local copy */
    uint32_t bytes_copied = 0;  /* running total of bytes going to user */
    uint32_t bytes_to_copy = 0; /* Number in this encryption 'chunk' */
    uint16_t crc = CRC_CCITT_START; /* running CRC value */
    uint32_t scm_location = SCM_BLACK_MEMORY + SCM_NON_RESERVED_OFFSET; /* next target for  ctext */
    unsigned padding_byte_count; /* number of bytes of padding stripped */
    uint8_t last_two_blocks[2*SCC_BLOCK_SIZE_BYTES()]; /* temp */
    uint32_t scm_error_status = 0;  /* register value */


    scm_control |= SCM_DECRYPT_MODE;

    if (verify_crc) {
        /* Save last two blocks (if there are at least two) of ciphertext for
           special treatment. */
        bytes_left -= SCC_BLOCK_SIZE_BYTES();
        if (bytes_left >= SCC_BLOCK_SIZE_BYTES()) {
            bytes_left -= SCC_BLOCK_SIZE_BYTES();
        }
    }

    /* Copy ciphertext into SCM BLACK memory */
    while (bytes_left && scm_error_status == 0) {

        /* Determine how much work to do this pass */
        if  (bytes_left > (scc_memory_size_bytes)) {
            bytes_to_copy = scc_memory_size_bytes;
        }
        else {
            bytes_to_copy = bytes_left;
        }

        if (bytes_copied + bytes_to_copy > *count_out_bytes) {
            scm_error_status = -1;
            break;
        }
        copy_to_scc(data_in, scm_location, bytes_to_copy, NULL);
        data_in += bytes_to_copy; /* move pointer */

#ifdef SCC_DEBUG
        printk("SCC: Starting decryption of %d bytes.\n", bytes_to_copy);
#endif

        /*  Do the work, wait for completion */
        scm_error_status = scc_do_crypto(bytes_to_copy, scm_control);

        copy_from_scc(SCM_RED_MEMORY + SCM_NON_RESERVED_OFFSET, data_out,
                      bytes_to_copy, &crc);
        bytes_copied += bytes_to_copy;
        data_out += bytes_to_copy;
        scm_location = SCM_BLACK_MEMORY + SCM_NON_RESERVED_OFFSET;

        /* Do housekeeping */
        bytes_left -= bytes_to_copy;

    } /* while bytes_left */

    /* At this point, either the process is finished, or this is verify mode */

    if (scm_error_status == 0) {
        if (!verify_crc) {
            *count_out_bytes = bytes_copied;
            return_code = SCC_RET_OK;
        }
        else {
            /* Verify mode.  There are one or two blocks of unprocessed
             * ciphertext sitting at data_in.  They need to be moved to the
             * SCM, decrypted, searched to remove padding, then the plaintext
             * copied back to the user (while calculating CRC, of course).
             */

            /* Calculate ciphertext still left */
            bytes_to_copy = count_in_bytes - bytes_copied;

            copy_to_scc(data_in, scm_location, bytes_to_copy, NULL);
            data_in += bytes_to_copy; /* move pointer */

#ifdef SCC_DEBUG
            printk("SCC: Finishing decryption (%d bytes).\n", bytes_to_copy);
#endif

            /*  Do the work, wait for completion */
            scm_error_status = scc_do_crypto(bytes_to_copy, scm_control);


            if (scm_error_status == 0) {
                /* Copy decrypted data back from SCM RED memory */
                copy_from_scc(SCM_RED_MEMORY + SCM_NON_RESERVED_OFFSET,
                              last_two_blocks, bytes_to_copy, NULL);

                /* (Plaintext) + crc + padding should be in temp buffer */
                if (scc_strip_padding(last_two_blocks + bytes_to_copy,
                                      &padding_byte_count) == SCC_RET_OK) {
                    bytes_to_copy -= padding_byte_count + CRC_SIZE_BYTES;

                    /* verify enough space in user buffer */
                    if (bytes_copied + bytes_to_copy <= *count_out_bytes) {
                        int i = 0;

                        /* Move out last plaintext and calc CRC */
                        while (i < bytes_to_copy) {
                            CALC_CRC(last_two_blocks[i], crc);
                            *data_out++ = last_two_blocks[i++];
                            bytes_copied++;
                        }

                        /* Verify the CRC by running over itself */
                        CALC_CRC(last_two_blocks[bytes_to_copy], crc);
                        CALC_CRC(last_two_blocks[bytes_to_copy+1], crc);
                        if (crc == 0) {
                            /* Just fine ! */
                            *count_out_bytes = bytes_copied;
                            return_code = SCC_RET_OK;
                        }
                        else {
                            return_code = SCC_RET_VERIFICATION_FAILED;
#ifdef SCC_DEBUG
                            printk("SCC:  CRC values are %04x, %02x%02x\n",
                                   crc,
                                   last_two_blocks[bytes_to_copy],
                                   last_two_blocks[bytes_to_copy+1]);
#endif
                        }
                    } /* if space available */
                } /* if scc_strip_padding... */
                else {
                    /* bad padding means bad verification */
                    return_code = SCC_RET_VERIFICATION_FAILED;
                }
            } /* scm_error_status == 0 */

        } /* verify_crc */
    } /* scm_error_status == 0 */


    return return_code;
} /* scc_decrypt */


/*****************************************************************************/
/* fn verify_slot_access()                                                   */
/*****************************************************************************/
inline static scc_return_t
verify_slot_access(uint64_t owner_id, uint32_t slot, uint32_t access_len)
{
    register scc_return_t status;

    if ((slot < SCC_KEY_SLOTS) && scc_key_info[slot].status
        && (scc_key_info[slot].owner_id == owner_id)
		&& (access_len <= SCC_KEY_SLOT_SIZE)) {
        status = SCC_RET_OK;
#ifdef SCC_DEBUG
        printk("SCC: Verify on slot %d succeeded\n", slot);
#endif
    } else {
#ifdef SCC_DEBUG
        if (slot >= SCC_KEY_SLOTS) {
            printk("SCC: Verify on bad slot (%d) failed\n", slot);
        } else if (scc_key_info[slot].status) {
            printk("SCC: Verify on slot %d failed (%Lx) \n", slot, owner_id);
        } else {
            printk("SCC: Verify on slot %d failed: not allocated\n", slot);
        }
#endif
        status = SCC_RET_FAIL;
    }

    return status;
}


/*****************************************************************************/
/* fn scc_alloc_slot()                                                       */
/*****************************************************************************/
/*!
 * Allocate a key slot to fit the requested size.
 *
 * @param value_size_bytes   Size of the key or other secure data
 * @param owner_id           Value to tie owner to slot
 * @param[out] slot          Handle to access or deallocate slot
 *
 * @return SCC_RET_OK on success, SCC_RET_INSUFFICIENT_SPACE if not slots of
 *         requested size are available.
 */
scc_return_t
scc_alloc_slot(uint32_t value_size_bytes, uint64_t owner_id, uint32_t *slot)
{
    scc_return_t status = SCC_RET_FAIL;
    unsigned long irq_flags;


    /* ACQUIRE LOCK to prevent others from using SCC crypto */
    spin_lock_irqsave(&scc_crypto_lock, irq_flags);

#ifdef SCC_DEBUG
    printk("SCC: Allocating %d-byte slot for 0x%Lx\n", value_size_bytes,
           owner_id);
#endif

    if ((value_size_bytes != 0) && (value_size_bytes <= SCC_MAX_KEY_SIZE)) {
        int i;

        for (i=0; i < SCC_KEY_SLOTS; i++) {
            if (scc_key_info[i].status == 0) {
                scc_key_info[i].owner_id = owner_id;
                scc_key_info[i].length = value_size_bytes;
                scc_key_info[i].status = 1; /* assigned! */
                *slot = i;
                status = SCC_RET_OK;
                break;          /* exit 'for' loop */
            }
        }

        if (status != SCC_RET_OK) {
            status = SCC_RET_INSUFFICIENT_SPACE;
        } else {
#ifdef SCC_DEBUG
            printk("SCC: Allocated slot %d (0x%Lx)\n", i, owner_id);
#endif
        }
    }

    spin_unlock_irqrestore(&scc_crypto_lock, irq_flags);

    return status;
}


/*****************************************************************************/
/* fn scc_dealloc_slot()                                                     */
/*****************************************************************************/
scc_return_t
scc_dealloc_slot(uint64_t owner_id, uint32_t slot)
{
    scc_return_t status;
    unsigned long irq_flags;
	int i;

    /* ACQUIRE LOCK to prevent others from using SCC crypto */
    spin_lock_irqsave(&scc_crypto_lock, irq_flags);

    status = verify_slot_access(owner_id, slot, 0);

    if (status == SCC_RET_OK) {
        scc_key_info[slot].owner_id = 0;
        scc_key_info[slot].status = 0; /* unassign */

		/* clear old info */
		for (i = 0; i < SCC_KEY_SLOT_SIZE; i += 4) {
			SCC_WRITE_REGISTER(SCM_RED_MEMORY +
					   scc_key_info[slot].offset + i, 0);
			SCC_WRITE_REGISTER(SCM_BLACK_MEMORY +
					   scc_key_info[slot].offset + i, 0);
		}
#ifdef SCC_DEBUG
        printk("SCC: Dellocated slot %d\n", slot);
#endif
    }

    spin_unlock_irqrestore(&scc_crypto_lock, irq_flags);

    return status;
}


/*****************************************************************************/
/* fn scc_load_slot()                                                        */
/*****************************************************************************/
/*!
 * Load a value into a slot.
 *
 * @param owner_id      Value of owner of slot
 * @param slot          Handle of slot
 * @param key_data      Data to load into the slot
 * @param key_length    Length, in bytes, of @c key_data to copy to SCC.
 *
 * @return SCC_RET_OK on success.  SCC_RET_FAIL will be returned if slot
 * specified cannot be accessed for any reason, or SCC_RET_INSUFFICIENT_SPACE
 * if @c key_length exceeds the size of the slot.
 */
scc_return_t
scc_load_slot(uint64_t owner_id, uint32_t slot, uint8_t *key_data,
              uint32_t key_length)
{
    scc_return_t status;
    unsigned long irq_flags;

    /* ACQUIRE LOCK to prevent others from using SCC crypto */
    spin_lock_irqsave(&scc_crypto_lock, irq_flags);

    status = verify_slot_access(owner_id, slot, key_length);
    if ((status == SCC_RET_OK) && (key_data != NULL)) {
        status = SCC_RET_FAIL;  /* reset expectations */

        if (key_length > SCC_KEY_SLOT_SIZE) {
#ifdef SCC_DEBUG
            printk("SCC: scc_load_slot() rejecting key of %d bytes.\n",
                   key_length);
#endif
            status = SCC_RET_INSUFFICIENT_SPACE;
        } else {
            if (copy_to_scc(key_data,
                            SCM_RED_MEMORY + scc_key_info[slot].offset,
                            key_length, NULL)) {
#ifdef SCC_DEBUG
                printk("SCC: RED copy_to_scc() failed for scc_load_slot()\n");
#endif
            } else {
                status = SCC_RET_OK;
            }
        }
    }

    spin_unlock_irqrestore(&scc_crypto_lock, irq_flags);

    return status;
} /* scc_load_slot */


/*****************************************************************************/
/* fn scc_encrypt_slot()                                                     */
/*****************************************************************************/
/*!
 * Allocate a key slot to fit the requested size.
 *
 * @param owner_id      Value of owner of slot
 * @param slot          Handle of slot
 * @param length        Length, in bytes, of @c black_data
 * @param black_data    Location to store result of encrypting RED data in slot
 *
 * @return SCC_RET_OK on success, SCC_RET_FAIL if slot specified cannot be
 *         accessed for any reason.
 */
scc_return_t scc_encrypt_slot(uint64_t owner_id, uint32_t slot,
                                     uint32_t length,
                                     uint8_t* black_data)
{
    unsigned long irq_flags;
    scc_return_t status;
    uint32_t crypto_status;
    uint32_t slot_offset = scc_key_info[slot].offset/SCC_BLOCK_SIZE_BYTES();

    /* ACQUIRE LOCK to prevent others from using crypto or releasing slot */
    spin_lock_irqsave(&scc_crypto_lock, irq_flags);

    status = verify_slot_access(owner_id, slot, length);
    if (status == SCC_RET_OK) {
        SCC_WRITE_REGISTER(SCM_BLACK_START, slot_offset);
        SCC_WRITE_REGISTER(SCM_RED_START, slot_offset);

        /* Use OwnerID as CBC IV to tie Owner to data */
        SCC_WRITE_REGISTER(SCM_INIT_VECTOR_0, *(uint32_t*)&owner_id);
        SCC_WRITE_REGISTER(SCM_INIT_VECTOR_1, *(((uint32_t*)&owner_id)+1));

        /* Set modes and kick off the encryption */
        crypto_status = scc_do_crypto(length,
                                      SCM_CONTROL_START_CIPHER | SCM_CBC_MODE);

        if (crypto_status != 0) {
#ifdef SCC_DEBUG
                printk("SCM encrypt red crypto failure: 0x%x\n",
                       crypto_status);
#endif
        } else {

            /* Give blob back to caller */
            if (! copy_from_scc(SCM_BLACK_MEMORY + scc_key_info[slot].offset,
                                black_data,
                                length, NULL)) {
                status = SCC_RET_OK;
#ifdef SCC_DEBUG
                printk("SCC: Encrypted slot %d\n", slot);
#endif
            }
        }
    }

    spin_unlock_irqrestore(&scc_crypto_lock, irq_flags);

    return status;
}


/*****************************************************************************/
/* fn scc_decrypt_slot()                                                     */
/*****************************************************************************/
/*!
 * Decrypt some black data and leave result in the slot.
 *
 * @param owner_id      Value of owner of slot
 * @param slot          Handle of slot
 * @param length        Length, in bytes, of @c black_data
 * @param black_data    Location of data to dencrypt and store in slot
 *
 * @return SCC_RET_OK on success, SCC_RET_FAIL if slot specified cannot be
 *         accessed for any reason.
 */
scc_return_t scc_decrypt_slot(uint64_t owner_id, uint32_t slot,
                                     uint32_t length,
                                     const uint8_t* black_data)
{
    unsigned long irq_flags;
    scc_return_t status;
    uint32_t crypto_status;
    uint32_t slot_offset = scc_key_info[slot].offset/SCC_BLOCK_SIZE_BYTES();

    /* ACQUIRE LOCK to prevent others from using crypto or releasing slot */
    spin_lock_irqsave(&scc_crypto_lock, irq_flags);

    status = verify_slot_access(owner_id, slot, length);
    if (status == SCC_RET_OK) {
        status = SCC_RET_FAIL;  /* reset expectations */

        /* Place black key in to BLACK RAM and set up the SCC */
        copy_to_scc(black_data, SCM_BLACK_MEMORY + scc_key_info[slot].offset,
                    length, NULL);

        SCC_WRITE_REGISTER(SCM_BLACK_START, slot_offset);
        SCC_WRITE_REGISTER(SCM_RED_START, slot_offset);

        /* Use OwnerID as CBC IV to tie Owner to data */
        SCC_WRITE_REGISTER(SCM_INIT_VECTOR_0, *(uint32_t*)&owner_id);
        SCC_WRITE_REGISTER(SCM_INIT_VECTOR_1, *(((uint32_t*)&owner_id)+1));

        /* Set modes and kick off the decryption */
        crypto_status = scc_do_crypto(length,
                                      SCM_CONTROL_START_CIPHER
                                      | SCM_CBC_MODE | SCM_DECRYPT_MODE);

        if (crypto_status != 0) {
#ifdef SCC_DEBUG
            printk("SCM decrypt black crypto failure: 0x%x\n",
                   crypto_status);
#endif
        } else {
            status = SCC_RET_OK;
        }
    }

    spin_unlock_irqrestore(&scc_crypto_lock, irq_flags);

    return status;
}


/*****************************************************************************/
/* fn scc_get_slot_info()                                                    */
/*****************************************************************************/
/*!
 * Determine address and value length for a give slot.
 *
 * @param owner_id      Value of owner of slot
 * @param slot          Handle of slot
 * @param address       Location to store kernel address of slot data
 * @param value_size_bytes Location to store allocated length of data in slot.
 *                         May be NULL if value is not needed by caller.
 * @param slot_size_bytes  Location to store max length data in slot
 *                         May be NULL if value is not needed by caller.
 *
 * @return SCC_RET_OK or error indication
 */
scc_return_t
scc_get_slot_info(uint64_t owner_id, uint32_t slot, uint32_t *address,
                  uint32_t *value_size_bytes, uint32_t *slot_size_bytes)
{
    scc_return_t status = verify_slot_access(owner_id, slot, 0);

    if (status == SCC_RET_OK) {
        *address = SCC_BASE + SCM_RED_MEMORY + scc_key_info[slot].offset;
        if (value_size_bytes != NULL) {
            *value_size_bytes = scc_key_info[slot].length;
        }
        if (slot_size_bytes != NULL) {
            *slot_size_bytes = SCC_KEY_SLOT_SIZE;
        }
    }

    return status;
}


/*****************************************************************************/
/* fn scc_wait_completion()                                                  */
/*****************************************************************************/
/*!
 * Poll looking for end-of-cipher indication. Only used
 * if @c SCC_SCM_SLEEP is not defined.
 *
 * @internal
 *
 * On a Tahiti, crypto under 230 or so bytes is done after the first loop, all
 * the way up to five sets of spins for 1024 bytes.  (8- and 16-byte functions
 * are done when we first look.  Zeroizing takes one pass around.
 */
static void
scc_wait_completion(void) {
    int i = 0;

    /* check for completion by polling */
    while (!is_cipher_done() && (i++ < SCC_CIPHER_MAX_POLL_COUNT)) {
        int j;
        /* kill time if loop not optimized away */
        for (j = 0; j < SCC_SPIN_COUNT; j++);
    }
#ifdef SCC_DEBUG
    printk("SCC: Polled DONE %d times\n", i);
#endif
} /* scc_wait_completion() */


/*****************************************************************************/
/* fn is_cipher_done()                                                       */
/*****************************************************************************/
/*!
 * This function returns non-zero if SCM Status register indicates
 * that a cipher has terminated or some other interrupt-generating
 * condition has occurred.
 */
static int
is_cipher_done(void)
{
    register uint32_t scm_status;
    register int cipher_done;

    scm_status = SCC_READ_REGISTER(SCM_STATUS);

    /*
     * Done when 'SCM is currently performing a function' bits are zero
     */
    cipher_done = ! (scm_status & (SCM_STATUS_ZEROIZING |
                                   SCM_STATUS_CIPHERING));

    return cipher_done;
} /* is_cipher_done() */


/*****************************************************************************/
/* fn check_register_accessible()                                            */
/*****************************************************************************/
/*!
 *  Given the current SCM and SMN status, verify that access to the requested
 *  register should be OK.
 *
 *  @param[in]   register_offset  register offset within SCC
 *  @param[in]   smn_status  recent value from #SMN_STATUS
 *  @param[in]   scm_status  recent value from #SCM_STATUS
 *
 *  @return   #SCC_RET_OK if ok, #SCC_RET_FAIL if not
 */
static scc_return_t
check_register_accessible (uint32_t register_offset, uint32_t smn_status,
                           uint32_t scm_status)
{
    int error_code = SCC_RET_FAIL;

    /* Verify that the register offset passed in is not among the verboten set
     * if the SMN is in Fail mode.
     */
    if (offset_within_smn(register_offset)) {
        if ((smn_status & SMN_STATUS_STATE_MASK) == SMN_STATE_FAIL) {
            if ( !((register_offset == SMN_STATUS) ||
                   (register_offset == SMN_COMMAND) ||
                   (register_offset == SMN_DEBUG_DETECT_STAT))) {
#ifdef SCC_DEBUG
                printk("SCC Driver: Note: Security State is in FAIL state.\n");
#endif
            } /* register not a safe one */
            else {
                /* SMN is in  FAIL, but register is a safe one */
                error_code = SCC_RET_OK;
            }
        } /* State is FAIL */
        else {
            /* State is not fail.  All registers accessible. */
            error_code = SCC_RET_OK;
        }
    } /* offset within SMN */
    /*  Not SCM register.  Check for SCM busy. */
    else if (offset_within_scm(register_offset)) {
        /* This is the 'cannot access' condition in the SCM */
        if ((scm_status & SCM_STATUS_BUSY)
            /* these are always available  - rest fail on busy*/
            && !((register_offset == SCM_STATUS) ||
                 (register_offset == SCM_ERROR_STATUS) ||
                 (register_offset == SCM_INTERRUPT_CTRL) ||
                 (register_offset == SCM_CONFIGURATION))) {
#ifdef SCC_DEBUG
            printk("SCC Driver: Note: Secure Memory is in BUSY state.\n");
#endif
        } /* status is busy & register inaccessible */
        else {
            error_code = SCC_RET_OK;
        }
    } /* offset within SCM */

    return error_code;

} /* check_register_accessible() */


/*****************************************************************************/
/* fn offset_within_smn()                                                    */
/*****************************************************************************/
/*!
 *  Check that the offset is with the bounds of the SMN register set.
 *
 *  @param[in]  register_offset    register offset of SMN.
 *
 *  @return   1 if true, 0 if false (not within SMN)
 */
inline static int
offset_within_smn(uint32_t register_offset)
{
    return register_offset >= SMN_STATUS
        && register_offset <= SMN_TIMER;
}


/*****************************************************************************/
/* fn offset_within_scm()                                                    */
/*****************************************************************************/
/*!
 *  Check that the offset is with the bounds of the SCM register set.
 *
 *  @param[in]  register_offset    Register offset of SCM
 *
 *  @return   1 if true, 0 if false (not within SCM)
 */
inline static int
offset_within_scm(uint32_t register_offset)
{
    return (register_offset >= SCM_RED_START)
           && (register_offset < scm_highest_memory_address);
    /* Although this would cause trouble for zeroize testing, this change would
     * close a security whole which currently allows any kernel program to access
     * any location in RED RAM.  Perhaps enforce in non-SCC_DEBUG compiles?
           && (register_offset <= SCM_INIT_VECTOR_1); */
}


/*****************************************************************************/
/* fn check_register_offset()                                                */
/*****************************************************************************/
/*!
 *  Check that the offset is with the bounds of the SCC register set.
 *
 *  @param[in]  register_offset    register offset of SMN.
 *
 * #SCC_RET_OK if ok, #SCC_RET_FAIL if not
 */
static scc_return_t
check_register_offset(uint32_t register_offset)
{
    int return_value = SCC_RET_FAIL;

    /* Is it valid word offset ? */
    if (SCC_BYTE_OFFSET(register_offset) == 0) {
        /* Yes. Is register within SCM? */
        if (offset_within_scm(register_offset)) {
            return_value = SCC_RET_OK;  /* yes, all ok */
        }
        /* Not in SCM.  Now look within the SMN */
        else if (offset_within_smn(register_offset)) {
            return_value = SCC_RET_OK;  /* yes, all ok */
        }
    }

    return return_value;
}



#ifdef SCC_REGISTER_DEBUG

/*****************************************************************************/
/* fn dbg_scc_read_register()                                                */
/*****************************************************************************/
/*!
 * Noisily read a 32-bit value to an SCC register.
 * @param offset        The address of the register to read.
 *
 * @return  The register value
 * */
static uint32_t
dbg_scc_read_register(uint32_t offset)
{
    uint32_t value;

    value = __raw_readl(scc_base+offset);

#ifndef SCC_RAM_DEBUG          /* print no RAM references */
    if ((offset < SCM_RED_MEMORY) || (offset >= scm_highest_memory_address)) {
#endif
        printk("SCC RD: 0x%4x : 0x%08x\n", offset, value );
#ifndef SCC_RAM_DEBUG
    }
#endif

    return value;
}


/*****************************************************************************/
/* fn dbg_scc_write_register()                                               */
/*****************************************************************************/
/*
 * Noisily read a 32-bit value to an SCC register.
 * @param offset        The address of the register to written.
 *
 * @param value         The new register value
 */
static void
dbg_scc_write_register(uint32_t offset, uint32_t value)
{

#ifndef SCC_RAM_DEBUG          /* print no RAM references */
    if ((offset < SCM_RED_MEMORY) || (offset >= scm_highest_memory_address)) {
#endif
        printk("SCC WR: 0x%4x : 0x%08x\n", offset, value);
#ifndef SCC_RAM_DEBUG
    }
#endif

    (void)__raw_writel(value, scc_base+offset);
}

#endif /* SCC_REGISTER_DEBUG */

