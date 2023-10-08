/*
 *  linux/include/asm-arm/boardrev.h
 *
 * Copyright 2006-2007 Motorola, Inc.
 *
 * This file contains board rev information.
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
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Initial version.  Added support for managing and 
 *                    querying board revision information.
 * 06/2007  Motorola  Include the file config.h.
 */

#ifndef __ASM_ARM_BOARDREV_H
#define __ASM_ARM_BOARDREV_H

#include <linux/config.h>

#ifdef CONFIG_MOT_FEAT_BRDREV

/* This macro generates the board revision name.  */
#define BRMACRONAME(brdrevname)       BOARDREV_##brdrevname

enum { BRMACRONAME(UNKNOWN) = 0x00000000 };


#ifdef __ARCH_ARM_BOARDREV_C

struct brdrevinfo
{
        char *  br_name;        /* board revision name */
        u32     br_namelen;     /* board revision name length */
        u32     br_val;         /* board revision numeric value */
};

#define BOARD_REV(name,val)     { #name, sizeof(#name), (val) },
#define LAST_REV                { 0, 0, 0 }

static struct brdrevinfo brdrevs[] __initdata = {

#else  /* ! __ARCH_ARM_BOARDREV_C */

#define BOARD_REV(name,val)     BRMACRONAME(name) = (val),
#define LAST_REV                BRMACRONAME(PLACEHOLDER) /* for compiler */

enum {

#endif /* __ARCH_ARM_BOARDREV_C */

/* ***************************************************************************
 *
 * NOTICE: Be careful when using boardrev() to test for a range of values.
 *
 * For instance, this test for P5A closed-phones will also evaluate to
 * true for P5A wingboards:
 * 
 *   if( ( boardrev() >= BOARD_REV_P5A ) && ( boardrev() < BOARD_REV_P6A ) ) {
 *       ...
 *   }
 *
 * It is generally safe to numerically compare board revisions within a
 * particular series of the same style of board. For insance boardrev()
 * for a P5B wingboard (BOARD_REV_P5BW) will always return a value less
 * than what it would return for a P5D wingboard (BOARD_REV_P5DW).
 *
 * However, the current convention is that boardrev() will always return
 * a higher value for any wingboard with the same P# name as a closed phone.
 * For instance boardrev() will return a numerically higher value for a P6A
 * wingboard (BOARD_REV_P6AW) than it will return for a P6H closed-phone
 * (BOARD_REV_P6H).
 *
 * ************************************************************************ */

#if defined(CONFIG_ARCH_MXC91231) || defined(CONFIG_ARCH_MXC91331) \
    || defined(CONFIG_ARCH_MXC91321)

    /* P0 Wingboards */
    BOARD_REV (P0A,     0x00000100)
    BOARD_REV (P0B,     0x00000110)
    BOARD_REV (P0C,     0x00000120)
    BOARD_REV (P0CPLUS, 0x00000128)
    BOARD_REV (P0D,     0x00000130)
    BOARD_REV (P0E,     0x00000140)
    BOARD_REV (P0F,     0x00000150)
    BOARD_REV (P0G,     0x00000160)
    BOARD_REV (P0H,     0x00000170)
    
    BOARD_REV (P0AW,    0x00000180)
    BOARD_REV (P0BW,    0x00000190)
    BOARD_REV (P0CW,    0x000001A0)
    BOARD_REV (P0DW,    0x000001B0)
    BOARD_REV (P0EW,    0x000001C0)
    BOARD_REV (P0FW,    0x000001D0)
    BOARD_REV (P0GW,    0x000001E0)
    BOARD_REV (P0HW,    0x000001F0)

    /* P1 Wingboards */
    BOARD_REV (P1A,     0x00000200)
    BOARD_REV (P1B,     0x00000210)
    BOARD_REV (P1BPLUS, 0x00000218)
    BOARD_REV (P1C,     0x00000220)
    BOARD_REV (P1D,     0x00000230)
    BOARD_REV (P1E,     0x00000240)
    BOARD_REV (P1F,     0x00000250)
    BOARD_REV (P1G,     0x00000260)
    BOARD_REV (P1H,     0x00000270)

    BOARD_REV (P1AW,    0x00000280)
    BOARD_REV (P1BW,    0x00000290)
    BOARD_REV (P1CW,    0x000002A0)
    BOARD_REV (P1DW,    0x000002B0)
    BOARD_REV (P1EW,    0x000002C0)
    BOARD_REV (P1FW,    0x000002D0)
    BOARD_REV (P1GW,    0x000002E0)
    BOARD_REV (P1HW,    0x000002F0)

    /* P2 Closed Phones */
    BOARD_REV (P2A,     0x00000300)
    BOARD_REV (P2B,     0x00000310)
    BOARD_REV (P2C,     0x00000320)
    BOARD_REV (P2D,     0x00000330)
    BOARD_REV (P2E,     0x00000340)
    BOARD_REV (P2F,     0x00000350)
    BOARD_REV (P2G,     0x00000360)
    BOARD_REV (P2H,     0x00000370)

    /* P2 Wingboard */
    BOARD_REV (P2AW,    0x00000380)
    BOARD_REV (P2BW,    0x00000390)
    BOARD_REV (P2CW,    0x000003A0)
    BOARD_REV (P2DW,    0x000003B0)
    BOARD_REV (P2EW,    0x000003C0)
    BOARD_REV (P2FW,    0x000003D0)
    BOARD_REV (P2GW,    0x000003E0)
    BOARD_REV (P2HW,    0x000003F0)

    /* P3 Closed Phones (except on Bute: P3 Brassboard) */
    BOARD_REV (P3A,     0x00000400)
    BOARD_REV (P3B,     0x00000410)
    BOARD_REV (P3C,     0x00000420)
    BOARD_REV (P3D,     0x00000430)
    BOARD_REV (P3E,     0x00000440)
    BOARD_REV (P3F,     0x00000450)
    BOARD_REV (P3G,     0x00000460)
    BOARD_REV (P3H,     0x00000470)

    /* P3 Wingboards */
    BOARD_REV (P3AW,    0x00000480)
    BOARD_REV (P3BW,    0x00000490)
    BOARD_REV (P3CW,    0x000004A0)
    BOARD_REV (P3DW,    0x000004B0)
    BOARD_REV (P3EW,    0x000004C0)
    BOARD_REV (P3FW,    0x000004D0)
    BOARD_REV (P3GW,    0x000004E0)
    BOARD_REV (P3HW,    0x000004F0)

    /* P4 Closed Phones */
    BOARD_REV (P4A,     0x00000500)
    BOARD_REV (P4B,     0x00000510)
    BOARD_REV (P4C,     0x00000520)
    BOARD_REV (P4D,     0x00000530)
    BOARD_REV (P4E,     0x00000540)
    BOARD_REV (P4F,     0x00000550)
    BOARD_REV (P4G,     0x00000560)
    BOARD_REV (P4H,     0x00000570)

    /* P4 Wingboards */
    BOARD_REV (P4AW,    0x00000580)
    BOARD_REV (P4BW,    0x00000590)
    BOARD_REV (P4CW,    0x000005A0)
    BOARD_REV (P4DW,    0x000005B0)
    BOARD_REV (P4EW,    0x000005C0)
    BOARD_REV (P4FW,    0x000005D0)
    BOARD_REV (P4GW,    0x000005E0)
    BOARD_REV (P4HW,    0x000005F0)

    /* P5 Closed Phones */
    BOARD_REV (P5A,     0x00000600)
    BOARD_REV (P5B,     0x00000610)
    BOARD_REV (P5C,     0x00000620)
    BOARD_REV (P5D,     0x00000630)
    BOARD_REV (P5E,     0x00000640)
    BOARD_REV (P5F,     0x00000650)
    BOARD_REV (P5G,     0x00000660)
    BOARD_REV (P5H,     0x00000670)

    /* P5 Wingboards */
    BOARD_REV (P5AW,    0x00000680)
    BOARD_REV (P5BW,    0x00000690)
    BOARD_REV (P5CW,    0x000006A0)
    BOARD_REV (P5DW,    0x000006B0)
    BOARD_REV (P5EW,    0x000006C0)
    BOARD_REV (P5FW,    0x000006D0)
    BOARD_REV (P5GW,    0x000006E0)
    BOARD_REV (P5HW,    0x000006F0)

    /* P6 Closed Phones */
    BOARD_REV (P6A,     0x00000700)
    BOARD_REV (P6B,     0x00000710)
    BOARD_REV (P6C,     0x00000720)
    BOARD_REV (P6D,     0x00000730)
    BOARD_REV (P6E,     0x00000740)
    BOARD_REV (P6F,     0x00000750)
    BOARD_REV (P6G,     0x00000760)
    BOARD_REV (P6H,     0x00000770)

    /* P6 Wingboards */
    BOARD_REV (P6AW,    0x00000780)
    BOARD_REV (P6BW,    0x00000790)
    BOARD_REV (P6CW,    0x000007A0)
    BOARD_REV (P6DW,    0x000007B0)
    BOARD_REV (P6EW,    0x000007C0)
    BOARD_REV (P6FW,    0x000007D0)
    BOARD_REV (P6GW,    0x000007E0)
    BOARD_REV (P6HW,    0x000007F0)

    /* P7 Closed Phones */
    BOARD_REV (P7A,     0x00000800)
    BOARD_REV (P7B,     0x00000810)
    BOARD_REV (P7C,     0x00000820)
    BOARD_REV (P7D,     0x00000830)
    BOARD_REV (P7E,     0x00000840)
    BOARD_REV (P7F,     0x00000850)
    BOARD_REV (P7G,     0x00000860)
    BOARD_REV (P7H,     0x00000870)

    /* P7 Wingboards */
    BOARD_REV (P7AW,    0x00000880)
    BOARD_REV (P7BW,    0x00000890)
    BOARD_REV (P7CW,    0x000008A0)
    BOARD_REV (P7DW,    0x000008B0)
    BOARD_REV (P7EW,    0x000008C0)
    BOARD_REV (P7FW,    0x000008D0)
    BOARD_REV (P7GW,    0x000008E0)
    BOARD_REV (P7HW,    0x000008F0)

#endif

    LAST_REV            /* must be the last board revision */

};    /* closes either the brdrevs definition or the enum */

/* boardrev() returns the board revision number.  */
extern u32  boardrev(void);

#endif  /* CONFIG_MOT_FEAT_BRDREV */

#endif  /* __ASM_ARM_BOARDREV_H */
