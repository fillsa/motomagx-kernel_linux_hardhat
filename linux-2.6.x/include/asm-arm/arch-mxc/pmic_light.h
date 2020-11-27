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

#ifndef _PMIC_LIGHT_H
#define _PMIC_LIGHT_H

/*!
 * @defgroup PMIC_LIGHT sc55112 Light Driver
 * @ingroup PMIC_DRVRS
 */

/*!
 * @file pmic_light.h
 * @brief This is the header of PMIC Light driver.
 *
 * @ingroup PMIC_LIGHT
 */

#include <asm/ioctl.h>

/*!
 * @name IOCTL user space interface
 */

/*! @{ */
/*!
 * Enable Backlight.
 * Argument type: none.
 */
#define PMIC_BKLIT_ENABLE                   _IO('p', 0xe0)
/*!
 * Disable Backlight.
 * Argument type: none.
 */
#define PMIC_BKLIT_DISABLE                  _IO('p', 0xe1)
/*!
 * Set backlight configuration.
 * Argument type: pointer to t_bklit_setting_param
 */
#define PMIC_SET_BKLIT                      _IOW('p', 0xe2, int)
/*!
 * Get backlight configuration.
 * Argument type: pointer to t_bklit_setting_param
 */
#ifdef CONFIG_MXC_MC13783_PMIC
#define PMIC_GET_BKLIT                      _IOWR('p', 0xe3, int)
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
#define PMIC_GET_BKLIT                      _IOR('p', 0xe3, int)
#endif
/*!
 * Ramp up configuration.
 * Argument type: t_bklit_channel
 */
#define PMIC_RAMPUP_BKLIT                   _IOW('p', 0xe4, int)
/*!
 * Ramp down configuration.
 * Argument type: t_bklit_channel
 */
#define PMIC_RAMPDOWN_BKLIT                 _IOW('p', 0xe5, int)
/*!
 * Enable Tri-color LED.
 * Argument type: t_tcled_enable_param
 */
#define PMIC_TCLED_ENABLE                   _IOW('p', 0xe6, int)
/*!
 * Disable Tri-color LED.
 * Argument type: t_funlight_bank
 */
#ifdef CONFIG_MXC_MC13783_PMIC
#define PMIC_TCLED_DISABLE                  _IOW('p', 0xe7, int)
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
#define PMIC_TCLED_DISABLE                  _IO('p', 0xe7)
#endif
/*!
 * Start Tri-color LED pattern.
 * Argument type: t_fun_param
 */
#define PMIC_TCLED_PATTERN                  _IOW('p', 0xe8, int)
#ifdef CONFIG_MXC_MC3783_PMIC
/*!
 * Enable Backlight & tcled.
 * Argument type: none.
 */
#define PMIC_BKLIT_TCLED_ENABLE             _IO('p', 0xe9)
/*!
 * Disable Backlight & tcled.
 * Argument type: none.
 */
#define PMIC_BKLIT_TCLED_DISABLE            _IO('p', 0xea)
/*!
 * Reset ramp up configuration.
 * Argument type: t_bklit_channel
 */
#define PMIC_OFF_RAMPUP_BKLIT               _IOW('p', 0xeb, int)
/*!
 * Reset ramp down configuration.
 * Argument type: t_bklit_channel
 */
#define PMIC_OFF_RAMPDOWN_BKLIT             _IOW('p', 0xec, int)
/*!
 * Set tcled ind configuration.
 * Argument type: t_tcled_ind_param
 */
#define PMIC_SET_TCLED			    _IOW('p', 0xed, int)
/*!
 * Get tcled ind configuration.
 * Argument type: t_tcled_ind_param
 */
#define PMIC_GET_TCLED			    _IOWR('p', 0xee, int)
#endif
/*! @} */

/*!
 * @enum t_bklit_mode
 * @brief Backlight modes.
 */
typedef enum {
	BACKLIGHT_CURRENT_CTRL_MODE,	/*! < Current control mode */
	BACKLIGHT_TRIODE_MODE	/*! < Triode mode */
} t_bklit_mode;

/*!
 * @enum t_bklit_channel
 * @brief Backlight channels.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
typedef enum {
	BACKLIGHT_LED1,		/*! < Backlight channel 1 */
	BACKLIGHT_LED2,		/*! < Backlight channel 2 */
	BACKLIGHT_LED3		/*! < Backlight channel 3 */
} t_bklit_channel;
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
typedef enum {
	BACKLIGHT_LED1,		/*! < Backlight channel 1 */
	BACKLIGHT_LED2		/*! < Backlight channel 2 */
} t_bklit_channel;
#endif

/*!
 * @enum t_bklit_strobe_mode
 * @brief Backlight Strobe Light Pulsing modes.
 */
typedef enum {
	/*!
	 * No Strobe Light Pulsing
	 */
	BACKLIGHT_STROBE_NONE,
	/*!
	 * Strobe Light Pulsing at 3.3% duty cycle over 300msec (Driver goes
	 * into Triode Mode with pulses constrained to 10msec.)
	 */
	BACKLIGHT_STROBE_FAST,
	/*!
	 * Strobe Light Pulsing at 10% duty cycle over 100msec (Driver goes
	 * into Triode Mode with pulses constrained to 10msec.)
	 */
	BACKLIGHT_STROBE_SLOW
} t_bklit_strobe_mode;

/*!
 * @struct t_bklit_setting_param
 * @brief Backlight setting.
 */

#ifdef CONFIG_MXC_MC13783_PMIC
typedef struct {
	t_bklit_channel channel;	/*!< Channel */
	t_bklit_mode mode;	/*!< Mode */
	t_bklit_strobe_mode strobe;	/*!< Strobe mode */
	unsigned char current_level;	/*!< Current level */
	unsigned char duty_cycle;	/*!< Duty cycle */
	unsigned char cycle_time;	/*!< Cycle time */
	bool edge_slow;		/*!< Edge Slow */
	bool en_dis;		/*!< Enable disable boost mode */
	unsigned int abms;	/*!< Adaptive boost
				 *   mode selection */
	unsigned int abr;	/*!< Adaptive
				 *   boost reference */
} t_bklit_setting_param;
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
typedef struct {
	t_bklit_channel channel;	/*!< Channel */
	t_bklit_mode mode;	/*!< Mode */
	t_bklit_strobe_mode strobe;	/*!< Strobe mode */
	unsigned char current_level;	/*!< Current level */
	unsigned char duty_cycle;	/*!< Duty cycle */
	unsigned char cycle_time;	/*!< Cycle time */
	bool edge_slow;		/*!< Edge Slow */
} t_bklit_setting_param;
#endif
/*!
 * @enum t_funlight_bank
 * @brief Tri-color LED fun light banks; this doesn't apply to sc55112.
 */
typedef enum {
	TCLED_FUN_BANK1 = 0,	/*! < Fun light bank 1 */
	TCLED_FUN_BANK2,	/*! < Fun light bank 2 */
	TCLED_FUN_BANK3		/*! < Fun light bank 3 */
} t_funlight_bank;

/*!
 * @enum t_tcled_mode
 * @brief Tri-color LED operation modes.
 *
 * The Tri-Color LED Driver circuitry includes 2 modes of operation. In LED
 * Indicator Mode, this circuitry operates as Red and Green LED Drivers with
 * flasher timing to indicate GSM network status. In Fun Light Mode, this
 * circuitry provides expanded capability for current control and distribution
 * that supplements the three channels.
 */
typedef enum {
	TCLED_IND_MODE = 0,	/*! < LED Indicator Mode */
	TCLED_FUN_MODE		/*! < Fun Light Mode */
} t_tcled_mode;

#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * @struct t_tcled_enable_param
 * @brief enable setting.
 */
typedef struct {
	t_funlight_bank bank;	/*!< Bank */
	t_tcled_mode mode;	/*!< Mode */
} t_tcled_enable_param;
#endif

/*!
 * @enum t_ind_channel
 * @brief Tri-color LED indicator mode channels.
 *
 */

#ifdef CONFIG_MXC_MC13783_PMIC
typedef enum {
	TCLED_IND_RED = 0,	/*! < Red LED */
	TCLED_IND_GREEN,	/*! < Green LED */
	TCLED_IND_BLUE		/*! < Blue LED */
} t_ind_channel;
#endif
#ifdef CONFIG_MXC_PMIC_SC55112
typedef enum {
	TCLED_IND_RED = 0,	/*! < Red LED */
	TCLED_IND_GREEN		/*! < Green LED */
} t_ind_channel;
#endif
/*!
 * @enum t_funlight_channel
 * @brief Tri-color LED fun light mode channels.
 *
 */
typedef enum {
	TCLED_FUN_CHANNEL1 = 0,	/*! < Fun light channel 1 (Red) */
	TCLED_FUN_CHANNEL2,	/*! < Fun light channel 2 (Green) */
	TCLED_FUN_CHANNEL3	/*! < Fun light channel 3 (Blue) */
} t_funlight_channel;

#ifndef CONFIG_MXC_MC13783_PMIC
/*!
 * @enum t_tcled_ind_blink_pattern
 * @brief Tri-color LED Indicator Mode blinking mode.
 */
typedef enum {
	TCLED_IND_OFF = 0,	/*! < Continuous off */
	TCLED_IND_BLINK_1,	/*! < 0.1s on 1.9s off */
	TCLED_IND_BLINK_2,	/*! < 0.2s on 1.8s off */
	TCLED_IND_BLINK_3,	/*! < 0.5s on 1.5s off */
	TCLED_IND_BLINK_4,	/*! < 0.25s on 0.75s off */
	TCLED_IND_BLINK_5,	/*! < 0.25s on 1.75s off */
	TCLED_IND_BLINK_6,	/*! < 0.05s on 1.95s off */
	TCLED_IND_BLINK_7,	/*! < 0.5s on 0.5s off */
	TCLED_IND_BLINK_8,	/*! < 0.5s off 0.5s on */
	TCLED_IND_BLINK_9,	/*! < 0.125s on 0.5s off 0.075s
				 * on 1.5s off*/
	TCLED_IND_BLINK_10,	/*! < 0.125s on 2.075s off */
	TCLED_IND_BLINK_11,	/*! < 0.625s off 0.075s on 1.5s off */
	TCLED_IND_ON		/*! < Continuous on */
} t_tcled_ind_blink_pattern;
#else
/*!
 * @enum t_tcled_ind_blink_pattern
 * @brief Tri-color LED Indicator Mode blinking mode.
 */
typedef enum {
	TCLED_IND_OFF = 0,	/*! < Continuous off */
	TCLED_IND_BLINK_1,	/*! < 1 / 31 */
	TCLED_IND_BLINK_2,	/*! < 2 / 31 */
	TCLED_IND_BLINK_3,	/*! < 3 / 31  */
	TCLED_IND_BLINK_4,	/*! < 4 / 31  */
	TCLED_IND_BLINK_5,	/*! < 5 / 31  */
	TCLED_IND_BLINK_6,	/*! < 6 / 31  */
	TCLED_IND_BLINK_7,	/*! < 7 / 31  */
	TCLED_IND_BLINK_8,	/*! < 8 / 31  */
	TCLED_IND_BLINK_9,	/*! < 9 / 31  */
	TCLED_IND_BLINK_10,	/*! < 10 / 31  */
	TCLED_IND_BLINK_11,	/*! < 11 / 31  */
	TCLED_IND_BLINK_12,	/*! < 12 / 31  */
	TCLED_IND_BLINK_13,	/*! < 13 / 31  */
	TCLED_IND_BLINK_14,	/*! < 14 / 31  */
	TCLED_IND_BLINK_15,	/*! < 15 / 31  */
	TCLED_IND_BLINK_16,	/*! < 16 / 31  */
	TCLED_IND_BLINK_17,	/*! < 17 / 31  */
	TCLED_IND_BLINK_18,	/*! < 18 / 31  */
	TCLED_IND_BLINK_19,	/*! < 19 / 31  */
	TCLED_IND_BLINK_20,	/*! < 20 / 31  */
	TCLED_IND_BLINK_21,	/*! < 21 / 31  */
	TCLED_IND_BLINK_22,	/*! < 22 / 31  */
	TCLED_IND_BLINK_23,	/*! < 23 / 31  */
	TCLED_IND_BLINK_24,	/*! < 24 / 31  */
	TCLED_IND_BLINK_25,	/*! < 25 / 31  */
	TCLED_IND_BLINK_26,	/*! < 26 / 31  */
	TCLED_IND_BLINK_27,	/*! < 27 / 31  */
	TCLED_IND_BLINK_28,	/*! < 28 / 31  */
	TCLED_IND_BLINK_29,	/*! < 29 / 31  */
	TCLED_IND_BLINK_30,	/*! < 30 / 31  */
	TCLED_IND_ON		/*! < Continuous on */
} t_tcled_ind_blink_pattern;
#endif

/*!
 * @enum t_tcled_cur_level
 * @brief Tri-color LED current levels.
 */
typedef enum {
	TCLED_CUR_LEVEL_1 = 0,	/*! < Tri-Color LED current level 1 */
	TCLED_CUR_LEVEL_2,	/*! < Tri-Color LED current level 2 */
	TCLED_CUR_LEVEL_3,	/*! < Tri-Color LED current level 3 */
	TCLED_CUR_LEVEL_4	/*! < Tri-Color LED current level 4 */
} t_tcled_cur_level;

/*!
 * @enum t_tcled_fun_cycle_time
 * @brief Tri-color LED fun light mode cycle time.
 */
typedef enum {
	TC_CYCLE_TIME_1 = 0,	/*! < Tri-Color LED cycle time 1 */
	TC_CYCLE_TIME_2,	/*! < Tri-Color LED cycle time 2 */
	TC_CYCLE_TIME_3,	/*! < Tri-Color LED cycle time 3 */
	TC_CYCLE_TIME_4		/*! < Tri-Color LED cycle time 4 */
} t_tcled_fun_cycle_time;

/*!
 * @enum t_tcled_fun_speed
 * @brief Tri-color LED fun light mode pattern speed.
 */
typedef enum {
	TC_OFF = 0,		/*! < Tri-Color pattern off */
	TC_SLOW,		/*! < Tri-Color slow pattern */
	TC_FAST			/*! < Tri-Color fast pattern */
} t_tcled_fun_speed;

/*!
 * @enum t_tcled_fun_speed
 * @brief Tri-color LED fun light mode pattern speed.
 */
typedef enum {
	TC_STROBE_OFF = 0,	/*! < No strobe */
	TC_STROBE_SLOW,		/*! < Slow strobe pattern */
	TC_STROBE_FAST		/*! < fast strobe pattern */
} t_tcled_fun_strobe_speed;

/*!
 * @enum t_chaselight_pattern
 * @brief Tri-color LED fun light mode chasing light patterns.
 */
typedef enum {
	RGB = 0,		/*!< R -> G -> B */
	BGR			/*!< B -> G -> R */
} t_chaselight_pattern;

/*!
 * This enumeration of Fun Light Pattern.
 */
typedef enum {
	/*!
	 * Blended ramps slow
	 */
	BLENDED_RAMPS_SLOW,
	/*!
	 * Blended ramps fast
	 */
	BLENDED_RAMPS_FAST,
	/*!
	 * Saw ramps slow
	 */
	SAW_RAMPS_SLOW,
	/*!
	 * Saw ramps fast
	 */
	SAW_RAMPS_FAST,
	/*!
	 * Blended bowtie slow
	 */
	BLENDED_BOWTIE_SLOW,
	/*!
	 * Blended bowtie fast
	 */
	BLENDED_BOWTIE_FAST,
	/*!
	 * Strobe slow
	 */
	STROBE_SLOW,
	/*!
	 * Strobe fast
	 */
	STROBE_FAST,
	/*!
	 * Chasing Light RGB Slow
	 */
	CHASING_LIGHT_RGB_SLOW,
	/*!
	 * Chasing Light RGB fast
	 */
	CHASING_LIGHT_RGB_FAST,
	/*!
	 * Chasing Light BGR Slow
	 */
	CHASING_LIGHT_BGR_SLOW,
	/*!
	 * Chasing Light BGR fast
	 */
	CHASING_LIGHT_BGR_FAST,
} t_fun_pattern;

/*!
 * @struct t_fun_param
 * @brief LED fun pattern IOCTL parameter
 */
typedef struct {
	t_funlight_bank bank;	/*!< TCLED bank */
	t_funlight_channel channel;	/*!< TCLED channel */
	t_fun_pattern pattern;	/*!< Fun pattern */
} t_fun_param;

/*!
 * @enum t_led_channel
 * @brief LED channels including backlight and tri-color LEDs.
 */
typedef enum {
	AUDIO_LED1,		/*! < Backlight channel 1 */
	AUDIO_LED2,		/*! < Backlight channel 2 */
	AUDIO_LEDR,		/*! < Fun light channel 1 (Red) */
	AUDIO_LEDG,		/*! < Fun light channel 2 (Green) */
	AUDIO_LEDB		/*! < Fun light channel 3 (Blue) */
} t_led_channel;

/*!
 * @enum t_aud_path
 * @brief LED audio modulation in-out audio channels
 */
typedef enum {
	MIXED_RX = 0,		/*!<  Mixed L & R Channel RX audio */
	TX			/*!<  TX path */
} t_aud_path;

/*!
 * @enum t_aud_gain
 * @brief LED audio modulation in-out audio channels
 */
typedef enum {
	GAIN_MINUS6DB = 0,	/*!< -6 dB */
	GAIN_0DB,		/*!< 0 dB */
	GAIN_6DB,		/*!< 6 dB */
	GAIN_12DB		/*!< 12 dB */
} t_aud_gain;

/*!
 * @struct t_tcled_ind_param
 * @brief LED parameter
 */
#ifdef CONFIG_MXC_MC13783_PMIC
typedef struct {
	t_funlight_bank bank;	/*! < tcled bank */
	t_ind_channel channel;	/*! < tcled channel */
	t_tcled_cur_level level;	/*! < tcled current level */
	t_tcled_ind_blink_pattern pattern;	/*! < tcled dutty cycle */
	bool skip;		/*! < tcled skip */
	bool rampup;		/*! < tcled rampup */
	bool rampdown;		/*! < tcled rampdown */
	bool half_current;	/*! < tcled half current */
} t_tcled_ind_param;
#endif

/* EXPORTED FUNCTIONS */
#ifdef __KERNEL__
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function enables backlight & tcled.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_tcled_master_enable(void);

/*!
 * This function disables backlight & tcled.
 *
 * @return       This function returns PMIC_SUCCESS if successful
 */
PMIC_STATUS pmic_bklit_tcled_master_disable(void);
#endif

/*!
 * This function enables backlight.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_master_enable(void);

/*!
 * This function disables backlight.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_master_disable(void);

/*!
 * This function sets backlight current level.
 * In sc55112, LED1 and LED2 are designed for a nominal full scale current
 * of 84mA in 12mA steps.  The channels are not individually adjustable, hence
 * the channel parameter is ignored.
 *
 * @param        channel   Backlight channel (Ignored in sc55112 because the
 *                         channels are not individually adjustable)
 * @param        level     Backlight current level, as the following table.
 *                         @verbatim
                               level     current
                               ------    -----------
                                 0         0 mA
                                 1         12 mA
                                 2         24 mA
                                 3         36 mA
                                 4         48 mA
                                 5         60 mA
                                 6         72 mA
                                 7         84 mA
                            @endverbatim
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_current(t_bklit_channel channel,
				   unsigned char level);

/*!
 * This function retrives backlight current level.
 * In sc55112, LED1 and LED2 are designed for a nominal full scale current
 * of 84mA in 12mA steps.  The channels are not individually adjustable, hence
 * the channel parameter is ignored.
 *
 * @param        channel   Backlight channel (Ignored in sc55112 because the
 *                         channels are not individually adjustable)
 * @param        level     Pointer to store backlight current level result.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_current(t_bklit_channel channel,
				   unsigned char *level);

/*!
 * This function sets a backlight channel duty cycle.
 * LED perceived brightness for each zone may be individually set by setting
 * duty cycle. The default setting is for 0% duty cycle; this keeps all zone
 * drivers turned off even after the master enable command. Each LED current
 * sink can be turned on and adjusted for brightness with an independent 4 bit
 * word for a duty cycle ranging from 0% to 100% in approximately 6.7% steps.
 *
 * @param        channel   Backlight channel.
 * @param        dc        Backlight duty cycle, as the following table.
 *                         @verbatim
                                dc        Duty Cycle (% On-time over Cycle Time)
                               ------    ---------------------------------------
                                  0        0%
                                  1        6.7%
                                  2        13.3%
                                  3        20%
                                  4        26.7%
                                  5        33.3%
                                  6        40%
                                  7        46.7%
                                  8        53.3%
                                  9        60%
                                 10        66.7%
                                 11        73.3%
                                 12        80%
                                 13        86.7%
                                 14        93.3%
                                 15        100%
                             @endverbatim
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_dutycycle(t_bklit_channel channel, unsigned char dc);

/*!
 * This function retrives a backlight channel duty cycle.
 *
 * @param        channel   Backlight channel.
 * @param        cycle     Pointer to backlight duty cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_dutycycle(t_bklit_channel channel,
				     unsigned char *dc);

/*!
 * This function sets a backlight channel cycle time.
 * Cycle Time is defined as the period of a complete cycle of
 * Time_on + Time_off. The default Cycle Time is set to 0.01 seconds such that
 * the 100 Hz on-off cycling is averaged out by the eye to eliminate
 * flickering. Additionally, the Cycle Time can be programmed to intentionally
 * extend the period of on-off cycles for a visual pulsating or blinking effect.
 *
 * @param        period    Backlight cycle time, as the following table.
 *                         @verbatim
                                period      Cycle Time
                               --------    ------------
                                  0          0.01 seconds
                                  1          0.1 seconds
                                  2          0.5 seconds
                                  3          2 seconds
                             @endverbatim
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_cycle_time(unsigned char period);

/*!
 * This function retrives a backlight channel cycle time setting.
 *
 * @param        period    Pointer to save backlight cycle time setting result.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_cycle_time(unsigned char *period);

/*!
 * This function sets backlight operation mode. There are two modes of
 * operations: current control and triode mode.
 * The Duty Cycle/Cycle Time control is retained in Triode Mode. Audio
 * coupling is not available in Triode Mode.
 *
 * @param        channel   Backlight channel.
 * @param        mode      Backlight operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_mode(t_bklit_channel channel, t_bklit_mode mode);
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function gets backlight operation mode. There are two modes of
 * operations: current control and triode mode.
 * The Duty Cycle/Cycle Time control is retained in Triode Mode. Audio
 * coupling is not available in Triode Mode.
 *
 * @param        channel   Backlight channel.
 * @param        mode      Backlight operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_mode(t_bklit_channel channel, t_bklit_mode * mode);
#endif
/*!
 * This function starts backlight brightness ramp up function; ramp time is
 * fixed at 0.5 seconds.
 *
 * @param        channel   Backlight channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_rampup(t_bklit_channel channel);
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function stops backlight brightness ramp up function;
 *
 * @param        channel   Backlight channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_off_rampup(t_bklit_channel channel);
#endif
/*!
 * This function starts backlight brightness ramp down function; ramp time is
 * fixed at 0.5 seconds.
 *
 * @param        channel   Backlight channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_rampdown(t_bklit_channel channel);
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function stops backlight brightness ramp down function.
 *
 * @param        channel   Backlight channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_off_rampdown(t_bklit_channel channel);
#endif
/*!
 * This function enables backlight analog edge slowing mode. Analog Edge
 * Slowing slows down the transient edges to reduce the chance of coupling LED
 * modulation activity into other circuits. Rise and fall times will be targeted
 * for approximately 50usec.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_enable_edge_slow(void);

/*!
 * This function disables backlight analog edge slowing mode. The backlight
 * drivers will default to an �Instant On� mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_disable_edge_slow(void);
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function gets backlight analog edge slowing mode. DThe backlight
 *
 * @param        edge      Edge slowing mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_get_edge_slow(bool * edge);
#endif
/*!
 * This function sets backlight Strobe Light Pulsing mode.
 *
 * @param        channel   Backlight channel.
 * @param        mode      Strobe Light Pulsing mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_bklit_set_strobemode(t_bklit_channel channel,
				      t_bklit_strobe_mode mode);

/*!
 * This function enables tri-color LED.
 *
 * @param        mode      Tri-color LED operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_enable(t_tcled_mode mode, t_funlight_bank bank);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_enable(t_tcled_mode mode);
#endif
/*!
 * This function disables tri-color LED.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_disable(t_funlight_bank bank);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_disable(void);
#endif
/*!
 * This function retrives tri-color LED operation mode.
 *
 * @param        mode      Pointer to Tri-color LED operation mode.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_get_mode(t_tcled_mode * mode, t_funlight_bank bank);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_get_mode(t_tcled_mode * mode);
#endif
/*!
 * This function sets a tri-color LED channel current level in indicator mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        level        Current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_ind_set_current(t_ind_channel channel,
				       t_tcled_cur_level level,
				       t_funlight_bank bank);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_ind_set_current(t_ind_channel channel,
				       t_tcled_cur_level level);
#endif
/*!
 * This function retrives a tri-color LED channel current level in indicator mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        level        Pointer to current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_ind_get_current(t_ind_channel channel,
				       t_tcled_cur_level * level,
				       t_funlight_bank bank);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_ind_get_current(t_ind_channel channel,
				       t_tcled_cur_level * level);
#endif
/*!
 * This function sets a tri-color LED channel blinking pattern in indication
 * mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        pattern      Blinking pattern.
 * @param        skip         If true, skip a cycle after each cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */

#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_ind_set_blink_pattern(t_ind_channel channel,
					     t_tcled_ind_blink_pattern pattern,
					     bool skip, t_funlight_bank bank);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_ind_set_blink_pattern(t_ind_channel channel,
					     t_tcled_ind_blink_pattern pattern,
					     bool skip);
#endif
/*!
 * This function retrives a tri-color LED channel blinking pattern in
 * indication mode.
 *
 * @param        channel      Tri-color LED channel.
 * @param        pattern      Pointer to Blinking pattern.
 * @param        skip         Pointer to a boolean variable indicating if skip
 *                            a cycle after each cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_ind_get_blink_pattern(t_ind_channel channel,
					     t_tcled_ind_blink_pattern *
					     pattern, bool * skip,
					     t_funlight_bank bank);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_ind_get_blink_pattern(t_ind_channel channel,
					     t_tcled_ind_blink_pattern *
					     pattern, bool * skip);
#endif
/*!
 * This function sets a tri-color LED channel current level in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        level        Current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_set_current(t_funlight_bank bank,
				       t_funlight_channel channel,
				       t_tcled_cur_level level);

/*!
 * This function retrives a tri-color LED channel current level
 * in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        level        Pointer to current level.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_get_current(t_funlight_bank bank,
				       t_funlight_channel channel,
				       t_tcled_cur_level * level);

/*!
 * This function sets tri-color LED cycle time in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        ct           Cycle time.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_set_cycletime(t_funlight_bank bank,
					 t_tcled_fun_cycle_time ct);

/*!
 * This function retrives tri-color LED cycle time in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        ct           Pointer to cycle time.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_get_cycletime(t_funlight_bank bank,
					 t_tcled_fun_cycle_time * ct);

/*!
 * This function sets a tri-color LED channel duty cycle in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        dc           Duty cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_set_dutycycle(t_funlight_bank bank,
					 t_funlight_channel channel,
					 unsigned char dc);

/*!
 * This function retrives a tri-color LED channel duty cycle in Fun Light mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        dc           Pointer to duty cycle.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_get_dutycycle(t_funlight_bank bank,
					 t_funlight_channel channel,
					 unsigned char *dc);

/*!
 * This function initiates Blended Ramp fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_blendedramps(t_funlight_bank bank,
					t_tcled_fun_speed speed);

/*!
 * This function initiates Saw Ramp fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_sawramps(t_funlight_bank bank,
				    t_tcled_fun_speed speed);

/*!
 * This function initiates Blended Bowtie fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_blendedbowtie(t_funlight_bank bank,
					 t_tcled_fun_speed speed);

/*!
 * This function initiates Chasing Lights fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        pattern      Chasing light pattern mode.
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_chasinglightspattern(t_funlight_bank bank,
						t_chaselight_pattern pattern,
						t_tcled_fun_speed speed);

/*!
 * This function initiates Strobe Mode fun light pattern.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        speed        Speed of pattern.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_strobe(t_funlight_bank bank,
				  t_funlight_channel channel,
				  t_tcled_fun_strobe_speed speed);

/*!
 * This function initiates Tri-color LED brightness Ramp Up function; Ramp time
 * is fixed at 1 second.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        rampup       Ramp-up configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_fun_rampup(t_funlight_bank bank,
				  t_funlight_channel channel, bool rampup);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_fun_rampup(t_funlight_bank bank,
				  t_funlight_channel channel);
#endif
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function gets Tri-color LED brightness Ramp Up function; Ramp time
 * is fixed at 1 second.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        rampup       Ramp-up configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_get_fun_rampup(t_funlight_bank bank,
				      t_funlight_channel channel,
				      bool * rampup);
#endif

/*!
 * This function initiates Tri-color LED brightness Ramp Down function; Ramp
 * time is fixed at 1 second.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        rampdown     Ramp-down configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
#ifdef CONFIG_MXC_MC13783_PMIC
PMIC_STATUS pmic_tcled_fun_rampdown(t_funlight_bank bank,
				    t_funlight_channel channel, bool rampdown);
#elif CONFIG_MXC_PMIC_SC55112
PMIC_STATUS pmic_tcled_fun_rampdown(t_funlight_bank bank,
				    t_funlight_channel channel);
#endif
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function initiates Tri-color LED brightness Ramp Down function; Ramp
 * time is fixed at 1 second.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 * @param        rampdown     Ramp-down configuration.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_get_fun_rampdown(t_funlight_bank bank,
					t_funlight_channel channel,
					bool * rampdown);
#endif

/*!
 * This function enables a Tri-color channel triode mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_triode_on(t_funlight_bank bank,
				     t_funlight_channel channel);

/*!
 * This function disables a Tri-color LED channel triode mode.
 *
 * @param        bank         Tri-color LED bank (This parameter is ignored
 *                            for sc55112).
 * @param        channel      Tri-color LED channel.
 *
 * @return       This function returns PMIC_SUCCESS if successful.
 */
PMIC_STATUS pmic_tcled_fun_triode_off(t_funlight_bank bank,
				      t_funlight_channel channel);

/*!
 * This function enables Tri-color LED edge slowing, this function does not
 * apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_enable_edge_slow(void);

/*!
 * This function disables Tri-color LED edge slowing, this function does not
 * apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_disable_edge_slow(void);

/*!
 * This function enables Tri-color LED half current mode, this function does
 * not apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_enable_half_current(void);

/*!
 * This function disables Tri-color LED half current mode, this function does
 * not apply to sc55112.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_disable_half_current(void);

/*!
 * This function enables backlight or Tri-color LED audio modulation.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_enable_audio_modulation(t_led_channel channel,
					       t_aud_path path,
					       t_aud_gain gain,
					       bool lpf_bypass);

/*!
 * This function disables backlight or Tri-color LED audio modulation.
 *
 * @return       This function returns PMIC_NOT_SUPPORTED.
 */
PMIC_STATUS pmic_tcled_disable_audio_modulation(void);
#ifdef CONFIG_MXC_MC13783_PMIC
/*!
 * This function enables the boost mode.
 * Only on mc13783 2.0 or higher
 *
 * @param       en_dis   Enable or disable the boost mode
 *
 * @return      This function returns 0 if successful.
 */
PMIC_STATUS pmic_bklit_set_boost_mode(bool en_dis);

/*!
 * This function gets the boost mode.
 * Only on mc13783 2.0 or higher
 *
 * @param       en_dis   Enable or disable the boost mode
 *
 * @return      This function returns 0 if successful.
 */
PMIC_STATUS pmic_bklit_get_boost_mode(bool * en_dis);

/*!
 * This function sets boost mode configuration
 * Only on mc13783 2.0 or higher
 *
 * @param    abms      Define adaptive boost mode selection
 * @param    abr       Define adaptive boost reference
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_bklit_config_boost_mode(unsigned int abms, unsigned int abr);

/*!
 * This function gets boost mode configuration
 * Only on mc13783 2.0 or higher
 *
 * @param    abms      Define adaptive boost mode selection
 * @param    abr       Define adaptive boost reference
 *
 * @return       This function returns 0 if successful.
 */
PMIC_STATUS pmic_bklit_gets_boost_mode(unsigned int *abms, unsigned int *abr);

#endif
#endif				/* __KERNEL__ */

#endif				/* _PMIC_LIGHT_H */
