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

#ifndef __MC13783_EXTERNAL_H__
#define __MC13783_EXTERNAL_H__

#include <asm/ioctl.h>

/*!
 * @defgroup MC13783DRVRS mc13783 Drivers
 */

/*!
 * @defgroup MC13783 mc13783 Protocol Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_external.h
 * @brief This file contains header of external functions of mc13783 driver.
 *
 * @ingroup MC13783
 */

/*!
 * This is the IOCTL define for mc13783 core driver.
 */
#define         MC13783_READ_REG          _IOWR('P',0, void*)
#define         MC13783_WRITE_REG         _IOWR('P',1, void*)
#define         MC13783_SUBSCRIBE         _IOWR('P',2, type_event_notification)
#define         MC13783_UNSUBSCRIBE       _IOWR('P',3, type_event_notification)

/*!
 * This is the enumeration of register name of mc13783
 */

/*!
 * This type define all mc13783 register.
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

/*!
 * This is event list of mc13783 interrupt
 */

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
	EVENT_CHGDETI = 5,
	/*!
	 * Charger over-voltage detection
	 */
	EVENT_CHGOVI = 6,
	/*!
	 * Charger path reverse current
	 */
	EVENT_CHGREVI = 7,
	/*!
	 * Charger path short circuit
	 */
	EVENT_CHGSHORTI = 8,
	/*!
	 * BP regulator current or voltage regulation
	 */
	EVENT_CCCVI = 9,
	/*!
	 * Charge current below threshold
	 */
	EVENT_CHRGCURRI = 10,
	/*!
	 * BP turn on threshold detection
	 */
	EVENT_BPONI = 11,
	/*!
	 * End of life / low battery detect
	 */
	EVENT_LOBATLI = 12,
	/*!
	 * Low battery warning
	 */
	EVENT_LOBATHI = 13,
	/*!
	 * USB detect
	 */
	EVENT_USBI = 14,
	/*!
	 * USB ID Line detect
	 */
	EVENT_IDI = 15,
	/*!
	 * Single ended 1 detect
	 */
	EVENT_SE1I = 16,
	/*!
	 * Car-kit detect
	 */
	EVENT_CKDETI = 17,
	/*!
	 * 1 Hz time-tick
	 */
	EVENT_E1HZI = 18,
	/*!
	 * Time of day alarm
	 */
	EVENT_TODAI = 19,
	/*!
	 * ON1B event
	 */
	EVENT_ONOFD1I = 20,
	/*!
	 * ON2B event
	 */
	EVENT_ONOFD2I = 21,
	/*!
	 * ON3B event
	 */
	EVENT_ONOFD3I = 22,
	/*!
	 * System reset
	 */
	EVENT_SYSRSTI = 23,
	/*!
	 * RTC reset occurred
	 */
	EVENT_RTCRSTI = 24,
	/*!
	 * Power cut event
	 */
	EVENT_PCI = 25,
	/*!
	 * Warm start event
	 */
	EVENT_WARMI = 26,
	/*!
	 * Memory hold event
	 */
	EVENT_MEMHLDI = 27,
	/*!
	 * Power ready
	 */
	EVENT_PWRRDYI = 28,
	/*!
	 * Thermal warning lower threshold
	 */
	EVENT_THWARNLI = 29,
	/*!
	 * Thermal warning higher threshold
	 */
	EVENT_THWARNHI = 30,
	/*!
	 * Clock source change
	 */
	EVENT_CLKI = 31,
	/*!
	 * Semaphore
	 */
	EVENT_SEMAFI = 32,
	/*!
	 * Microphone bias 2 detect
	 */
	EVENT_MC2BI = 33,
	/*!
	 * Headset attach
	 */
	EVENT_HSDETI = 34,
	/*!
	 * Stereo headset detect
	 */
	EVENT_HSLI = 35,
	/*!
	 * Thermal shutdown ALSP
	 */
	EVENT_ALSPTHI = 36,
	/*!
	 * Short circuit on AHS outputs
	 */
	EVENT_AHSSHORTI = 37,
	/*!
	 * number of event
	 */
	EVENT_NB = 38,
} type_event;

/*!
 * This enumeration defines all priorities of mc13783 request.
 */
typedef enum {
	/*!
	 * mc13783 ADC management priority
	 */
	PRIO_ADC = 1,
	/*!
	 * mc13783 Power management priority
	 */
	PRIO_PWM = 2,
	/*!
	 * the most priority is battery management
	 */
	PRIO_BATTERY = 3,
	/*!
	 * mc13783 Audio priority
	 */
	PRIO_AUDIO = 4,
	/*!
	 * mc13783 Connectivity priority
	 */
	PRIO_CONN = 5,
	/*!
	 * mc13783 RTC priority
	 */
	PRIO_RTC = 6,
	/*!
	 * mc13783 Light priority
	 */
	PRIO_LIGHT = 7,
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
	 * call back function, called when event occur
	 */
	void (*callback) (void);
	/*!
	 * call back function with parameter, called when event occur
	 */
	void (*callback_p) (void *);
	/*!
	 * call back parameter
	 */
	void *param;
} type_event_notification;

/*!
 * This structure is used with IOCTL.
 * It defines register, register value and event number
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
	 * event number
	 */
	unsigned int event;
} register_info;

#ifndef bool
#define bool int
#endif

/*!
 * This structure is used to read all sense bits of mc13783.
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
} t_sense_bits;

/* EXPORTED FUNCTIONS */

/*!
 * This function is called by mc13783 clients to read a register on mc13783.
 *
 * @param        priority   priority of access
 * @param        reg   	    number of register
 * @param        reg_value   return value of register
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_read_reg(t_prio priority, int reg, unsigned int *reg_value);

/*!
 * This function is called by mc13783 clients to write a register on mc13783.
 *
 * @param        priority   priority of access
 * @param        reg   	    number of register
 * @param        reg_value   New value of register
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_write_reg(t_prio priority, int reg, unsigned int *reg_value);

/*!
 * This function is called by mc13783 clients to write a register with immediate value on mc13783.
 *
 * @param        priority   priority of access
 * @param        reg   	    number of register.
 * @param        reg_value   New value of register.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_write_reg_value(t_prio priority, int reg, unsigned int reg_value);

/*!
 * This function is called by mc13783 clients to set a bit in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    New value of the bit (true or false)
 *
 * @return       This function returns 0 if successful.
 */

int mc13783_set_reg_bit(t_prio priority, int reg, int index, int value);

/*!
 * This function is called by mc13783 clients to get a bit in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    return value of the bit (true or false)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_get_reg_bit(t_prio priority, int reg, int index, int *value);

/*!
 * This function is called by mc13783 clients to set a value in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    New value of the bit (true or false)
 * @param        nb_bits    width of the value (in number of bits)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_set_reg_value(t_prio priority, int reg, int index, int value,
			  int nb_bits);

/*!
 * This function is called by mc13783 clients to get a value in a register of mc13783.
 *
 * @param        priority   priority of access
 * @param        reg	    Register
 * @param        index	    Index in register of the bit
 * @param        value	    New value of bits
 * @param        nb_bits    width of the value (in number of bits)
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_get_reg_value(t_prio priority, int reg, int index, int *value,
			  int nb_bits);

/*!
 * This function is called by mc13783 clients to subscribe on an event.
 *
 * @param        event_sub   structure of event, it contains type of event and callback
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_event_subscribe(type_event_notification event_sub);

/*!
* This function is called by mc13783 clients to un-subscribe on an event.
*
* @param        event_unsub   structure of event, it contains type of event and callback
*
* @return       This function returns 0 if successful.
*/
int mc13783_event_unsubscribe(type_event_notification event_unsub);

/*!
* This function is called to initialize an event.
*
* @param        event  structure of event.
*
* @return       This function returns 0 if successful.
*/
int mc13783_event_init(type_event_notification * event);

/*!
* This function is called to read all sense bits of mc13783.
*
* @param        sense_bits  structure of all sense bits.
*
* @return       This function returns 0 if successful.
*/
int mc13783_get_sense(t_sense_bits * sense_bits);

/* This function is implemented in mc13783 core module
 * It tells us whether mc13783 card has been correctly detected or not
 */
int mc13783_core_loaded(void);

/*!
 * This function returns the mc13783 version. Version of IC is detected
 * at initialization time by reading a specific register in mc13783 IC.
 * See mc13783_init function for more information.
 *
 * @return       This function returns  version of mc13783 IC or -1 if mc13783 was
 *		 not detected properly.
 */
int mxc_mc13783_get_ic_version(void);

#endif				/* __MC13783_EXTERNAL_H__ */
