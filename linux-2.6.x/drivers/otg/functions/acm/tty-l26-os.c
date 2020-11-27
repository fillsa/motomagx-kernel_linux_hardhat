/* * otg/functions/acm/tty-l26-os.c
 * @(#) balden@belcarra.com|otg/functions/acm/tty-l26-os.c|20060407231727|45722
 *
 *      Copyright (c) 2003-2005 Belcarra Technologies Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006,2007 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 08/16/2006         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language 
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 03/24/2007         Motorola         check for acm if needed
 * 06/05/2007         Motorola         Changes to support HSDPA
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */

/*!
 * @file otg/functions/acm/tty-l26-os.c
 * @brief ACM Function Driver private defines
 *
 * This is the OS portion of the TTY version of the
 * ACM driver.
 *
 *                    TTY           
 *                    Interface     
 *    Upper           +----------+  
 *    Edge            | tty-l26  |  <--------
 *    Implementation  +----------+  
 *                                  
 *                                  
 *    Function        +----------+  
 *    Descriptors     | tty-if   |  
 *    Registration    +----------+  
 *                                  
 *                                  
 *    Function        +----------+  
 *    I/O             | acm      |  
 *    Implementation  +----------+  
 *
 *
 *    Module          +----------+
 *    Loading         | acm-l26  |   
 *                    +----------+
 *
 *
 *
 * @ingroup TTYFunction
 */


#include <otg/otg-compat.h>

#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/ctype.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/atomic.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/smp_lock.h>
#include <linux/slab.h>
#include <linux/serial.h>

#ifdef CONFIG_OTG_ACM_CONSOLE
#include <linux/console.h>
#include <linux/config.h>
#include <linux/kernel.h>
#endif

#include <linux/devfs_fs_kernel.h>

#include <otg/usbp-chap9.h>
#include <otg/usbp-cdc.h>
#include <otg/usbp-func.h>

#include <linux/capability.h>
#include <otg/otg-trace.h>
#include <public/otg-node.h>
#include "acm.h"
#include "tty.h"


/* 
 * All functions take one of struct tty or struct usbd_function_instance to
 * identify the port and/or function.
 */

static struct tty_struct **tty_table;
static struct usbd_function_instance **function_table;
static u8 tty_minor_count;

/* ******************************************************************************************* */

void tty_l26_wakeup_writers(void *data);
void tty_l26_recv_flip(void *data);
void tty_l26_call_hangup(void *data);
int tty_l26_block_until_ready(struct tty_struct *tty, struct file *filp);
int tty_l26_loop_xmit_chars(struct usbd_function_instance *function_instance, 
                int count, const unsigned char *buf);
int tty_l26_os_recv_chars(struct usbd_function_instance *function_instance, u8 *cp, int n);
unsigned int tty_l26_tiocm(struct usbd_function_instance *function_instance);
static int tty_l26_tiocmset(struct tty_struct *tty, struct file *file, unsigned int set, unsigned int clear);

void tty_l26_recv_start(void *data);
void tty_l26_recv_start_bh(struct usbd_function_instance *function_instance);
void tty_l26_os_line_coding(struct usbd_function_instance *function_instance);

#ifdef CONFIG_OTG_ACM_CONSOLE
static void acm_serial_console_init (void);
static void acm_serial_console_exit (void);
#endif

/* ******************************************************************************************* */
/* ******************************************************************************************* */
/* Linux TTY Functions
 */
/*!
 * @brief tty_l26_open
 *
 * Called by TTY layer to open device.
 *
 * @param tty
 * @param filp
 * @returns non-zero for error
 */
static int tty_l26_open(struct tty_struct *tty, struct file *filp)
{
        
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        BOOL nonblocking;
        unsigned long flags;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", acm);
        #endif
        /* First open */
        if(!os_private) {
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG0(TTY, "FIRST OPEN");
                #endif
                tty_table[index] = tty;
                tty->low_latency = 1;
                THROW_UNLESS((os_private = CKMALLOC(sizeof(struct os_private), GFP_KERNEL)), error);

                local_irq_save(flags);
                os_private->index = index;
                os_private->tiocgicount = 0;
                tty->driver_data = os_private;
                init_waitqueue_head(&os_private->open_wait);
                init_waitqueue_head(&os_private->tiocm_wait);
                init_waitqueue_head(&os_private->send_wait);
                PREPARE_WORK_ITEM(os_private->wqueue, tty_l26_wakeup_writers, tty);
                PREPARE_WORK_ITEM(os_private->hqueue, tty_l26_call_hangup, tty);
                PREPARE_WORK_ITEM(os_private->rqueue, tty_l26_recv_start, tty);
                local_irq_restore(flags);
        
                /* 
                 * To truly emulate the old dual-device approach of having a non-blocking
                 * device (e.g cu0) and a blocking device (e.g. tty0) we would need to
                 * track blocking and non-blocking opens separately.  We don't.  This
                 * may lead to funny behavior in the multiple open case.
                 */
                if (function_instance) {
                        /* 
                         * Allow the user to open only after ACM is configured by the host. 
                         * All other opens should fail.
                         */
                         
                        #ifdef CONFIG_OTG_TRACE
                        TRACE_MSG0(TTY, "acm_open will be called");
                        #endif
                        local_irq_save(flags);
                        if(acm_open(function_instance)) {
                                atomic_inc(&os_private->used);
                                return -ENODEV;
                        }
                        acm_set_local(function_instance, 1);
                        local_irq_restore(flags);
                        os_private->flags |= TTYFD_OPENED;
                } else { // NO function_instance - not in acm mode
                        atomic_inc(&os_private->used);
                        return -EINVAL;
                }
        }

        /* Increment open count*/
        atomic_inc(&os_private->used);
        nonblocking = BOOLEAN(filp->f_flags & O_NONBLOCK);      /* O_NONBLOCK is same as O_NDELAY */

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY,"f_flags: %08x NONBLOCK: %x", filp->f_flags, nonblocking);
        #endif
        /* All done if non blocking open
         */
        RETURN_ZERO_IF(nonblocking);

        /* Block open - wait until ready
          */
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "BLOCKING - wait until ready");
        #endif
        return tty_l26_block_until_ready(tty, filp);


        /* The tty layer calls tty_l26_close() even if this open fails,
         * so any cleanup will be done there.
         */

        CATCH(error) {
                return -ENOMEM;
        }
}

/*!
 * @brief tty_l26_close
 *
 * Called by TTY layer to close device.
 *
 * @param tty
 * @param filp
 * @returns non-zero for error
 */
static void tty_l26_close(struct tty_struct *tty, struct file *filp)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int open_count;
        unsigned long flags;

        /* 
         * decrement used counter
         */
        open_count = atomic_pre_dec(&os_private->used);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY, "acm: %x, open_count value: %d", (int)acm, open_count);
        #endif
        RETURN_IF (open_count > 0);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"FINAL CLOSE");
        #endif
        if (function_instance) {
                local_irq_save(flags);
                acm_force_xmit_chars(function_instance);
                acm_close(function_instance);
                local_irq_restore(flags);
        }
    
        if(acm) {
            TRACE_MSG0(TTY,"ACM");
            atomic_set(&acm->wakeup_writers,0);           
        }

        /* This should never happen if this is the last close,
         * but it can't hurt to check.
         */
        wake_up_interruptible(&os_private->open_wait);

        while ( PENDING_WORK_ITEM(os_private->wqueue) ||
                PENDING_WORK_ITEM(os_private->hqueue) ||
                PENDING_WORK_ITEM(os_private->rqueue)) 
        {
                TRACE_MSG0(TTY, "waiting for the work queues");
                SCHEDULE_TIMEOUT(1);
        }

        PREPARE_WORK_ITEM(os_private->wqueue, NULL, NULL);
        PREPARE_WORK_ITEM(os_private->hqueue, NULL, NULL);
        PREPARE_WORK_ITEM(os_private->rqueue, NULL, NULL);

        if (os_private) {
                LKFREE(os_private);
                os_private=NULL;
                tty->driver_data=NULL;
        }
        tty_table[index]=NULL;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"FINISHED");
        #endif
}

/* ******************************************************************************************* */


/*!
 * @brief tty_l26_block_until_ready
 *
 * Called from tty_l26_open to implement blocking open. Wait for DTR indication
 * from acm.
 *
 * Called by TTY layer to open device.
 *
 * @param tty
 * @param filp
 * @returns non-zero for error
 */
int tty_l26_block_until_ready(struct tty_struct *tty, struct file *filp)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned long flags;
        int rc = 0;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif

        for (;;) {
                /* check if the file has been closed */
                if (tty_hung_up_p(filp)) {
                        TRACE_MSG0(TTY,"tty_hung_up_p()");
                        return -ERESTARTSYS;
                }
                /* check for pending signals */
                if (signal_pending(current)) {
                        TRACE_MSG0(TTY,"signal_pending()");
                        return -ERESTARTSYS;
                }
                /* check if the module is unloading */
                if (os_private->exiting) {
                        TRACE_MSG0(TTY,"module exiting()");
                        return -ENODEV;
                }
                /* check if HOST is open.
                 */
                local_irq_save(flags);
                if (acm_get_d0_dtr(function_instance)) {
                        local_irq_restore(flags);
                        TRACE_MSG0(TTY,"found DTR");
                        return 0;
                }
                local_irq_restore(flags);
                /* sleep */
                interruptible_sleep_on(&os_private->open_wait);
        }
}

/* Transmit Function - called by tty layer ****************************************************** */

/*!
 * @brief tty_l26_chars_in_buffer
 *
 * Called by TTY layer to determine amount of pending write data.
 *
 * @param tty - pointer to tty data structure
 * @return amount of pending write data
 */
static int tty_l26_chars_in_buffer(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        int rc;

        rc = acm_chars_in_buffer(function_instance);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY, "chars in buffer: %d", rc);
        #endif
        return rc;
}

/*! tty_l26_write
 *
 * Called by TTY layer to write data.
 * @param tty - pointer to tty data structure
 * @param buf - pointer to data to send
 * @param  count - number of bytes want to send.
 * @return count - number of bytes actually sent.
 */
int tty_l26_write(struct tty_struct *tty, const unsigned char *buf, int count)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int total_sent_chars= 0;
        unsigned long flags;


        #ifdef CONFIG_OTG_TRACE
        {
                int i;
                const char *cp = buf;
                TRACE_MSG3(TTY,"tty:%x -> count: %d loopback: %d from_usr: %d", tty, count, os_private->tiocm & TIOCM_LOOP);

                for (i = 0; i < count; i += 8) {
                        TRACE_MSG8(TTY, "sos909 SEND [%02x %02x %02x %02x %02x %02x %02x %02x]",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                        );
                
                }
               
        }
        #endif
        RETURN_ZERO_UNLESS(acm && count);
        /* loopback mode
         */
        if (unlikely(os_private->tiocm & TIOCM_LOOP)) {
                return tty_l26_loop_xmit_chars(function_instance, count, buf);
        } else { 
                local_irq_save(flags); 
                total_sent_chars += acm_try_xmit_chars(function_instance, count, buf);

                /* could not write everything asked for */ 
                if(unlikely(total_sent_chars < count)) {
                        TRACE_MSG2(TTY,"asked to write %d bytes: wrote %d bytes only",count,total_sent_chars);
                        atomic_set(&acm->wakeup_writers,1); 
                }
                local_irq_restore(flags);
                return total_sent_chars;
        }
}


/*! tty_l26_write_room
 *
 * Called by TTY layer get amount of write room available.
 *
 * @param tty - pointer to tty data structure
 * @return amount of write room available
 */
static int tty_l26_write_room(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int rc;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"tty: %x", (int)tty);
        #endif
        /* loopback mode
         */
        if (os_private->tiocm & TIOCM_LOOP)
                return  tty->ldisc.receive_room(tty);

        RETURN_ZERO_UNLESS(acm);
        rc = acm_write_room(function_instance);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY, "write room: %d", rc);
        #endif
        return rc;
}

/*! tty_l26_throttle
 *
 * Called by LDISC layer to throttle (do not allow received data.)
 * We check if TTY_THROTTLED flag is set before we send data to LDISC
 * @return amount of write room available
 */
static void tty_l26_throttle(struct tty_struct *tty)
{
        TRACE_MSG1(TTY,"tty: %x", (int)tty);
}

/*! tty_l26_unthrottle
 *
 * Called by LDISC layer to unthrottle (allow received data.)
 *
 * @return amount of write room available
 */
static void tty_l26_unthrottle(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        if(acm == NULL) {
            TRACE_MSG0(TTY,"ACM is null");
            return ;
        }
        TRACE_MSG1(TTY,"UNTHROTLE called: scheduling queue push. tty: %x", (int)tty);
        SET_WORK_ARG(acm->out_work, acm);
        RETURN_IF(NO_WORK_DATA((acm->out_work)) || PENDING_WORK_ITEM((acm->out_work)));
        SCHEDULE_WORK((acm->out_work));
}


/*! tty_l26_fd_ioctl
 *
 * Used by TTY layer to execute IOCTL command.
 *
 * Called by TTY layer to open device.
 *
 * Unhandled commands must return -ENOIOCTLCMD for correct operation.
 *
 * @param tty
 * @param file
 * @param cmd
 * @param arg
 * @returns non-zero for error
 */
static int tty_l26_fd_ioctl(struct tty_struct *tty, struct file *file, unsigned int cmd, unsigned long arg)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        int rc;

        TRACE_MSG3(TTY,"entered: %8x dir: %x size: %x", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));

        switch(cmd) {
        case TIOCGSERIAL:
                TRACE_MSG0(TTY, "TIOCGSERIAL");
                RETURN_EINVAL_IF (copy_to_user((void *)arg, &os_private->serial_struct, sizeof(struct serial_struct)));
                return 0;

        case TIOCSSERIAL:
                TRACE_MSG0(TTY, "TIOCSSERIAL");
                return -EINVAL;

        case TIOCSERCONFIG:
        case TIOCSERGETLSR: /* Get line status register */
        case TIOCSERGSTRUCT:
                TRACE_MSG0(TTY, "TIOCSER*");
                return -EINVAL;

                /*
                 * Wait for any of the 4 modem inputs (DCD,RI,DSR,CTS) to change
                 * - mask passed in arg for lines of interest
                 *   (use |'ed TIOCM_RNG/DSR/CD/CTS for masking)
                 * Caller should use TIOCGICOUNT to see which one it was
                 *
                 * Note: CTS is always true, RI is always false.
                 * DCD and DSR always follow DTR signal from host.
                 */
        case TIOCMIWAIT:
                TRACE_MSG0(TTY, "TIOCGMIWAIT");
                while (1) {
                        u32 saved_tiocm = os_private->tiocm;
                        u32 changed_tiocm;

                        interruptible_sleep_on(&os_private->tiocm_wait);

                        /* see if a signal did it */
                        if (signal_pending(current))
                                return -ERESTARTSYS;

                        os_private->tiocm = tty_l26_tiocm(function_instance);
                        changed_tiocm = saved_tiocm ^ os_private->tiocm;
                        RETURN_ZERO_IF ( (changed_tiocm | TIOCM_CAR) | (changed_tiocm | TIOCM_DSR) );
                        return -EIO;
                }
                /* NOTREACHED */

                /*
                 * Get counter of input serial line interrupts (DCD,RI,DSR,CTS)
                 * Return: write counters to the user passed counter struct
                 * NB: both 1->0 and 0->1 transitions are counted except for
                 *     RI where only 0->1 is counted.
                 */
        case TIOCGICOUNT:
                TRACE_MSG0(TTY, "TIOCGICOUNT");
                if (copy_to_user((void *)arg, &os_private->tiocgicount, sizeof(int)))
                        return -EFAULT;
                return 0;

        case TCSETS:
                TRACE_MSG0(TTY, "TCSETS: ");
                return -ENOIOCTLCMD;
        case TCFLSH:
                TRACE_MSG0(TTY, "TCFLSH: x");
                return -ENOIOCTLCMD;

        case TCGETS:
                TRACE_MSG0(TTY, "TCGETS: ");
                return -ENOIOCTLCMD;

        default:
                TRACE_MSG1(TTY, "unknown cmd: %08x", cmd);
                return -ENOIOCTLCMD;
        }
        return -ENOIOCTLCMD;
}


/*! tty_l26_set_termios
 *
 * Used by TTY layer to set termios structure according to current status.
 *
 * @param tty - pointer to acm private data structure
 * @param termios_old - termios structure
 */
static void tty_l26_set_termios(struct tty_struct *tty, struct termios *termios_old)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        unsigned long flags;
        unsigned int c_cflag = tty->termios->c_cflag;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"tty: %x", (int)tty);
        #endif
        local_irq_save(flags);
        /* see if CLOCAL has changed */
        if ((termios_old->c_cflag ^ tty->termios->c_cflag ) & CLOCAL)
                acm_set_local(function_instance, tty->termios->c_cflag & CLOCAL);

        /* send break?  */
        if ((termios_old->c_cflag & CBAUD) && !(c_cflag & CBAUD))
                acm_bBreak(function_instance);
        local_irq_restore(flags);
}


/* The following functions has to be modified 
 */
#if 0 
/*! tty_l26_wait_until_sent
 *
 * Used by TTY layer to wait until pending data drains (is sent.)
 *
 * @param tty      - pointer to acm private data structure
 * @param timeout  - wait this amount of time, or forever if zero
 */
static void tty_l26_wait_until_sent(struct tty_struct *tty, int timeout)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];

        TRACE_MSG0(TTY, "--");

        RETURN_UNLESS(acm_send_queued(function_instance));

        /* XXX This will require a wait structure and flag */
        interruptible_sleep_on(&os_private->send_wait);
}


/*! tty_l26_flush_chars
 *
 * Used by TTY layer to flush pending data to hardware.
 *
 * @param tty - pointer to acm private data structure
 */
static void tty_l26_flush_chars(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];
        unsigned long flags;

        TRACE_MSG0(TTY, "--");

        /* queue data in overflow buffer */
        local_irq_save(flags);
        acm_force_xmit_chars(function_instance);
        local_irq_restore(flags);
}

/*! tty_l26_flush_buffer
 *
 * Used by TTY layer to flush pending data to /dev/null (cancel.)
 *
 * @param tty - pointer to acm private data structure
 */
static void tty_l26_flush_buffer(struct tty_struct *tty)
{
        int index = TTYINDEX(tty);
        struct os_private *os_private = tty->driver_data;
        struct usbd_function_instance *function_instance = function_table[index];

        TRACE_MSG0(TTY, "--");

        /* clear overflow buffer */

}

#endif

/*! tty_operations
 */
static struct tty_operations tty_ops = {

        .open =                  tty_l26_open,
        .close =                 tty_l26_close,
        .write =                 tty_l26_write,
        .write_room =            tty_l26_write_room,
        .ioctl =                 tty_l26_fd_ioctl,
        .throttle =              tty_l26_throttle,
        .unthrottle =            tty_l26_unthrottle,
        .chars_in_buffer =       tty_l26_chars_in_buffer,
        .set_termios =           tty_l26_set_termios,
        #if 0
        .wait_until_sent =       tty_l26_wait_until_sent,
        .flush_chars =           tty_l26_flush_chars,
        .flush_buffer =          tty_l26_flush_buffer,
        #endif

};

/* Transmit Function - called by tty layer ****************************************************** */

/* ******************************************************************************************* */
/* ******************************************************************************************* */
/* TTY OS Functions
 */

#define LOOP_BUF 16
/*!
 * @brief tty_l26_loop_xmit_chars
 *
 * Implement data loop back, send xmit data back as received data.
 *
 * @param function_instance 
 * @param count - number of bytes to send
 * @param buf - pointer to data to send
 * @return count - number of bytes actually sent.
 */
int tty_l26_loop_xmit_chars(struct usbd_function_instance *function_instance, 
                int count, const unsigned char *buf)
{

        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty->driver_data;
        int received = 0;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm %x", acm);
        #endif
        while (count > 0) {
                u8 copybuf[LOOP_BUF];
                int i;
                int space =  tty->ldisc.receive_room(tty);
                int recv = MIN (space, LOOP_BUF);
                recv = MIN(recv, count);

                BREAK_UNLESS(recv);

                memcpy ((void *)copybuf, (void*)(buf + received), recv);

                count -= recv;
                received += recv;
                #ifdef CONFIG_OTG_TRACE
                TRACE_MSG3(TTY, "count: %d recv: %d received: %d", count, recv, received);
                #endif
                tty_l26_os_recv_chars(function_instance, copybuf, recv);
        }
        return received;
}


static int tty_l26_setbit(u32 result, u32 mask, BOOL value, char *msg)
{
        UNLESS (value) 
                return result;

        result |= mask;
        
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG3(TTY, "result: %04x mask: %04x %s", result, mask, msg);
        #endif
        return result;
}

/*!
 * @brief tty_l26_tiocm
 * @param function_instance
 * @param tiocm
 * @return - computed tiocm value
 */
unsigned int tty_l26_tiocm(struct usbd_function_instance *function_instance)
{
        u32 tiocm = 0;
        unsigned long flags;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm %x",(int)function_instance ? function_instance->privdata : NULL );
        #endif
        local_irq_save(flags);
        tiocm = tty_l26_setbit(tiocm, TIOCM_DTR, acm_get_bRxCarrier(function_instance), "TIOCM_DTR (bRxCarrier)");
        tiocm = tty_l26_setbit(tiocm, TIOCM_RTS, !acm_get_throttled(function_instance), "TIOCM_RTS (TRUE)");
        tiocm = tty_l26_setbit(tiocm, TIOCM_CTS, acm_get_d0_dtr(function_instance), "TIOCM_CTS (TRUE)");
        tiocm = tty_l26_setbit(tiocm, TIOCM_CAR, acm_get_d1_rts(function_instance), "TIOCM_CAR (D1 RTS)"); 
        tiocm = tty_l26_setbit(tiocm, TIOCM_RI, FALSE, "TIOCM_RING");
        tiocm = tty_l26_setbit(tiocm, TIOCM_DSR, acm_get_d0_dtr(function_instance), "TIOCM_DSR (D0 DTR)"); 
        tiocm = tty_l26_setbit(tiocm, TIOCM_OUT1, FALSE, "TIOCM_OUT1 (FALSE)");
        tiocm = tty_l26_setbit(tiocm, TIOCM_OUT2, FALSE, "TIOCM_OUT2 (FALSE)");
        local_irq_restore(flags);
        return tiocm;
}


/*! tty_l26_tiocmget
 *
 *              Peripheral      Host            Internal
 * TIOCM_DTR    bRxCarrier
 * TIOCM_RTS    Always TRUE
 * TIOCM_CTS                    Always TRUE
 * TIOCM_CAR                    D1
 * TIOCM_RI                     Always FALSE
 * TIOCM_DSR                    D0
 * TIOCM_OUT1   bRingSignal
 * TIOCM_OUT2   bOverrrun
 * TIOCM_LOOP                                   Loopback
 */
static int tty_l26_tiocmget(struct tty_struct *tty, struct file *file)
{

        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY, "acm: %x", (int)acm);
        #endif
        RETURN_ZERO_UNLESS(acm);

        return tty_l26_tiocm(function_instance);

}

/*! tty_l26_tiocsget
 *              Peripheral      Host            Internal
 * TIOCM_DTR    bRxCarrier
 * TIOCM_RTS    Always TRUE
 * TIOCM_OUT1   bRingSignal
 * TIOCM_OUT2   bOverrun
 * TIOCM_LOOP                                   Loopback
 *
 */
static int tty_l26_tiocmset(struct tty_struct *tty, struct file *file,
                unsigned int set, unsigned int clear)
{
        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned long flags;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG3(TTY,"acm: %x set: %04x clear: %04x", (int)acm, set, clear);
        #endif
        RETURN_ZERO_UNLESS(acm);

        local_irq_save(flags);
        /* Set DTR -> DTR */
        if (set & TIOCM_DTR)
                acm_set_bRxCarrier(function_instance, 1);
        if (clear & TIOCM_DTR)
                acm_set_bRxCarrier(function_instance, 0);

        /* Set Loop back */
        if (set & TIOCM_LOOP) 
                acm_set_loopback(function_instance, 1);
        if (clear & TIOCM_LOOP) 
                acm_set_loopback(function_instance, 0);

        /* OUT1->Ring */
        if ((set & TIOCM_OUT1)) 
                acm_bRingSignal(function_instance);

        /* OUT2->Overrun */
        if ((set & TIOCM_OUT2)) 
                acm_bOverrun(function_instance);

        /* !RTS->Break */
        if (set & TIOCM_RTS) 
                acm_set_throttle(function_instance, 0);
        if (clear & TIOCM_RTS) 
                acm_set_throttle(function_instance, 1);
        local_irq_restore(flags);
        return 0;
}

/*! tty_l26_break_ctl
 */
static int tty_l26_break_ctl(struct tty_struct *tty, int flag)
{
        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        unsigned long flags;
        RETURN_ZERO_UNLESS(acm);
        local_irq_save(flags);
        if (flag)
                acm_bBreak(function_instance);
        local_irq_restore(flags);
        return 0;
}

/* ********************************************************************************************** */
/* ********************************************************************************************* */

/*! 
 * @brief tty_l26_schedule
 *
 * Schedule a bottom half handler work item.
 * @param tty
 * @param queue
 * @return none
 */
static void tty_l26_schedule(struct tty_struct *tty, WORK_ITEM *queue)
{ 
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"tty: %x", tty);
        #endif
        SET_WORK_ARG(*queue, tty);
        RETURN_IF(NO_WORK_DATA((*queue)) || PENDING_WORK_ITEM((*queue)));
        SCHEDULE_WORK((*queue));
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"task %p scheduled", queue); 
        #endif
}


/*!
 * @brief tty_l26_call_hangup
 *
 * Bottom half handler to safely send hangup signal to process group.
 *
 * @param data
 * @return none
 */
void tty_l26_call_hangup(void *data)
{
        struct tty_struct *tty = data;
        struct os_private *os_private = tty ? tty->driver_data : NULL;

        /* XXX Do we need to protect this
         */
        RETURN_UNLESS(tty && os_private);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG3(TTY,"tty: %x c_cflag: %02x CLOCAL: %d", tty, 
                        tty->termios->c_cflag, 
                        BOOLEAN(tty->termios->c_cflag & CLOCAL)  
                  );
        #endif

        //if (tty && !(os_private->c_cflag & CLOCAL))
        tty_hangup(tty);

        wake_up_interruptible(&os_private->open_wait);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"exited"); 
        #endif
}

/*!
 * @brief tty_l26_os_schedule_hangup
 *
 * Schedule hangup bottom half handler.
 *
 * @param function_instance
 * @return none

 */
void tty_l26_os_schedule_hangup(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        RETURN_UNLESS(os_private);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "entered");
        #endif
        tty_l26_schedule(tty, &os_private->hqueue);
}


/*! tty_l26_wakeup_writers
 *
 * Bottom half handler to wakeup pending writers.
 *
 * @param data - pointer to acm private data structure
 */
void tty_l26_wakeup_writers(void *data)
{
        struct tty_struct *tty = data;
        struct os_private *os_private = tty ? tty->driver_data : NULL;
        int index = os_private->index;
        struct usbd_function_instance *function_instance = function_table[index];


        unsigned long flags;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"tty: %x", tty);
        #endif
        RETURN_UNLESS(os_private);

        RETURN_UNLESS(acm_ready(function_instance));
        RETURN_UNLESS(atomic_read(&os_private->used));
        RETURN_UNLESS(tty);

        if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) && tty->ldisc.write_wakeup)
                (tty->ldisc.write_wakeup)(tty);

        wake_up_interruptible(&tty->write_wait);
}

/*!
 * @brief tty_l26_os_wakeup_opens
 *
 * Called by acm_ to wakeup processes blocked in open waiting for DTR.
 *
 * @param function_instance
 * @return none
 */
void tty_l26_os_wakeup_opens(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", acm);
        #endif
        RETURN_UNLESS(os_private);

        wake_up_interruptible(&os_private->open_wait);
}

/*!
 * @brief  tty_l26_os_schedule_wakeup_writers
 *
 * Called by acm to schedule a wakeup writes bottom half handler.
 *
 * @param function_instance - pointer to acm private data structure
 * @return none
 */
void tty_l26_os_schedule_wakeup_writers(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        /* XXX Do we need to protect this
         */
        RETURN_UNLESS(os_private);

        tty_l26_schedule(tty, &os_private->wqueue);

        // verify ok
        wake_up_interruptible(&os_private->send_wait);
}

/*! 
 * @brief tty_l26_os_wakeup_state
 *
 * Called by acm_fd to wakeup processes blocked waiting for state change
 *
 * @param function_instance - pointer to acm private data structure
 * @return none
 */
void tty_l26_os_wakeup_state(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        /* XXX Do we need to protect this
         */
        RETURN_UNLESS(os_private);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "entered");
        #endif
        wake_up_interruptible(&os_private->tiocm_wait);
}


/*! 
 * @brief tty_l26_os_recv_chars
 *
 * Called by acm when data has been received. This will
 * receive n bytes starting at cp. 
 *
 * @param function_instance
 * @param n - number of bytes to send
 * @param cp - pointer to data to send
 * @return bytes written or -1 if error
 */
int tty_l26_os_recv_chars(struct usbd_function_instance *function_instance, u8 *cp, int n)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        int ldisc_room=0;
        RETURN_EINVAL_UNLESS(tty);

	/* Flip it on to the line discipline */
        if (!test_bit(TTY_THROTTLED, &tty->flags)) {
                /* try to write as much to ldisc as possible */ 
                ldisc_room = MIN(n, tty->ldisc.receive_room(tty)); 

                #ifdef CONFIG_OTG_TRACE
                {
                int i;
                TRACE_MSG1(TTY,"ldisc_room = %d",ldisc_room);

                for (i = 0; i < n; i += 8) {
                        TRACE_MSG8(TTY, "sos909 %02x %02x %02x %02x %02x %02x %02x %02x",
                                        cp[i + 0], cp[i + 1], cp[i + 2], cp[i + 3],
                                        cp[i + 4], cp[i + 5], cp[i + 6], cp[i + 7]
                                  );
                }
                }
                #endif

                tty->ldisc.receive_buf(tty, cp, NULL, ldisc_room);
                return ldisc_room;
        }

        return -1;

}

/* ********************************************************************************************* */

/*!
 * @brief tty_l26_recv_start
 *
 * Bottom half handler to restart acm recv queue...
 *
 * This is implemented here because the background scheduling may be OS
 * dependant.. but it must call the common acm_start_recv_urbs() function to
 * do the work.
 *
 * @param data
 * @return none
 */
void tty_l26_recv_start(void *data)
{
        struct tty_struct *tty = data;
        int index = TTYINDEX(tty);
        struct usbd_function_instance *function_instance = function_table[index];
        unsigned long flags;

        RETURN_UNLESS(function_instance);

        local_irq_save(flags);
        acm_start_recv_urbs(function_instance);
        local_irq_restore(flags);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"exited");
        #endif
}

/*!
 * @brief tty_l26_recv_start_bh
 *
 * Schedule hangup bottom half handler.
 *
 * @param function_instance
 * @return none

 */
void tty_l26_recv_start_bh(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
        RETURN_UNLESS(os_private);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "entered");
        #endif
        tty_l26_schedule(tty, &os_private->rqueue);
}

void tty_l26_os_line_coding(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        struct tty_struct *tty = tty_table[index];
        struct os_private *os_private = tty ? tty->driver_data : tty;
        struct cdc_acm_line_coding *line_coding = &acm->line_coding;

        RETURN_UNLESS(os_private);

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY, "entered");
        #endif
        os_private->serial_struct.custom_divisor = 1;
        os_private->serial_struct.baud_base = line_coding->dwDTERate;
        os_private->serial_struct.flags = line_coding->bCharFormat << 16 |
                line_coding->bParityType << 8 | line_coding->bDataBits;
}

/*!
 * @brief  tty_l26_os_tiocgicount
 *
 * Called by tty when they want to know the number of interrupts.
 * @param function_instance
 * @return void
 */
void tty_l26_os_tiocgicount(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index;
        struct tty_struct *tty;
        struct os_private *os_private;
        
        index = acm->index;
        tty = tty_table[index];
        os_private = tty ? tty->driver_data : tty;
 
        if(os_private) {
                os_private->tiocgicount++;
        }
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", (int)acm);
        #endif
}

/* ********************************************************************************************* */
/*!
 * @brief  tty_l26_os_enable
 *
 * Called by acm when the function driver is enabled.
 * @param function_instance
 * @return int
 */
int tty_l26_os_enable(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = 0;
        struct tty_struct *tty;
        struct os_private *os_private;

        if (function_table[index]) {
                TRACE_MSG0(TTY,"func table already exists ");
                return 0;
        }
        function_table[index] = function_instance;
        acm->index = index;
        tty = tty_table[index];
        os_private = tty ? tty->driver_data : NULL;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY, "OS Flags: %x", os_private ? os_private->flags : 0);
        #endif

        RETURN_ZERO_UNLESS(os_private && (os_private->flags & TTYFD_OPENED));
        atomic_or_set(&acm->flags, ACM_OPENED);
        tty_l26_os_schedule_wakeup_writers(function_instance);

        return 0;
}

/*!
 * @brief tty_l26_os_disable
 * @param function_instance
 * @return none
 *
 * Called by acm when the function driver is disabled.
 */

void tty_l26_os_disable(struct usbd_function_instance *function_instance)
{
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int index = acm->index;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG1(TTY,"acm: %x", acm);
        #endif
        function_table[index] = NULL;
        // XXX MODULE UNLOCK HERE
}

struct acm_os_ops tty_l26_os_ops = {
        .enable = tty_l26_os_enable,
        .disable = tty_l26_os_disable,
        .wakeup_opens = tty_l26_os_wakeup_opens,
        .wakeup_state = tty_l26_os_wakeup_state,
        .schedule_wakeup_writers = tty_l26_os_schedule_wakeup_writers,
        .recv_chars = tty_l26_os_recv_chars,
        .schedule_hangup = tty_l26_os_schedule_hangup,
        .recv_start_bh = tty_l26_recv_start_bh,
        .line_coding = tty_l26_os_line_coding,
        .tiocgicount = tty_l26_os_tiocgicount,
};

/* ************************************************************************** */
/* ************************************************************************** */

/*! tty_driver
 */
static struct tty_driver *tty_driver;


/* USB Module init/exit ************************************************************************ */

/*! 
 * @brief tty_l26_init - module init
 *
 * This is called immediately after the module is loaded or during
 * the kernel driver initialization if linked into the kernel.
 * @param name
 * @param minor_num
 * @return int
 */

int tty_l26_init (char *name, int minor_num)
{
        int i;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG2(TTY, "name: %s minor_num: %d", name, minor_num);
        #endif

        THROW_UNLESS (tty_table = (struct tty_struct **) 
                        CKMALLOC(sizeof(struct tty_struct *)* minor_num, GFP_KERNEL ), error) ;

        THROW_UNLESS (function_table = (struct usbd_function_instance **)
                        CKMALLOC(sizeof(struct usbd_function_instance *) * minor_num, GFP_KERNEL), error) ;

        tty_minor_count = minor_num;

        THROW_UNLESS((tty_driver = alloc_tty_driver(minor_num)), error);

        /* update init_termios and register as tty driver 
         */
        tty_driver->owner = THIS_MODULE;
        tty_driver->driver_name = name;
        tty_driver->name = ACM_DRIVER_DEVFS_NAME;
        tty_driver->devfs_name = ACM_DEVFS_NAME;
        tty_driver->major = ACM_TTY_MAJOR;
        tty_driver->minor_start = ACM_TTY_MINOR_START;
        tty_driver->minor_num = minor_num;
        tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
        tty_driver->subtype = SERIAL_TYPE_NORMAL;
        tty_driver->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_NO_DEVFS;
        tty_driver->init_termios = tty_std_termios;
        tty_driver->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;

        tty_set_operations(tty_driver, &tty_ops);
        tty_driver->tiocmget= tty_l26_tiocmget;
        tty_driver->tiocmset= tty_l26_tiocmset;
        THROW_IF(tty_register_driver(tty_driver), error);

        for (i = 0; i < tty_minor_count; i++) {
                tty_register_device(tty_driver, i, NULL);
        }
        #ifdef CONFIG_OTG_ACM_CONSOLE
        acm_serial_console_init();
        #endif

        CATCH(error) {
                printk(KERN_ERR"%s: ERROR\n", __FUNCTION__);
                if(tty_driver) put_tty_driver(tty_driver);
                if (tty_table) LKFREE(tty_table);
                if (function_table) LKFREE(function_table);
                tty_driver=NULL;
                tty_table=NULL;
                function_table=NULL;
                return -EINVAL;
        }
        return 0;
}


/*!
 *@brief tty_l26_exit - module cleanup
 *
 * This is called prior to the module being unloaded.
 */
void tty_l26_exit (void)
{
        int i;
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"tty_l26_exit!");
        #endif
        #ifdef CONFIG_OTG_ACM_CONSOLE
        acm_serial_console_exit();
        #endif
        for (i = 0; i < tty_minor_count; i++)
                tty_unregister_device(tty_driver, i);

        tty_unregister_driver(tty_driver);
        put_tty_driver(tty_driver);
        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"after tty_unregister_driver!\n");
        #endif
        if (tty_table) LKFREE(tty_table);
        if (function_table) LKFREE(function_table);
        tty_driver=NULL;
        tty_table=NULL;
        function_table=NULL;

        #ifdef CONFIG_OTG_TRACE
        TRACE_MSG0(TTY,"tty_l26_exit");
        #endif
}

#ifdef CONFIG_OTG_ACM_CONSOLE
static struct tty_driver *acm_console_device(struct console *co, int *index)
{
        *index = co->index;
        return tty_driver;
}


static void acm_console_write(struct console *co, const char *buf, unsigned count)
{
        int index = co->index;
        struct usbd_function_instance *function_instance = function_table[index];
        struct acm_private *acm = function_instance ? function_instance->privdata : NULL;
        int total_sent_chars= 0;
        unsigned long flags;

        RETURN_UNLESS(acm && count);
        local_irq_save(flags);
        total_sent_chars += acm_try_xmit_chars(function_instance, count, buf);
        local_irq_restore(flags);
        /* could not write everything asked for */
        if(unlikely(total_sent_chars < count)) {
                atomic_set(&acm->wakeup_writers,1);
        }
}

static struct console acmcons = {
        .name  ="acm",
        .write =acm_console_write,
        .flags =CON_PRINTBUFFER,
        .device=acm_console_device,
        .index =-1,
};

static void acm_serial_console_init ()
{
        TRACE_MSG0(TTY,"console init");
        register_console(&acmcons);
}


static void acm_serial_console_exit (void)
{
        TRACE_MSG0(TTY,"console exit");
        unregister_console(&acmcons);
}
#endif

