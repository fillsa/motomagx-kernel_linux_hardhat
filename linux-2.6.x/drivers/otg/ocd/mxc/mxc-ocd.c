/*
 * otg/ocd/mxc/mxc-ocd.c -- USB Device Controller driver 
 * @(#) balden@seth2.belcarratech.com|otg/ocd/mxc/mxc-ocd.c|20051116203403|13113
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@lbelcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 23/03/2007         Motorola         PLL and clock controls
 * 08/24/2007         Motorola         Changes for Open Src compliance.
 */
/*
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
/*!
 * @file otg/ocd/mxc/mxc-ocd.c
 * @brief Freescale USB OTG Controller Driver
 *
 * This is the OTG Controller Driver. It does generic configuration
 * and handles the all USBOTG interrupts.
 *
 * There is no board or platform level code here.
 *
 * @ingroup FSOTG
 *
 */

#include <otg/pcd-include.h>
#include <otghw/mxc-hardware.h>
#include <asm/arch/clock.h>

int mxc_transceiver_mode;

/* ********************************************************************************************* */
#define TIMEOUT_VALUE 1000
/*!
 * mxc_func_clock_on() - enable function controller clock
 */
void mxc_func_clock_on(void)
{
        u32 timeout = TIMEOUT_VALUE;
        fs_orl(OTG_CORE_CLK_CTRL, MODULE_FUNC_CLK);
        while(!( fs_rl(OTG_CORE_CLK_CTRL) & MODULE_FUNC_CLK)) { timeout--; if (!timeout) break; }
}       

/*!
 * mxc_host_clock_on() - enable host controller clock
 */
void mxc_host_clock_on(void)
{
        u32 timeout = TIMEOUT_VALUE;
        fs_orl(OTG_CORE_CLK_CTRL, MODULE_HOST_CLK);
        while(!( fs_rl(OTG_CORE_CLK_CTRL) & MODULE_HOST_CLK)) { timeout--; if (!timeout) break; }
}       

/*!
 * mxc_func_clock_off() - disable function controller clock
 */
void mxc_func_clock_off(void)
{       
        u32 timeout = TIMEOUT_VALUE;
        fs_andl(OTG_CORE_CLK_CTRL, 0x0);
        while((fs_rl(OTG_CORE_CLK_CTRL) & (MODULE_FUNC_CLK | MODULE_MAIN_CLK))) { timeout--; if (!timeout) break; }
}       

/*!
 * mxc_host_clock_off() - disable host controller clock
 */
void mxc_host_clock_off(void)
{       
        u32 timeout = TIMEOUT_VALUE;
        fs_andl(OTG_CORE_CLK_CTRL, 0x0);
        while((fs_rl(OTG_CORE_CLK_CTRL) & (MODULE_HOST_CLK | MODULE_MAIN_CLK))) { timeout--; if (!timeout) break; }
}       

/*!
 * mxc_main_clock_on() - enable main clock
 */
void mxc_main_clock_on(void)
{
        u32 timeout = TIMEOUT_VALUE;
        fs_orl(OTG_CORE_CLK_CTRL, MODULE_MAIN_CLK);
        while(!( fs_rl(OTG_CORE_CLK_CTRL) & MODULE_MAIN_CLK)) { timeout--; if (!timeout) break; }
}

/*!
 * mxc_main_clock_off() - disable main clock
 */
void mxc_main_clock_off(void)
{
        u32 timeout = TIMEOUT_VALUE;
        fs_wl_set(OCD, OTG_CORE_CLK_CTRL, 0);
        while((fs_rl(OTG_CORE_CLK_CTRL) & MODULE_MAIN_CLK)) { timeout--; if (!timeout) break; }
}

/*!
 * mxc_set_transceiver_mode(int)
 */
void mxc_set_transceiver_mode(int mode)
{   
        mxc_transceiver_mode = mode;
        fs_andl(OTG_CORE_HWMODE, ~0xf0);
        fs_orl(OTG_CORE_HWMODE, (mode << 6) | (mode << 4));           // set to software hnp
        TRACE_MSG1(OCD, "hwmode: %08x", fs_rl(OTG_CORE_HWMODE));
}       



/* ********************************************************************************************* */

/*!
 * mxc_disable_interrupts() - disable interrupts
 */
void mxc_disable_interrupts (void)
{
        fs_wl(OTG_CORE_CINT_STEN, 0 );
        fs_wl(OTG_CORE_HINT_STEN, 0 );
        fs_wl(OTG_HOST_SINT_STEN, 0 );
        fs_wl(OTG_HOST_XYINT_STEN, 0 );
        fs_wl(OTG_HOST_ETD_EN, 0);
        fs_wl(OTG_HOST_ETD_DONE, 0 );
        fs_wl(OTG_FUNC_SINT_STEN, 0 );
        fs_wl(OTG_FUNC_XYINT_STEN, 0 );
        fs_wl(OTG_FUNC_EP_EN, 0 );
        fs_wl(OTG_FUNC_EP_DEN, 0 );
}

/* ********************************************************************************************* */

/* ********************************************************************************************* */
/*!
 * mxc_init() - initial tcd setup
 * Allocate interrupts and setup hardware.
 */
void mxc_init (void)
{
        int timeout; 
        unsigned long flags;
        u32 mode = XCVR_SE0_D_NEW;

        TRACE_MSG0(OCD, "FS_INIT");
        local_irq_save (flags);

        fs_wl(OTG_SYS_CTRL, 0x0);

        /* 2. Ensure hardware is reset and cleared
         */
        fs_clear_words((volatile u32 *)IO_ADDRESS(OTG_DMA_BASE), (16*16/4));
        fs_clear_words((void *)IO_ADDRESS(OTG_FUNC_BASE), 0x200);
        mxc_main_clock_off();

        fs_wl_set(OCD, OTG_CORE_RST_CTRL, MODULE_RSTI2C | 0x3f);
        while (fs_rl(OTG_CORE_RST_CTRL));

        /* 3. OTG Hardware Mode and clocks
         * set to diff, diff and Configure the OTG to behave as function
         */
        TRACE_MSG0(OCD, "3. OTG Software Mode and clock");

        fs_orl(OTG_CORE_HWMODE, MODULE_CRECFG_SHNP);           // set to software hnp
        mxc_set_transceiver_mode(mxc_transceiver_mode);
        TRACE_MSG2(PCD, "FS_INIT: set hwmode: %08x want %08x", fs_rl(OTG_CORE_HWMODE), MODULE_CRECFG_SHNP); 

        fs_andl(OTG_CORE_HNP_CSTAT, ~0x00000800);
        fs_rl(OTG_CORE_HNP_CSTAT);

        fs_wl_set(OCD, OTG_CORE_HNP_T3PCR, 0x00000000);
        fs_rl(OTG_CORE_HNP_T3PCR);

        TRACE_MSG0(OCD, "6. Enable ");
        TRACE_MSG0(OCD, "enable core interrupts");

        fs_wl(OTG_CORE_CINT_STEN, 0);
        
        fs_wl(OTG_CORE_CINT_STEN, MODULE_ASHNPINT_EN | 
                MODULE_ASHCINT_EN | MODULE_HNPINT_EN | MODULE_FCINT | MODULE_HCINT);
        
        
        TRACE_MSG0(OCD, "enable host interrupts");
        fs_wl(OTG_CORE_HINT_STEN, HNP_I2COTGINT_EN | HNP_AWAITBTO_EN |  
                HNP_AIDLEBDTO_EN | HNP_SRPSUCFAIL_EN | HNP_SRPINT_EN | HNP_VBUSERROR_EN |
                HNP_ABSEVAILD_EN | HNP_ABUSVALID_EN | HNP_MASSLVCHG_EN | HNP_IDCHANGE_EN); 

        TRACE_MSG0(OCD, "disable various host interrupts");
        fs_wl(OTG_HOST_XYINT_STEN, 0);
        fs_wl(OTG_HOST_ETD_EN, 0);
        fs_wl(OTG_HOST_ETD_DONE, 0);
        fs_wl(OTG_HOST_SINT_STEN, 0);

        TRACE_MSG0(OCD, "disable various function interrupts");
        fs_wl(OTG_FUNC_XYINT_STEN, 0); 
        fs_wl(OTG_FUNC_EP_EN, 0); 
        fs_wl(OTG_FUNC_EP_DEN, 0);

        fs_wl(OTG_DMA_DINT_STEN, 0x3);

        // XXX note that newer designs than the mx21 will also need to check
        // and/or set OTG_CORE_INTERRUPT_STEN


        TRACE_MSG0(OCD, "--");
        TRACE_MSG0(OCD, "8. Ready ");
        TRACE_MSG1(OCD, "CINT_STEN: %08x", fs_rl(OTG_CORE_CINT_STEN));

        local_irq_restore (flags);
}

extern void mxc_stop_ep(int epn, int dir);

/*!
 * mxc_exit() - de-initialize
 */
void mxc_exit(void)
{
        int i;
        unsigned long flags;
        TRACE_MSG0(OCD, "FS_EXIT");

        local_irq_save (flags);
        for (i = 0; i < 16; i++) {
                mxc_stop_ep(i, USB_DIR_IN);
                mxc_stop_ep(i, USB_DIR_OUT);
        }
        fs_clear_words((volatile u32 *)IO_ADDRESS(OTG_EP_BASE), (32*16/4));
        fs_wl( OTG_CORE_HWMODE, 0xa3);
        mxc_main_clock_off();
        local_irq_restore (flags);
}


/*!
 * mxc_ocd_init() - used to initialize/enable or disable the tcd driver
 * @param otg
 * @param flag
 */
void mxc_ocd_init(struct otg_instance *otg, u8 flag)
{
        TRACE_MSG0(OCD, "--");
        switch (flag) {
        case SET:
                TRACE_MSG0(OCD, "FS_OCD_EN SET");
                mxc_init();
                otg_event(otg, TCD_OK | ID_FLOAT, OCD, "OCD_OK");
                break;
        case RESET:
                mxc_exit();
                TRACE_MSG0(OCD, "FS_OCD_EN RESET");
                otg_event(otg, TCD_OK | ID_FLOAT, OCD, "OCD_OK");
                break;
        }
}

/* ********************************************************************************************* */


struct ocd_ops ocd_ops;

extern irqreturn_t mxc_pcd_int_hndlr (int irq, void *dev_id, struct pt_regs *regs);
extern irqreturn_t mxc_hcd_hw_int_hndlr(int irq, void *dev_id, struct pt_regs *regs);

extern void mxc_func_clock_on(void);
extern void mxc_func_clock_off(void);
extern void mxc_main_clock_on(void);
extern void mxc_main_clock_off(void);
extern void mxc_set_hw_mode(int);

#if defined(CONFIG_OTG_USB_PERIPHERAL) || defined(CONFIG_OTG_BDEVICE_WITH_SRP)
irqreturn_t mxc_hcd_hw_int_hndlr(int irq, void *dev_id, struct pt_regs *regs)
{
        return IRQ_HANDLED;
}
#endif /* defined(CONFIG_OTG_USB_PERIPHERAL) || defined(CONFIG_OTG_BDEVICE_WITH_SRP) */


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
        u32 hnp_cstat = fs_rl(OTG_CORE_HNP_CSTAT);

        /* get and clear interrupts
         */
        fs_wl_set(OCD, OTG_CORE_HINT_STAT, hint_stat);
        printk(KERN_INFO"%s: HINT_STAT: %08x HNP_CSTAT: %08x\n", __FUNCTION__, hint_stat, hnp_cstat); 

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

        TRACE_MSG2(OCD, "SYS_CTRL: %08x CORE_CLK: %08x", sys_ctrl, fs_rl(OTG_CORE_CLK_CTRL));

        mxc_main_clock_on();                          // turn on Main Clock
        mxc_func_clock_on();                                    // turn on Function Clock

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
        if (dma_interrupts > 10) {
                TRACE_MSG1(OCD, "DMA INT #%08x disabling DMA error interrupts",dint_stat);
                fs_wl(OTG_DMA_DINT_STEN, 0x0);
                fs_wl(OTG_DMA_EP_ERR, 0x0);
        } 
        else {
                if (dint_stat & 0x1) {  // FIXME - find the #def for 0x1
                        // HCD DMA error
                        mxc_hcd_hw_int_hndlr(irq, NULL, regs);
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
        mxc_hcd_hw_int_hndlr(irq, NULL, regs);
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
        u32 hnp_cstat = fs_rl(OTG_CORE_HNP_CSTAT);

        static u32 ocd_interrupts = 0;

        int handled = 0;

        TRACE_MSG4(OCD, "CLK: %08x sys: %08x cint: %08x hint: %08x", fs_rl(OTG_CORE_CLK_CTRL), sys_ctrl, cint_stat, hint_stat);

        if (sys_ctrl & (SYS_CTRL_HOST_WU_INT_STAT | SYS_CTRL_FNT_WU_INT_STAT)) 
                TRACE_MSG1(OCD, "SYS_CTRL: HOST/FUNC %08x", sys_ctrl);
        

        /* N.B. OTG_CORE_CINT_STAT interrupts are self clearing when interrupt
         * sources have been cleared. 
         */
        UNLESS(cint_stat || hint_stat) {
                return IRQ_HANDLED;
        }

        if (hint_stat) {
                
                fs_wl(OTG_CORE_HINT_STAT, hint_stat);
                printk(KERN_INFO"%s: HINT_STAT: %08x HNP_CSTAT: %08x\n", __FUNCTION__, hint_stat, hnp_cstat); 
        }

        if (cint_stat & MODULE_ASHNPINT) {                              // asynchronous HNP interrupt, enable Main clock
                u32 hnp_cstat = fs_rl(OTG_CORE_HNP_CSTAT);
                u32 hint_stat = fs_rl(OTG_CORE_HINT_STAT);
                TRACE_MSG1(OCD, "MODULE_ASHNPINT %08x", hint_stat);

                mxc_main_clock_on();                          // turn on Main Clock

                fs_wl_set(OCD, OTG_CORE_HINT_STAT, hint_stat);
                ocd_hnp_int_hndlr(irq, dev_id, regs);
        }
        if (cint_stat & MODULE_ASFCINT) {                               // Asynchronous Function interrupt, enable Func clock
                TRACE_MSG1(OCD, "MODULE_ASFCINT %08x", cint_stat);
                otg_event_irq(ocd_instance->otg, B_SESS_VLD | A_SESS_VLD, PCD, "MX2ADS ASFCINT");
                mxc_func_clock_on();                                    // turn on Function Clock
        }

        if (cint_stat & MODULE_ASHCINT) {                               // Asynchronous Host interrupt, enable Host clock
                TRACE_MSG1(OCD, "MODULE_ASHCINT %08x", cint_stat);
                mxc_host_clock_on();                         // turn on Host Clock
        }

        if ((cint_stat & MODULE_HNPINT) || hint_stat)                                  // HNP interrupt
                ocd_hnp_int_hndlr(irq, dev_id, regs);
        
        if (cint_stat & MODULE_FCINT) 
                mxc_pcd_int_hndlr(irq, dev_id, regs);

        if (cint_stat & MODULE_HCINT) {
                mxc_hcd_hw_int_hndlr(irq, NULL, regs);
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
        pcd_instance->otg->interrupts++;
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
        pcd_instance->otg->interrupts++;
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
        pcd_instance->otg->interrupts++;
        TRACE_MSG1(OCD, "CLK: %08x", fs_rl(OTG_CORE_CLK_CTRL) );

        return mxc_pcd_int_hndlr(irq, dev_id, regs);
}

/*!
 * hcd_host_int_hndlr_isr() - main hcd host interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 */
irqreturn_t hcd_host_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs)
{
        pcd_instance->otg->interrupts++;
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
        pcd_instance->otg->interrupts++;
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
        pcd_instance->otg->interrupts++;
        return ocd_dma_int_hndlr (irq, dev_id, regs);
}

/* ********************************************************************************************* */
extern void mxc_ocd_init (struct otg_instance *otg, u8 flag);
extern int mxc_ocd_mod_init (void);
extern void mxc_ocd_mod_exit (void);

#if defined(CONFIG_OTG_GPTR)
int mxc_gptcr_start_timer(struct otg_instance *otg, int usec);
u64 mxc_gptcr_trace_ticks (void);
u64 mxc_gptcr_trace_elapsed(u64 *t1, u64 *t2);
#endif /* defined(CONFIG_OTG_GPTR) */
#if defined(CONFIG_OTG_HRT)
int mxc_hrt_start_timer(struct otg_instance *otg, int usec);
u64 mxc_hrt_trace_ticks (void);
u64 mxc_hrt_trace_elapsed(u64 *t1, u64 *t2);
#endif /* defined(CONFIG_OTG_HRT) */

#if !defined(OTG_C99)
struct ocd_ops ocd_ops;
void mxc_ocd_global_init(void)
{
        ZERO(ocd_ops);
        #if defined(CONFIG_OTG_TR_AUTO)
        ocd_ops.capabilities = OCD_CAPABILITIES_TR | OCD_CAPABILITIES_AUTO;
        #else 
        ocd_ops.capabilities = OCD_CAPABILITIES_TR;
        #endif
        ocd_ops.ocd_init_func = mxc_ocd_init;
        ocd_ops.mod_init = mxc_ocd_mod_init;
        ocd_ops.mod_exit = mxc_ocd_mod_exit;
#if defined(CONFIG_OTG_GPTR)
        ocd_ops.start_timer = mxc_gptcr_start_timer;
        ocd_ops.ticks = mxc_gptcr_trace_ticks;
        ocd_ops.elapsed = mxc_gptcr_trace_elapsed;
#endif  /* defined(CONFIG_OTG_GPTR) */
#if defined(CONFIG_OTG_HRT)
        ocd_ops.start_timer = mxc_hrt_start_timer;
        ocd_ops.ticks = mxc_hrt_trace_ticks;
        ocd_ops.elapsed = mxc_hrt_trace_elapsed;
#endif /* defined(CONFIG_OTG_HRT) */
}
#else /* !defined(OTG_C99) */
//void mxc_ocd_global_init(void) {

//}
struct ocd_ops ocd_ops = {
        #if defined(CONFIG_OTG_TR_AUTO)
        .capabilities = OCD_CAPABILITIES_TR | OCD_CAPABILITIES_AUTO,
        #else 
        .capabilities = OCD_CAPABILITIES_TR,
        #endif
        .ocd_init_func = mxc_ocd_init,
        .mod_init = mxc_ocd_mod_init,
        .mod_exit = mxc_ocd_mod_exit,
#if defined(CONFIG_OTG_GPTR)
        .start_timer = mxc_gptcr_start_timer,
        .ticks = mxc_gptcr_trace_ticks,
        .elapsed = mxc_gptcr_trace_elapsed,
#endif  /* defined(CONFIG_OTG_GPTR) */
#if defined(CONFIG_OTG_HRT)
        .start_timer = mxc_hrt_start_timer,
        .ticks = mxc_hrt_trace_ticks,
        .elapsed = mxc_hrt_trace_elapsed,
#endif /* defined(CONFIG_OTG_HRT) */
};
#endif /* !defined(OTG_C99) */

/* ********************************************************************************************* */

/*!
 * mxc_ocd_mod_init() - initial tcd setup
 * Allocate interrupts and setup hardware.
 */
int mxc_ocd_mod_init (void)
{
        int bwkup = request_irq (INT_USB_WAKEUP, pcd_bwkup_int_hndlr_isr, OTG_INTERRUPT, UDC_NAME " USBD BWKUP", NULL);
        int func = request_irq (INT_USB_FUNC, pcd_func_int_hndlr_isr, OTG_INTERRUPT, UDC_NAME " USBD FUNC", NULL);
        int ctrl = request_irq (INT_USB_CTRL, ocd_ctrl_int_hndlr_isr, OTG_INTERRUPT, UDC_NAME " USBD CTRL", NULL);
        int dma = request_irq (INT_USB_DMA, ocd_dma_int_hndlr_isr, OTG_INTERRUPT, UDC_NAME " USBD DMA", NULL);
        int clk_ret=-1;

        RETURN_EINVAL_IF(bwkup || dma || func || ctrl);

        /* USB_CLK is shared by SD and USB. To switch on/off the USBCLK,
         * we request the OS to turn it on/off the USB PLL.
         * USB PLL must be requested at init *
         * AHB-dma, and IPG-register access, clocks must also be started */
        TRACE_MSG0(OCD,"INT started, starting clocks");
        clk_ret = mxc_pll_request_pll(USBPLL);
        if(clk_ret !=0) {
              TRACE_MSG0(OCD,"ERROR: Could not start the clocks, returning");
              printk("ERROR: Request USBPLL Failed\n");
              return -1;
        }             
        mxc_clks_enable(USB_IPG_CLK);
        mxc_clks_enable(USB_AHB_CLK); 

        return 0;
}

extern void mxc_stop_ep(int epn, int dir);

/*!
 * mxc_ocd_mod_exit() - de-initialize
 */
void mxc_ocd_mod_exit (void)
{
        int clk_ret=-1;

        TRACE_MSG0(OCD,"Stoping clocks");
        /* Switch off USB clocks and USBPLL *
         * USB PLL must be released at exit *
         * AHB and IPG clocks must be switched off */
        mxc_clks_disable(USB_AHB_CLK);
        mxc_clks_disable(USB_IPG_CLK);
        clk_ret = mxc_pll_release_pll(USBPLL);
        if(clk_ret !=0) {
              TRACE_MSG0(OCD,"ERROR: Could not stop the clocks");
              printk("ERROR: Release USBPLL Failed\n");
        }

        TRACE_MSG0(OCD, "clocks started, freeing irqs");
        free_irq (INT_USB_WAKEUP, NULL);
        free_irq (INT_USB_FUNC, NULL);
        free_irq (INT_USB_CTRL, NULL);
        free_irq (INT_USB_DMA, NULL);
}


