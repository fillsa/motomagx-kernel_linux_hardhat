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
 * @defgroup MC13783_CONNECTIVITY mc13783 Connectivity Driver
 * @ingroup MC13783DRVRS
 */

/*!
 * @file mc13783_connectivity.h
 * @brief This is the header of mc13783 Connectivity  driver.
 *
 * @ingroup MC13783_CONNECTIVITY
 */

#ifndef         __MC13783_CONNECTIVITY_H__
#define         __MC13783_CONNECTIVITY_H__

#define         MC13783_CONNECTIVITY_INIT                 _IOWR('C',1, int)
#define         MC13783_CONNECTIVITY_CHECK_API            _IOWR('C',2, int)
#define         MC13783_CONNECTIVITY_CHECK_USB_DETECT     _IOWR('C',3, int)

#ifdef __ALL_TRACES_CONNECTIVITY__
#define TRACEMSG_CONNECTIVITY(fmt,args...)  printk(fmt,##args)
#else				/* __ALL_TRACES__ */
#define TRACEMSG_CONNECTIVITY(fmt,args...)
#endif				/* __ALL_TRACES__ */

/*!
 * Enumeration of all types of pull down switcher
 */
typedef enum {

	/*!
	 * pull down variable 1.5k
	 */
	PD_PU = 0,

	/*!
	 * pull down variable 15k UPD
	 */
	PD_UPD_15,

	/*!
	 * pull down variable 15k UDM
	 */
	PD_UDM_15,

	/*!
	 * pull down variable 150k UDP
	 */
	PD_UDP_150,

	/*!
	 * pull down UID
	 */
	PD_UID,
} t_pd_type;

/*!
 * Enumeration of all current limit.
 */
typedef enum {

	/*!
	 * current limit to 200mA
	 */
	CL_200 = 0,

	/*!
	 * current limit to 910uA for 10ms
	 */
	CL_910_10,

	/*!
	 * current limit to 910uA for 20ms
	 */
	CL_910_20,

	/*!
	 * current limit to 910uA for 30ms
	 */
	CL_910_30,

	/*!
	 * current limit to 910uA for 40ms
	 */
	CL_910_40,

	/*!
	 * current limit to 910uA for 50ms
	 */
	CL_910_50,

	/*!
	 * current limit to 910uA for 60ms
	 */
	CL_910_60,

	/*!
	 * current limit to 910uA
	 */
	CL_910,
} t_current_limit;

/*!
 * Enumeration of all interface mode.
 */
typedef enum {

	/*!
	 * USB mode
	 */
	IM_USB = 0,

	/*!
	 * RS232 mode 1
	 */
	IM_RS232_1,

	/*!
	 * RS232 mode 2
	 */
	IM_RS232_2,

	/*!
	 * mono audio mode
	 */
	IM_MONO_AUDIO,

	/*!
	 * stereo audio mode
	 */
	IM_STEREO_AUDIO,
} t_interface_mode;

/*!
 * Enumeration of pulse line.
 */
typedef enum {

	/*!
	 * UID line
	 */
	PL_UID = 0,

	/*!
	 * UDM line
	 */
	PL_UDM,
} t_pulse_line;

/*!
 * Enumeration of VUSB regulator.
 */
typedef enum {

	/*!
	 * VUSBIN on boost
	 */
	VUSB_BOOST = 0,

	/*!
	 * VUSBIN on VUSB
	 */
	VUSB_VUSB,

	/*!
	 * VUSBIN on BP
	 */
	VUSB_BP,
} t_vusb_regulator;

/*!
 * Enumeration of interrupt of connectivity.
 */
typedef enum {

	/*!
	 * USB detect
	 */
	IT_CONVITY_USBI = 0,

	/*!
	 * USB ID Line detect
	 */
	IT_CONVITY_IDI,

	/*!
	 * Single ended 1 detect
	 */
	IT_CONVITY_SE1I,
} t_convity_int;

/*!
 * Enumeration of VUSB voltage.
 */
typedef enum {

	/*!
	 * VUSB voltage on 2.775V
	 */
	VUSB_2_775 = 0,

	/*!
	 * VUSB voltage on 3.3V
	 */
	VUSB_3_3,
} t_vusb;

/*
 * CONNECTIVITY mc13783 API
 */

/* EXPORTED FUNCTIONS */

/*!
 * This function enables low speed mode for USB.
 *
 * @param        low_speed  	if true, the low speed mode is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_speed_mode(bool low_speed);

/*!
 * This function returns the USB speed mode selected.
 *
 * @param        speed          return value of speed mode if false,
 *                              the full speed mode is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_speed_mode(bool * low_speed);

/*!
 * This function enables/disables USB suspend mode.
 *
 * @param        suspend  	if true, the suspend mode is enabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_state(bool suspend);

/*!
 * This function returns the USB suspend mode selected.
 *
 * @param        suspend        return value of suspend mode if true,
 *                              the suspend mode is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_state(bool * suspend);

/*!
 * This function configures pull-down switched in/out.
 *
 * @param         pdswtch 	define type of pull down switcher.
 * @param         in            if true, switch is in else is out.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_pull_down_switch(t_pd_type pdswtch, bool in);

/*!
 * This function returns state of pull-down switcher in/out.
 *
 * @param         pdswtch 	define type of pull down switcher.
 * @param         in            return value, if true, switch is 'in' else is
 *                              'out'.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_pull_down_switch(t_pd_type pdswtch, bool * in);

/*!
 * This function enables/disables VBUS.
 * This API configures the VBUSPDENB bits of USB register
 * @param        vbus 	if true, vbus pull-down nmos switch is off.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vbus(t_vusb vbus);

/*!
 * This function returns state of VBUS.
 *
 * @param        vbus 	if true, vbus pull-down nmos switch is off.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vbus(t_vusb * vbus);

/*!
 * This function configures the vbus regulator current limit.
 *
 * @param        current_limit 	value of current limit.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vbus_reg(t_current_limit current_limit);

/*!
 * This function gets the vbus regulator current limit.
 *
 * @param        current_limit 	return value of current limit.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vbus_reg(t_current_limit * current_limit);

/*!
 * This function enables/disables DLP Timer.
 * This API configures the DLPSRP bits of USB register
 *
 * @param        dlp 	if true, DLP Timer is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_dlp_timer(bool dlp);

/*!
 * This function returns state of DLP Timer.
 *
 * @param        dlp 	if true, DLP Timer is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_dlp_timer(bool * dlp);

/*!
 * This function configures auto connect of UDP.
 * This API configures the SE0CONN bits of USB register
 *
 * @param        udp_ac 	if true, UDP is automatically connected when SE0
 *                              is detected..
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_udp_auto_connect(bool udp_ac);

/*!
 * This function returns UDP auto connect state.
 *
 * @param        udp_ac 	if true, UDP is automatically connected when SE0
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_udp_auto_connect(bool * udp_ac);

/*!
 * This function configures USB transceiver.
 * This API configures the USBXCVREN bits of USB register
 *
 * @param        usb_trans 	if true, USB transceiver is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_usb_transceiver(bool usb_trans);

/*!
 * This function gets state of USB transceiver.
 *
 * @param        usb_trans 	if true, USB transceiver is enabled.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_usb_transceiver(bool * usb_trans);

/*!
 * This function connects or disconnects 1k5 pull-up and UDP/UMD pull-down.
 * This API configures the PULLOVER bits of USB register
 *
 * @param        connect 	if true, variable 1k5 and UDP/UDM pull-down
 *                              are disconnected.
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_var_disconnect(bool connect);

/*!
 * This function returns 1k5 pull-up and UDP/UMD pull-down.
 *
 * @param        connect 	if true, variable 1k5 and UDP/UDM pull-down
 *                              are disconnected.
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_var_disconnect(bool * connect);

/*!
 * This function configures connectivity interface mode.
 *
 * @param        mode 	define mode of interface.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_interface_mode(t_interface_mode mode);

/*!
 * This function gets connectivity interface mode.
 *
 * @param        mode 	return mode of interface.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_interface_mode(t_interface_mode * mode);

/*!
 * This function configures single/differential USB mode.
 *
 * This API configures the DATSE0 bits of USB register
 * @param        sgle 	if true, single ended is selected,
 *                      if false, differential is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_single_ended_mode(bool sgle);

/*!
 * This function gets single/differential USB mode.
 *
 * @param        sgle 	if true, single ended is selected,
 *                      if false, differential is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_single_ended_mode(bool * sgle);

/*!
 * This function configures directional USB mode.
 * This API configures the BIDIR bits of USB register
 * @param        bidir 	if true, unidirectional is selected,
 *                      if false, bidirectional is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_directional_mode(bool bidir);

/*!
 * This function gets directional USB mode.
 *
 * @param        bidir 	if true, unidirectional is selected,
 *                      if false, bidirectional is selected.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_directional_mode(bool * bidir);

/*!
 * This function launches a pulse on UID or UDM line.
 *
 * @param        line 	define line type.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_pulse(t_pulse_line line);

/*!
 * This function configures UID pull type.
 * This API configures the IDPUCNTRL bits of USB register
 *
 * @param        pull 	if true, pulled high by 4uA.
 *                      if false, pulled high through 220k resistor
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_udm_pull(bool pull);

/*!
 * This function returns UID pull type.
 *
 * @param        pull 	if true, pulled high by 4uA.
 *                      if false, pulled high through 220k resistor
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_udm_pull(bool * pull);

/*!
 * This function configures UDP pull up type.
 * This API configures the USBCNTRL bits of USB register
 *
 * @param        pull 	if true, controlled by USBEN pin
 *                      if false, controlled by SPI bits.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_udp_pull(bool pull);

/*!
 * This function returns UDP pull up type.
 *
 * @param        pull 	if true, controlled by USBEN pin
 *                      if false, controlled by SPI bits.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_udp_pull(bool * pull);

/*!
 * This function configures VUSB regulator.
 *
 * @param        reg_value      define value of regulator.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vusb_reg(t_vusb_regulator reg_value);

/*!
 * This function returns VUSB regulator.
 *
 * @param        reg_value      return value of regulator.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vusb_reg(t_vusb_regulator * reg_value);

/*!
 * This function controls the VUSB output voltage.
 * This API configures the VUSB bits of USB register
 *
 * @param        out 	if true, output voltage is set to 3.3V
 *                      if false, output voltage is set to 2.775V.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_vusb_voltage(t_vusb out);

/*!
 * This function reads the VUSB output voltage.
 *
 * @param        out 	if true, output voltage is set to 3.3V
 *                      if false, output voltage is set to 2.775V.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_vusb_voltage(t_vusb * out);

/*!
 * This function enables/disables VUSB and VBUS output.
 * This API configures the VUSBEN and VBUSEN bits of USB register
 *
 * @param        out_type true, for VUSB
 *                        false, for VBUS
 * @param        out 	if true, output is enabled
 *                      if false, output is disabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_set_output(bool out_type, bool out);

/*!
 * This function reads VUSB and VBUS output.
 *
 * @param        out_type true, for VUSB
 *                        false, for VBUS
 * @param        out 	if true, output is enabled
 *                      if false, output is disabled
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_output(bool out_type, bool * out);

/*!
 * This function configures Swaps TX and RX in RS232 mode.
 * It sets the RSPOL bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, swaps TX and RX in RS232 mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_swaps_txrx_en(bool en);

/*!
 * This function gets Swaps TX and RX configuration mode.
 * It gets the RSPOL bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, swaps TX and RX in RS232 mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_swaps_txrx(bool * en);

/*!
 * This function configures TX tristates mode.
 * It sets the RSTRI bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, TX is in tristates mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_tx_tristates_en(bool en);

/*!
 * This function gets TX tristates mode in RS232.
 * It gets the RSTRI bits of USB register
 * Only on mc13783 2.0 or higher
 *
 * @param        en 	if true, TS is in tristates mode.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_get_tx_tristates(bool * en);

/*!
 * This function is used to un/subscribe on connectivity event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 * @param        sub      	define if Un/subscribe event.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_event(t_convity_int event, void *callback, bool sub);

/*!
 * This function is used to subscribe on connectivity event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_event_sub(t_convity_int event, void *callback);

/*!
 * This function is used to unsubscribe on connectivity event IT.
 *
 * @param        event  	type of event.
 * @param        callback  	event callback function.
 *
 * @return       This function returns 0 if successful.
 */
int mc13783_convity_event_unsub(t_convity_int event, void *callback);

/*!
 * This function is used to tell if mc13783 Connectivity driver has been correctly loaded.
 *
 * @return       This function returns 1 if Connectivity was successfully loaded
 * 		 0 otherwise.
 */
int mc13783_convity_loaded(void);

#endif				/* __MC13783_CONNECTIVITY_H__ */
