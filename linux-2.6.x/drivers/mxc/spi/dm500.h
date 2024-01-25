/*
 *  dm500.h - Header file for TI DM500 SPI Driver
 *
 *  Copyright (C) 2007-2008 Motorola, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  This header file contains dm500 SPI driver buffer and IOCTL definitions	
 */

/*
 *  DATE         AUTHOR     COMMENT
 *  ----         ------     -------
 *  05/30/2007   Motorola   Initial version
 *  12/06/2007   Motorola   Update DM500_BD to be align with OMX buffer type
 *  03/13/2008   Motorola   Add new IOCTL to set spi clock
 */

#ifndef DM500_H
#define DM500_H

#include <linux/ioctl.h>

typedef struct
{
   unsigned char *ubuf;// pointer to user buffer
   unsigned int len;   // number of bytes to transfer
   unsigned int addr;  // ARM Internal Memory address (not used for DM500_EXCHANGE)
} DM500_BD;            // DM500 buffer descriptor

typedef enum
{
  SPICLK_8MHz=0,
  SPICLK_16MHz
} DM500_SPICLKMODE;

#define DM500_IOC_MAGIC 'D'

#define DM500_READ_AIM  _IOR(DM500_IOC_MAGIC,  0, DM500_BD)
#define DM500_WRITE_AIM _IOW(DM500_IOC_MAGIC,  1, DM500_BD)
#define DM500_EXCHANGE  _IOWR(DM500_IOC_MAGIC, 2, DM500_BD)
#define DM500_SET_SPICLK  _IOWR(DM500_IOC_MAGIC, 3, DM500_SPICLKMODE)

#endif  /* DM500_H */
