/*
 * linux/include/asm-arm/arch-ixp4xx/timex.h
 * 
 */

#include <asm/hardware.h>

/*
 * We use IXP425 General purpose timer for our timer needs, it runs at 
 * 66.66... MHz
 */
#define CLOCK_TICK_RATE (66666666)

#ifndef __ASSEMBLY__

static inline cycles_t get_cycles(void)
{
	cycles_t cycles;					

	__asm__ __volatile__("mrc p14, 0, %0, c1, c1, 0" : "=r" (cycles));

	return cycles;
}
#endif

