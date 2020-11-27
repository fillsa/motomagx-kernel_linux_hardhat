/*
 * otg/functions/acm/tty.h
 * @(#) sl@belcarra.com|otg/functions/acm/tty.h|20060403224909|42511
 *
 *      Copyright (c) 2003-2005 Belcarra
 *	Copyright (c) 2005-2006 Belcarra Technologies 2005 Corp
 *
 * By:
 *      Stuart Lynne <sl@belcarra.com>,
 *      Bruce Balden <balden@belcarra.com>
 *
 *      Copyright (c) 2006 Motorola, Inc. 
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 01/27/2006         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language 
 * 12/11/2006         Motorola         Changes for Open src compliance.
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
 * @defgroup TTYFunction ACM-TTY
 * @ingroup functiongroup
 */
/*!
 * @file otg/functions/acm/tty.h
 * @brief ACM Function Driver private defines
 *
 *
 * This is an ACM Function Driver. The upper edge is exposed
 * to the hosting OS as a Posix type character device. The lower
 * edge implements the USB Device Stack API.
 *
 * These are emulated and set by the tty driver as appropriate
 * to model a virutal serial port. The
 *
 *      TIOCM_RNG       RNG (Ring)                              not used
 *      TIOCM_LE        DSR (Data Set Ready / Line Enable)
 *      TIOCM_DSR       DSR (Data Set Ready)
 *      TIOCM_CAR       DCD (Data Carrier Detect)
 *      TIOCM_CTS       CTS (Clear to Send)
 *      TIOCM_CD        TIOCM_CAR
 *      TIOCM_RI        TIOCM_RNG
 *
 * These are set by the application:
 *
 *      TIOCM_DTR       DTR (Data Terminal Ready)
 *      TIOCM_RTS       RTS (Request to Send)
 *
 *      TIOCM_LOOP      Set into loopback mode
 *      TIOCM_OUT1      Not used.
 *      TIOCM_OUT2      Not used.
 *
 *
 * The following File and  termio c_cflags are used:
 *
 *      O_NONBLOCK      don't block in tty_open()
 *      O_EXCL          don't allow more than one open
 *
 *      CLOCAL          ignore DTR status
 *      CBAUD           send break if set to B0
 *
 * Virtual NULL Modem
 *
 * Peripheral to Host via Serial State Notification (write ioctls)
 *
 * Application             TIOCM           Null Modem      ACM (DCE)       Notificaiton    Host (DTE)
 * -------------------------------------------------------------------------------------------------------------
 * Request to send         TIOCM_RTS       RTS ->  CTS     CTS             Not Available   Clear to Send (N/A)
 * Data Terminal Ready     TIOCM_DTR       DTR ->  DSR     DSR             bTxCarrier      Data Set Ready
 *                                         DTR ->  DCD     DCD             bRXCarrier      Carrier Detect
 * Ring Indicator          TIOCM_OUT1      OUT1 -> RI      RI              bRingSignal     Ring Indicator
 * Overrun                 TIOCM_OUT2      OUT2 -> Overrun Overrun         bOverrun        Overrun
 * Send Break              B0              B0   -> Break   Break           bBreak          Break Received
 * -------------------------------------------------------------------------------------------------------------
 *
 *
 * Host to Peripheral via Set Control Line State
 *
 * Host (DTE)              Line State      ACM (DCE)       Null Modem      TIOCM           Peripheral (DTE)
 * -------------------------------------------------------------------------------------------------------------
 * Data Terminal Ready     D0              DTR             DTR ->  DSR     TIOCM_DSR       Data Set Ready
 *                                                         DTR ->  DCD     TIOCM_CAR       Carrier Detect
 * Request To Send         D1              RTS             RTS ->  CTS     TIOCM_CTS       Clear to Send
 * -------------------------------------------------------------------------------------------------------------
 *
 *
 * @ingroup TTYFunction
 */

extern struct usbd_function_driver tty_function_driver;

#define TTYFD_OPENED      (1 << 0)                /*! OPENED flag */
#define TTYFD_CLOCAL      (1 << 1)                /*! CLOCAL flag */
//#define TTYFD_LOOPBACK    (1 << 2)               /*! LOOPBACK flag */
#define TTYFD_EXCLUSIVE   (1 << 3)                /*! EXCLUSIVE flag */
//#define TTYFD_THROTTLED (1 << 4)                /*! THROTTLED flag */

#define TTYINDEX(tty)     tty->index


/*! @struct tty_private
 */
struct os_private {
        int index;

        WORK_STRUCT wqueue;                     /*!< task queue for writer wakeup  */
        WORK_STRUCT hqueue;                     /*!< task queue for hangup */

        u32 flags;                              /*!< flags */

        u32 tiocm;                              /*!< tiocm settings */

        struct serial_struct serial_struct;     /*!< serial structure used for TIOCSSERIAL and TIOCGSERIAL */

        wait_queue_head_t tiocm_wait;
        u32 tiocgicount;

        wait_queue_head_t open_wait;            /*! wait queue for blocking open*/
        int exiting;                            /*! True if module exiting */

        wait_queue_head_t send_wait;            /*! wait queue for blocking open*/
        int sending;

        WORK_STRUCT rqueue;                     /*!< task queue for hangup */

        atomic_t used;
        int minor_num;

};

int tty_l26_init(char *, int);
void tty_l26_exit(void);

int tty_if_init(void);
void tty_if_exit(void);

