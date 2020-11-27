/*
 * Copyright © 2005-2008, Motorola, All Rights Reserved.
 *
 * This program is licensed under a BSD license with the following terms:
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, this list of conditions
 *   and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice, this list of
 *   conditions and the following disclaimer in the documentation and/or other materials provided
 *   with the distribution.
 * - Neither the name of Motorola nor the names of its contributors may be used to endorse or
 *   promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Motorola 2008-Apr-21 - Resolve integration issue.
 * Motorola 2008-Feb-27 - Add OPENING/CLOSING transition for sliders
 * Motorola 2008-Feb-26 - Removed unused key definition KEYPAD_KODAK.
 * Motorola 2007-Sep-24 - Support lens cover.
 * Motorola 2007-Jul-02 - Added support of morphing modes.
 * Motorola 2006-Nov-30 - Support key lock feature.
 * Motorola 2006-Oct-10 - Add New Key support
 * Motorola 2006-Sep-19 - Clean up unused key_handlers
 * Motorola 2006-Sep-05 - Remove first slider event
 * Motorola 2005-Feb-09 - Initial Creation
 */

#ifndef __KEYPAD_H__
#define __KEYPAD_H__

#include <linux/ioctl.h>
#include <linux/morphing_mode.h>

/* device name from ezx */
#define DEVFS_NAME         "keypad0"
#define KEYPAD_DEVICENAME  "/dev/keypad0"

/* device name from juix, same as keypad0  */
#define DEVFS_JNAME         "keypad"
#define KEYPAD_JDEVICENAME  "/dev/keypad"

/* device name for inserting events */
#define DEVFS_INAME         "keypadI"
#define KEYPADI_DEVICENAME  "/dev/keypadI"

/* device name for getting bitmap information */
#define DEVFS_BNAME         "keypadB"
#define KEYPADB_DEVICENAME  "/dev/keypadB"

/*
 * status of flip/slider
 */
typedef enum {
    OPEN = 0,
    CLOSE,
    OPENING,
    CLOSING,
    
    /* Add extra states above here */
    INVALID
}STATUS_E;

/**************************************************
 * These are needed for User Space applications
 * but should not be used in the kernel.  STATUS_E
 * should be used instead
 **************************************************/
 
/*
 * status of flip
 */
typedef enum {
    FLIP_OPEN = 0,
    FLIP_CLOSE,
    FLIP_OPENING,
    FLIP_CLOSING,
    
    /* Add extra states above here */
    FLIP_INVALID
}FLIP_STATUS_E;

/*
 * status of slider
 */
typedef enum {
    SLIDER_OPEN = 0,
    SLIDER_CLOSE,
    SLIDER_OPENING,
    SLIDER_CLOSING,
    
    /* Add extra states above here */
    SLIDER_INVALID
}SLIDER_STATUS_E;

/*
 * A key event is an unsigned short.  The low-order byte contains the
 * key code.  The high-order byte contains flags.
 */
#define EVENTSIZE             sizeof(unsigned short)

#define KEYDOWN            0x8000
#define KEYUP              0x0000
#define KEYTRANSITION      0x1000

#define EVENTINSERTED      0x4000
#define ASCII_INSERTED     0x2000
#define EVENTFROMHARDWARE  0x0000

#define KEY_IS_DOWN(x)        ((x) & KEYDOWN)
#define EVENT_WAS_INSERTED(x) ((x) & EVENTINSERTED)
#define IS_ASCII_EVENT(x)     ((x) & ASCII_INSERTED)
#define KEYCODE(x)            ((x) & 0x00ff)

/*
 * the constant KEY_k is both the code for key k in an event returned by
 * read() and the bit position for key k in the bitmap returned by
 * ioctl(KEYPAD_IOC_GETBITMAP).
 */
#define KEYPAD_NONE        255

#define KEYPAD_0           0
#define KEYPAD_1           1
#define KEYPAD_2           2
#define KEYPAD_3           3
#define KEYPAD_4           4
#define KEYPAD_5           5
#define KEYPAD_6           6
#define KEYPAD_7           7
#define KEYPAD_8           8
#define KEYPAD_9           9
#define KEYPAD_POUND       10
#define KEYPAD_STAR        11

#define KEYPAD_UP          12
#define KEYPAD_DOWN        13
#define KEYPAD_LEFT        14
#define KEYPAD_RIGHT       15
#define KEYPAD_CENTER      16

#define KEYPAD_NAV_UP      KEYPAD_UP
#define KEYPAD_NAV_DOWN    KEYPAD_DOWN
#define KEYPAD_NAV_LEFT    KEYPAD_LEFT
#define KEYPAD_NAV_RIGHT   KEYPAD_RIGHT
#define KEYPAD_NAV_CENTER  KEYPAD_CENTER

#define KEYPAD_MENU        17
#define KEYPAD_SLEFT       18
#define KEYPAD_SRIGHT      19
#define KEYPAD_SOFT_LEFT   KEYPAD_SLEFT
#define KEYPAD_SOFT_RIGHT  KEYPAD_SRIGHT
#define KEYPAD_LEFT_MENU   KEYPAD_SLEFT

#define KEYPAD_OK          20
#define KEYPAD_CANCEL      21
#define KEYPAD_CLEAR       22
#define KEYPAD_CLEAR_BACK  KEYPAD_CLEAR
#define KEYPAD_CARRIER     23
#define KEYPAD_CARRIER2    KEYPAD_CARRIER /* old name */
#define KEYPAD_MESSAGING   24
#define KEYPAD_CARRIER1    KEYPAD_MESSAGING /* old name */

#define KEYPAD_CAMERA      25
#define KEYPAD_IMAGING     KEYPAD_CAMERA
#define KEYPAD_CAMERA_TAKE_PIC  KEYPAD_CAMERA

#define KEYPAD_ACTIVATE    26
#define KEYPAD_FLIP        27
#define KEYPAD_SEND        28
#define KEYPAD_VOICE_CALL_SEND  KEYPAD_SEND

#define KEYPAD_POWER_END   29
#define KEYPAD_POWER       KEYPAD_POWER_END
#define KEYPAD_HANGUP      30
#define KEYPAD_HOME        31

#define KEYPAD_VAVR        32
#define KEYPAD_VA          KEYPAD_VAVR
#define KEYPAD_VR          KEYPAD_VAVR

#define KEYPAD_PTT         33
#define KEYPAD_JOG_UP           34
#define KEYPAD_ROCKER_UP        KEYPAD_JOG_UP
#define KEYPAD_JOG_MIDDLE       35
#define KEYPAD_ROCKER_CENTER    KEYPAD_JOG_MIDDLE
#define KEYPAD_JOG_DOWN         36
#define KEYPAD_ROCKER_DOWN      KEYPAD_JOG_DOWN

#define KEYPAD_SIDE_UP     37
#define KEYPAD_VOLUP       KEYPAD_SIDE_UP

#define KEYPAD_SIDE_DOWN   38
#define KEYPAD_VOLDOWN     KEYPAD_SIDE_DOWN

#define KEYPAD_SIDE_SELECT 39
#define KEYPAD_SMART       KEYPAD_SIDE_SELECT

#define KEYPAD_HEADSET     40
#define KEYPAD_EMU_HEADSET 41

#define KEYPAD_AUD_PRV          42
#define KEYPAD_AUD_NEXT         43
#define KEYPAD_PLAY_PAUSE       44
#define KEYPAD_iTUNE            45

#define KEYPAD_SCREEN_LOCK      46

#define KEYPAD_VIDEO_CALL_SEND 47
#define KEYPAD_SPEAKER_PHONE   48

#define KEYPAD_SPARE6          49
#define KEYPAD_SPARE0          50

#define KEYPAD_MAIL            51
#define KEYPAD_ZOOM_IN         52
#define KEYPAD_ZOOM_OUT        53

#define KEYPAD_CAMERA_FOCUS    54

#define KEYPAD_SPARE1          55
#define KEYPAD_SPARE2          56
#define KEYPAD_SPARE3          57
#define KEYPAD_SPARE4          58
#define KEYPAD_SPARE5          59

#define KEYPAD_SLIDER          62

#define KEYPAD_KEYV_0          63
#define KEYPAD_KEYV_1          64
#define KEYPAD_KEYV_2          65
#define KEYPAD_KEYV_3          66

#define KEYPAD_CAPTURE_PLAYBACK_TOGGLE  67
#define KEYPAD_STILL_VIDEO_TOGGLE       68

#define KEYPAD_LOCK            69

#define KEYPAD_SHARE           70
#define KEYPAD_TRASH           71
#define KEYPAD_REVIEW          72
/* Don't use Kodak name. Remove this when not needed by apps. */
#define KEYPAD_KODAK           KEYPAD_REVIEW

#define KEYPAD_LENS_COVER      73
/* Add new keycodes above here.  Also change the MAXCODE
 * to the last keycode added */
#define KEYPAD_MAXCODE    KEYPAD_LENS_COVER

/*
 * status of headset
 */
#define HEADSET_IN      0
#define HEADSET_OUT     1

#define KEYPAD_IOCTL_BASE    'k'

#define NUM_WORDS_IN_BITMAP 4

/*!
 * return the bitmap of keys currently down.
 * The bitmap is an array of NUM_WORDS_IN_BITMAP unsigned long words.
 * The bitmap looks like this:
 *
 * word:      3                2                1                0
 *  -------------------------------------------------------------------
 * | 127 126 .. 97 96 | 95 94 .. 65 64 | 63 62 .. 33 32 | 31 30 .. 1 0 |
 *  -------------------------------------------------------------------
 * Bit n corresponds to key code n.
 */

#define KEYPAD_IOC_GETBITMAP      _IOR(KEYPAD_IOCTL_BASE,1,unsigned long *)

/*!
 * Inserts a keypad event as if it were read during a keypad interrupt.\n
 * The exact event as specified (but with the EVENTINSERTED flag on) will be\n
 * returned by read() when it reaches the front of the queue.\n
 * A key press event consists of the key code or'd with KEYDOWN.\n
 * A key release event consists of just the key code.\n
 * The argument is an unsigned short containing the key code.\n
 */
#define KEYPAD_IOC_INSERT_EVENT   _IOW(KEYPAD_IOCTL_BASE,2,unsigned short)

/*!
 * Inserts an ascii key code into the read queue.  The exact event as
 * specified (but with the flags EVENTINSERTED and ASCII_INSERTED turned
 * on) will be returned by read() when it reaches the front of the queue.
 */
#define KEYPAD_IOC_INSERT_ASCII_EVENT   _IOW(KEYPAD_IOCTL_BASE,3,unsigned short)

struct autorepeatinfo {
    int r_repeat;                /* 1 to do autorepeat, 0 to not do it */
    int r_time_to_first_repeat;  /* time to first repeat in milliseconds */
    int r_time_between_repeats;  /* time between repeats in milliseconds */
};

/*!
 * get settings for what to do about autorepeat on held keys
 */
#define KEYPAD_IOC_GET_AUTOREPEAT _IOR(KEYPAD_IOCTL_BASE,6,struct autorepeat *)

/*!
 * specify what to do about autorepeat on held keys
 */
#define KEYPAD_IOC_SET_AUTOREPEAT _IOW(KEYPAD_IOCTL_BASE,6,struct autorepeat *)

/*!
 *  get recent key pressed
 */
#define KEYPAD_IOC_GET_RECENT_KEY  _IOR(KEYPAD_IOCTL_BASE,7,unsigned short *)

/*!
 * get the state of the flip
 */
#define KEYPAD_IOC_GET_FLIP_STATUS _IOR(KEYPAD_IOCTL_BASE,8,STATUS_E *)
/*!
 * set flip status
 */
#define KEYPAD_IOC_SET_FLIP_STATUS _IOW(KEYPAD_IOCTL_BASE,8,STATUS_E)

/*!
 * get the state of the slider
 */
#define KEYPAD_IOC_GET_SLIDER_STATUS _IOR(KEYPAD_IOCTL_BASE,9,STATUS_E *)
/*!
 * set slider status
 */
#define KEYPAD_IOC_SET_SLIDER_STATUS _IOW(KEYPAD_IOCTL_BASE,9,STATUS_E)

/*!
 * get the state of the keylock
 */
#define KEYPAD_IOC_GET_KEYLOCK_STATUS _IOR(KEYPAD_IOCTL_BASE,10,STATUS_E *)
/*!
 * set keylock status
 */
#define KEYPAD_IOC_SET_KEYLOCK_STATUS _IOW(KEYPAD_IOCTL_BASE,10,STATUS_E)

/*!
 * the keypad should suspend operation
 */
#define KEYPAD_IOC_POWER_SAVE     _IO(KEYPAD_IOCTL_BASE,11)

/*!
 * the keypad should resume operation
 */
#define KEYPAD_IOC_POWER_RESTORE  _IO(KEYPAD_IOCTL_BASE,12)

/*!
 * Set keypad mapping table
 */
#define KEYPAD_IOC_SET_MAPPING_TABLE _IOW(KEYPAD_IOCTL_BASE,13,MORPHING_MODE_E)

/*!
 * get the state of the lens cover
 */
#define KEYPAD_IOC_GET_LENS_COVER_STATUS _IOR(KEYPAD_IOCTL_BASE,14,STATUS_E *)
/*!
 * set the state of the lens cover
 */
#define KEYPAD_IOC_SET_LENS_COVER_STATUS _IOW(KEYPAD_IOCTL_BASE,14,STATUS_E)

#endif /* __KEYPAD_H__ */
