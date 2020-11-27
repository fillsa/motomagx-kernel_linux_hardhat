/*
 * Debugging macro include header
 *
 *  Copyright 2004-2006 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

        .macro  addruart,rx
                mrc     p15, 0, \rx, c1, c0
                tst     \rx, #1                 /* MMU enabled? */
#ifdef CONFIG_SERIAL_8250_CONSOLE
                ldreq   \rx, =CS4_BASE_ADDR + 0x10000 /* Physical Addr of EXT UART A */
                ldrne   \rx, =CS4_IO_ADDRESS(CS4_BASE_ADDR + 0x10000) /* Virtual Addr */
#else
                ldreq   \rx, =MXC_LL_UART_PADDR /* Physical Addr of MXC UART */
                ldrne   \rx, =MXC_LL_UART_VADDR /* Virtual Addr */
#endif

        .endm

        .macro  senduart,rd,rx
#ifdef CONFIG_SERIAL_8250_CONSOLE
#ifdef MXC_LL_EXT_UART_16BIT_BUS
                strh    \rd, [\rx]
#else
                strb    \rd, [\rx]
#endif
#else
                strh    \rd, [\rx, #0x40]
#endif
        .endm

        .macro  busyuart,rd,rx
        .endm

        .macro  waituart,rd,rx
#ifdef CONFIG_SERIAL_8250_CONSOLE
#ifdef MXC_LL_EXT_UART_16BIT_BUS
1001:           ldrh    \rd, [\rx, #0xA]        /* LSR_TEMPT ? */
#else
1001:           ldrb    \rd, [\rx, #0x5]        /* LSR_TEMPT ? */
#endif
                tst     \rd, #0x40
#else
1001:           ldrh    \rd, [\rx, #0x98]       /* TXFE ? */
                tst     \rd, #0x4000
#endif
                beq     1001b
        .endm
