/*
 * otg/mxc/mxc-gpio.c - Freescale USBOTG common gpio and iomux setting
 * @(#) balden@belcarra.com|otg/ocd/mxc/mxc-gpio.c|20051117000523|28921
 *
 *      Copyright (c) 2004-2005 Belcarra Technologies
 *
 * By:
 *      Stuart Lynne <sl@belcara.com>,
 *      Bruce Balden <balden@belcara.com>
 *
 *      Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 06/12/2006         Motorola         OTG name change
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 07/26/2007         Motorola         Changes for Open Src compliance.
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
 */
/*!
 * @file otg/ocd/mxc/mxc-gpio.c
 * @brief Freescale USB Host Controller Driver
 *
 * @ingroup FSOTG
 */ 


#include <otg/pcd-include.h>

#include <asm/mot-gpio.h>
#include <otghw/mxc-hardware.h>
#include <otghw/isp1301.h>
#include <otghw/isp1301-hardware.h>

#if defined(CONFIG_OTG_ZASEVB_ISP1301)
#include <asm/mot-gpio.h>
#include <otghw/mxc-hardware.h>
#include <asm/arch/board.h>
#include <otghw/isp1301.h>
#include <otghw/isp1301-hardware.h>
#endif

#if defined(CONFIG_MACH_I30030EVB) || defined(CONFIG_MACH_I30030ADS) || \
    defined(CONFIG_MACH_MXC30030EVB) || defined(CONFIG_MACH_MXC30030ADS) ||  \
    defined(CONFIG_MACH_ARGONLVPHONE)
#define ZGPIO_PORT 0
#define ZGPIO_PIN  2
#endif              /* defined(CONFIG_MACH_I30030EVB) */

#if defined(CONFIG_MACH_MXC27530EVB) || defined(CONFIG_ARCH_MXC27530EVB)
#define ZGPIO_PORT 2
#define ZGPIO_PIN  12
#endif /* CONFIG_MACH_MXC27530EVB */

#if defined(CONFIG_MACH_MXC91131EVB) || defined(CONFIG_ARCH_MXC91131)
#define ZGPIO_PORT 1
#define ZGPIO_PIN 30
#endif /* CONFIG_MACH_MXC91131EVB */


#if defined(CONFIG_OTG_ZASEVB_ISP1301)

/* ********************************************************************************************* */
/*!
 * zasevb_gpio_int_hndlr() - gpio interrupt handler
 * @param irq
 * @param dev_id
 * @param regs
 * @return interrupt handler status
 * This disables the gpio interrup and schedules the isp1301 bottom half handler.
 *
 */
static irqreturn_t zasevb_gpio_int_hndlr (int irq, void *dev_id, struct pt_regs *regs)
{
        gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, FALSE);

	TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: SCHEDULE WORK");
        isp1301_bh_wakeup(FALSE);

        return IRQ_HANDLED;
}

/*!
 * zasevb_isp1301_bh()- call isp1301 bottom half handler
 * @param arg
 * This is a wrapper to the isp1301 bottom half handler, it
 * re-enables the gpio interrupt after processing complete.
*/
void zasevb_isp1301_bh(void *arg)
{
        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: ISP1301_BH");
        isp1301_bh(arg);
        TRACE_MSG0(TCD, "ZASEVB GPIO INTERRUPT: REENABLE");
        gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, TRUE);
}




/*!
 * This function set iomux settings depends on platform and usb mode.
 *
 * @param  usb_mode     setting usb mode.
 *   
 */

int mxc_iomux_gpio_isp1301_set (int usb_mode)
{

	int gpio = 1;	

	printk (KERN_INFO"MXC gpio setting for isp1301\n");

	isp1301_mod_init(&zasevb_isp1301_bh);
	
	TRACE_MSG0(TCD, "5. IOMUX and GPIO Interrupt Configuration");

        #ifdef CONFIG_MACH_MXC27530EVB
        iomux_config_mux(AP_GPIO_AP_C12, OUTPUTCONFIG_DEFAULT, INPUTCONFIG_NONE); // XXX INPUTCONFIG_DEFAULT?
        #endif /* CONFIG_MACH_MXC27530EVB */
		
        #ifdef CONFIG_MACH_ARGONLVPHONE
        iomux_config_mux(PIN_GPIO2, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_MACH_ARGONLVPHONE */
		
        #ifdef CONFIG_MACH_MXC91131EVB
        iomux_config_mux(SPI2_SS1_PIN, MUX0_OUT, GPIO_MUX1_IN);
        #endif /* CONFIG_MACH_MXC91131EVB */
		
	//Settung interrupt for ISP1301
        gpio_config(ZGPIO_PORT, ZGPIO_PIN, false, GPIO_INT_FALL_EDGE);
        gpio = gpio_request_irq(ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO, zasevb_gpio_int_hndlr,
		                        SA_SHIRQ, "ISP1301", (void *) &ocd_ops);
        THROW_IF(gpio, error);
        gpio_config_int_en(ZGPIO_PORT, ZGPIO_PIN, TRUE);  // XXX this might not be needed
				

        #if defined(CONFIG_ARCH_MXC91231)
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        #if 0
        switch(hwmode) {
        case XCVR_SE0_D_NEW:
        case XCVR_D_SE0_NEW:
                iomux_config_mux(AP_GPIO_AP_C16,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
                iomux_config_mux(AP_GPIO_AP_C17,OUTPUTCONFIG_FUNC3, INPUTCONFIG_FUNC3);
                printk(KERN_INFO"%s: EXTRA IOMUX\n", __FUNCTION__);
                break;

	case XCVR_D_D:
	case XCVR_SE0_SE0:
                printk(KERN_INFO"%s: NO EXTRA IOMUX\n", __FUNCTION__);
                break;
        }
        #endif
        #endif /* CONFIG_ARCH_MXC91231 */
						
        #if defined(CONFIG_MACH_ARGONLVPHONE)
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_MACH_ARGONLVPHONE */
							
	
	#ifdef CONFIG_ARCH_MXC91131
        iomux_config_mux(USB_DAT_VP_PIN,  MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x00
        iomux_config_mux(USB_SE0_VM_PIN,  MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x01
        iomux_config_mux(USB_TXOE_B_PIN,  MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x09
        iomux_config_mux(USB_RXD_PIN,     MUX0_OUT, MUX0_IN);           // iomux_com_base + 0x3c
	#endif


        CATCH(error) {
                printk(KERN_INFO"%s: failed\n", __FUNCTION__);
                UNLESS (gpio) gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
                return -EINVAL;
        }
	
	return 0;
}

int mxc_iomux_gpio_isp1301_reset (void)
{
	gpio_free_irq (ZGPIO_PORT, ZGPIO_PIN, GPIO_HIGH_PRIO);
	return 0;
}

#endif	//CONFIG_OTG_ZASEVB_ISP1301



//#ifdef CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY
int mxc_iomux_gpio_mc13783_set (int usb_mode)
{

	printk (KERN_INFO"MXC gpio setting for mc13783\n");
        #if defined(CONFIG_ARCH_MXC91231)
        printk(KERN_INFO"IOMUX setting for MXC91231\n");
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC1, INPUTCONFIG_FUNC1);
        #endif

        #if defined(CONFIG_MACH_ARGONLVPHONE) 
        printk(KERN_INFO"IOMUX setting for ARGONLV REF\n");
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_MACH_ARGONLVPHONE */
								
	
	return 0;
}


int mxc_iomux_gpio_mc13783_reset (void)
{

        #if defined(CONFIG_ARCH_MXC91231)
        iomux_config_mux(SP_USB_TXOE_B, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_DAT_VP, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_SE0_VM, OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        iomux_config_mux(SP_USB_RXD,    OUTPUTCONFIG_FUNC2, INPUTCONFIG_FUNC2);
        #endif

        #if defined(CONFIG_MACH_ARGONLVPHONE)
        iomux_config_mux(PIN_USB_XRXD,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPOUT, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VPIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_TXENB, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        iomux_config_mux(PIN_USB_VMIN,  OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
        #endif /* CONFIG_MACH_ARGONLVPHONE */
											
	
	return 0;
}
//#endif	//CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY

#if defined(CONFIG_OTG_ZASEVB_ISP1301)
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_isp1301_set);
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_isp1301_reset);
#endif
//#ifdef CONFIG_OTG_ZASEVB_MC13783_CONNECTIVITY
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_mc13783_set);
OTG_EXPORT_SYMBOL(mxc_iomux_gpio_mc13783_reset);
//#endif


