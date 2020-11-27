/*
 * Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

 /*!
  * @defgroup SPI Configurable Serial Peripheral Interface (CSPI) Driver
  */

 /*!
  * @file spi.h
  * @brief This header file contains SPI driver exported functions prototypes.
  *
  * @ingroup SPI
  */

#ifndef __SPI_H__
#define __SPI_H__

/*!
 * This enumeration describes index for both SPI1 and SPI2 modules.
 */
typedef enum {
	/*!
	 * SPI1 index
	 */
	SPI1 = 0,
	/*!
	 * SPI2 index
	 */
	SPI2 = 1,
	/*!
	 * SPI3 index
	 */
	SPI3 = 2
} module_nb_t;

/*!
 * This enumeration defines the different slave selects of a SPI module.
 */
typedef enum {
	/*!
	 * Slave Select 0
	 */
	SS_0 = 0,
	/*!
	 * Slave Select 1
	 */
	SS_1 = 1,
	/*!
	 * Slave Select 2
	 */
	SS_2 = 2,
	/*!
	 * Slave Select 3
	 */
	SS_3 = 3
} slave_select_t;

/*!
 * This enumeration indicates the priority of a client.
 */
typedef enum {
	/*!
	 * low priority
	 */
	LOW = 1,
	/*!
	 * medium priority
	 */
	MEDIUM = 2,
	/*!
	 * high priority
	 */
	HIGH = 3
} client_prio_t;

/*!
 * This structure is used as a parameter when calling \b spi_get_device_id.
 * It describes the configuration of the SPI interface.
 */
typedef struct {
	/*!
	 * the module number
	 */
	module_nb_t module_number;
	/*!
	 * the client priority
	 */
	client_prio_t priority;
	/*!
	 * the bit rate
	 */
	unsigned long bit_rate;
	/*!
	 * the Tx delay
	 */
	unsigned int tx_delay;
	/*!
	 * the bit count
	 */
	unsigned int bit_count;
	/*!
	 * the operation mode
	 */
	bool master_mode;
	/*!
	 * the polarity
	 */
	bool active_high_polarity;
	/*!
	 * the phase
	 */
	bool phase;
	/*!
	 * the slave select polarity
	 */
	bool active_high_ss_polarity;
	/*!
	 * the slave select waveform
	 */
	bool ss_low_between_bursts;
	/*!
	 * the slave select
	 */
	slave_select_t ss_asserted;
} spi_config;

/*!
 * This structure is used by spi-dev to manage data exchange between dev and core services.
 */
typedef struct {
	/*!
	 * pointer on the first data byte
	 */
	char *data;
	/*!
	 * the length of the data
	 */
	int length;
	/*!
	 * pointer on the device handle
	 */
	void *dev_handle;
} spi_frame;

/*!
 * This function gets a SPI device handler.
 * It allocates a spi_config structure to manage the SPI device configuration.
 *
 * @param        config        the SPI device configuration
 * @return       This function returns a handle on the allocated structure
 */
void *spi_get_device_id(spi_config * config);

/*!
 * This function sends a SPI frame.
 *
 * @param        buffer                the buffer to send
 * @param        nb                    the number of bytes to send
 * @param        client_handle         the handle to identify the SPI device
 * @return       This function returns the number of bytes sent minus the number of received bytes.
 */
ssize_t spi_send_frame(unsigned char *buffer, unsigned int nb,
		       spi_config * client_handle);

int mxc_spi_is_active(int spi_id);

#endif				/* __SPI_H__ */
