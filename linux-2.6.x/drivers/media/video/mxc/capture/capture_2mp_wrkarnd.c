/*
 * capture_2mp_wrkarnd.c
 *
 * 2-megapixel image capture hardware workarounds for 
 * the Freescale Semiconductor MXC91231 platform with SDR memory
 *
 * Copyright (C) 2006 Motorola Inc.
 *
 * Author: Motorola Inc.
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
 */

/*
 * Revision History:
 *
 *                    Modification
 * Author                 Date        Description of Changes
 * ----------------   ------------   -------------------------
 * Motorola           08/07/2006     MAX slave 0 enable/disable
 * Motorola           08/07/2006     GEM dummy transfer
 * Motorola           08/06/2006     Creation; RTIC dummy xfr
 *
 *
 */

#include <linux/module.h>
#include <linux/config.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/delay.h>

#include "capture_2mp_wrkarnd.h"

static unsigned long max_slave0_setting;
static unsigned long max_slv1_mpr1_setting;
static unsigned long max_mst0_mgpcr0_setting;
static unsigned long max_mst1_mgpcr1_setting;
static unsigned long max_slv1_sgpcr1_setting;

/* MAX slave 0 workaround functions*/
void max_slave0_disable(void) /*do it when in capture*/
{
   max_slave0_setting = __raw_readl(CLKCTL_GP_CTRL) & 0x00000800;
   if(max_slave0_setting)
     {
       __raw_writel(max_slave0_setting, CLKCTL_GP_CER);
     }
}

void max_slave0_restore(void)
{
   if(max_slave0_setting)
     {
       __raw_writel(max_slave0_setting, CLKCTL_GP_SER);
     }
}

/* End of MAX slave 0 workaround functions*/

/* CSD0 fake SDRAM workaround functions*/
void disable_fake_sdram(void)
{
  /*in case there is a current drain issue*/
}
void enable_fake_sdram(void)
{
  /*in case there is a current drain issue*/
}
/* End of CSD0 fake SDRAM workaround functions*/

/* GEMK workaround function*/

void start_gem(void)
{
   uint32_t reg;
	
   reg = __raw_readl(GEM_CTL);
   __raw_writel(reg | GEM_STRT_BIT, GEM_CTL);  /* start bit, access size=16bits, DIR=1, E=1 */
}

void stop_gem (void)
{
   uint32_t reg;
   
   reg = __raw_readl(GEM_CTL);
   __raw_writel(reg | GEM_STOP_BIT, GEM_CTL);  /* stop bit, access size=16bits, DIR=1, E=1 */
}
void enable_gem_clock(void)
{
   uint32_t reg;
   
   reg = __raw_readl(CRM_AP_ALMPMRG);
   __raw_writel(reg | MASK_CLOCKS_GEMK_AP_ALWAYS_ON, CRM_AP_ALMPMRG); /* GEMK Clock always on */
}

void disable_gem_clock(void)
{
   uint32_t reg;
   
   reg = __raw_readl(CRM_AP_ALMPMRG);
   __raw_writel(reg & ~MASK_CLOCKS_GEMK_AP_ALWAYS_ON, CRM_AP_ALMPMRG); /* GEMK as previous to enable gem_clock */
}

void configure_gem (u32 rd_fm_start, u32 wr_fm_start, u32 fm_length, u32 burst_write, u32 burst_read, u32 rd_wr_size)
{
	//----------------------------------------    
    	// Get GEM ready to run for this frame  
    	//----------------------------------------    

    	__raw_writel(0x00000800, GEM_CTL);              // Do a software reset first 
    	__raw_writel(burst_write + burst_read + rd_wr_size + 0x00000008, GEM_CTL);
                                                   // set wr/rd size to 16 bits and privledge mode 
    	__raw_writel(0x00000070, GEM_CTL2);             //DMA mode, use read  pointer  for writing
    	__raw_writel(0xca23de75, GEM_KC1);              // set the Key #1 value 
    	__raw_writel(0x590bb1d3, GEM_KC0);              // set the Key #2 value 
    	__raw_writel(0x233e2ccc, GEM_INPUT);            // set the INPUT value 
    	// set the frame start address 
    	__raw_writel(rd_fm_start, GEM_READ_FRAME_START);
    	__raw_writel(wr_fm_start, GEM_WRITE_FRAME_START);
    	__raw_writel(0x00000005, GEM_HEADER_LENGTH);    // set the header length 
    	__raw_writel(fm_length, GEM_FRAME_LENGTH);     // set the frame length 
}

void gem_setup (void)
{
	configure_gem(GEM_SRC_ADDR,GEM_SRC_ADDR ,GEM_XFR_SIZE, GEM_SINGLE_WR_MODE, GEM_SINGLE_RD_MODE, GEM_RD_WR_SIZE_16);
}

/*End of GEMK workaround functions*/


/* Start of RTIC+MAX workaround function */

// to start RTIC read
void start_rtic(void)
{
  __raw_writel(RTIC_CMD_HASH_ONCE, RTIC_CMD_REG); // ENABLE RTIC
}

void stop_rtic(void)
{
  __raw_writel(RTIC_CMD_SW_RST, RTIC_CMD_REG); // STOP RTIC
}

void enable_rtic_clock(void)
{
  uint32_t reg;
  
  reg = __raw_readl(CRM_AP_ALMPMRG);
  __raw_writel(reg | MASK_CLOCKS_RTIC_AP_ALWAYS_ON, CRM_AP_ALMPMRG); /* RTIC Clock always on */
}

void disable_rtic_clock(void)
{
  uint32_t reg;
   
  reg = __raw_readl(CRM_AP_ALMPMRG);
  __raw_writel(reg & ~MASK_CLOCKS_RTIC_AP_ALWAYS_ON, CRM_AP_ALMPMRG);
}

// configuration function
void configure_rtic(u32 hash_once_mem_en, u32 burst_size, u32 mema_addr, u32 memb_addr, u32 memc_addr, u32 memd_addr, u32 mem_len) {

//----------------------------------------    
// Get rtic ready to run for this frame  
//----------------------------------------    

  //Give a sw reset
  __raw_writel(RTIC_CMD_SW_RST, RTIC_CMD_REG);
  asm volatile ("nop\n"
		"nop\n"
		"nop\n"
		"nop\n");

  //program other registers
  __raw_writel((hash_once_mem_en + burst_size), RTIC_CTL_REG); 
                                           //Enable Memory A, one-time hashing,
                                           //1 words burst, int disabled
  // set the frame start address 
  __raw_writel(mema_addr, RTIC_MEM_A_ADDR_1);
  __raw_writel(mem_len, RTIC_MEM_A_LEN_1);
  __raw_writel((mema_addr + 0x0020), RTIC_MEM_A_ADDR_2);
  __raw_writel(0x00000000, RTIC_MEM_A_LEN_2);

  //configure memb space
  __raw_writel(memb_addr, RTIC_MEM_B_ADDR_1);
  __raw_writel(mem_len, RTIC_MEM_B_LEN_1);
  __raw_writel((memb_addr + 0x0020), RTIC_MEM_B_ADDR_2);
  __raw_writel(0x00000000, RTIC_MEM_B_LEN_2);

  //configure memc space
  __raw_writel(memc_addr, RTIC_MEM_C_ADDR_1);
  __raw_writel(mem_len, RTIC_MEM_C_LEN_1);
  __raw_writel((memc_addr + 0x0020), RTIC_MEM_C_ADDR_2);
  __raw_writel(0x00000000, RTIC_MEM_C_LEN_2);

  //configure memd space
  __raw_writel(memd_addr, RTIC_MEM_D_ADDR_1);
  __raw_writel(mem_len, RTIC_MEM_D_LEN_1);
  __raw_writel((memd_addr + 0x0020), RTIC_MEM_D_ADDR_2);
  __raw_writel(0x00000000, RTIC_MEM_D_LEN_2);

  //set wdog timer of rtic
  __raw_writel(0x0000ffff, RTIC_WDOG_TMR);  //enough cycles before timeout error comes
  __raw_writel(0x00000000, RTIC_DMA_THR);  // fastest accesses from RTIC
}

// To setup RTIC
void rtic_setup(void)
{
  configure_rtic(RTIC_EN_HASH1_MEMA, RTIC_BURST_1_WORD, RTIC_SRC_ADDR, 0x00000000, 0x00000000, 0x00000000, RTIC_XFR_SIZE);
}

// Function to 
//      1. Give RTIC-AP highest priority on Max Slave Port S1
//      2. Allow arbirtation on M0 and M1 during INCR burst at any time.
//      3. Max Priority scheme fixed (default)
void max_rtic_incr_setup (void)
{
    // 0. Backup the current setting of the MAX for restoration later
       max_slv1_mpr1_setting =   __raw_readl(MAX_SLV1_MPR1) ;  
       max_mst0_mgpcr0_setting = __raw_readl(MAX_MST0_MGPCR0) ;
       max_mst1_mgpcr1_setting = __raw_readl(MAX_MST1_MGPCR1) ;
       max_slv1_sgpcr1_setting = __raw_readl(MAX_SLV1_SGPCR1) ;

    // 1. Give RTIC Highest priority
        __raw_writel (0x00540213, MAX_SLV1_MPR1); // default for M5, M4, M2, M1. Highest (0) for RTIC (M3) , 3 for M0
    
    // 2. configure AULB bits for ARM so that arbitration can happen during INCR burst
        __raw_writel(0x00000001, MAX_MST0_MGPCR0); // arbitration is allowed at any time
        __raw_writel(0x00000001, MAX_MST1_MGPCR1 ); // arbitration is allowed at any time

    // 3. Configure Max in Fixed priority scheme for Slave ports S1
        // make ARB = 00
        __raw_writel(__raw_readl(MAX_SLV1_SGPCR1) & 0xFFFFFEFF , MAX_SLV1_SGPCR1); // 
	__raw_writel(__raw_readl(MAX_SLV1_SGPCR1) & 0xFFFFFDFF , MAX_SLV1_SGPCR1); //
 
} //max_rtic_incr_setup 

// function to restore the MAX configuration

void restore_max_configuration(void)
{
    __raw_writel(max_slv1_mpr1_setting, MAX_SLV1_MPR1);
    __raw_writel(max_mst0_mgpcr0_setting, MAX_MST0_MGPCR0);
    __raw_writel(max_mst1_mgpcr1_setting, MAX_MST1_MGPCR1);
    __raw_writel(max_slv1_sgpcr1_setting, MAX_SLV1_SGPCR1);
} // restore_max_configuration 

/* End of RTIC+MAX workaround functions*/



MODULE_AUTHOR("Motorola Inc.");
MODULE_DESCRIPTION("2-megapixel image capture HW workarounds for Freescale Semiconductor MXC91231 platform with SDR memory");
MODULE_LICENSE("GPL");

EXPORT_SYMBOL(enable_fake_sdram);
EXPORT_SYMBOL(start_gem);
EXPORT_SYMBOL(start_rtic);
EXPORT_SYMBOL(max_slave0_restore);
EXPORT_SYMBOL(disable_fake_sdram);
EXPORT_SYMBOL(disable_rtic_clock);
EXPORT_SYMBOL(stop_rtic);
EXPORT_SYMBOL(disable_gem_clock);
EXPORT_SYMBOL(stop_gem);
EXPORT_SYMBOL(restore_max_configuration);
EXPORT_SYMBOL(max_rtic_incr_setup);
EXPORT_SYMBOL(rtic_setup);
EXPORT_SYMBOL(enable_rtic_clock);
EXPORT_SYMBOL(gem_setup);
EXPORT_SYMBOL(enable_gem_clock);
EXPORT_SYMBOL(max_slave0_disable);
