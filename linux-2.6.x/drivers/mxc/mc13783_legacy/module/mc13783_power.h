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

/*!
 * @defgroup MC13783_POWER mc13783 Power Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_power.h
 * @brief This is the header of mc13783 Power driver.
 *
 * @ingroup MC13783_POWER
 */

#ifndef         __MC13783_POWER_H__
#define         __MC13783_POWER_H__

/*
 * Includes
 */
#include <asm/ioctl.h>

#define         MC13783_POWER_INIT                       _IOWR('P',1, int)
#define         MC13783_POWER_FULL_CHECK                 _IOWR('P',2, int)
#define         MC13783_POWER_CONTROL_CHECK              _IOWR('P',3, int)
#define         MC13783_SWITCHER_CHECK                   _IOWR('P',4, int)
#define         MC13783_REGULATOR_CHECK                  _IOWR('P',5, int)

#ifdef __ALL_TRACES__
#define TRACEMSG_POWER(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_POWER(fmt,args...)
#endif				/* __ALL_TRACES__ */

/*!
 * This struct list all state reads in Power Up Sense register
 */
struct t_p_up_sense {
	/*!
	 * Power up sense ictest.
	 * By pulling ICTEST pin high, during startup or when the part is
	 * already active, the test mode is enabled.
	 */
	bool state_ictest;
	/*!
	 * Power up sense clksel.
	 * If the CLKSEL pin is connected to VMC13783, the output of the
	 * internal RC oscillator will not be routed to the clock output pins
	 * under any circumstances. If the CLKSEL pin is grounded, however, the
	 * internal RC oscillator output will be routed to clock output pins
	 * when the external crystal 32kHz oscillator is not running.
	 */
	bool state_clksel;
	/*!
	 * Power up mode supply 1.
	 * The PUMS1 and PUMS2 determine the initial setup for the voltage
	 * level of the switchers and regulators and if they get enabled or not.
	 * There are up to 9 possible setting, see mc13783 documentation
	 * for more details on the assignment.
	 */
	bool state_pums1;
	/*!
	 * Power up mode supply 2.
	 */
	bool state_pums2;
	/*!
	 * Power up mode supply 3.
	 * Three different power up sequences are selectable via the PUMS3
	 * setting.
	 */
	bool state_pums3;
	/*!
	 * Power up sense charge mode 0.
	 * chrgmod0 and chrgmode1 pins allow to select the mode of operation for
	 * the charger interface.
	 */
	bool state_chrgmode0;
	/*!
	 * Power up sense charge mode 1.
	 */
	bool state_chrgmode1;
	/*!
	 * Power up sense USB mode.
	 * UMOD0 and UMOD1 pins allow to configure the default USB operating
	 * mode. Note that UMOD0 is a trinary pin and UMOD1 a binary pin.
	 */
	bool state_umod;
	/*!
	 * Power up sense bootmode enable for USB/RS232.
	 * mc13783 supports booting on USB. The boot mode is entered by the
	 * USBEN pin being forced high which enables the USB transceiver
	 * and the VUSB regulator supplied from BP.
	 */
	bool state_usben;
	/*!
	 * Power up sense switcher 1a1b joined.
	 * @verbatim
	 0 = SW1A and SW1B independent operation
	 1 = SW1A and SW1B joined operation
	 @endverbatim
	 */
	bool state_sw_1a1b_joined;
	/*!
	 * Power up sense switcher 1a1b joined.
	 * @verbatim
	 0 = SW2A and SW2B independent operation
	 1 = SW2A and SW2B joined operation
	 @endverbatim
	 */
	bool state_sw_2a2b_joined;
};

/*!
 * This struct is used to configure a switcher (SW1A, SW1B ...)
 * There are in total 4 buck switcher units: SW1A, SW1B, SW2A and SW2B.
 */
struct t_switcher_conf {
	/*!
	 * Switcher setting.
	 * Output voltage.
	 */
	int sw_setting;
	/*!
	 * Switcher setting dvs.
	 * The buck switchers support dynamic voltage scaling (DVS).
	 * sw_setting_dvs allow to control output voltage with DVS.
	 */
	int sw_setting_dvs;
	/*!
	 * Switcher standby setting.
	 * sw_setting_standby allow to control output voltage in standby.
	 */
	int sw_setting_standby;
	/*!
	 * Switcher operation mode.
	 * @verbatim
	 Available operating mode are:
	 0   OFF
	 1   PWM mode. No Pulse Skipping.
	 2   PWM mode. Pulse Skipping Allowed.
	 3   Low Power PFM mode.
	 @endverbatim
	 */
	int sw_op_mode;
	/*!
	 * Switcher standby mode.
	 * In case STANDBY is made high, the mode as set by the standby mode
	 * is applied.
	 */
	int sw_op_mode_standby;
	/*!
	 * Switcher dvs speed.
	 * The tables below give key performance parameters and bit
	 * definitions for the DVS voltage manipulation registers.
	 * @verbatim
	 DVS Speed           Function
	 -----------------   --------------------------
	 0                   Transition speed is dictated by the current
	 limit and input -output conditions
	 1                   25mV step each 4us
	 2                   25mV step each 8us
	 3                   25mV step each 16us
	 @endverbatim
	 */
	int sw_dvs_speed;
	/*!
	 * Switcher panic mode.
	 * To avoid that the output sags too much during large transients
	 * and therefore gets out of the desired range, a so called
	 * panic mode is available.
	 * @verbatim
	 0= panic mode disabled
	 1= panic mode enabled
	 @endverbatim
	 */
	bool sw_panic_mode;
	/*!
	 * Switcher soft start.
	 * To avoid high inrush currents at startup, a soft start mechanism is
	 * implemented.
	 * @verbatim
	 0= soft start disabled
	 1= soft start enabled
	 @endverbatim
	 */
	bool sw_softstart;
};

/*!
 * This struct is used to configure power cut mode
 */
struct t_power_cut_conf {
	/*!
	 * Power cut counter enable.
	 * Enable the power count function.
	 * @verbatim
	 false= power cut disabled
	 true= power cut enabled
	 @endverbatim
	 */
	bool pc_counter_en;
	/*!
	 * Power cut auto user off.
	 * Automatic transition to user off during power cut.
	 * @verbatim
	 false= function disabled
	 true= function enabled
	 @endverbatim
	 */
	bool pc_auto_user_off;
	/*!
	 * Power cut user off 32k enable.
	 * Keeps the CLK32KMCU active during user off power cut modes.
	 * @verbatim
	 false= function disabled
	 true= function enabled
	 @endverbatim
	 */
	bool pc_user_off_32k_en;
	/*!
	 * Power cut timer.
	 * Determine the maximum duration of a powercut. This timer is set to
	 * a preset value. When a power cut occurs the timer will internally
	 * be decremented till it expires, meaning counted down to zero.
	 * The contents of PCT[7:0] does not reflect the actual counted
	 * down value but will keep the programmed value and therefore does
	 * not have to be reprogrammed after each power cut.
	 */
	int pc_timer;
	/*!
	 * Power cut counter.
	 * The number of times a power cut mode occurs is counted with this
	 * 4 bit counter.
	 * After a successful power up after a power cut, software will have
	 * to clear the power cut counter.
	 */
	int pc_counter;
	/*!
	 * Power cut max number pc.
	 * Limit the upfront power cut counter. When the contents pc_counter is
	 * equal to pc_max_nb_pc the next power cut will not be supported.
	 */
	int pc_max_nb_pc;
	/*!
	 * Power cut ext timer.
	 * The extended power cut timer starts running when entering Memory
	 * Hold or User Off Extended Power Cut mode. If the timer is set to
	 * zero or when it expires, the state machine transitions to the
	 * Invalid Power mode.
	 */
	int pc_ext_timer;
	/*!
	 * Power cut timer infinite.
	 * Extended power cut timer set to infinite.
	 */
	bool pc_ext_timer_inf;
};

struct t_vbkup_conf {
	/*!
	 * Vbackup enable.
	 * Enables VBKUP1 or VBKUP2 in startup modes, on and user off wait
	 * modes.
	 * VBKUP1 switch is used to supply memory circuit.
	 * VBKUP2 switch is used to supply processor cores circuit.
	 * @verbatim
	 false= function disabled
	 true= function enabled
	 @endverbatim
	 */
	bool vbkup_en;
	/*!
	 * Vbackup automatically enable.
	 * Automatically enables VBKUP1/VBKUP2 in the user off modes.
	 * @verbatim
	 false= function disabled
	 true= function enabled
	 @endverbatim
	 */
	bool vbkup_auto_en;
	/*!
	 * Vbackup voltage.
	 * Sets VBKUP1/VBKUP2 voltage
	 * @verbatim
	 Parameter     Value       Function
	 ------------  ----------  -------------
	 VBKUP1        0           output = 1.0V
	 1           output = 1.2V
	 2           output = 1.575V
	 3           output = 1.8V
	 VBKUP2        0           output = 1.0V
	 1           output = 1.2V
	 2           output = 1.5V
	 3           output = 1.8V
	 @endverbatim
	 */
	int vbkup_voltage;
};

struct t_pll_conf {
	/*!
	 * PLL enable.
	 * The PLL generates an effective 1.048MHz signal based upon the
	 * 32.768 kHz oscillator signal by multiplying it by 32.
	 * Low power standby modes are provided controlled by the standby pins.
	 * In the lowest power standby mode, the switchers no longer have
	 * to be PWM controlled and their output is maintained based on
	 * hysteresis control. If all switchers are in this mode and the ADC
	 * is not used, the PLL is automatically powered off unless the
	 * pll_en is set to true.
	 */
	bool pll_en;
	/*!
	 * PLL factor.
	 * Internally, the PLL may generate higher clock frequencies for its
	 * proper use.
	 * @verbatim
	 PLLX     Multiplication Factor      Switching Frequency (kHz)
	 -------  -------------------------  -------------------------
	 0        28                           917 504
	 1        29                           950 272
	 2        30                           983 040
	 3        31                         1 015 808
	 4        32                         1 048 576
	 5        33                         1 081 344
	 6        34                         1 114 112
	 7        35                         1 146 880
	 @endverbatim
	 */
	int pll_factor;
};

struct t_sw3_conf {
	/*!
	 * Switcher 3 enable.
	 * Enable SW3 switch. SW3 is a boost supply, which can be programmed
	 * to a fixed output voltage level. SW3 supplies the backlights,
	 * and the regulators for the USB.
	 * @verbatim
	 false= SW3 disabled
	 true= SW3 enabled
	 @endverbatim
	 */
	bool sw3_en;
	/*!
	 * Switcher 3 setting.
	 * @verbatim
	 Setting        Vout
	 -------------  ------------
	 0              5.0V
	 1              5.0V
	 2              5.0V
	 3              5.5V
	 @endverbatim
	 */
	int sw3_setting;
	/*!
	 * Switcher 3 control standby.
	 * @verbatim
	 false= SW3 mode not controlled by Standby
	 true=  SW3 mode controlled by Standby
	 @endverbatim
	 */
	bool sw3_ctr_stby;
	/*!
	 * Switcher 3 operation mode.
	 * Available operating mode are:
	 * @verbatim
	 false   OFF
	 true    Low power mode enable
	 @endverbatim
	 */
	bool sw3_op_mode;
};

/*!
 * This enumeration define all VBKUP switcher.
 */
typedef enum {
	/*!
	 * VBKUP1.
	 * VBKUP1 switch is used to supply memory circuit.
	 */
	VBKUP_1 = 0,
	/*!
	 * VBKUP2.
	 * VBKUP2 switch is used to supply processor cores circuit.
	 */
	VBKUP_2,
} t_vbkup;

/*!
 * This enumeration define all On_OFF button
 */
typedef enum {
	/*!
	 * ON1B.
	 * Power on/off button.
	 */
	BT_ON1B = 0,
	/*!
	 * ON2B.
	 * Accessory power on/off button.
	 */
	BT_ON2B,
	/*!
	 * ON3B.
	 * Third power on/off button.
	 */
	BT_ON3B,
} t_button;

/*!
 * This enumeration define all buck switcher.
 * There are in total 4 buck switcher units: SW1A, SW1B, SW2A and SW2B.
 * The units have the same topology. The A units are strictly identical
 * as are the B units.
 */
typedef enum {
	/*!
	 * SW1A.
	 */
	SW_SW1A = 0,
	/*!
	 * SW1B.
	 */
	SW_SW1B,
	/*!
	 * SW2A.
	 */
	SW_SW2A,
	/*!
	 * SW2B.
	 */
	SW_SW2B,

} t_switcher;

/*!
 * This enumeration define all regulator enabled by regen
 */
typedef enum {
	/*!
	 * VAudio.
	 * The audio section is supplied from a dedicated regulator VAUDIO,
	 * except for the loudspeaker amplifier Alsp which is directly
	 * supplied from the battery.
	 */
	REGU_VAUDIO = 0,
	/*!
	 * VIOHI.
	 * High level voltage for IO.
	 */
	REGU_VIOHI,
	/*!
	 * VIOLO.
	 * Low level voltage for IO.
	 */
	REGU_VIOLO,
	/*!
	 * VDIG.
	 * Regulator for general digital section.
	 */
	REGU_VDIG,
	/*!
	 * VGEN.
	 * Regulator for graphics accellerator.
	 */
	REGU_VGEN,
	/*!
	 * VRFDIG.
	 * Regulator for transceiver digital section.
	 */
	REGU_VRFDIG,		/*5 */
	/*!
	 * VRFREF.
	 * Regulator for transceiver reference.
	 */
	REGU_VRFREF,
	/*!
	 * VRFCP.
	 * Regulator for transceiver charge pump.
	 */
	REGU_VRFCP,
	/*!
	 * VSIM.
	 * Regulator for SIM card
	 */
	REGU_VSIM,
	/*!
	 * VESIM.
	 * Regulator for eSIM card
	 */
	REGU_VESIM,
	/*!
	 * VCAM.
	 * Regulator for camera.
	 */
	REGU_VCAM,		/*10 */
	/*!
	 * VRFBG.
	 * Reference output for transceiver.
	 */
	REGU_VRFBG,
	/*!
	 * VVIB.
	 * Regulator for vibrator motor.
	 */
	REGU_VVIB,
	/*!
	 * VRF1.
	 * Regulator for transceiver.
	 */
	REGU_VRF1,
	/*!
	 * VRF2.
	 * Regulator for receiver.
	 */
	REGU_VRF2,
	/*!
	 * VMMC1.
	 * Regulator for MMC1 module.
	 * The MMC card can be either a hot swap MMC or SD card or an
	 * extension module. VMMC1 and VMMC2 can also be used to supply
	 * other peripherals like a Bluetooth or WLAN.
	 */
	REGU_VMMC1,
	/*!
	 * VMMC2.
	 * Regulator for MMC2 module.
	 */
	REGU_VMMC2,
	/*!
	 * GPO1.
	 * Discrete regulator controlled by General purpose output 1.
	 */
	REGU_GPO1,
	/*!
	 * GPP2.
	 * Discrete regulator controlled by General purpose output 2.
	 */
	REGU_GPO2,
	/*!
	 * GPO3.
	 * Discrete regulator controlled by General purpose output 3.
	 */
	REGU_GPO3,
	/*!
	 * GPO4.
	 * Discrete regulator controlled by General purpose output 4.
	 */
	REGU_GPO4,
	/*!
	 * REGU_NUMBER.
	 * Total number of regulators.
	 */
	REGU_NUMBER,
} t_regulator;

/*!
 * This enumeration define all power interrupts
 */
typedef enum {
	/*!
	 * BP turn on threshold detection.
	 * Corresponds to attaching a charged battery to the phone.
	 */
	PWR_IT_BPONI = 0,
	/*!
	 * End of life / low battery detect.
	 * Low battery low threshold warning.
	 */
	PWR_IT_LOBATLI,
	/*!
	 * Low battery warning.
	 * Low battery high threshold warning.
	 */
	PWR_IT_LOBATHI,
	/*!
	 * ON1B event.
	 * Power on/off button pulled low.
	 */
	PWR_IT_ONOFD1I,
	/*!
	 * ON2B event.
	 * Accessory power on/off button pulled low.
	 */
	PWR_IT_ONOFD2I,
	/*!
	 * ON3B event.
	 * Third power on/off button pulled low.
	 */
	PWR_IT_ONOFD3I,
	/*!
	 * System reset.
	 */
	PWR_IT_SYSRSTI,
	/*!
	 * Power ready.
	 * Inform the processor that the switcher outputs have reached
	 * their new setpoint and that a power gate is fully conducting.
	 */
	PWR_IT_PWRRDYI,
	/*!
	 * Power cut event.
	 */
	PWR_IT_PCI,
	/*!
	 * Warm start event.
	 */
	PWR_IT_WARMI,
	/*!
	 * Memory hold event.
	 */
} t_pwr_int;

/*
 * POWER mc13783 API
 */

/* EXPORTED FUNCTIONS */

/*!
 * This function returns power up sense value
 *
 * @param        p_up_sense  	Value of power up sense
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_power_mode_sense(struct t_p_up_sense *p_up_sense);

/*!
 * This function configures the Regen assignment for all regulator
 * The REGEN pin allows to enable or disable one or more regulators and
 * switchers of choice. The REGEN function can be used in two ways.
 * It can be used as a regulator enable pin like with SIMEN
 * where the SPI programming is static and the REGEN pin is dynamic.
 * It can also be used in a static fashion where REGEN is maintained
 * high while the regulators get enabled and disabled dynamically
 * via SPI. In that case REGEN functions as a master enable.
 *
 * @param        regu     	Type of regulator
 * @param        en_dis  	If true, the regulator is enabled by regen.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_regen_assig(t_regulator regu, bool en_dis);

/*!
 * This function gets the Regen assignment for all regulator
 *
 * @param        regu     	Type of regulator
 * @param        en_dis  	Return value, if true :
 *                              the regulator is enabled by regen.
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regen_assig(t_regulator regu, bool * en_dis);

/*!
 * This function sets the Regen polarity.
 * The polarity of the REGEN pin is programmable with the REGENINV bit,
 * when set to "0" it is active high, when set to a "1" it is active low.
 *
 * @param        en_dis  	If true regen is inverted.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_regen_inv(bool en_dis);

/*!
 * This function gets the Regen polarity.
 *
 * @param        en_dis  	If true regen is inverted.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regen_inv(bool * en_dis);

/*!
 * This function configures 4 buck switcher units: SW1A, SW1B, SW2A
 * and SW2B.
 * The configuration consist in selecting the output voltage, the dvs voltage,
 * the standby voltage, the operation mode, the standby operation mode, the
 * dvs speed, the panic mode and the softstart.
 *
 * @param        sw  	 Define type od switcher.
 * @param        sw_conf Define new configuration.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_conf_switcher(t_switcher sw, struct t_switcher_conf *sw_conf);

/*!
 * This function returns configuration of one switcher.
 *
 * @param        sw  	 Define type od switcher.
 * @param        sw_conf Return value of configuration.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_switcher(t_switcher sw, struct t_switcher_conf *sw_conf);

/*!
 * This function enables or disables a regulator.
 *
 * @param        regu           Define the regulator.
 * @param        en_dis  	If true, enable the regulator.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_regu_en(t_regulator regu, bool en_dis);

/*!
 * This function returns state of regulator.
 *
 * @param        regu           Define the regulator.
 * @param        en_dis  	If true, regulator is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regu_state(t_regulator regu, bool * en_dis);

/*!
 * This function configures a regulator.
 *
 * @param        regu           Define the regulator.
 * @param        stby  	        If true, the regulator is controlled by standby.
 * @param        mode  	        Configure the regulator operating mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_conf_regu(t_regulator regu, bool stby, bool mode);

/*!
 * This function gets configuration of one regulator.
 *
 * @param        regu           Define the regulator.
 * @param        stby  	        If true, the regulator is controlled by standby.
 * @param        mode  	        Return the regulator operating mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regu_conf(t_regulator regu, bool * stby, bool * mode);

/*!
 * This function sets value of regulator setting.
 *
 * @param        regu           Define the regulator.
 * @param        setting        Define the regulator setting.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_regu(t_regulator regu, int setting);

/*!
 * This function gets value of regulator setting.
 *
 * @param        regu           Define the regulator.
 * @param        setting        Return the regulator setting.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_regu(t_regulator regu, int *setting);

/*!
 * This function enables power cut mode.
 * A power cut is defined as a momentary loss of power.
 * In the power cut modes, the backup cell voltage is indirectly monitored
 * by a comparator sensing the on chip VRTC supply.
 *
 * @param        en           If true, power cut enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_cut_en(bool en);

/*!
 * This function gets power cut mode.
 *
 * @param        en           If true, power cut is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_power_cut(bool * en);

/*!
 * This function enables warm start mode.
 * When User Off Wait expired, the Wait mode is exited for User Off mode
 * or Memory Hold mode depending on warm starts being enabled.
 *
 * @param        en           If true, warm start enable.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_warm_start_en(bool en);

/*!
 * This function gets warm start mode.
 *
 * @param        en           If true, warm start is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_warm_start(bool * en);

/*!
 * This function send SPI command for entering user off modes.
 *
 * @param        en           If true, entering user off modes
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_user_off_en(bool en);

/*!
 * This function returns value of user off modes SPI command.
 *
 * @param        en           If true, user off modes is started
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_user_off_en(bool * en);

/*!
 * This function enables CLK32KMCU clock.
 *
 * @param        en           If true, enable CLK32K
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_32k_en(bool en);

/*!
 * This function gets state of CLK32KMCU clock.
 *
 * @param        en           If true, CLK32K is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_32k_state(bool * en);

/*!
 * This function enables automatically VBKUP2 in the memory hold modes.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, enable VBKUP2AUTOMH
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_vbkup2_auto_en(bool en);

/*!
 * This function gets state of automatically VBKUP2AUTOMH.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, CLK32K is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_vbkup2_auto_state(bool * en);

/*!
 * This function enables battery detect function.
 * Battery detect comparator will compare the voltage at ADIN5 with BATTDET.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, enable BATTDETEN
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_bat_det_en(bool en);

/*!
 * This function gets state of battery detect function.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, BATTDETEN is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_bat_det_state(bool * en);

/*!
 * This function enables esim control voltage.
 * VESIM supplies the eSIM card.
 * The MMC card can be either a hot swap MMC or SD card or an extension module.
 * Only on mc13783 2.0 or higher
 *
 * @param        vesim          If true, enable VESIMESIMEN
 * @param        vmmc1          If true, enable VMMC1ESIMEN
 * @param        vmmc2          If true, enable VMMC2ESIMEN
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_esim_v_en(bool vesim, bool vmmc1, bool vmmc2);

/*!
 * This function gets esim control voltage values.
 * Only on mc13783 2.0 or higher
 *
 * @param        vesim          If true, enable VESIMESIMEN
 * @param        vmmc1          If true, enable VMMC1ESIMEN
 * @param        vmmc2          If true, enable VMMC2ESIMEN
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_gets_esim_v_state(bool * vesim, bool * vmmc1, bool * vmmc2);

/*!
 * This function enables control of the regulator for the vibrator motor (VVIB)
 * by VIBEN pin.
 * Only on mc13783 2.0 or higher
 *
 * @param        en           If true, enable VIBPINCTRL
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_vib_pin_en(bool en);

/*!
 * This function gets state of control of VVIB by VIBEN pin.
 * Only on mc13783 2.0 or higher
 * @param        en           If true, VIBPINCTRL is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_gets_vib_pin_state(bool * en);

/*!
 * This function configures power cut mode.
 *
 * @param        pc  	Define configuration of power cut mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_cut_conf(struct t_power_cut_conf *pc);

/*!
 * This function gets configuration of power cut mode.
 *
 * @param        pc  	Return configuration of power cut mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_cut_get_conf(struct t_power_cut_conf *pc);

/*!
 * This function configures v-backup during power cut.
 * Configuration parameters are : enable or not VBKUP; set or nor automatic
 * enabling of VBKUP in the memory hold and user off modes; set VBKUP voltage.
 * VBKUP1 switch is used to supply memory circuit.
 * VBKUP2 switch is used to supply processor cores circuit.
 *
 * @param        vbkup  	Type of VBLUP (1 or2).
 * @param        vbkup_conf  	Define configuration of VBLUP.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_vbkup_conf(t_vbkup vbkup, struct t_vbkup_conf *vbkup_conf);

/*!
 * This function gets configuration of v-backup.
 *
 * @param        vbkup  	Type of VBLUP (1 or2).
 * @param        vbkup_conf  	Return configuration of VBLUP.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_vbkup_conf(t_vbkup vbkup,
				 struct t_vbkup_conf *vbkup_conf);

/*!
 * This function sets BP detection threshold.
 * @verbatim
          BPDET       UVDET  LOBATL        LOBATH          BPON
          ---------   -----  ------------  --------------  ------------
            0         2.6    UVDET + 0.2   UVDET + 0.4     3.2
            1         2.6    UVDET + 0.3   UVDET + 0.5     3.2
            2         2.6    UVDET + 0.4   UVDET + 0.7     3.2
            3         2.6    UVDET + 0.5   UVDET + 0.8     3.2
   @endverbatim
 *
 * @param        threshold           Define threshold
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_bp_threshold(int threshold);

/*!
 * This function gets BP detection threshold.
 *
 * @param        threshold           Return threshold
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_bp_threshold(int *threshold);

/*!
 * This function selects EOL function instead of LOBAT.
 * If the EOL function is set, the LOBATL threshold is not monitored but
 * the drop out on the VRF1, VRF2 and VRFREF regulators.
 *
 * @param        eol          If true, selects EOL function
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_eol_function(bool eol);

/*!
 * This function gets EOL function instead of LOBAT.
 *
 * @param        eol          If true, selects EOL function
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_eol_function(bool * eol);

/*!
 * This function sets coincell charger voltage setting.
 * @verbatim
                VCOIN[2:0]      V
                --------------  --------
                0               2.50
                1               2.70
                2               2.80
                3               2.90
                4               3.00
                5               3.10
                6               3.20
                7               3.30
   @endverbatim
 *
 * @param        voltage         Value of voltage setting
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_coincell_voltage(int voltage);

/*!
 * This function gets coincell charger voltage setting.
 *
 * @param        voltage         Return value of voltage setting
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_coincell_voltage(int *voltage);

/*!
 * This function enables coincell charger.
 *
 * @param        en         If true, enable the coincell charger
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_coincell_charger_en(bool en);

/*!
 * This function gets state of coincell charger.
 *
 * @param        en         If true, the coincell charger is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_coincell_charger_en(bool * en);

/*!
 * This function enables auto reset after a system reset.
 *
 * @param        en         If true, the auto reset is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_auto_reset_en(bool en);

/*!
 * This function gets auto reset configuration.
 *
 * @param        en         If true, the auto reset is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_auto_reset_en(bool * en);

/*!
 * This function configures a system reset on a button.
 *
 * @param       bt         Type of button.
 * @param       sys_rst    If true, enable the system reset on this button
 * @param       deb_time   Sets the debounce time on this button pin
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_conf_button(t_button bt, bool sys_rst, int deb_time);

/*!
 * This function gets configuration of a button.
 *
 * @param       bt         Type of button.
 * @param       sys_rst    If true, the system reset is enabled on this button
 * @param       deb_time   Gets the debounce time on this button pin
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_conf_button(t_button bt, bool * sys_rst, int *deb_time);

/*!
 * This function configures switcher PLL.
 * PLL configuration consists enabling or not PLL in power off mode and
 * to select the internal multiplication factor.
 *
 * @param       pll_conf   Configuration of PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_conf_pll(struct t_pll_conf *pll_conf);

/*!
 * This function gets configuration of switcher PLL.
 *
 * @param       pll_conf   Configuration of PLL.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_conf_pll(struct t_pll_conf *pll_conf);

/*!
 * This function configures SW3 switcher.
 * SW3 supplies the backlights, and the regulators for the USB.
 * SW3 configuration consists in enabling or not the switch, selecting the
 * voltage, enabling or not control by standby pin and enabling or not
 * low power mode.
 *
 * @param       sw3_conf   Configuration of sw3 switcher.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_set_sw3(struct t_sw3_conf *sw3_conf);

/*!
 * This function gets configuration of SW3 switcher.
 *
 * @param       sw3_conf   Configuration of sw3 switcher.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_get_sw3(struct t_sw3_conf *sw3_conf);

/*!
 * This function is used to subscribe on power event IT.
 *
 * @param        event  	Type of event.
 * @param        callback  	Event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_event_sub(t_pwr_int event, void *callback);

/*!
 * This function is used to un subscribe on power event IT.
 *
 * @param        event  	Type of event.
 * @param        callback  	Event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_power_event_unsub(t_pwr_int event, void *callback);

/*!
 * This function is used to tell if mc13783 Power has been correctly loaded.
 *
 * @return       This function returns 1 if Power was successfully loaded
 * 		 0 otherwise.
 */
int mc13783_power_loaded(void);

#endif				/* __MC13783_POWER_H__ */
