/*
 * ks_otp_api.h - Kernel space OTP API definitions
 *
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * 24-Apr-2006  Motorola        Added SCS1 interface.
 * 27-Apr-2007  Motorola        Fix the problem that suomiMixer crash when BT device is reconnected.
 * 01-Aug-2007  Motorola        Add comments for oss compliance.
 * 13-Aug-2007  Motorola        Add comments.
 */


#ifndef  _KS_OTP_API_H_
#define  _KS_OTP_API_H_

/* 
 * ks_otp provides kernel space access to certain OTP data fields that are
 * required by other kernel modules.
 */

/* Defines for the security mode */
#define MOT_SECURITY_MODE_ENGINEERING (0x01)
#define MOT_SECURITY_MODE_PRODUCTION  (0x02)
#define MOT_SECURITY_MODE_NO_SECURITY (0x08)

/* Definitions for the Bound Signature state  */
#define BS_DIS_ENABLED                (0)
#define BS_DIS_DISABLED               (1)

/* Definitions for the Product State */
#define PRE_ACCEPTANCE_ACCEPTANCE     (0)
#define LAUNCH_SHIP                   (1)

/* Return the unique id length in bytes */
extern int mot_otp_get_uniqueid_length(void);

/* Copy the unique id into the user supplied buffer */
extern int mot_otp_get_uniqueid(unsigned char *buf, int *err);

/* Copy the security mode into the user supplied buffer */
extern int mot_otp_get_mode(unsigned char *security_mode, int *err);

/* Copy the bound signature disable state into the user supplied buffer */
extern int mot_otp_get_boundsignaturestate(unsigned char *bs_dis, int *err);
 
/* Copy the product state into the user supplied buffer */
extern int mot_otp_get_productstate(unsigned char *pr_st, int *err);

/* Get the value of the SCS1 register */
extern int mot_otp_get_scs1(unsigned char *scs1, int *err);

#endif  /*_KS_OTP_API_H_*/
