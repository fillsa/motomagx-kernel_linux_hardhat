/*
 * display_spi_inter.h - Header file for interface between display app
 * and SPI driver
 *
 * Copyright (C) 2007 Motorola, Inc.
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * DATE         AUTHOR     COMMENT
 * ----         ------     -------
 * 05/01/2007   Motorola   Initial version
 */
#ifndef _LINUX_SPI_DISPLAY_H_
#define _LINUX_SPI_DISPLAY_H_

#include <linux/ioctl.h>

#define SPI_DISPLAY_MAGIC_NUM 0xCF

struct spi_command_frame {
    char *data;
    int len;
};

#define SPI_COMMAND_WRITE _IOW(SPI_DISPLAY_MAGIC_NUM, 0x1, struct spi_command_frame *)

#endif
