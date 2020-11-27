/*
 * capture_2mp_wrkarnd.h
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/*
 * Revision History:
 *
 *                    Modification  
 * Author                 Date          Description of Changes
 * ----------------   ------------     -------------------------
 * Motorola            08/07/2006      MAX #define's
 * Motoroola           08/07/2006      GEM #define's
 * Motorola            08/06/2006      Creation; RTIC #define's
 *
 *
 */

#ifndef __CAPTURE_2MP_WRKARND_H__
#define __CAPTURE_2MP_WRKARND_H__

#include <asm/arch/mxc91231.h>

/* SDRAM definitions */
#define ESDCTL_REG_BASE            (IO_ADDRESS(ESDCTL_BASE_ADDR))
#define ESDCTL0                    (ESDCTL_REG_BASE)
#define ESDCFG0                    (ESDCTL_REG_BASE + 0x04)
#define ESDMISC                    (ESDCTL_REG_BASE + 0x10)
#define CSD0_FAKE_SDRAM_START_ADDR 0x80000000

/* Fake memory definitions for RTIC */
#define RTIC_XFR_SIZE	      0x02000000
#define RTIC_SRC_ADDR	      CSD0_FAKE_SDRAM_START_ADDR

/* Fake memory definitions for GEM */
#define GEM_SRC_ADDR	   (CSD0_FAKE_SDRAM_START_ADDR + RTIC_XFR_SIZE)
#define GEM_XFR_SIZE 	   0x007FFFFF

//--------------
// GEM-AP defines
//--------------
/* The following 8 #define's are bit-field values for
 * the GEM control register GEM_CTL
 */
/* GEM_CTL[bit 21] = 0 for single read mode;
 * GEM_CTL[bit 21] = 1 for burst read mode
 */
#define GEM_BURST_RD_MODE  0x00200000
#define GEM_SINGLE_RD_MODE 0x00000000

/* GEM_CTL[bit 22] = 0 for single write mode;
 * GEM_CTL[bit 22] = 1 for burst write mode
 */
#define GEM_BURST_WR_MODE  0x00400000
#define GEM_SINGLE_WR_MODE 0x00000000

/* GEM_CTL[bit 7] = 0 for 16-bit access
 * GEM_CTL[bit 7] = 1 for 32-bit access
 */
#define GEM_RD_WR_SIZE_16  0x00000000
#define GEM_RD_WR_SIZE_32  0x00000080

#define GEM_STRT_BIT       0x00001000
#define GEM_STOP_BIT       0x00000200

//#########################################
//# GEM_AP  Extra module in SCM-A11       #
//#########################################

#define CRM_AP_REG_BASE    (IO_ADDRESS(CRM_AP_BASE_ADDR))
#define CRM_AP_ALMPMRG	   (CRM_AP_REG_BASE + 0x34)  // Clock gating register including GEMK_AP

#define	MASK_CLOCKS_GEMK_AP_ALWAYS_ON 	0x00000020
#define	MASK_CLOCKS_RTIC_AP_ALWAYS_ON 	0x00000004	

#define GEM_AP_REG_BASE        (IO_ADDRESS(GEMK_BASE_ADDR))

#define GEM_CTL                (GEM_AP_REG_BASE)      	  //Control Register
#define GEM_STAT               (GEM_AP_REG_BASE + 0x0004) //Status Register
#define GEM_KC1                (GEM_AP_REG_BASE + 0x0008) //GPRS Kc1 Register
#define GEM_KC0                (GEM_AP_REG_BASE + 0x000c) //GPRS Kc0 Register
#define GEM_INPUT              (GEM_AP_REG_BASE + 0x0010) //GPRS Input Register
#define GEM_READ_FRAME_START   (GEM_AP_REG_BASE + 0x0014) //Read Frame Start Register
#define GEM_HEADER_LENGTH      (GEM_AP_REG_BASE + 0x0018) //GPRS Header Length Register
#define GEM_FRAME_LENGTH       (GEM_AP_REG_BASE + 0x001c) //Frame Length Register
#define GEM_ACCESS_DELAY       (GEM_AP_REG_BASE + 0x0020) //Access Delay Register
#define GEM_CTL2               (GEM_AP_REG_BASE + 0x0024) //Control 2 Register
#define GEM_IMAC1              (GEM_AP_REG_BASE + 0x0028) //3GPP f9 IMAC1 Register
#define GEM_IMAC0              (GEM_AP_REG_BASE + 0x002c) //3GPP f9 IMAC0 Register
#define GEM_CK_IK3             (GEM_AP_REG_BASE + 0x0030) //KGCORE Key3 Register
#define GEM_CK_IK2             (GEM_AP_REG_BASE + 0x0034) //KGCORE Key2 Register
#define GEM_CK_IK1             (GEM_AP_REG_BASE + 0x0038) //KGCORE Key1 Register
#define GEM_CK_IK0             (GEM_AP_REG_BASE + 0x003c) //KGCORE Key0 Register
#define GEM_WRITE_FRAME_START  (GEM_AP_REG_BASE + 0x0040) //Write Frame Start Register
#define GEM_KGCORE_IN1         (GEM_AP_REG_BASE + 0x004c) //KGCORE Input1 Register
#define GEM_KGCORE_IN0         (GEM_AP_REG_BASE + 0x0050) //KGCORE Input0 Register

/* End of GEMK workaround definitions*/


//--------------
// RTIC-AP defines
//--------------
#define RTIC_REG_BASE           (IO_ADDRESS(RTIC_BASE_ADDR))
#define RTIC_STAT_REG           (RTIC_REG_BASE+0x0000)
#define RTIC_CMD_REG            (RTIC_REG_BASE+0x0004)
#define RTIC_CTL_REG            (RTIC_REG_BASE+0x0008)
#define RTIC_DMA_THR            (RTIC_REG_BASE+0x000C)
#define RTIC_MEM_A_ADDR_1       (RTIC_REG_BASE+0x0010)
#define RTIC_MEM_A_LEN_1        (RTIC_REG_BASE+0x0014)
#define RTIC_MEM_A_ADDR_2       (RTIC_REG_BASE+0x0018)
#define RTIC_MEM_A_LEN_2        (RTIC_REG_BASE+0x001C)

#define RTIC_WDOG_TMR           (RTIC_REG_BASE+0x0094) 
#define MEM_A_HASH_RES1         (RTIC_REG_BASE+0x00A0) 
#define MEM_A_HASH_RES2         (RTIC_REG_BASE+0x00A4) 
#define MEM_A_HASH_RES3         (RTIC_REG_BASE+0x00A8) 
#define MEM_A_HASH_RES4         (RTIC_REG_BASE+0x00AC) 
#define MEM_A_HASH_RES5         (RTIC_REG_BASE+0x00B0) 

#define MEM_B_HASH_RES1         (RTIC_REG_BASE+0xC0) 
#define MEM_B_HASH_RES2         (RTIC_REG_BASE+0xC4) 
#define MEM_B_HASH_RES3         (RTIC_REG_BASE+0xC8) 
#define MEM_B_HASH_RES4         (RTIC_REG_BASE+0xCC) 
#define MEM_B_HASH_RES5         (RTIC_REG_BASE+0xD0)

#define MEM_C_HASH_RES1         (RTIC_REG_BASE+0xE0) 
#define MEM_C_HASH_RES2         (RTIC_REG_BASE+0xE4) 
#define MEM_C_HASH_RES3         (RTIC_REG_BASE+0xE8) 
#define MEM_C_HASH_RES4         (RTIC_REG_BASE+0xEC) 
#define MEM_C_HASH_RES5         (RTIC_REG_BASE+0xF0)

#define MEM_D_HASH_RES1         (RTIC_REG_BASE+0x100) 
#define MEM_D_HASH_RES2         (RTIC_REG_BASE+0x104) 
#define MEM_D_HASH_RES3         (RTIC_REG_BASE+0x108) 
#define MEM_D_HASH_RES4         (RTIC_REG_BASE+0x10C) 
#define MEM_D_HASH_RES5         (RTIC_REG_BASE+0x110)

#define RTIC_MEM_B_ADDR_1       (RTIC_REG_BASE + 0x030)
#define RTIC_MEM_B_LEN_1        (RTIC_REG_BASE + 0x034)
#define RTIC_MEM_B_ADDR_2       (RTIC_REG_BASE + 0x038)
#define RTIC_MEM_B_LEN_2        (RTIC_REG_BASE + 0x03c)
#define RTIC_MEM_C_ADDR_1       (RTIC_REG_BASE + 0x050)
#define RTIC_MEM_C_LEN_1        (RTIC_REG_BASE + 0x054)
#define RTIC_MEM_C_ADDR_2       (RTIC_REG_BASE + 0x058)
#define RTIC_MEM_C_LEN_2        (RTIC_REG_BASE + 0x05c)
#define RTIC_MEM_D_ADDR_1       (RTIC_REG_BASE + 0x070)
#define RTIC_MEM_D_LEN_1        (RTIC_REG_BASE + 0x074)
#define RTIC_MEM_D_ADDR_2       (RTIC_REG_BASE + 0x078)
#define RTIC_MEM_D_LEN_2        (RTIC_REG_BASE + 0x07c)

/* The following #define's are bit-field values for
 * the RTIC control register RTIC_CTL_REG
 */
#define RTIC_BURST_1_WORD       0x00000000
#define RTIC_BURST_2_WORD       0x00000002
#define RTIC_BURST_4_WORD       0x00000004
#define RTIC_BURST_8_WORD       0x00000006
#define RTIC_BURST_16_WORD      0x00000008
#define RTIC_EN_HASH1_MEMA      0x00000010
#define RTIC_EN_HASH1_MEMB      0x00000020
#define RTIC_EN_HASH1_MEMC      0x00000040
#define RTIC_EN_HASH1_MEMD      0x00000080
#define RTIC_EN_HASH1_ALL       0x000000F0

/* The following are bit-field definitions for
 * the RTIC command register RTIC_CMD_REG
 */
#define RTIC_CMD_HASH_ONCE      0x00000004
#define RTIC_CMD_SW_RST         0x00000002
#define RTIC_CMD_CLR_IRQ        0x00000001

/* The following is a bit-field definition for
 * the RTIC status register RTIC_STAT_REG
 */
#define RTIC_STAT_HASH_DONE     0x00000002

//#########################################
//# MAX                                   #
//#########################################

#define MAX_REG_BASE     (IO_ADDRESS(MAX_BASE_ADDR))

#define MAX_SLV0_BASE    (MAX_REG_BASE+0x000) // base location for slave 0
#define MAX_SLV1_BASE    (MAX_REG_BASE+0x100) // base location for slave 1

#define MAX_MST0_BASE    (MAX_REG_BASE+0x800) // base location for master 0
#define MAX_MST1_BASE    (MAX_REG_BASE+0x900) // base location for master 1

#define MAX_SLV0_MPR0    (MAX_SLV0_BASE+0x00) // 32bit max slv0 master priority reg
#define MAX_SLV0_AMPR0   (MAX_SLV0_BASE+0x04) // 32bit max slv0 alt priority reg
#define MAX_SLV0_SGPCR0  (MAX_SLV0_BASE+0x10) // 32bit max slv0 general ctrl reg
#define MAX_SLV0_ASGPCR0 (MAX_SLV0_BASE+0x14) // 32bit max slv0 alt generl ctrl reg

#define MAX_SLV1_MPR1    (MAX_SLV1_BASE+0x00) // 32bit max slv1 master priority reg
#define MAX_SLV1_AMPR1   (MAX_SLV1_BASE+0x04) // 32bit max slv1 alt priority reg
#define MAX_SLV1_SGPCR1  (MAX_SLV1_BASE+0x10) // 32bit max slv1 general ctrl reg
#define MAX_SLV1_ASGPCR1 (MAX_SLV1_BASE+0x14) // 32bit max slv1 alt generl ctrl reg

#define MAX_MST0_MGPCR0  (MAX_MST0_BASE+0x00) // 32bit max mast0 general ctrl reg
#define MAX_MST1_MGPCR1  (MAX_MST1_BASE+0x00) // 32bit max mast1 general ctrl reg

#define CLKCTL_REG_BASE  (IO_ADDRESS(CLKCTL_BASE_ADDR))
#define CLKCTL_GP_CTRL   (CLKCTL_REG_BASE + 0x00)
#define CLKCTL_GP_SER    (CLKCTL_REG_BASE + 0x04)
#define CLKCTL_GP_CER    (CLKCTL_REG_BASE + 0x08)

extern void enable_gem_clock(void);
extern void enable_rtic_clock(void);
extern void enable_fake_sdram(void);
extern void gem_setup(void);
extern void rtic_setup(void);
extern void max_rtic_incr_setup(void);
extern void disable_gem_clock(void);
extern void disable_rtic_clock(void);
extern void disable_fake_sdram(void);
extern void max_slave0_disable(void);
extern void start_gem(void);
extern void start_rtic(void);
extern void stop_gem(void);
extern void stop_rtic(void);
extern void max_slave0_restore(void);
extern void restore_max_configuration(void);
#endif
