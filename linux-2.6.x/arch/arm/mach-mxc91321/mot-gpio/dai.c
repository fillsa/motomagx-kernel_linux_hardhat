/*
 * linux/arch/arm/mach-mxc91321/mot-gpio/dai.c
 *
 * ArgonLV implementation of Digital Audio Interface GPIO control.
 *
 * Copyright 2006 Motorola, Inc.
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

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 01-Nov-2006  Motorola        Initial revision.
 */

#include "mot-gpio-argonlv.h"

#ifdef CONFIG_MOT_FEAT_GPIO_API_DAI
void gpio_dai_enable(void)
{
    /* Stub function to work around error in APAL driver. */
    return;
}


void gpio_dai_disable(void)
{
    /* Stub function to work around error in APAL driver. */
    return;
}
#endif /* CONFIG_MOT_FEAT_GPIO_API_DAI */

