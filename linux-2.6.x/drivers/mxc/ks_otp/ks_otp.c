/*
 * ks_otp.c - Kernel space access to OTP functions.
 *
 * Copyright 2006-2007 Motorola, Inc.
 *
 */

/*
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 * 25-Apr-2007  Motorola        Add SCS1 interface.
 */


#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/ks_otp_api.h>


/* 
 * An ifdef is required for the type of OTP used. ZAS based products
 * use e-fuses.
 */
#if defined(CONFIG_MOT_FEAT_OTP_EFUSE)


/* Defines and external declarations for e-fuse based OTP */

#define HAB_TYPE_BANK                 (0x00)
#define HAB_TYPE_OFFSET               (0x10)
#define HAB_TYPE_LENGTH               (0x01)

#define UNIQUE_ID_BANK                (0x02)
#define UNIQUE_ID_OFFSET              (0x04)
#define UNIQUE_ID_LENGTH              (0x09)

#define BS_DIS_BANK                   (0x00)
#define BS_DIS_OFFSET                 (0x44)
#define BS_DIS_LENGTH                 (0x01)

#define PS_BANK                       (0x00)
#define PS_OFFSET                     (0x44)
#define PS_LENGTH                     (0x01)

/* HAB Type field values */
#define ENGINEERING_BIT_MASK          (0x01)
#define PRODUCTION_BIT_MASK           (0x02)
#define NO_SECURITY_BIT_MASK          (0x04)

#define ENGINEERING_BIT_COMPARE       (ENGINEERING_BIT_MASK)
#define PRODUCTION_BIT_COMPARE        (PRODUCTION_BIT_MASK)
#define NO_SECURITY_BIT_COMPARE       (NO_SECURITY_BIT_MASK)

/* Bound Signature Disable and Product State defines */
#define BOUND_SIGNATURE_MASK          (0x01)
#define PRODUCTION_MODE_MASK          (0x02)
#define PRODUCTION_MODE_SHIFT         (1)

/* Sets the user supplied error buffer if it exists */
#define SET_ERR(err, err_code) (((err) != NULL) ? (*(err) = (err_code)) : 0)

extern int efuse_read(int bank, int offset, int len, unsigned char *buf);

extern int efuse_get_scs1 (unsigned char *buf);


/* Get the length of the unuque id field */
int mot_otp_get_uniqueid_length(void)
{
    return UNIQUE_ID_LENGTH;
}

/* Get the unique id */
int mot_otp_get_uniqueid(unsigned char *buf, int *err)
{
    int ret;
    ret = efuse_read(UNIQUE_ID_BANK, UNIQUE_ID_OFFSET, UNIQUE_ID_LENGTH, buf);
    
    if (ret != 0)
    {
        SET_ERR(err, ret);
        return -1;
    }

    SET_ERR(err, 0);
    return 0;
}

/* Get the security mode */
int mot_otp_get_mode(unsigned char *security_mode, int *err)
{
    int ret;
    unsigned char mode;

    *security_mode = MOT_SECURITY_MODE_PRODUCTION;

    ret =  efuse_read(HAB_TYPE_BANK,HAB_TYPE_OFFSET,HAB_TYPE_LENGTH,&mode);
    if (ret == 0)
    {
        if ((mode & PRODUCTION_BIT_MASK) == PRODUCTION_BIT_COMPARE)
        {
            *security_mode = MOT_SECURITY_MODE_PRODUCTION;
        }
        else if ((mode & ENGINEERING_BIT_MASK) == ENGINEERING_BIT_COMPARE)
        {
            *security_mode = MOT_SECURITY_MODE_ENGINEERING;
        }
        else if ((mode & NO_SECURITY_BIT_MASK) == NO_SECURITY_BIT_COMPARE)
        {
            *security_mode = MOT_SECURITY_MODE_NO_SECURITY;
        }
        else
        {
            *security_mode = MOT_SECURITY_MODE_PRODUCTION;
        }
    }
    else
    {
        SET_ERR(err, ret);
        return -1;
    }

    SET_ERR(err, 0);
    return 0;
}

/* Get the state of the bound signature disable */
int mot_otp_get_boundsignaturestate(unsigned char *bs_dis, int *err)
{
    int ret;
    ret = efuse_read(BS_DIS_BANK, BS_DIS_OFFSET, BS_DIS_LENGTH, bs_dis);
    
    if (ret == 0)
    {
        *bs_dis = *bs_dis & BOUND_SIGNATURE_MASK;
    }
    else
    {
        *bs_dis = BS_DIS_DISABLED;
        SET_ERR(err, ret);
        return -1;
    }

    SET_ERR(err, 0);
    return 0;
}
 
/* Get the state of the product (Pre-Acceptance/Acceptance, or Launch/ship) */
int mot_otp_get_productstate(unsigned char *pr_st, int *err)
{
    int ret;
    ret = efuse_read(PS_BANK, PS_OFFSET, PS_LENGTH, pr_st);
    
    if (ret == 0)
    {
        *pr_st = (*pr_st & PRODUCTION_MODE_MASK) >> PRODUCTION_MODE_SHIFT;
    }
    else
    {
        *pr_st = LAUNCH_SHIP;
        SET_ERR(err, ret);
        return -1;
    }

    SET_ERR(err, 0);
    return 0;
}
 
/* Get the value of the SCS1 register */
int mot_otp_get_scs1(unsigned char *scs1, int *err)
{
    int ret;
    ret = efuse_get_scs1(scs1);
    if (ret != 0)
    {
        SET_ERR(err, ret);
        return -1;
    }
    SET_ERR(err, 0);
    return 0;

}

EXPORT_SYMBOL(mot_otp_get_uniqueid_length);
EXPORT_SYMBOL(mot_otp_get_uniqueid);
EXPORT_SYMBOL(mot_otp_get_mode);
EXPORT_SYMBOL(mot_otp_get_boundsignaturestate);
EXPORT_SYMBOL(mot_otp_get_productstate);
EXPORT_SYMBOL(mot_otp_get_scs1);

MODULE_AUTHOR ("Motorola");
MODULE_DESCRIPTION ("Motorola kernel space OTP kernel module");
MODULE_LICENSE ("GPL");

#endif /* CONFIG_MOT_FEAT_OTP_EFUSE */
