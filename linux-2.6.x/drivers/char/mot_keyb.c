/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2005-2008 Motorola, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2008-Apr-21 - Adding autorepeat for EMU headseet
 * Motorola 2008-Oct-22 - Disable the sending of simultaneous press key event of volume up/down key 
 *                        to application layer while nevis slider is closed. 
 * Motorola 2008-Mar-28 - Send release for unmapped keys
 * Motorola 2008-Mar-18 - Update Nevis keypad matrix
 * Motorola 2008-Mar-12 - Changed autorepeat from capacitive touch keycodes to headset keycodes
 * Motorola 2008-Mar-11 - Added handling of MORPHING_MODE_PHONE_WITHOUT_REVIEW.
 * Motorola 2008-Feb-27 - Add OPENING/CLOSING transition for sliders and flips
 * Motorola 2008-Feb-06 - Add keypad support for Nevis
 * Motorola 2008-Jan-30 - Added xPIXL keymap and keylock, lens cover sidekeys.
 * Motorola 2007-Sep-04 - Conditional power key filter for flip only
 * Motorola 2007-Jun-27 - Dont allow power key events if flip is closed
 * Motorola 2007-May-16 - increase delay in scan_columns
 * Motorola 2007-May-01 - Remove events that are the same as previously added to the queue
 * Motorola 2007-Mar-01 - Disable keys under flip
 * Motorola 2007-Feb-13 - Add suspend/resume for slider and flip
 * Motorola 2007-Jan-10 - Update status of flip/slider on power up
 * Motorola 2006-Nov-30 - Support key lock feature.
 * Motorola 2006-Nov-17 - Add key_map to support Kassos
 * Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
 * Motorola 2006-Oct-20 - Fix compile issue for none flip/slider phone
 * Motorola 2006-Oct-11 - Change keycodes for KeyV
 * Motorola 2006-Sep-20 - Add autorepeat for capacitive touch keycodes
 * Motorola 2006-Sep-19 - Clean up defects in driver
 * Motorola 2006-Sep-06 - Fix keycode bound check
 * Motorola 2006-Sep-05 - Remove first slider event
 * Motorola 2005-Jul-17 - Initial Creation
 */

/*!
 * @file mot_keyb.c
 *
 * @brief Driver for Motorola keypad, derived from mxc_keyb.c.
 * 
 * The keypad driver is designed as a character driver which interacts with 
 * low level keypad port hardware. Upon opening, the Keypad driver initializes 
 * the keypad port. When the keypad interrupt happens the driver scans the 
 * keypad matrix for key press/release.  The scancode of the key press/release 
 * events are internally queued. When an application does a read on the keypad
 * driver, the scan codes from the keypad queue are copied to user. The user
 * can configure the keypad driver in raw mode or map mode. The user can also
 * configure the keypad matrix for upto 8 rows and 8 columns. 
 *
 * @ingroup keypad 
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/boardrev.h>
#include <asm/arch/hardware.h>
#include <asm/arch/board.h>
#include <asm/arch/gpio.h>
#include <asm/irq.h>
#include <asm/mot-gpio.h>
#include <linux/kd.h>
#include <linux/fs.h>
#include <linux/kbd_kern.h>
#include <linux/ioctl.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/keypad.h>
#include <linux/device.h>
#include <linux/mpm.h>
#include <linux/morphing_mode.h>

/* Module header file */
#include "mot_keyb.h"

/*! Number of keypad rows and columns */
 
/* SCM-A11 Family */
#if defined(CONFIG_ARCH_MXC91231)
#define ROWS 5
static short int COLS = 6;
/* Argon Family */
#elif defined(CONFIG_ARCH_MXC91321)
#define ROWS 8
static short int COLS = 6;
#else
#error "Unknown architecture!"
#endif

/*! Auto repeat timer for headset */
struct timer_list headset_timer;
/*! Auto repeat timer for emu headset */
struct timer_list emu_headset_timer;

/*! Keep track of the scancode for the timer */
static unsigned short headset_scancode;
/*! Keep track of the scancode for the timer */
static unsigned short emu_headset_scancode;

/*! This structure holds the keypad Queue for storing the scancode for key press/release. */ 
static struct keypad_priv kpp_dev;

/*! This pointer identifies the start of the list of registered callbacks */
static keyeventcb* keyeventcb_chain = NULL;

/*! This pointer points to an array of autorepeat_info structures -- one for each key */
static autorepeat_info* autorepeat_key_info;

/*! An index into the autorepeat_key_info array locating the first key pressed */
static unsigned char start_autorepeat_key_info;

/*! An index into the autorepeat_key_info array locating the last key pressed */
static unsigned char end_autorepeat_key_info;

/*! A lock used to keep autorepeat info in synch */
static spinlock_t autorepeat_lock = SPIN_LOCK_UNLOCKED;

/*!
 * The maximum index into the autorepeat_key_info array. This value is also considered
 * an invalid index into the autorepeat_key_info array.
 */
static unsigned char max_autorepeat_key_info;

/*! For doing spin locks */
static spinlock_t keypad_lock = SPIN_LOCK_UNLOCKED;
static spinlock_t keyeventcb_lock = SPIN_LOCK_UNLOCKED;

/*! Number of time /dev/keypad0 is open */
static int reading_opens = 0;

/*! Number of time /dev/keypadB is open */
static int bitmap_opens = 0;

/*! current bitmap of keys down */
static unsigned long keybitmap[NUM_WORDS_IN_BITMAP];

/*! last bitmap read by ioctl */
static unsigned long lastioctlbitmap[NUM_WORDS_IN_BITMAP]; 

/*! Current state of the keys and how many are down */
static unsigned short cur_rcmap[ROWS];
static int keycnt;
/*! Previous state of the keys and how many were down */
static unsigned short prev_rcmap[ROWS];
static int prev_keycnt;

/*! Minor number for /dev/keypadB used to decide whether to set lastioctlbitmap in GETBITMAP ioctl */
int Bminor;

#ifdef CONFIG_PM
/*!
 * Keep track of whether or not a key is currently held down so we can
 * reject any attempts of sleeping if necessary.
 */
static unsigned int reject_suspend = 0;
#endif

/*! This variable indicates whether keypad driver is in suspend or in resume mode. */
static unsigned short suspend = 0;

/*! This static variable indicates whether a key event is pressed/released. */
static unsigned short press = 0;

/*! This static variable indicates whether the flip is opened, opening, closing, or closed. */
static STATUS_E flip_state = -1;

/*! This static variable indicates whether the slider is opened, opening, closing, or closed. */
static STATUS_E slider_state = -1;

/*! This says whether the keypad is stable or being debounced. */
static KEYPAD_STATE_E keystate = STABLE;

/*! States which rows should not currently be scanned */
static unsigned long invalid_rows = 0;

/*! States which columns should not currently be scanned */
static unsigned long invalid_columns = 0;

/*! This is how many msecs we wait while polling for key release */
/*  We're just using the number from the freescale keypad driver */
#define RELEASE_POLL_DELAY         10

/*!
 *  Mapcode array contains the scan code to map code mapping. This is used
 *  when the keypad mode is XLATE. 
 */
/* All other SCM-A11 */
#if defined(CONFIG_ARCH_MXC91231)

#if defined(CONFIG_MACH_SCMA11REF)
static unsigned short *key_map;

static unsigned short key_map_P0[] = {
        /* row 0 */
    KEYPAD_6,          KEYPAD_NAV_RIGHT,
    KEYPAD_SPARE1,     KEYPAD_NAV_CENTER,
	KEYPAD_5, KEYPAD_CARRIER,
        /* row 1 */
    KEYPAD_8,          KEYPAD_NAV_DOWN,
    KEYPAD_SPARE2,     KEYPAD_SLEFT,
	KEYPAD_STAR, KEYPAD_MESSAGING,
        /* row 2 */
    KEYPAD_7,          KEYPAD_9,
    KEYPAD_VA,         KEYPAD_SRIGHT,
	KEYPAD_CLEAR_BACK, KEYPAD_SPARE3,
        /* row 3 */
    KEYPAD_2,          KEYPAD_NAV_UP,
    KEYPAD_SEND,       KEYPAD_0,
	KEYPAD_1, KEYPAD_SPARE4,
        /* row 4 */
    KEYPAD_4,          KEYPAD_NAV_LEFT,
    KEYPAD_SPARE5,     KEYPAD_POUND,
	KEYPAD_3, KEYPAD_SPARE6,
};

static unsigned short key_map_P1_P2[] = {
        /* row 0 */
    KEYPAD_4,          KEYPAD_NAV_LEFT,
    KEYPAD_3,          KEYPAD_NAV_CENTER,
	KEYPAD_MESSAGING, KEYPAD_NONE,
        /* row 1 */
    KEYPAD_8,          KEYPAD_NAV_DOWN,
    KEYPAD_1,          KEYPAD_SOFT_RIGHT,
	KEYPAD_CLEAR_BACK, KEYPAD_CLEAR_BACK,
        /* row 2 */
    KEYPAD_9,          KEYPAD_7,
    KEYPAD_5,          KEYPAD_POUND,
	KEYPAD_STAR, KEYPAD_0,
        /* row 3 */
    KEYPAD_2,          KEYPAD_NAV_UP,
    KEYPAD_PTT,        KEYPAD_SEND,
    KEYPAD_IMAGING,    KEYPAD_SMART,
    /* row 4 */
    KEYPAD_6,          KEYPAD_NAV_RIGHT,
    KEYPAD_SIDE_UP,    KEYPAD_SOFT_LEFT,
	KEYPAD_SIDE_DOWN, KEYPAD_VA,
};

/* For the P2 Wingboard */
static unsigned short key_map_P2W[] = {
        /* row 0 */
    KEYPAD_6,           KEYPAD_NAV_RIGHT,
    KEYPAD_1,           KEYPAD_NAV_CENTER,
    KEYPAD_CARRIER,     KEYPAD_NONE,
    /* row 1 */
    KEYPAD_8,           KEYPAD_NAV_DOWN,
    KEYPAD_3,           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,  KEYPAD_MESSAGING,
    /* row 2 */
    KEYPAD_7,           KEYPAD_9,
    KEYPAD_5,           KEYPAD_STAR,
    KEYPAD_POUND,       KEYPAD_0,
    /* row 3 */
    KEYPAD_2,           KEYPAD_NAV_UP,
    KEYPAD_PTT,         KEYPAD_SEND,
    KEYPAD_IMAGING,     KEYPAD_SMART,
    /* row 4 */
    KEYPAD_4,           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,     KEYPAD_SOFT_RIGHT,
        KEYPAD_SIDE_DOWN, KEYPAD_VA,
};

#elif defined(CONFIG_MACH_SAIPAN)
static unsigned short key_map[] = {
    /* row 0 */
    KEYPAD_6, KEYPAD_NAV_RIGHT, KEYPAD_1, KEYPAD_NAV_CENTER,
    KEYPAD_CARRIER, KEYPAD_NONE,
    /* row 1 */
    KEYPAD_8, KEYPAD_NAV_DOWN, KEYPAD_3, KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK, KEYPAD_NONE,
    /* row 2 */
    KEYPAD_7, KEYPAD_9, KEYPAD_5, KEYPAD_STAR,
    KEYPAD_POUND, KEYPAD_0,
    /* row 3 */
    KEYPAD_2, KEYPAD_NAV_UP, KEYPAD_NONE, KEYPAD_SEND,
    KEYPAD_IMAGING, KEYPAD_NONE,
    /* row 4 */
    KEYPAD_4, KEYPAD_NAV_LEFT, KEYPAD_SIDE_UP, KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN, KEYPAD_VA,
};

#elif defined(CONFIG_MACH_ELBA)
static unsigned short key_map[] = {
        /* row 0 */
    KEYPAD_6, KEYPAD_NAV_RIGHT, KEYPAD_1, KEYPAD_NAV_CENTER,
    KEYPAD_CARRIER, KEYPAD_CAPTURE_PLAYBACK_TOGGLE,
    /* row 1 */
    KEYPAD_8, KEYPAD_NAV_DOWN, KEYPAD_3, KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK, KEYPAD_STILL_VIDEO_TOGGLE,
    /* row 2 */
    KEYPAD_7, KEYPAD_9, KEYPAD_5, KEYPAD_STAR,
    KEYPAD_POUND, KEYPAD_0,
    /* row 3 */
    KEYPAD_2, KEYPAD_NAV_UP, KEYPAD_HANGUP, KEYPAD_SEND,
    KEYPAD_ZOOM_IN, KEYPAD_ZOOM_OUT,
    /* row 4 */
    KEYPAD_4, KEYPAD_NAV_LEFT, KEYPAD_SIDE_UP, KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN, KEYPAD_VA,
};

#elif defined(CONFIG_MACH_NEVIS)
static unsigned short key_map[] = {
        /* row 0 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_6,          KEYFLAG_SLIDER_DISABLED|KEYPAD_NAV_RIGHT, 
    KEYFLAG_SLIDER_DISABLED|KEYPAD_1,          KEYFLAG_SLIDER_DISABLED|KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                               KEYPAD_NONE,
    /* row 1 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_8,          KEYFLAG_SLIDER_DISABLED|KEYPAD_NAV_DOWN, 
    KEYFLAG_SLIDER_DISABLED|KEYPAD_3,          KEYFLAG_SLIDER_DISABLED|KEYPAD_SOFT_LEFT,
    KEYFLAG_SLIDER_DISABLED|KEYPAD_CLEAR_BACK, KEYPAD_NONE,
    /* row 2 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_7,          KEYFLAG_SLIDER_DISABLED|KEYPAD_9, 
    KEYFLAG_SLIDER_DISABLED|KEYPAD_5,          KEYFLAG_SLIDER_DISABLED|KEYPAD_STAR,
    KEYFLAG_SLIDER_DISABLED|KEYPAD_POUND,      KEYFLAG_SLIDER_DISABLED|KEYPAD_0,
    /* row 3 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_2,          KEYFLAG_SLIDER_DISABLED|KEYPAD_NAV_UP, 
    KEYPAD_NONE,                               KEYFLAG_SLIDER_DISABLED|KEYPAD_SEND,
    KEYPAD_NONE,                               KEYPAD_NONE,
    /* row 4 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_4,          KEYFLAG_SLIDER_DISABLED|KEYPAD_NAV_LEFT, 
    KEYPAD_SIDE_UP,                            KEYFLAG_SLIDER_DISABLED|KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                          KEYPAD_NONE,
};


#elif defined(CONFIG_MACH_XPIXL)
/* The xPIXL morphing feature is mostly camera-related.  The keys which
 * distinguish camera modes from each other are toggle, share, and trash.
 */
static const unsigned short keymap_all[] = {
   /* row 0 */
   KEYPAD_6,                              KEYPAD_NAV_RIGHT,
   KEYPAD_1,                              KEYPAD_NAV_CENTER,
   KEYPAD_REVIEW,                         KEYPAD_TRASH,
   /* row 1 */
   KEYPAD_8,                              KEYPAD_NAV_DOWN,
   KEYPAD_3,                              KEYPAD_SOFT_LEFT,
   KEYPAD_CLEAR_BACK,                     KEYPAD_CAPTURE_PLAYBACK_TOGGLE,
   /* row 2 */
   KEYPAD_7,                              KEYPAD_9,
   KEYPAD_5,                              KEYPAD_STAR,
   KEYPAD_POUND,                          KEYPAD_0,
   /* row 3 */
   KEYPAD_2,                              KEYPAD_NAV_UP,
   KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
   KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
   /* row 4 */
   KEYPAD_4,                              KEYPAD_NAV_LEFT,
   KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
   KEYPAD_SIDE_DOWN,                      KEYPAD_SHARE
};

static const unsigned short keymap_none[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
};

static const unsigned short keymap_phone[] = {
    /* row 0 */
    KEYPAD_6,                              KEYPAD_NAV_RIGHT,
    KEYPAD_1,                              KEYPAD_NAV_CENTER,
    KEYPAD_REVIEW,                         KEYPAD_NONE,
    /* row 1 */
    KEYPAD_8,                              KEYPAD_NAV_DOWN,
    KEYPAD_3,                              KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_NONE,
    /* row 2 */
    KEYPAD_7,                              KEYPAD_9,
    KEYPAD_5,                              KEYPAD_STAR,
    KEYPAD_POUND,                          KEYPAD_0,
    /* row 3 */
    KEYPAD_2,                              KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_4,                              KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_NONE,
};

static const unsigned short keymap_phone_noreview[] = {
    /* row 0 */
    KEYPAD_6,                              KEYPAD_NAV_RIGHT,
    KEYPAD_1,                              KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 1 */
    KEYPAD_8,                              KEYPAD_NAV_DOWN,
    KEYPAD_3,                              KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_NONE,
    /* row 2 */
    KEYPAD_7,                              KEYPAD_9,
    KEYPAD_5,                              KEYPAD_STAR,
    KEYPAD_POUND,                          KEYPAD_0,
    /* row 3 */
    KEYPAD_2,                              KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_4,                              KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_NONE,
};
static const unsigned short keymap_cam_all[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_TRASH,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_CAPTURE_PLAYBACK_TOGGLE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_SHARE,
};

static const unsigned short keymap_cam_nav_only[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_NONE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_NONE,
};

static const unsigned short keymap_cam_share[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_NONE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_SHARE,
};

static const unsigned short keymap_cam_trash[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_TRASH,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_NONE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_NONE,
};

static const unsigned short keymap_cam_toggle[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_CAPTURE_PLAYBACK_TOGGLE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_NONE,
};

static const unsigned short keymap_cam_noshare[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_TRASH,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_CAPTURE_PLAYBACK_TOGGLE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_NONE,
};

static const unsigned short keymap_cam_notrash[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_CAPTURE_PLAYBACK_TOGGLE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_SHARE,
};

static const unsigned short keymap_cam_notoggle[] = {
    /* row 0 */
    KEYPAD_NONE,                           KEYPAD_NAV_RIGHT,
    KEYPAD_NONE,                           KEYPAD_NAV_CENTER,
    KEYPAD_NONE,                           KEYPAD_TRASH,
    /* row 1 */
    KEYPAD_NONE,                           KEYPAD_NAV_DOWN,
    KEYPAD_NONE,                           KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK,                     KEYPAD_NONE,
    /* row 2 */
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    KEYPAD_NONE,                           KEYPAD_NONE,
    /* row 3 */
    KEYPAD_NONE,                           KEYPAD_NAV_UP,
    KEYPAD_CAMERA_FOCUS,                   KEYPAD_SEND,
    KEYPAD_CAMERA_TAKE_PIC,                KEYPAD_NONE,
    /* row 4 */
    KEYPAD_NONE,                           KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                        KEYPAD_SOFT_RIGHT,
    KEYPAD_SIDE_DOWN,                      KEYPAD_SHARE,
};

static const unsigned short *key_map;

static void set_keymap(MORPHING_MODE_E mode);
#else /* xPIXL */

static unsigned short key_map[] = {
        /* row 0 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_6,     KEYPAD_NAV_RIGHT,
    KEYFLAG_SLIDER_DISABLED|KEYPAD_1,     KEYPAD_NAV_CENTER,
    KEYPAD_CARRIER,                       KEYPAD_NONE,
        /* row 1 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_8,     KEYPAD_NAV_DOWN,
    KEYFLAG_SLIDER_DISABLED|KEYPAD_3,     KEYPAD_SOFT_LEFT,
    KEYPAD_CLEAR_BACK, KEYPAD_MESSAGING,
    /* row 2 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_7,     KEYFLAG_SLIDER_DISABLED|KEYPAD_9,
    KEYFLAG_SLIDER_DISABLED|KEYPAD_5,     KEYFLAG_SLIDER_DISABLED|KEYPAD_STAR,
    KEYFLAG_SLIDER_DISABLED|KEYPAD_POUND, KEYFLAG_SLIDER_DISABLED|KEYPAD_0,
    /* row 3 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_2,     KEYPAD_NAV_UP,
    KEYPAD_SMART,                         KEYPAD_SEND,
    KEYPAD_IMAGING,                       KEYPAD_PTT,
    /* row 4 */
    KEYFLAG_SLIDER_DISABLED|KEYPAD_4,     KEYPAD_NAV_LEFT,
    KEYPAD_SIDE_UP,                       KEYPAD_SOFT_RIGHT,
	KEYPAD_SIDE_DOWN, KEYPAD_VA,
};
#endif

#elif defined(CONFIG_ARCH_MXC91321)

static unsigned short *key_map;

static unsigned short key_map_kassos_P0[] = {
    /* row 0 */
    KEYPAD_9, KEYPAD_7, KEYPAD_CAMERA_FOCUS,
    KEYPAD_NONE, KEYPAD_5, KEYPAD_CAMERA_TAKE_PIC,
    /* row 1 */
    KEYPAD_0, KEYPAD_CARRIER, KEYPAD_NONE,
    KEYPAD_1, KEYPAD_NONE, KEYPAD_NONE,
    /* row 2 */
    KEYPAD_POUND, KEYPAD_VOICE_CALL_SEND, KEYPAD_VR,
    KEYPAD_3, KEYPAD_NONE, KEYPAD_NONE,
    /* row 3 */
    KEYPAD_STAR, KEYPAD_VIDEO_CALL_SEND, KEYPAD_ROCKER_UP,
    KEYPAD_NAV_RIGHT, KEYPAD_NONE, KEYPAD_NONE,
    /* row 4 */
    KEYPAD_8, KEYPAD_MAIL, KEYPAD_ROCKER_CENTER,
    KEYPAD_NAV_LEFT, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 5 */
    KEYPAD_2, KEYPAD_SOFT_RIGHT, KEYPAD_ZOOM_IN,
    KEYPAD_NAV_DOWN, KEYPAD_NONE, KEYPAD_NONE,
    /* row 6 */
    KEYPAD_4, KEYPAD_CLEAR_BACK, KEYPAD_ZOOM_OUT,
    KEYPAD_NAV_UP, KEYPAD_NONE, KEYPAD_NONE,
    /* row 7 */
    KEYPAD_6, KEYPAD_SOFT_LEFT, KEYPAD_ROCKER_DOWN,
    KEYPAD_NAV_CENTER, KEYPAD_NONE, KEYPAD_NONE,
};

static unsigned short key_map_kassos_P1[] = {
    /* row 0 */
    KEYPAD_9, KEYPAD_7, KEYPAD_1, KEYPAD_NONE, 
    KEYPAD_5, KEYPAD_8, KEYPAD_2, KEYPAD_0,
    /* row 1 */
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_6,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_4, KEYPAD_NONE,
    /* row 2 */
    KEYPAD_POUND, KEYPAD_VOICE_CALL_SEND, KEYPAD_NONE, KEYPAD_3,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE,
    /* row 3 */
    KEYPAD_STAR, KEYPAD_VIDEO_CALL_SEND, KEYPAD_NONE, KEYPAD_NAV_RIGHT,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 4 */
    KEYPAD_CARRIER, KEYPAD_MAIL, KEYPAD_VOLDOWN, KEYPAD_NAV_LEFT,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 5 */
    KEYPAD_NONE, KEYPAD_SOFT_RIGHT, KEYPAD_SMART, KEYPAD_NAV_DOWN,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 6 */
    KEYPAD_NONE, KEYPAD_CLEAR_BACK, KEYPAD_VR, KEYPAD_NAV_UP,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 7 */
    KEYPAD_NONE, KEYPAD_SOFT_LEFT, KEYPAD_VOLUP, KEYPAD_NAV_CENTER,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
};

static unsigned short key_map_P3[] = {
    /* row 0 */
    KEYPAD_9, KEYPAD_7, KEYPAD_CAMERA_FOCUS,
    KEYPAD_NONE, KEYPAD_5, KEYPAD_CAMERA_TAKE_PIC,
    /* row 1 */
    KEYPAD_0, KEYPAD_SPARE1, KEYPAD_SPARE2,
    KEYPAD_1, KEYPAD_NONE, KEYPAD_NONE,
    /* row 2 */
    KEYPAD_POUND, KEYPAD_VOICE_CALL_SEND, KEYPAD_VR,
    KEYPAD_3, KEYPAD_NONE, KEYPAD_NONE,
    /* row 3 */
    KEYPAD_STAR, KEYPAD_VIDEO_CALL_SEND, KEYPAD_ROCKER_UP,
    KEYPAD_NAV_RIGHT, KEYPAD_NONE, KEYPAD_NONE,
    /* row 4 */
    KEYPAD_8, KEYPAD_MAIL, KEYPAD_ROCKER_CENTER,
    KEYPAD_NAV_LEFT, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 5 */
    KEYPAD_2, KEYPAD_SOFT_RIGHT, KEYPAD_ZOOM_IN,
    KEYPAD_NAV_DOWN, KEYPAD_NONE, KEYPAD_NONE,
    /* row 6 */
    KEYPAD_4, KEYPAD_CLEAR_BACK, KEYPAD_ZOOM_OUT,
    KEYPAD_NAV_UP, KEYPAD_NONE, KEYPAD_NONE,
    /* row 7 */
    KEYPAD_6, KEYPAD_SOFT_LEFT, KEYPAD_ROCKER_DOWN,
    KEYPAD_NAV_CENTER, KEYPAD_NONE, KEYPAD_NONE,
};

static unsigned short key_map_P4[] = {
    /* row 0 */
    KEYPAD_9, KEYPAD_7, KEYPAD_1, KEYPAD_NONE, 
    KEYPAD_5, KEYPAD_8, KEYPAD_2, KEYPAD_0,
    /* row 1 */
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_6,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_4, KEYPAD_NONE,
    /* row 2 */
    KEYPAD_POUND, KEYPAD_VOICE_CALL_SEND, KEYPAD_NONE, KEYPAD_3,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE,
    /* row 3 */
    KEYPAD_STAR, KEYPAD_VIDEO_CALL_SEND, KEYPAD_NONE, KEYPAD_NAV_RIGHT,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 4 */
    KEYPAD_NONE, KEYPAD_MAIL, KEYPAD_VOLDOWN, KEYPAD_NAV_LEFT,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 5 */
    KEYPAD_NONE, KEYPAD_SOFT_RIGHT, KEYPAD_SMART, KEYPAD_NAV_DOWN,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 6 */
    KEYPAD_NONE, KEYPAD_CLEAR_BACK, KEYPAD_VR, KEYPAD_NAV_UP,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
    /* row 7 */
    KEYPAD_NONE, KEYPAD_SOFT_LEFT, KEYPAD_VOLUP, KEYPAD_NAV_CENTER,
    KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, KEYPAD_NONE, 
};
#endif

/* keypad inactive rows/columns */ 
#if defined(CONFIG_MOT_FEAT_SLIDER)  /* for slider phone */
#if defined(CONFIG_ARCH_MXC91231)
#if defined(CONFIG_MACH_NEVIS)
/* Disable everything for Nevis since all keys are under slider */
#define KEYPAD_CLOSED_INACTIVE_ROWS    (0x0F)
#define KEYPAD_CLOSED_INACTIVE_COLUMNS (0x2B)
#else
/* Ascension & Marco inactive rows/columns */
#define KEYPAD_CLOSED_INACTIVE_ROWS    (0x04)
#define KEYPAD_CLOSED_INACTIVE_COLUMNS (0x01)
#endif /* Nevis */
#endif

#elif defined(CONFIG_MOT_FEAT_FLIP)  /* for flip phone */
#if defined(CONFIG_ARCH_MXC91231)
/* Lido & Pico inactive rows/columns */
#define KEYPAD_CLOSED_INACTIVE_ROWS    (0x07)
#define KEYPAD_CLOSED_INACTIVE_COLUMNS (0x0B)
#elif  defined(CONFIG_ARCH_MXC91321)
#define KEYPAD_CLOSED_INACTIVE_ROWS    (0x00)
#define KEYPAD_CLOSED_INACTIVE_COLUMNS (0x00)
#endif

#else    /* for non-flip/slider or unkown type phone */
#if defined(CONFIG_MACH_ELBA)
/* Elba inactive rows/columns */
#define KEYPAD_CLOSED_INACTIVE_ROWS    (0x1F)
#define KEYPAD_CLOSED_INACTIVE_COLUMNS (0x3F)
#else
#define KEYPAD_CLOSED_INACTIVE_ROWS    (0x00)
#define KEYPAD_CLOSED_INACTIVE_COLUMNS (0x00)
#endif
#endif

static inline void turn_on_bit(unsigned long map[], int);
static inline void turn_off_bit(unsigned long map[], int);

static inline void insert_raw_event (unsigned short event);

static void mxc_kpp_handle_timer (unsigned long);
static void column_row_enabler (STATUS_E status);

DECLARE_TASKLET(mxc_kpp_tasklet, mxc_kpp_handle_timer, 0);

#if defined(CONFIG_MOT_FEAT_FLIP)
static inline void flip_status_handler(STATUS_E, unsigned short);
#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
static inline void slider_status_handler(STATUS_E, unsigned short);
#endif

#if defined(CONFIG_MACH_ELBA)
static inline void keylock_status_handler(STATUS_E, unsigned short);
#elif defined(CONFIG_MACH_NEVIS)
static STATUS_E flip_slider_get_status(void);
#endif

/*! Sidekeys are inputs that create key events but don't come from the key 
 *  matrix.  For now, keylock and lens cover. SIDEKEY_T structs hold the
 *  private data for one of these. 
 */
typedef struct
{
    /*! A timer used for interrupt handling */
    struct timer_list timer;
    /*! The open/closed status */
    STATUS_E status;
    /*! The scancode for the key event queue */
    unsigned short keycode;
    /*! Get the hardware status */
    STATUS_E (*read_status)(void);
} SIDEKEY_T;

static STATUS_E read_keylock(void);
static STATUS_E read_lenscover(void);
static void sidekey_status_handler(SIDEKEY_T*, STATUS_E, unsigned short event);
static void sidekey_status_check(SIDEKEY_T*);
static int sidekey_request_irq(const char *name, int irq, SIDEKEY_T*);
static void sidekey_free_irq(int irq, SIDEKEY_T*);
static irqreturn_t sidekey_irq_handler(int irq, void* dev_id, struct pt_regs*);
static void sidekey_timer_handler(unsigned long data);

/*! SIDEKEY_T instance for key lock
 */
static SIDEKEY_T keylock_data = {
    .keycode = KEYPAD_LOCK,
    .read_status = read_keylock,
    .status = (STATUS_E)-1 /* Force sending initial status. */
};
/*! SIDEKEY_T instance for lens cover
 */
static SIDEKEY_T lenscover_data = {
    .keycode = KEYPAD_LENS_COVER,
    .read_status = read_lenscover,
    .status = (STATUS_E)-1
};

/*! KEYLOCK_IRQ and LENSCOVER_IRQ are the interrupt numbers passed to request_irq.
 */
#define KEYLOCK_IRQ   INT_EXT_INT3
#define LENSCOVER_IRQ INT_EXT_INT4

/*!
 * This function initializes the keypad queue. This is called by keypad driver
 * open function.
 */
static inline void mxc_kpp_initq(void)
{
    kpp_dev.queue.head = kpp_dev.queue.tail = 0;
    kpp_dev.queue.len = 0;
    kpp_dev.queue.s_lock = SPIN_LOCK_UNLOCKED;
    init_waitqueue_head(&kpp_dev.queue.waitq);
}

/*!
 * This function fetches the next character available in keypad queue.
 * This is called by the keypad driver read function.
 *
 * @return  scancode from queue if a character is available, else it
 * returns 0.
 */
static inline unsigned short mxc_kpp_pullq(void)
{
    unsigned short result;
    unsigned long flags;

    spin_lock_irqsave(&kpp_dev.queue.s_lock, flags);
    if (!kpp_dev.queue.len) 
    {
        spin_unlock_irqrestore(&kpp_dev.queue.s_lock, flags);
        return 0;
    }
    result = kpp_dev.queue.buf[kpp_dev.queue.head];
#ifdef KPP_DEBUG
    printk("KPP: data from queue %d\n", result);
    printk("Value of queue.head ... %d\n", kpp_dev.queue.head);
    printk("value of queue.tail ... %d\n", kpp_dev.queue.tail);
    printk("value of queue.len ... %d\n", kpp_dev.queue.len);
#endif
    kpp_dev.queue.head++;
    kpp_dev.queue.head &= (KPP_BUF_SIZE - 1);
    kpp_dev.queue.len--;
    spin_unlock_irqrestore(&kpp_dev.queue.s_lock, flags);
    return result;
}

/*!
 * This function puts the input scancode into the keypad queue. This is called
 * by the keypad driver when it recognizes a new key press/release
 * event on the keypad. 
 *
 * @param   scancode      character to be queued
 */   
static inline void mxc_kpp_pushq(unsigned short scancode)
{
    unsigned long flags;
#if defined(CONFIG_MACH_NEVIS)
    bool both_vol_keys_pressed = false;
#endif
    spin_lock_irqsave(&kpp_dev.queue.s_lock, flags);

    switch (KEYCODE (scancode))
    {
#if defined(CONFIG_MOT_FEAT_FLIP)
        case KEYPAD_FLIP:
            if (KEY_IS_DOWN(scancode)) 
            {
                mpm_event_notify(MPM_EVENT_DEVICE, EVENT_DEV_FLIP, DEV_OFF);
            }
            else 
            { /* key up == flip open */
                mpm_event_notify(MPM_EVENT_DEVICE, EVENT_DEV_FLIP, DEV_ON);
            }
            break;
#endif
#if defined(CONFIG_MOT_FEAT_SLIDER)
        case KEYPAD_SLIDER:
            if (KEY_IS_DOWN(scancode)) 
            {
                mpm_event_notify(MPM_EVENT_DEVICE, EVENT_DEV_SLIDER, DEV_OFF);
            }
            else 
            { /* key up == slider open */
                mpm_event_notify(MPM_EVENT_DEVICE, EVENT_DEV_SLIDER, DEV_ON);
            }
            break;
#endif
        default:
            mpm_event_notify(MPM_EVENT_DEVICE, EVENT_DEV_KEY, 0);
    }

    if (kpp_dev.queue.len < KPP_BUF_SIZE) 
    {
        if (!IS_ASCII_EVENT(scancode)) 
        {
            if (KEY_IS_DOWN(scancode)) 
            {
                turn_on_bit(keybitmap, KEYCODE(scancode));
            }
            else 
            {
                turn_off_bit(keybitmap, KEYCODE(scancode));
            }
        }
#if defined(CONFIG_MACH_NEVIS)
    /* Check for the simultaneous press of VOL_UP and VOL_DOWN key only when slider is CLOSED in Nevis ONLY */
	if ((slider_state == CLOSE) && 
		/* VOL_UP and VOL_DOWN Key? */
		(keycnt == 2))
	{
		if (((KEYCODE(kpp_dev.recent_key_value) == KEYPAD_SIDE_UP) &&  
			(KEYCODE(scancode) == KEYPAD_SIDE_DOWN))
		|| ((KEYCODE(kpp_dev.recent_key_value)== KEYPAD_SIDE_DOWN) &&  
			(KEYCODE(scancode) == KEYPAD_SIDE_UP)))
		{
			both_vol_keys_pressed = true;
# ifdef KPP_DEBUG
			printk("KPP: BOTH VOL UP and DOWN key pressed. PreviousKey = %x, CurrentKey = %x \n", 
			KEYCODE(kpp_dev.recent_key_value), KEYCODE(scancode));
# endif
		}
	}
#endif
#if defined(CONFIG_MACH_NEVIS)
	if (both_vol_keys_pressed == false)
	{
#endif
        kpp_dev.queue.buf[kpp_dev.queue.tail] = scancode;
        kpp_dev.queue.tail++;
        kpp_dev.queue.tail &= (KPP_BUF_SIZE - 1);
        kpp_dev.queue.len++;
        wake_up_interruptible(&kpp_dev.queue.waitq);
# ifdef KPP_DEBUG
        printk("KPP: pressed key added %u\n", kpp_dev.queue.buf[kpp_dev.queue.tail]);
        printk("Value of queue.head  %d\n", kpp_dev.queue.head);
        printk("value of queue.tail  %d\n", kpp_dev.queue.tail);
        printk("value of queue.len  %d\n", kpp_dev.queue.len);
# endif
#if defined(CONFIG_MACH_NEVIS)
    }
#endif

    }
    kpp_dev.recent_key_value = scancode;

    spin_unlock_irqrestore(&kpp_dev.queue.s_lock, flags);
}

/*!
 * This function checks if the keypad queue is empty. 
 *
 * @return      0 if queue is empty, non-zero if queue is not empty. 
 */   
static inline int mxc_kpp_q_empty(void)
{
    return (kpp_dev.queue.len == 0);
}

/*
 * Turn on the bit position in map for the specified key code.
 */
static inline void turn_on_bit(unsigned long map[], int code)
{
    if ((code < 0) || (code > KEYPAD_MAXCODE))
    {
        return;
    }
    map[WORD_IN_BITMAP(code)] |= BIT_FOR_CODE(code);
}

/*
 * Turn off the bit position in map for the specified key code.
 */
static inline void turn_off_bit(unsigned long map[], int code)
{
    if ((code < 0) || (code > KEYPAD_MAXCODE))
    {
        return;
    }
    map[WORD_IN_BITMAP(code)] &= ~BIT_FOR_CODE(code);
}

/*!
 * This function takes scancode as input, checks if the keypad mode is RAW or
 * XLATE and pushes scancode or mapcode into the keypad queue accordingly.
 * This is called by the keypad driver when a key press/release event is
 * recognized on the keypad.
 *
 * @param   scancode        scancode of the key pressed/released
 */
void mxc_kpp_handle_mode(unsigned short scancode)
{
    keyeventcb*    cbchain;
    unsigned short queued_code;
    unsigned short mapcode;

    /*
     * kpp_mode variable is checked for mode of keypad.  The keypad mode
     * can be set by the user using ioctl call.  If the keypad mode is in
     * RAW mode, then the raw scancode generated is pushed into keypad
     * queue.  Else if the keypad mode is in map mode, then the mapcode
     * for the scancode is generated and pushed into keypad queue.
     */
    if (kpp_dev.kpp_mode == KEYPAD_RAW) 
    {
        queued_code = scancode;

        mxc_kpp_pushq(scancode);
#ifdef KPP_DEBUG
        printk("KPP: scancode 0x%x\n", scancode);
#endif
    } 
    else if (kpp_dev.kpp_mode == KEYPAD_XLATE) 
    {
#if ! defined(CONFIG_MACH_XPIXL)
        mapcode = key_map[scancode]&KEY_CODE_MASK;	
#endif	
        if (press == 1) 
        {
#if defined(CONFIG_MACH_XPIXL)
            mapcode = key_map[scancode]&KEY_CODE_MASK;
#endif
            mapcode |= KEYDOWN;
        } 
#if defined(CONFIG_MACH_XPIXL)
else
        {
            mapcode = keymap_all[scancode]&KEY_CODE_MASK;
        }
#endif

        queued_code = mapcode;

#ifdef KPP_DEBUG
        printk("KPP: mapcode %d\n", mapcode);
#endif
        mxc_kpp_pushq(mapcode);
    }
    else
    {
        return;
    }

    cbchain = keyeventcb_chain;
    while(cbchain)
    {
        cbchain->notify_function(queued_code, cbchain->user_data);
        cbchain = cbchain->next_callback;
    }
}

/*!
 * This function returns 1 if the current scan shows multiple keys
 * sharing a row and column, 0 otherwise.
 */
static inline int shared_row_and_column(void) 
{
    char keysincol[COLS];    /* # keys down in col j */
    char keysinrow[ROWS];    /* # keys down in row i */
    int commoncol = 0;       /* > 0 if there are keys sharing a column */
    int i,j;
  
    memset (keysincol, 0, sizeof(keysincol));
    memset (keysinrow, 0, sizeof(keysinrow));

    for (j = 0; j < COLS; j++) 
    {
        for (i = 0; i < ROWS; i++) 
        {
            if (TEST_BIT(cur_rcmap[i], j)) 
            {
                keysincol[j]++;
                keysinrow[i]++;
            }
        }
    
        if (keysincol[j] > 1) 
        {
            commoncol = 1;
        }
    }

    /* if no keys shared a column, we're done */
    if (commoncol == 0) 
    {
        return 0;
    }

    /* reach here if there were keys sharing a column */
    for (j=0;j<COLS;j++) 
    {
        if (keysincol[j] < 2) 
        {
            /* column j wasn't shared; go on to next column */
            continue;
        }
    
        for (i=0;i<ROWS;i++) 
        {
            if (TEST_BIT(cur_rcmap[i], j)) 
            {
                /* reach here for key down in row i of a shared column */
                /* if row i is also shared, we could have a "ghost key" */
                if (keysinrow[i] > 1) 
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/*!
 * This function reads the hardware registers to get the row and column
 * information for the keys that are down.  It puts the information in the
 * specified array and returns the number of keys down.
 */
int inline scan_columns(unsigned short *map) 
{
    unsigned short reg_val;
    int row, col;
    unsigned int scancode;

#ifdef KPP_DEBUG
    unsigned short ustmp;
#endif

    /* wmb() linux kernel function which guarantees orderings in write operations */
    wmb();

    keycnt = 0;
    memset(map, 0, sizeof(cur_rcmap));
        
    for (col = 0; col < COLS; col++) 
    {        
        /* Col */
        if(invalid_columns & (1<<col))
        {
            continue;
        }
       
        /* 2. Write 1.s to KPDR[15:8] setting column data to 1.s */
        reg_val = ((1 << COLS) - 1) << 8;
        writew(reg_val, KPDR);

        /* 3. Configure columns as totem pole outputs(for quick discharging of keypad capacitance) */
        reg_val = readw(KPCR);
        reg_val &= (1 << ROWS) - 1;
        writew(reg_val, KPCR);
        udelay(40);

        /* 4. Configure columns as open-drain */
        reg_val |= ((1 << COLS) - 1) << 8;
        writew(reg_val, KPCR);

        /* 5. Write a single column to 0, others to 1.
         * 6. Sample row inputs and save data. Multiple key presses can be detected on a single column.
         * 7. Repeat steps 2 - 6 for remaining columns.
         * Col bit starts at 8th bit in KPDR */
        reg_val = ((1 << COLS) - 1) << 8;
        reg_val &= ~(1 << (8 + col));
        writew(reg_val, KPDR);

        /*
         * A delay in the signal timing may exist due to the
         * capacitive loading on the signal from the column, 
         * through the switch, and back to the row. That's why
         * it's necessary to wait a little bit between turning
         * off a column in the KPDR and reading the KPDR.  The
         * delay size depends on the board design and the method
         * used to interface to the keypad.
         */

        /* Read row input */
        udelay(40);
        reg_val = readw(KPDR);

#ifdef KPP_DEBUG
        printk("KPP: data register 0x%x\n", reg_val);
        ustmp = readw(KPSR);
        printk("KPP: status register 0x%x\n", ustmp);
#endif

#if (defined(CONFIG_MOT_FEAT_SLIDER) || defined(CONFIG_MOT_FEAT_FLIP) || defined(CONFIG_MACH_ELBA)) && !defined(CONFIG_MACH_XPIXL)
        scancode = col;
#endif
        for (row = 0; row < ROWS; row++) 
        { /* sample row */
#if defined(CONFIG_MOT_FEAT_SLIDER)
            if((key_map[scancode]&KEYFLAG_SLIDER_DISABLED) && kpp_dev.status == CLOSE)
#elif defined(CONFIG_MOT_FEAT_FLIP)
            if((key_map[scancode]&KEYFLAG_FLIP_DISABLED) && kpp_dev.status == CLOSE)
#elif !defined(CONFIG_MACH_XPIXL)
            if((key_map[scancode]&KEYFLAG_KEYLOCK_DISABLED) && kpp_dev.status == CLOSE)
#endif
#if (defined(CONFIG_MOT_FEAT_SLIDER) || defined(CONFIG_MOT_FEAT_FLIP)|| defined(CONFIG_MACH_ELBA)) && !defined(CONFIG_MACH_XPIXL)
            {
                scancode += COLS;
                continue;
            }
            scancode += COLS;
#endif

            if(invalid_rows&(1<<row))
            {
                continue;
            }

            if (TEST_BIT(reg_val, row) == 0) 
            {
                map[row] = BITSET(map[row], col);
                keycnt++;
#ifdef KPP_DEBUG
                printk("KPP: key pressed (row %d, col %d)\n", row, col);
#endif
            } 
        }
    }

#ifdef KPP_DEBUG
    ustmp = readw(KPSR);
    printk("KPP: status regs value %x\n", ustmp);
#endif

    /* 8. Return all columns to 0 in preparation for standby mode.
     * 9. Clear KPKD and KPKR status bit(s) by writing to a .1.,
     * set the KPKR synchronizer chain by writing "1" to KRSS register,
     * clear the KPKD synchronizer chain by writing "1" to KDSC register
     */
    writew(invalid_columns<<8, KPDR);

    reg_val = readw(KPSR);
    reg_val |= KBD_STAT_KPKD | KBD_STAT_KPKR | KBD_STAT_KRSS | KBD_STAT_KDSC;
    writew(reg_val, KPSR);

    /* 
     * If there are keys down sharing a row and column, we might
     * have a "ghost" key reported that's not actually down.
     * We prevent that by ignoring such scans.
     */
    if (keycnt > 3) 
    {
        if (shared_row_and_column() == 1) 
        {
                memcpy(map, prev_rcmap, sizeof(prev_rcmap));
            keycnt = prev_keycnt;
        }
    }

    return keycnt;
}

/*!
 * This function returns 1 (true) if the two passed-in row arrays
 * are the same, 0 (false) if they are not the same.
 */
int inline map_same (unsigned short *map1, unsigned short *map2) 
{
    return((memcmp(map1, map2, (ROWS * sizeof(*map1)))) == 0);
}


/*!
 * This function is called to scan the keypad matrix to find out the key press
 * and key release events. Make scancode and break scancode are generated for
 * key press and key release events. 
 *
 * The following scanning sequence are done for 
 * keypad row and column scanning,
 * -# Write 1's to KPDR[15:8], setting column data to 1's 
 * -# Configure columns as totem pole outputs(for quick discharging of keypad
 * capacitance)
 * -# Configure columns as open-drain
 * -# Write a single column to 0, others to 1.  
 * -# Sample row inputs and save data. Multiple key presses can be detected on
 * a single column.
 * -# Repeat the above steps for remaining columns.
 * -# Return all columns to 0 in preparation for standby mode.
 * -# Clear KPKD and KPKR status bit(s) by writing to a 1,
 *    Set the KPKR synchronizer chain by writing "1" to KRSS register,
 *    Clear the KPKD synchronizer chain by writing "1" to KDSC register
 *    
 * @result    Number of key pressed/released.
 */ 
static int mxc_kpp_scan_matrix(void)
{
    unsigned short rcmap[ROWS];
    unsigned char  autorepeat_index;

    int col, row;
    unsigned short scancode;
    unsigned long  flags;

    if (keystate == DEBOUNCE) 
    {
        keycnt = scan_columns(rcmap);
        if (map_same (cur_rcmap, rcmap)) 
        {
            keystate = STABLE;
        }
        else 
        {
            memcpy(cur_rcmap, rcmap, sizeof(cur_rcmap));
            return 1;
        }
    }
    else 
    { /* state is STABLE*/
        memcpy(prev_rcmap, cur_rcmap, sizeof(prev_rcmap));
        prev_keycnt = keycnt;
        keycnt = scan_columns(cur_rcmap);
        
        if (!map_same (prev_rcmap, cur_rcmap)) 
        {
            keystate = DEBOUNCE;
            return 1;
        }
        else 
        {
            return keycnt;
        }
    }

    /* Check key press status change */

    /*
     * prev_rcmap array will contain the previous status of the keypad
     * matrix.  cur_rcmap array will contains the present status of the
     * keypad matrix. If a bit is set in the array, that (row, col) bit is
     * pressed, else it is not pressed. 
     *
     * XORing these two variables will give us the change in bit for
     * particular row and column.  If a bit is set in XOR output, then that
     * (row, col) has a change of status from the previous state.  From
     * the diff variable the key press and key release of row and column
     * are found out. 
     *
     */
    for (row = 0; row < ROWS; row++)
    {
        unsigned short diff;

        /* Calculate the change in the keypad row status */
        diff = prev_rcmap[row] ^ cur_rcmap[row];

        for (col = 0; col < COLS; col++)
        {
            if ((diff >> col) & 0x1)
            {
                /* There is a status change on col */
                scancode = ((row * COLS) + col);
                if ((prev_rcmap[row] & BITSET(0, col)) == 0)
                {
                    /* Previous state is 0, so now a key is pressed */
                    spin_lock_irqsave(&autorepeat_lock, flags);

                    if (start_autorepeat_key_info == max_autorepeat_key_info)
                    {
                        autorepeat_key_info[scancode].next_keypress     = max_autorepeat_key_info;
                        autorepeat_key_info[scancode].previous_keypress = max_autorepeat_key_info;
                        autorepeat_key_info[scancode].repeat_time       = jiffies+kpp_dev.autorepeat_info.r_time_to_first_repeat;

                        start_autorepeat_key_info = scancode;
                        end_autorepeat_key_info   = scancode;

                        if(kpp_dev.autorepeat_info.r_repeat)
                        {
                            kpp_dev.autorepeat_timer.expires = autorepeat_key_info[scancode].repeat_time;
                            add_timer(&kpp_dev.autorepeat_timer);
                        }
                    }
                    else
                    {
                        autorepeat_key_info[scancode].next_keypress     = max_autorepeat_key_info;
                        autorepeat_key_info[scancode].previous_keypress = end_autorepeat_key_info;
                        autorepeat_key_info[scancode].repeat_time       = jiffies+kpp_dev.autorepeat_info.r_time_to_first_repeat;
                        autorepeat_key_info[end_autorepeat_key_info].next_keypress = scancode;

                        end_autorepeat_key_info = scancode;

                        if(kpp_dev.autorepeat_info.r_repeat &&
                           (autorepeat_key_info[scancode].repeat_time < kpp_dev.autorepeat_timer.expires 
                            || !timer_pending(&kpp_dev.autorepeat_timer)))
                        {
                            del_timer(&kpp_dev.autorepeat_timer);

                            kpp_dev.autorepeat_timer.expires = autorepeat_key_info[scancode].repeat_time;
                            add_timer(&kpp_dev.autorepeat_timer);
                        }
                    }

                    spin_unlock_irqrestore(&autorepeat_lock, flags);
                    press = 1;

#ifdef KPP_DEBUG
                    printk("Press   (%d, %d) scan=%d\n", row, col, scancode);
#endif
                }
                else
                {
                    /* Previous state is not 0, so now a key is released */
                    spin_lock_irqsave(&autorepeat_lock, flags);

                    if(autorepeat_key_info[scancode].previous_keypress != max_autorepeat_key_info)
                    {
                        autorepeat_index                                    = autorepeat_key_info[scancode].previous_keypress;
                        autorepeat_key_info[autorepeat_index].next_keypress = autorepeat_key_info[scancode].next_keypress;
                    }
                    else
                    {
                        start_autorepeat_key_info = autorepeat_key_info[scancode].next_keypress;
                    }

                    if(autorepeat_key_info[scancode].next_keypress != max_autorepeat_key_info)
                    {
                        autorepeat_index                                        = autorepeat_key_info[scancode].next_keypress;
                        autorepeat_key_info[autorepeat_index].previous_keypress = autorepeat_key_info[scancode].previous_keypress;
                    }
                    else
                    {
                        end_autorepeat_key_info = autorepeat_key_info[scancode].previous_keypress;
                    }

                    if(end_autorepeat_key_info == max_autorepeat_key_info &&
                       start_autorepeat_key_info == max_autorepeat_key_info)
                    {
                        del_timer(&kpp_dev.autorepeat_timer);
                    }

                    spin_unlock_irqrestore(&autorepeat_lock, flags);
                                        press = 0;
#ifdef KPP_DEBUG
                    printk("Release (%d, %d) scan=%d\n", row, col, scancode);
#endif                                                
                }

                mxc_kpp_handle_mode(scancode);
            }
        }
    }

#ifdef KPP_DEBUG
    printk("KPP: cur_rcmap ");
    {
        int i;
        for (i=0;i<ROWS;i++) 
        { 
            printk("0x%X ", cur_rcmap[i]); 
        }
        printk("\n");
    }
    printk("keycnt: %d\n", keycnt);
#endif
    return keycnt;
}

/*!
 * This is called when the autorepeat timer goes off.
 */
static void
autorepeat_handle_timer (unsigned long unused)
{
    unsigned long expired_time;
    unsigned long next_timer;
    unsigned long flags;
    unsigned char scancode;

    spin_lock_irqsave(&autorepeat_lock, flags);
    
    expired_time = kpp_dev.autorepeat_timer.expires;
    next_timer   = expired_time;
    scancode     = start_autorepeat_key_info;

    while(scancode != max_autorepeat_key_info)
    {
        if(autorepeat_key_info[scancode].repeat_time == expired_time)
        {
            press = 1;
            mxc_kpp_handle_mode(scancode);

            autorepeat_key_info[scancode].repeat_time += kpp_dev.autorepeat_info.r_time_between_repeats;
        }

        if(autorepeat_key_info[scancode].repeat_time < next_timer || next_timer == expired_time)
        {
            next_timer = autorepeat_key_info[scancode].repeat_time;
        }

        scancode = autorepeat_key_info[scancode].next_keypress;
    }

    mod_timer(&kpp_dev.autorepeat_timer, next_timer);
    spin_unlock_irqrestore(&autorepeat_lock, flags);
}

/*!
 * This is called when the headset timer goes off.
 */
static void
headset_handle_timer (unsigned long unused)
{
    unsigned long flags;

    spin_lock_irqsave(&autorepeat_lock, flags);

    if(headset_scancode != 0xFFFF)
    {
        insert_raw_event(headset_scancode);
        
        headset_timer.expires = jiffies + kpp_dev.autorepeat_info.r_time_between_repeats;
    }

    add_timer(&headset_timer);
    spin_unlock_irqrestore(&autorepeat_lock, flags);
}

/*!
 * This is called when the headset timer goes off.
 */
static void
emu_headset_handle_timer (unsigned long unused)
{
    unsigned long flags;

    spin_lock_irqsave(&autorepeat_lock, flags);

    if(emu_headset_scancode != 0xFFFE)
    {
        insert_raw_event(emu_headset_scancode);
        
        emu_headset_timer.expires = jiffies + kpp_dev.autorepeat_info.r_time_between_repeats;
    }

    add_timer(&emu_headset_timer);
    spin_unlock_irqrestore(&autorepeat_lock, flags);
}

/*!
 * This function is called to start the timer for scanning the keypad if the
 * keypad has any key pressed. When the keypad has any key pressed then we are
 * in polling mode and the timer is set for next keypad scan. 
 * When there are no keys pressed on the keypad we
 * return back to interrupt mode, waiting for a keypad key press event. 
 *
 * @param data  Opaque data passed back by kernel. Not used.
 */
static void mxc_kpp_handle_timer(unsigned long data)
{
    unsigned short reg_val;
    unsigned long flags;

    if (mxc_kpp_scan_matrix() > 0) 
    {
        kpp_dev.poll_timer.expires = jiffies + (RELEASE_POLL_DELAY * HZ) / 1000;
        add_timer(&kpp_dev.poll_timer);
    } 
    else 
    {
        del_timer (&kpp_dev.autorepeat_timer);

        local_irq_save(flags);
      
        /* Stop scanning and wait for interrupt. Enable press interrupt and disable release interrupt. */
        reg_val = readw(KPSR);
        reg_val |= KBD_STAT_KDIE;
        reg_val &= ~KBD_STAT_KRIE;
        reg_val |= KBD_STAT_KPKR | KBD_STAT_KPKD;
        writew(reg_val, KPSR);

        reject_suspend = 0;

        local_irq_restore(flags);
    }
}

/*!
 * This function is the keypad Interrupt handler.
 * This function checks for keypad status register (KPSR) for key press 
 * and key release interrupt. If key press interrupt has occurred, then the key
 * press and key release interrupt in the KPSR are disabled. If the key
 * release interrupt has occurred, then key release interrupt is disabled and
 * key press interrupt is enabled in the KPSR.
 * It then calls mxc_kpp_scan_matrix to check for any key pressed/released.
 * If any key is found to be pressed, then a timer is set to call
 * mxc_kpp_scan_matrix function for every RELEASE_POLL_DELAY ms.
 *
 * @param   irq      The Interrupt number
 * @param   dev_id   Driver private data
 * @param   regs     Holds a snapshot of the processors context before the
 *                   processor entered the interrupt code
 *
 * @result    The function returns \b IRQ_RETVAL(1) if interrupt was handled,
 *            returns \b IRQ_RETVAL(0) if the interrupt was not handled. 
 *            \b IRQ_RETVAL is defined in include/linux/interrupt.h.
 */ 
static irqreturn_t mxc_kpp_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    unsigned short reg_val;

    /* Delete the polling timer */
    del_timer(&kpp_dev.poll_timer);
    reg_val = readw(KPSR);

    /* Check if it is key press interrupt */
    if (reg_val & KBD_STAT_KPKD) 
    {
        reject_suspend = 1;
    
#ifdef KPP_DEBUG
        printk("KPP: Key press interrupt\n");
#endif
        /* Disable key press(KDIE status bit) interrupt clear key release(KRIE status bit) interrupt */
        reg_val &= ~KBD_STAT_KDIE;
        writew(reg_val, KPSR);
#ifdef KPP_DEBUG
        reg_val = readw(KPSR); 
        printk("KPP: Status reg value after disable 0x%x\n", reg_val);
#endif
    } 
    else if (reg_val & KBD_STAT_KPKR) 
    {
        /* Check if it is key release interrupt */
#ifdef KPP_DEBUG
        printk("KPP: Key release interrupt occurred (not expected)\n");
#endif
        /* set key press(KDIE status bit) interrupt. clear key release(KRIE status bit) interrupt.*/ 
        reg_val |= KBD_STAT_KDIE;
        writew(reg_val, KPSR);
    } 
    else 
    {
        /* spurious interrupt */
        return IRQ_RETVAL(0);
    }

    /* for power management */
    mpm_handle_ioi();

    tasklet_schedule(&mxc_kpp_tasklet);

    return IRQ_RETVAL(1);
}

/*!
 * This function is called when the keypad driver is opened.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 *
 * @result    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int mxc_kpp_open(struct inode *inode, struct file *file)
{
#ifdef KPP_DEBUG
    printk("KPP: kdb_open() called... \n");
#endif

    spin_lock(&keypad_lock);
    
    if (reading_opens > 0) 
    {
        spin_unlock(&keypad_lock);
        return -EBUSY;
    }
    
    reading_opens++;
    spin_unlock(&keypad_lock);
   
    return 0;
}

/*!
 * This function is called to open /dev/keypadI, the device for
 * software key event insertion.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 *
 * @result    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int mxc_kppI_open(struct inode *inode, struct file *file)
{
#ifdef KPP_DEBUG
    printk("KPP: kdbI_open() called... \n");
#endif
   
    return 0;
}

/*!
 * This function is called to open /dev/keypadB, the device for getting
 * bitmaps of current keys down.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 *
 * @result    The function returns 0 on success and a non-zero value on
 *            failure.
 */
static int mxc_kppB_open(struct inode *inode, struct file *file)
{
#ifdef KPP_DEBUG
    printk("KPP: kdbI_open() called... \n");
#endif
   
    spin_lock(&keypad_lock);
    if (bitmap_opens > 0) 
    {
        spin_unlock(&keypad_lock);
        return -EBUSY;
    }
    bitmap_opens++;
   
    /*
     * poll returns when lastioctlbitmap is different from keybitmap.
     * we set lastioctlbitmap to keybitmap here so that a poll right
     * after an open returns when the bitmap is different from when
     * the open was done, not when the bitmap is different from the
     * last time some other process read it
     */
    memcpy (lastioctlbitmap, keybitmap, sizeof(keybitmap));
    spin_unlock(&keypad_lock);

    return 0;
}

/*!
 * This function is called to close the keypad device.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 *
 * @result    The function returns 0 on success and a non-zero value on 
 *            failure.
 *
 */
static int mxc_kpp_close(struct inode *inode, struct file *file)
{
#ifdef KPP_DEBUG
    printk("Kpp: Keypad close function is executed \n");
#endif
    reading_opens--;
    return 0;
}

/*!
 * This function is called to close /dev/keypadI.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 *
 * @result    The function returns 0 on success and a non-zero value on 
 *            failure.
 *
 */
static int mxc_kppI_close(struct inode *inode, struct file *file)
{
#ifdef KPP_DEBUG
    printk("Kpp: keypadI close function is executed \n");
#endif
    return 0;
}

/*!
 * This function is called to close /dev/keypadB.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 *
 * @result    The function returns 0 on success and a non-zero value on 
 *            failure.
 *
 */
static int mxc_kppB_close(struct inode *inode, struct file *file)
{
#ifdef KPP_DEBUG
    printk("Kpp: keypadI close function is executed \n");
#endif
    bitmap_opens--;
    return 0;
}

#ifdef CONFIG_PM

/*!
 * This function is called to put the keypad in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   dev   the device structure used to give information on keypad
 *                to suspend.
 * @param   state the power state the device is entering.
 * @param   level the stage in device suspension process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
static int mxc_kpp_suspend(struct device *dev, u32 state, u32 level)
{
#ifdef KPP_DEBUG
    printk("KPP:  MXC_Keypad suspend function.%08lX\n", level);
#endif

    if(reject_suspend)
    {
        return -EBUSY;
    }

    suspend = 1;

    return 0;
}

/*!
 * This function is called to bring the keypad back from a low power state.
 * Refer to the document driver-model/driver.txt in the kernel source tree
 * for more information.
 *
 * @param   dev   the device structure used to give information on keypad
 *                to resume.
 * @param   level the stage in device resumption process that we want the
 *                device to be put in.
 *
 * @return  The function always returns 0.
 */
static int mxc_kpp_resume(struct device *dev, u32 level)
{
#ifdef KPP_DEBUG
    printk("KPP: MXC_Keypad resume function.%08lX\n", level);
#endif

    suspend = 0;

    return 0;
}
#else

static int mxc_kpp_suspend(struct device *dev, u32 state, u32 level)
{
    return 0;
}

static int mxc_kpp_resume(struct device *dev, u32 level)
{
    return 0;
}

#endif /* CONFIG_PM */

/*!
 * This is power management callback function called in device driver 
 * structure.
 *
 * @param   dev   the device structure used to store device specific
 *                information that is used by the suspend, resume and remove
 *                functions
 *
 * @return  The function always returns 0.
 */
static int mxc_kpp_probe(struct device *dev)
{
#ifdef KPP_DEBUG
    printk("Probe function called.\n");
#endif
    return 0;
}

/*!
 * This function is called to put an event on the read queue
 * without verifying that it is a valid event and without putting
 * an EVENTINSERTED flag on it.
 *
 * @param    event    The event to put on the queue
 */
static inline void insert_raw_event (unsigned short event)
{
    int kpp_mode_save;

    kpp_mode_save = kpp_dev.kpp_mode;
    kpp_dev.kpp_mode = KEYPAD_RAW;
    mxc_kpp_handle_mode (event);
    kpp_dev.kpp_mode = kpp_mode_save;
}

/*!
 * This function is called to put an event on the read queue.
 *
 * @param    event    The event to put on the queue
 *
 * @result    The function returns 0 on success and a non-zero value on 
 *            failure.
 *
 */
static int insert_event (unsigned short event)
{
    if (KEYCODE(event) > KEYPAD_MAXCODE)
    {
        printk("KPP: inserted key code %d too big\n", KEYCODE(event));
        return -EINVAL;
    }

#if defined(CONFIG_MOT_FEAT_FLIP)
    if (KEYCODE(event) == KEYPAD_FLIP) 
    {
/* Nevis started as a slider and is changing to a flip, so in order to support
   both cases, this code must stay */    
#if defined(CONFIG_MACH_NEVIS)
        if(KEY_IS_DOWN(event))
        {
            if(event & KEYTRANSITION)
            {
                /* Flip is CLOSING */
                flip_status_handler(CLOSING, EVENTINSERTED);
            }
            else
            {
                /* Flip is CLOSED */
                flip_status_handler(CLOSE, EVENTINSERTED);
            }
        }
        else
        {
            if(event & KEYTRANSITION)
            {
                /* Flip is OPENING */
                flip_status_handler(OPENING, EVENTINSERTED);
            }
            else
            {
                /* Flip is OPEN */
                flip_status_handler(OPEN, EVENTINSERTED);
            }
        }
#else /* Not Nevis */
        if(KEY_IS_DOWN(event))
        {
            flip_status_handler(CLOSE, EVENTINSERTED);
        }
        else
        {
            flip_status_handler(OPEN, EVENTINSERTED);
        }
#endif /* Nevis */

        return 0;
    }
#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
    if (KEYCODE(event) == KEYPAD_SLIDER) 
    {
/* Nevis started as a slider and is changing to a flip, so in order to support
   both cases, this code must stay */   
#if defined(CONFIG_MACH_NEVIS)
        if(KEY_IS_DOWN(event))
        {
            if(event & KEYTRANSITION)
            {
                /* Slider is CLOSING */
                slider_status_handler(CLOSING, EVENTINSERTED);
            }
            else
            {
                /* Slider is CLOSED */
                slider_status_handler(CLOSE, EVENTINSERTED);
            }
        }
        else
        {
            if(event & KEYTRANSITION)
            {
                /* Slider is OPENING */
                slider_status_handler(OPENING, EVENTINSERTED);
            }
            else
            {
                /* Slider is OPEN */
                slider_status_handler(OPEN, EVENTINSERTED);
            }
        }
#else /* Not Nevis */
        if(KEY_IS_DOWN(event))
        {
            slider_status_handler(CLOSE, EVENTINSERTED);
        }
        else
        {
            slider_status_handler(OPEN, EVENTINSERTED);
        }
#endif /* Nevis */

        return 0;
    }
#endif /*defined(CONFIG_MOT_FEAT_SLIDER)*/

#if defined(CONFIG_MACH_ELBA)
    if (KEYCODE(event) == KEYPAD_LOCK) 
    {
		keylock_status_handler(KEY_IS_DOWN(event) ? CLOSE : OPEN, EVENTINSERTED);		
/*        if(KEY_IS_DOWN(event))
        {
            keylock_status_handler(CLOSE, EVENTINSERTED);
        }
        else
        {
            keylock_status_handler(OPEN, EVENTINSERTED);
        }
*/
        return 0;
    }
#elif defined( CONFIG_MACH_XPIXL)
    if (KEYCODE(event) == KEYPAD_LOCK) 
    {
        sidekey_status_handler(&keylock_data, KEY_IS_DOWN(event) ? CLOSE : OPEN, EVENTINSERTED);
        return 0;
    }
    else if(KEYCODE(event) == KEYPAD_LENS_COVER)
    {
        sidekey_status_handler(&lenscover_data, KEY_IS_DOWN(event) ? CLOSE : OPEN, EVENTINSERTED);
        return 0;
    }
#endif
    event |= EVENTINSERTED;

    insert_raw_event(event);
    return 0;
}

/*!
 * This function is called to put an ascii key event on the read queue.
 *
 * @param    event    The event to put on the queue
 *
 * @result    The function returns 0 on success and a non-zero value on 
 *            failure.
 *
 */
static int insert_ascii_event (unsigned short event)
{
    if (event > 0xff) 
    {
        printk("KPP: inserted ascii code 0x%x too big\n", event);
        return -EINVAL;
    }

    event |= EVENTINSERTED;
    event |= ASCII_INSERTED;

    insert_raw_event(event);
    return 0;
}

#ifdef CONFIG_MACH_XPIXL
/*! This is used by the set mapping table ioctl command to set the keymap. 
 */
static void set_keymap(MORPHING_MODE_E mode)
{
    switch (mode)
    {
        default: /* xPIXL defaults to phone mode. */
        case MORPHING_MODE_PHONE:
            key_map = keymap_phone;
            break;
        case MORPHING_MODE_PHONE_WITHOUT_REVIEW:
            key_map = keymap_phone_noreview;
            break;
        case MORPHING_MODE_STANDBY:
            key_map = keymap_none;
            break;
        case MORPHING_MODE_NAVIGATION:
            key_map = keymap_cam_nav_only;
            break;
        case MORPHING_MODE_STILL_VIDEO_CAPTURE:
            key_map = keymap_cam_toggle;
            break;
        case MORPHING_MODE_STILL_VIDEO_PLAYBACK:
            key_map = keymap_cam_all;
            break;
        case MORPHING_MODE_STILL_VIDEO_PLAYBACK_WITHOUT_SHARE:
            key_map = keymap_cam_noshare;
            break;
        case MORPHING_MODE_STILL_VIDEO_PLAYBACK_WITHOUT_TRASH:
            key_map = keymap_cam_notrash;
            break;
        case MORPHING_MODE_STILL_VIDEO_PLAYBACK_WITHOUT_TOGGLE:
            key_map = keymap_cam_notoggle;
            break;
        case MORPHING_MODE_SHARE:
            key_map = keymap_cam_share;
            break;
        case MORPHING_MODE_TRASH:
            key_map = keymap_cam_trash;
            break;
        case MORPHING_MODE_TEST:
            key_map = keymap_all;
            break;
    }
}
#endif

/*!
 * This function is called when a ioctl call is made from user space.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 * @param    cmd      Ioctl command
 * @param    arg      Ioctl argument
 *
 *                    Following are the ioctl commands for user to use:\n
 *                    KEYPAD_IOC_INSERT_EVENT : Inserts a keypad event as if
 *                    it were read from the keypad hardware.
 *                    KEYPAD_IOC_GET_AUTOREPEAT : Get information about 
 *                    autorepeat.  The argument is a pointer to struct
 *                    autorepeat.
 *                    KEYPAD_IOC_SET_AUTOREPEAT : Set information about 
 *                    autorepeat.  The argument is a pointer to struct
 *                    autorepeat.
 *                    KEYPAD_IOC_GET_RECENT_KEY : Get the most recently
 *                    read key event. The argument is unsigned short *.
 *                    KEYPAD_IOC_GETBITMAP : Get a bitmap of the keys that
 *                    are currently down.  The argument is a pointer to an
 *                    array of longs.
 *                    KEYPAD_IOC_SET_FLIP_STATUS : Set the status of the flip.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_FLIP_STATUS : Get the status of the flip.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_SLIDER_STATUS : Set the status of the slider.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_SLIDER_STATUS : Get the status of the slider.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_KEYLOCK_STATUS : Set the status of the lock key.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_KEYLOCK_STATUS : Get the status of the lock key.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_LENS_COVER_STATUS : Set the status of the lens cover.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_LENS_COVER_STATUS : Get the status of the lens cover.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_MAPPING_TABLE : Set the keymap.
 *                    The argument is MORPHING_MODE_E.
 *
 * @result    The function returns 0 on success and a non-zero value on 
 *            failure.
 */
int mxc_kpp_ioctl(struct inode *inode, struct file * file, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct autorepeatinfo ar;
#if defined(CONFIG_MOT_FEAT_SLIDER) || defined(CONFIG_MOT_FEAT_FLIP) || defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_XPIXL)
    STATUS_E status = (STATUS_E)arg;
#endif                         
#ifdef KPP_DEBUG
    printk("KPP: Keypad ioctl function called (cmd %x arg 0x%08lX)\n", cmd, arg);
#endif

    if (suspend == 1)      
    {
        printk("KPP: Keypad driver is in suspend mode.\n");
        return -EPERM;
    } 
    else 
    {
        switch (cmd) 
        {
            case KEYPAD_IOC_GETBITMAP:
                spin_lock_irq(&keypad_lock);
                ret = copy_to_user ((void *)arg, keybitmap, NUM_WORDS_IN_BITMAP * sizeof(unsigned long));
                spin_unlock_irq(&keypad_lock);
                return (ret != 0) ? -EFAULT : 0;
                break;

            case KEYPAD_IOC_INSERT_EVENT:
                return insert_event ((unsigned short)(arg));
                break;

            case KEYPAD_IOC_GET_AUTOREPEAT:
                ar.r_repeat = kpp_dev.autorepeat_info.r_repeat;
            
                /* convert from jiffies to milliseconds */
                ar.r_time_to_first_repeat = kpp_dev.autorepeat_info.r_time_to_first_repeat * 1000/HZ;
                ar.r_time_between_repeats =  kpp_dev.autorepeat_info.r_time_between_repeats * 1000/HZ;

                if (copy_to_user ((void *)arg, &ar, sizeof(struct autorepeatinfo)) != 0)
                {
                    return -EFAULT;
                }
                break;
                
            case KEYPAD_IOC_SET_AUTOREPEAT:
                if (copy_from_user (&kpp_dev.autorepeat_info, (void *)arg, sizeof(struct autorepeatinfo)) != 0)
                {
                    return -EFAULT;
                }
  
                if (kpp_dev.autorepeat_info.r_repeat == 0)
                {
                    del_timer (&kpp_dev.autorepeat_timer);
                    del_timer(&headset_timer);
                    del_timer(&emu_headset_timer);
                }
	  
                /* times are specified in milliseconds; convert to jiffies */
                kpp_dev.autorepeat_info.r_time_to_first_repeat = (kpp_dev.autorepeat_info.r_time_to_first_repeat * HZ)/1000;
                kpp_dev.autorepeat_info.r_time_between_repeats = (kpp_dev.autorepeat_info.r_time_between_repeats * HZ)/1000;
                 
                break;

            case KEYPAD_IOC_GET_RECENT_KEY:  /* recent key value */
                return put_user(kpp_dev.recent_key_value, (unsigned short *)arg);
                break;

#if defined(CONFIG_MOT_FEAT_FLIP)
            case KEYPAD_IOC_GET_FLIP_STATUS: /* get flip status */
                return put_user(kpp_dev.status, (STATUS_E *)arg);
                break;

            case KEYPAD_IOC_SET_FLIP_STATUS: /* set flip status */
                if(status >= INVALID)
                {
                    return -EINVAL;
                }
                flip_status_handler(status, 0);
                break;
#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
            case KEYPAD_IOC_GET_SLIDER_STATUS: /* get slider status */
                return put_user(kpp_dev.status, (STATUS_E *)arg);
                break;

            case KEYPAD_IOC_SET_SLIDER_STATUS: /* set slider status */
                if(status >= INVALID)
                {
                    return -EINVAL;
                }
                slider_status_handler(status, 0);
                break;
#endif

#if defined(CONFIG_MACH_ELBA)
            case KEYPAD_IOC_GET_KEYLOCK_STATUS: /* get keylock status */
                return put_user(kpp_dev.status, (STATUS_E *)arg);
                break;

            case KEYPAD_IOC_SET_KEYLOCK_STATUS: /* set keylock status */
                if ((status != CLOSE) && (status != OPEN)) 
                {
                    return -EINVAL;
                }
                keylock_status_handler(status, 0);
                break;
#elif defined(CONFIG_MACH_XPIXL)
            case KEYPAD_IOC_GET_KEYLOCK_STATUS: /* get keylock status */
                return put_user(keylock_data.status, (STATUS_E *)arg);
                break;
            case KEYPAD_IOC_SET_KEYLOCK_STATUS: /* set keylock status */
                if (status != CLOSE && status != OPEN)
                {
                    return -EINVAL;
                }
                sidekey_status_handler(&keylock_data, status, 0);
                break;
            case KEYPAD_IOC_GET_LENS_COVER_STATUS: /* get lens cover status */
                return put_user(lenscover_data.status, (STATUS_E *)arg);
                break;
            case KEYPAD_IOC_SET_LENS_COVER_STATUS: /* set lens cover status */
                if (status != CLOSE && status != OPEN)
                {
                    return -EINVAL;
                }
                sidekey_status_handler(&lenscover_data, status, 0);
                break;
            case KEYPAD_IOC_SET_MAPPING_TABLE:
                set_keymap((MORPHING_MODE_E)arg);
                break;
#endif /* xPIXL */
            case KEYPAD_IOC_POWER_SAVE:
            case KEYPAD_IOC_POWER_RESTORE:
                printk(KERN_WARNING "POWER ioctls not supported yet.\n");
                break;
            default:
#ifdef KPP_DEBUG
                printk("KPP: ioctl: unknown cmd=%d,\n", cmd);
#endif
                return  -EINVAL;
                break;
        }

#ifdef KPP_DEBUG
        printk("KPP: ioctl done .....\n");
#endif
    }
    return 0;
}

/*!
 * This function is called for an ioctl on /dev/keypadI or /dev/keypadB.
 *
 * @param    inode    Pointer to device inode 
 * @param    file     Pointer to device file structure
 * @param    cmd      Ioctl command
 * @param    arg      Ioctl argument
 *
 *                    Following are the ioctl commands for user to use:
 *                    KEYPAD_IOC_INSERT_EVENT : Inserts a keypad event as if
 *                    it were read from the keypad hardware.
 *                    KEYPAD_IOC_INSERT_ASCII_EVENT : Inserts an ascii key
 *                    code into the keypad read queue.
 *                    KEYPAD_IOC_GETBITMAP : Get a bitmap of the keys that
 *                    are currently down.  The argument is a pointer to an
 *                    array of longs.
 *                    KEYPAD_IOC_GET_RECENT_KEY : Get the most recently
 *                    read key event. The argument is unsigned short *.
 *                    KEYPAD_IOC_SET_FLIP_STATUS : Set the status of the flip.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_FLIP_STATUS : Get the status of the flip.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_SLIDER_STATUS : Set the status of the slider.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_SLIDER_STATUS : Get the status of the slider.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_KEYLOCK_STATUS : Set the status of the lock key.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_KEYLOCK_STATUS : Get the status of the lock key.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_LENS_COVER_STATUS : Set the status of the lens cover.
 *                    The argument is STATUS_E.
 *                    KEYPAD_IOC_GET_LENS_COVER_STATUS : Get the status of the lens cover.
 *                    The argument is STATUS_E *.
 *                    KEYPAD_IOC_SET_MAPPING_TABLE : Set the keymap.
 *                    The argument is MORPHING_MODE_E.
 *
 * @result    The function returns 0 on success and a non-zero value on 
 *            failure.
 *
 */
int mxc_kppIB_ioctl(struct inode *inode, struct file * file, unsigned int cmd, unsigned long arg)
{
    int ret;
    unsigned int minor = MINOR(file->f_dentry->d_inode->i_rdev);
#if defined(CONFIG_MOT_FEAT_SLIDER) || defined(CONFIG_MOT_FEAT_FLIP) || defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_XPIXL)
    STATUS_E status = (STATUS_E)arg;
#endif
#ifdef KPP_DEBUG
    printk("KPP: KeypadIB ioctl function called (cmd %x arg 0x%08lX)\n", cmd, arg);
#endif

    if (suspend == 1)      
    {
        printk("KPP: Keypad driver is in suspend mode.\n");
        return -EPERM;
    } 
    else 
    {
        switch (cmd) 
        {
            case KEYPAD_IOC_GETBITMAP:
                spin_lock_irq(&keypad_lock);
                ret = copy_to_user ((void *)arg, keybitmap, NUM_WORDS_IN_BITMAP * sizeof(unsigned long));
                
                if (minor == Bminor) 
                {
                    memcpy (lastioctlbitmap, keybitmap, sizeof(keybitmap));
                }

                spin_unlock_irq(&keypad_lock);
                return (ret != 0) ? -EFAULT : 0;
                break;

            case KEYPAD_IOC_INSERT_EVENT:
                return insert_event ((unsigned short)(arg));
                break;

            case KEYPAD_IOC_INSERT_ASCII_EVENT:
                return insert_ascii_event ((unsigned short)(arg));
                break;

            case KEYPAD_IOC_GET_RECENT_KEY:  /* recent key value */
                return put_user(kpp_dev.recent_key_value, (unsigned short *)arg);
                break;

#if defined(CONFIG_MOT_FEAT_FLIP)
            case KEYPAD_IOC_GET_FLIP_STATUS: /* get flip status */
                return put_user(kpp_dev.status, (STATUS_E *)arg);
                break;

            case KEYPAD_IOC_SET_FLIP_STATUS: /* set flip status */
                if(status >= INVALID)
                {
                    return -EINVAL;
                }
                flip_status_handler(status, 0);
                break;
#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
            case KEYPAD_IOC_GET_SLIDER_STATUS: /* get slider status */
                return put_user(kpp_dev.status, (STATUS_E *)arg);
                break;

            case KEYPAD_IOC_SET_SLIDER_STATUS: /* set slider status */
                if(status >= INVALID)
                {
                    return -EINVAL;
                }
                slider_status_handler(status, 0);
                break;
#endif

#if defined(CONFIG_MACH_ELBA)
            case KEYPAD_IOC_GET_KEYLOCK_STATUS: /* get keylock status */
                return put_user(kpp_dev.status, (STATUS_E *)arg);
                break;

            case KEYPAD_IOC_SET_KEYLOCK_STATUS: /* set keylock status */
                if ((status != CLOSE) && (status != OPEN)) 
                {
                    return -EINVAL;
                }
                keylock_status_handler(status, 0);
                break;
#elif defined( CONFIG_MACH_XPIXL)
            case KEYPAD_IOC_GET_KEYLOCK_STATUS: /* get keylock status */
                return put_user(keylock_data.status, (STATUS_E *)arg);
                break;
            case KEYPAD_IOC_SET_KEYLOCK_STATUS: /* set keylock status */
                if (status != CLOSE && status != OPEN)
                {
                    return -EINVAL;
                }
                sidekey_status_handler(&keylock_data, status, 0);
                break;
            case KEYPAD_IOC_GET_LENS_COVER_STATUS: /* get lens cover status */
                return put_user(lenscover_data.status, (STATUS_E *)arg);
                break;
            case KEYPAD_IOC_SET_LENS_COVER_STATUS: /* set lens cover status */
                if (status != CLOSE && status != OPEN)
                {
                    return -EINVAL;
                }
                sidekey_status_handler(&lenscover_data, status, 0);
                break;
            case KEYPAD_IOC_SET_MAPPING_TABLE:
                set_keymap((MORPHING_MODE_E)arg);
                break;
#endif
            default:
#ifdef KPP_DEBUG
                printk("KPP: ioctl: unknown cmd=%d,\n", cmd);
#endif
                return  -EINVAL;
                break;
        }

#ifdef KPP_DEBUG
        printk("KPP: ioctl done .....\n");
#endif
    }
    return 0;
}

/*!
 * This function is called when read system call is executed.
 *
 *  @param  file        Pointer to device file structure
 *  @param  buf         User buffer, where read data to be placed.
 *  @param  count       Size of the requested data transfer
 *  @param  pos         File position where the user is accessing.
 *
 *  @result      Number of bytes read.
 */
static ssize_t mxc_kpp_read(struct file * file, char * buf, size_t count, loff_t *pos)
{
    ssize_t i = count;
    unsigned short c;
    int ret;

#ifdef KPP_DEBUG
    printk("KPP: read called....\n");
    printk("value of read count %d\n", i);
#endif
    if (suspend == 1)      
    {
        printk("KPP: Keypad driver is in suspend mode.\n");
        return -EPERM;
    } 
    else 
    {
        if (count == 0) 
        {
            return 0;
        }

        /* 
         * If we don't have any data in queue and if we are in 
         * NONBLOCK mode return error to user.  If we are not
         * in NONBLOCK mode, go to interruptible sleep until data 
         * is available.
         */
        if (mxc_kpp_q_empty() && (file->f_flags & O_NONBLOCK)) 
        {
            return -EAGAIN;
        }

        /* No data and blocking read */
        while (mxc_kpp_q_empty()) 
        {
            if (wait_event_interruptible(kpp_dev.queue.waitq,(kpp_dev.queue.head != kpp_dev.queue.tail))) 
            {
                return -ERESTARTSYS; 
            }
        }

        /*
         * Read data till the number of bytes user has requested
         * or we run out.  We stop when i-1>0 instead of i>0 because
         * we need to return an even number of bytes.
         */
        while ( ((i-1) > 0) && !mxc_kpp_q_empty() ) 
        {
            c = mxc_kpp_pullq();
#ifdef KPP_DEBUG
            printk("KPP: data from queue 0x%x\n", c);
#endif
            if ((ret = put_user(c, (unsigned short *)buf)) != 0) 
            {
                return ret;
            }
            
            buf += EVENTSIZE;
            i -= EVENTSIZE;
        } 

        if (signal_pending(current))    
        {
            return -ERESTARTSYS;
        }
    }

    /* Change the file access time */
    file->f_dentry->d_inode->i_atime = CURRENT_TIME;
    return count-i;
}

/*!
 * This function is called to check if any data is available. This is called
 * by the kernel poll functionality.
 *
 * @param    file      Pointer to device file structure
 * @param    wait      Poll table structure is used within the kernel to 
 *                     implement the poll and select system calls; it is 
 *                     declared in linux/poll.h.
 *
 * @result      Bit mask events POLLIN or POLLRDNORM if data is available in
 *              keypad queue, else returns zero.
 */
static unsigned int mxc_kpp_poll(struct file *file, poll_table *wait)
{
#ifdef KPP_DEBUG
    printk("Kpp:polling function called .....\n");
    printk("Value of queue.head ... %d\n", kpp_dev.queue.head);
    printk("value of queue.tail ... %d\n", kpp_dev.queue.tail);
#endif
    poll_wait(file, &kpp_dev.queue.waitq, wait);
    return (!mxc_kpp_q_empty()) ? (POLLIN | POLLRDNORM) : 0;
}

/*!
 * This function is called to check if the bitmap of current keys down
 * has changed since the last time it was accessed.
 *
 * @param    file      Pointer to device file structure
 * @param    wait      Poll table structure is used within the kernel to 
 *                     implement the poll and select system calls; it is 
 *                     declared in linux/poll.h.
 *
 * @result      Bit mask events POLLIN or POLLRDNORM if data is available in
 *              keypad queue, else returns zero.
 */
static unsigned int mxc_kppB_poll(struct file *file, poll_table *wait)
{
    int i;

#ifdef KPP_DEBUG
    printk("KppB:polling function called .....\n");
#endif
    poll_wait(file, &kpp_dev.queue.waitq, wait);
        
    for (i=0; i < NUM_WORDS_IN_BITMAP; i++) 
    {
        if (lastioctlbitmap[i] != keybitmap[i]) 
        {
            return (POLLIN | POLLRDNORM);
        }
    }
    return 0;
}

/*!
 * This is called from elsewhere in the kernel when a key event needs to be
 * generated via software (e.g. capacitive touch keys)
 *
 * @param    keycode    The keycode for the event, as defined in 
 *                      include/linux/keypad.h
 * @param    up_down    KEYDOWN if the key is pressed, KEYUP if the key is 
 *                      released
 *
 *                      For Nevis, KEYTRANSITION and KEYDOWN if slider is closing,
 *                      KEYTRANSITION and KEYUP if slider is opening.
 *
 * @return   -EINVAL    Either the keycode is out-of-bounds (i.e. above 
 *                      MAX_KEYCODE), or the up_down is invalid (i.e. 
 *                      neither KEYDOWN nor KEYUP)
 *           0          The keycode was successfully generated
 */
int generate_key_event(unsigned short keycode, unsigned short up_down)
{
    unsigned long  flags;
   
    if ((up_down != KEYUP) && (up_down != KEYDOWN))
    {
        printk(KERN_ERR "KPP: Attempted to generate key_event w/ invalid up_down: %x\n", up_down);
        return -EINVAL;
    }

    if (keycode > KEYPAD_MAXCODE)
    {
        printk("KPP: inserted key code %d too big\n", keycode);
        return -EINVAL;
    }

#if defined(CONFIG_MOT_FEAT_FLIP)
    if (keycode == KEYPAD_FLIP) 
    {
#if defined(CONFIG_MACH_NEVIS)
        if(KEY_IS_DOWN(up_down))
        {
            if(up_down & KEYTRANSITION)
            {
                /* Flip is CLOSING */
                flip_status_handler(CLOSING, 0);
            }
            else
            {
                /* Flip is CLOSED */
                flip_status_handler(CLOSE, 0);
            }
        }
        else
        {
            if(up_down & KEYTRANSITION)
            {
                /* Flip is OPENING */
                flip_status_handler(OPENING, 0);
            }
            else
            {
                /* Flip is OPEN */
                flip_status_handler(OPEN, 0);
            }
        }
#else /* Not Nevis */
        if(KEY_IS_DOWN(up_down))
        {
            flip_status_handler(CLOSE, 0);
        }
        else
        {
            flip_status_handler(OPEN, 0);
        }
#endif /* Nevis */

        return 0;
    }
#endif /*defined(CONFIG_MOT_FEAT_FLIP)*/

#if defined(CONFIG_MOT_FEAT_SLIDER)
    if (keycode == KEYPAD_SLIDER) 
    {
#if defined(CONFIG_MACH_NEVIS)
        if(KEY_IS_DOWN(up_down))
        {
            if(up_down & KEYTRANSITION)
            {
                /* Slider is CLOSING */
                slider_status_handler(CLOSING, 0);
            }
            else
            {
                /* Slider is CLOSED */
                slider_status_handler(CLOSE, 0);
            }
        }
        else
        {
            if(up_down & KEYTRANSITION)
            {
                /* Slider is OPENING */
                slider_status_handler(OPENING, 0);
            }
            else
            {
                /* Slider is OPEN */
                slider_status_handler(OPEN, 0);
            }
        }
#else /* Not Nevis */
        if(KEY_IS_DOWN(up_down))
        {
            slider_status_handler(CLOSE, 0);
        }
        else
        {
            slider_status_handler(OPEN, 0);
        }
#endif /* Nevis */
        return 0;
    }
#endif


#if defined(CONFIG_MACH_ELBA)
    if (keycode == KEYPAD_LOCK) 
    {
		keylock_status_handler(KEY_IS_DOWN(up_down) ? CLOSE : OPEN, 0);
/*        if(KEY_IS_DOWN(up_down))
        {
            keylock_status_handler(CLOSE, 0);
        }
        else
        {
            keylock_status_handler(OPEN, 0);
        }
*/
        return 0;
    }
#elif defined(CONFIG_MACH_XPIXL)
    else if (keycode == KEYPAD_LOCK)
    {
        sidekey_status_handler(&keylock_data, KEY_IS_DOWN(up_down) ? CLOSE : OPEN, 0);
        return 0;
    }
    else if (keycode == KEYPAD_LENS_COVER)
    {
        sidekey_status_handler(&lenscover_data, KEY_IS_DOWN(up_down) ? CLOSE : OPEN, 0);
        return 0;
    }
#endif
    /* do autorepeat for KEYPAD_HEADSET key*/
    if(keycode == KEYPAD_HEADSET)
    {
        /* Press */
        if(up_down == KEYDOWN)
        {
            spin_lock_irqsave(&autorepeat_lock, flags);

            headset_scancode = keycode | up_down;

            del_timer(&headset_timer);
            
            if(kpp_dev.autorepeat_info.r_repeat)
            {
                headset_timer.expires = jiffies + kpp_dev.autorepeat_info.r_time_to_first_repeat;
                add_timer(&headset_timer);
            }
            
            spin_unlock_irqrestore(&autorepeat_lock, flags);
        }
        else if(up_down == KEYUP) /* release */
        {
            spin_lock_irqsave(&autorepeat_lock, flags);
            headset_scancode = 0xFFFF;
            del_timer(&headset_timer);
            spin_unlock_irqrestore(&autorepeat_lock, flags);
        }
    }


/* do autorepeat for KEYPAD_EMU_HEADSET key*/
    if(keycode == KEYPAD_EMU_HEADSET)
    {
	/* Press */
	if(up_down == KEYDOWN)
	{
		spin_lock_irqsave(&autorepeat_lock, flags);

		emu_headset_scancode = keycode | up_down;

		del_timer(&emu_headset_timer);
		
		if(kpp_dev.autorepeat_info.r_repeat)
		{
			emu_headset_timer.expires = jiffies + kpp_dev.autorepeat_info.r_time_to_first_repeat;
			add_timer(&emu_headset_timer);
		}
		
		spin_unlock_irqrestore(&autorepeat_lock, flags);
	}
	else if(up_down == KEYUP) /* release */
	{
		spin_lock_irqsave(&autorepeat_lock, flags);
		emu_headset_scancode = 0xFFFE;
		del_timer(&emu_headset_timer);
		spin_unlock_irqrestore(&autorepeat_lock, flags);
	}
    }
#if defined(CONFIG_MOT_FEAT_FLIP) || defined(CONFIG_MACH_NEVIS)
    /* If the power key is pressed when the flip is closed, or if the product is Nevis,
       do not add the keycode to the queue. (Nevis is a slider but power key is under the slider) */
    if((keycode == KEYPAD_HANGUP) && (kpp_dev.status == CLOSE))
    {
        /* Ignore event */
    }
    else
#endif
    {
        insert_raw_event (keycode | up_down);
    }
    return 0;
}

void* register_keyevent_callback (void (*notify_function)(unsigned short, void*), void* user_data)
{
    keyeventcb*   newkeyeventcb;
    unsigned long flags;

    if(!notify_function)
    {
        return NULL;
    }

    newkeyeventcb = (keyeventcb*)kmalloc(sizeof(keyeventcb), GFP_KERNEL);
    if(!newkeyeventcb)
    {
        return NULL;
    }

    newkeyeventcb->notify_function   = notify_function;
    newkeyeventcb->user_data         = user_data;
    newkeyeventcb->previous_callback = NULL;

    spin_lock_irqsave(&keyeventcb_lock, flags);

    if(keyeventcb_chain != NULL)
    {
        keyeventcb_chain->previous_callback = newkeyeventcb;
    }

    newkeyeventcb->next_callback = keyeventcb_chain;
    keyeventcb_chain             = newkeyeventcb;

    spin_unlock_irqrestore(&keyeventcb_lock, flags);

    return (void*)newkeyeventcb;
}

void unregister_keyevent_callback (void* keyeventcbid)
{
    keyeventcb*   freekeyeventcb;
    keyeventcb*   previouscb;
    keyeventcb*   nextcb;
    unsigned long flags;

    if(keyeventcbid == NULL)
    {
        return;
    }

    freekeyeventcb = (keyeventcb*)keyeventcbid;

    spin_lock_irqsave(&keyeventcb_lock, flags);

    nextcb     = freekeyeventcb->next_callback;
    previouscb = freekeyeventcb->previous_callback;

    if(previouscb != NULL)
    {
        previouscb->next_callback = nextcb;
    }
    else
    {
        keyeventcb_chain = nextcb;
    }

    if(nextcb != NULL)
    {
        nextcb->previous_callback = previouscb;
    }

    spin_unlock_irqrestore(&keyeventcb_lock, flags);

    kfree(freekeyeventcb);
}

#if defined(CONFIG_MOT_FEAT_FLIP)
/*!
 * handle a flip event
 */
static inline void flip_status_handler(STATUS_E status, unsigned short event)
{
    /* enable/disable rows/columns */
    column_row_enabler(status);
    
    if(flip_state != status)
    {
        switch (status) 
        {
            case CLOSE:
#if defined(CONFIG_MACH_NEVIS)
                /* If the previous state was OPEN, send CLOSING then CLOSED */
                if(flip_state == OPEN)
                {
                    insert_raw_event(KEYDOWN | KEYTRANSITION | KEYPAD_FLIP | event);
                }
#endif /* Nevis */
                insert_raw_event(KEYDOWN | KEYPAD_FLIP | event);
#ifdef KPP_DEBUG
                printk("KPP: keypad flip closed\n");
#endif
                break;
            
            case OPEN:
#if defined(CONFIG_MACH_NEVIS)
                /* If the previous state was CLOSED, send OPENING then OPEN */
                if(flip_state == CLOSE)
                {
                    insert_raw_event(KEYUP | KEYTRANSITION | KEYPAD_FLIP | event);
                }
#endif /* Nevis */
                insert_raw_event(KEYUP | KEYPAD_FLIP | event);
#ifdef KPP_DEBUG
                printk("KPP: keypad flip open\n");
#endif
                break;
  
#if defined(CONFIG_MACH_NEVIS)
            case CLOSING:
                /* If the previous state was OPEN, send CLOSING */
                if(flip_state == OPEN)
                {
                    insert_raw_event(KEYDOWN | KEYTRANSITION | KEYPAD_FLIP | event);
                }

#ifdef KPP_DEBUG
                printk("KPP: keypad flip closing\n");
#endif                
                break;
            
            case OPENING:
                /* If the previous state was CLOSED, send OPENING */
                if(flip_state == CLOSE)
                {
                    insert_raw_event(KEYUP | KEYTRANSITION | KEYPAD_FLIP | event);
                }

#ifdef KPP_DEBUG
                printk("KPP: keypad flip opening\n");
#endif                
                break;
#endif /* Nevis */
  
            default:
                break;
        }
        
        flip_state = status;
    }
}

static inline void flip_status_check(void)
{
#if defined(CONFIG_MACH_NEVIS)
    STATUS_E temp = flip_slider_get_status();
    printk("KPP: Flip status is set to %d\n", temp);
    if(temp != INVALID)
    {
        flip_status_handler(temp, 0);
    }

#else /* Not Nevis */
    if (!gpio_flip_open()) 
    {
        flip_status_handler(CLOSE, 0); /* flip closed */
    }
    else 
    {
        flip_status_handler(OPEN, 0);
    }
#endif /* Nevis */
}

/* when switch timeout, call this */
static void flip_timer_handler(unsigned long unused)
{
    flip_status_check();
    
    /* Allow suspend */
    reject_suspend = 0;
    
    wake_up_interruptible(&kpp_dev.queue.waitq);
}

static irqreturn_t flip_irq_handler(int irq, void *ptr, struct pt_regs *regs)
{
    struct timer_list *t = &(kpp_dev.timer);

    /* Dont allow suspend */
    reject_suspend = 1;
            
    gpio_flip_clear_int ();
    del_timer(t);
    t->expires = jiffies + 10;
    add_timer(t);

    return IRQ_RETVAL(1);
}

#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
/*!
 * handle a slider event
 */
static inline void slider_status_handler(STATUS_E status, unsigned short event)
{
    /* enable/disable rows/columns */
    column_row_enabler(status);

    if(slider_state != status)
    {
        switch (status) 
        {
            case CLOSE:
#if defined(CONFIG_MACH_NEVIS)
                /* If the previous state was OPEN, send CLOSING then CLOSED */
                if(slider_state == OPEN)
                {
                    insert_raw_event(KEYDOWN | KEYTRANSITION | KEYPAD_SLIDER | event);
                }
#endif /* Nevis */
                insert_raw_event(KEYDOWN | KEYPAD_SLIDER | event);
                
#ifdef KPP_DEBUG
                printk("KPP: keypad slider closed\n");
#endif
                break;
  
            case OPEN:
#if defined(CONFIG_MACH_NEVIS)
                /* If the previous state was CLOSED, send OPENING then OPEN */
                if(slider_state == CLOSE)
                {
                    insert_raw_event(KEYUP | KEYTRANSITION | KEYPAD_SLIDER | event);
                }
#endif /* Nevis */
                insert_raw_event(KEYUP | KEYPAD_SLIDER | event);

#ifdef KPP_DEBUG
                printk("KPP: keypad slider open\n");
#endif
                break;
   
#if defined(CONFIG_MACH_NEVIS)
            case CLOSING:
                /* If the previous state was OPEN, send CLOSING */
                if(slider_state == OPEN)
                {
                    insert_raw_event(KEYDOWN | KEYTRANSITION | KEYPAD_SLIDER | event);
                }

#ifdef KPP_DEBUG
                printk("KPP: keypad slider closing\n");
#endif                
                break;
            
            case OPENING:
                /* If the previous state was CLOSED, send OPENING */
                if(slider_state == CLOSE)
                {
                    insert_raw_event(KEYUP | KEYTRANSITION | KEYPAD_SLIDER | event);
                }

#ifdef KPP_DEBUG
                printk("KPP: keypad slider opening\n");
#endif                
                break;
#endif /* Nevis */
   
            default:
                break;
        }
        
        slider_state = status;
    }
}

static inline void slider_status_check(void)
{
#if defined(CONFIG_MACH_NEVIS)
    STATUS_E temp = flip_slider_get_status();
    printk("KPP: Slider status is set to %d\n", temp);
    if(temp != INVALID)
    {
        slider_status_handler(temp, 0);
    }

#else /* Not Nevis */
    if (!gpio_slider_open()) 
    {
        slider_status_handler(CLOSE, 0); /* slider closed */
    }
    else 
    {
        slider_status_handler(OPEN, 0);
    }
#endif /* Nevis */
}

/* when switch timeout, call this */
static void slider_timer_handler(unsigned long unused)
{
    slider_status_check();
    
    /* Allow suspend */
    reject_suspend = 0;
    
    wake_up_interruptible(&kpp_dev.queue.waitq);
}

static irqreturn_t slider_irq_handler(int irq, void *ptr, struct pt_regs *regs)
{
    struct timer_list *t = &(kpp_dev.timer);

    /* Dont allow suspend */
    reject_suspend = 1;
    
    del_timer(t);
    t->expires = jiffies + 10;
    add_timer(t);

    return IRQ_RETVAL(1);
}
#endif

#if defined(CONFIG_MACH_ELBA)
/*!
 * handle a keylock event
 */
static inline void keylock_status_handler(STATUS_E status, unsigned short event)
{
    /* enable/disable rows/columns */
    column_row_enabler(status);

    switch (status) 
    {
        case CLOSE:
            insert_raw_event(KEYDOWN | KEYPAD_LOCK | event);
#ifdef KPP_DEBUG
            printk("KPP: keypad locked\n");
#endif
            break;
  
        case OPEN:
            insert_raw_event(KEYUP | KEYPAD_LOCK | event);
#ifdef KPP_DEBUG
            printk("KPP: keypad unlocked\n");
#endif
            break;
   
        default:
            break;
    }
}

static inline void keylock_status_check(void)
{
    if (!gpio_keylock_open()) 
    {
        keylock_status_handler(CLOSE, 0); /* key unlocked */
    }
    else 
    {
        keylock_status_handler(OPEN, 0);
    }
}

/* when switch timeout, call this */
static void keylock_timer_handler(unsigned long unused)
{
    keylock_status_check();
    wake_up_interruptible(&kpp_dev.queue.waitq);
}

static irqreturn_t keylock_irq_handler(int irq, void *ptr, struct pt_regs *regs)
{
    struct timer_list *t = &(kpp_dev.timer);
        
    del_timer(t);
    t->expires = jiffies + 10;
    add_timer(t);

    return IRQ_RETVAL(1);
}

#elif defined(CONFIG_MACH_NEVIS)
/*!
 * Determine the state of the flip/slider
 *
 * @return The state of the slider
 */
static STATUS_E flip_slider_get_status(void)
{
    int open;
    int close;
    STATUS_E ret = INVALID;
    
    open = edio_get_data(ED_INT0);
    close = edio_get_data(ED_INT3);
    
    /* If OPEN = 0 and CLOSE = 1, then slider is OPEN */
    if(!open && close)
    {
        ret = OPEN;
    }
    /* If OPEN = 1 and CLOSE = 0, then slider is CLOSED */
    else if(open && !close)
    {
        ret = CLOSE;
    }
    /* If OPEN = 1 and CLOSE = 1, then flip/slider is transitioning */
    else if(open && close)
    {
#if defined(CONFIG_MOT_FEAT_SLIDER)
        if(slider_state == OPEN)
#elif defined(CONFIG_MOT_FEAT_FLIP)
        if(flip_state == OPEN)
#endif        
        {
            ret = CLOSING;
        }
        else
        {
            ret = OPENING;
        }
    }
    
    
    return ret;    
}
#endif /* Nevis */

#ifdef CONFIG_MACH_XPIXL
/*!
 * Check for a sidekey status change
 *
 * @param data  Keylock or lens cover data.
 */
static void sidekey_status_check(SIDEKEY_T* data)
{
    sidekey_status_handler(data, data->read_status(), 0);
}

/*!
 * Handle a sidekey status change
 *
 * @param data      The data associated with a particular sidekey input.
 * @param status    The new status.
 * @param event     A key event to be filled in, maybe with flags already set.
 */
static void sidekey_status_handler(SIDEKEY_T* data, STATUS_E status, unsigned short event)
{
    column_row_enabler(status);

    if (data->status != status)
    {
#ifdef KPP_DEBUG
        printk("KPP: sidekey status change %d to %d\n", data->status, status);
#endif
        data->status = status;
        insert_raw_event(event | data->keycode | (status==OPEN ? KEYUP : KEYDOWN));
    }
}

/*! This function is called on a timeout after the interrupt handler. 
 *
 *  @param data  The contents of the timer's "data" member.
 */
static void sidekey_timer_handler(unsigned long data)
{
    sidekey_status_check((SIDEKEY_T*)data);
    reject_suspend = 0;
    wake_up_interruptible(&kpp_dev.queue.waitq);
}

/*! 
 * This function is called in response to an interrupt from a sidekey.
 *
 * @param irq   Interrupt number
 * @param data  The data associated with a particular sidekey input.
 * @return IRQ_RETVAL(1) means the interrupt was handled.
 */
static irqreturn_t sidekey_irq_handler(int irq, void *data, struct pt_regs* unused)
{
    struct timer_list *t = &((SIDEKEY_T*)data)->timer;

    reject_suspend = 1;

    del_timer(t);
    t->expires = jiffies + 10;
    add_timer(t);

    return IRQ_RETVAL(1);
}

/*!
 * Register for sidekey interrupt
 *
 * @param name  The name of the sidekey.
 * @param irq   The interrupt number.
 * @param data  The data associated with the sidekey.
 *
 * @return Zero: success, non-zero: error code.
 */
static int sidekey_request_irq(const char *name, int irq, SIDEKEY_T* data)
{
    set_irq_type(irq, IRQT_BOTHEDGE);
    return request_irq(irq, sidekey_irq_handler, 0, name, data);
}

/*!
 * Free sidekey interrupt
 *
 * @param irq   The interrupt number.
 * @param data  The data associated with the sidekey.
 */
static void sidekey_free_irq(int irq, SIDEKEY_T* data)
{
    free_irq(irq, data);
}

/*!
 * Get the current state of the keylock input
 *
 * @return The status (open/closed).
 */
static STATUS_E read_keylock(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_KEYPAD_LOCK) ? OPEN : CLOSE;
}

/*!
 * Get the current state of the lenscover input
 *
 * @return The status (open/closed).
 */
static STATUS_E read_lenscover(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_LENS_COVER) ? CLOSE : OPEN;
}
#endif /* CONFIG_MACH_XPIXL */
/*!
 * @brief enables/disables columns and rows in keypad
 *
 * This function will enable/disable the necessary columns and rows depending on if the 
 * slider, flip, or keylock is open or closed.
 *
 * @param  status - status of the slider/flip
 */
static void column_row_enabler (STATUS_E status)
{
    unsigned short reg_val;

    /* Set the status */
    kpp_dev.status = status;

    if(status == CLOSE)
    {
        invalid_rows    |= KEYPAD_CLOSED_INACTIVE_ROWS;
        invalid_columns |= KEYPAD_CLOSED_INACTIVE_COLUMNS;
    }
    else
    {
        invalid_rows    &= ~KEYPAD_CLOSED_INACTIVE_ROWS;
        invalid_columns &= ~KEYPAD_CLOSED_INACTIVE_COLUMNS;
    }

    /* Enable number of rows in keypad (KPCR[7:0])
     * Configure keypad columns as open-drain (KPCR[15:8])
     *
     * Configure the rows/cols in KPP
     * LSB nibble in KPP is for 8 rows
     * MSB nibble in KPP is for 8 cols
     */
    reg_val  = (1 << ROWS) - 1;          /* LSB */
    reg_val &= ~invalid_rows;
    reg_val |= ((1 << COLS) - 1) << 8;  /* MSB */
    writew(reg_val, KPCR);

    /* Write 0's to KPDR[15:8] */
    writew(invalid_columns<<8, KPDR);

    /* Configure columns as output, rows as input (KDDR[15:0]) */
    reg_val = ((1 << COLS) - 1) << 8;
    writew(reg_val, KDDR);
}

/*!
 * This structure contains pointers for /dev/keypad0 and /dev/keypad
 * device driver entry points. 
 * The driver register function in init module will call this
 * structure.
 */
static struct file_operations keypad_fops = {
    .owner          = THIS_MODULE,
    .open           = mxc_kpp_open,
    .release        = mxc_kpp_close,
    .read           = mxc_kpp_read,
    .ioctl          = mxc_kpp_ioctl,
    .poll           = mxc_kpp_poll,
};

static struct miscdevice mxc_keyb_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MOD_NAME,
    .fops = &keypad_fops,
    .devfs_name = DEVFS_NAME
};

static struct miscdevice mxc_keyb_miscJ_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MOD_NAMEJ,
    .fops = &keypad_fops,
    .devfs_name = DEVFS_JNAME
};

/*!
 * This structure contains pointers for /dev/keypadI entry points.
 * The driver register function in init module will call this
 * structure.
 */
static struct file_operations keypadI_fops = {
    .owner          = THIS_MODULE,
    .open           = mxc_kppI_open,
    .release        = mxc_kppI_close,
    .ioctl          = mxc_kppIB_ioctl,
};

static struct miscdevice mxc_keyb_miscI_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MOD_NAMEI,
    .fops = &keypadI_fops,
    .devfs_name = DEVFS_INAME
};

/*!
 * This structure contains pointers for /dev/keypadB entry points.
 * The driver register function in init module will call this
 * structure.
 */
static struct file_operations keypadB_fops = {
    .owner          = THIS_MODULE,
    .open           = mxc_kppB_open,
    .release        = mxc_kppB_close,
    .ioctl          = mxc_kppIB_ioctl,
    .poll           = mxc_kppB_poll,
};

static struct miscdevice mxc_keyb_miscB_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = MOD_NAMEB,
    .fops = &keypadB_fops,
    .devfs_name = DEVFS_BNAME
};

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxc_keyb = {
    .name           = MOD_NAME,
    .bus            = &platform_bus_type,
    .probe          = mxc_kpp_probe,
    .suspend        = mxc_kpp_suspend,
    .resume         = mxc_kpp_resume,
};

/*! Device Definition for MXC keypad. */
static struct platform_device mxc_keyb_device = {
    .name           = MOD_NAME,
    .id             = 0,
};

/*! 
 * This function is called for module initialization.
 * It registers keypad char driver and requests for KPP irq number.
 * It also does the initialization of the keypad device.
 *
 * The following steps are used for keypad configuration,\n
 * -# Enable number of rows in the keypad control register (KPCR[7:0}).\n
 * -# Write 0's to KPDR[15:8]\n
 * -# Configure keypad columns as open-drain (KPCR[15:8])\n
 * -# Configure columns as output, rows as input (KDDR[15:0])\n
 * -# Clear the KPKD status flag (write 1 to it) and synchronizer chain\n
 * -# Set KDIE control bit, clear KRIE control bit\n
 * -# Set the KPPEN bit for debouncing\n
 * In this function the keypad queue initialization is done. 
 * The keypad IOMUX configuration are done here.
 *
 * @return      0 on success and a non-zero value on failure.
 */
static int __init mxc_kpp_init(void)
{
    int retval, ret;
    unsigned int   alloc_size;
    unsigned short reg_val;
#ifdef KPP_DEBUG
    printk("KPP: mxc_kpp_init() called \n");
#endif

    memset (keybitmap, 0, sizeof(keybitmap));
    memset (lastioctlbitmap, 0, sizeof(lastioctlbitmap));
        
    kpp_dev.autorepeat_info.r_repeat = 1;
    kpp_dev.autorepeat_info.r_time_to_first_repeat = DEFAULT_JIFFIES_TO_FIRST_REPEAT;
    kpp_dev.autorepeat_info.r_time_between_repeats = DEFAULT_JIFFIES_TO_NEXT_REPEAT;

#if defined(CONFIG_MACH_SCMA11REF)
    ret = boardrev();
    if (ret >= BOARDREV_P2AW) 
    {
        key_map = key_map_P2W;
    }
    else if (((ret >= BOARDREV_P1A) && (ret < BOARDREV_P2AW)) || (ret == BOARDREV_UNKNOWN)) 
    {
        key_map = key_map_P1_P2;
    }
    else 
    {
        key_map = key_map_P0;
    }
#elif defined(CONFIG_MACH_XPIXL)
    key_map = keymap_phone; /* Phone mode is the default. */
#elif (defined(CONFIG_MACH_KASSOS))
    ret = boardrev();
    if ((ret < BOARDREV_P1A) || (ret == BOARDREV_UNKNOWN))
    {
        key_map = key_map_kassos_P0;
    }
    else
    {
        key_map = key_map_kassos_P1;
        COLS = 8;
    }
#elif defined(CONFIG_ARCH_MXC91321)
#if defined(CONFIG_MACH_BUTEREF)
    ret = boardrev();
    if ((ret < BOARDREV_P4A) || (ret == BOARDREV_UNKNOWN)) 
    {
        key_map = key_map_P3;
    }
    else 
    {
        key_map = key_map_P4;
        COLS = 8;
    }
#else /* Non-BUTEREF ArgonLV Products */
    key_map = key_map_P4;
    COLS = 8;
#endif /* BUTEREF */
#endif /* MXC91321 */

    max_autorepeat_key_info = ROWS*COLS;
    alloc_size              = max_autorepeat_key_info*sizeof(autorepeat_info);
    autorepeat_key_info     = (autorepeat_info*)kmalloc(alloc_size, GFP_KERNEL);
    if(!autorepeat_key_info)
    {
        goto ALLOC_AUTOREPEAT_INFO_FAILED;
    }

    start_autorepeat_key_info = max_autorepeat_key_info;
    end_autorepeat_key_info   = max_autorepeat_key_info;

    headset_scancode = 0xFFFF;
    emu_headset_scancode = 0xFFFE;
    
#ifdef KPP_DEBUG
    printk("KPP: base address  :  %08lX\n", (unsigned long)KPP_BASE_ADDR);
    printk("KPP: status reg address:  %08lX\n", (unsigned long)KPSR);
    printk("KPP: control reg address :  %08lX\n", (unsigned long)KPCR);
    printk("KPP: data direction  reg address : %08lX\n", (unsigned long)KDDR);
    printk("KPP: data reg address : %08lX\n", (unsigned long)KPDR);
    /* Read the value of registers */
    reg_val = readw(KPSR);
    printk("KPP: status reg: %08X\n", reg_val);
    reg_val = readw(KPCR);
    printk("KPP: control reg: %08X\n", reg_val);
    reg_val = readw(KPDR);
    printk("KPP: data reg: %08X\n", reg_val);
    reg_val = readw(KDDR);
    printk("KPP: data direction reg : %08X\n", reg_val);
#endif  /* KPP_DEBUG */

    /* Initialize the polling timer */
    init_timer(&kpp_dev.poll_timer);
    kpp_dev.poll_timer.function = mxc_kpp_handle_timer;

    /* Initialize the autorepeat timer */
    init_timer(&kpp_dev.autorepeat_timer);
    kpp_dev.autorepeat_timer.function = autorepeat_handle_timer;

    /* Initialize the headset timer */
    init_timer(&headset_timer);
    headset_timer.function = headset_handle_timer;

/* Initialize the emu headset timer */
   init_timer(&emu_headset_timer);
   emu_headset_timer.function = emu_headset_handle_timer;

#if defined(CONFIG_MOT_FEAT_FLIP)
    /* Initialize the flip interrupt timer */
    init_timer(&kpp_dev.timer);
    kpp_dev.timer.function = flip_timer_handler;
#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
    /* Initialize the slider interrupt timer */
    init_timer(&kpp_dev.timer);
    kpp_dev.timer.function = slider_timer_handler;
#endif

#if defined(CONFIG_MACH_ELBA)
    /* Initialize the keylock interrupt timer */
    init_timer(&kpp_dev.timer);
    kpp_dev.timer.function = keylock_timer_handler;
#elif defined(CONFIG_MACH_XPIXL)
    /* Initialize the keylock and lens cover timer structs. */
    init_timer(&keylock_data.timer);
    keylock_data.timer.function = sidekey_timer_handler;
    keylock_data.timer.data = (unsigned long)&keylock_data;

    init_timer(&lenscover_data.timer);
    lenscover_data.timer.function = sidekey_timer_handler;
    lenscover_data.timer.data = (unsigned long)&lenscover_data;
#endif    
    /* IOMUX configuration for keypad */
    gpio_keypad_active();

    /* Configure keypad */

    /* Enable number of rows in keypad (KPCR[7:0])
     * Configure keypad columns as open-drain (KPCR[15:8])
     * 
     * Configure the rows/cols in KPP 
     * LSB nibble in KPP is for 8 rows
     * MSB nibble in KPP is for 8 cols
     */
    reg_val = (1 << ROWS) - 1;          /* LSB */
    reg_val &= ~invalid_rows;
    reg_val |= ((1 << COLS) - 1) << 8;  /* MSB */
    writew(reg_val, KPCR);

    /* Write 0's to KPDR[15:8] */
    writew((invalid_columns<<8), KPDR);

    /* Configure columns as output, rows as input (KDDR[15:0]) */
    reg_val = ((1 << COLS) - 1) << 8;
    writew(reg_val, KDDR);

    /* 
     * Clear the KPKD status flag (write 1 to it) and synchronizer chain.
     * Set KDIE control bit, clear KRIE control bit (avoid false release
     * events. Enable the KPPEN bit for debouncing.
     */
    reg_val = readw(KPSR);
    reg_val |= KBD_STAT_KPKD;
    reg_val &= ~KBD_STAT_KRSS;
    reg_val |= KBD_STAT_KDIE;
    reg_val &= ~KBD_STAT_KRIE;
    reg_val |= KBD_STAT_KPPEN;
    writew(reg_val, KPSR);

    /* Clear the input queue */
    mxc_kpp_initq();

#if defined(CONFIG_MOT_FEAT_FLIP)
    /* get flip status */
        
    /* Currently this call can stay and the first flip open/close can be
     * added to the queue, it does not seem to cause any issues so until
     * it does, this should never change. */
    flip_status_check();
#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
#if defined(CONFIG_MACH_NEVIS)
    /* get slider status and send the first indication */
    slider_status_check();
#else /* Not Nevis */
    /* get slider status */
    if(!gpio_slider_open())
    {
        column_row_enabler(CLOSE);
        mpm_event_notify(MPM_EVENT_DEVICE, EVENT_DEV_SLIDER, DEV_OFF);
    }
    else
    {
        column_row_enabler(OPEN);
        mpm_event_notify(MPM_EVENT_DEVICE, EVENT_DEV_SLIDER, DEV_ON);
    }
#endif /* Nevis */  
#endif /* Slider */

#if defined(CONFIG_MACH_ELBA)
    /* get keylock status */
    keylock_status_check();
#elif defined( CONFIG_MACH_XPIXL)
    /* Do first-time sidekey checks to initialize status variables. */
    sidekey_status_check(&keylock_data);
    sidekey_status_check(&lenscover_data);
#endif
    /*
     * Request for IRQ number for keypad port. The Interrupt handler
     * function (mxc_kpp_interrupt) is called when ever interrupt occurs on
     * keypad port.
     */
    retval = request_irq(INT_KPP, mxc_kpp_interrupt, 0, MOD_NAME, &kpp_dev);

    if (retval) 
    {
        printk("KPP: request_irq(%d) returned error %d\n", INT_KPP, retval);
        goto REQUEST_KPPINT_FAILED;
    }

#ifdef KPP_DEBUG
    printk("KPP: irq enabled: %d\n", INT_KPP);
#endif

#if defined(CONFIG_MOT_FEAT_FLIP)
#if defined(CONFIG_MACH_NEVIS)
    set_irq_type(INT_EXT_INT0, IRQT_BOTHEDGE);
    set_irq_type(INT_EXT_INT3, IRQT_BOTHEDGE);
    retval = request_irq(INT_EXT_INT0, flip_irq_handler, 0, "flip_open", NULL);
    retval |= request_irq(INT_EXT_INT3, flip_irq_handler, 0, "flip_close", NULL);
#else /* Not Nevis */
    retval = gpio_flip_request_irq (flip_irq_handler, 0, "flip", NULL);
#endif /* Nevis */
    if (retval) 
    {
        printk("KPP: gpio_flip_request_irq returned error %d\n", retval);
        free_irq(INT_KPP, MOD_NAME);
        goto REQUEST_FLIPINT_FAILED;
    }
#ifdef KPP_DEBUG
    printk("KPP: flip irq enabled\n");
#endif

#endif

#if defined(CONFIG_MOT_FEAT_SLIDER)
#if defined(CONFIG_MACH_NEVIS)
    set_irq_type(INT_EXT_INT0, IRQT_BOTHEDGE);
    set_irq_type(INT_EXT_INT3, IRQT_BOTHEDGE);
    retval = request_irq(INT_EXT_INT0, slider_irq_handler, 0, "slider_open", NULL);
    retval |= request_irq(INT_EXT_INT3, slider_irq_handler, 0, "slider_close", NULL);
#else /* Not Nevis */
    retval = gpio_slider_request_irq (slider_irq_handler, 0, "slider", NULL);
#endif /* Nevis */
    
    if (retval) 
    {
        printk("KPP: gpio_slider_request_irq returned error %d\n", retval);
        free_irq(INT_KPP, MOD_NAME);
/* Currently there is no product that will support both the slider and the flip, but
 * this code is here for future products that could possibly support both */
#if defined(CONFIG_MOT_FEAT_FLIP)
        gpio_flip_free_irq(NULL);
#endif
        goto REQUEST_SLIDERINT_FAILED;
    }
#ifdef KPP_DEBUG
    printk("KPP: slider irq enabled\n");
#endif

#endif

#if defined(CONFIG_MACH_ELBA)
    retval = gpio_keylock_request_irq (keylock_irq_handler, 0, "keylock", NULL);
    if (retval) 
    {
        printk("KPP: gpio_keylock_request_irq returned error %d\n", retval);
        free_irq(INT_KPP, MOD_NAME);
        goto REQUEST_KEYLOCKINT_FAILED;
    }
#ifdef KPP_DEBUG
    printk("KPP: keylock irq enabled\n");
#endif

#elif defined( CONFIG_MACH_XPIXL)
    retval = sidekey_request_irq("keylock", KEYLOCK_IRQ, &keylock_data);
    if (retval) 
    {
        printk("KPP: keylock irq request returned error %d\n", retval);
        free_irq(INT_KPP, MOD_NAME);
        goto REQUEST_KEYLOCKINT_FAILED;
    }
#ifdef KPP_DEBUG
    printk("KPP: keylock irq enabled\n");
#endif
    retval = sidekey_request_irq("lenscover", LENSCOVER_IRQ, &lenscover_data);
    if (retval)
    {
        printk("KPP: lenscover irq request returned error %d\n", retval);
        free_irq(INT_KPP, MOD_NAME);
        goto REQUEST_LENSCOVERINT_FAILED;
    }
#ifdef KPP_DEBUG
    printk("KPP: lens cover irq enabled\n");
#endif
#endif /* xPIXL */

    retval = driver_register(&mxc_keyb);
    if(retval)
    {
        goto REGISTER_DRIVER_FAILED;
    }

    retval = platform_device_register(&mxc_keyb_device);
    if(retval)
    {
        goto REGISTER_DRIVER_FAILED;
    }

    if ((ret = misc_register(&mxc_keyb_misc_device))) 
    {
        printk(KERN_ERR "KPP: misc_register failed\n");
        goto REGISTER_KEYB_FAILED;
    }

    if ((ret = misc_register(&mxc_keyb_miscJ_device))) 
    {
        printk(KERN_ERR "KPP: misc_register failed\n");
        goto REGISTER_KEYBJ_FAILED;
    }

    if ((ret = misc_register(&mxc_keyb_miscI_device))) 
    {
        printk(KERN_ERR "KPP: misc_register failed\n");
        goto REGISTER_KEYBI_FAILED;
    }

    if ((ret = misc_register(&mxc_keyb_miscB_device))) 
    {
        printk(KERN_ERR "KPP: misc_register failed\n");
        goto REGISTER_KEYBB_FAILED;
    }
   
    /* used to decide whether to set lastioctlbitmap in GETBITMAP ioctl */
    Bminor = mxc_keyb_miscB_device.minor;

    return 0;

REGISTER_KEYBB_FAILED:
    misc_deregister(&mxc_keyb_miscI_device);
REGISTER_KEYBI_FAILED:
    misc_deregister(&mxc_keyb_miscJ_device);
REGISTER_KEYBJ_FAILED:
    misc_deregister(&mxc_keyb_misc_device);
REGISTER_KEYB_FAILED:
    platform_device_unregister(&mxc_keyb_device);
REGISTER_DEVICE_FAILED:
    driver_unregister(&mxc_keyb);
REGISTER_DRIVER_FAILED:
#if defined(CONFIG_MACH_ELBA)
    gpio_keylock_free_irq(NULL);
#elif defined(CONFIG_MACH_XPIXL)
    sidekey_free_irq(LENSCOVER_IRQ, &lenscover_data);
REQUEST_LENSCOVERINT_FAILED:
    sidekey_free_irq(KEYLOCK_IRQ, &keylock_data);
REQUEST_KEYLOCKINT_FAILED:
#endif

#ifdef CONFIG_MOT_FEAT_SLIDER
#if defined(CONFIG_MACH_NEVIS)
    /* Free FLIP_OPEN and FLIP_CLOSE */
    free_irq(INT_EXT_INT0, NULL);
    free_irq(INT_EXT_INT3, NULL);
#else /* Not Nevis */
    gpio_slider_free_irq(NULL);
#endif /* Nevis */
#endif /* Slider */

#ifdef CONFIG_MOT_FEAT_FLIP
#if defined(CONFIG_MACH_NEVIS)
    /* Free FLIP_OPEN and FLIP_CLOSE */
    free_irq(INT_EXT_INT0, NULL);
    free_irq(INT_EXT_INT3, NULL);
#else /* Not Nevis */
    gpio_flip_free_irq(NULL);
#endif /* Nevis */
#endif /* Flip */

    free_irq(INT_KPP, MOD_NAME);

#ifdef CONFIG_MOT_FEAT_SLIDER
REQUEST_SLIDERINT_FAILED:
#endif
#ifdef CONFIG_MOT_FEAT_FLIP
REQUEST_FLIPINT_FAILED:
#endif

REQUEST_KPPINT_FAILED:
    kfree(autorepeat_key_info);

ALLOC_AUTOREPEAT_INFO_FAILED:
    return -1;
}

static inline void uninitialize_registers(void)
{
    unsigned short reg_val;

    /* 
     * Clear the KPKD status flag (write 1 to it) and synchronizer chain.
     * Set KDIE control bit, clear KRIE control bit (avoid false release
     * events. Disable the keypad GPIO pins.
     */
    writew(0x00, KPCR);
    writew(0x00, KPDR);
    writew(0x00, KDDR);
    reg_val = readw(KPSR);
    reg_val |= KBD_STAT_KPKD;
    reg_val &= ~KBD_STAT_KRSS;
    reg_val |= KBD_STAT_KDIE;
    reg_val &= ~KBD_STAT_KRIE;
    writew(reg_val, KPSR);
    gpio_keypad_inactive();
}

/*!
 * This function is called whenever the module is removed from the kernel.
 * The keypad interrupts are disabled. It calls gpio_keypad_inactive function
 * to switch gpio configuration into default state. It unregisters the keypad
 * driver from kernel and frees the irq number.
 */
static void __exit mxc_kpp_cleanup(void)    
{
    keyeventcb*   freekeyeventcb;
    unsigned long flags;

#ifdef KPP_DEBUG
    printk("KPP: module cleanup is called \n");
#endif

#if defined(CONFIG_MOT_FEAT_FLIP)
#if defined(CONFIG_MACH_NEVIS)
    /* Free FLIP_OPEN and FLIP_CLOSE */
    free_irq(INT_EXT_INT0, NULL);
    free_irq(INT_EXT_INT3, NULL);
#else /* Not Nevis */
    gpio_flip_free_irq(NULL);
#endif /* Nevis */
    del_timer(&kpp_dev.timer);
#endif
#if defined(CONFIG_MOT_FEAT_SLIDER)
#if defined(CONFIG_MACH_NEVIS)
    /* Free FLIP_OPEN and FLIP_CLOSE */
    free_irq(INT_EXT_INT0, NULL);
    free_irq(INT_EXT_INT3, NULL);
#else /* Not Nevis */
    gpio_slider_free_irq(NULL);
#endif /* Nevis */
    del_timer(&kpp_dev.timer);
#endif
#if defined(CONFIG_MACH_ELBA)
    gpio_keylock_free_irq(NULL);
    del_timer(&kpp_dev.timer);
#endif
#ifdef CONFIG_MACH_XPIXL
    sidekey_free_irq(KEYLOCK_IRQ, &keylock_data);
    sidekey_free_irq(LENSCOVER_IRQ, &lenscover_data);
    del_timer(&lenscover_data.timer);
    del_timer(&keylock_data.timer);
#endif
    free_irq(INT_KPP, MOD_NAME);
    uninitialize_registers();
    del_timer(&kpp_dev.poll_timer);
    del_timer(&kpp_dev.autorepeat_timer);
    del_timer(&headset_timer);
    del_timer(&emu_headset_timer);

    misc_deregister(&mxc_keyb_misc_device);
    misc_deregister(&mxc_keyb_miscJ_device);
    misc_deregister(&mxc_keyb_miscI_device);
    misc_deregister(&mxc_keyb_miscB_device);
    driver_unregister(&mxc_keyb);
    platform_device_unregister(&mxc_keyb_device);

    spin_lock_irqsave(&keyeventcb_lock, flags);

    while(keyeventcb_chain)
    {
        freekeyeventcb   = keyeventcb_chain;
        keyeventcb_chain = keyeventcb_chain->next_callback;

        kfree(freekeyeventcb);
    }

    spin_unlock_irqrestore(&keyeventcb_lock, flags);

    kfree(autorepeat_key_info);
}

module_init(mxc_kpp_init);
module_exit(mxc_kpp_cleanup);

EXPORT_SYMBOL(generate_key_event);
EXPORT_SYMBOL(register_keyevent_callback);
EXPORT_SYMBOL(unregister_keyevent_callback);


#if defined(CONFIG_MACH_ELBA)
/**
 * Register for key lock interrupt
 *
 * Return value (non-zero): Failed 
 * Return value (zero):     Successful
 */
int gpio_keylock_request_irq(gpio_irq_handler handler, unsigned long irq_flags,
        const char *devname, void *dev_id)
{
    set_irq_type(INT_EXT_INT3, IRQT_BOTHEDGE);
    return request_irq(INT_EXT_INT3, handler, irq_flags, devname, dev_id);
}

/**
 * Free key lock interrupt
 */
int gpio_keylock_free_irq(void *dev_id)
{
    free_irq(INT_EXT_INT3, dev_id);
    return 0;
}

/**
 * Get the current state of the lock key
 *
 * Return value (non-zero): keys are locked 
 * Return value (zero):     keys are unlocked
 */
int gpio_keylock_open(void)
{
    return gpio_signal_get_data_check(GPIO_SIGNAL_KEYS_LOCKED);
}
#endif
