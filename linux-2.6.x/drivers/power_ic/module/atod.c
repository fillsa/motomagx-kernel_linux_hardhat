/*
 * Copyright (C) 2004-2008 Motorola, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 * Motorola 2008-Feb-20 - Support A2D measurements on charger current
 * Motorola 2007-Aug-08 - Fix AtoD sample indexing
 * Motorola 2007-Jul-05 - Eliminate competition on ATOD converter
 *                        Support ATOD phasing on battery thermistor
 * Motorola 2007-Jan-25 - Adding support for power management
 * Motorola 2007-Jan-08 - Updated copyright
 * Motorola 2006-Dec-05 - Fix issue on TX atod
 * Motorola 2006-Nov-01 - Support LJ7.1 Reference Design
 * Motorola 2006-Oct-02 - Removing limit check for phased slope 
 * Motorola 2006-Oct-10 - Update File
 * Motorola 2006-Aug-10 - Fix issues on A/D measurement over battery and charger current 
 * Motorola 2006-Jul-31 - Update comments
 * Motorola 2006-Jul-25 - Fix for RTCBATT Voltage AtoD reading
 * Motorola 2006-Jul-20 - Support ambient light sensor
 * Motorola 2006-Jul-11 - Thermistor on/off for AtoD reading
 * Motorola 2006-May-15 - Moved some variable declarations to start of functions.
 * Motorola 2006-Feb-07 - Updated convert_to_milliamps.
 * Motorola 2005-May-12 - Added phasing capability.
 * Motorola 2005-Feb-28 - Add more robust routines to work around hardware issues.
 * Motorola 2004-Dec-17 - Rewrote software for a less generic interface.
 */

/*!
 * @file atod.c
 *
 * @ingroup poweric_atod
 * 
 * @brief Power IC AtoD converter management interface.
 *
 * This file contains all of the functions and internal data structures needed
 * to implement the power IC AtoD interface. This includes:
 *
 *    - The low-level functions required to ensure exclusive hardware access. 
 *    - The public functions that can be used to request AtoD conversions from kernel-space.
 *    - The ioctl() handler called from the driver core to deal with requests from user-space.
 *
 * The main responsibilitiy of the interface is to ensure that simultaneous requests are 
 * handled appropriately. The biggest problem in managing these requests is that while most
 * requests by necessity must be blocked while a conversion is in progress, one type of request
 * (we're calling this a "postponable" request) must cede the hardware to the others if necessary
 * and be retried later.
 *
 * The request that can be postponed is the request for measurements of the battery and current.
 * In certain situations, this conversion must be timed relative to a transmit burst to either 
 * guarantee that the measurement was taken outside or inside the burst. The measurement inside
 * the burst is usually required in order to monitor the battery when close to the shutoff level.
 * Each time the transmitter is keyed, its load causes a significant sag in the battery voltage
 * which can result in the hardware shutting off the phone. This is guarded against by forcing
 * the hardware to take measurements of the battery during the transmit burst so that the phone 
 * can be shut down gracefully if necessary.
 *
 * If a GSM phone were guaranteed to transmit during every frame (4.6ms), then this might not be 
 * a problem. However, because of DTx (Discontinuous Transmission), this is not guaranteed. When
 * the user is not talking and DTx is enabled, the phone does not transmit each frame but instead
 * leaves the transmitter off, periodically updating background noise information instead. In the
 * worst case, this results in a series of bursts for a noise speech encoder frame approximately 
 * every half a second. This would mean that in the very worst case, the converter would be 
 * set up and the conversion would not occur until the next burst that was something like 500ms
 * later.
 *
 * Given a conversion is typically on the order of hundreds of microseconds in duration, a 
 * potential delay of hundreds of milliseconds is unacceptable.
 *
 * Ignoring the mechanism of the conversion request (function call in kernel-space vs. 
 * ioctl() from user-space), in general the two types of conversion are executed as follows.
 *
 * Exclusive conversion:
 * 
 *    - Obtain the hardware lock for hardware access (sleep until obtained).
 *    - Program the hardware for the specific conversion.
 *    - Start the conversion immediately.
 *    - While the conversion has not yet completed:
 *        - poll the hardware for completion.
 *    - Retrieve the results of the conversion from the hardware.
 *    - If a conversion was postponed, restart it.
 *    - Give up the hardware lock so that other conversions may occur.
 *    - Process the samples and return as necessary.
 *
 * This type of conversion will obtain a lock and will hold it for the duration of the conversion.
 * This lock must be obtained by a conversion request before the hardware can be manipulated, so
 * the thread that holds the lock can be assured that the entire process will be completed without
 * interference.
 *
 * There is no delay added to the polling loop other than that incurred in reading the state of
 * the ASC bit. This has been seen to take in the region of 50us and the total time spent waiting
 * in the loop is only about 150us, so there didn't appear to be much reason to add a udelay()
 * or equivalent.
 *
 * Initially a call to schedule() was planned to be executed within the body of the loop, but 
 * since the total polling time is so short, this was removed.
 *
 * The other type of conversion (postponable) will work alongside this.
 *
 * Postponable conversion:
 *
 *    - Obtain the postponable conversion lock (this will ensure only one postponable conversion 
 *      runs at a time), sleeping until obtained.
 *    - Obtain the hardware lock for hardware access (sleep until obtained).
 *    - Program the hardware for a hardware-timed converison
 *    - Give up the hardware lock so that an exclusive conversion may start.
 *    - Wait for the conversion to be completed (or for the timeout to expire).
 *    - Copy the results of the conversion.
 *    - Process the samples and return as necessary.
 *    - Give up the postponable conversion lock so that another postponable converison may occur.
 *
 * By giving up the hardware lock before waiting for the conversion to complete, this allows
 * another exclusive conversion to grab the hardware and interrupt this conversion. Because the
 * hardware lock is not held for the duration of this conversion, a second lock is required to
 * ensure that only one postponable conversion request can proceed at any given time.
 *
 * This requires that the code be written so that both conversions cooperate with each other.
 * In particular, a process that interrupts an postponable conversion must restart that
 * conversion once its exclusive conversion is done.
 *
 * The non-blocking version of a postponable conversion does not cause the calling process to
 * sleep while the conversion is in progress, but instead sets up the hardware for the conversion
 * and returns. The calling process can then later check for the conversion's completion by
 * poll()ing the device.
 * 
 * The end result of all this should be that all exclusive conversions should complete safely, 
 * and that they cannot be delayed in the case where another postponable conversion is waiting.
 *
 * IMPORTANT: the kernel-space functions assume that they are called from a thread as they will
 * cause that thread to sleep. To avoid kernel panics, these functions will check their context
 * and will fail if currently in an interrupt context.
 */

#include <asm/delay.h>
#include <asm/errno.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/power_ic.h>
#include <linux/power_ic_kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/types.h>
#include <stdbool.h>

#include "../core/gpio.h"
#include "../core/os_independent.h"

#include "atod.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
EXPORT_SYMBOL(power_ic_atod_single_channel);
EXPORT_SYMBOL(power_ic_atod_general_conversion);
EXPORT_SYMBOL(power_ic_atod_raw_conversion);
#endif

/******************************************************************************
* Local constants
******************************************************************************/
#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* Bits in ADC0... */
#define TS_M_MASK              0x0007000
#define TS_M_SHIFT             12
#define TS_REF_LOW_PWR_MASK    0x0000800
#define TS_REFENB_MASK         0x0000400
#define ADINC1_MASK            0x0010000
#define ADINC2_MASK            0x0020000
#define BATT_I_ADC_MASK        0x0000004
#define CHRG_I_ADC_MASK        0x0000002
#define LICELLCON_INDEX        0
#define LICELLCON_BITS         1
#define BIT_ENABLE             1
#define BIT_DISABLE            0
#define PHASING_NEEDED         1
#define PHASING_NOT_NEEDED     0

/* Bits in ADC1... */
#define ADEN_MASK              0x0000001
#define RAND_MASK              0x0000002
#define AD_SEL1_MASK           0x0000008
#define AD_SEL1_SHIFT          3
#define ADA1_MASK              0x00000E0
#define ADA1_SHIFT             5
#define ADA2_MASK              0x0000700
#define ADA2_SHIFT             8
#define ATO_MASK               0x007F800
#define ATO_SHIFT              11
#define ATOX_MASK              0x0080000
#define ATOX_SHIFT             19
#define ASC_MASK               0x0100000
#define ASC_SHIFT              20
#define ADTRIGIGN_MASK         0x0200000
#define ADTRIG_ONESHOT_MASK    0x0400000

/* Bits in ADC2. */
#define ADD1_MASK              0x0000FFC
#define ADD1_SHIFT             2
#define ADD2_MASK              0x0FFC000
#define ADD2_SHIFT             14

/* The number of channels sampled in a single bank. */
#define SAMPLES_PER_BANK       8

/* The delay programmed for in-burst and out of burst conversions, in cycles of the
 * 16.384kHz ATO clock. For now, these are the same timings as used in the past. */
#define DELAY_IN_BURST      7
#define DELAY_OUT_OF_BURST  0x4B

/* Used for register definitions to cut down on the amount of duplicate code */
#define GET_RESULTS_ADC           POWER_IC_REG_ATLAS_ADC_2
#define DISABLE_CONVERT_REG       POWER_IC_REG_ATLAS_ADC_1
#define ARM_ONE_SHOT_REG          POWER_IC_REG_ATLAS_ADC_1
#define START_CONVERT_REG_ASC     POWER_IC_REG_ATLAS_ADC_1
#define START_CONVERT_REG_ATO     POWER_IC_REG_ATLAS_ADC_1
#define TS_LOW_PWR_MOD_REG        POWER_IC_REG_ATLAS_ADC_0

#define EVENT_ADCDONEI            POWER_IC_EVENT_ATLAS_ADCDONEI
#define EVENT_TSI                 POWER_IC_EVENT_ATLAS_TSI     

/* An current AtoD is converted to millamps using these. According to hardware team,
 * current in mA = (atod - 238) * 5.6 mA/count. AtoD measurements below the min current
 * will be zeroed as this is the offset that is subtracted from the AtoD before scaling. */
#define CONVERT_MA_MIN_CURRENT      238
#define CONVERT_MA_MULT             5600
#define CONVERT_MA_DIV              1000

/* These values are used by Atlas for battery current alginment and conversion */
#define BATTI_ZERO                   512          /* Used to align the atod reading on battery current */
#define BATTI_READING_DIVIDER        1024         /* For calculation of battery current */
#define BATTI_READING_RESOLUTION     (int)(5.6 * BATTI_READING_DIVIDER)   /* For Atlas, a/d on batti is 5.6mA per count */
#define NEGATIVE_BATTI_MASK          0xFFFFFC00   /* Used to convert negative battery current */
#define SIGNED_PHASING_MIN_RESULT    -512         /*<! Minimum 10-bit result allowed after phasing. */
#define SIGNED_PHASING_MAX_RESULT    511          /*<! Maximum 10-bit result allowed after phasing. */

#define ATOD_NUM_RAW_RESULTS  8

/* Defines for Charger Thermistor control 
 * GPO1 for BUTE
 * GPO3 for all other ATLAS products
 */
#define CHRG_THERM_REG    POWER_IC_REG_ATLAS_PWR_MISC
#define CHRG_THERM_BITS   1
#define CHRG_THERM_ON     1
#define CHRG_THERM_OFF    0

#ifdef CONFIG_ARCH_MXC91321
#define CHRG_THERM_INDEX  6
#else
#define CHRG_THERM_INDEX  10
#endif

/* This is the offset used in determining whether any samples taken should be filtered.
 * if a sample is more than this many counts below the highest sample in the set, it
 * is dropped from the average. */
#define HACK_SAMPLE_FILTER_OFFSET       20

/* If we drop more than this number of samples, we'll retry the conversion. */
#define HACK_TOO_MANY_SAMPLES_FILTERED  3

#endif /* Doxygen skips over this */

/*! @name Phasing Constants
 * These are various constants used for phasing.
 */
/*@{*/
/*! Default zero offset correction. */
#define PHASING_ZERO_OFFSET     0x00
/*! Default zero slope correction. */
#define PHASING_ZERO_SLOPE      0x80
/*! Minimum 10-bit result allowed after phasing. */
#define PHASING_MIN_RESULT      0x0000
/*! Maximum 10-bit result allowed after phasing. */
#define PHASING_MAX_RESULT      0x03FF
/*!  8-bit offset multiplied to be applied to 10-bit AtoD. */
#define PHASING_OFFSET_MULTIPLY 4
/*! Shift for division after slope multipler. */
#define PHASING_SLOPE_DIVIDE    7
/*! Phasing not available */
#define PHASING_NOT_AVAILABLE   -1
/*@}*/


/******************************************************************************
* Local type definitions
******************************************************************************/

/*!
 * @brief  Used internally to indicate which type of conversion has been requested.
 */
typedef enum
{
    POWER_IC_ATOD_TYPE_SINGLE_CHANNEL, /*!< An immediate conversion of a single channel. */
    POWER_IC_ATOD_TYPE_BATT_AND_CURR,  /*!< Battery and current conversion. */
    POWER_IC_ATOD_TYPE_GENERAL,        /*!< Typical set of conversions - temperature, supply, etc. */
    POWER_IC_ATOD_TYPE_RAW             /*!< Intended only for testing the hardware. */
} POWER_IC_ATOD_TYPE_T;

/*!
 * @brief  The structure used to specify what conversion exclusive_conversion() should set up.
 */
typedef struct
{
    POWER_IC_ATOD_TYPE_T     type;    /*!< The type of the conversion. */
    POWER_IC_ATOD_CHANNEL_T  channel; /*!< Only needed for the single-channel conversion. */    
    POWER_IC_ATOD_TIMING_T   timing;  /*!< Ignored for single-channel conversion. */
    POWER_IC_ATOD_CURR_POLARITY_T  curr_polarity; /*!< Polarity for batt/current conversion. */
    int                      timeout; /*!< Timeout for postponable conversion, in seconds. */
    POWER_IC_ATOD_AD6_T      ad6_sel; /*!< Indicates to take atod on PA temperature or Licell */
} POWER_IC_ATOD_REQUEST_T;

/*!
 * @brief  Pick a bank of channels...
 */
typedef enum
{
    POWER_IC_ATOD_BANK_0,
    POWER_IC_ATOD_BANK_1
} POWER_IC_ATOD_BANK_T;

/*!
 * @brief  The phasing data is stored in NVM in the following byte order.
 *
 * It is important that this order be kept the same for both Linux and legacy products.
 */ 
typedef enum
{
    ATOD_PHASING_BATT_CURR_OFFSET,
    ATOD_PHASING_BATT_CURR_SLOPE,
    ATOD_PHASING_CHRGR_CURR_OFFSET,
    ATOD_PHASING_CHRGR_CURR_SLOPE,
    ATOD_PHASING_BATTPLUS_OFFSET,
    ATOD_PHASING_BATTPLUS_SLOPE,
    ATOD_PHASING_BPLUS_OFFSET,
    ATOD_PHASING_BPLUS_SLOPE,
    ATOD_PHASING_THERMISTOR_OFFSET,
    ATOD_PHASING_THERMISTOR_SLOPE,
    ATOD_PHASING_MOBPORTB_OFFSET,
    ATOD_PHASING_MOBPORTB_SLOPE,
    
    ATOD_PHASING_NUM_VALUES

} ATOD_PHASING_VALUES_T;

typedef enum
{
    ATOD_CHANNEL_ATLAS_BATT,
    ATOD_CHANNEL_ATLAS_BATT_CURR,
    ATOD_CHANNEL_ATLAS_BPLUS,
    ATOD_CHANNEL_ATLAS_MOBPORTB,
    ATOD_CHANNEL_ATLAS_CHRG_CURR,
    ATOD_CHANNEL_ATLAS_TEMPERATURE,
    ATOD_CHANNEL_ATLAS_COIN_CELL,
    ATOD_CHANNEL_ATLAS_PA_THERM = ATOD_CHANNEL_ATLAS_COIN_CELL, 
    ATOD_CHANNEL_ATLAS_CHARGER_ID,
    ATOD_CHANNEL_ATLAS_AD8,  
    ATOD_CHANNEL_ATLAS_AD9,
    ATOD_CHANNEL_ATLAS_AD10,
    ATOD_CHANNEL_ATLAS_AD11,
    
    ATOD_CHANNEL_NUM_VALUES
} ATOD_CHANNEL_T;

/******************************************************************************
* Local variables
******************************************************************************/
/*! This mutex must be grabbed before altering any converter hardware setting. */
static DECLARE_MUTEX(hardware_mutex);

/*! This mutex ensures that only one postponable conversion is being handled at any time. */
static DECLARE_MUTEX(postponable_mutex);

#ifndef DOXYGEN_SHOULD_SKIP_THIS /* Doxygen does a really bad job of documenting this stuff... */
/*! Completion for the postponable conversion - uses a queue to allow a timeout. */
DECLARE_WAIT_QUEUE_HEAD(postponable_complete_queue);
#endif

/*! This flag will be set while a conversion that can be postponed is in progress, This flag
 *  must only be modified when the converter lock is held. */
static bool postponable_conversion_started = false;

/*! This variable is used to remember the current polarity originally requested for a
 *  postponable conversion. */
static POWER_IC_ATOD_CURR_POLARITY_T postponable_conversion_polarity = POWER_IC_ATOD_CURR_POLARITY_DISCHARGE;

/*! Remembers the AtoD timing originally requested for a postponable conversion. */
static POWER_IC_ATOD_TIMING_T postponable_conversion_timing = POWER_IC_ATOD_TIMING_IN_BURST;

/*!  This flag will be set when a postponable conversion is complete.*/
static bool postponable_conversion_event_flag = false;

/*! Holds the results from a postponable conversion. The results are fetched when the interrupt
 *  occurs so that they are not potentially overwritten by a following exclusive conversion. */
static int postponable_results[SAMPLES_PER_BANK];

/*! Indicates whether an error occurred when fetching the results of a postponable conversion. */
static int postponable_results_error = 0;
 
/*! Phasing data is stored here. */
static unsigned char phasing_values[ATOD_PHASING_NUM_VALUES] =
{
    /* Batt Current. */
    PHASING_ZERO_OFFSET,
    PHASING_ZERO_SLOPE,
    /* Unused. */
    PHASING_ZERO_OFFSET,
    PHASING_ZERO_SLOPE,
    /* Batt+. */
    PHASING_ZERO_OFFSET,
    PHASING_ZERO_SLOPE,
    /* B+. */
    PHASING_ZERO_OFFSET,
    PHASING_ZERO_SLOPE,
    /* Batt Thermistor. */
    PHASING_ZERO_OFFSET,
    PHASING_ZERO_SLOPE,
    /* MobportB. */
    PHASING_ZERO_OFFSET,
    PHASING_ZERO_SLOPE
};

/*! Lookup table pairing AtoD channels with phasing data. Used to indicate whether 
 *  phasing is available for each channel. Channels that do not have phasing available
 *  will be marked with ATOD_PHASING_NUM_VALUES. Those that have phasing available will
 *  will be marked with the index of the phasing offset for the channel. */
static const int phasing_channel_lookup[ATOD_CHANNEL_NUM_VALUES] = 
{
    ATOD_PHASING_BATTPLUS_OFFSET,
    ATOD_PHASING_BATT_CURR_OFFSET,
    ATOD_PHASING_NUM_VALUES,      /* Application supply */
    ATOD_PHASING_MOBPORTB_OFFSET,  
    ATOD_PHASING_CHRGR_CURR_OFFSET,      /* Charger current */
    ATOD_PHASING_THERMISTOR_OFFSET, /* Temperature */
    ATOD_PHASING_NUM_VALUES,      /* Coin Cell */
    ATOD_PHASING_NUM_VALUES,      /* Atlas Die Temperature */
    ATOD_PHASING_NUM_VALUES,
    ATOD_PHASING_NUM_VALUES,
    ATOD_PHASING_NUM_VALUES,
    ATOD_PHASING_NUM_VALUES
};

/*! Lookup table pairing AtoD channels with the actual channel for Atlas. 
 *  The values in this table match the order of the enum in power_ic.h which is NOT
 *  hardware dependent.
 */
static const int atod_channel_lookup[POWER_IC_ATOD_NUM_CHANNELS] = 
{
    ATOD_CHANNEL_ATLAS_PA_THERM,
    ATOD_CHANNEL_ATLAS_BATT,
    ATOD_CHANNEL_ATLAS_BATT_CURR,
    ATOD_CHANNEL_ATLAS_BPLUS,
    ATOD_CHANNEL_ATLAS_CHARGER_ID,
    ATOD_CHANNEL_ATLAS_CHRG_CURR,
    ATOD_CHANNEL_ATLAS_COIN_CELL,
    ATOD_CHANNEL_ATLAS_MOBPORTB,
    ATOD_CHANNEL_ATLAS_TEMPERATURE,
    ATOD_CHANNEL_ATLAS_AD8
};

static POWER_IC_ATOD_TIMING_T last_conversion = POWER_IC_ATOD_TIMING_IMMEDIATE;

/******************************************************************************
* Local function prototypes
******************************************************************************/
static int phasing_available(POWER_IC_ATOD_CHANNEL_T channel);
static int set_phasing_values(unsigned char * values);
static int phase_atod(int offset, int slope, int atod);
static int phase_atod_10(int offset, int slope, int atod);
static int phase_channel(POWER_IC_ATOD_CHANNEL_T channel, int atod);
static int disable_converter(void);
static int arm_one_shot(void);
static int start_conversion(POWER_IC_ATOD_REQUEST_T * request);
static int set_single_channel_conversion(POWER_IC_ATOD_REQUEST_T * request);
static int set_bank_conversion(POWER_IC_ATOD_BANK_T bank, POWER_IC_ATOD_REQUEST_T * request);
static int set_batt_current_conversion(POWER_IC_ATOD_CURR_POLARITY_T curr_polarity);
static int set_up_hardware_exclusive_conversion(POWER_IC_ATOD_REQUEST_T * request);
static int get_results(int * results);
static int convert_to_milliamps(int sample);
static int exclusive_conversion(POWER_IC_ATOD_REQUEST_T * request, int * samples);
static int non_blocking_conversion(POWER_IC_ATOD_REQUEST_T * request);

static int atod_complete_event_handler(POWER_IC_EVENT_T unused);

static int HACK_sample_filter(int * samples, int * average);

static int power_ic_atod_nonblock_begin_conversion(POWER_IC_ATOD_TIMING_T timing,
                                            POWER_IC_ATOD_CURR_POLARITY_T polarity);
static int power_ic_atod_nonblock_cancel_conversion(void);
static ATOD_CHANNEL_T atod_channel_available(POWER_IC_ATOD_CHANNEL_T channel);
static int power_ic_atod_get_therm(void);
static int power_ic_atod_set_therm(int on);
static int power_ic_atod_dummy_conversion(void);
static int power_ic_atod_get_lithium_coin_cell_en(void);
static int power_ic_atod_set_lithium_coin_cell_en(int value);
static int power_ic_atod_current_and_batt_conversion(POWER_IC_ATOD_TIMING_T timing,
                                                         int * batt_result, int * curr_result, int phasing);
/******************************************************************************
* Local functions
******************************************************************************/

/*!
 * @brief Indicates if phasing is available.
 *
 * This function indicates whether phasing is available for a channel and the 
 * location of the phasing parameters in the phasing table.
 *
 * @param        channel   The AtoD channel to check for phasing.
 *
 * @return returns PHASING_NOT_AVAILABLE if no phasing is available, or the 
 * index in the phasing table of the phasing offset if phasing exists.
 */
static int phasing_available(POWER_IC_ATOD_CHANNEL_T channel)
{
    if(phasing_channel_lookup[channel] != ATOD_PHASING_NUM_VALUES)
    {
        return phasing_channel_lookup[channel];
    }
    
    return PHASING_NOT_AVAILABLE;
}

/*!
 * @brief Sets the table of phasing values.
 *
 * This function takes a set of phasing values and, after some basic checking
 * for validity, overwrites the driver's table of values.
 *
 * @param        values    The new phasing values to be set.
 *
 * @return returns 0 if the new values are accepted and copied successfully.
 */
static int set_phasing_values(unsigned char * values)
{
    int i;
    
    tracemsg(_k_d("   Setting new AtoD phasing values."));
    
    for(i = 1; i< ATOD_PHASING_NUM_VALUES; i+=2)
    {
        tracemsg(_k_d("   values[%d..%d]: 0x%X, 0x%X"), i-1, i, values[i-1], values[i]);
    }

    /* If a signal caused us to continue without getting the mutex, then quit with an error. */
    if(down_interruptible(&hardware_mutex) != 0)
    {
        printk("POWER IC:   process received signal while waiting for hardware mutex. Exiting.");
        return -EINTR;
    }
    
    memcpy(phasing_values, values, ATOD_PHASING_NUM_VALUES);
    up(&hardware_mutex);
    
    return 0;
}

/*!
 * @brief Phases a single AtoD measurement.
 *
 * This function uses the supplied phasing parameters to correct slight 
 * imperfections in the response of the AtoD converter.
 *
 * @param        offset   The offset that should be applied from phasing. Note
 *                        that this is a signed value.
 * @param        slope    The multiplier for correcting the slope of the AtoD response.
 * @param        atod     The AtoD measurement to be phased.
 *                        
 * @return returns 0 if successful.
 *
 * @note Where possible, phase_channel() should be used instead of this function. This
 * function presumes that the caller knows how the phasing data is stored and whether
 * phasing is available for the AtoD channel, whereas phase_channel() does all that for
 * the caller based on the passed channel.
 */
static int phase_atod(int offset, int slope, int atod)
{
    int phased_atod = atod;
    
    /* First, correct the slope if the phasing indicates an adjustment is needed. */
    if(slope != PHASING_ZERO_SLOPE)
    {
        phased_atod = phased_atod * slope;
        phased_atod = phased_atod >> PHASING_SLOPE_DIVIDE;
    }
    
    /* Next, correct any fixed offset in the response. The offset is just a signed
     * 8-bit value that should be added to the AtoD measurement. */
    phased_atod += (offset * PHASING_OFFSET_MULTIPLY);
    
    /* Finally, restrict the result to the valid range of AtoD measurements. */
    if(phased_atod > PHASING_MAX_RESULT)
    {
        phased_atod = PHASING_MAX_RESULT;
    }
    else if(phased_atod < PHASING_MIN_RESULT)
    {
        phased_atod = PHASING_MIN_RESULT;
    }
    
    return phased_atod;
}

/*!
 * @brief Phases a single AtoD measurement (for use with unscaled offsets).
 *
 * This function uses the supplied phasing parameters to correct slight 
 * imperfections in the response of the AtoD converter.
 *
 * @param        offset   The offset that should be applied from phasing. Note
 *                        that this is a signed value and is the raw value to be 
 *                        applied to the 10-bit DAC reading.
 * @param        slope    The multiplier for correcting the slope of the AtoD response.
 * @param        atod     The AtoD measurement to be phased.
 *                        
 * @return returns 0 if successful.
 *
 * @note Where possible, phase_channel() should be used instead of this function. This
 * function presumes that the caller knows how the phasing data is stored and whether
 * phasing is available for the AtoD channel, whereas phase_channel() does all that for
 * the caller based on the passed channel.
 */
static int phase_atod_10(int offset, int slope, int atod)
{
    int phased_atod = atod;
    
    /* First, correct the slope if the phasing indicates an adjustment is needed. */
    if(slope != PHASING_ZERO_SLOPE)
    {
        phased_atod = phased_atod * slope;
        phased_atod = phased_atod >> PHASING_SLOPE_DIVIDE;
    }
    
    /* Next, correct any fixed offset in the response. The offset is just a signed
     * 8-bit value that should be added to the AtoD measurement.  This is for the 
     * special cases when the offset is with respect to the raw 10-bit DAC and has 
     * not been scaled for 8-bit. */
    phased_atod += offset;
    
    /* Finally, restrict the result to the valid range of AtoD measurements. */
    if(phased_atod > SIGNED_PHASING_MAX_RESULT)
    {
        phased_atod = PHASING_MAX_RESULT;
    }
    else if(phased_atod < SIGNED_PHASING_MIN_RESULT)
    {
        phased_atod = PHASING_MIN_RESULT;
    }
    return phased_atod;
}

/*!
 * @brief Phases an AtoD measurement from a given channel.
 *
 * This function phases a channel's AtoD measurement based on whether phasing is available
 * and handles the stored phasing parameters so that the caller doesn't need to care.
 *
 * @param        channel  The channel from which the AtoD measurement was taken.
 * @param        atod     The AtoD measurement to be phased.
 *                        
 * @return returns 0 if successful.
 *
 * @note Where possible, phase_channel() should be used in preference to phase_atod().
 * This function hides the knowledge of all phasing parameters and operations from the
 * caller and is much nicer to use than phase_atod(). The main exception is current
 * measurements - they are not a discrete channel in themselves, so they don't conform
 * to the way the rest of the phasing is set up.
 */
static int phase_channel(POWER_IC_ATOD_CHANNEL_T channel, int atod)
{
    int phased_atod = atod;
    int phasing_offset;
    signed char signed_offset;
    int atod_channel = atod_channel_available(channel);
    
    if(atod_channel == ATOD_CHANNEL_NUM_VALUES)
    {
        printk("POWER IC: AtoD channel is not a valid channel on this Power IC.");
        return -EINVAL;
    }
    
    phasing_offset = phasing_available(atod_channel);
    
    /* We only need to phase the measurement if there is phasing available for this
     * channel. */
    if(phasing_offset != PHASING_NOT_AVAILABLE)
    {
        /* The offset is actually a signed value. */
        signed_offset = (signed char) phasing_values[phasing_offset];
        
        if ((channel == POWER_IC_ATOD_CHANNEL_BATT_CURR) ||
            (channel == POWER_IC_ATOD_CHANNEL_CHRG_CURR))
        {
            phased_atod = phase_atod_10(signed_offset, phasing_values[phasing_offset + 1], atod);  
        }
        else
        {
            phased_atod = phase_atod(signed_offset, phasing_values[phasing_offset + 1], atod);
        }
    }
    return phased_atod;
}

/*!
 * @brief Disables the converter.
 *
 * @pre The caller of this function must hold the hardware lock prior to 
 * using this function.
 *
 * @return returns 0 if successful.
 */
static int disable_converter(void)
{
    return(power_ic_set_reg_mask(DISABLE_CONVERT_REG, ADEN_MASK | ADTRIGIGN_MASK , ADTRIGIGN_MASK));
}

/*!
 * @brief Arms the one-shot trigger for a conversion.
 *
 * This function arms the one-shot hardware trigger for a conversion. The one-shot 
 * trigger ensures that after a conversion completes, no further conversions will be 
 * performed on subsequent transmit bursts, ensuring that the software can retrieve the
 * results of the conversion This is only needed for a hardware-timed conversion.
 *
 * @pre The caller of this function must hold the hardware lock prior to 
 * using this function.
 *
 * @return 0 if successful.
 */
static int arm_one_shot(void)
{
    int error;
    
    /* One-shot is armed by clearing and then setting the ADTRIG_ONESHOT bit. */
    error = power_ic_set_reg_mask(ARM_ONE_SHOT_REG, ADTRIG_ONESHOT_MASK, 0);
    
    if(error != 0)
    {
        return error;
    }
    
    return (power_ic_set_reg_mask(ARM_ONE_SHOT_REG, ADTRIG_ONESHOT_MASK | ADTRIGIGN_MASK, ADTRIG_ONESHOT_MASK));
}

/*!
 * @brief Starts an AtoD conversion.
 * 
 * This function triggers an AtoD conversion with the currently programmed settings.
 * The function will either start a conversion immediately or set up a hardware-timed
 * conversion taken either inside or outside the transmit burst depending on the input.
 *
 * @pre The caller of this function must hold the hardware lock prior to 
 * using this function.
 *
 * @param         request   Pointer to structure that describes the request.
 *
 * @return 0 if successful.
 */
static int start_conversion(POWER_IC_ATOD_REQUEST_T * request)
{
    int mask;
    int setup;
    unsigned short ato = 0;
    unsigned short atox = 0;
    int error;

    last_conversion = request->timing;

    if(request->timing == POWER_IC_ATOD_TIMING_IMMEDIATE)
    {
       if( (request->channel == POWER_IC_ATOD_CHANNEL_CHRG_CURR)  &&  (request->type == POWER_IC_ATOD_TYPE_SINGLE_CHANNEL) )
       {
           atox = 1;
       }
 
       /* An immediate conversion is triggered by setting the ASC bit. */   
       mask = ASC_MASK | ADTRIGIGN_MASK | ADTRIG_ONESHOT_MASK | ATO_MASK | ATOX_MASK;
       setup = ASC_MASK | ADTRIGIGN_MASK |((ato << ATO_SHIFT) & ATO_MASK) | ((atox << ATOX_SHIFT) & ATOX_MASK);       
       return(power_ic_set_reg_mask(START_CONVERT_REG_ASC, mask, setup));
   }
    else /* This is a hardware-timed conversion. */
    {
        if(request->timing == POWER_IC_ATOD_TIMING_IN_BURST)
        {
            ato = DELAY_IN_BURST;
            atox = 0;
        }
        else
        {
            ato = DELAY_OUT_OF_BURST;
            atox = 0;
        }
        
        /* The timing for a hardware-controlled conversion is handled by setting ATO and ATOX. */
        mask = ATO_MASK | ATOX_MASK | ADTRIGIGN_MASK;
        setup = ((ato << ATO_SHIFT) & ATO_MASK) | ((atox << ATOX_SHIFT) & ATOX_MASK) | ADTRIGIGN_MASK;
        
        tracemsg(_k_d("    hardware-timed conversion - setting delay: mask 0x%X, data 0x%X."), 
                 mask, setup); 

        error = power_ic_set_reg_mask(START_CONVERT_REG_ATO, mask, setup);
        
        if(error != 0)
        {
            return error;
        }
        
        /* Allow the conversion to begin when the hardware is ready. */
        return(arm_one_shot());
    }
}

/*!
 * @brief Sets up conversion of a single AtoD channel.
 *
 * This function programs the hardware to convert a single channel. The converter 
 * will be enabled if this succeeds and will then be ready to perform the conversion.
 *
 * @pre The caller of this function must hold the hardware lock prior to 
 * using this function.
 *
 * @param        request   Pointer to structure that describes the request.
 *
 * @return 0 if successful.
 */
static int set_single_channel_conversion(POWER_IC_ATOD_REQUEST_T * request)
{
    int setup;
    int atod_channel = atod_channel_available(request->channel);
    POWER_IC_UNUSED int error = 0;
    
    if(atod_channel == ATOD_CHANNEL_NUM_VALUES)
    {
        printk("POWER IC: AtoD channel is not a valid channel on this Power IC.");
        return -EINVAL;
    }

    /* If reading the thermistor, turn on the corresponding GPO */
    if(request->channel == POWER_IC_ATOD_CHANNEL_TEMPERATURE)
    {
        error |= power_ic_atod_set_therm(BIT_ENABLE);
        mdelay(2);
    }

    if((request->channel == POWER_IC_ATOD_CHANNEL_COIN_CELL) && (power_ic_atod_get_lithium_coin_cell_en() == 0))
    {
        /* Enable the lithium coin cell reading */
        error |= power_ic_atod_set_lithium_coin_cell_en(BIT_ENABLE);
    }
    else if((request->channel == POWER_IC_ATOD_CHANNEL_PA_THERM) && (power_ic_atod_get_lithium_coin_cell_en() != 0))
    {
        /* Enable the PA thermistor reading */
        error |= power_ic_atod_set_lithium_coin_cell_en(BIT_DISABLE);
    }
    
    /* Pick the bank and channel. */
    if(atod_channel < SAMPLES_PER_BANK)
    {
        setup = ADEN_MASK | RAND_MASK | ((atod_channel << ADA1_SHIFT) & ADA1_MASK);
    }
    else
    {
        setup = ADEN_MASK | RAND_MASK | AD_SEL1_MASK | 
            (((atod_channel - SAMPLES_PER_BANK) << ADA1_SHIFT) & ADA1_MASK);
    }
    
    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_ADC_0, (BATT_I_ADC_MASK | CHRG_I_ADC_MASK), (BATT_I_ADC_MASK | CHRG_I_ADC_MASK));
    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_ADC_1,(ADEN_MASK | RAND_MASK | AD_SEL1_MASK 
                                                             | ADA1_MASK), setup);

    return error; 
}

/*!
 * @brief Sets up a conversion of all the AtoD channels on either bank.
 *
 * This function programs the hardware to convert either bank of 7 channels.  The 
 * converter will be enabled if this succeeds and will then be ready to perform 
 * the conversion.
 *
 * @pre The caller of this function must hold the hardware lock prior to 
 * using this function.
 *
 * @param        bank      The bank of channels to be sampled.
 * @param        request   Pointer to structure that describes the request.
 *
 * @return 0 if successful.
 */
static int set_bank_conversion(POWER_IC_ATOD_BANK_T bank, POWER_IC_ATOD_REQUEST_T * request)
{
    int setup= ADEN_MASK | ((bank << AD_SEL1_SHIFT) & AD_SEL1_MASK);
    
    int error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_ADC_0, (BATT_I_ADC_MASK | CHRG_I_ADC_MASK), 0);
    
    /* If reading the thermistor, turn on the corresponding GPO */
    error |= power_ic_atod_set_therm(BIT_ENABLE);
    mdelay(2);
    
    if((request->ad6_sel == POWER_IC_ATOD_AD6_LI_CELL) && (power_ic_atod_get_lithium_coin_cell_en() == 0))
    {
        /* Enable the lithium coin cell reading */
        error |= power_ic_atod_set_lithium_coin_cell_en(BIT_ENABLE);
    }
    else if((request->ad6_sel == POWER_IC_ATOD_AD6_PA_THERM) && (power_ic_atod_get_lithium_coin_cell_en() != 0))
    {
        /* Enable the PA thermistor reading */
        error |= power_ic_atod_set_lithium_coin_cell_en(BIT_DISABLE);
    }
    
    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_ADC_1, (ADEN_MASK | RAND_MASK 
        | AD_SEL1_MASK), setup);
           
    return error;
}

/*!
 * @brief Sets up a conversion of both battery voltage and current.
 * 
 * This function programs the hardware to perform a battery/current conversion. The hardware
 * provides a mode where the current drawn from the battery can be sampled at the same time
 * as an AtoD reading of the battery. The charger software can then use both pieces of 
 * information to better deal with sags in battery voltage.
 *
 * @pre The caller of this function must hold the hardware lock prior to 
 * using this function.
 *
 * @param        curr_polarity  Selects in which direction current will be measured.
 *
 * @return returns 0 if successful.
 */
static int set_batt_current_conversion(POWER_IC_ATOD_CURR_POLARITY_T curr_polarity)
{
    int setup;
    int channel = atod_channel_available(POWER_IC_ATOD_CHANNEL_BATT_CURR);
    int error;
    
    if(channel == ATOD_CHANNEL_NUM_VALUES)
    {
        printk("POWER IC: AtoD channel is not a valid channel on this Power IC.");
        return -EINVAL;
    }

    /* A battery & current conversion is enabled by setting the BATT_I_ADC bit.
     * RAND is set to take 4 pairs of voltage and current. */
    setup = ADEN_MASK | RAND_MASK | (channel << ADA1_SHIFT);
    error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_ADC_0, BATT_I_ADC_MASK, BATT_I_ADC_MASK);
    error |= power_ic_set_reg_mask(POWER_IC_REG_ATLAS_ADC_1,(ADEN_MASK | RAND_MASK 
        | AD_SEL1_MASK | ADA1_MASK), setup);
    
    return error;
}

/*!
 * @brief Sets the hardware for a specified type of conversion.
 * 
 * This function prepares the hardware to perform the specified conversion. The conversion
 * will not be triggered, but the hardware will be set up so the specified conversion will
 * be performed once the converter is triggered.
 *
 * @pre The caller of this function must hold the hardware lock prior to using
 * this function. Although this function does not directly alter the hardware's
 * settings, the functions it calls do.
 *
 * @param        request   Pointer to structure that describes the request.
 *
 * @return 0 if successful.
 */
static int set_up_hardware_exclusive_conversion(POWER_IC_ATOD_REQUEST_T * request)
{
    switch(request->type)
    {
        case POWER_IC_ATOD_TYPE_SINGLE_CHANNEL:
        case POWER_IC_ATOD_TYPE_RAW: /* Fallthrough - only results of raw conversion differ. */
            return (set_single_channel_conversion(request));
            
        case POWER_IC_ATOD_TYPE_BATT_AND_CURR:
            return(set_batt_current_conversion(request->curr_polarity));
            
        case POWER_IC_ATOD_TYPE_GENERAL:
            return(set_bank_conversion(POWER_IC_ATOD_BANK_0, request));
            
        default:
            return -EINVAL;
    }     
}

/*!
 * @brief Fetches the results of a previously completed conversion.
 *
 * This function reads the conversion results from the hardware. The results are not 
 * interpreted in any way - the function just passes back the results with no
 * knowledge of their meaning.
 *
 * @pre The caller of this function must hold the hardware lock prior to 
 * using this function.
 *
 * @param        results   Pointer to array that will store the results. The array must
 *                         a minimum of SAMPLES_PER_BANK elements in size.
 *
 * @return 0 if successful.
 */
static int get_results(int * results)
{
    int setup;
    int mask = ADA1_MASK | ADA2_MASK;
    int error;
    int i, j;
    int result;
    
    /* To save time and SPI traffic, fetch the results two at a time. */
    for (i = 0; i < SAMPLES_PER_BANK; i+=2)
    {
        j = i+1;
        
        setup = (i << ADA1_SHIFT) | (j  << ADA2_SHIFT);
 
        error = power_ic_set_reg_mask(POWER_IC_REG_ATLAS_ADC_1, mask, setup);

        if(error != 0)
        {
            printk(("POWER IC:   error %d while selecting result."), error);
            return error;
        }
        
        /* Get the results. This retrieves two channels at once. */
        error = power_ic_read_reg(GET_RESULTS_ADC, &result);
        
        if(error != 0)
        {
            printk(("POWER IC:   error %d while fetching result %d."), error, i);
            return error;
        }

        results[i] = (result & ADD1_MASK) >> ADD1_SHIFT;

        /* For SCM-A11, there are two results, and the SAMPLES_PER_BANK is increased by 1,
           so the value of three still applies. */
        if(j < SAMPLES_PER_BANK)
        {
            results[j] = (result & ADD2_MASK) >> ADD2_SHIFT;
        }
    }
    
    return 0;
}

/*!
 * @brief Converts a current AtoD measurement to milliamps.
 *
 * @param        sample    AtoD sample to be converted.
 *
 * @return       current in milliamps.
 */
static int convert_to_milliamps(int sample)
{
    return ((sample * BATTI_READING_RESOLUTION) / BATTI_READING_DIVIDER);
}

/*!
 * @brief Performs a conversion immediately that cannot be interrupted by other conversions.
 *
 * This function performs a conversion that cannot be interrupted by any other
 * conversion. The conversion is performed immediately. If the converter is waiting
 * for a hardware-timed conversion to occur, then this will steal the hardware and 
 * perform this conversion before returning control to the other process to retry the
 * hardware-timed conversion.
 *
 * @pre This function tries to claim the hardware lock, The caller cannot already
 * hold this lock or deadlock will result.
 *
 * @pre This function will cause the calling process to sleep if another conversion
 * is in progress, and as such cannot be used in an interrupt context.
 *
 * @param        request   Pointer to structure that describes the request.
 * @param        samples   Pointer to an array where the raw samples should be stored.
 *                         The array must a minimum of SAMPLES_PER_BANK elements in size.
 *
 * @return 0 if successful.
 */
static int exclusive_conversion(POWER_IC_ATOD_REQUEST_T * request, int * samples)
{
    int error;
    int conversion_in_progress = true;

    /* If a signal caused us to continue without getting the mutex, then quit with an error. */
    if(down_interruptible(&hardware_mutex) != 0)
    {
        printk("   process received signal while waiting for hardware mutex. Exiting.");
        return -EINTR;
    }
    
    /* Check to see if we have stolen the converter while it was waiting for a hardware-timed
     * conversion to take place. If so, then the interrupt must be masked for now. Once this
     * conversion is complete, the other conversion will be restarted. */
    if(postponable_conversion_started)
    {
        tracemsg(_k_d("POWER IC: AtoD - stole converter from postponable conversion."));
        
        error = power_ic_event_mask(EVENT_ADCDONEI);
    
        if(error != 0)
        {
            printk(("POWER IC: AtoD - error %d masking AtoD interrupt."), error);
            goto quit_due_to_error;
        }
    }
    
    /* Set up the hardware then start the conversion */
    error = set_up_hardware_exclusive_conversion(request);
    
    if(error != 0)
    {
        printk(("POWER IC: AtoD - error %d setting up exclusive conversion."), error);
        goto quit_due_to_error;
    }
    
    /* All exclusive conversions are performed immediately. */
    request->timing = POWER_IC_ATOD_TIMING_IMMEDIATE;
    error = start_conversion(request);
    
    if(error != 0)
    {
        printk(("POWER IC: AtoD - error %d starting exclusive conversion."), error);
        goto quit_due_to_error;
    }

    /* Wait for the conversion to complete. */
    while(conversion_in_progress)
    {
        /* Figure out whether the conversion has completed. ASC clears once the conversion 
         * is complete. This read has been seen to take about 50us, so a further delay probably
         * isn't necessary. */
        power_ic_get_reg_value(START_CONVERT_REG_ASC, ASC_SHIFT, &conversion_in_progress, 1);
    }

    /* The conversion is complete - get the results. */
    error = get_results(samples);
    
    if(error != 0)
    {
        printk(("POWER IC: AtoD - error %d getting results of exclusive conversion."), error);
        goto quit_due_to_error;
    }
    
    /* Disable the converter. If it's needed to restart the postponable conversion, it'll
     * be turned back on at that point. */
    error = disable_converter();
            
    if(error != 0)
    {
        printk(("POWER IC: AtoD - error %d turning off converter."), error);
        goto quit_due_to_error;
    }
    
    /* Finally, if we stole the converter from a postponable conversion that was in progress,
     * restart that conversion. */
    if((postponable_conversion_started) && (!postponable_conversion_event_flag))
    {
        /* Set the converter up to the the postponable conversion's batt/current conversion. */
        error = set_batt_current_conversion(postponable_conversion_polarity);
        
        if(error != 0)
        {
            printk(("POWER IC: AtoD - error %d setting batt/current conversion."), error);
            goto quit_due_to_error;
        }
        
        /* Unmask the conversion complete interrupt so that it's ready for the hardware-
         * timed conversion*/
        error = power_ic_event_clear(EVENT_ADCDONEI);
        error |= power_ic_event_unmask(EVENT_ADCDONEI);
        
        if(error != 0)
        {
            printk(("POWER IC: AtoD - error %d unmasking AtoD interrupt."), error);
            goto quit_due_to_error;            
        }
        
        /* Restart the postponable conversion. */
        request->timing = postponable_conversion_timing;
        error = start_conversion(request);
    }
    
quit_due_to_error:        
    /* Since the conversion is complete, turn off the corresponding GPO */
    if(power_ic_atod_get_therm() != 0)
    {
        power_ic_atod_set_therm(BIT_DISABLE);
    }
    /* Set LICELLCON off to enable LiCell charge */
    if (power_ic_atod_get_lithium_coin_cell_en() != 0)
    {
        power_ic_atod_set_lithium_coin_cell_en(BIT_DISABLE);
    }

    /* Done - release the converter. */
    up(&hardware_mutex);

    return error;
}

/*!
 * @brief Starts a conversion that may be interrupted by any exclusive conversion.
 * 
 * This function starts a hardware-timed conversion that may be interrupted by any 
 * exclusive conversion while waiting for the hardware to trigger the conversion. This
 * is done for hardware-timed conversions as there is the potential for a long delay 
 * while the hardware waits for the next transmit burst.
 *
 * This function does not block the caller until the conversion completes. This 
 * is intended for use by the task that contains the battery charger so it does 
 * not block the entire task's operation.
 *
 * @param        request   Pointer to structure that describes the request.
 *
 * @return       non-zero if an error occurs..
 */
static int non_blocking_conversion(POWER_IC_ATOD_REQUEST_T * request)
{
    int error = 0;
    
    tracemsg(_k_d("AtoD - starting non-blocking conversion."));
    
    /* If a signal caused us to continue without getting the mutex, then quit with an error. */
    if(down_interruptible(&postponable_mutex) != 0)
    {
        printk("POWER IC:   process received signal while waiting for postponable mutex. Exiting.");
        return -EINTR;
    }
    
    /* If a signal caused us to continue without getting the mutex, then quit with an error. */
    if(down_interruptible(&hardware_mutex) != 0)
    {
        printk("POWER IC:   process received signal while waiting for hardware mutex. Exiting.");
        return -EINTR;
    }
    
    /* Remember that we started a new conversion. */
    postponable_conversion_started = true;
    postponable_conversion_event_flag = false;
        
    /* Set up hardware for battery/current conversion. */
    postponable_conversion_polarity = request->curr_polarity;
    error = set_batt_current_conversion(request->curr_polarity);
        
    if(error != 0)
    {
        printk(("POWER IC: AtoD - error %d setting batt/current conversion."), error);
        goto quit_due_to_error;
    }
        
    /* Unmask the conversion complete interrupt. */
    error = power_ic_event_clear(EVENT_ADCDONEI);
    error |= power_ic_event_unmask(EVENT_ADCDONEI);
    
    if(error != 0)
    {
        printk(("POWER IC: AtoD - error %d unmasking AtoD interrupt."), error);
        goto quit_due_to_error;
    }
        
    /* Set up timing for this conversion. */
    error = start_conversion(request);
    postponable_conversion_timing = request->timing;
        
    if(error != 0)
    {
        printk(("POWER IC: AtoD - error %d starting postponable conversion."), error);
        goto quit_due_to_error;
    }              

    /* Allow other conversion access to the hardware (possibly postponing this conversion)
     * then exit. It is up to the caller to check back later for a completed conversion. */
    up(&hardware_mutex);
    up(&postponable_mutex);
    tracemsg(_k_d("AtoD - non-blocking conversion ready."));

    return 0;

quit_due_to_error:
    /* Make sure that the converter is available after an error occurs. Mask the
     * interrupt and release the lockes. */
    power_ic_event_mask(EVENT_ADCDONEI);
    up(&hardware_mutex);
    up(&postponable_mutex);
    
    return error;
}

/*!
 * @brief Conversion complete event handler.
 *
 * This function is the handler for a conversion complete event from the power IC. It 
 * isn't called directly, but is registered with the driver core as a callback for when
 * a conversion complete event occurs.
 *
 * This handler is only used for hardware-timed (postponable) conversions - all other
 * conversions are done through polling to eliminate the overhead of context switching.
 *
 *
 * @param        unused     The event type (not used).
 *
 * @return 1 to indicate the event was handled.
 */
static int atod_complete_event_handler(POWER_IC_EVENT_T unused)
{
    tracemsg(_k_d("AtoD conversion complete event."));
   
    /* The conversion complete interrupt is only used for non-immediate conversions.
     * Since this interrupt has occurred, we want to wake the conversion that's waiting
     * on completion. */
    postponable_conversion_event_flag = true;
    wake_up(&postponable_complete_queue);
    
    /* Get the results of the conversion before they are possibly overwritten by an
     * exclusive conversion. */
    postponable_results_error = get_results(postponable_results);
    
    /* We're done with the converter. */
    disable_converter();
    
    /* This should pretty much handle all conversion complete events... */
    return 1;
}

/*!
 * @brief filters bad results from samples to be averaged.
 *
 * This function takes a set of samples (presumably all from the same channel) and
 * calculates an average of the samples, discarding those that look bad so they aren't
 * used in the average. This function only filters what look like abnormally low samples.
 *
 * This function is currently only intended for the mobport channel. Currently we're seeing
 * varying results in the set of samples in this channel - most of the samples are OK, but
 * randomly one or two of the samples will be at half the level of all the others in the set.
 * We don't know why this is happening just yet, so this function is to be used to hack 
 * around the problem for now.
 *
 * @param        samples   Points to a set of SAMPLES_PER_BANK samples.
 * @param        average   The suggested average for this set of samples.
 *
 * @return the number of samples discarded. For example, 0 means all the samples were good,
 * 1 shows that one of the samples was discarded and not used in the average, etc.
 */
static int HACK_sample_filter(int * samples, int * average)
{
    int highest_sample = 0;
    int bad_sample_threshold;
    int filtered_average = 0;
    int num_samples = 0;
    int i;

    /* First, find out the highest of the samples. */
    for(i=0; i<SAMPLES_PER_BANK; i++)
    {
        if(samples[i] > highest_sample)
        {
            highest_sample = samples[i];
        }
    }
    
    /* This will be our threshold for rejecting samples. */
    bad_sample_threshold = highest_sample - HACK_SAMPLE_FILTER_OFFSET;
    
    /* Now, walk through the set of samples, ignoring the samples that are too many counts
     * below the highest one in the set. */
    for(i=0; i<SAMPLES_PER_BANK; i++)
    {
        /* If sample is above the threshold, then use it for averaging. */
        if(samples[i] >= bad_sample_threshold)
        {
            filtered_average += samples[i];
            num_samples++;
        }
    }
    
    /* If any samples were good, then go ahead and average them. */
    if(num_samples > 0)
    {
        *average = filtered_average / num_samples;
    }
    
    /* Figure out how many samples were disacarded. */
    return(SAMPLES_PER_BANK - num_samples);
}

/*!
 * @brief Indicates if the atod channel is available
 *
 * This function indicates whether the channel passed is available on this power IC.
 *
 * @param        channel  The channel to look up
 *                        
 * @return returns the hardware channel.
 */
static ATOD_CHANNEL_T atod_channel_available(POWER_IC_ATOD_CHANNEL_T channel)
{
    if (channel >= POWER_IC_ATOD_NUM_CHANNELS)
    {
        printk(("POWER IC: Channel out of range: requested %d. \n"), channel);
        return ATOD_CHANNEL_NUM_VALUES;      
    }
    return atod_channel_lookup[channel];
}

/*!
 * @brief Gets the value of the thermistor pin
 *
 * This function will return if the GPO for the thermistor pin is turned on or off
 *
 * @return 1 if GPO is set 
 */
static int power_ic_atod_get_therm(void)
{
    int data = 0;
    
    power_ic_get_reg_value(CHRG_THERM_REG, CHRG_THERM_INDEX, &data, CHRG_THERM_BITS);
    return(data);
}

/*!
 * @brief This function will set the thermistor pin on/off
 *
 * This function will turn on or off the GPO that controls the thermistor for charging
 *
 * @param  on  Turns the GPO for thermistor on or off
 *
 * @return 0 if successful
 */
static int power_ic_atod_set_therm(int on)
{
     
    /* Any input above zero is on.. */
    if(on > 0)
    {
        on = 1;
    }
    else
    {
        on = 0;
    }
    
    return(power_ic_set_reg_bit(CHRG_THERM_REG, CHRG_THERM_INDEX, on));
}

/*!
 * @brief Gets the value of the licell enable pin
 *
 * This function will return if the LICELLON is enabled or disabled
 *
 * @return 1 if LICELLON is set
 */
static int power_ic_atod_get_lithium_coin_cell_en(void)
{
    int data = 0;
    
    power_ic_get_reg_value(POWER_IC_REG_ATLAS_ADC_0, LICELLCON_INDEX, &data, LICELLCON_BITS);
    return(data);
}

/*!
 * @brief Sets the lithium coin cell AtoD reading.
 *
 * This function sets the lithium coin cell enable bit so that RTC Batt AtoD Voltage can be
 * read.
 *
 * @param  value   The value to be set in the enable bit.
 *
 * @return 0 if successful.
 */
static int power_ic_atod_set_lithium_coin_cell_en(int value)
{
    int error;
    
    if(value > 0)
    {
        /* Enable the lithium coin cell reading */
        if((error = power_ic_set_reg_bit(
                                    POWER_IC_REG_ATLAS_ADC_0, 
                                    LICELLCON_INDEX, 
                                    BIT_ENABLE)) != 0)
        {
            printk("POWER IC: AtoD - Error in enabling the lithium coin cell AtoD reading.");
        }
    }
    else
    {
        /* Disable the lithium coin cell reading */ 
        if((error = power_ic_set_reg_bit(
                                    POWER_IC_REG_ATLAS_ADC_0, 
                                    LICELLCON_INDEX, 
                                    BIT_DISABLE)) != 0)
        {
            printk("POWER IC: AtoD - Error in disabling the lithium coin cell AtoD reading.");
        }
    }
    return (error);
}

/*!
 * @brief Converts the battery voltage and current.
 *
 * This function performs a conversion of the battery and current channels. The hardware 
 * ensures that the current measured is that at the same point in time that the voltage
 * is sampled, the idea being that a sag in battery voltage due to current drawn can be
 * accounted for.
 *
 * This function can only be used to start immediate conversions.
 *
 * @pre This function will cause the calling process to sleep while waiting for the 
 * conversion to be completed, and so cannot be used in an interrupt context.
 *
 * @param        timing       Indicates the timing needed for the conversion.
 * @param        timeout_secs This request will time out if no conversion takes place. Set to zero
 *                            if no timeout is required.
 * @param        polarity     The direction in which current should be measured.
 * @param        batt_result  Where the battery sample should be stored.
 * @param        curr_result  Where the current sample should be stored.
 *
 * @return POWER_IC_ATOD_CONVERSION_TIMEOUT if conversion timed out, 
 * POWER_IC_ATOD_CONVERSION_COMPLETE if conversion was successfully completed.
 */
static int power_ic_atod_current_and_batt_conversion(POWER_IC_ATOD_TIMING_T timing,
                                                int * batt_result, int * curr_result, int phasing)
{
    int average = 0;
    int error;
    int i;
    int samples[SAMPLES_PER_BANK];
    int current_batt_diag_data[2] = {0};
    
    POWER_IC_ATOD_REQUEST_T request = 
    {
        .type = POWER_IC_ATOD_TYPE_BATT_AND_CURR,
        .timing = POWER_IC_ATOD_TIMING_IMMEDIATE,
    };
    
    /* Don't allow the conversion to proceed if in interrupt context. */
    if(in_interrupt())
    {
        printk("POWER IC: batt/current AtoD conversion attempted in interrupt context. Exiting.");
        return -EPERM;
    }
    
    if(timing == POWER_IC_ATOD_TIMING_IMMEDIATE)
    {
        /* Perform the conversion immediately. This will sleep. */
        error = exclusive_conversion(&request, samples);
    }
    else
    {
         /* The non-blocking interface must be used for non-immediate conversions. */
        return -EINVAL;
    }
    
    /* Average the battery voltage. */
    for(i=0; i < (SAMPLES_PER_BANK - 1); i+=2)
    {
        average += samples[i];
    }

    if(phasing != PHASING_NOT_NEEDED)
    { 
         *batt_result = phase_channel(POWER_IC_ATOD_CHANNEL_BATT, average / (SAMPLES_PER_BANK/2));
    }
    else
    {
         *batt_result =  average / (SAMPLES_PER_BANK/2);   
    }
    current_batt_diag_data[0] = *batt_result;
        
    /* Average all of the current samples taken. */
    average = 0;
    for(i=1; i<SAMPLES_PER_BANK; i+=2)
    {
        if (samples[i] >= BATTI_ZERO)
        {
            /* Set upper bits to 1's */
            samples[i] = samples[i] | NEGATIVE_BATTI_MASK;
        }
        average += samples[i];
    }
    
    average = average / (SAMPLES_PER_BANK/2);
    
    if(phasing != PHASING_NOT_NEEDED)
    {
        *curr_result = phase_atod_10((signed char)phasing_values[ATOD_PHASING_BATT_CURR_OFFSET], 
                                 phasing_values[ATOD_PHASING_BATT_CURR_SLOPE], average);
    }
    else
    {
        *curr_result = average;
    }
    
    current_batt_diag_data[1] = *curr_result;

    return error;
}


/*******************************************************************************************
 * Global functions to be called only from driver
 ******************************************************************************************/

/*!
 * @brief ioctl() handler for the AtoD interface.
 *
 * This function is the ioctl() interface handler for all AtoD operations. It is not called
 * directly through an ioctl() call on the power IC device, but is executed from the core ioctl
 * handler for all ioctl requests in the range for AtoD operations.
 *
 * @param        cmd       ioctl() request received.
 * @param        arg       typically points to structure giving further details of conversion.
 *
 * @return 0 if successful.
 */
int atod_ioctl(unsigned int cmd, unsigned long arg)
{
    /* This is scratch space where the data passed by the caller will be stored temporarily.
     * Since there are several possible structures that can be passed with an AtoD ioctl()
     * request, it is wasteful to have one of each on the stack when only one will be used in
     * any given call. */
    int temp[10];
    int error;
    int i;
    int phasing = PHASING_NEEDED;
    
    /* Handle the request. */
    switch(cmd)
    {
        case POWER_IC_IOCTL_ATOD_SINGLE_CHANNEL:
        case POWER_IC_IOCTL_ATOD_CHRGR_CURR:
            /* Get the data accompanying the request. from user space. */
            if(copy_from_user((void *)temp, (void *)arg, 
                             sizeof(POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T)) != 0)
            {
                printk("POWER IC:   error copying single channel data from user space.");
                return -EFAULT;
            }
            
            /*Protect from sending the wrong channel for the POWER_IC_IOCTL_ATOD_CHRGR_CURR call from user space*/
            if (cmd == POWER_IC_IOCTL_ATOD_CHRGR_CURR)
            {
                ((POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *) temp)->channel = POWER_IC_ATOD_CHANNEL_CHRG_CURR;    
            }
            
            /* Convert the channel requested. */
            if((error = power_ic_atod_single_channel( 
                             ((POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *) temp)->channel, 
                            &((POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *) temp)->result)) != 0)
            {
                return error;
            }
            
             /* The follow if will be used to convert the curr_result on charger current channel from DACs to milliamps since this an easier 
             number to work with.*/
            if (cmd == POWER_IC_IOCTL_ATOD_CHRGR_CURR)
            { 
                ((POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *) temp)->result =
                    convert_to_milliamps((( POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *) temp)->result);
            }
                              
            /* Return the conversion result back to the user. */
            if(put_user( ((POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *) temp)->result,
                         &((POWER_IC_ATOD_REQUEST_SINGLE_CHANNEL_T *) arg)->result) != 0)
            {
                printk("POWER IC:   error copying conversion result to user space.");
                return -EFAULT;
            }
            
            break;
        
        case POWER_IC_IOCTL_ATOD_BATT_AND_CURR: 
        case POWER_IC_IOCTL_ATOD_BATT_AND_CURR_PHASED:
        case POWER_IC_IOCTL_ATOD_BATT_AND_CURR_UNPHASED:
        
            if(cmd ==  POWER_IC_IOCTL_ATOD_BATT_AND_CURR_UNPHASED)
            {
                phasing = PHASING_NOT_NEEDED;
            }
            
            /* Get the data accompanying the request.from user space. */
            if(copy_from_user((void *)temp, (void *)arg, 
                  sizeof(POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T)) != 0)
            {
                printk("POWER IC:   error copying batt/curr data from user space.");
                return -EFAULT;
            }
            
            /* Convert both the battery and current. */
            if((error = power_ic_atod_current_and_batt_conversion( 
                              ((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->timing,
                             &((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->batt_result,
                             &((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->curr_result,phasing)) != 0)
            {
                return error;
            }
            
            /* Since the POWER_IC_IOCTL_ATOD_BATT_AND_CURR will be used during normal operation.  The follow if will be used 
            * to convert the curr_result from DACs to milliamps since this an easier number to work with.    
            */
            if(cmd == POWER_IC_IOCTL_ATOD_BATT_AND_CURR)
            {
                ((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->curr_result =
                    convert_to_milliamps(((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->curr_result);
            }
                
            /* Return the conversion result back to the user. */
            if(copy_to_user((void *) arg, (void *) temp, 
                   sizeof(POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T)) != 0)
            {
                printk("POWER IC:   error copying conversion results to user space.");
                return -EFAULT;
            }    
            
            break;
            
        case POWER_IC_IOCTL_ATOD_GENERAL:
            /* No input data accompanies this request from user-space. Just perform the 
             * conversion. */
            if((error = power_ic_atod_general_conversion(
                                     (POWER_IC_ATOD_RESULT_GENERAL_CONVERSION_T *) temp)) != 0)
            {
                printk("POWER IC:   error copying general data from user space.");
                return error;
            }
                                     
            /* Return the conversion results back to the user. */
            if(copy_to_user((void *)arg, (void *) temp, 
                   sizeof(POWER_IC_ATOD_RESULT_GENERAL_CONVERSION_T)) != 0)
            {
                printk("POWER IC:   error copying conversion results to user space.");
                return -EFAULT;
            }        

            break;
            
        case POWER_IC_IOCTL_ATOD_RAW:
            /* Get the data accompanying the request. from user space. */
            if(copy_from_user((void *)temp, (void *)arg, 
                             sizeof(POWER_IC_ATOD_REQUEST_RAW_T)) != 0)
            {
                printk("POWER IC:   error copying raw request data from user space.");
                return -EFAULT;
            }
        
            tracemsg(_k_d("=> Raw conversion on channel %d"),
                             ((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->channel);
            
            /* Convert the channel requested. */
            if((error = power_ic_atod_raw_conversion(
                             ((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->channel, 
                             ((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->results,
                             &((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->num_results)) != 0)
            {
                return error;
            }
            
            
            if (((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->channel == POWER_IC_ATOD_CHANNEL_CHRG_CURR)
            {
                for(i=0; i<POWER_IC_ATOD_NUM_RAW_RESULTS; i++)
                {   
                    if (((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->results[i] >= BATTI_ZERO)
                    {
                        /* Set upper bits to 1's */
                        ((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->results[i] = ((POWER_IC_ATOD_REQUEST_RAW_T *) temp)->results[i] | NEGATIVE_BATTI_MASK;
                    }
                 }
            }
          
            /* Return the conversion results back to the user. */
            if(copy_to_user((void *) arg, (void *) temp, 
                   sizeof(POWER_IC_ATOD_REQUEST_RAW_T)) != 0)
            {
                printk("   error copying conversion results to user space.");
                return -EFAULT;
            }
            
            break;
            
        case POWER_IC_IOCTL_ATOD_BEGIN_CONVERSION:
            /* start a general conversion before any trigged conversion */
            if (last_conversion != POWER_IC_ATOD_TIMING_IMMEDIATE)
            {
                if((error = power_ic_atod_dummy_conversion()) != 0)
                {
                    tracemsg(_k_d("   error starting dummy conversion."));
                    return error;
                }
            }

            /* Get the data accompanying the request.from user space. */
            if(copy_from_user((void *)temp, (void *)arg, 
                  sizeof(POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T)) != 0)
            {
                printk("POWER IC:   error copying begin conversion data from user space");
                return -EFAULT;
            }
            
            tracemsg(_k_d("=> Non-blocking convert batt and current: timing %d, polarity %d"), 
                             ((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->timing, 
                             ((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->polarity);  
                             
                             
            /* Kick off the conversion. This will not wait for completion. */
            if((error = power_ic_atod_nonblock_begin_conversion( 
                              ((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->timing,
                              ((POWER_IC_ATOD_REQUEST_BATT_AND_CURR_T *) temp)->polarity)) != 0)
            {
                return error;
            }
            
            break;
            
        case  POWER_IC_IOCTL_ATOD_CANCEL_CONVERSION:
            tracemsg(_k_d("=> Cancelling non-blocking AtoD conversion."));
            
            /* This interface doesn't use any parameters. Just try to cancel the conversion. */
            return(power_ic_atod_nonblock_cancel_conversion());
            break;
            
        case POWER_IC_IOCTL_ATOD_SET_PHASING:
            tracemsg(_k_d("=> Setting AtoD phasing values."));
            
            /* Get the phasing values passed in from user space. */
            if(copy_from_user((void *)temp, (void *)arg, 
                  ATOD_PHASING_NUM_VALUES) != 0)
            {
                printk("POWER IC:   error copying phasing data from user space.");
                return -EFAULT;
            }
            
            return set_phasing_values((unsigned char *)temp);
            break;

        default: /* This shouldn't be able to happen, but just in case... */
            printk(("POWER IC: => 0x%X unsupported AtoD ioctl command"), (int) cmd);
            return -ENOTTY;
            break;
  
        }

    return 0;
}

/*!
 * @brief the poll() handler for the AtoD interface.
 *
 * This function will add the postponable AtoD wait queue to the poll table to allow
 * for polling for conversion complete to occur. If the conversion completes, the 
 * function returns and indicates that the results are ready.
 
 * @param        file        file pointer
 * @param        wait        poll table for this poll()
 *
 * @return 0 if no data to read, (POLLIN|POLLRDNORM) if results available
 */

unsigned int atod_poll(struct file *file, poll_table *wait)
{
    unsigned int retval = 0;


    /* Add our wait queue to the poll table */
    poll_wait (file, &postponable_complete_queue, wait);

    /* If there are changes, indicate that there is data available to read */
    if (postponable_conversion_event_flag)
    {
        /* If an error somehow occurred getting the results, report it. */
        if(postponable_results_error)
        {
            retval = (POLLERR | POLLRDNORM);
        }
        else /* Everything's OK - the data is ready to be read. */
        {
            retval = (POLLIN | POLLRDNORM);
        }
    }
    return retval;
}


/*!
 * @brief the read() handler for the AtoD interface.
 *
 * This function implements the read() system call for the AtoD interface. The 
 * function returns up to one conversion result per call as a a pair of signed
 * ints. The first int is the averaged battery measurement and the second is the
 * averaged current in milliamps.
 *
 * If no events are pending, the function will return immediately with a return
 * value of 0, indicating that no data was copied into the buffer.
 *
 * @param        file        file pointer
 * @param        buf         buffer pointer to copy the data to
 * @param        count       size of buffer
 * @param        ppos        read position (ignored)
 *
 * @return 0 if no data is available, 8 if results are copied, < 0 on error
 */

ssize_t atod_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    ssize_t retval = 0;
    int results[2];
    int i;
    int average = 0;
    
    /* If there isn't space in the buffer for the results, return a failure. Do not 
     * consider the process complete if this happens as the results are still available. */
    if(count < sizeof(results))
    {
        return -EFBIG;
    }
    
    /* Only get the results if they're available. */
    if(postponable_conversion_event_flag)
    {
        /* If a signal caused us to continue without getting the mutex, then quit with an error. */
        if(down_interruptible(&postponable_mutex) != 0)
        {
            tracemsg(_k_d("   process received signal while waiting for postponable mutex. Exiting."));
            return -EINTR;
        }

        if(!postponable_results_error)
        {

            /* Average the battery voltage. */
            for(i=0; i < (SAMPLES_PER_BANK - 1); i+=2)
            {
                average += postponable_results[i];
            }

            results[0] = phase_channel(POWER_IC_ATOD_CHANNEL_BATT, average / (SAMPLES_PER_BANK/2));
           
            /* Average all of the current samples taken. */
            average = 0;
            for(i=1; i<SAMPLES_PER_BANK; i+=2)
            {
                if (postponable_results[i] >= BATTI_ZERO)
                {
                    /* Set upper bits to 1's */
                    postponable_results[i] = postponable_results[i] | NEGATIVE_BATTI_MASK;
                }
                average += postponable_results[i];
            }

            results[1] = convert_to_milliamps(phase_atod_10((signed char)phasing_values[ATOD_PHASING_BATT_CURR_OFFSET], 
                                                            phasing_values[ATOD_PHASING_BATT_CURR_SLOPE], 
                                                            (average / (SAMPLES_PER_BANK/2))));
           
            /* Copy the results back to user-space. */
            if( !(copy_to_user((int *)buf, (void *)results, sizeof(results))) )
            {
                /* Data was copied successfully. */
                retval = sizeof(results);
            }
            else
            {
                return -EFAULT;
            }
        }
        else
        {
            /* There was an error getting the results after the conversion. Set up
             * to return with an error. */
            retval = postponable_results_error;
        }
        
        /* The results have been read, so the postponable conversions can be reset. */
        postponable_conversion_started = false;
        postponable_conversion_event_flag = false;
        up(&postponable_mutex);
        tracemsg(_k_d("AtoD - process %d released postponable conversion lock."), current->pid);        
    }

    return retval;
}

/*!
 * @brief Initialisation function for the AtoD interface.
 *
 * This function initialises the AtoD interface. It should be called only once from the 
 * driver init routine.
 *
 * @return 0 if successful.
 */
int __init atod_init(void)
{
    tracemsg(_k_d("Initialising AtoD interface."));
    
    /* Register our handler for AtoD complete event. */
    power_ic_event_subscribe(EVENT_ADCDONEI, &atod_complete_event_handler);
     
    return 0;
}


/*!
 * @brief Converts the battery voltage and current.
 *
 * This function begins a conversion of the battery and current channels, but does not 
 * sleep or otherwise wait for the conversion to complete. The caller is therefore not
 * blocked and is free to do other stuff while the conversion proceeds. It is then the
 * caller's responsibility to then poll for the conversion's completion.
 *
 *
 * As with the blocking version of the battery and current conversion, the hardware 
 * ensures that the current measured is that at the same point in time that the voltage
 * is sampled, the idea being that a sag in battery voltage due to current drawn can be
 * accounted for.
 *
 * If the timing specified is immediate, then an error will be returned. This interface
 * is only intended for taking conversions that are triggered by the hardware.
 *
 * @param        timing       Indicates the timing needed for the conversion.
 * @param        polarity     The direction in which current should be measured.
 *
 * @return non-zero if an error occurs.
 */
static int power_ic_atod_nonblock_begin_conversion(POWER_IC_ATOD_TIMING_T timing,
                                            POWER_IC_ATOD_CURR_POLARITY_T polarity)
{
    int error = 0;
    
    POWER_IC_ATOD_REQUEST_T request = 
    {
        .type = POWER_IC_ATOD_TYPE_BATT_AND_CURR,
        .timing = timing,
        .curr_polarity = polarity,
    };
    
    if(timing == POWER_IC_ATOD_TIMING_IMMEDIATE)
    {
        /* This interface isn't supposed to be used for immediate conversions. This timing
         * is invalid. */
        return -EINVAL; 
    }
    else
    {
        /* Start the conversion as requested. This will return without waiting for the
         * conversion to complete. */
        error = non_blocking_conversion(&request);
    }
    
    return error;
}

/*!
 * @brief Cancels an outstanding non-blocking conversion.
 *
 * This function cancels non-blocking conversion that was previously started with 
 * power_ic_atod_nonblock_begin_conversion() or through the equivalent ioctl() call.
 * This will disable the interrupt, clear the necessary flags and give up the semaphore
 * so that any other postponable conversions may take place.
 *
 * @pre This function will release the semaphore used to ensure that only one postponable
 * conversion is in progress at a given time. If this function is called while a blocking
 * postponable conversion is in progress, its semaphore will be released and things will
 * go wrong if another conversion is waiting for the semaphore.
 *
 * @return non-zero if an error occurs.
 */
static int power_ic_atod_nonblock_cancel_conversion(void)
{
    int error = 0;
    
    /* If no conversion has been started, then calling this is wrong. */
    if(!postponable_conversion_started)
    {
        printk("POWER IC:   Error: no non-blocking conversion in progress. Unable to cancel.");
        return -EINVAL;
    }

    /* If a signal caused us to continue without getting the mutex, then quit with an error. */
    if(down_interruptible(&postponable_mutex) != 0)
    {
        printk("POWER IC:   process received signal while waiting for postponable mutex. Exiting.");
        return -EINTR;
    }
  
    error = power_ic_event_mask(EVENT_ADCDONEI);
    
    if(error != 0)
    {
        tracemsg(_k_d("AtoD - error %d disabling AtoD interrupt."), error);
    }
    
    /* Clear the flags involved with a postponable conversion. */
    postponable_conversion_started = false;
    postponable_conversion_event_flag = false;
    
    /* Finally, give up the postponable lock. This will allow other postponable conversions
     * to proceed. */
    up(&postponable_mutex);
    tracemsg(_k_d("AtoD - process %d released postponable conversion lock."), current->pid);
    
    return error;
}

/*******************************************************************************************
 * External interface functions
 ******************************************************************************************/
/*!
 * @brief Converts a single AtoD channel.
 *
 * Performs a conversion of a single specified channel that cannot be interrupted 
 * by any other conversion, and returns the average for the conversion results.
 *
 * @pre This function will cause the calling process to sleep while waiting for the 
 * conversion to be completed. Therefore, it cannot be used from an interrupt context.
 *
 * @param        channel   The channel that should be converted..
 * @param        result    Points to where the average of the samples should be stored.
 *
 * @return 0 if successful.
 */
int power_ic_atod_single_channel(POWER_IC_ATOD_CHANNEL_T channel, int * result)
{
    int average;
    int error;
    int i;
    int samples[SAMPLES_PER_BANK];
    int filtered_samples;
    int filtered_average;
    
    POWER_IC_ATOD_REQUEST_T request = 
    {
        .type = POWER_IC_ATOD_TYPE_SINGLE_CHANNEL,
        .channel = channel,
    };


    /* Don't allow the conversion to proceed if in interrupt context. */
    if(in_interrupt())
    {
        printk("POWER IC: single-channel AtoD conversion attempted in interrupt context. Exiting.");
        return -EPERM;
    }
    
HACK_retry_conversion:

    /* Perform the conversion. This will sleep. */
    error = exclusive_conversion(&request, samples);
   
    if(error != 0)
    {
        *result = 0;
        return error;
    }
   
    average = 0;
    if ((channel == POWER_IC_ATOD_CHANNEL_BATT_CURR) ||
        (channel == POWER_IC_ATOD_CHANNEL_CHRG_CURR))
    {
        for(i=1; i<SAMPLES_PER_BANK; i+=2)
        {   
            if (samples[i] >= BATTI_ZERO)
            {
                /* Set upper bits to 1's */
                samples[i] = samples[i] | NEGATIVE_BATTI_MASK;
            }
            average += samples[i];
        }
        average = average / (SAMPLES_PER_BANK/2);
    }
    else
    {
        /* For a single channel conversion, the average will be returned. */
        for(i=0; i<SAMPLES_PER_BANK; i++)
        {
            average += samples[i];
        }
        average = average / SAMPLES_PER_BANK;
    }
    
    /* If this is the channel for mobportb, do filtering on the results just to make sure. */
    if(channel == POWER_IC_ATOD_CHANNEL_MOBPORTB)
    {
        filtered_samples = HACK_sample_filter(samples, &filtered_average);
        
        /* If any samples were filtered out, use the filtered average instead. */
        if(filtered_samples > 0)
        {
            tracemsg(_k_d("HACK: %d samples filtered - average 0x%X (would have been 0x%X)"),
                     filtered_samples, filtered_average, average);
                     
            /* If too many samples were dropped, then redo the conversion. Jumping around like
             * this sucks, but hopefully this can be removed once we get to the bottom of this. */
            if(filtered_samples > HACK_TOO_MANY_SAMPLES_FILTERED)
            {
                tracemsg(_k_d("HACK: %d samples filtered is too many. Retrying conversion."),
                         filtered_samples);
                goto HACK_retry_conversion;
            }
        
        *result = phase_channel(channel, filtered_average);
        }
        else
        {
            *result = phase_channel(channel, average);
        }
    }
    else
    {
        *result = phase_channel(channel, average);
    }
    
    return error;
}
 
/*!
 * @brief Converts a general set of AtoD channels.
 * 
 * This function performs a conversion of the general set of channels, including battery, 
 * temperature, etc. Each individual result is returned - the results are not averaged.
 * This conversion cannot be iinterrupted by any other conversion.
 *
 * @pre This function will cause the calling process to sleep while waiting for the 
 * conversion to be completed, and so cannot be used in an interrupt context.
 *
 * @param        result    Points to structure where conversion results should be stored.
 *
 * @return 0 if successful.
 */
int power_ic_atod_general_conversion(POWER_IC_ATOD_RESULT_GENERAL_CONVERSION_T * result)
{
    int error;
    int samples[ATOD_CHANNEL_NUM_VALUES+1];
    
    POWER_IC_ATOD_REQUEST_T request = 
    {
        .type = POWER_IC_ATOD_TYPE_GENERAL,
        .ad6_sel = result->ad6_sel,
    };

    /* Don't allow the conversion to proceed if in interrupt context. */
    if(in_interrupt())
    {
        printk("POWER IC: general AtoD conversion attempted in interrupt context. Exiting.");
        return -EPERM;
    }
    
    /* Perform the conversion. This will sleep. */
    error = exclusive_conversion(&request, samples);
    
    if(error != 0)
    {
        return error;
    }
    
    /* Copy the results individually so no assumptions made about channel ordering. */
    result->coin_cell   = phase_channel(POWER_IC_ATOD_CHANNEL_COIN_CELL,   
					samples[(atod_channel_available(POWER_IC_ATOD_CHANNEL_COIN_CELL))%8]);
    result->battery     = phase_channel(POWER_IC_ATOD_CHANNEL_BATT,        
					samples[(atod_channel_available(POWER_IC_ATOD_CHANNEL_BATT))%8]);
    result->bplus       = phase_channel(POWER_IC_ATOD_CHANNEL_BPLUS,       
					samples[(atod_channel_available(POWER_IC_ATOD_CHANNEL_BPLUS))%8]);
    result->mobportb    = phase_channel(POWER_IC_ATOD_CHANNEL_MOBPORTB,    
					samples[(atod_channel_available(POWER_IC_ATOD_CHANNEL_MOBPORTB))%8]);
    result->charger_id  = phase_channel(POWER_IC_ATOD_CHANNEL_CHARGER_ID,  
					samples[(atod_channel_available(POWER_IC_ATOD_CHANNEL_CHARGER_ID))%8]);
    result->temperature = phase_channel(POWER_IC_ATOD_CHANNEL_TEMPERATURE, 
					samples[(atod_channel_available(POWER_IC_ATOD_CHANNEL_TEMPERATURE))%8]);
    
    return 0;
}

/*!
 * @brief Dummy conversion on a general set of AtoD channels.
 * 
 * This function performs a dummy conversion of the general set of channels. It is to fix the 
 * Atlas issue where one shot doesn't function for consecutive hardware-trigged atod.
 *
 * @param        void
 *
 * @return 0 if successful.
 */
int power_ic_atod_dummy_conversion(void)
{
    int error;
    int samples[SAMPLES_PER_BANK];
    
    POWER_IC_ATOD_REQUEST_T request = 
    {
        .type = POWER_IC_ATOD_TYPE_GENERAL,
    };
   
    /* Don't allow the conversion to proceed if in interrupt context. */
    if(in_interrupt())
    {
        tracemsg("general AtoD conversion attempted in interrupt context. Exiting.");
        return -EPERM;
    }
    
    /* Perform the conversion. This will sleep. */
    error = exclusive_conversion(&request, samples);
   
    return error;
}

/*!
 * @brief Performs a conversion for testing the converter hardware.
 *
 * This function performs a set of conversions on a single channel similar to the 
 * single channel request, but instead returns all of the individual measurements 
 * unphased and in the same format as read from the hardware. This conversion 
 * will be performed immediately if the converter is not in use, and cannot be 
 * interrupted by any other conversion.
 *
 * @pre This function will cause the calling process to sleep while waiting for the 
 * conversion to be completed, and so cannot be used in an interrupt context.
 *
 * @param        channel   This channel will be converted.
 * @param        samples   POWER_IC_ATOD_NUM_RAW_RESULTS samples will be stored here.
 * @param        length    Number of samples being stored.
 *
 * @return 0 if successful.
 */
int power_ic_atod_raw_conversion(POWER_IC_ATOD_CHANNEL_T channel, int * samples, int * length)
{
    int error = 0;
    
    POWER_IC_ATOD_REQUEST_T request = 
    {
        .type = POWER_IC_ATOD_TYPE_RAW,
        .channel = channel,
    };

    *length = ATOD_NUM_RAW_RESULTS;
    
    /* Don't allow the conversion to proceed if in interrupt context. */
    if(in_interrupt())
    {
        printk("POWER IC: raw AtoD conversion attempted in interrupt context. Exiting.");
        return -EPERM;
    }
    
    /* Perform the conversion. This will sleep. The results will be copied straight to the
     * caller's array with no post-processing. */
    error = exclusive_conversion(&request, samples);
  
    return (error);
}
