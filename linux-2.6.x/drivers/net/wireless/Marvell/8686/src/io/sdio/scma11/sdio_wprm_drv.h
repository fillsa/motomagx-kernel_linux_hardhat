#ifndef _SDIO_WPRM_DRV_H_
#define _SDIO_WPRM_DRV_H_
/** @file sdio_wprm_drv.h
  *  
  * @brief This is header file for Power management APIs for SDIO driver
  */
/** 
  * @section copyright_sec Copyright
  *
  * (c) Copyright Motorola 2006
  *
  * This file is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * at your option) any later version.
  * This file is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.
  *
  * Revision History:
  * Author           Date            Description
  * Motorola         01-Sep-2006     Creation.
  * Motorola         03-Oct-2006     Improvement of power up sequence
  */
  
/*---------------------------------- Includes --------------------------------*/

/*---------------------------------- Globals ---------------------------------*/

/*---------------------------------- APIs ------------------------------------*/
void sdio_wprm_wlan_power_on(void);
void sdio_wprm_wlan_power_off(void);

#endif
