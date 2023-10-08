/*
 * Copyright 2005-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2006-2007 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 06/06/2006   Motorola  Fixed free_hot_cold page error while using
 *                        FSL crypto through TPAPI
 * 10/23/2007   Motorola  Add mpm advise calls and clock gating.
 *
 */

/*!
 * @file sah_hardware_interface.c
 *
 * @brief Provides an interface to the SAHARA hardware registers.
 *
 * @ingroup MXCSAHARA2
 */

/* SAHARA Includes */
#include <sah_driver_common.h>
#include <sah_hardware_interface.h>
#include <sah_memory_mapper.h>
#include <sah_kernel.h>

#ifdef SAHARA_MOT_FEAT_PM
#include <linux/mpm.h>
#endif

#if defined DIAG_DRV_IF || defined(DO_DBG)
#include <diagnostic.h>
#ifndef LOG_KDIAG
#define LOG_KDIAG(x) os_printk("%s\n", x)
#endif

static void sah_Dump_Link(const char *prefix, const sah_Link *link);
void sah_Dump_Words(const char *prefix, const unsigned *data, unsigned length);


/* This is for sprintf() to use when constructing output. */
#define DIAG_MSG_SIZE   1024
/* was 200 */
#define MAX_DUMP        200
static char Diag_msg[DIAG_MSG_SIZE];

#endif /* DIAG_DRV_IF */

/*!
 * Number of descriptors sent to Sahara.  This value should only be updated
 * with the main queue lock held.
 */
uint32_t dar_count;

#ifdef SAHARA_MOT_FEAT_PM
extern int mpm_sahara_dev_num;
#endif

/*! The "link-list optimize" bit in the Header of a Descriptor */
#define SAH_HDR_LLO 0x01000000


/* IO_ADDRESS() is Linux macro -- need portable equivalent */
#define SAHARA_BASE_ADDRESS             IO_ADDRESS(SAHA_BASE_ADDR)
#define SAHARA_VERSION_REGISTER_OFFSET  0x000
#define SAHARA_DAR_REGISTER_OFFSET      0x004
#define SAHARA_CONTROL_REGISTER_OFFSET  0x008
#define SAHARA_COMMAND_REGISTER_OFFSET  0x00C
#define SAHARA_STATUS_REGISTER_OFFSET   0x010
#define SAHARA_ESTATUS_REGISTER_OFFSET  0x014
#define SAHARA_FLT_ADD_REGISTER_OFFSET  0x018
#define SAHARA_CDAR_REGISTER_OFFSET     0x01C
#define SAHARA_IDAR_REGISTER_OFFSET     0x020

/*! Register within Sahara which contains hardware version. (1 or 2).  */
#define SAHARA_VERSION_REGISTER (SAHARA_BASE_ADDRESS + \
                                 SAHARA_VERSION_REGISTER_OFFSET)

/*! Register within Sahara which is used to provide new work to the block. */
#define SAHARA_DAR_REGISTER     (SAHARA_BASE_ADDRESS + \
                                 SAHARA_DAR_REGISTER_OFFSET)

/*! Register with Sahara which is used for configuration. */
#define SAHARA_CONTROL_REGISTER (SAHARA_BASE_ADDRESS + \
                                 SAHARA_CONTROL_REGISTER_OFFSET)

/*! Register with Sahara which is used for changing status. */
#define SAHARA_COMMAND_REGISTER (SAHARA_BASE_ADDRESS + \
                                 SAHARA_COMMAND_REGISTER_OFFSET)

/*! Register with Sahara which is contains status and state. */
#define SAHARA_STATUS_REGISTER  (SAHARA_BASE_ADDRESS + \
                                 SAHARA_STATUS_REGISTER_OFFSET)

/*! Register with Sahara which is contains error status information. */
#define SAHARA_ESTATUS_REGISTER (SAHARA_BASE_ADDRESS + \
                                 SAHARA_ESTATUS_REGISTER_OFFSET)

/*! Register with Sahara which is contains faulting address information. */
#define SAHARA_FLT_ADD_REGISTER (SAHARA_BASE_ADDRESS + \
                                 SAHARA_FLT_ADD_REGISTER_OFFSET)

/*! Register with Sahara which is contains current descriptor address. */
#define SAHARA_CDAR_REGISTER    (SAHARA_BASE_ADDRESS + \
                                 SAHARA_CDAR_REGISTER_OFFSET)

/*! Register with Sahara which is contains initial descriptor address (of a
    chain). */
#define SAHARA_IDAR_REGISTER    (SAHARA_BASE_ADDRESS + \
                                 SAHARA_IDAR_REGISTER_OFFSET)


/* Local Functions */
#if defined DIAG_DRV_IF || defined DO_DBG
void sah_Dump_Region(
    const char *prefix, const unsigned char *data, unsigned length);
#endif /* DIAG_DRV_IF */

/* time out value when polling SAHARA status register for completion */
static uint32_t sah_poll_timeout = 0xFFFFFFFF;

/*!
 * Polls Sahara to determine when its current operation is complete
 *
 * @return   last value found in Sahara's status register
 */
sah_Execute_Status sah_Wait_On_Sahara()
{
    uint32_t count = 0;  /* ensure we don't get stuck in the loop forever */
    sah_Execute_Status status;  /* Sahara's status register */


#ifdef DIAG_DRV_IF
    LOG_KDIAG("Entered sah_Wait_On_Sahara");
#endif

    do {
        /* get current status register from Sahara */
        status = sah_HW_Read_Status() & SAH_EXEC_STATE_MASK;

        /* timeout if SAHARA takes too long to complete */
        if (++count == sah_poll_timeout) {
            status = SAH_EXEC_FAULT;
#ifdef DIAG_DRV_IF
            LOG_KDIAG("sah_Wait_On_Sahara timed out");
#endif
        }

    /* stay in loop as long as Sahara is still busy */
    } while ((status == SAH_EXEC_BUSY) ||
             (status == SAH_EXEC_DONE1_BUSY2));
    
    return status;
}


/*!
 * This function resets the SAHARA hardware. The following operations are
 * performed:
 *   1. Resets SAHARA.
 *   2. Requests BATCH mode.
 *   3. Enables interrupts.
 *   4. Requests Little Endian mode.
 *
 * @brief     SAHARA hardware reset function.
 *
 * @return   void
 */
int sah_HW_Reset(void)
{
    sah_Execute_Status sah_state;
    int  status;  /* this is the value to return to the calling routine */
    uint32_t saha_control = 0;

#ifdef SAHARA1
    saha_control |= CTRL_LITTLE_END;
#else
#ifndef CONFIG_VIRTIO_SUPPORT

#ifndef USE_3WORD_BURST
/***************** HARDWARE BUG WORK AROUND ******************/
/*
 * On current versions of HW, the ALLOW_1K_CROSSING flag must be undefined if
 * this flag is going to be undefined (allowing 8-word bursts), so that a
 * software workaround will get compiled in.
 *
 * The workaround is as follows:  By examining and modifying sah_Link chains
 * used in output (writes by Sahara), the driver can prevent Sahara from ever
 * having a link which crosses a 1k boundary on a non-word-aligned offset.
 */
    saha_control |= (8 << 16);  /* Allow 8-word burst */
#else
/***************** HARDWARE BUG WORK AROUND ******************/
/* A burst size of > 4 can cause Sahara DMA to issue invalid AHB transactions
 * when crossing 1KB boundaries.  By limiting the 'burst size' to 3, these
 * invalid transactions will not be generated, but Sahara will still transfer
 * data more efficiently than if the burst size were set to 1.
 */
    saha_control |= (3 << 16);  /* Limit DMA burst size. For versions 2/3 */
#endif /* USE_3WORD_BURST */

#endif /* CONFIG_VIRTIO_SUPPORT */

#endif /* SAHARA1 */

#ifdef DIAG_DRV_IF
    snprintf(Diag_msg, DIAG_MSG_SIZE,
            "Address of SAHARA_BASE_ADDRESS = 0x%08x\n", SAHARA_BASE_ADDRESS );
    LOG_KDIAG(Diag_msg);
#endif

#ifdef SAHARA_MOT_FEAT_PM
    MPM_DRIVER_ADVISE(mpm_sahara_dev_num, MPM_ADVICE_DRIVER_IS_BUSY);
#ifdef DIAG_DRV_IF
    printk(KERN_ALERT"sahara(sah_HW_Reset):MPM_ADVICE_DRIVER_IS_BUSY\n"); 
#endif
#endif /* SAHARA_MOT_FEAT_PM */

    /* Write the Reset & BATCH mode command to the SAHARA Command register. */
    sah_HW_Write_Command(CMD_BATCH | CMD_RESET);

/***************** VIRTIO PATCH ******************/
#ifdef CONFIG_VIRTIO_SUPPORT
    /* Pause 1/4 second for System C model to complete initialization */
    msleep(250);

#endif /* CONFIG_VIRTIO_SUPPORT */
/***************** END VIRTIO PATCH ******************/

    sah_state = sah_Wait_On_Sahara();

#ifdef SAHARA_MOT_FEAT_PM
    MPM_DRIVER_ADVISE(mpm_sahara_dev_num, MPM_ADVICE_DRIVER_IS_NOT_BUSY);
#ifdef DIAG_DRV_IF
    printk(KERN_ALERT"sahara(sah_HW_Reset):MPM advise MPM_ADVICE_DRIVER_IS_NOT_BUSY\n");
#endif
#endif /* SAHARA_MOT_FEAT_PM */

    /* on reset completion, check that Sahara is in the idle state */
    status = (sah_state == SAH_EXEC_IDLE) ?  0 : OS_ERROR_FAIL_S;

    /* Set initial value out of reset */
    sah_HW_Write_Control(saha_control);

/***************** HARDWARE BUG WORK AROUND ******************/
/* In order to set the 'auto reseed' bit, must first acquire a random value. */
        /*
         * to solve a hardware bug, a random number must be generated before
         * the 'RNG Auto Reseed' bit can be set. So this generates a random
         * number that is thrown away.
         *
         * Note that the interrupt bit has not been set at this point so
         * the result can be polled.
         */
#ifdef DIAG_DRV_IF
    LOG_KDIAG("Create and submit Random Number Descriptor");
#endif

    if (status == OS_ERROR_OK_S) {
        /* place to put random number */
        volatile uint32_t* random_data_ptr;
        sah_Head_Desc* random_desc;
        dma_addr_t desc_dma;
        dma_addr_t rand_dma;
        const int rnd_cnt = 3;      /* how many random 32-bit values to get */

        /* Get space for data -- assume at least 32-bit aligned! */
        random_data_ptr = os_alloc_memory(rnd_cnt * sizeof(uint32_t),
                                          GFP_ATOMIC);

        random_desc = sah_Alloc_Head_Descriptor();

        if ((random_data_ptr == NULL) || (random_desc == NULL)) {
            status = OS_ERROR_FAIL_S;
        } else {
            int i;

            /* Clear out values */
            for (i = 0; i < rnd_cnt; i++) {
                random_data_ptr[i] = 0;
            }

            rand_dma = os_pa(random_data_ptr);

            random_desc->desc.header = 0xB18C0000; /* LLO get random number */
            random_desc->desc.len1 = rnd_cnt * sizeof(*random_data_ptr);
            random_desc->desc.ptr1 = (void*)rand_dma;
            random_desc->desc.original_ptr1 = (void*)random_data_ptr;
            random_desc->desc.len2 = 0; /* not used */
            random_desc->desc.ptr2 = 0; /* not used */
            random_desc->desc.next = 0; /* chain terminates here */
            random_desc->desc.original_next = 0; /* chain terminates here */

            desc_dma = random_desc->desc.dma_addr;

            /* Force in-cache data out to RAM */
            os_cache_clean_range(random_data_ptr, rnd_cnt * sizeof(uint32_t));

#ifdef SAHARA_MOT_FEAT_PM
            MPM_DRIVER_ADVISE(mpm_sahara_dev_num, MPM_ADVICE_DRIVER_IS_BUSY);
#ifdef DIAG_DRV_IF
            printk(KERN_ALERT"sahara(sah_HW_Write_DAR):MPM_ADVICE_DRIVER_IS_BUSY\n");
#endif /* DIAG_DRV_IF */
#endif /* SAHARA_MOT_FEAT_PM */

            /* pass descriptor to Sahara */
            sah_HW_Write_DAR(desc_dma);

            /*
             * Wait for RNG to complete (interrupts are disabled at this point
             * due to sahara being reset previously) then check for error
             */
            sah_state = sah_Wait_On_Sahara();

#ifdef SAHARA_MOT_FEAT_PM
            MPM_DRIVER_ADVISE(mpm_sahara_dev_num, MPM_ADVICE_DRIVER_IS_NOT_BUSY);
#ifdef DIAG_DRV_IF
            printk(KERN_ALERT"sahara(sah_HW_Reset):MPM advise MPM_ADVICE_DRIVER_IS_NOT_BUSY for radom number.\n");
#endif
#endif /* SAHARA_MOT_FEAT_PM */

            /* Force CPU to ignore in-cache and reload from RAM */
            os_cache_inv_range(random_data_ptr, rnd_cnt * sizeof(uint32_t));

            /* if it didn't move to done state, an error occured */
            if (
#ifndef SUBMIT_MULTIPLE_DARS
                (sah_state != SAH_EXEC_IDLE) &&
#endif
                (sah_state != SAH_EXEC_DONE1)
               ) {
                status = OS_ERROR_FAIL_S;
                os_printk("(sahara) Failure: state is %08x; random_data is"
                          " %08x\n", sah_state, *random_data_ptr);
                os_printk("(sahara) CDAR: %08x, IDAR: %08x, FADR: %08x,"
                          " ESTAT: %08x\n",
                          sah_HW_Read_CDAR(), sah_HW_Read_IDAR(),
                          sah_HW_Read_Fault_Address(),
                          sah_HW_Read_Error_Status());
            } else {
                int i;
                int seen_rand = 0;

                for (i = 0; i < rnd_cnt; i++) {
                    if (*random_data_ptr != 0) {
                        seen_rand = 1;
                        break;
                    }
                }
                if (!seen_rand) {
                    status = OS_ERROR_FAIL_S;
                    os_printk("(sahara) Error:  Random number is zero!\n");
                }
            }
        }

        if (random_data_ptr) {
            os_free_memory((void*)random_data_ptr);
        }
        if (random_desc) {
            sah_Free_Head_Descriptor(random_desc);
        }
    }
/***************** END HARDWARE BUG WORK AROUND ******************/


    if (status == 0) {
#ifndef SAHARA1
        saha_control |= CTRL_RNG_RESEED;
#endif

#ifndef SAHARA_POLL_MODE
        saha_control |= CTRL_INT_EN; /* enable interrupts */
#endif

#ifdef DIAG_DRV_IF
        LOG_KDIAG("Setting up Sahara's Control Register\n");
#endif
        /* Rewrite the setup to the SAHARA Control register */
        sah_HW_Write_Control(saha_control);

#ifdef SAHARA_POLL_MODE
        /* timeout on reading SAHARA status register */
        sah_poll_timeout = SAHARA_POLL_MODE_TIMEOUT;
#endif
    }
    return status;
}


/*!
 * This function enables High Assurance mode.
 *
 * @brief     SAHARA hardware enable High Assurance mode.
 *
 * @return   FSL_RETURN_OK_S             - if HA was set successfully
 * @return   FSL_RETURN_INTERNAL_ERROR_S - if HA was not set due to SAHARA
 *                                         being busy.
 */
fsl_shw_return_t sah_HW_Set_HA(void)
{
    /* This is the value to write to the register */
    uint32_t value;

    /* Read from the control register. */
    value = sah_HW_Read_Control();

    /* Set the HA bit */
    value |= CTRL_HA;

    /* Write to the control register. */
    sah_HW_Write_Control(value);

    /* Read from the control register. */
    value = sah_HW_Read_Control();

    return (value & CTRL_HA) ? FSL_RETURN_OK_S : FSL_RETURN_INTERNAL_ERROR_S;
}


/*!
 * This function reads the SAHARA hardware Version Register.
 *
 * @brief     Read SAHARA hardware Version Register.
 *
 * @return   uint32_t Status value.
 */
uint32_t sah_HW_Read_Version(void)
{
    return os_read32(SAHARA_VERSION_REGISTER);
}


/*!
 * This function reads the SAHARA hardware Control Register.
 *
 * @brief     Read SAHARA hardware Control Register.
 *
 * @return   uint32_t Status value.
 */
uint32_t sah_HW_Read_Control(void)
{
    return os_read32(SAHARA_CONTROL_REGISTER);
}


/*!
 * This function reads the SAHARA hardware Status Register.
 *
 * @brief     Read SAHARA hardware Status Register.
 *
 * @return   uint32_t Status value.
 */
uint32_t sah_HW_Read_Status(void)
{
    return os_read32(SAHARA_STATUS_REGISTER);
}


/*!
 * This function reads the SAHARA hardware Error Status Register.
 *
 * @brief     Read SAHARA hardware Error Status Register.
 *
 * @return   uint32_t Error Status value.
 */
uint32_t sah_HW_Read_Error_Status(void)
{
    return os_read32(SAHARA_ESTATUS_REGISTER);
}


/*!
 * This function reads the SAHARA hardware Descriptor Address Register.
 *
 * @brief     Read SAHARA hardware DAR Register.
 *
 * @return   uint32_t DAR value.
 */
uint32_t sah_HW_Read_DAR(void)
{
    return os_read32(SAHARA_DAR_REGISTER);
}


/*!
 * This function reads the SAHARA hardware Current Descriptor Address Register.
 *
 * @brief     Read SAHARA hardware CDAR Register.
 *
 * @return   uint32_t CDAR value.
 */
uint32_t sah_HW_Read_CDAR(void)
{
    return os_read32(SAHARA_CDAR_REGISTER);
}


/*!
 * This function reads the SAHARA hardware Initial Descriptor Address Register.
 *
 * @brief     Read SAHARA hardware IDAR Register.
 *
 * @return   uint32_t IDAR value.
 */
uint32_t sah_HW_Read_IDAR(void)
{
    return os_read32(SAHARA_IDAR_REGISTER);
}


/*!
 * This function reads the SAHARA hardware Fault Address Register.
 *
 * @brief     Read SAHARA Fault Address Register.
 *
 * @return   uint32_t Fault Address value.
 */
uint32_t sah_HW_Read_Fault_Address(void)
{
    return os_read32(SAHARA_FLT_ADD_REGISTER);
}


/*!
 * This function writes a command to the SAHARA hardware Command Register.
 *
 * @brief     Write to SAHARA hardware Command Register.
 *
 * @param    command     An unsigned 32bit command value.
 *
 * @return   void
 */
void sah_HW_Write_Command(uint32_t command)
{
    os_write32(SAHARA_COMMAND_REGISTER, command);
}


/*!
 * This function writes a control value to the SAHARA hardware Control
 * Register.
 *
 * @brief     Write to SAHARA hardware Control Register.
 *
 * @param    control     An unsigned 32bit control value.
 *
 * @return   void
 */
void sah_HW_Write_Control(uint32_t control)
{
    os_write32(SAHARA_CONTROL_REGISTER, control);
}


/*!
 * This function writes a descriptor address to the SAHARA Descriptor Address
 * Register.
 *
 * @brief     Write to SAHARA Descriptor Address Register.
 *
 * @param    pointer     An unsigned 32bit descriptor address value.
 *
 * @return   void
 */
void sah_HW_Write_DAR(uint32_t pointer)
{
    os_write32(SAHARA_DAR_REGISTER, pointer);
    dar_count++;
}

#if defined DIAG_DRV_IF || defined DO_DBG

/*!
 * Dump chain of descriptors to the log.
 *
 * @brief Dump descriptor chain
 *
 * @param    chain     Kernel virtual address of start of chain of descriptors
 *
 * @return   void
 */
void sah_Dump_Chain(const sah_Desc *chain)
{

    LOG_KDIAG("Chain for Sahara");
        
    while (chain != NULL) {
        sah_Dump_Words("Desc", (unsigned *)chain, 6 /* #words in h/w link */);
        if (chain->ptr1) {
            if (chain->header & SAH_HDR_LLO) {
                sah_Dump_Region(" Data1", chain->original_ptr1, chain->len1 );
            } else {
                sah_Dump_Link(" Link1", chain->original_ptr1);
            }
        }
        if (chain->ptr2) {
            if (chain->header & SAH_HDR_LLO) {
                sah_Dump_Region(" Data2", chain->original_ptr2, chain->len2 );
            } else {
                sah_Dump_Link(" Link2", chain->original_ptr2);
            }
        }

        chain = chain->original_next;
    }

}


/*!
 * Dump chain of links to the log.
 *
 * @brief Dump chain of links
 *
 * @param    prefix    Text to put in front of dumped data
 * @param    link      Kernel virtual address of start of chain of links
 *
 * @return   void
 */
static void sah_Dump_Link(const char *prefix, const sah_Link *link)
{
    while (link != NULL) {
        sah_Dump_Words(prefix, (unsigned *)link, 3 /* # words in h/w link */);
        link = link->original_next;
    }
}


/*!
 * Dump given region of data to the log.
 *
 * @brief Dump data
 *
 * @param    prefix    Text to put in front of dumped data
 * @param    data      Kernel virtual address of start of region to dump
 * @param    length    Amount of data to dump
 *
 * @return   void
 */
void sah_Dump_Region(
        const char *prefix, const unsigned char *data, unsigned length)
{
    unsigned count;
    char *output;
    unsigned data_len;

    sprintf(Diag_msg, "%s (%08X,%u):", prefix, (uint32_t)data, length);

    /* Restrict amount of data to dump */
    if (length > MAX_DUMP) {
        data_len = MAX_DUMP;
    } else {
        data_len = length;
    }

    /* We've already printed some text in output buffer, skip over it */
    output = Diag_msg + strlen(Diag_msg);

    for (count = 0; count < data_len; count++) {
        if (count % 4 == 0) {
            *output++ = ' ';
        }
        sprintf(output, "%02X", *data++);
        output += 2;
    }

    LOG_KDIAG(Diag_msg);
}


/*!
 * Dump given word of data to the log.
 *
 * @brief Dump data
 *
 * @param    prefix       Text to put in front of dumped data
 * @param    data         Kernel virtual address of start of region to dump
 * @param    word_count   Amount of data to dump
 *
 * @return   void
 */
void sah_Dump_Words(
    const char *prefix, const unsigned *data, unsigned word_count)
{
    char *output;

    sprintf(Diag_msg, "%s (%08X,%uw): ", prefix, (uint32_t)data, word_count);


    /* We've already printed some text in output buffer, skip over it */
    output = Diag_msg + strlen(Diag_msg);

    while (word_count--) {
        sprintf(output, "%08X ", *data++);
        output += 9;
    }

    LOG_KDIAG(Diag_msg);
}

#endif  /* DIAG_DRV_IF */

/* End of sah_hardware_interface.c */
