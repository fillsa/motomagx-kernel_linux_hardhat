/*
 * linux/arch/arm/mach-pxa/bulverde_voltage.c
 *
 * Bulverde voltage change driver.
 *
 * Author: <source@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <asm/hardware.h>
#include <asm/arch/pxa-regs.h>

/* For ioremap */
#include <asm/io.h>

#include "bulverde_voltage.h"

#define CP15R0_REV_MASK		0x0000000f
#define PXA270_C5		0x7

static u32 chiprev;
static unsigned int blvd_min_vol, blvd_max_vol;
static int mvdt_size;

static volatile int *ramstart;

struct MvDAC {
	unsigned int mv;
	unsigned int DACIn;
} *mvDACtable;

/*
 *  Transfer desired mv to required DAC value.
 *  Vcore = 1.3v - ( 712uv * DACIn )
 */
static struct MvDAC table_c0[] = {
	{1425, 0},
	{1400, 69},
	{1300, 248},
	{1200, 428},
	{1100, 601},
	{1000, 777},
	{950, 872},
	{868, 1010},
	{861, 0xFFFFFFFF},
};

/*
 *  Transfer desired mv to required DAC value, update for new boards,
 *  according to "Intel PXA27x Processor Developer's Kit User's Guide,
 *  April 2004, Revision 4.001"
 *  Vcore = 1.5V - (587uV * DAC(input)).
 */
static struct MvDAC table_c5[] = {
	{1500, 0},
	{1484,25},
	{1471,50},
	{1456,75},
	{1441,100},
	{1427,125},
	{1412,150},
	{1397,175},
	{1383,200},
	{1368,225},
	{1353,250},
	{1339,275},
	{1323,300},
	{1309,325},
	{1294,350},
	{1280,375},
	{1265,400},
	{1251,425},
	{1236,450},
	{1221,475},
	{1207,500},
	{1192,525},
	{1177,550},
	{1162,575},
	{1148,600},
	{1133,625},
	{1118,650},
	{1104,675},
	{1089,700},
	{1074,725},
	{1060,750},
	{1045,775},
	{1030,800},
	{1016,825},
	{1001,850},
	{986,875},
	{972,900},
	{957,925},
	{942,950},
	{928,975},
	{913,1000},
	{899, 1023},
};

unsigned int bulverde_validate_voltage(unsigned int mv)
{
	/*
	 *      Just to check whether user specified mv
	 *      can be set to the CPU.
	 */
	if ((mv >= blvd_min_vol) && (mv <= blvd_max_vol))
		return mv;
	else
		return 0;
}

/*
 *	According to bulverde's manual, set the core's voltage.
 */
void bulverde_set_voltage(unsigned int mv)
{
	vm_setvoltage(mv2DAC(mv));
}

/*
 * Prepare for a voltage change, possibly coupled with a frequency
 * change
 */
static void dpm_power_change_cmd(unsigned int DACValue, int coupled);

void bulverde_prep_set_voltage(unsigned int mv)
{
	dpm_power_change_cmd(mv2DAC(mv), 1 /* coupled */ );
}

unsigned int mv2DAC(unsigned int mv)
{
	int i, num = mvdt_size;

	if (mvDACtable[0].mv <= mv) {	/* Max or bigger */
		/* Return the first one */
		return mvDACtable[0].DACIn;
	}

	if (mvDACtable[num - 1].mv >= mv) {	/* Min or smaller */
		/* Return the last one */
		return mvDACtable[num - 1].DACIn;
	}

	/* The biggest and smallest value cases are covered, now the
	   loop may skip those */
	for (i = 1; i <= (num - 1); i++) {
		if ((mvDACtable[i].mv >= mv) && (mvDACtable[i + 1].mv < mv)) {
			return mvDACtable[i].DACIn;
		}
	}

	/* Should never get here */
	return 0;
}

/*
 *	Functionality: Initialize PWR I2C. 
 *	Argument:      None
 *	Return:        void
*/
int bulverde_vcs_init(void)
{
	/* we distinguish new and old boards by proc chip 
	 * revision, we assume new boards have C5 proc 
	 * revision and we use the new table (table_c5) for them,
	 * for all other boards we use the old table (table_c0).
	 * Note, the logics won't work and inaccurate voltage 
	 * will be set if C5 proc installed to old board 
	 * and vice versa.
	 */

	asm("mrc%? p15, 0, %0, c0, c0" : "=r" (chiprev));

	chiprev &= CP15R0_REV_MASK;

	if ( chiprev == PXA270_C5 ) {
		mvDACtable = table_c5;
		mvdt_size = sizeof(table_c5) / sizeof(struct MvDAC);
		blvd_min_vol = BLVD_MIN_VOL_C5;
		blvd_max_vol = BLVD_MAX_VOL_C5;
	} else {
		mvDACtable = table_c0;
		mvdt_size = sizeof(table_c0) / sizeof(struct MvDAC);
		blvd_min_vol = BLVD_MIN_VOL_C0; 
		blvd_max_vol = BLVD_MAX_VOL_C0;
	}

	CKEN |= 0x1 << 15;
	CKEN |= 0x1 << 14;
	PCFR |= 0x60;

	/* map the first page of sdram to an uncached virtual page */
	ramstart = (int *)ioremap(PHYS_OFFSET, 4096);

	printk(KERN_INFO "CPU voltage change initialized.\n");

	return 0;
}

void bulverde_voltage_cleanup(void)
{
	/* unmap the page we used*/
	iounmap((void *)ramstart);
}


void bulverde_change_voltage(void)
{
	unsigned long flags;
	unsigned int unused;


	local_irq_save(flags);

	__asm__ __volatile__("\n\
    @ BV B0 WORKAROUND - Core hangs on voltage change at different\n\
    @ alignments and at different core clock frequencies\n\
    @ To ensure that no external fetches occur, we want to store the next\n\
    @ several instructions that occur after the voltage change inside\n\
    @ the cache. The load dependency stall near the retry label ensures \n\
    @ that any outstanding instruction cacheline loads are complete before \n\
    @ the mcr instruction is executed on the 2nd pass. This procedure \n\
    @ ensures us that the internal bus will not be busy. \n\
					\n\
    b	    2f				\n\
    nop					\n\
    .align  5				\n\
2:					\n\
    ldr     r0, [%1]			@ APB register read and compare \n\
    cmp     r0, #0			@ fence for pending slow apb reads \n\
					\n\
    mov     r0, #8			@ VC bit for PWRMODE \n\
    movs    r1, #1			@ don't execute mcr on 1st pass \n\
					\n\
    @ %1 points to uncacheable memory to force memory read \n\
					\n\
retry:					\n\
    ldreq   r3, [%2]			@ only stall on the 2nd pass\n\
    cmpeq   r3, #0			@ cmp causes fence on mem transfers\n\
    cmp     r1, #0			@ is this the 2nd pass? \n\
    mcreq   p14, 0, r0, c7, c0, 0	@ write to PWRMODE on 2nd pass only \n\
					\n\
    @ Read VC bit until it is 0, indicates that the VoltageChange is done.\n\
    @ On first pass, we never set the VC bit, so it will be clear already.\n\
					\n\
VoltageChange_loop:			\n\
    mrc     p14, 0, r3, c7, c0, 0	\n\
    tst     r3, #0x8			\n\
    bne     VoltageChange_loop		\n\
					\n\
    subs    r1, r1, #1		@ update conditional execution counter\n\
    beq     retry":"=&r"(unused)
			     :"r"(&CCCR), "r"(ramstart)
			     :"r0", "r1", "r3");

	local_irq_restore(flags);

}

static void clr_all_sqc(void)
{
	int i = 0;
	for (i = 0; i < 32; i++)
		PCMD(i) &= ~PCMD_SQC;
}

static void clr_all_mbc(void)
{
	int i = 0;
	for (i = 0; i < 32; i++)
		PCMD(i) &= ~PCMD_MBC;
}

static void clr_all_dce(void)
{
	int i = 0;
	for (i = 0; i < 32; i++)
		PCMD(i) &= ~PCMD_DCE;
}

static void set_mbc_bit(int ReadPointer, int NumOfBytes)
{
	PCMD0 |= PCMD_MBC;
	PCMD1 |= PCMD_MBC;
}

static void set_lc_bit(int ReadPointer, int NumOfBytes)
{
	PCMD0 |= PCMD_LC;
	PCMD1 |= PCMD_LC;
	PCMD2 |= PCMD_LC;
}

static void set_cmd_data(unsigned char *DataArray, int StartPoint, int size)
{
	PCMD0 &= 0xFFFFFF00;
	PCMD0 |= DataArray[0];
	PCMD1 &= 0xFFFFFF00;
	PCMD1 |= DataArray[1];
	PCMD2 &= 0xFFFFFF00;
	PCMD2 |= DataArray[2];
}

/* coupled indicates that this VCS is to be coupled with a FCS */
static void dpm_power_change_cmd(unsigned int DACValue, int coupled)
{
	unsigned char dataArray[3];

	dataArray[0] = 0;	/*      Command 0       */
	dataArray[1] = (DACValue & 0x000000FF);	/*      data LSB        */
	dataArray[2] = (DACValue & 0x0000FF00) >> 8;	/*      data MSB        */

	PVCR = 0;

	PCFR &= ~PCFR_FVC;
	PVCR &= 0xFFFFF07F;	/*      no delay is necessary   */
	PVCR &= 0xFFFFFF80;	/*  clear slave address         */
	PVCR |= 0x20;		/*      set slave address       */

	PVCR &= 0xFE0FFFFF;	/*      clear read pointer 0    */
	PVCR |= 0;

	/*  DCE and SQC are not necessary for single command */
	clr_all_sqc();
	clr_all_dce();

	clr_all_mbc();
	set_mbc_bit(0, 2);

	/*  indicate the last byte of this command is holded in this register */
	PCMD2 &= ~PCMD_MBC;

	/*  indicate this is the first command and last command also        */
	set_lc_bit(0, 3);

	/*  programming the command data bit        */
	set_cmd_data(dataArray, 0, 2);

	/* Enable Power I2C */
	PCFR |= PCFR_PI2CEN;

	if (coupled) {
		/* Enable Power I2C and FVC */
		PCFR |= PCFR_FVC;
	}
}

void vm_setvoltage(unsigned int DACValue)
{
	dpm_power_change_cmd(DACValue, 0 /* not-coupled */ );
	/* Execute voltage change sequence      */
	bulverde_change_voltage();	/* set VC on the PWRMODE on CP14 */
}
