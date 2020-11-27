/*
 * Copyright 2004 Freescale Semiconductor, Inc.
 * Copyright (C) 2005-2008 Motorola Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Motorola 2008-May-23 - OSS CV Fix
 * Motorola 2007-Sep-24 - Support lens cover.
 * Motorola 2006-Nov-30 - Support key lock feature.
 * Motorola 2006-Aug-03 - Add autorepeat
 * Motorola 2006-Mar-21 - Added code for Slider
 * Motorola 2005-Nov-15 - Added code for Flip
 * Motorola 2005-Jun-27 - Initial Creation
 */

#ifndef __MOT_KEYB_H__
#define __MOT_KEYB_H__

/*!
 * @file mot_keyb.h
 *
 * @brief Motorola keypad header file, derived from mxc_keyb.h.
 * 
 * @ingroup keypad
 */ 

/*! Keypad Module Name */
/* main device, /dev/keypad0 */
#define MOD_NAME  "mxckpd"
/* for /dev/keypad, alternate name for /dev/keypad0 */
#define MOD_NAMEJ  "mxckpdJ"
/* for /dev/keypadI, device for doing software event insertion */
#define MOD_NAMEI  "mxckpdI"
/* for /dev/keypadB, device for getting bitmap information */
#define MOD_NAMEB  "mxckpdB;"
 
/*! Keypad irq number */ 
#define KPP_IRQ  INT_KPP
 
/*! Comment KPP_DEBUG to disable debug messages */
/* #define KPP_DEBUG        1  */

/*! XLATE mode selection */
#define KEYPAD_XLATE        0

/*! RAW mode selection */
#define KEYPAD_RAW          1

/*! Key event flag definitions */
#define KEYFLAG_SLIDER_DISABLED    0x0100
#define KEYFLAG_FLIP_DISABLED      0x0200
#define KEYFLAG_KEYLOCK_DISABLED   0x0400

/*! Keypad matrix keycode mask */
#define KEY_CODE_MASK 0x00FF

/* Keypad Control Register Address */ 
#define KPCR    IO_ADDRESS(KPP_BASE_ADDR + 0x00)

/* Keypad Status Register Address */ 
#define KPSR    IO_ADDRESS(KPP_BASE_ADDR + 0x02)
 
/* Keypad Data Direction Address */ 
#define KDDR    IO_ADDRESS(KPP_BASE_ADDR + 0x04)
 
/* Keypad Data Register */ 
#define KPDR    IO_ADDRESS(KPP_BASE_ADDR + 0x06)
 
/* Key Press Interrupt Status bit */
#define KBD_STAT_KPKD        0x01
 
/* Key Release Interrupt Status bit */ 
#define KBD_STAT_KPKR        0x02
 
/* Key Depress Synchronizer Chain Status bit */ 
#define KBD_STAT_KDSC        0x04
 
/* Key Release Synchronizer Status bit */ 
#define KBD_STAT_KRSS        0x08
 
/* Key Depress Interrupt Enable Status bit */ 
#define KBD_STAT_KDIE        0x100
 
/* Key Release Interrupt Enable */ 
#define KBD_STAT_KRIE        0x200
 
/* Keypad Clock Enable  */ 
#define KBD_STAT_KPPEN       0x400

/*! Buffer size of keypad queue. Should be a power of 2. */ 
#define KPP_BUF_SIZE    128
 
/*! Test whether bit is set for integer c */ 
#define TEST_BIT(c, n) ((c) & (0x1 << (n)))
 
/*! Set nth bit in the integer c */
#define BITSET(c, n)   ((c) | (1 << (n)))
 
/*! Reset nth bit in the integer c */ 
#define BITRESET(c, n) ((c) & ~(1 << (n)))

/*! states for debouncing */
typedef enum {
    STABLE,    /* things are stable */
    DEBOUNCE   /* we're debouncing */
}KEYPAD_STATE_E;

/*! Default number of jiffies to first and succeeding autorepeats of held keys */
#define DEFAULT_JIFFIES_TO_FIRST_REPEAT 30
#define DEFAULT_JIFFIES_TO_NEXT_REPEAT  30

 /*!
 * This structure represents the keypad queue where scancode/mapcode 
 * for key press/release are queued.
 */
typedef struct keypad_queue 
{
    /*! Head of queue */
    unsigned short head;
    
    /*! Tail of queue */
    unsigned short tail;
    
    /*! Number of data in queue */
    unsigned short len;
    
    /*! Spinlock operation */
    spinlock_t s_lock;
    
    /*! Wait queue head */
    wait_queue_head_t waitq;
    
    /*! Keypad queue buffer size */
    unsigned short buf[KPP_BUF_SIZE];
} keypad_queue;

/*! Keypad Private Data Structure */
typedef struct keypad_priv 
{
    /*! Major number */
    int major;

    /*! Keypad Queue Structure */
    struct keypad_queue queue;

    /*!  
     * This variable represents the keypad mode of operation. The keypad
     * can be configured either in raw mode or map mode using ioctl call.
     * In raw mode the scancode of key press and key release are queued
     * into keypad queue, while in map mode the mapcode of the key press
     * and key release are queued into keypad queue.
     */
    unsigned char kpp_mode;

    /*! Timer used for Keypad polling. */
    struct timer_list poll_timer;

    /*! Timer used for autorepeat timing. */
    struct timer_list autorepeat_timer;

    /*! Most recent key event */
    unsigned short recent_key_value;

    /*! Information about autorepeat */
    struct autorepeatinfo autorepeat_info;

    /*! Timer used for flip/slider interrupt handling */
    struct timer_list timer;

    /*! Timer used for lens cover interrupt handling */
    struct timer_list lens_cover_timer;
    
    /*! Information about flip/slider */
    STATUS_E status;
        
    /*! Information about lens cover */
    STATUS_E lens_cover_status;
        
} keypad_priv;

/*! Key event notification structure */
typedef struct keyeventcb
{
    /*! Function pointer identifying a user defined callback */
    void (*notify_function) (unsigned short, void*);

    /*! User defined pointer to user specific data */
    void* user_data;

    /*! Pointer to the next callback in a list */
    struct keyeventcb* next_callback;

    /*! Pointer to the previous callback in a list */
    struct keyeventcb* previous_callback;
}keyeventcb;

/*! Auto repeat structure */
typedef struct autorepeat_info
{
    /*! The next key pressed (pressed later in time) */
    unsigned char next_keypress;

    /*! The previous key pressed (pressed earlier in time) */
    unsigned char previous_keypress;

    /*! Store the time for key repeat */
    unsigned long repeat_time;
} autorepeat_info;

/* If a bitmap is 2 words, it looks like this:
 * word:        1                          0
 *  ----------------------------------------------------
 * | 63 62 ........... 33 32 | 31 30 ............. 1 0 |
 *  ----------------------------------------------------
 * bit 0 is 1<<0, bit 1 is 1<<1, ... bit 31 is 1<<31, all in word 0
 * bit 32 is 1<<0, bit 33 is 1<<1, ... in word 1
 * We want this:
 *     code       WORD_IN_BITMAP   BIT_FOR_CODE
 *      0               0           0x1
 *      1               0           0x2
 *          ...
 *     30               0           0x40000000
 *     31               0           0x80000000
 *     32               1           0x1
 *     33               1           0x2
 *          ...
 *     63               1           0x80000000
 */
#define WORD_IN_BITMAP(code) (code >> 5)
#define BIT_FOR_CODE(code)   (1 << (code & 0x1f))

#endif  /* __MOT_KEYB_H__ */

