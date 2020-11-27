/*
 * Copyright 2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA
 *
 */

#ifndef __PMIC_EXTERNAL_H__
#define __PMIC_EXTERNAL_H__

#ifdef __KERNEL__
#include <linux/list.h>
#endif

/*!
 * @defgroup PMIC_DRVRS Drivers
 */

/*!
 * @defgroup Protocol Driver
 * @ingroup PMIC_DRVRS
 */

/*!
 * @file pmic_external.h
 * @brief This file contains interface of PMIC protocol driver.
 *
 * @ingroup PMIC_DRVRS
 */

#include <asm/ioctl.h>
#include <asm/arch/pmic_status.h>

/*!
 * This is the enumeration of versions of PMIC
 */
typedef enum {
	PMIC_mc13783 = 1,	/*!< mc13783 */
	PMIC_sc55112 = 2	/*!< sc55112 */
} t_pmic_version;
#ifdef CONFIG_MXC_PMIC_SC55112
/*!
 * @name IOCTL definitions for sc55112 core driver
 */
/*! @{ */
/*! Read a PMIC register */
#define PMIC_READ_REG          _IOWR('P', 0xa0, void*)
/*! Write a PMIC register */
#define PMIC_WRITE_REG         _IOWR('P', 0xa1, void*)
/*! Subscribe a PMIC interrupt event */
#define PMIC_SUBSCRIBE         _IOWR('P', 0xa2, type_event_notification)
/*! Unsubscribe a PMIC interrupt event */
#define PMIC_UNSUBSCRIBE       _IOWR('P', 0xa3, type_event_notification)
/*! @} */
#endif

/*!
 * This is PMIC registers valid bits
 */
#define PMIC_ALL_BITS          0xFFFFFF
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This is the enumeration of register names of MC13783
 */
typedef enum {
	/*!
	 * REG_INTERRUPT_STATUS_0
	 */
	REG_INTERRUPT_STATUS_0 = 0,
	/*!
	 * REG_INTERRUPT_MASK_0
	 */
	REG_INTERRUPT_MASK_0,
	/*!
	 * REG_INTERRUPT_SENSE_0
	 */
	REG_INTERRUPT_SENSE_0,
	/*!
	 * REG_INTERRUPT_STATUS_1
	 */
	REG_INTERRUPT_STATUS_1,
	/*!
	 * REG_INTERRUPT_MASK_1
	 */
	REG_INTERRUPT_MASK_1,
	/*!
	 * REG_INTERRUPT_SENSE_1
	 */
	REG_INTERRUPT_SENSE_1,
	/*!
	 * REG_POWER_UP_MODE_SENSE
	 */
	REG_POWER_UP_MODE_SENSE,
	/*!
	 * REG_REVISION
	 */
	REG_REVISION,
	/*!
	 * REG_SEMAPHORE
	 */
	REG_SEMAPHORE,
	/*!
	 * REG_ARBITRATION_PERIPHERAL_AUDIO
	 */
	REG_ARBITRATION_PERIPHERAL_AUDIO,
	/*!
	 * REG_ARBITRATION_SWITCHERS
	 */
	REG_ARBITRATION_SWITCHERS,
	/*!
	 * REG_ARBITRATION_REGULATORS_0
	 */
	REG_ARBITRATION_REGULATORS_0,
	/*!
	 * REG_ARBITRATION_REGULATORS_1
	 */
	REG_ARBITRATION_REGULATORS_1,
	/*!
	 * REG_POWER_CONTROL_0
	 */
	REG_POWER_CONTROL_0,
	/*!
	 * REG_POWER_CONTROL_1
	 */
	REG_POWER_CONTROL_1,
	/*!
	 * REG_POWER_CONTROL_2
	 */
	REG_POWER_CONTROL_2,
	/*!
	 * REG_REGEN_ASSIGNMENT
	 */
	REG_REGEN_ASSIGNMENT,
	/*!
	 * REG_CONTROL_SPARE
	 */
	REG_CONTROL_SPARE,
	/*!
	 * REG_MEMORY_A
	 */
	REG_MEMORY_A,
	/*!
	 * REG_MEMORY_B
	 */
	REG_MEMORY_B,
	/*!
	 * REG_RTC_TIME
	 */
	REG_RTC_TIME,
	/*!
	 * REG_RTC_ALARM
	 */
	REG_RTC_ALARM,
	/*!
	 * REG_RTC_DAY
	 */
	REG_RTC_DAY,
	/*!
	 * REG_RTC_DAY_ALARM
	 */
	REG_RTC_DAY_ALARM,
	/*!
	 * REG_SWITCHERS_0
	 */
	REG_SWITCHERS_0,
	/*!
	 * REG_SWITCHERS_1
	 */
	REG_SWITCHERS_1,
	/*!
	 * REG_SWITCHERS_2
	 */
	REG_SWITCHERS_2,
	/*!
	 * REG_SWITCHERS_3
	 */
	REG_SWITCHERS_3,
	/*!
	 * REG_SWITCHERS_4
	 */
	REG_SWITCHERS_4,
	/*!
	 * REG_SWITCHERS_5
	 */
	REG_SWITCHERS_5,
	/*!
	 * REG_REGULATOR_SETTING_0
	 */
	REG_REGULATOR_SETTING_0,
	/*!
	 * REG_REGULATOR_SETTING_1
	 */
	REG_REGULATOR_SETTING_1,
	/*!
	 * REG_REGULATOR_MODE_0
	 */
	REG_REGULATOR_MODE_0,
	/*!
	 * REG_REGULATOR_MODE_1
	 */
	REG_REGULATOR_MODE_1,
	/*!
	 * REG_POWER_MISCELLANEOUS
	 */
	REG_POWER_MISCELLANEOUS,
	/*!
	 * REG_POWER_SPARE
	 */
	REG_POWER_SPARE,
	/*!
	 * REG_AUDIO_RX_0
	 */
	REG_AUDIO_RX_0,
	/*!
	 * REG_AUDIO_RX_1
	 */
	REG_AUDIO_RX_1,
	/*!
	 * REG_AUDIO_TX
	 */
	REG_AUDIO_TX,
	/*!
	 * REG_AUDIO_SSI_NETWORK
	 */
	REG_AUDIO_SSI_NETWORK,
	/*!
	 * REG_AUDIO_CODEC
	 */
	REG_AUDIO_CODEC,
	/*!
	 * REG_AUDIO_STEREO_DAC
	 */
	REG_AUDIO_STEREO_DAC,
	/*!
	 * REG_AUDIO_SPARE
	 */
	REG_AUDIO_SPARE,
	/*!
	 * REG_ADC_0
	 */
	REG_ADC_0,
	/*!
	 * REG_ADC_1
	 */
	REG_ADC_1,
	/*!
	 * REG_ADC_2
	 */
	REG_ADC_2,
	/*!
	 * REG_ADC_3
	 */
	REG_ADC_3,
	/*!
	 * REG_ADC_4
	 */
	REG_ADC_4,
	/*!
	 * REG_CHARGER
	 */
	REG_CHARGER,
	/*!
	 * REG_USB
	 */
	REG_USB,
	/*!
	 * REG_CHARGE_USB_SPARE
	 */
	REG_CHARGE_USB_SPARE,
	/*!
	 * REG_LED_CONTROL_0
	 */
	REG_LED_CONTROL_0,
	/*!
	 * REG_LED_CONTROL_1
	 */
	REG_LED_CONTROL_1,
	/*!
	 * REG_LED_CONTROL_2
	 */
	REG_LED_CONTROL_2,
	/*!
	 * REG_LED_CONTROL_3
	 */
	REG_LED_CONTROL_3,
	/*!
	 * REG_LED_CONTROL_4
	 */
	REG_LED_CONTROL_4,
	/*!
	 * REG_LED_CONTROL_5
	 */
	REG_LED_CONTROL_5,
	/*!
	 * REG_SPARE
	 */
	REG_SPARE,
	/*!
	 * REG_TRIM_0
	 */
	REG_TRIM_0,
	/*!
	 * REG_TRIM_1
	 */
	REG_TRIM_1,
	/*!
	 * REG_TEST_0
	 */
	REG_TEST_0,
	/*!
	 * REG_TEST_1
	 */
	REG_TEST_1,
	/*!
	 * REG_TEST_2
	 */
	REG_TEST_2,
	/*!
	 * REG_TEST_3
	 */
	REG_TEST_3,
	/*!
	 * REG_NB
	 */
	REG_NB,
} mc13783_reg;
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
/*!
 * This is the enumeration of register names of sc55112
 */
typedef enum {
	/*!
	 * REG_ISR
	 */
	REG_ISR = 0,
	/*!
	 * REG_IMR
	 */
	REG_IMR,
	/*!
	 * REG_PSTAT
	 */
	REG_PSTAT,
	/*!
	 * REG_INT_SEL
	 */
	REG_INT_SEL,
	/*!
	 * REG_SWCTRL
	 */
	REG_SWCTRL,
	/*!
	 * REG_VREG
	 */
	REG_VREG,
	/*!
	 * REG_IRQ_TEST
	 */
	REG_IRQ_TEST,
	/*!
	 * REG_TMOD_TRIM_3
	 */
	REG_TMOD_TRIM_3,
	/*!
	 * REG_BATT_DAC
	 */
	REG_BATT_DAC,
	/*!
	 * REG_ADC1
	 */
	REG_ADC1,
	/*!
	 * REG_ADC2
	 */
	REG_ADC2,
	/*!
	 * REG_AUD_CODEC
	 */
	REG_AUD_CODEC,
	/*!
	 * REG_RX_AUD_AMPS
	 */
	REG_RX_AUD_AMPS,
	/*!
	 * REG_ST_DAC
	 */
	REG_ST_DAC,
	/*!
	 * REG_RTC_TOD
	 */
	REG_RTC_TOD,
	/*!
	 * REG_RTC_TODA
	 */
	REG_RTC_TODA,
	/*!
	 * REG_RTC_DAY
	 */
	REG_RTC_DAY,
	/*!
	 * REG_RTC_DAYA
	 */
	REG_RTC_DAYA,
	/*!
	 * REG_RTC_CAL
	 */
	REG_RTC_CAL,
	/*!
	 * REG_PWRCTRL
	 */
	REG_PWRCTRL,
	/*!
	 * REG_BUSCTRL
	 */
	REG_BUSCTRL,
	/*!
	 * REG_BACKLIGHT_1
	 */
	REG_BACKLIGHT_1,
	/*!
	 * REG_BACKLIGHT_2
	 */
	REG_BACKLIGHT_2,
	/*!
	 * REG_TC_CONTROL1
	 */
	REG_TC_CONTROL1,
	/*!
	 * REG_TC_CONTROL2
	 */
	REG_TC_CONTROL2,
	/*!
	 * REG_ARB_REG
	 */
	REG_ARB_REG,
	/*!
	 * REG_TX_AUD_AMPS
	 */
	REG_TX_AUD_AMPS,
	/*!
	 * REG_GP_REG
	 */
	REG_GP_REG,
	/*!
	 * REG_TEST
	 */
	REG_TEST,
	/*!
	 * REG_TMOD_CTRL_1
	 */
	REG_TMOD_CTRL_1,
	/*!
	 * REG_TMOD_CTRL_2
	 */
	REG_TMOD_CTRL_2,
	/*!
	 * REG_TMOD_CTRL_3
	 */
	REG_TMOD_CTRL_3,
	/*!
	 * REG_RX_STATIC1
	 */
	REG_RX_STATIC1,
	/*!
	 * REG_RX_STATIC2
	 */
	REG_RX_STATIC2,
	/*!
	 * REG_RX_STATIC3
	 */
	REG_RX_STATIC3,
	/*!
	 * REG_RX_STATIC4
	 */
	REG_RX_STATIC4,
	/*!
	 * REG_RX_DYNAMIC
	 */
	REG_RX_DYNAMIC,
	/*!
	 * REG_TX_DYNAMIC
	 */
	REG_TX_DYNAMIC,
	/*!
	 * REG_TX_DYNAMIC2
	 */
	REG_TX_DYNAMIC2,
	/*!
	 * REG_TX_STATIC1
	 */
	REG_TX_STATIC1,
	/*!
	 * REG_TX_STATIC2
	 */
	REG_TX_STATIC2,
	/*!
	 * REG_TX_STATIC3
	 */
	REG_TX_STATIC3,
	/*!
	 * REG_TX_STATIC4
	 */
	REG_TX_STATIC4,
	/*!
	 * REG_TX_STATIC5
	 */
	REG_TX_STATIC5,
	/*!
	 * REG_TX_STATIC6
	 */
	REG_TX_STATIC6,
	/*!
	 * REG_TX_STATIC7
	 */
	REG_TX_STATIC7,
	/*!
	 * REG_TX_STATIC8
	 */
	REG_TX_STATIC8,
	/*!
	 * REG_TX_STATIC9
	 */
	REG_TX_STATIC9,
	/*!
	 * REG_TX_STATIC10
	 */
	REG_TX_STATIC10,
	/*!
	 * REG_TX_STATIC11
	 */
	REG_TX_STATIC11,
	/*!
	 * REG_TX_STATIC12
	 */
	REG_TX_STATIC12,
	/*!
	 * REG_TX_STATIC13
	 */
	REG_TX_STATIC13,
	/*!
	 * REG_TX_STATIC14
	 */
	REG_TX_STATIC14,
	/*!
	 * REG_TX_STATIC15
	 */
	REG_TX_STATIC15,
	/*!
	 * REG_TX_STATIC16
	 */
	REG_TX_STATIC16,
	/*!
	 * REG_TX_STATIC17
	 */
	REG_TX_STATIC17,
	/*!
	 * REG_TX_STATIC18
	 */
	REG_TX_STATIC18,
	/*!
	 * REG_TONE_GEN
	 */
	REG_TONE_GEN,
	/*!
	 * REG_GPS_REG
	 */
	REG_GPS_REG,
	/*!
	 * REG_TMODE_TRIM_1
	 */
	REG_TMODE_TRIM_1,
	/*!
	 * REG_TMODE_TRIM_2
	 */
	REG_TMODE_TRIM_2,
	/*!
	 * REG_T_RD_ERROR
	 */
	REG_T_RD_ERROR,
	/*!
	 * REG_T_RD_NPCOUNT
	 */
	REG_T_RD_NPCOUNT,
	/*!
	 * REG_ADC2_ALL
	 */
	REG_ADC2_ALL,
	/*!
	 * REG_NB
	 */
	REG_NB
} sc55112_reg;
#endif

/*!
 * Generic PMIC register enumeration type.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
typedef mc13783_reg pmic_reg;
#else
typedef sc55112_reg pmic_reg;
#endif

#ifdef CONFIG_MXC_MC13783_PMIC
typedef enum {
	/*!
	 * ADC has finished requested conversions
	 */
	EVENT_ADCDONEI = 0,
	/*!
	 * ADCBIS has finished requested conversions
	 */
	EVENT_ADCBISDONEI = 1,
	/*!
	 * Touchscreen wakeup
	 */
	EVENT_TSI = 2,
	/*!
	 * ADC reading above high limit
	 */
	EVENT_WHIGHI = 3,
	/*!
	 * ADC reading below low limit
	 */
	EVENT_WLOWI = 4,
	/*!
	 * Charger attach and removal
	 */
	EVENT_CHGDETI = 6,
	/*!
	 * Charger over-voltage detection
	 */
	EVENT_CHGOVI = 7,
	/*!
	 * Charger path reverse current
	 */
	EVENT_CHGREVI = 8,
	/*!
	 * Charger path short circuit
	 */
	EVENT_CHGSHORTI = 9,
	/*!
	 * BP regulator current or voltage regulation
	 */
	EVENT_CCCVI = 10,
	/*!
	 * Charge current below threshold
	 */
	EVENT_CHRGCURRI = 11,
	/*!
	 * BP turn on threshold detection
	 */
	EVENT_BPONI = 12,
	/*!
	 * End of life / low battery detect
	 */
	EVENT_LOBATLI = 13,
	/*!
	 * Low battery warning
	 */
	EVENT_LOBATHI = 14,
	/*!
	 * USB detect
	 */
	EVENT_USBI = 16,
	/*!
	 * USB ID Line detect
	 */
	EVENT_IDI = 19,
	/*!
	 * Single ended 1 detect
	 */
	EVENT_SE1I = 21,
	/*!
	 * Car-kit detect
	 */
	EVENT_CKDETI = 22,
	/*!
	 * 1 Hz time-tick
	 */
	EVENT_E1HZI = 24,
	/*!
	 * Time of day alarm
	 */
	EVENT_TODAI = 25,
	/*!
	 * ON1B event
	 */
	EVENT_ONOFD1I = 27,
	/*!
	 * ON2B event
	 */
	EVENT_ONOFD2I = 28,
	/*!
	 * ON3B event
	 */
	EVENT_ONOFD3I = 29,
	/*!
	 * System reset
	 */
	EVENT_SYSRSTI = 30,
	/*!
	 * RTC reset occurred
	 */
	EVENT_RTCRSTI = 31,
	/*!
	 * Power cut event
	 */
	EVENT_PCI = 32,
	/*!
	 * Warm start event
	 */
	EVENT_WARMI = 33,
	/*!
	 * Memory hold event
	 */
	EVENT_MEMHLDI = 34,
	/*!
	 * Power ready
	 */
	EVENT_PWRRDYI = 35,
	/*!
	 * Thermal warning lower threshold
	 */
	EVENT_THWARNLI = 36,
	/*!
	 * Thermal warning higher threshold
	 */
	EVENT_THWARNHI = 37,
	/*!
	 * Clock source change
	 */
	EVENT_CLKI = 38,
	/*!
	 * Semaphore
	 */
	EVENT_SEMAFI = 39,
	/*!
	 * Microphone bias 2 detect
	 */
	EVENT_MC2BI = 41,
	/*!
	 * Headset attach
	 */
	EVENT_HSDETI = 42,
	/*!
	 * Stereo headset detect
	 */
	EVENT_HSLI = 43,
	/*!
	 * Thermal shutdown ALSP
	 */
	EVENT_ALSPTHI = 44,
	/*!
	 * Short circuit on AHS outputs
	 */
	EVENT_AHSSHORTI = 45,
	/*!
	 * number of event
	 */
	EVENT_NB = 38,
} type_event;
#endif

#ifdef CONFIG_MXC_PMIC_SC55112
/*!
 * This is event list of sc55112 interrupt
 */

typedef enum {
	/*!
	 * completion of the 7 programmed A/D conversions in standard
	 * operation
	 */
	EVENT_ADCDONEI = 0,
	/*!
	 * touchscreen press
	 */
	EVENT_TSI = 1,
	/*!
	 * interrupt is from the 1Hz output.
	 */
	EVENT_1HZI = 2,
	/*!
	 * A/D word read in ADC digital comparison mode exceeding the
	 * WHIGH[5:0] word.
	 */
	EVENT_WHI = 3,
	/*!
	 * A/D word read in ADC digital comparison mode reading below
	 * the WLOW[5:0] word.
	 */
	EVENT_WLI = 4,
	/*!
	 * RTC_TOD = RTC_TODA; RTC_DAY = RTC_DAYA
	 */
	EVENT_TODAI = 5,
	/*!
	 * occurs on rising and falling debounced edges of USBDET_4.4V.
	 */
	EVENT_USB_44VI = 6,
	/*!
	 * ON/OFF button was pressed.
	 */
	EVENT_ONOFFI = 7,
	/*!
	 * ON/OFF2 button was pressed.
	 */
	EVENT_ONOFF2I = 8,
	/*!
	 * interrupt occurs on rising and falling edges of USBDET_0.8V
	 */
	EVENT_USB_08VI = 9,
	/*!
	 * interrupt occurs on rising and falling debounced edges of
	 * MOBPORTB (EXT B+).
	 */
	EVENT_MOBPORTI = 10,
	/*!
	 * Interrupt linked to PTT_DET , to be debounced on both edges.
	 */
	EVENT_PTTI = 11,
	/*!
	 * Triggered on debounced transition of A1_INT
	 */
	EVENT_A1I = 12,
	/*!
	 * Power Cut transition occurred when PCEN=1 and B+ was
	 * re-applied before the Power Cut timer expired
	 */
	EVENT_PCI = 14,
	/*!
	 * warm start to the MCU
	 */
	EVENT_WARMI = 15,
	/*!
	 * End of Life (low battery shut off)
	 */
	EVENT_EOLI = 16,
	/*!
	 * positive or negative edge of CLK_STAT
	 */
	EVENT_CLKI = 17,
	/*!
	 * interrupt occurs on rising and falling debounced edges of
	 * USBDET_2.0V.
	 */
	EVENT_USB_20VI = 18,
	/*!
	 * Interrupt generated from the de-bounced output of the USB
	 * ID-detect comparator.
	 */
	EVENT_AB_DETI = 19,
	/*!
	 * completion of the 7 programmed A/D conversions in standard
	 * operation.
	 */
	EVENT_ADCDONE2I = 20,
	/*!
	 * Will be set (1) only if SYS_RST_MODE bits = 10 and (2) the
	 * BATT_DET_IN/SYS_RESTART input was asserted for the minimum
	 * debounce time.
	 */
	EVENT_SOFT_RESETI = 21,
	/*!
	 * Number of events.
	 */
	EVENT_NB,
} type_event;
#endif

#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This enumeration defines all priorities of mc13783 request.
 */
typedef enum {
	/*!
	 * PMIC internal access priority
	 */
	PRIO_CORE = 0,
	/*!
	 * PMIC ADC management priority
	 */
	PRIO_ADC,
	/*!
	 * PMIC Power management priority
	 */
	PRIO_PWM,
	/*!
	 * PMIC Battery management priority
	 */
	PRIO_BATTERY,
	/*!
	 * PMIC Audio priority
	 */
	PRIO_AUDIO,
	/*!
	 * PMIC Connectivity priority
	 */
	PRIO_CONN,
	/*!
	 * PMIC RTC priority
	 */
	PRIO_RTC,
	/*!
	 * PMIC Light priority
	 */
	PRIO_LIGHT,
	/*!
	 * PMIC GPS priority
	 */
	PRIO_GPS,
} t_prio;

/*!
 * This structure is used to define an event.
 */
typedef struct {
	/*!
	 * event type
	 */
	type_event event;

	/*!
	 * Keeps a list of subscribed clients
	 * to an event. Internal use.
	 */
	void *next;

	/*!
	 * call back function with parameter, called when event occur
	 */
	void (*callback) (void *);
	/*!
	 * call back parameter
	 */
	void *param;
} type_event_notification;

/*!
 * This enumeration all senses of MC13783.
 */
typedef enum {
	/*!
	 * Charger attach sense
	 */
	SENSE_CHGDETS,
	/*!
	 * Charger over-voltage sense
	 */
	SENSE_CHGOVS,
	/*!
	 * Charger reverse current
	 * If 1 current flows into phone
	 */
	SENSE_CHGREVS,
	/*!
	 * Charger short circuit
	 */
	SENSE_CHGSHORTS,
	/*!
	 * Charger regulator operating mode
	 */
	SENSE_CCCVS,
	/*!
	 * Charger current below threshold
	 */
	SENSE_CHGCURRS,
	/*!
	 * BP turn on
	 */
	SENSE_BPONS,
	/*!
	 * Low bat detect
	 */
	SENSE_LOBATLS,
	/*!
	 * Low bat warning
	 */
	SENSE_LOBATHS,
	/*!
	 * USB 4V4
	 */
	SENSE_USB4V4S,
	/*!
	 * USB 2V0
	 */
	SENSE_USB2V0S,
	/*!
	 * USB 0V8
	 */
	SENSE_USB0V8S,
	/*!
	 * ID Floats
	 */
	SENSE_ID_FLOATS,
	/*!
	 * ID Gnds
	 */
	SENSE_ID_GNDS,
	/*!
	 * Single ended
	 */
	SENSE_SE1S,
	/*!
	 * Car-kit detect
	 */
	SENSE_CKDETS,
	/*!
	 * mic bias detect
	 */
	SENSE_MC2BS,
	/*!
	 * headset attached
	 */
	SENSE_HSDETS,
	/*!
	 * ST headset attached
	 */
	SENSE_HSLS,
	/*!
	 * Thermal shutdown ALSP
	 */
	SENSE_ALSPTHS,
	/*!
	 * short circuit on AHS
	 */
	SENSE_AHSSHORTS,
	/*!
	 * ON1B pin is hight
	 */
	SENSE_ONOFD1S,
	/*!
	 * ON2B pin is hight
	 */
	SENSE_ONOFD2S,
	/*!
	 * ON3B pin is hight
	 */
	SENSE_ONOFD3S,
	/*!
	 * System reset power ready
	 */
	SENSE_PWRRDYS,
	/*!
	 * Thermal warning higher threshold
	 */
	SENSE_THWARNHS,
	/*!
	 * Thermal warning lower threshold
	 */
	SENSE_THWARNLS,
	/*!
	 * Clock source is XTAL
	 */
	SENSE_CLKS,
} t_sensor;

/*!
 * This structure is used to read all sense bits of MC13783.
 */
typedef struct {
	/*!
	 * Charger attach sense
	 */
	bool sense_chgdets;
	/*!
	 * Charger over-voltage sense
	 */
	bool sense_chgovs;
	/*!
	 * Charger reverse current
	 * If 1 current flows into phone
	 */
	bool sense_chgrevs;
	/*!
	 * Charger short circuit
	 */
	bool sense_chgshorts;
	/*!
	 * Charger regulator operating mode
	 */
	bool sense_cccvs;
	/*!
	 * Charger current below threshold
	 */
	bool sense_chgcurrs;
	/*!
	 * BP turn on
	 */
	bool sense_bpons;
	/*!
	 * Low bat detect
	 */
	bool sense_lobatls;
	/*!
	 * Low bat warning
	 */
	bool sense_lobaths;
	/*!
	 * USB 4V4
	 */
	bool sense_usb4v4s;
	/*!
	 * USB 2V0
	 */
	bool sense_usb2v0s;
	/*!
	 * USB 0V8
	 */
	bool sense_usb0v8s;
	/*!
	 * ID Floats
	 */
	bool sense_id_floats;
	/*!
	 * ID Gnds
	 */
	bool sense_id_gnds;
	/*!
	 * Single ended
	 */
	bool sense_se1s;
	/*!
	 * Car-kit detect
	 */
	bool sense_ckdets;
	/*!
	 * mic bias detect
	 */
	bool sense_mc2bs;
	/*!
	 * headset attached
	 */
	bool sense_hsdets;
	/*!
	 * ST headset attached
	 */
	bool sense_hsls;
	/*!
	 * Thermal shutdown ALSP
	 */
	bool sense_alspths;
	/*!
	 * short circuit on AHS
	 */
	bool sense_ahsshorts;
	/*!
	 * ON1B pin is hight
	 */
	bool sense_onofd1s;
	/*!
	 * ON2B pin is hight
	 */
	bool sense_onofd2s;
	/*!
	 * ON3B pin is hight
	 */
	bool sense_onofd3s;
	/*!
	 * System reset power ready
	 */
	bool sense_pwrrdys;
	/*!
	 * Thermal warning higher threshold
	 */
	bool sense_thwarnhs;
	/*!
	 * Thermal warning lower threshold
	 */
	bool sense_thwarnls;
	/*!
	 * Clock source is XTAL
	 */
	bool sense_clks;
} t_sensor_bits;
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
/*!
 * This enumeration defines all priorities of mc13783 request.
 */
typedef enum {
	/*!
	 * PMIC internal access priority
	 */
	PRIO_CORE = 0,
	/*!
	 * PMIC ADC management priority
	 */
	PRIO_ADC,
	/*!
	 * PMIC Power management priority
	 */
	PRIO_PWM,
	/*!
	 * PMIC Battery management priority
	 */
	PRIO_BATTERY,
	/*!
	 * PMIC Audio priority
	 */
	PRIO_AUDIO,
	/*!
	 * PMIC Connectivity priority
	 */
	PRIO_CONN,
	/*!
	 * PMIC Light priority
	 */
	PRIO_LIGHT,
	/*!
	 * PMIC GPS priority
	 */
	PRIO_GPS,
} t_prio;

/*!
 * This structure is used to define an event.
 */
typedef struct {
	/*!
	 * event type
	 */
	type_event event;
	/*!
	 * call back function with parameter, called when event occur
	 */
	void (*callback) (void *);
	/*!
	 * call back parameter
	 */
	void *param;
} type_event_notification;

/*!
 * This structure is used with IOCTL.
 * It defines register, register value, register mask and event number
 */
typedef struct {
	/*!
	 * register number
	 */
	int reg;
	/*!
	 * value of register
	 */
	unsigned int reg_value;
	/*!
	 * mask of bits, only used with PMIC_WRITE_REG
	 */
	unsigned int reg_mask;
	/*!
	 * event number
	 */
	unsigned int event;
} register_info;

/*!
 * Define bool type if it is not defined.
 */
#ifndef bool
#define bool int
#endif

/*!
 * This enumeration all senses of sc55112.
 */
typedef enum {
	/*!
	 * Status of USB 4.4V Comparator
	 */
	SENSOR_USBDET_44V = 0,
	/*!
	 * The logic output of ONOFFSNS
	 */
	SENSOR_ONOFFSNS,
	/*!
	 * The logic output of ONOFFSNS2
	 */
	SENSOR_ONOFFSNS2,
	/*!
	 * Status of USB 0.8V Comparator
	 */
	SENSOR_USBDET_08V,
	/*!
	 * The power up state of the radio
	 */
	SENSOR_MOBSNSB,
	/*!
	 * Status of PTT_DET pin
	 */
	SENSOR_PTTSNS,
	/*!
	 * Status of A1_INT pin
	 */
	SENSOR_A1SNS,
	/*!
	 * Status of USB 2.0V Comparator
	 */
	SENSOR_USBDET_20V,
	/*!
	 * Current state of EOL comparator output
	 */
	SENSOR_EOL_STAT,
	/*!
	 * 32 kHz external oscillation
	 */
	SENSOR_CLK_STAT,
	/*!
	 * System reset
	 */
	SENSOR_SYS_RST,
	/*!
	 * Warm system reset
	 */
	SENSOR_WARM_SYS_RST,
	/*!
	 * State of the BATT_DET_IN/SYS_RST input
	 */
	SENSOR_BATT_DET_IN_SNS,
} t_sensor;

/*!
 * This structure is used to read all sense bits of sc55112.
 */
typedef struct {
	/*!
	 * Status of USB 4.4V Comparator
	 */
	bool usbdet_44v;
	/*!
	 * The logic output of ONOFFSNS
	 */
	bool onoffsns;
	/*!
	 * The logic output of ONOFFSNS2
	 */
	bool onoffsns2;
	/*!
	 * Status of USB 0.8V Comparator
	 */
	bool usbdet_08v;
	/*!
	 * The power up state of the radio
	 */
	bool mobsnsb;
	/*!
	 * Status of PTT_DET pin
	 */
	bool pttsns;
	/*!
	 * Status of A1_INT pin
	 */
	bool a1sns;
	/*!
	 * Status of USB 2.0V Comparator
	 */
	bool usbdet_20v;
	/*!
	 * Current state of EOL comparator output
	 */
	bool eol_stat;
	/*!
	 * 32 kHz external oscillation
	 */
	bool clk_stat;
	/*!
	 * System reset
	 */
	bool sys_rst;
	/*!
	 * Warm system reset
	 */
	bool warm_sys_rst;
	/*!
	 * State of the BATT_DET_IN/SYS_RST input
	 */
	bool batt_det_in_sns;
} t_sensor_bits;
#endif

/* EXPORTED FUNCTIONS */

/*!
 * This function returns the PMIC Revision in system.
 * This API needs to be called to know the Revision at Run time
 * @return       This function returns PMIC Revision.
 */
int mxc_pmic_get_ic_version(void);
/*!
 * This function returns the PMIC version in system.
 *
 * @return       This function returns PMIC version.
 */
t_pmic_version pmic_get_version(void);

/*!
 * This function is called by PMIC clients to read a register on PMIC.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value   return value of register
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_read_reg(t_prio priority, int reg, unsigned int *reg_value,
			  unsigned int reg_mask);
#else
PMIC_STATUS pmic_read_reg(t_prio priority, int reg, unsigned int *reg_value);
#endif
/*!
 * This function is called by PMIC clients to write a register on MC13783.
 *
 * @param        priority   priority of access
 * @param        reg        number of register
 * @param        reg_value  New value of register
 * @param        reg_mask   Bitmap mask indicating which bits to modify
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_write_reg(t_prio priority, int reg, unsigned int reg_value,
			   unsigned int reg_mask);

/*!
 * This function is called by PMIC clients to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_event_subscribe(type_event_notification * event_sub);
#else
PMIC_STATUS pmic_event_subscribe(type_event_notification event_sub);
#endif
/*!
* This function is called by PMIC clients to un-subscribe on an event.
*
* @param        event_unsub   structure of event, it contains type of event and callback
*
* @return       This function returns PMIC_SUCCESS if successful.
*/
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_event_unsubscribe(type_event_notification * event_unsub);
#else
PMIC_STATUS pmic_event_unsubscribe(type_event_notification event_unsub);
#endif
/*!
* This function is called to initialize an event.
*
* @param        event  structure of event.
*
* @return       This function returns PMIC_SUCCESS if successful.
*/
PMIC_STATUS pmic_event_init(type_event_notification * event);

/*!
* This function is called to read all sensor bits of PMIC.
*
* @param        sensor    Sensor to be checked.
*
* @return       This function returns true if the sensor bit is high;
*               or returns false if the sensor bit is low.
*/
bool pmic_check_sensor(t_sensor sensor);

/*!
* This function checks one sensor of PMIC.
*
* @param        sensor_bits  structure of all sensor bits.
*
* @return       This function returns PMIC_SUCCESS if successful.
*/
PMIC_STATUS pmic_get_sensors(t_sensor_bits * sensor_bits);

#endif				/* __PMIC_EXTERNAL_H__ */
