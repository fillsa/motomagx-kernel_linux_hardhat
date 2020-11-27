/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

 /*
  * The code contained herein is licensed under the GNU Lesser General
  * Public License.  You may obtain a copy of the GNU Lesser General
  * Public License Version 2.1 or later at the following locations:
  *
  * http://www.opensource.org/licenses/lgpl-license.html
  * http://www.gnu.org/copyleft/lgpl.html
  */

#ifndef _PMIC_ADC_H
#define _PMIC_ADC_H

/*!
 * @defgroup PMIC_ADC Digitizer Driver
 * @ingroup PMIC_DRVRS
 */

/*!
 * @file pmic_adc.h
 * @brief This is the header of PMIC ADC driver.
 *
 * @ingroup PMIC_ADC
 */

#include <asm/ioctl.h>

/*!
 * @name IOCTL user space interface
 */

/*! @{ */
/*!
 * Initialize ADC.
 * Argument type: none.
 */
#define PMIC_ADC_INIT                   _IO('p', 0xb0)
/*!
 * De-initialize ADC.
 * Argument type: none.
 */
#define PMIC_ADC_DEINIT                 _IO('p', 0xb1)
/*!
 * Convert one channel.
 * Argument type: pointer to t_adc_convert_param.
 */
#define PMIC_ADC_CONVERT                _IOWR('p', 0xb2, int)
/*!
 * Convert one channel eight samples.
 * Argument type: pointer to t_adc_convert_param.
 */
#define PMIC_ADC_CONVERT_8X             _IOWR('p', 0xb3, int)
/*!
 * Convert multiple channels.
 * Argument type: pointer to t_adc_convert_param.
 */
#define PMIC_ADC_CONVERT_MULTICHANNEL   _IOWR('p', 0xb4, int)
/*!
 * Set touch screen operation mode.
 * Argument type: t_touch_mode.
 */
#define PMIC_ADC_SET_TOUCH_MODE         _IOW('p', 0xb5, int)
/*!
 * Get touch screen operation mode.
 * Argument type: pointer to t_touch_mode.
 */
#define PMIC_ADC_GET_TOUCH_MODE         _IOR('p', 0xb6, int)
/*!
 * Get touch screen sample.
 * Argument type: pointer to t_touch_sample.
 */
#define PMIC_ADC_GET_TOUCH_SAMPLE       _IOR('p', 0xb7, int)
/*!
 * Get battery current.
 * Argument type: pointer to unsigned short.
 */
#define PMIC_ADC_GET_BATTERY_CURRENT    _IOR('p', 0xb8, int)
/*!
 * Activate comparator.
 * Argument type: pointer to t_adc_comp_param.
 */
#define PMIC_ADC_ACTIVATE_COMPARATOR    _IOW('p', 0xb9, int)
/*!
 * De-active comparator.
 * Argument type: none.
 */
#define PMIC_ADC_DEACTIVE_COMPARATOR    _IOW('p', 0xba, int)
#ifdef CONFIG_MXC_PMIC_SC55112

/*! @} */

/*!
 * Return the touch screen value interface.
 */
#define GET_TOUCH_SCREEN_VALUE          _IOWR('D',3, int)
/*!
 * Install touch screen read interface.
 */
#define TOUCH_SCREEN_READ_INSTALL       _IOWR('D',4, int)
/*!
 * Remove touch screen read interface.
 */
#define TOUCH_SCREEN_READ_UNINSTALL     _IOWR('D',5, int)
#endif
/*!
 * Install touch screen read interface.
 */
#define PMIC_TOUCH_SCREEN_READ_INSTALL  _IO('p',0xbb)
/*!
 * Remove touch screen read interface.
 */
#define PMIC_TOUCH_SCREEN_READ_UNINSTALL _IO('p',0xbc)
/*! @} */
#ifdef CONFIG_MXC_PMIC_SC55112
/*!
 * @name List of all input channels for sc55112 ADC
 */
/*! @{ */

/*! Li-Ion cell battary voltage  */
#define LICELL                0x0001
/*! BAT sense  */
#define BATSENSE              0x0002
/*! Raw EXTB+ sense  */
#define RAWEXTBPLUSSENSE      0x0004
/*! MPB sense  */
#define MPBSENSE              0x0008
/*! AD4  */
#define AD4                   0x0010
/*! AD5 or charging current */
#define AD5_CHG_ISENSE        0x0020
/*! AD6 or battery current */
#define AD6_BATT_I_ADC        0x0040
/*! USB Id  */
#define USB_ID                0x0080
/*! AD7  */
#define AD7                   0x0100
/*! AD8  */
#define AD8                   0x0200
/*! AD9  */
#define AD9                   0x0400
/*! Touch Screen X1 */
#define TSX1                  0x0800
/*! Touch Screen X2 */
#define TSX2                  0x1000
/*! Touch screen Y1 */
#define TSY1                  0x2000
/*! Touch screen Y2 */
#define TSY2                  0x4000
/*! Ground */
#define GND                   0x8000
#endif
/*! @} */
/*!
 * @name Touch Screen minimum and maximum values
 */
#define TS_X_MIN                80	/*! < Minimum X */
#define TS_Y_MIN                80	/*! < Minimum Y */

#define TS_X_MAX                1000	/*! < Maximum X */
#define TS_Y_MAX                1000	/*! < Maximum Y */
/*! @} */
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This enumeration defines input channels for PMIC ADC
 */
typedef enum {
	/*!
	 * Battery Voltage
	 */
	BATTERY_VOLTAGE,
	/*!
	 * Battery Current
	 */
	BATTERY_CURRENT,
	/*!
	 * Application Supply
	 */
	APPLICATION_SUPPLAY,
	/*!
	 * Charger Voltage
	 */
	CHARGE_VOLTAGE,
	/*!
	 * Charger Current
	 */
	CHARGE_CURRENT,
	/*!
	 * General Purpose ADIN5
	 */
	GENERAL_PURPOSE_ADIN5,
	/*!
	 * General Purpose ADIN6
	 */
	GENERAL_PURPOSE_ADIN6,
	/*!
	 * General Purpose ADIN7
	 */
	GENERAL_PURPOSE_ADIN7,
	/*!
	 * Touch screen X or General Purpose ADIN8
	 */
	TS_X_POSITION1,
	/*!
	 * Touch screen X or General Purpose ADIN9
	 */
	TS_X_POSITION2,
	/*!
	 * Touch screen X or General Purpose ADIN10
	 */
	TS_X_POSITION3,
	/*!
	 * Touch screen Y or General Purpose ADIN11
	 */
	TS_Y_POSITION1,
	/*!
	 * Touch screen Y
	 */
	TS_Y_POSITION2,
	/*!
	 * Touch screen Y
	 */
	TS_Y_POSITION3,
	/*!
	 * Touch screen contact 1
	 */
	TS_CONTACT_RESISTANCE1,
	/*!
	 * Touch screen contact 2
	 */
	TS_CONTACT_RESISTANCE2,
	/*!
	 * Licell
	 */
	LICELL,
	/*!
	 * Thermistor
	 */
	THERMISTOR,
	/*!
	 * Die temperature
	 */
	DIE_TEMP,
	/*!
	 * UID
	 */
	USB_ID,
	/*!
	 * Raw EXTB+ sense
	 */
	RAWEXTBPLUSSENSE,
	/*!
	 * Touch screen Y or General Purpose ADIN4
	 */
	GENERAL_PURPOSE_ADIN4,
} t_channel;
#endif
/*!
 * This enumeration defines reason of ADC Comparator interrupt.
 */
typedef enum {
	/*!
	 * Greater than WHIGH
	 */
	GTWHIGH,
	/*!
	 * Less than WLOW
	 */
	LTWLOW,
} t_comp_exception;

/*!
 * ADC comparator callback function type
 */
typedef void (*t_comparator_cb) (t_comp_exception reason);

/*!
 * This enumeration defines the touch screen operation modes.
 */
typedef enum {
	/*!
	 * Touch Screen X position
	 */
	TS_X_POSITION = 0,
	/*!
	 * Touch Screen Y position
	 */
	TS_Y_POSITION = 1,
	/*!
	 * Pressure
	 */
	TS_PRESSURE = 2,
	/*!
	 * Plate X
	 */
	TS_PLATE_X = 3,
	/*!
	 * Plate Y
	 */
	TS_PLATE_Y = 4,
	/*!
	 * Standby
	 */
	TS_STANDBY = 5,
	/*!
	 * No touch screen, TSX1, TSX2, TSY1 and TSY2 are used as  general
	 * purpose A/D inputs.
	 */
	TS_NONE = 6,
} t_touch_mode;

/*!
 * This structure is used to report touch screen sample.
 */
typedef struct {
	/*!
	 * Touch Screen X position
	 */
	short x_position;
	/*!
	 * Touch Screen Y position
	 */
	short y_position;
	/*!
	 * Touch Screen contact value
	 */
	short pressure;
} t_touch_sample;

/*!
 * This enumeration defines ADC conversion modes.
 */
typedef enum {
	/*!
	 * Sample 8 channels, 1 sample per channel
	 */
	ADC_8CHAN_1X = 0,
	/*!
	 * Sample 1 channel 8 times
	 */
	ADC_1CHAN_8X,
} t_conversion_mode;

/*!
 * This structure is used with IOCTL code \a PMIC_ADC_CONVERT,
 * \a PMIC_ADC_CONVERT_8X and \a PMIC_ADC_CONVERT_MULTICHANNEL.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
typedef struct {
	/*!
	 * channel or channels to be sampled.
	 */
	t_channel channel;
	/*!
	 * holds up to 16 sampling results
	 */
	unsigned short result[16];
} t_adc_convert_param;

typedef struct {
	/*!
	 * wlow.
	 */
	unsigned char wlow;
	/*!
	 * whigh.
	 */
	unsigned char whigh;
	/*!
	 * channel to monitor
	 */
	t_channel channel;
	/*!
	 * callback function.
	 */
	t_comparator_cb callback;
} t_adc_comp_param;

#endif

#ifdef CONFIG_MXC_PMIC_SC55112
/*!
 * This structure is used with IOCTL code \a PMIC_ADC_CONVERT,
 * \a PMIC_ADC_CONVERT_8X and \a PMIC_ADC_CONVERT_MULTICHANNEL.
 */
typedef struct {
	/*!
	 * channel or channels to be sampled.
	 */
	unsigned short channel;
	/*!
	 * holds upto 16 sampling results
	 */
	unsigned short result[16];
} t_adc_convert_param;

/*!
 * This structure is used with IOCTL code \a PMIC_ADC_ACTIVE_COMPARATOR.
 */
typedef struct {
	/*!
	 * WLOW.
	 */
	unsigned char wlow;
	/*!
	 * WHIGH.
	 */
	unsigned char whigh;
	/*!
	 * Callback function.
	 */
	t_comparator_cb callback;
} t_adc_comp_param;
#endif
/*!
 * This structure is used with IOCTL code \a PMIC_ADC_ACTIVE_COMPARATOR.
 */
/*typedef struct {
	/!
	 * wlow.
	 */
//      unsigned char wlow;
	/*!
	 * whigh.
	 */
//      unsigned char whigh;
	/*!
	 * channel to monitor
	 */
//      t_channel channel;
	/*!
	 * callback function.
	 */
//      t_comparator_cb callback;
//} t_adc_comp_param;

/* EXPORTED FUNCTIONS */

#ifdef __KERNEL__
/*!
 * This function initializes all ADC registers with default values. This
 * function also registers the interrupt events.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_init(void);

/*!
 * This function disables the ADC, de-registers the interrupt events.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_deinit(void);

/*!
 * This function triggers a conversion and returns one sampling result of one
 * channel.
 *
 * @param        channel   The channel to be sampled
 * @param        result    The pointer to the conversion result. The memory
 *                         should be allocated by the caller of this function.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_adc_convert(unsigned short channel, unsigned short *result);
#else
PMIC_STATUS pmic_adc_convert(t_channel channel, unsigned short *result);
#endif
/*!
 * This function triggers a conversion and returns eight sampling results of
 * one channel.
 *
 * @param        channel   The channel to be sampled
 * @param        result    The pointer to array to store eight sampling results.
 *                         The memory should be allocated by the caller of this
 *                         function.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_adc_convert_8x(unsigned short channel, unsigned short *result);
#else
PMIC_STATUS pmic_adc_convert_8x(t_channel channel, unsigned short *result);
#endif
/*!
 * This function triggers a conversion and returns sampling results of each
 * specified channel.
 *
 * @param        channels  This input parameter is bitmap to specify channels
 *                         to be sampled.
 * @param        result    The pointer to array to store sampling result.
 *                         The order of the result in the array is from lowest
 *                         channel number to highest channel number of the
 *                         sampled channels.
 *                         The memory should be allocated by the caller of this
 *                         function.
 *			   Note that the behavior of this function might differ
 *			   from one platform to another regarding especially
 *			   channels order.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_adc_convert_multichnnel(unsigned short channels,
					 unsigned short *result);
#else
PMIC_STATUS pmic_adc_convert_multichnnel(t_channel channels,
					 unsigned short *result);
#endif
/*!
 * This function sets touch screen operation mode.
 *
 * @param        touch_mode   Touch screen operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_set_touch_mode(t_touch_mode touch_mode);

/*!
 * This function retrieves the current touch screen operation mode.
 *
 * @param        touch_mode   Pointer to the retrieved touch screen operation
 *                            mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_get_touch_mode(t_touch_mode * touch_mode);

/*!
 * This function retrieves the current touch screen operation mode.
 *
 * @param        touch_sample Pointer to touch sample.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_get_touch_sample(t_touch_sample * touch_sample);

/*!
 * This function starts a Battery Current mode conversion.
 *
 * @param        mode      Conversion mode.
 * @param        result    Battery Current measurement result.
 *                         if \a mode = ADC_8CHAN_1X, the result is \n
 *                             result[0] = (BATTP - BATT_I) \n
 *                         if \a mode = ADC_1CHAN_8X, the result is \n
 *                             result[0] = BATTP \n
 *                             result[1] = BATT_I \n
 *                             result[2] = BATTP \n
 *                             result[3] = BATT_I \n
 *                             result[4] = BATTP \n
 *                             result[5] = BATT_I \n
 *                             result[6] = BATTP \n
 *                             result[7] = BATT_I
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_get_battery_current(t_conversion_mode mode,
					 unsigned short *result);

/*!
 * This function actives the comparator.  When comparator is activated and ADC
 * is enabled, the 8th converted value will be digitally compared against the
 * window defined by WLOW and WHIGH registers.
 *
 * @param        low      Comparison window low threshold (WLOW).
 * @param        high     Comparison window high threshold (WHIGH).
 * @param        callback Callback function to be called when the converted
 *                        value is beyond the comparison window.  The callback
 *                        function will pass a parameter of type
 *                        \b t_comp_expection to indicate the reason of
 *                        comparator exception.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_adc_active_comparator(unsigned char low,
				       unsigned char high,
				       t_comparator_cb callback);
#else
PMIC_STATUS pmic_adc_active_comparator(unsigned char low,
				       unsigned char high,
				       t_channel channel,
				       t_comparator_cb callback);
#endif
/*!
 * This function de-actives the comparator.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_adc_deactive_comparator(void);

/*!
 * This function enables the touch screen read interface.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_adc_install_ts(void);

/*!
 * This function disables the touch screen read interface.
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_adc_remove_ts(void);
#endif				/* _KERNEL */
#endif				/* _PMIC_ADC_H */
