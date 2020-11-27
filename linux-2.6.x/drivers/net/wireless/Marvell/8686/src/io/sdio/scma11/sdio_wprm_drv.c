/** @file sdio_wprm_drv.c
  *  
  * @brief This file includes Power management APIs for SDIO driver
  */
/** 
  * @section copyright_sec Copyright
  *
  * (c) Copyright Motorola 2006-2008
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
  * Motorola         12-Oct-2006     SDHC GPIO API change.
  *                                     Use define for SDHC module number
  * Motorola         21-Dec-2006     Remove build warning.  
  * Motorola         26-Feb-2007     Correct bad compilation switch for P3A wingboards.  
  * Motorola         15-Jan-2008     Supporting BT/WLAN single antenna solution (HW_BT_WLAN_SINGLE_ANTENNA)
  */


/*---------------------------------- Includes --------------------------------*/
#include <asm/system.h>
#include <asm/boardrev.h>
#include <asm/mot-gpio.h>
#include <linux/power_ic.h>
#include <linux/delay.h>
#include "sdhc.h"
#include "sdio_wprm_drv.h"

extern int power_ic_set_reg_value(POWER_IC_REGISTER_T reg, int index, int value, int nb_bits);
extern void gpio_sdhc_inactive(int module);
extern void gpio_wlan_powerdown_set_data(__u32 data);
extern void gpio_wlan_reset_set_data(__u32 data);

/*---------------------------------- Local prototype -------------------------*/
void wprm_wlan_power_supply(POWER_IC_PERIPH_ONOFF_T onoff);


/*---------------------------------- Local Function --------------------------*/
/* ATLAS REGISTERS defines */
#define ATLAS_START_CHANGE_BIT_9       9
#define ATLAS_START_CHANGE_BIT_23     23
#define ATLAS_START_CHANGE_BIT_21     21
#define ATLAS_START_CHANGE_BIT_3       3
#define ATLAS_START_CHANGE_BIT_15     15
#define ATLAS_START_CHANGE_BIT_4       4

#define NB_BITS_TO_CHANGE_3            3
#define NB_BITS_TO_CHANGE_1            1
#define NB_BITS_TO_CHANGE_2            2

#define VOLTAGE_3_0V_VAL_7             7
#define VOLTAGE_1_8V_VAL_0             0
#define VOLTAGE_1_8V_VAL_3             3


#define SET_ATLAS_BIT_TO_1             1 
#define SET_ATLAS_BIT_TO_0             0 


#ifdef  CONFIG_MOT_FEAT_BT_WLAN_SINGLE_ANTENNA
/* Definitions for controlling the antenna switch controlled by wlan device */
/* For BT/Wlan single antenna solution */
#define ANTENNA_REQUIRED               1
#define ANTENNA_NOT_REQUIRED           0

/* antenna_status 
 * Description:
 *     Checks whether other device is using the antenna. For BT/WLAN single
 *     antenna solutions, the antenna switch must be kept in default state
 * Parameters:
 *     None 
 * Return Value:
 *     1 - Antenna is being used by Bluetooth
 *     0 - Antenna is not used by Bluetooth
 */
static int antenna_status(void)
{
    /* If the BT device is enabled it might need the switch supply to be kept on */
    if(gpio_bluetooth_power_get_data() == GPIO_DATA_HIGH)
    {
        /* BT device enabled. 
          This solutions impacts current SW at a minimum. 
          This might leave the antenna switch supply on for
          FM, but this is drawing very little power (<20uA for 88W8686) */
        return(ANTENNA_REQUIRED);
    }
    return ANTENNA_NOT_REQUIRED;
}
#endif /* CONFIG_MOT_FEAT_BT_WLAN_SINGLE_ANTENNA */


/* wprm_wlan_power_supply --- WLAN module power supply turn on/off
 * Description:
 *     WLAN module power supply turn on/off.
 * Parameters:
 *     POWER_IC_PERIPH_ONOFF_T onoff =: 
 *         POWER_IC_PERIPH_OFF - turn off
 *         POWER_IC_PERIPH_ON  - turn on
 * Return Value:
 *     None.
 */
void wprm_wlan_power_supply(POWER_IC_PERIPH_ONOFF_T onoff)
{
    if (POWER_IC_PERIPH_ON == onoff) 
    {
        /* If phone is a winboard P3A or older (Marco is P3A close phone < P3AW)*/
        if((boardrev() == BOARDREV_P3AW) || (boardrev() == BOARDREV_P2BW)) {

            /* VMMC2 regulator setting*/
            power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_SET_1, ATLAS_START_CHANGE_BIT_9,
                               VOLTAGE_3_0V_VAL_7, NB_BITS_TO_CHANGE_3);
                   
            /* VMMC2ESIMEN regulator setting*/
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REGEN_ASSIGN, ATLAS_START_CHANGE_BIT_23,
                                SET_ATLAS_BIT_TO_0, NB_BITS_TO_CHANGE_1);
                    
            /* Set VMMC2EN bit to 1 to power on VMMC2 regulator */
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_1, ATLAS_START_CHANGE_BIT_21,
                                SET_ATLAS_BIT_TO_1, NB_BITS_TO_CHANGE_1);
        }
        else
        {
            /* VMMC2ESIMEN regulator setting*/
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REGEN_ASSIGN, ATLAS_START_CHANGE_BIT_23,
                                SET_ATLAS_BIT_TO_0, NB_BITS_TO_CHANGE_1);
                    
            /* VESIMESIMEN regulator setting*/
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REGEN_ASSIGN, ATLAS_START_CHANGE_BIT_21,
                                SET_ATLAS_BIT_TO_0, NB_BITS_TO_CHANGE_1);
                    
            /* VMMC2 regulator setting*/
            power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_SET_1, ATLAS_START_CHANGE_BIT_9,
                               VOLTAGE_3_0V_VAL_7, NB_BITS_TO_CHANGE_3);

            /* VDIG regulator setting*/
            power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_SET_0, ATLAS_START_CHANGE_BIT_4,
                               VOLTAGE_1_8V_VAL_3, NB_BITS_TO_CHANGE_2);
         
            /* VESIM regulator setting*/
            power_ic_set_reg_value(POWER_IC_REG_ATLAS_REG_SET_0, ATLAS_START_CHANGE_BIT_15,
                               VOLTAGE_1_8V_VAL_0, NB_BITS_TO_CHANGE_1);
                   
            /* Set VMMC2EN bit to 1 to power on VMMC2 regulator */
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_1 , ATLAS_START_CHANGE_BIT_21,
                               SET_ATLAS_BIT_TO_1, NB_BITS_TO_CHANGE_1);
                   
            /* Set VDIGEN bit to 1 to power on VDIG regulator */
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_0 , ATLAS_START_CHANGE_BIT_9,
                               SET_ATLAS_BIT_TO_1 , NB_BITS_TO_CHANGE_1);

            /* Set VESIMEN bit to 1 to power on VESIM regulator */
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_1 , ATLAS_START_CHANGE_BIT_3,
                               SET_ATLAS_BIT_TO_1 , NB_BITS_TO_CHANGE_1);
                   
        }
    }
    else
    {		    
        /* If phone is a winboard P3A or older (Marco is P3A close phone < P3AW)*/
        if((boardrev() == BOARDREV_P3AW) || (boardrev() == BOARDREV_P2BW)) {

            /* Set VMMC2EN bit to 0 to power off VMMC2 regulator */
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_1, ATLAS_START_CHANGE_BIT_21 ,
                                SET_ATLAS_BIT_TO_0, NB_BITS_TO_CHANGE_1);
        }
        else
        {

            /* Set VESIMEN bit to 0 to power off VESIM regulator */
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_1 , ATLAS_START_CHANGE_BIT_3,
                               SET_ATLAS_BIT_TO_0 , NB_BITS_TO_CHANGE_1);
                   
            /* Set VDIGEN bit to 0 to power off VDIG regulator */
            power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_0 , ATLAS_START_CHANGE_BIT_9,
                               SET_ATLAS_BIT_TO_0 , NB_BITS_TO_CHANGE_1);

            /* Set VMMC2EN bit to 0 to power off VMMC2 regulator */
#ifdef CONFIG_MOT_FEAT_BT_WLAN_SINGLE_ANTENNA
            /* Set VMMC2EN bit to 0 if Bluetooth is OFF (for BT/WLAN single antenna solutions especially) */
            if(antenna_status() == ANTENNA_NOT_REQUIRED)  
#endif /* CONFIG_MOT_FEAT_BT_WLAN_SINGLE_ANTENNA */
            {
                power_ic_set_reg_value( POWER_IC_REG_ATLAS_REG_MODE_1 , ATLAS_START_CHANGE_BIT_21,
                                  SET_ATLAS_BIT_TO_0, NB_BITS_TO_CHANGE_1);
            }
                            
        }
    }
    return;
}


/*---------------------------------- APIs ------------------------------------*/
/* wprm_wlan_power_on --- WLAN module power on
 * Description:
 *     WLAN module powers on.
 * Parameters:
 *     None.
 * Return Value:
 *     None.
 */
void sdio_wprm_wlan_power_on(void){
    
    /* Set WLAN_RESET GPIO to low and WLAN_PWR_DWN_B to high*/
    gpio_wlan_reset_set_data(GPIO_DATA_LOW);
    gpio_wlan_powerdown_set_data(GPIO_DATA_HIGH);
    
    /* WLAN module power supply turn on */
    wprm_wlan_power_supply(POWER_IC_PERIPH_ON);
    
    /* Wait for 20ms and set WLAN_RESET pin to high */
    mdelay(20); 
    gpio_wlan_reset_set_data(GPIO_DATA_HIGH);
     
    return;
}


/* wprm_wlan_power_off --- WLAN module power off
 * Description:
 *     WLAN module powers off.
 * Parameters:
 *     None.
 * Return Value:
 *     None.
 */
void sdio_wprm_wlan_power_off(void){

    /* WLAN module GPIO clean up when exit */  
    gpio_sdhc_inactive(SDHC2_MODULE);
    
    /* WLAN module power supply turn off */
    wprm_wlan_power_supply(POWER_IC_PERIPH_OFF);

    return;
}

