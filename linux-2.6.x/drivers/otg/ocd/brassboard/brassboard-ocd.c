#if 0
// XXX This file is not used 
//
/*
 * otg/ocd/brassboard/brassboard-ocd.c -- USB Device Controller driver 
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@lbelcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/09/2006         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at
 * your option) any later version.  You should have
 * received a copy of the GNU General Public License
 * along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA
 *
 */
/*!
 * @file otg/ocd/brassboard/brassboard-ocd.c
 * @brief ZAS EVG USB OCT Controller Driver
 * This is the OTG Controller Driver. 
 *
 * This implements overall configuration and control of the Freescale OTG hardware
 * and implements the main interrupt handler.
 *
 * The GPT2 timer is used for the OTG timer.
 *
 * The GPT3 timer is used as a free running counter for a source of ticks
 * for use with the trace facility.
 *
 * @ingroup BRASSBOARD
 *
 * @{
 */

#include <otg/pcd-include.h>
#include <linux/pci.h>
#include <asm/arch/gpio.h>
#include <asm/arch/board.h>

#include <otghw/mxc-hardware.h>

struct ocd_ops ocd_ops;

extern irqreturn_t fs_pcd_int_hndlr (int irq, void *dev_id, struct pt_regs *regs);
extern irqreturn_t hcd_hw_int_hndlr(int irq, void *dev_id, struct pt_regs *regs);
extern int brassboard_tcd_mod_init(void);
extern void brassboard_tcd_mod_exit(void);

extern void fs_func_clock_on(void);
extern void fs_func_clock_off(void);
extern void fs_main_clock_on(void);
extern void fs_main_clock_off(void);

#define TIMEOUT_VALUE 1000

/* ********************************************************************************************* */
/*!
 * ocd_hnp_int_hndlr() - HNP interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t ocd_hnp_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        u32 hint_stat = fs_rl(OTG_CORE_HINT_STAT);

        /* get and clear interrupts
         */
        fs_wl_clr(OCD, OTG_CORE_HINT_STAT, hint_stat);

        return IRQ_HANDLED;
}

/*!
 * pcd_bwkup_int_hndlr() - wakeup interrupt
 * @param irq
 * @param dev_id
 * @param regs
 */
static irqreturn_t pcd_bwkup_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        u32 sys_ctrl = fs_rl(OTG_SYS_CTRL);
        //static int bwkup_count = 0;

        TRACE_MSG2(OCD, "SYS_CTRL: %08x CORE_CLK: %08x", sys_ctrl, fs_rl(OTG_CORE_CLK_CTRL));
        printk(KERN_INFO"%s: %08x CORE_CLK: %08x\n", __FUNCTION__, sys_ctrl, fs_rl(OTG_CORE_CLK_CTRL));

        //fs_wl(OTG_SYS_CTRL, sys_ctrl & ~0x0F000000);
        fs_main_clock_on();                          // turn on Main Clock
        fs_func_clock_on();                                    // turn on Function Clock

        //if (bwkup_count++ > 10)
        //        fs_wl(OTG_SYS_CTRL, 0);

        return IRQ_HANDLED;
}

/*!
 * ocd_dma_int_hndlr() - DMA interrupt
 * @param irq
 * @param dev_id
 * @param regs
 */
static irqreturn_t ocd_dma_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        u32 dint_stat = fs_rl(OTG_DMA_DINT_STAT);
        static u32 dma_interrupts = 0;
        dma_interrupts += 1;
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        if (dma_interrupts > 10) {
                TRACE_MSG1(OCD, "DMA INT #%08x disabling DMA error interrupts",dint_stat);
                fs_wl(OTG_DMA_DINT_STEN, 0x0);
        } else {
                //TRACE_MSG1(OCD, "DMA INT #%08x",dint_stat);
                if (dint_stat & 0x1) {  // FIXME - find the #def for 0x1
                        // HCD DMA error
                        hcd_hw_int_hndlr(irq, NULL, regs);
                }
                fs_wl(OTG_DMA_DINT_STAT,fs_rl(OTG_DMA_DINT_STAT));
        }
        return IRQ_HANDLED;
}

/*!
 * hcd_host_int_hndlr() - Host interrupt
 * @param irq
 * @param dev_id
 * @param regs
 */
static irqreturn_t hcd_host_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        u32 sint_stat = fs_rl(OTG_HOST_SINT_STAT);
        static u32 hcd_interrupts = 0;
        //printk(KERN_INFO"%s: %d\n", __FUNCTION__, hcd_interrupts++);
        hcd_hw_int_hndlr(irq, NULL, regs);
        return IRQ_HANDLED;
}

/*!
 * ocd_ctrl_int_hndlr - Control and I2C interrupt
 * Process USBOTG related interrupts.
 * @param irq
 * @param dev_id
 * @param regs
 */
static irqreturn_t ocd_ctrl_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        u32 sys_ctrl = fs_rl(OTG_SYS_CTRL);            // C.f. 23.8 USB Control Register
        u32 cint_stat = fs_rl(OTG_CORE_CINT_STAT);     // C.f. 23.9.2 USBOTG Module Interrupt Status Register
        u32 hint_stat = fs_rl(OTG_CORE_HINT_STAT);     // C.f. 23.9.12 HNP Interrupt Status Register

        static u32 ocd_interrupts = 0;

        int handled = 0;

        TRACE_MSG4(OCD, "CLK: %08x sys: %08x cint: %08x hint: %08x", fs_rl(OTG_CORE_CLK_CTRL), sys_ctrl, cint_stat, hint_stat);

        /* wakeup interrupts are signalled when clocks are off
         */
        //if (sys_ctrl & (SYS_CTRL_I2C_WU_INT_STAT | SYS_CTRL_OTG_WU_INT_STAT)) 
        //        brassboard_i2c_hndlr(irq, dev_id, regs);
        
        if (sys_ctrl & (SYS_CTRL_HOST_WU_INT_STAT | SYS_CTRL_FNT_WU_INT_STAT)) 
                TRACE_MSG1(OCD, "SYS_CTRL: HOST/FUNC %08x", sys_ctrl);
        

        /* N.B. OTG_CORE_CINT_STAT interrupts are self clearing when interrupt
         * sources have been cleared. 
         */
        UNLESS(cint_stat || hint_stat) {
                return IRQ_HANDLED;
        }

        if (hint_stat) {
                //if (hint_stat & HNP_I2COTGINT) 
                //        TRACE_MSG1(OCD, "OTG_CORE_HINT_STAT HNP_I2COTGINT %08x ignored (periodic?)", hint_stat);
                
                fs_wl(OTG_CORE_HINT_STAT, hint_stat);
        }

        if (cint_stat & MODULE_ASHNPINT) {                              // asynchronous HNP interrupt, enable Main clock
                u32 hnp_cstat = fs_rl(OTG_CORE_HNP_CSTAT);
                u32 hint_stat = fs_rl(OTG_CORE_HINT_STAT);
                TRACE_MSG1(OCD, "MODULE_ASHNPINT %08x", hint_stat);

                fs_main_clock_on();                          // turn on Main Clock

                //if (hnp_cstat & MODULE_ISBDEV) 
                //        TRACE_MSG0(OCD, "ISBDEV");
                
                //if (hnp_cstat & MODULE_ISADEV) 
                //        TRACE_MSG0(OCD, "ISADEV");
                
                fs_wl_clr(OCD, OTG_CORE_HINT_STAT, hint_stat);
                //brassboard_cstat();
                ocd_hnp_int_hndlr(irq, dev_id, regs);
        }
        if (cint_stat & MODULE_ASFCINT) {                               // Asynchronous Function interrupt, enable Func clock
                TRACE_MSG1(OCD, "MODULE_ASFCINT %08x", cint_stat);
                otg_event_irq(ocd_instance->otg, B_SESS_VLD | A_SESS_VLD, PCD, "MX2ADS ASFCINT");
                fs_func_clock_on();                                    // turn on Function Clock
        }

        if (cint_stat & MODULE_ASHCINT) {                               // Asynchronous Host interrupt, enable Host clock
                TRACE_MSG1(OCD, "MODULE_ASHCINT %08x", cint_stat);
                fs_host_clock_on();                         // turn on Host Clock
        }

        if ((cint_stat & MODULE_HNPINT) || hint_stat)                                  // HNP interrupt
                ocd_hnp_int_hndlr(irq, dev_id, regs);
        
        if (cint_stat & MODULE_FCINT) 
                fs_pcd_int_hndlr(irq, dev_id, regs);

        if (cint_stat & MODULE_HCINT) {
                //TRACE_MSG1(OCD, "MODULE_HCINT %08x", cint_stat);
                hcd_hw_int_hndlr(irq, NULL, regs);
        }

        return IRQ_HANDLED;
}

/* ********************************************************************************************* */
/*!
 * pcd_bwkup_int_hndlr_isr() - main bwkkup interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t pcd_bwkup_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
        ocd_ops.interrupts++;
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG0(OCD, "--");
        return pcd_bwkup_int_hndlr (irq, dev_id, regs);
}

/*!
 * ocd_hnp_int_hndlr_isr() - main ocd hnp interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t ocd_hnp_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
        ocd_ops.interrupts++;
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG0(OCD, "--");
        return ocd_hnp_int_hndlr (irq, dev_id, regs);
}

/*!
 * pcd_func_int_hndlr_isr() - main pcd function interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t pcd_func_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
        ocd_ops.interrupts++;
        //printk(KERN_INFO"%s:\n", __FUNCTION__);
        TRACE_MSG0(OCD, "--");
        return fs_pcd_int_hndlr(irq, dev_id, regs);
        //return pcd_func_int_hndlr (irq, dev_id, regs);
}

/*!
 * hcd_host_int_hndlr_isr() - main hcd host interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t hcd_host_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
        ocd_ops.interrupts++;
        TRACE_MSG0(OCD, "--");
        return hcd_host_int_hndlr (irq, dev_id, regs);
}

/*!
 * ocd_ctrl_hndlr_isr() - main ocd controller interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t ocd_ctrl_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
        ocd_ops.interrupts++;
        return ocd_ctrl_int_hndlr (irq, dev_id, regs);
}

/*!
 * ocd_dma_int_hndlr_isr() - main dma interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t ocd_dma_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
        ocd_ops.interrupts++;
        return ocd_dma_int_hndlr (irq, dev_id, regs);
}

/* ********************************************************************************************* */
extern void fs_ocd_init(struct otg_instance *otg, u8 flag);
#if 0
/*!
 * brassboard_ocd_init() - used to initialize/enable or disable the tcd driver
 * @param otg
 * @param flag
 */
void brassboard_ocd_init(struct otg_instance *otg, u8 flag)
{
        ////u32 mode = XCVR_SE0_D;
        ////u32 mode = XCVR_D_SE0;
        //u32 mode = XCVR_D_D;
        u32 mode = XCVR_SE0_SE0;

        TRACE_MSG0(OCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(OCD, "OCD_EN SET");
                fs_ocd_init(otg, flag);

                TRACE_MSG0(OCD, "HWMODE Setup");

                fs_andl(OTG_CORE_HWMODE, ~0xf0);
                fs_orl(OTG_CORE_HWMODE, (mode << 6) | (mode << 4));           // set to software hnp

                TRACE_MSG2(OCD, "PCD_EN: set hwmode: %08x want %08x", fs_rl(OTG_CORE_HWMODE), (mode << 6) | (mode << 4));
                TRACE_MSG1(OCD, "hwmode: %08x", fs_rl(OTG_CORE_HWMODE));

                break;
        case RESET:
                fs_exit();
                TRACE_MSG0(OCD, "OCD_EN RESET");
                fs_ocd_init(otg, flag);
                break;
        }
}
#endif

/* ********************************************************************************************* */
extern int fsgptcr_mod_init (int divisor, int multiplier);
void fsgptcrmod_exit (void);
int fsgptcr_start_timer(struct otg_instance *otg, int usec);
extern int brassboard_ocd_mod_init (void);
extern void brassboard_ocd_mod_exit (void);
#if !defined(OTG_C99)
struct ocd_ops ocd_ops;
void brassboard_ocd_global_init(void)
{
        ZERO(ocd_ops);
        #if defined(CONFIG_OTG_TR_AUTO)
        ocd_ops.capabilities = OCD_CAPABILITIES_TR | OCD_CAPABILITIES_AUTO;
        #else 
        ocd_ops.capabilities = OCD_CAPABILITIES_TR;
        #endif
        ocd_ops.ocd_init_func = fs_ocd_init;
        ocd_ops.mod_init = brassboard_ocd_mod_init;
        ocd_ops.mod_exit = brassboard_ocd_mod_exit;
        ocd_ops.start_timer = fsgptcr_start_timer;
        ocd_ops.ticks = brassboard_trace_ticks;
        ocd_ops.elapsed = brassboard_trace_elapsed;
}
#else /* !defined(OTG_C99) */
void brassboard_ocd_global_init(void) {

}
struct ocd_ops ocd_ops = {
        #if defined(CONFIG_OTG_TR_AUTO)
        .capabilities = OCD_CAPABILITIES_TR | OCD_CAPABILITIES_AUTO,
        #else 
        .capabilities = OCD_CAPABILITIES_TR,
        #endif
        .ocd_init_func = brassboard_ocd_init,
        .mod_init = brassboard_ocd_mod_init,
        .mod_exit = brassboard_ocd_mod_exit,
        .start_timer = fsgptcr_start_timer,
        .ticks = brassboard_trace_ticks,
        .elapsed = brassboard_trace_elapsed,
};
#endif /* !defined(OTG_C99) */

/* ********************************************************************************************* */

void gpio_dump(void)
{
        #if 0
        int i;
        for (i = GPIOA; i <= GPIOF; i++) {
                TRACE_MSG0(OCD, "--------------------");
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_GIUS", i, _reg_GPIO_GIUS(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_GPR", i, _reg_GPIO_GPR(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_DDIR", i, _reg_GPIO_DDIR(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_OCR1", i, _reg_GPIO_OCR1(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_OCR2", i, _reg_GPIO_OCR2(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_ICR1", i, _reg_GPIO_ICR1(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_ICR2", i, _reg_GPIO_ICR2(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_IMR", i, _reg_GPIO_IMR(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_ISR", i, _reg_GPIO_ISR(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_GPR", i, _reg_GPIO_GPR(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_ICONFA1", i, _reg_GPIO_ICONFA1(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_ICONFA2", i, _reg_GPIO_ICONFA2(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_ICONFB1", i, _reg_GPIO_ICONFB1(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_ICONFB2", i, _reg_GPIO_ICONFB2(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_DR", i, _reg_GPIO_DR(i));
                TRACE_MSG3(OCD, "%20s[%x] %08x", "GPIO_SSR", i, _reg_GPIO_SSR(i));
        }
        #endif
}

/*!
 * brassboard_ocd_mod_init() - initial tcd setup
 * Allocate interrupts and setup hardware.
 */
int brassboard_ocd_mod_init (void)
{

	// patw - avoid the whole thing
	//return 0;

	
	printk(KERN_INFO"%s: AAAA1\n",__FUNCTION__);
        int bwkup = request_irq (INT_USB_WAKEUP, pcd_bwkup_int_hndlr_isr, SA_INTERRUPT, UDC_NAME " USBD BWKUP", NULL);
	printk(KERN_INFO"bwkup: %d\n", bwkup);
        int func = request_irq (INT_USB_FUNC, pcd_func_int_hndlr_isr, SA_INTERRUPT, UDC_NAME " USBD FUNC", NULL);
	printk(KERN_INFO"func: %d\n", func);
	// patw - got errno -16 for some odd reason, dummy it out
	int ctrl = 0;
        //int ctrl = request_irq (OTG_USBCTRL, ocd_ctrl_int_hndlr_isr, SA_INTERRUPT, UDC_NAME " USBD CTRL", NULL);
	//printk(KERN_INFO"ctrl: %d\n", ctrl);
        //int host = request_irq (OTG_USBHOST, hcd_host_int_hndlr_isr, SA_INTERRUPT, UDC_NAME " USBD HOST", NULL);
        int host = 0;
	printk(KERN_INFO"host: %d\n", host);
        int dma = request_irq (INT_USB_DMA, ocd_dma_int_hndlr_isr, SA_INTERRUPT, UDC_NAME " USBD DMA", NULL);
	printk(KERN_INFO"dma: %d\n", dma);
        int timer = request_irq (INT_GPT, brassboard_timer_int_hndlr, SA_INTERRUPT, UDC_NAME " OTG TIMER", NULL);
	printk(KERN_INFO"timer: %d\n", timer);

        TRACE_MSG0(OCD, "1. Interrupts requested");

	printk(KERN_INFO"%s: AAAA11\n",__FUNCTION__);
        THROW_IF(bwkup || dma || func || host || ctrl || timer, error);
	printk(KERN_INFO"%s: AAAA2\n",__FUNCTION__);

        /*
         * clocks
         *
         *        *_reg_GPT_GPTCR = 0x300300;
         *        *_reg_GPT_GPTOCR1 = temp_cycles;
         *        *_reg_GPT_GPTCR = 0x300301;
         *
         *              31   27   23   19   15   11   7    3
         *              |    |    |    |    |    |    |    |
         *              0000 0000 0011 0000 0000 0011 0000 0001
         *                           |             |   |      |
         *                          20             9   6      1
         *                         OM1           FRR CLK     EN
         *                                           SRC
         *
         *              OM1     0x3
         *              FRR     0x1
         *              CLKSRC  0x4
         *              EN      0x1
         *
         *              0x0030 0301;
         *
         *
         */

        TRACE_MSG0(OCD, "2. CRM AP - current");

        TRACE_MSG1(OCD, "CRM_AP_ASCSR    %08x", *_reg_CRM_AP_ASCSR      );
        TRACE_MSG1(OCD, "CRM_AP_ACDR     %08x", *_reg_CRM_AP_ACDR       );
        TRACE_MSG1(OCD, "CRM_AP_ACDER1   %08x", *_reg_CRM_AP_ACDER1     );
        TRACE_MSG1(OCD, "CRM_AP_ACDER2   %08x", *_reg_CRM_AP_ACDER2     );

        TRACE_MSG1(OCD, "CRM_AP_ACGCR    %08x", *_reg_CRM_AP_ACGCR      );
        TRACE_MSG1(OCD, "CRM_AP_ACCGCR   %08x", *_reg_CRM_AP_ACCGCR     );
        TRACE_MSG1(OCD, "CRM_AP_AMLPMRA  %08x", *_reg_CRM_AP_AMLPMRA    );
        TRACE_MSG1(OCD, "CRM_AP_AMLPMRB  %08x", *_reg_CRM_AP_AMLPMRB    );

        TRACE_MSG1(OCD, "CRM_AP_AMLPMRC  %08x", *_reg_CRM_AP_AMLPMRC    );
        TRACE_MSG1(OCD, "CRM_AP_AMLPRMD  %08x", *_reg_CRM_AP_AMLPRMD    );
        TRACE_MSG1(OCD, "CRM_AP_AMLPRME1 %08x", *_reg_CRM_AP_AMLPRME1   );
        TRACE_MSG1(OCD, "CRM_AP_AMLPRME2 %08x", *_reg_CRM_AP_AMLPRME2   );

        TRACE_MSG1(OCD, "CRM_AP_AMLPMRF  %08x", *_reg_CRM_AP_AMLPMRF    );
        TRACE_MSG1(OCD, "CRM_AP_AMLPMRG  %08x", *_reg_CRM_AP_AMLPMRG    );
        TRACE_MSG1(OCD, "CRM_AP_APGCR    %08x", *_reg_CRM_AP_APGCR      );
        TRACE_MSG1(OCD, "CRM_AP_ACSR     %08x", *_reg_CRM_AP_ACSR       );

        TRACE_MSG1(OCD, "CRM_AP_ADCR     %08x", *_reg_CRM_AP_ADCR       );
        TRACE_MSG1(OCD, "CRM_AP_ACR      %08x", *_reg_CRM_AP_ACR        );
        TRACE_MSG1(OCD, "CRM_AP_AMCR     %08x", *_reg_CRM_AP_AMCR       );
        TRACE_MSG1(OCD, "CRM_AP_APCR     %08x", *_reg_CRM_AP_APCR       );

        TRACE_MSG1(OCD, "CRM_AP_AMORA    %08x", *_reg_CRM_AP_AMORA      );
        TRACE_MSG1(OCD, "CRM_AP_AMORB    %08x", *_reg_CRM_AP_AMORB      );
        TRACE_MSG1(OCD, "CRM_AP_AGPR     %08x", *_reg_CRM_AP_AGPR       );
        TRACE_MSG1(OCD, "CRM_AP_APRA     %08x", *_reg_CRM_AP_APRA       );

        TRACE_MSG1(OCD, "CRM_AP_APRB     %08x", *_reg_CRM_AP_APRB       );
        TRACE_MSG1(OCD, "CRM_AP_APOR     %08x", *_reg_CRM_AP_APOR       );

        /*
         * ipg_enable_clk
         *
         * CKOH pin
         * ACR register |= 0x4 << 12
         */


        /* C.f. Figure 0-75. USBOTG
         *
         * ipg_enable_clk
         * AMORB:mob[4]
         * AMLPMRE1:mlpme7[2:0]
         * ap_com_pclk[0]
         * ap_lps_usbotg_en
         * AMLMRG:mlpmg5[2:0]
         * ap_com_ahb_clk[1]
         * ACDER2(12):usben
         * usb_clk
         *
         */
       
        /* C.f. Table 5-3 Module_en Mapping
         * C.f. 5.3.4.22 AP Module_en Override Register B
         */
	printk(KERN_INFO"%s: AAAA22\n",__FUNCTION__);
        TRACE_MSG0(OCD, "2. CRM AP - setup");
        *_reg_CRM_AP_AMORB |= (1 << (36 - 32)) | (1 << (52 - 32));

        TRACE_MSG1(OCD, "CRM_AP_AMORB    %08x", *_reg_CRM_AP_AMORB      );

        TRACE_MSG0(OCD, "3. Setup GPT");
#define MXC91231_MULTIPLIER       12              
#define MXC91231_DIVISOR          155             // ~10.
        fsgptcr_mod_init(MXC91231_DIVISOR, MXC91231_MULTIPLIER);


	return 0;

        CATCH(error) {
        	printk(KERN_INFO"brassboard_ocd__BBBB\n");
                return -EINVAL;
        }
        return 0;
}

extern void fs_stop_ep(int epn, int dir);

/*!
 * brassboard_ocd_mod_exit() - de-initialize
 */
void brassboard_ocd_mod_exit (void)
{
        fsptrcr_mod_exit();
        #if 0
        int i;
        unsigned long flags;

        _reg_CRM_PCCR1 &= ~(1<<27);                                             // disable GPT3

        TRACE_MSG0(OCD, "free irqs");
        for (i = OTG_USBWKUP; i <= OTG_USBCTRL; i++) free_irq (i, NULL);        // Free IRQ's
        free_irq (INT_GPT2, NULL);
        //free_irq (INT_GPIO, NULL);
        #endif
}

/* @} */
#endif


