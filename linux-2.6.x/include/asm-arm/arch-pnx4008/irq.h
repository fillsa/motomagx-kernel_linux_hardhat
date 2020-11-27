/*
 * include/asm-arm/arch-pnx4008/irq.h
 *
 * PNX4008 IRQ controller driver - header file
 * this one is used in entry-arnv.S as well so it cannot contain C code
 *
 * Copyright (c) 2005 Philips Semiconductors
 * Copyright (c) 2005 MontaVista Software, Inc.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#define MIC_VA_BASE             IO_ADDRESS(PNX4008_INTCTRLMIC_BASE)
#define SIC1_VA_BASE            IO_ADDRESS(PNX4008_INTCTRLSIC1_BASE)
#define SIC2_VA_BASE            IO_ADDRESS(PNX4008_INTCTRLSIC2_BASE)

extern void __init PNX4008_init_irq(void);
