/***********************************************************************************
 ** ********************************************************************************
 ** fx2lp-pcd.c - Cypress FX2LP HiSpeed USB Peripheral Controller driver
 **
 ** Cypress FX2LP HiSpeed USB Peripheral Controller driver
 **
 ** Portability:
 **     This file contains H/W specific code. Partially portable
 **
 ** ********************************************************************************
 ** ********************************************************************************/

/***********************************************************************************
 *** COMPILATION FLAGS
 *** -----------------_
 *** NOT_USED     - This code is commented for experimental purpose and is **NOT**
 ***                supposed to be defined in the makefile.
 ***
 ***********************************************************************************/

/*
 *  Copyright (c) 2006-2008, Motorola, All Rights Reserved.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are 
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be 
 *  used to endorse or promote products derived from this software without 
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE 
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 03/08/2006         Motorola, Inc.   Initial create
 * 09/21/2006         Motorola, Inc.   call hotplug script if FX2LP WAKEUP not 
 *                                     connected
 * 10/30/2006         Motorola, Inc.   Bus Speed initialization issues
 * 11/21/2006         Motorola, Inc.   Redesign
 * 02/08/2007         Motorola, Inc.   Flush IN FIFO on cancel and enable EP
 * 03/09/2007         Motorola, Inc.   Add checks for pointer deref in handle_pcd_isr
 * 03/12/2007         Motorola, Inc.   Cleanup comment
 * 03/09/2007         Motorola, Inc.   Check pointers in ep_short_packet
 * 03/23/2007         Motorola, Inc.   Check for urb null pointer
 * 05/15/2007         Motorola, Inc.   Don't schedule a suspend bh if already scheduled
 * 11/23/2007         Motorola, Inc.   Added comments for connect to host flag
 * 06/12/2008         Motorola, Inc,   Fix mismatching tags issue in SCSI CSW
 * 08/08/2008         Motorola, Inc.   Commented out OTG debug from dmesg 
*/
/***********************************************************************************
 *** SYSTEM SPECIFIC HEADERS
 ***********************************************************************************/

#include <otg/pcd-include.h>
#include <otg/usbp-func.h>
#include <linux/string.h>
#include <fx2lp_hardware.h>
#include <linux/delay.h>
#include <fx2lp_ready_events.h>
#include <asm/preempt.h>
#include <asm/timex.h>
#include <asm/arch/gpio.h>


#ifdef CONFIG_OTG_GENERIC_HOTPLUG
#include <otg/hotplug.h>
struct usb_hotplug_private usb_hstest_hotplug;
#endif
/***********************************************************************************
 *** FUNCTION DECLARATIONS
 ***********************************************************************************/
static void fx2lp_send_cmd_irq(PFX2LP_PCD pfx2lp, u8 cmd, u8 data);
static void fx2lp_pcd_setup_ep (struct pcd_instance *pcd, unsigned int epn, struct usbd_endpoint_instance *endpoint);
static FX2LP_BOOL fx2lp_ep_config (FX2LP_EPX epx, u8 ep_dir, u8 ep_type, u16 max_pkt_size);
static void fx2lp_do_out_0 (PFX2LP_PCD, struct usbd_endpoint_instance *);
static void fx2lp_do_out_26_irq(PFX2LP_PCD, struct usbd_endpoint_instance *);
static void fx2lp_do_out_26_packet(struct usbd_endpoint_instance *, u16);
static void fx2lp_pcd_start_endpoint_in (struct pcd_instance *, 
                struct usbd_endpoint_instance *);
static void fx2lp_pcd_start_endpoint_out ( struct pcd_instance *, 
                struct usbd_endpoint_instance *);
static void fx2lp_start_in_data_ep0(PFX2LP_PCD, 
                struct usbd_endpoint_instance *);
static void fx2lp_start_in_data_ep1(PFX2LP_PCD,
                struct usbd_endpoint_instance *);
static void fx2lp_start_in_data_ep26_irq(PFX2LP_PCD,
                struct usbd_endpoint_instance *);
static void fx2lp_do_in_data_ep26_bh(struct usbd_endpoint_instance *);
static void fx2lp_do_out_data_ep26_bh(struct usbd_endpoint_instance *);
static u16 fx2lp_do_in_data_ep26(PFX2LP_PCD ,
                struct usbd_endpoint_instance *);
static void fx2lp_finish_in_ep26_irq(PFX2LP_PCD,
                struct usbd_endpoint_instance *);
static void fx2lp_handle_cmd_out(PFX2LP_PCD pfx2lp);
static void fx2lp_pcd_mod_exit_l26(void);

extern int fx2lp_pcd_mod_init(void);
extern irqreturn_t  fx2lp_handle_pcd_isr(int irq, void *dev_id, struct pt_regs *regs);
extern FX2LP_BOOL fx2lp_write_reg(PFX2LP_PCD pfx2lp, u8 reg_addr, u8  data);
extern u8 		  fx2lp_get_ep_config_reg(u32 ep_num);
extern u8 		  fx2lp_get_ep_pktlen_reg  (u32 ep_num);
extern u8 		  fx2lp_get_ep_pf_reg (u32 ep_num);
extern irqreturn_t pcd_func_int_hndlr_isr (int irq, void *dev_id, struct pt_regs *regs);


static FX2LP_BOOL usb_ready_test_flag = FALSE;



/***********************************************************************************
 *** FX2LP MACROS
 ***********************************************************************************/

#define FULL_WORD_COUNT(N) (((N)+1)>>1)

MOD_PARM_STR (usbUseCase, "Use case for USB", NULL);



/* Crosslink ep and eps. Debug version should probably be removed */
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
void
_fx2lp_link_ep_eps(
    const char *func, 
    struct usbd_endpoint_instance *pep, 
    PEP_STATUS peps)
{
    pep->privdata = peps;
    peps->endpoint_inst = pep;
    printk ("\n---%s: crosslink ep %x(%d), eps %x(%d)", func, pep,
                peps->ep_num, peps, peps->epx);
}
#define fx2lp_link_ep_eps(PEP,PEPS) _fx2lp_link_ep_eps(__FUNCTION__, PEP, PEPS)
#else
#define fx2lp_link_ep_eps(PEP,PEPS) \
    (void)((PEP)->privdata=(PEPS),(PEPS)->endpoint_inst=(PEP))
#endif




/***********************************************************************************
 *** FX2LP EXTERN VARIABLES
 ***********************************************************************************/
extern FX2LP_PCD     fx2lp_pcd;
extern struct usbd_device_request request;
extern wait_queue_head_t  response_wait_queue;


/*
 * This is debug code.  It will never appear in the product.
 * It should probably be removed, but not yet.
 */
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_RWDATA)
void fx2lp_debugdump_rwdata(const char *tag, char *buf, int len)
{
    printk ("\n   %s (%d): ", tag, len);
    if (len > 32)
        len = 32;

    while (--len >= 0)
    {
        printk (" %02x", *buf++);
    }
    printk ("\n");
}
#else
#define fx2lp_debugdump_rwdata(T,B,L) ((void)0)
#endif

/***********************************************************************************
 *** FX2LP GLOBAL VARIABLES
 ***********************************************************************************/
FX2LP_BOOL    is_fx2lp_connected_to_host=FALSE;
FX2LP_BOOL    is_fx2lp_hstest_done=FALSE;

//-----------------------------------------------------------------------------------
//  linux kernel 2.4 FX2LP PCD specific structures
//-----------------------------------------------------------------------------------


int fx2lp_pcd_mod_init(void)
{
	return 0;
}


//-----------------------------------------------------------------------------------
//  linux kernel 2.6 style FX2LP PCD initialization
//-----------------------------------------------------------------------------------


/***********************************************************************************
 ***
 *** FX2LP PCD SPECIFIC FUNCTIONS
 ***
 ***********************************************************************************/
//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_send_cmd_irq
//  
// DESCRIPTION:      Send the given cmd to FX2LP firmware
//
// ARGUMENTS PASSED: pfx2lp  - fx2lp instance
//                   cmd     - Plus cmd
//
// RETURN VALUE:     None
//
// PRE-CONDITIONS:
//    Ready pin must be high.  Called only from ready pin handlers.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
// 
//-----------------------------------------------------------------------------------

static void 
fx2lp_send_cmd_irq(PFX2LP_PCD pfx2lp, u8 cmd, u8 data)
{
    volatile u16    *pfifo_addr;
    u16              val;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_REGISTERS)
    printk("\n----------FX2LP cmd 0x%x (%d)", cmd, data);
#endif
    TRACE_MSG2(PCD,"############ Send cmd (%x) with data (%d)\n", cmd, data);

    pfifo_addr = pfx2lp->pcmd_in_addr;
    val = cmd;
    val |= data<<8;
    
    // write value to FIFO
    *pfifo_addr = FX2LP_WORD_SWAP(val);
    // do a dummy write to interrupt address to commit (pull down RDY pin)
    fx2lp_gen_intr(pfx2lp, 0);
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_remote_wakeup
// 
// DESCRIPTION:      Initiate a remote wakeup to the host.
// 
// ARGUMENTS PASSED: param otg  - otg instance
//                   param flag - SET or RESET
// 
// RETURN VALUE:     None.
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static void 
fx2lp_remote_wakeup(struct otg_instance *otg, u8 flag)
{
    unsigned long flags;

    TRACE_MSG0(PCD, "FX2LP_REMOTE_WAKEUP: ");
    //local_irq_save (flags);
    // TBD - remote wakeup registers
    //local_irq_restore (flags);
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:          fx2lp_ep_config_ep0
// 
// DESCRIPTION:       Initialize ep0
// 
// ARGUMENTS PASSED:
//                    pfx2lp       - Local info
//                    max_pkt_size - EP size {64}
// RETURN VALUE:
//    None
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:   
//    None
//-----------------------------------------------------------------------------------
static void
fx2lp_ep_config_ep0(PFX2LP_PCD pfx2lp, u16 max_pkt_size)
{
    PEP_STATUS     peps = fx2lp_get_ep_status(pfx2lp, FX2LP_EP0);

    TRACE_MSG0(PCD,"--- Enter ---");

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk ("\nfx2lp_ep_config_ep0 (%d) %d", 
        max_pkt_size, peps->is_initialized);
#endif

    peps->pkt_size_assigned = max_pkt_size;
    if (!peps->is_initialized)
    {
        peps->do_out_data = fx2lp_do_out_0;
        peps->start_in_data = fx2lp_start_in_data_ep0;
        peps->is_initialized = TRUE;
        peps->bufsavail = 1;
    }
}


/*==========================================================================

FUNCTION:  fx2lp_ep_config

DESCRIPTION:
    Configure one FX2LP endpoint

ARGUMENTS PASSED:
    epx         Index of the endpoint
    ep_dir      Direction flag
    ep_type     Type of endpoint (BULK, INTERRUPT)
    max_pkt_size What it says

RETURN VALUE:
    FX2LP_BOOL  FAIL if epx is bad

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    A lot of the real work is done by the event handler.

==========================================================================*/
static FX2LP_BOOL
fx2lp_ep_config (FX2LP_EPX epx, u8 ep_dir, u8 ep_type, u16 max_pkt_size)                                                    
{ 
    PFX2LP_PCD     pfx2lp  = &fx2lp_pcd;
    struct pcd_instance *pcd = PFX2LP_PCD(pfx2lp);
    u8             reg_val = 0;
    u8             low_byte_pkt_size;
    u8             high_byte_pkt_size;
    u8             low_byte_count;
    u8             high_byte_count;
    u8             reg_offset;
    PEP_STATUS     peps;
    u8             bm_attributes = ep_dir | ep_type;
    u16            outintr;
    u16            inintr;
    FX2LP_RDY_EV_NUM_T cfgevt;

    TRACE_MSG0(PCD,"--- Enter ---");

    if(!(peps = fx2lp_get_ep_status(pfx2lp, epx)))
        return FAIL;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk ("\nfx2lp_ep_config(%d(%d),%02x,%02x,%d)",
        epx, peps->ep_num, ep_dir, ep_type, max_pkt_size);
#endif

    switch (epx)
    {
        case FX2LP_EP0: 
            fx2lp_ep_config_ep0 (pfx2lp, max_pkt_size);
            break;

        case FX2LP_EP1: 
            peps->pkt_size_assigned = max_pkt_size;
            
            // Write the endpoint configuration
            reg_val  =  (EP1IN_CFG_DEFAULT_VALUE | FX2LP_EP_VALID_FLAG)
                            & ~FX2LP_EP_STALL_FLAG;
            reg_val |=  (ep_type<<4);             // Transfer type
            peps->cfg_reg_val =  reg_val;
            peps->is_initialized = TRUE;
            // EP1OUT init - let FX2LP firmware configure it for us!
            peps->doconfig |= FX2LP_DO_CONFIG;
            fx2lp_schedule_ready_event_irq(pfx2lp, FX2LP_RDY_EV_N_EP1_CFG);

            peps->do_out_data = NULL;   /* shouldn't ever happen */
            peps->start_in_data = fx2lp_start_in_data_ep1;
            peps->bufsavail = 1;
            break;

        // EP2 and EP6 are configurable
        case FX2LP_EP2:
        case FX2LP_EP6:
#ifdef FX2LP_ACM_URB_FIX            
           if(strstr (pcd->bus->function_instance->function_driver->name, "acm") && 
                      bm_attributes == (USB_DIR_OUT | USB_ENDPOINT_BULK) &&
                      pcd->bus->high_speed == BS_HIGH_SPEED)
                peps->pkt_size_assigned = EP2_PACKET_SIZE_FS;
           else              
#endif
           peps->pkt_size_assigned = (pcd->bus->high_speed == BS_HIGH_SPEED )? EP2_PACKET_SIZE_HS : EP2_PACKET_SIZE_FS;


            // Write the endpoint configuration
            if (epx == FX2LP_EP2) 
            {
                reg_val =  EP2_CFG_DEFAULT_VALUE & 
                    ~(FX2LP_EP_STALL_FLAG | FX2LP_EP_DIRECTION_IN);
                outintr = INT_M_EP2OUTNE_E;
                inintr = INT_M_EP2INNF_E;
                cfgevt = FX2LP_RDY_EV_N_EP2_CFG;
            }
            else
            {
                reg_val =  EP6_CFG_DEFAULT_VALUE & 
                    ~(FX2LP_EP_STALL_FLAG | FX2LP_EP_DIRECTION_IN);
                outintr = INT_M_EP6OUTNE_E;
                inintr = INT_M_EP6INNF_E;
                cfgevt = FX2LP_RDY_EV_N_EP6_CFG;
            }

            if (ep_dir == USB_DIR_OUT)             
            {
                peps->doconfig |= (FX2LP_DO_DATACOMMIT |
                            FX2LP_DO_EPXPFH | FX2LP_DO_EPXPFL);
                pfx2lp->eintr_en |= outintr;
#if FX2LP_OUT_AS_BH
                PREPARE_WORK_ITEM (peps->start_rxtx,
                        fx2lp_do_out_data_ep26_bh, peps->endpoint_inst);
#endif
            }
            else
            {
                peps->bufsavail = 2;
                reg_val |= FX2LP_EP_DIRECTION_IN;  
                pfx2lp->eintr_en |= inintr;
#if FX2LP_IN_AS_BH
                PREPARE_WORK_ITEM (peps->start_rxtx,
                        fx2lp_do_in_data_ep26_bh, peps->endpoint_inst);
#endif
            }
                
            peps->outinfo.outavail = 0;
            reg_val |= 0x2                      // Double Buffering
                    | FX2LP_EP_VALID_FLAG
                    | (ep_type<<4);             // Transfer type

            peps->cfg_reg_val =  reg_val;

            peps->do_out_data = fx2lp_do_out_26_irq;
            peps->start_in_data = fx2lp_start_in_data_ep26_irq;
            peps->is_initialized = TRUE;
            peps->doconfig |= 
                FX2LP_DO_PKTLENH | FX2LP_DO_PKTLENL | FX2LP_DO_CONFIG;

            TRACE_MSG0(PCD,"--- Modify Interrupt Enable 1 register -----");
            fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_INTR_EN1);
            fx2lp_schedule_ready_event(pfx2lp, cfgevt);
            break;

        default:
            break;
    }

    peps->outinfo.rcd_pktlen = peps->pkt_size_assigned;
    peps->outinfo.shortpend = -1;

    return PASS;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:          fx2lp_start_device
// 
// DESCRIPTION:       This routine start the device and set up the endpoints 
//                    based on the enumeration info.
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static FX2LP_BOOL
fx2lp_start_device(struct pcd_instance *pcd)
{
    struct usbd_bus_instance *bus = pcd->bus;
    struct usbd_endpoint_instance *endpoint;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\n%s : %d", __FUNCTION__, __LINE__);
#endif
    TRACE_MSG0(PCD,"--- Enter --- ");

    /* fix up EP0  */
    endpoint = bus->endpoint_array;
    endpoint->new_wMaxPacketSize[0] = endpoint->new_wMaxPacketSize[1] 
        = EP0_PACKET_SIZE;

    return PASS;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_stop_device
// 
// DESCRIPTION:      stop the fx2lp device
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
FX2LP_BOOL
fx2lp_stop_device(PFX2LP_PCD pfx2lp)
{
    PEP_STATUS   peps;
    FX2LP_EPX    epx;

    TRACE_MSG0(PCD,"Enter");
    if(pfx2lp == NULL)
        return FAIL;

    for (epx = 0; epx < FX2LP_NUM_EPS; ++epx) 
    {
        peps = fx2lp_get_ep_status(pfx2lp, epx);
        if(peps && peps->is_initialized)
        {
            peps->is_initialized = FALSE;
        }
    }    

    
    return PASS;    
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:          fx2lp_pcd_start
// 
// DESCRIPTION:       start the FX2LP
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static void 
fx2lp_pcd_start (struct pcd_instance *pcd)
{
    TRACE_MSG0(PCD, "Enter");
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\nfx2lp_pcd_start:--------");
#endif
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:           fx2lp_pcd_stop
// 
// DESCRIPTION:        stop the FX2LP device
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static void 
fx2lp_pcd_stop (struct pcd_instance *pcd)
{
    TRACE_MSG0(PCD, "FX2LP STOP");
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_pcd_disable
// 
// DESCRIPTION:      disable the pcd 
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
void fx2lp_pcd_disable(struct pcd_instance *pcd)
{
    PFX2LP_PCD                pfx2lp = PCD_PFX2LP(pcd);

    TRACE_MSG0(PCD, "Enter");
    /* if fx2 already disabled, return */
    if(is_fx2lp_connected_to_host == false) {
       printk("Disable called again\n");
       TRACE_MSG0(PCD, "Disable called again");
       return;
    } 

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\n%s : %d", __FUNCTION__, __LINE__);

    printk("\n +++++++++++++++++++++++++++++++++ I am in ---- (%s:%d)  ",__FUNCTION__,__LINE__);
#endif
    TRACE_MSG0(PCD, "Disable the pcd..");

    is_fx2lp_connected_to_host = FALSE;

    if(pfx2lp->device_state == FX2LP_SUSPENDED)
    {
        gpio_usb_hs_wakeup_set_data(GPIO_HIGH);
	gpio_usb_hs_wakeup_set_data(GPIO_LOW); 
    }

    msleep(100);

    fx2lp_disconnect_device (pfx2lp);
    //Set the connected flag to FALSE so that we can re-enumerate on mode switching.
    // Note comment remains above but flag being set to false was removed in 6.1 version

    // reset all the structure variables and pointers. 
    fx2lp_pcd_init();
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_pcd_disable_ep
// 
// DESCRIPTION:      disable the EP 
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
void fx2lp_pcd_disable_ep(struct pcd_instance *pcd, unsigned int ep)
{
     TRACE_MSG0(PCD, "--- Enter ---");
}


//-----------------------------------------------------------------------------------
// 
// FUNCTION:          fx2lp_pcd_assign_endpoint
// 
// DESCRIPTION:
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static int 
fx2lp_pcd_assign_endpoint(FX2LP_EPX epx, 
    struct usbd_endpoint_map *endpoint_map, 
    u16 is_special_case, 
                          struct usbd_endpoint_request *requested_endpoints) 
{
    int hs = 0;
    u8 physical_endpoint = fx2lp_epx_info[epx].phys_epn;

    endpoint_map->index = requested_endpoints->index;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\nfx2lp_pcd_assign_endpoint(%d) %x %x",
        epx, physical_endpoint, requested_endpoints->bmAttributes);
#endif

    // we support both FS and HS. So, update the endpoint_map accordingly
    for (hs = 0; hs <=1; hs++)
    {
        // Common params (For Full Speed and HiSpeed)
        endpoint_map->bEndpointAddress[hs] = physical_endpoint | (USB_DIR_IN & requested_endpoints->bmAttributes);
        endpoint_map->physicalEndpoint[hs] = (physical_endpoint * 2) + ((USB_DIR_IN & requested_endpoints->bmAttributes) ? 1 : 0); 
        endpoint_map->bmAttributes[hs] = requested_endpoints->bmAttributes;

        if (physical_endpoint < 2 || hs == 0)
        {    // for EP0 and EP1
            endpoint_map->wMaxPacketSize[hs] = 0x40;
            endpoint_map->transferSize[hs] = requested_endpoints->fs_requestedTransferSize;
        }
        else
        {
#ifdef FX2LP_ACM_URB_FIX            
            if(is_special_case)
                {
                endpoint_map->wMaxPacketSize[hs] = 0x40;
                endpoint_map->transferSize[hs] = requested_endpoints->fs_requestedTransferSize;
                }
            else
#endif                
                {
                endpoint_map->wMaxPacketSize[hs] = 0x200;
                endpoint_map->transferSize[hs] = requested_endpoints->hs_requestedTransferSize;
                }
        }
        
        endpoint_map->bInterval[hs] = requested_endpoints->bInterval;
    }
    return 0;
}


//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_pcd_request_endpoints
// 
// DESCRIPTION:
//                  pcd                 - 
//                  endpoint_map_array  -
//                  endpoints_requested -
//                  requested_endpoints -
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static int 
fx2lp_pcd_request_endpoints(struct pcd_instance *pcd,     
        struct usbd_endpoint_map     *endpoint_map_array, 
        int  endpoints_requested,     
        struct usbd_endpoint_request *requested_endpoints)
{
    struct usbd_bus_instance *bus = pcd->bus;
    int                             i;
    PFX2LP_PCD                      pfx2lp = PCD_PFX2LP(pcd);
    FX2LP_EPX                       epx;
    struct usbd_endpoint_instance   *ep;
    PEP_STATUS    peps;

    TRACE_MSG1(PCD, "REQUEST ENDPOINTS: %d", endpoints_requested);
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("REQUEST ENDPOINTS: %d", endpoints_requested);
#endif

    /* there ought to be a better place to do this, but get
       ep0 crosslinking done now */
    peps = &pfx2lp->pep_status[FX2LP_EP0];
    peps->epx = FX2LP_EP0;
    peps->ep_num = 0;
    fx2lp_link_ep_eps(&bus->endpoint_array[0], peps);

    //assign the high_speed flag. 
    //assume full speed.  If it's really high speed, we'll get
    //an interrupt telling us that.
    bus->high_speed = BS_FULL_SPEED;
    init_fx2lp_epn(pfx2lp);
    for (i = 0; i < endpoints_requested; i++) 
    {
        struct usbd_endpoint_map *endpoint_map = endpoint_map_array + i;
        u16    is_special_case = FALSE;
        if(!(epx = fx2lp_get_priority_based_epx(pfx2lp, 
                requested_endpoints[i].bmAttributes)))
        {
            return -EINVAL;
        }
// Fix KW error        
        if(epx >= FX2LP_INVALID)
        {
            TRACE_MSG1(PCD,"Invalid ep: %d\n",epx);
            return -EINVAL;
        }


#ifdef FX2LP_ACM_URB_FIX            
        if(strstr (bus->function_instance->function_driver->name, "acm") && 
            requested_endpoints[i].bmAttributes == (USB_DIR_OUT | USB_ENDPOINT_BULK) && 
            bus->high_speed == BS_HIGH_SPEED)
            {
            printk("\n %s : For ACM, configuring Hispeed 512 byte bulk out endpoint to 64 byte.\n",__FUNCTION__);
            is_special_case = TRUE;
            }
#endif            
        fx2lp_pcd_assign_endpoint (epx, endpoint_map, is_special_case, &requested_endpoints[i]);

        /* crosslink endpoint_instance and endpoint status */
        ep = bus->endpoint_array + 
                endpoint_map->physicalEndpoint[bus->high_speed];
        peps = &pfx2lp->pep_status[epx];
        peps->ep_num = fx2lp_epx_info[epx].phys_epn;
        peps->epx = epx;
        fx2lp_link_ep_eps(ep, peps);
    }
    return 0;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_pcd_set_endpoints
// 
// DESCRIPTION:      setup the physical endpoints for the endpoint map
// 
// ARGUMENTS PASSED:
//                    pcd                 -
//                    endpoints_requested -
//                    endpoint_map_array  -
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static int 
fx2lp_pcd_set_endpoints (struct pcd_instance *pcd, int endpoints_requested, struct usbd_endpoint_map *endpoint_map_array)
{
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\nfx2lp_pcd_set_endpoints(%d)", endpoints_requested);
#endif
    TRACE_MSG0(PCD, "Enable system control interrupts");
    // TBD          
    return 0;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_pcd_set_address
// 
// DESCRIPTION:      set the USB address for this device
// 
// ARGUMENTS PASSED:
//                    pcd     -
//                    address -
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    This shouldn't be called. FX2LP firmware should handle this.
// 
//-----------------------------------------------------------------------------------
static void inline
fx2lp_pcd_set_address (struct pcd_instance *pcd, u8 address)
{        
    return;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_pcd_setup_ep
// 
// DESCRIPTION:      setup the given endpoint
// 
// ARGUMENTS PASSED:
//                   pcd      - pcd instance
//                   epn      - endpoint number
//                   endpoint - endpoint instance
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static void 
fx2lp_pcd_setup_ep (struct pcd_instance *pcd, unsigned int epn, struct usbd_endpoint_instance *endpoint)
{
    int hs = pcd->bus->high_speed;
    PFX2LP_PCD    pfx2lp = PCD_PFX2LP(pcd);
    PEP_STATUS    peps;
    FX2LP_EPX     epx;

    peps = (PEP_STATUS)endpoint->privdata;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("\nfx2lp_pcd_setup_ep(%d, %d, %x, %x)", epn, 
        peps ? peps->epx : -1,
        endpoint, peps);
#endif
    /* this gets called for all endpoints, but most aren't used */
    if (peps != NULL)
    {
        TRACE_MSG2(PCD, "epn[%d] START EPN: config %02x", epn, endpoint->new_bEndpointAddress[hs]);
        epx = peps->epx;

        if (epn && endpoint->new_bEndpointAddress[hs])
        {
            fx2lp_ep_config(epx, 
                endpoint->new_bEndpointAddress[hs] & USB_DIR_IN,
                endpoint->new_bmAttributes[hs] & USB_ENDPOINT_MASK,
                endpoint->new_wMaxPacketSize[hs]);
        }

        //ask FX2LP to connect to the host
        if (is_fx2lp_connected_to_host == FALSE)
        {
            is_fx2lp_connected_to_host = TRUE;

            gpio_usb_hs_wakeup_set_data(GPIO_HIGH);
            gpio_usb_hs_wakeup_set_data(GPIO_LOW); 

            //Enable BUS activity wakeup.
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
            printk("Send BUS ACTIVITY CMD from setup_ep\n");
#endif
            fx2lp_schedule_ready_event (pfx2lp, FX2LP_RDY_EV_N_BUS_ACTIVE);

            // send enumerate. This is for master controlled enumeration.
            // Connects to USB. This command is sent after sending the 
            // Pass through On cmd when the external master is ready to
            // handle enumerations standard requests
            fx2lp_schedule_ready_event_irq(pfx2lp, FX2LP_RDY_EV_N_ENUMERATE);
        }
    }
    return;
}               

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_endpoint_halted
// 
// DESCRIPTION:      check if the given endpoint is stalled or not
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static int 
fx2lp_endpoint_halted (struct pcd_instance *pcd, struct usbd_endpoint_instance *endpoint)
{
    PEP_STATUS    peps = (PEP_STATUS)endpoint->privdata;

    /* Can only read register in special cases */
    return ((peps->cfg_reg_val & FX2LP_EP_STALL_FLAG) != 0);
}
//-----------------------------------------------------------------------------------
// 
// FUNCTION:           fx2lp_halt_endpoint
// 
// DESCRIPTION:        stall or unstall the given endpoint
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
int 
fx2lp_halt_endpoint(
    struct pcd_instance *pcd, 
    struct usbd_endpoint_instance *endpoint, 
    int flag)
{
    PFX2LP_PCD    pfx2lp = PCD_PFX2LP(pcd);
    PEP_STATUS    peps = (PEP_STATUS)endpoint->privdata;

    if (peps->epx == FX2LP_EP0)
        return 0;

    if (flag)
    {
        peps->cfg_reg_val |= FX2LP_EP_STALL_FLAG;
    }
    else
    {
        peps->cfg_reg_val &= ~FX2LP_EP_STALL_FLAG;
    }
    fx2lp_schedule_ready_event(pfx2lp, fx2lp_epx_info[peps->epx].stallev);

    return 0;
}


//-----------------------------------------------------------------------------------
// 
// FUNCTION:          fx2lp_pcd_cancel_in_irq
// 
// DESCRIPTION:       cancel IN urb
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static void 
fx2lp_pcd_cancel_in_irq (struct pcd_instance *pcd, struct usbd_urb *urb)
{
    struct usbd_endpoint_instance *endpoint = urb->endpoint;
    int    epn = endpoint->new_bEndpointAddress[0] & 0x7f;
    PFX2LP_PCD        pfx2lp = (PFX2LP_PCD) pcd->privdata;
    PEP_STATUS     peps = fx2lp_get_ep_status(pfx2lp, FX2LP_EP6);
 
    TRACE_MSG1(PCD,"Cancelling the FIFO for ep %d", epn);
// This should be always EP6 for us
    fx2lp_write_reg_worsp_irq(pfx2lp, FX2LP_REG_EPINPKTEND, FX2LP_FLAG_FLUSH_FIFO6);
// Indicate that both the FIFOs have been flushed.
    peps->bufsavail = 2;

    return;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:          fx2lp_pcd_cancel_out_irq
// 
// DESCRIPTION:       cancel OUT urb
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES: 
//    None.
// 
//-----------------------------------------------------------------------------------
static void inline
fx2lp_pcd_cancel_out_irq (struct pcd_instance *pcd,struct usbd_urb *urb)
{
    struct usbd_endpoint_instance *endpoint = urb->endpoint;
    int    epn = endpoint->new_bEndpointAddress[0] & 0x7f;
    // Need this for SDMA
    return;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:          fx2lp_handle_outne
// 
// DESCRIPTION:       
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES: 
//    None.
// 
//-----------------------------------------------------------------------------------
static void
fx2lp_handle_outne(
    PFX2LP_PCD          pfx2lp,
    PEP_STATUS          peps)
{

    if (peps == NULL) 
    {
       TRACE_MSG0(PCD,"*** peps is NULL");
       printk ("\nfx2lp_handle_outne, peps is NULL\n");
       return;
    }

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    printk ("\nINT_M_OUTNE(%d) %d %d", 
        peps->ep_num,
        (peps->outinfo.rcd_pktlen > 0),
        peps->bufsavail);
#endif

    if (peps->endpoint_inst == NULL) 
    {
       TRACE_MSG0(PCD,"*** endpoint_inst is NULL");
       printk ("\nfx2lp_handle_outne, endpoint_inst is NULL\n");
       return;
    }

    peps->outinfo.outavail++;

    if (peps->endpoint_inst->rcv_urb != NULL)
    {
        fx2lp_do_out_26_irq(pfx2lp, peps->endpoint_inst);
    }
    return;
}

static void
fx2lp_handle_short_notify(
    PFX2LP_PCD          pfx2lp,
    PEP_STATUS          peps,
    u16                 len)
{
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    printk ("\nINT_M_EP%d6SHORTPKTAVAIL (%d)", peps->ep_num, len);
#endif
    peps->outinfo.shortpend = len;

    fx2lp_req_short_packet(pfx2lp, peps);
    return;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_handle_pcd_isr
// 
// DESCRIPTION:      fx2lp interrupt handler
// 
// ARGUMENTS PASSED:
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
// 
//-----------------------------------------------------------------------------------
irqreturn_t 
fx2lp_handle_pcd_isr(int irq, void *dev_id, struct pt_regs *regs)
{
    PFX2LP_PCD           pfx2lp = &fx2lp_pcd;
    struct pcd_instance *pcd = PFX2LP_PCD(pfx2lp);
    u32                  flags;
    u8                   epn;
    PEP_STATUS           peps;
    u16                  intr_source;

    if(!pfx2lp)
    {
        printk("\nError... pfx2lp is NULL\n ");
        return IRQ_NONE;
    }	
    if (pcd == NULL)
    {
#if FX2LP_DETAIL_KPF
        printk ("\nfx2lp_handle_pcd_isr: Correcting NULL pcd!\n");
#endif
        pcd = pcd_instance;
    }

    intr_source = FX2LP_WORD_SWAP(*(pfx2lp->pintr_addr));
    TRACE_MSG1(PCD,"Enter HandleUSBInterrupt: %x\n",intr_source);

    if (usb_ready_test_flag == FALSE)
    {
       if ( (intr_source & 0xFF) != INT_M_READY)
       {

         TRACE_MSG1(PCD,"First interrupt: 0x%x from FX2 chip is not a ready - ignore ",intr_source);
         printk("First interrupt: 0x%x from FX2 chip is not a ready - ignore ",intr_source);
         return IRQ_HANDLED;
       }
    }


    if (intr_source & INT_M_INTMODE)           // Is an Extended Interrupt ?
    {
        struct usbd_endpoint_instance *endpoint;

        if(intr_source & INT_M_READREGRESP)
        {
            TRACE_MSG0(PCD,"####### INTR -> INT_M_READREGRESP");
            pfx2lp->wout_read_reg_resp_cmd = intr_source & 0x3FFF;
            pfx2lp->is_read_response_received = TRUE;
            wake_up(&response_wait_queue);
    		return IRQ_HANDLED;
        }

        switch(intr_source & INT_B_EXTND_MASK) // check only lower byte
        {
            case INT_M_EP2SHORTPKTAVAIL:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_EP2SHORTPKTAVAIL");
                peps = &pfx2lp->pep_status[FX2LP_EP2];
                if (peps == NULL) 
                {
                    TRACE_MSG0(PCD,"*** peps is NULL");
                    printk ("\nfx2lp_handle_pcd_isr, EP2shortpktavail: peps is NULL\n");
                    break;
                }
                fx2lp_handle_short_notify (pfx2lp, peps, 
                    intr_source & INT_B_PKTLEN);
                break;

            case INT_M_EP6SHORTPKTAVAIL:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_EP6SHORTPKTAVAIL");
                peps = &pfx2lp->pep_status[FX2LP_EP6];
                if (peps == NULL) 
                {
                    TRACE_MSG0(PCD,"*** peps is NULL");
                    printk ("\nfx2lp_handle_pcd_isr, EP6shortpktavail: peps is NULL\n");
                    break;
                }
                fx2lp_handle_short_notify (pfx2lp, peps, 
                    intr_source & INT_B_PKTLEN);
                break;
                
                
#ifdef FX2LP_OBSOLETE_CODE
            case INT_M_WRITEREGRESP:
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_REGISTERS)
                printk("\n INTR -> INT_M_WRITEREGRESP\n");
#endif
                TRACE_MSG0(PCD,"####### INTR -> INT_M_WRITEREGRESP");
                pfx2lp->wout_write_reg_resp_cmd = intr_source & 0xFF;
                pfx2lp->is_write_response_received = TRUE;
                wake_up_interruptible(&response_wait_queue);
                break;
#endif
                
            case INT_M_EP1INBUFAVAIL:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_EP1INBUFAVAIL");
                peps = &pfx2lp->pep_status[FX2LP_EP1];
                if (peps == NULL) 
                {
                    TRACE_MSG0(PCD,"*** peps is NULL");
                    printk ("\nfx2lp_handle_pcd_isr, EP1inbufavail: peps is NULL\n");
                    break;
                }
                peps->bufsavail = 1;
                endpoint = peps->endpoint_inst;
                if (endpoint->tx_urb)
                {
                    /* write an EP1 packet as soon as possible */
                    fx2lp_schedule_ready_event_irq(pfx2lp, 
                        FX2LP_RDY_EV_N_WRITE_EP1);
                }
                break;
                
            case INT_M_CMDRDY:
#if (FX2LP_DETAIL_KPF & (FX2LP_DETAIL_REGISTERS|FX2LP_DETAIL_EVENTS))
                printk("\n INTR -> INT_M_CMDRDY\n");
#endif
                TRACE_MSG0(PCD,"####### INTR -> INT_M_CMDRDY");
                peps = &pfx2lp->pep_status[FX2LP_EP6];
                if(peps->sending_in_short)
                peps->sending_in_short = FALSE; 
                pfx2lp->is_cmd_rdy = TRUE;
                fx2lp_do_ready_evt_irq(pfx2lp);
                break;

            case INT_M_OUTNE:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_OUTNE");

                if ((intr_source & INT_M_EPNO) == 2)
                {
                    peps = &pfx2lp->pep_status[FX2LP_EP2];
                }
                else
                {
                    peps = &pfx2lp->pep_status[FX2LP_EP6];
                }
                if (peps == NULL) 
                {
                    TRACE_MSG0(PCD,"*** peps is NULL");
                    printk ("\nfx2lp_handle_pcd_isr, INT_M_OUTNE: peps is NULL\n");
                    break;
                }
                fx2lp_handle_outne (pfx2lp, peps);
                break;

            case INT_M_INNF:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_INNF");
                if ((intr_source & INT_M_EPNO) == 2)
                {
                    peps = &pfx2lp->pep_status[FX2LP_EP2];
                }
                else
                {
                    peps = &pfx2lp->pep_status[FX2LP_EP6];
                }
                if (peps == NULL) 
                {
                    TRACE_MSG0(PCD,"*** peps is NULL");
                    printk ("\nfx2lp_handle_pcd_isr, INT_M_INNF: peps is NULL\n");
                    break;
                }
                endpoint = peps->endpoint_inst;

                peps->bufsavail++;
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
                printk ("\nINT_M_INNF(%d) bav %d, ur %x", intr_source & INT_M_EPNO,
                    peps->bufsavail, endpoint->tx_urb);
#endif
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_COPYSUMMARY)
                printk (" i%c", (peps->bufsavail == 1) ? '-' : '+');
#endif
                if (peps->bufsavail >= 1)
                {
                    /* try to move more data */
                    fx2lp_start_in_data_ep26_irq(pfx2lp, endpoint);
                }
                break;


            case INT_M_SUSPEND:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_BUSSUSPEND");
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
                printk("\nGetting a SUSPEND interrupt\n");
#endif
                if(PENDING_WORK_ITEM(pfx2lp->bus_suspend_bh)) 
                {
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
                  printk(KERN_ERR"%s: SUSPEND bh already scheduled\n", __FUNCTION__);
#endif
                }
                else
                {
		  if(g_usb_bus_disable_ing()){printk("\n LYN BUS disable_ing, ret from suspend \n");break;}
                  SCHEDULE_WORK(pfx2lp->bus_suspend_bh);
                }
                break;

            case INT_M_RESUME:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_BUSRESUME");
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
                printk("Got a BUSRESUME\n");
#endif
                if(PENDING_WORK_ITEM(pfx2lp->bus_resume_bh)) 
                {
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
                  printk(KERN_ERR"%s: RESUME bh already scheduled\n", __FUNCTION__);
#endif
                }
                else
                {
		  if(g_usb_bus_disable_ing()){printk("\n LYN BUS disable_ing, ret from resume \n");break;}
                  SCHEDULE_WORK(pfx2lp->bus_resume_bh);
                }
                break;
        
            case INT_M_HS:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_HS");
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
                printk("\n INTR -> INT_M_HS\n");
#endif
                pcd->bus->high_speed = BS_HIGH_SPEED;
                break;

            case INT_M_INSHORTPKTACK:
                {
                    static FX2LP_BOOL report = TRUE;
                    if (report)
                    {
                        report = FALSE;
                        printk ("\n\n-------GOT INT_M_INSHORTPKTACK-------\n\n");
                    }
                }
                break;
        
            default:
                printk ("\nFX2LP: Intr received 0x%x", intr_source);
                break;
           }
    }
    else // standard interrupt
    {
        switch(intr_source & 0xFF)
        {
            case INT_M_READY:
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
                printk("\nINTR -> INT_M_READY\n");
#endif
                TRACE_MSG0(PCD,"####### INTR -> INT_M_READY");
                printk("####### INTR -> INT_M_READY RECEIVED");
                usb_ready_test_flag = TRUE;
                 

                fx2lp_ep_config_ep0(pfx2lp, 64);

                // wake up the queue in modinit
                wake_up_interruptible(&wkupfail_wait_queue);

                fx2lp_schedule_ready_event_irq(pfx2lp,
                                FX2LP_RDY_EV_N_SET_PTHRU);
                break;

            case INT_M_CMDSTATRDY:
                TRACE_MSG0(PCD,"####### INTR -> INT_M_CMDSTATRDY");
				fx2lp_handle_cmd_out(pfx2lp);
                break;
                
            case INT_M_USBRESET:
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
                printk ("\nINTR -> INT_M_USBRESET\n");
#endif
                TRACE_MSG0(PCD,"####### INTR -> INT_M_USBRESET");
                fx2lp_enable_interrupts(pfx2lp);

                fx2lp_schedule_ready_event_irq(pfx2lp,
                                FX2LP_RDY_EV_N_USBRESET);
              	break;

            case INT_M_EP0BUF: 
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
                printk("\n INTR -> INT_M_EP0BUF\n");
#endif
                TRACE_MSG0(PCD,"####### INTR -> INT_M_EP0BUF");
                peps = &pfx2lp->pep_status[FX2LP_EP0];
                if (peps == NULL) 
                {
                    TRACE_MSG0(PCD,"*** peps is NULL");
                    printk ("\nfx2lp_handle_pcd_isr, INT_M_EP0BUF: peps is NULL\n");
                    break;
                }
                peps->bufsavail = 1;
                if (peps->endpoint_inst->tx_urb != NULL)
                {
                    fx2lp_schedule_ready_event_irq(pfx2lp, 
                            FX2LP_RDY_EV_N_WRITE_EP0);
                }
                break;


            case INT_M_SETUP:
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
                printk("\n INTR -> INT_M_SETUP\n");
#endif
                TRACE_MSG0(PCD,"####### INTR -> INT_M_SETUP");
                fx2lp_read_setup_pkt_irq(pfx2lp);
                break;

            case INT_M_FLAGS:
                {
                    static FX2LP_BOOL report = TRUE;
                    if (report)
                    {
                        report = FALSE;
                        printk ("\n\n-------GOT INT_M_FLAGS-------\n\n");
                    }
                }
                break;

            default:
                printk ("\nFX2LP: Unexpected intr 0x%x", intr_source);
                break;
           }
    }
    return IRQ_HANDLED;
}

//-----------------------------------------------------------------------------------
// 
// FUNCTION:         fx2lp_pcd_en_func
// 
// DESCRIPTION:      enable the fx2lp PCD and USBD stack
// 
// ARGUMENTS PASSED:
//                   otg  - otg instance
//                   flag - SET:enable or RESET:disable
// 
// RETURN VALUE:
// 
// PRE-CONDITIONS:
//    None.
// 
// POST-CONDITIONS:
//    None.
// 
// IMPORTANT NOTES:
//    None.
// 
//-----------------------------------------------------------------------------------
static void 
fx2lp_pcd_en_func (struct otg_instance *otg, u8 flag)
{
    unsigned long             flags;
    struct pcd_instance      *pcd = otg->pcd;
    PFX2LP_PCD                pfx2lp  = PCD_PFX2LP(pcd);
    struct usbd_bus_instance *bus = pcd->bus;
    int                       epn;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk ("\nfx2lp_pcd_en_func(%s) called %x %x",
            (flag == SET) ? "SET" : "RESET",
            pfx2lp, pcd);
#endif

    switch (flag) {
    case SET:
        TRACE_MSG0(PCD, "PCD_EN: SET" );
        printk("PCD_EN:SETTTTTTT\n");
        fx2lp_setup_dev (pfx2lp);
        fx2lp_start_device(pcd);
        pcd->active = TRUE;
        break;

    case RESET:
        pcd->active = FALSE;
        // TBD : what are we supposed to do?
        //       stop device and all EPs? or ?
        //       check what fx2lp is doing indepth!
        //       also disconnect the device
        usbd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
        usbd_bus_event_handler_irq (bus, DEVICE_DESTROY, 0);
        break;
    }
}

//-----------------------------------------------------------------------------------
//  LINUX / USB CORE SPECIFIC GLOBALS
//-----------------------------------------------------------------------------------

struct 
usbd_pcd_ops usbd_pcd_ops = {
    .bmAttributes          = 0x80,  // sree : check HW
    .max_endpoints         = FX2LP_MAX_ENDPOINTS,
    .high_speed_capable    = TRUE,
    .ep0_packetsize        = FX2LP_EP0_PACKETSIZE,
    .capabilities          = REMOTE_WAKEUP_SUPPORTED, // sree : check HW, i guess it is supported!
    .name                  = FX2LP_NAME,
    .start                 = fx2lp_pcd_start,
    .stop                  = fx2lp_pcd_stop,
    .disable               = fx2lp_pcd_disable,
    .disable_ep            = fx2lp_pcd_disable_ep,
    .start_endpoint_in     = fx2lp_pcd_start_endpoint_in,
    .start_endpoint_out    = fx2lp_pcd_start_endpoint_out,
    .request_endpoints     = fx2lp_pcd_request_endpoints,
    .set_endpoints         = fx2lp_pcd_set_endpoints,
    .set_address           = fx2lp_pcd_set_address,
    .setup_ep              = fx2lp_pcd_setup_ep,
    .halt_endpoint         = fx2lp_halt_endpoint,
    .endpoint_halted       = fx2lp_endpoint_halted,
    .cancel_in_irq         = fx2lp_pcd_cancel_in_irq,
    .cancel_out_irq        = fx2lp_pcd_cancel_out_irq
};

//-----------------------------------------------------------------------------------
//  linux kernel 2.6 specific structures
//-----------------------------------------------------------------------------------

    int fx2lp_pcd_mod_init_l26(void);
    void fx2lp_pcd_mod_exit_l26(void);

    struct pcd_ops pcd_ops = 
    {
        .mod_init = fx2lp_pcd_mod_init_l26,     // called for module init
    #ifdef MODULE
        .mod_exit = fx2lp_pcd_mod_exit_l26,     // called for module exit
    #endif
        .pcd_en_func = fx2lp_pcd_en_func,
        .pcd_init_func = pcd_init_func,
        .remote_wakeup_func = fx2lp_remote_wakeup,
    };

int fx2lp_pcd_mod_init_l26(void)
{
    int platform_registered = 0;
    int retval=0;
    u32 timeout = 100; // 1Second in jiffies
    PFX2LP_PCD pfx2lp = &fx2lp_pcd;
    PEP_STATUS peps = fx2lp_get_ep_status(pfx2lp, FX2LP_EP0);
    FX2LP_BOOL *flag = &peps->is_initialized; 
    int val=0;

    usb_ready_test_flag = FALSE;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk("--- Entering fx2lp_pcd_mod_init_l26 ---\n");
#endif
    TRACE_MSG0(PCD, "--- Enter ---");


#ifdef CONFIG_OTG_GENERIC_HOTPLUG    
    usb_hstest_hotplug.function_instance = NULL;
    usb_hstest_hotplug.dev_name = "usb_hstest";
    hotplug_init(&usb_hstest_hotplug);
#endif

    // initialize the FX2LP related data structures. Moved here from pcd.c
    pfx2lp->is_pcd_mod_init = TRUE;
    fx2lp_pcd_init();


    // register with interrupt handlers
    retval = gpio_usb_hs_int_request_irq (pcd_func_int_hndlr_isr, OTG_INTERRUPT, "FX2LP INTR", NULL);
    if(retval<0)
    {
        printk("\n*********************************************\n");
        printk("\n REQUEST_IRQ failed..  : %d", retval);
        printk("\n*********************************************\n");
        return -1;
    }
    // wakeup the chip.. Check if this is working. Currently we use a non sleeping firmware.
    TRACE_MSG2(PCD,"Pulling the wakeup HIGH in %s at %d ", __FUNCTION__, __LINE__);
    //printk("Pulling the wakeup HIGH in %s at %d ", __FUNCTION__, __LINE__);

    gpio_usb_hs_wakeup_set_data(GPIO_HIGH);
    
    TRACE_MSG2(PCD,"Pulling the wakeup LOW in %s at %d ", __FUNCTION__, __LINE__);
    //printk("Pulling the wakeup LOW in %s at %d ", __FUNCTION__, __LINE__);
    gpio_usb_hs_wakeup_set_data(GPIO_LOW); 

#ifdef CONFIG_OTG_GENERIC_HOTPLUG
    TRACE_MSG1(PCD, "hstest_done [%d]", is_fx2lp_hstest_done);
    
    // do this the first time fx2 is being initialized after a reset
    if ( is_fx2lp_hstest_done == FALSE )

    {
        val = wait_event_interruptible_timeout(wkupfail_wait_queue, (usb_ready_test_flag == TRUE), timeout);
        // determine if there's time left on the timer (or we caught a signal)
        if ( (usb_ready_test_flag != TRUE) && 
              MODPARM(usbUseCase) && 
              !strcmp(MODPARM(usbUseCase),"factory_hstest") 
           )
        {
            // no, so indicate that the wakeup line failed
             usb_hstest_hotplug.flags = HOTPLUG_FUNC_ENABLED;
             usb_hstest_hotplug.hotplug_status = hotplug_unknown;
             printk("\nFACTORY TEST: High Speed factory check failed  %s:%d ", __FUNCTION__, __LINE__);
             TRACE_MSG0(PCD, "FACTORY TEST: High Speed factory check failed");
             generic_os_hotplug(&usb_hstest_hotplug);
        }
        else
        {
          if ( (usb_ready_test_flag != TRUE) ) 
          {
            TRACE_MSG0(PCD, "READY INTERRUPT NOT GENERATED WITHIN TIMEOUT\n");
            printk("\n READY INTERRUPT NOT GENERATED WITHIN TIMEOUT %s:%d \n", __FUNCTION__, __LINE__);

          }
          else
          {
            TRACE_MSG0(PCD, "READY INTERRUPT GENERATED\n");
            //printk("\n READY INTERRUPT GENERATED %s:%d \n ", __FUNCTION__, __LINE__);


          }
        }
        is_fx2lp_hstest_done = TRUE;
    }
#endif
    return 0;
}

static void fx2lp_pcd_mod_exit_l26(void)
{
    PFX2LP_PCD pfx2lp = &fx2lp_pcd;

#ifdef CONFIG_OTG_GENERIC_HOTPLUG
    while (PENDING_WORK_ITEM(usb_hstest_hotplug.hotplug_bh)) 
    {
      printk(KERN_ERR"%s: waiting for HS_TEST hotplug bh\n", __FUNCTION__);
      schedule_timeout(10 * HZ);    
    }  
#endif    
    
    // Connect the D+/D- lines back to Full Speed path 
    gpio_usb_hs_switch_set_data (GPIO_LOW);

    //Finally free all the ISR's and exit
    gpio_usb_hs_int_free_irq(NULL);

    TRACE_MSG0(PCD, "AAAA - FX2LP MOD EXIT");
    printk(KERN_INFO"%s: AAAA - FX2LP MOD EXIT\n", __FUNCTION__); 
}

int
fx2lp_handle_setup_cplt (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    *pfx2lp->pcmd_in_addr = FX2LP_WORD_SWAP(FX2LP_PLUS_SETUP_CPLT_CMD);
    fx2lp_gen_intr(pfx2lp, 0);
    return (0);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_write_ep0

DESCRIPTION:
    Event handler: Do a write to EP0

ARGUMENTS PASSED:
    pfx2lp
    eip (ignored)

RETURN VALUE:
    -1  Why were we called?
    0   We've written all we can right now
    1   We did first of two blocks, still need to do second

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
     The following reuses pfx2lp->ep0_partial as a flag
     to remember that we've just written the first of two 32-byte
     commands that comprise a 32-64-byte packet.  (If the output
     is a multiple of 32 bytes, we still need to write one zero-length
     command to tell FX2LP that we're done.)

==========================================================================*/
/*
 */
int
fx2lp_handle_write_ep0 (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    struct usbd_endpoint_instance *ep;
    struct usbd_urb *ltx_urb;
    u32 bytes_to_write;
    pu16 pbuf;
    pu16 epbuf;
    volatile u16     *pfifo_addr;
    PEP_STATUS     peps;
    int ret = -1;

    peps = &pfx2lp->pep_status[FX2LP_EP0];
    ep = peps->endpoint_inst;

    pfifo_addr = pfx2lp->pcmd_in_addr;
    if (peps->send_zlp == TRUE)
    {

        *pfifo_addr = FX2LP_WORD_SWAP(
            FX2LP_PLUS_IN_PHASE_COMMAND(0));

        fx2lp_gen_intr(pfx2lp, 0);

        peps->send_zlp = FALSE;
    //  We've written  the zero length packet.
        return 0;
    }
    
    TRACE_MSG0(PCD,"Enter");

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    if (ep->tx_urb == NULL)
    {
        pcd_tx_next_irq(ep);
        if (ep->tx_urb == NULL)
        {
            printk ("\n\n-----fx2lp_handle_write_ep0(%x %x) no urb\n\n", 
                ep, peps);
            return (ret);
        }
        else
        {
            printk ("\n\n----fx2lp_handle_write_ep0() fixed NULL tx_urb\n\n");
        }
    }
#endif

    if ((peps->bufsavail > 0) &&        /* FX2LP is ready for EP0 data */
        (ltx_urb = ep->tx_urb) != NULL)  /* and we have some to send */
    {
        bytes_to_write = ltx_urb->actual_length - ep->sent;

        if (bytes_to_write > FX2LP_MAX_COMMAND_LENGTH)
        {
            bytes_to_write = FX2LP_MAX_COMMAND_LENGTH;
        }

        pbuf = (pu16)(ltx_urb->buffer + ep->sent);


        *pfifo_addr = FX2LP_WORD_SWAP(
            FX2LP_PLUS_IN_PHASE_COMMAND(bytes_to_write));

        for (epbuf = pbuf + FULL_WORD_COUNT(bytes_to_write);
                pbuf < epbuf;
                ++pbuf)
        {
            *pfifo_addr = FX2LP_WORD_SWAP(*pbuf);
        }
        fx2lp_gen_intr(pfx2lp, 0);

        ep->sent += bytes_to_write;

        ret = 0;

        /*
         * Note, if we just did a full FX2LP_MAX_COMMAND_LENGTH of
         * data, we definitely need to come around again, even if
         * it's to write a zero-length command
         */
        if (bytes_to_write < FX2LP_MAX_COMMAND_LENGTH)
        {
            /* we've sent all data. Do "setup complete" when we can */
            TRACE_MSG0(PCD,"Sending setup complete");
            fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_SETUP_CPLT);
            peps->bufsavail = 0;
            pfx2lp->ep0_partial = FALSE;
            TRACE_MSG0(PCD,"PCD TX COMPLETE IRQ");
            if (pcd_tx_complete_irq (ep, 0) != NULL)
            {
                ret = 1;        /* still more data to do */
            }
        }
        else if (pfx2lp->ep0_partial)
        {
            /* 
             * just sent 2nd of two blocks, and there's more to follow.
             * need to wait for bufsavail to be set.
             */
            peps->bufsavail = 0;
            pfx2lp->ep0_partial = FALSE;
        }
        else
        {
            ret = 1;        /* we have more to do when ready */
            pfx2lp->ep0_partial = TRUE;
        }
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
        printk("\nfx2lp_handle_write_ep0 wrote %d, nx %d av %d re %d",
            bytes_to_write, pfx2lp->ep0_partial, peps->bufsavail, ret);
#endif
    }
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    else
    {
        printk("\n\n\n!!!!!fx2lp_handle_write_ep0 av %d, urb %d\n\n",
            peps->bufsavail, (ep->tx_urb != 0));
    }
#endif

    return (ret);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_ep_short

DESCRIPTION:
    Tell FX2LP that we just gave it a short IN packet for EP2 or EP6.

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    None

PRE-CONDITIONS:
    All of the packet except, if present, the last odd byte has
    already been sent.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_ep_short (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    struct usbd_endpoint_instance *ep;
    struct usbd_urb *ltx_urb;
    volatile u16     *pfifo_addr;
    u16 packet_len;
    u16 cmd;
    PEP_STATUS  peps;

    peps = &(pfx2lp->pep_status[eip->arg]);
    if(peps == NULL)
    {
	printk("peps == NULL returning");
	return -1;
    }
    ep = peps->endpoint_inst;
	
    pfifo_addr = pfx2lp->pcmd_in_addr;
    if(pfifo_addr == NULL)
    {
	printk("pfifo_addr == NULL returning");
	return -1;
    }
    ltx_urb = ep->tx_urb;

    if(ltx_urb == NULL) 
    {
        TRACE_MSG0(PCD,"LTX URB is NULL");
        printk("USBFX2: LTX URB is NULL\n");
        printk("ltx_urb == NULL returning");
        return -1;
    }

    /* compute bytes in last short packet */
    packet_len = ltx_urb->actual_length & (peps->pkt_size_assigned - 1);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    printk ("\n>>>%s: %d 0x%x %d", __FUNCTION__, eip->arg, ltx_urb, packet_len);
#endif

    if (eip->arg == FX2LP_EP2)
    {
        cmd = FX2LP_PLUS_IN_SHORT_PKT_EP2_CMD;
    }
    else
    {
        cmd = FX2LP_PLUS_IN_SHORT_PKT_EP6_CMD;
    }
    *pfifo_addr = FX2LP_WORD_SWAP(cmd | swab(packet_len));

    if (packet_len & 1)
    {
        /* need to also write that last byte */
        *pfifo_addr = FX2LP_WORD_SWAP(
                    (u16)*(ltx_urb->buffer + ltx_urb->actual_length - 1));
    }

    fx2lp_gen_intr(pfx2lp, 0);
    

    /* We're done with this urb */
    ltx_urb = pcd_tx_complete_irq (ep, 0);

    if ((ltx_urb != NULL) && (peps->bufsavail > 0))
    {
        /* we can start processing next urb */
        fx2lp_start_in_data_ep26_irq(pfx2lp, ep);
    }

    return (0);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_short_ack

DESCRIPTION:
    Send short packet ACK to FX2LP

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    None

PRE-CONDITIONS:
    None

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_short_ack (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    fx2lp_send_cmd_irq(pfx2lp,
        FX2LP_PLUS_IN_OUT_SHORT_PKT_MSG_ACK_CMD, eip->arg);
    return (0);
}
/*==========================================================================

FUNCTION:  fx2lp_handle_ep1_write

DESCRIPTION:
    Write one packet of EP1 IN data

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0 normally (data written)
    -1 if nothing was there to write.

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_ep1_write (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    struct usbd_endpoint_instance *ep;
    PEP_STATUS  peps;
    struct usbd_urb *ltx_urb;
    volatile u16     *pfifo_addr;
    u16 bytes_to_write;
    u16 *pbuf;
    u16 *epbuf;
    int ret;

    peps = &pfx2lp->pep_status[FX2LP_EP1];
    ep = peps->endpoint_inst;

    if ((peps->bufsavail > 0) && ((ltx_urb = ep->tx_urb) != NULL))
    {
        /*
         * Note that bytes_to_write could be zero if we're
         * sending ZLP after a full packet.
         */
        bytes_to_write = ltx_urb->actual_length - ep->sent;

        if (bytes_to_write > 0)
        {
            pfifo_addr = pfx2lp->pcmd_in_addr;
            if (bytes_to_write > peps->pkt_size_assigned)
            {
                bytes_to_write = peps->pkt_size_assigned;
            }

            pbuf = (pu16)(ltx_urb->buffer + ep->sent);

            for ( 
               epbuf = pbuf + FULL_WORD_COUNT(bytes_to_write);
               pbuf < epbuf;
               ++pbuf)
            {
                *pfifo_addr = FX2LP_WORD_SWAP(*pbuf);
            }
            ep->sent += bytes_to_write;
        }
        peps->bufsavail = 0;
        fx2lp_gen_intr(pfx2lp, FX2LP_EP1IN_EOP_CMD(bytes_to_write));

        if ((ep->sent >= ltx_urb->actual_length) && !pcd_tx_sendzlp(ep))
        {
            /* 
             * We're done with this urb. If there's another,
             * we'll get it next time through.
             */
            pcd_tx_complete_irq (ep, 0);
        }
        ret = 0;
    }
    else
    {
        /* can't do a command */
        ret = -1;
    }
    return (ret);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_ep_config

DESCRIPTION:
    Write a command associated with configuring an endpoint.

    peps->doconfig has bits say what still needs to be written.

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0   All done with this endpoint
    1   Need to write more when CMDRDY seen.

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    This handler will run multiple times as CMDRDY is received,
    clearing a bit in peps->doconfig each time to reflect what it has
    done.

==========================================================================*/
int
fx2lp_handle_ep_config (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    struct usbd_endpoint_instance *ep;
    PEP_STATUS     peps;
    u16 val;
    u8             reg_offset;

    peps = &pfx2lp->pep_status[eip->arg];
    ep = peps->endpoint_inst;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_INIT)
    printk ("\nfx2lp_handle_ep_config(%d) 0x%x",
        eip->arg, peps->doconfig);
#endif
    if ((peps->doconfig & (FX2LP_DO_PKTLENH | FX2LP_DO_PKTLENL)) != 0)
    {
        reg_offset = fx2lp_get_ep_pktlen_reg(eip->arg);

        if ((peps->doconfig & FX2LP_DO_PKTLENH) != 0)
        {
            fx2lp_write_reg_worsp_irq(pfx2lp, reg_offset, 
                HIBYTE(peps->pkt_size_assigned)
                    |FX2LP_FLAG_WORDWIDE|FX2LP_FLAG_ZEROLEN);
            peps->doconfig &= ~FX2LP_DO_PKTLENH;
        }
        else
        {
            fx2lp_write_reg_worsp_irq(pfx2lp, reg_offset+1, 
                LOBYTE(peps->pkt_size_assigned));
            peps->doconfig &= ~FX2LP_DO_PKTLENL;
        }
    }
    else if ((peps->doconfig & (FX2LP_DO_EPXPFH | FX2LP_DO_EPXPFL)) != 0)
    {
        reg_offset = fx2lp_get_ep_pf_reg(eip->arg);

        if ((peps->doconfig & FX2LP_DO_EPXPFH) != 0)
        {
            fx2lp_write_reg_worsp_irq(pfx2lp, reg_offset, 
                HIBYTE(peps->pkt_size_assigned-1));
            peps->doconfig &= ~FX2LP_DO_EPXPFH;
        }
        else
        {
            fx2lp_write_reg_worsp_irq(pfx2lp, reg_offset+1, 
                LOBYTE(peps->pkt_size_assigned-1));
            peps->doconfig &= ~FX2LP_DO_EPXPFL;
        }
    }
    else if ((peps->doconfig & FX2LP_DO_DATACOMMIT) != 0)
    {
        /* there must be a method to this madness */
        fx2lp_send_cmd_irq (pfx2lp, FX2LP_PLUS_OUT_DATACOMMIT_CMD,
            (eip->arg == FX2LP_EP2 ? 0 : 2));
        peps->doconfig &= ~FX2LP_DO_DATACOMMIT;
    }
    else if ((peps->doconfig & FX2LP_DO_CONFIG) != 0)
    {
        reg_offset = fx2lp_get_ep_config_reg(eip->arg);
        fx2lp_write_reg_worsp_irq(pfx2lp, reg_offset, peps->cfg_reg_val);
        peps->doconfig &= ~FX2LP_DO_CONFIG;
    }
    else if ((peps->doconfig & FX2LP_DO_INTRCFG) != 0)
    {
        /* only for EP1 interrupt */
        peps->doconfig &= ~FX2LP_DO_INTRCFG;

        val = FX2LP_PLUS_IN_EP1OUTEN_CMD;
        if (peps->is_initialized)
        {
            val |= 1 << 8;      /* enabled */
        }
        *pfx2lp->pcmd_in_addr = val;
        *pfx2lp->pcmd_in_addr = FX2LP_WORD_SWAP(1);     /* interrupt */
        fx2lp_gen_intr(pfx2lp, 0);              /* kick fx2lp */
    }
    else
    {
        /* something's broken */
        peps->doconfig = 0;
        return (-1);
    }

    if (peps->doconfig == 0)
    {
        return (0);
    }
    return (1);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_send_cmd

DESCRIPTION:
    Send one command to FX2LP.  The command is specified by eip->arg.

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_send_cmd (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    fx2lp_send_cmd_irq(pfx2lp, eip->arg, 0);
    return (0);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_read_register

DESCRIPTION:
    Start a register read.

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0   Read was issued
    -1  Didn't find anything to read

PRE-CONDITIONS:
    The register to read is in pfx2lp->register_to_read

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_read_register (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    volatile u16    *pfifo_addr;

    if (pfx2lp->register_to_read != 0)
    {
        pfifo_addr = pfx2lp->pcmd_in_addr;
        /* send command to read the register */
        *pfifo_addr = FX2LP_WORD_SWAP (
            FX2LP_PLUS_READ_REG_CMD | ((u16)pfx2lp->register_to_read << 8));
        fx2lp_gen_intr(pfx2lp, 0);

        return (0);
    }
    else
    {
        return (-1);     /* not actually anything to do */
    }
}

/*==========================================================================

FUNCTION:  fx2lp_handle_set_stall_ep0

DESCRIPTION:
    Stall EP0

ARGUMENTS PASSED:
    pfx2lp
    eip (ignored)

RETURN VALUE:
    0

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_set_stall_ep0 (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    fx2lp_write_reg_worsp_irq(pfx2lp, FX2LP_REG_SETUP, 1);
    return (0);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_set_stall

DESCRIPTION:
    Stall EP1, 2 or 6, as specified by eip->arg.

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0   Normal return
    -1  Not initialzed yet?

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_set_stall (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    PEP_STATUS    peps = &pfx2lp->pep_status[eip->arg];
    u8            arg;
    FX2LP_BOOL    stalled;

    stalled = ((peps->cfg_reg_val & FX2LP_EP_STALL_FLAG) != 0);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    printk ("\nfx2lp_handle_set_stall(%d) %d %d", 
            fx2lp_epx_info[peps->epx].phys_epn,
            stalled, peps->is_initialized);
#endif

    if (peps->is_initialized)
    {
        /* need to set or clear stall status */
        if (stalled)
        {
            /* endpoint should be stalled */
            fx2lp_write_reg_worsp_irq(pfx2lp, 
                fx2lp_get_ep_config_reg(eip->arg),
                peps->cfg_reg_val);
        }
        else
        {
            /* endpoint should be NOT stalled */
            /* EP1 is always IN, and for some reason this maps to EP0 */
            arg = fx2lp_epx_info[peps->epx].phys_epn;
            if (peps->epx == FX2LP_EP1)
            {
                arg = 0;
            }

            fx2lp_send_cmd_irq(pfx2lp,
                    FX2LP_PLUS_IN_CLEAR_DATA_TOGGLE_AND_STALL_CMD, arg);
        }

        return (0);
    }
    else
    {
        return (-1);
    }

}

/*==========================================================================

FUNCTION:  fx2lp_handle_ifconfig

DESCRIPTION:
    Send an IFCONFIG command

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    eip->arg identifies special value to send

==========================================================================*/
int
fx2lp_handle_ifconfig (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    printk(" I am in HANDLE IFCONFIG : %d",pfx2lp->ifconfig | eip->arg);
    fx2lp_write_reg_worsp_irq(pfx2lp, FX2LP_REG_IFCONFIG,
        pfx2lp->ifconfig | eip->arg);
    pfx2lp->is_fx2lp_ready = TRUE;
    return (0);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_intr_en

DESCRIPTION:
    Do interrupt enable

ARGUMENTS PASSED:
    pfx2lp
    eip         -> arg identifies whether to do FX2LP_REG_INTENABLE
                    or FX2LP_REG_INTENABLE1

RETURN VALUE:
    0

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_intr_en (
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    if (eip->arg != 0)
    {
        fx2lp_write_reg_worsp_irq (pfx2lp, FX2LP_REG_INTENABLE1,
                pfx2lp->eintr_en);
    }
    else
    {
        fx2lp_write_reg_worsp_irq (pfx2lp, 
                FX2LP_REG_INTENABLE,
                INT_M_SETUP|INT_M_EP0BUF|INT_M_USBRESET|
                INT_M_ENUMOK|INT_M_CMDSTATRDY|INT_M_READY);
    }
    return (0);
}    

/*==========================================================================

FUNCTION:  fx2lp_handle_usb_reset

DESCRIPTION:
    Flush EP6

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_usb_reset(
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    struct pcd_instance *pcd = PFX2LP_PCD(pfx2lp);
    struct usbd_bus_instance *bus = pcd->bus;

// First flush the fifo. Once we get the cmdrdy, do the reset ack
    fx2lp_write_reg_worsp_irq(pfx2lp, FX2LP_REG_EPINPKTEND, FX2LP_FLAG_FLUSH_FIFO6);
    fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_USBRESET_ACK);
    return (0);
}

/*==========================================================================

FUNCTION:  fx2lp_handle_usb_reset_ack

DESCRIPTION:
   Issue FX2LP_PLUS_USBRESET_ACK_CMD

ARGUMENTS PASSED:
    pfx2lp
    eip

RETURN VALUE:
    0

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_usb_reset_ack(
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    struct pcd_instance *pcd = PFX2LP_PCD(pfx2lp);
    struct usbd_bus_instance *bus = pcd->bus;

    fx2lp_send_cmd_irq(pfx2lp, FX2LP_PLUS_IN_USBRESET_ACK_CMD, 0);
    usbd_bus_event_handler_irq (bus, DEVICE_RESET, 0);
    return (0);
}



/*==========================================================================

FUNCTION:  fx2lp_handle_usb_activity_cmd

DESCRIPTION:
    Issue FX2LP_PLUS_IN_WAKEUP_USB_ACTIVITY_CMD

ARGUMENTS PASSED:
    pfx2lp
    eip (ignored)

RETURN VALUE:
    0

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
int
fx2lp_handle_usb_activity_cmd(
    PFX2LP_PCD pfx2lp,
    const FX2LP_RDY_EV_INFO_T *eip)
{
    TRACE_MSG0(PCD,"Enter");
    fx2lp_send_cmd_irq(pfx2lp, FX2LP_PLUS_IN_WAKEUP_USB_ACTIVITY_CMD, eip->arg);
    return (0);
}
/*==========================================================================

FUNCTION:  fx2lp_start_in_data_ep0

DESCRIPTION:
    Schedule CMDRDY event to send a block of IN data for EP0

ARGUMENTS PASSED:
    pfx2lp
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None

==========================================================================*/
static void
fx2lp_start_in_data_ep0(
    PFX2LP_PCD pfx2lp,
    struct usbd_endpoint_instance *endpoint)
{
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    printk ("\nfx2lp_start_in_data_ep0(%x)", endpoint->tx_urb);
#endif
    fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_WRITE_EP0);
}
/*==========================================================================

FUNCTION:  fx2lp_start_in_data_ep1

DESCRIPTION:
    Schedule CMDRDY event to send a block of IN data for EP1

ARGUMENTS PASSED:
    pfx2lp
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
void
fx2lp_start_in_data_ep1(
    PFX2LP_PCD pfx2lp,
    struct usbd_endpoint_instance *endpoint)
{
    fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_WRITE_EP1);
}

/*==========================================================================

FUNCTION:  fx2lp_start_in_data_ep26_irq

DESCRIPTION:
    Initiate sending a block of IN data for EP 2 or 6.  This is called
    when a new IN urb is queued.

ARGUMENTS PASSED:
    pfx2lp
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    Called with interrupts blocked.
    tx_urb != NULL

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None

==========================================================================*/
void
fx2lp_start_in_data_ep26_irq(
    PFX2LP_PCD pfx2lp,
    struct usbd_endpoint_instance *endpoint)
{
    PEP_STATUS peps = (PEP_STATUS)endpoint->privdata;

    // ASSERT (endpoint->tx_urb != NULL);
    // ASSERT (peps->bufsavail > 0);

#if FX2LP_IN_AS_BH
    if (!peps->bh_active)
    {
        TRACE_MSG0(PCD,"No bh active - Schedule a new one");
        peps->bh_active = TRUE;
        SCHEDULE_WORK(peps->start_rxtx);
    }
#else
    ltx_urb = endpoint->tx_urb;
    while ((peps->bufsavail > 0) && !peps->sending_in_short)
    {
        peps->bufsavail--;

        /* Copy one packet of data */
        endpoint->sent += fx2lp_do_in_data_ep26 (pfx2lp, endpoint);

        if (endpoint->sent >= ltx_urb->actual_length)
        {
            fx2lp_finish_in_ep26_irq(pfx2lp, endpoint);
            if ((ltx_urb = endpoint->tx_urb) == NULL)
            {
                break;
            }
        }
    }
#endif
}

/*==========================================================================

FUNCTION:  fx2lp_do_in_data_ep26

DESCRIPTION:
    Copy one packet of EP2/6 IN data to FX2LP.  Return count copied.

ARGUMENTS PASSED:
    pfx2lp
    endpoint

RETURN VALUE:
    Count of bytes copied from urb.

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    This can be called with interrupts enabled or not.  It is up to
    the caller to update the urb and buffer count, paying attention to 
    necessary mutual exclusion.

==========================================================================*/
static u16
fx2lp_do_in_data_ep26(
    PFX2LP_PCD pfx2lp,
    struct usbd_endpoint_instance *endpoint)
{
    PEP_STATUS  peps = (PEP_STATUS)endpoint->privdata;
    volatile pu16 pfifo_addr = peps->pfifo_addr;
    struct usbd_urb *ltx_urb = endpoint->tx_urb;
    u16         bytes_to_write;
    pu16        pbuf, epbuf;


    /* how much can we do in this block? */
    bytes_to_write = ltx_urb->actual_length - endpoint->sent;
    if (bytes_to_write > peps->pkt_size_assigned)
    {
        bytes_to_write = peps->pkt_size_assigned;
    }

    pbuf = (pu16)(ltx_urb->buffer + endpoint->sent);
    fx2lp_debugdump_rwdata (">>> IN data", (char *)pbuf, bytes_to_write);
    /* Note: We only copy full words.   Leave last odd byte for later */
    for (epbuf = pbuf + (bytes_to_write / (sizeof (*pbuf)));
        pbuf < epbuf;
        ++pbuf)
    {
        *pfifo_addr = FX2LP_WORD_SWAP(*pbuf);
    }

    return (bytes_to_write);
}

/*==========================================================================

FUNCTION:  fx2lp_finish_in_ep26_irq

DESCRIPTION:
    Processing when all data sent from an EP2/6 OUT urb.  Handle
    special cases for a short packet.

ARGUMENTS PASSED:
    pfx2lp
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    Called with interrupts blocked.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None

==========================================================================*/
static void
fx2lp_finish_in_ep26_irq(
    PFX2LP_PCD pfx2lp,
    struct usbd_endpoint_instance *endpoint)
{
    PEP_STATUS  peps = (PEP_STATUS)endpoint->privdata;

    /* this is the last packet of the urb. */
    if (pcd_tx_sendzlp(endpoint)
        || ((endpoint->tx_urb->actual_length & (peps->pkt_size_assigned - 1))
                    != 0))
    {
        /* 
         * it's a short packet, so need to send command
         * this will send notice of short (or zero-len) packet.
         * pcd_tx_complete_irq() will be called once the short
         * packet notification has been given to fx2lp.
         */
        peps->sending_in_short = TRUE;
        fx2lp_schedule_ready_event_irq(pfx2lp, 
            fx2lp_epx_info[peps->epx].short_packet_event);
    }
    else
    {
        /* complete the urb */
        pcd_tx_complete_irq(endpoint, 0);
    }
}

/*==========================================================================

FUNCTION:  fx2lp_do_in_data_ep26_bh

DESCRIPTION:
    Bottom half to send one or more IN packets for EP 2 or 6.

ARGUMENTS PASSED:
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    peps->bh_active is set

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    This will continue copying data as long as IN packets are
    available in FX2LP and IN urbs are queued.

==========================================================================*/
#if FX2LP_IN_AS_BH
static void
fx2lp_do_in_data_ep26_bh(
    struct usbd_endpoint_instance *endpoint)
{
    struct usbd_urb *ltx_urb;
    struct pcd_instance *pcd;
    u16         bytes_to_write;
    PEP_STATUS  peps = (PEP_STATUS)endpoint->privdata;
    unsigned long flags;
    PFX2LP_PCD pfx2lp;
    u16         reps = 0;

    local_irq_save (flags);
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    printk ("\n>>>>fx2lp_do_in_data_ep26_bh(%d) bav %d, busy %d, urb %x",
        peps->ep_num, peps->bufsavail, peps->sending_in_short,
        endpoint->tx_urb);
#endif
    if ((peps->bufsavail < 2) 
        || peps->sending_in_short
        || ((ltx_urb = endpoint->tx_urb) == NULL))
    {
        peps->bh_active = FALSE;
        local_irq_restore (flags);
        return;                 /* nothing to do */
    }

    pcd = (struct pcd_instance *) ltx_urb->bus->privdata;
    pfx2lp = PCD_PFX2LP(pcd);

    do
    {
        peps->bufsavail--;

        /* do data copy w/o blocking interrupts */
        local_irq_restore (flags);

        /* how much can we do in this block? */
        bytes_to_write = fx2lp_do_in_data_ep26 (pfx2lp, endpoint);

        local_irq_save (flags);
        endpoint->sent += bytes_to_write;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
        printk ("... %d to %x ", bytes_to_write, ltx_urb);
#endif
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_LOOPCTS)
        ++reps;
#endif

        if (endpoint->sent >= ltx_urb->actual_length)
        {
            TRACE_MSG1(PCD,"finishing in ep26 irq for %p\n",ltx_urb);
            fx2lp_finish_in_ep26_irq(pfx2lp, endpoint);
            if (peps->sending_in_short)
            {
                TRACE_MSG1(PCD," Sending in ep26 short for %p\n",ltx_urb);
                break;  /* short packet started */
            }
            TRACE_MSG1(PCD," SENT in ep26 short for %p\n",ltx_urb);
            ltx_urb = endpoint->tx_urb;
        }

    } while ((peps->bufsavail > 0) && (ltx_urb != NULL));

    peps->bh_active = FALSE;
    local_irq_restore (flags);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_LOOPCTS)
    {
        static int c[4], passes;

        reps = MIN(reps, ((sizeof c)/(sizeof (c[0]))-1));
        c[reps]++;
        if (++passes > 50)
        {
            printk ("  --IN %d %d %d %d\n", c[0], c[1], c[2], c[3]);
            passes = 0;
            memset (c, 0, sizeof (c));
        }
    }
#endif
}
#endif

/*==========================================================================

FUNCTION:  fx2lp_pcd_start_endpoint_in

DESCRIPTION:
    Called from bus driver to start an IN transfer.  It is only called
    when a new IN urb is queued and none was there before.

ARGUMENTS PASSED:
    pcd
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    Called with interrupts blocked.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
static void
fx2lp_pcd_start_endpoint_in (
    struct pcd_instance *pcd, 
    struct usbd_endpoint_instance *endpoint)
{
    PFX2LP_PCD pfx2lp = PCD_PFX2LP(pcd);
    PEP_STATUS  peps = (PEP_STATUS)endpoint->privdata;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    {
        int hs = pcd->bus->high_speed;
        printk ("\n>>>fx2lp_pcd_start_endpoint_in (%x) ur %x, ep %x, le %d, fl %x, bav %d",
                endpoint,
                endpoint->tx_urb,
                peps->ep_num,
                endpoint->tx_urb->actual_length,
                endpoint->tx_urb->flags,
                peps->bufsavail);
    }
#endif
    /* first check that we're not busy, and that there's something to do */
    if ((endpoint->tx_urb != NULL) && (peps->bufsavail > 0))
    {
        (peps->start_in_data)(pfx2lp, endpoint);
    }
}


/*==========================================================================

FUNCTION:  fx2lp_pcd_start_endpoint_out

DESCRIPTION:
    Called from bus driver to start an OUT transfer.  It is only called
    when a new OUT urb is queued and none was there before.

ARGUMENTS PASSED:
    pcd
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    Called with interruputs blocked.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
static void
fx2lp_pcd_start_endpoint_out(
    struct pcd_instance *pcd,
    struct usbd_endpoint_instance *endpoint)
{
    PEP_STATUS  peps;
    struct usbd_urb   *rcv_urb;

    peps = (PEP_STATUS)endpoint->privdata;
    if ((peps->outinfo.outavail > 0)
     && (rcv_urb = pcd_rcv_next_irq(endpoint)) != NULL)
    {
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
        printk ("\n<<<fx2lp_pcd_start_endpoint_out (%x, %x, %d, %x)",
                    peps->ep_num,
                    rcv_urb,
                    rcv_urb->buffer_length,
                    rcv_urb->flags);
#endif
        (peps->do_out_data)(PCD_PFX2LP(pcd), endpoint);
    }
}

/*==========================================================================

FUNCTION:  fx2lp_do_out_26_packet

DESCRIPTION:
    Copy one packet of EP 2 or 6 OUT data to the part. packetlen bytes
    of data are copied to ep->rcv_urb.

ARGUMENTS PASSED:
    endpoint
    packetlen

RETURN VALUE:
    None

PRE-CONDITIONS:
    ep->rcv_urb is not NULL.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    Nothing here depends on interrupts being enabled or not.  The
    caller must handle any mutual exclusion issues.

    The caller must update ep->rcv_urb.

==========================================================================*/
static void 
fx2lp_do_out_26_packet(
    struct usbd_endpoint_instance *ep,
    u16         packetlen)
{
    PEP_STATUS          peps = (PEP_STATUS)ep->privdata;
    struct usbd_urb     *rcv_urb = ep->rcv_urb;
    pu16                pbuf, buffer_stop;
    volatile u16        *pfifo_addr;

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_DATAFLOW)
    printk ("\n<<<<fx2lp_do_out_26_packet(%d, %x): ps %d, al %d, bs %d",
        peps->ep_num,
        rcv_urb,
        packetlen, 
        rcv_urb->actual_length, rcv_urb->buffer_length);
#endif

    pfifo_addr = peps->pfifo_addr;

    pbuf = (pu16)(rcv_urb->buffer + rcv_urb->actual_length);
    buffer_stop = pbuf + FULL_WORD_COUNT(packetlen);
    while (pbuf < buffer_stop)
    {
        *pbuf = FX2LP_WORD_SWAP(*pfifo_addr);
        pbuf++;
    }
    fx2lp_debugdump_rwdata("<<< OUT data", 
        rcv_urb->buffer + rcv_urb->actual_length, packetlen);
}
/*==========================================================================

FUNCTION:  fx2lp_do_out_26_irq

DESCRIPTION:
    Initiate copying of OUT data for EP 2 or 6. This is called when
    the bus driver queues a new OUT urb and none was already queued.

ARGUMENTS PASSED:
    pfx2lp
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    Called with interrupts disabled.
    There is an rcv_urb pending.
    outinfo.outavail > 0

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    Two versions of this routine are supplied.  One does the copy now.
    One starts the bottom half.

==========================================================================*/
static void 
fx2lp_do_out_26_irq(
    PFX2LP_PCD pfx2lp, 
    struct usbd_endpoint_instance *ep)
{
    PEP_STATUS  peps = (PEP_STATUS)ep->privdata;

    // ASSERT (peps->outinfo.outavail > 0);
    // ASSERT (ep->rcv_urb != NULL);

#if FX2LP_OUT_AS_BH
    /*
     * There's at least one packet pending, so fire work item to handle it. 
     */
    if (!peps->bh_active)
    {
        peps->bh_active = TRUE;
        SCHEDULE_WORK(peps->start_rxtx);
    }
#else
    {
        pu16                pbuf, buffer_stop;
        short               packetlen;
        volatile u16        *pfifo_addr;

        do
        {
            packetlen = peps->outinfo.rcd_pktlen;
            peps->outinfo.rcd_pktlen = peps->pkt_size_assigned;

            fx2lp_do_out_26_packet (ep, packetlen);
            pcd_rcv_complete_irq (ep, packetlen, 0);

            if (--peps->outinfo.outavail == 0)
            {
                /* this was the last packet available */
                if (peps->outinfo.shortpend >= 0)
                {
                    /* we were notified that a short packet was received. */
                    /* ask to get it now */
                    fx2lp_req_short_packet(pfx2lp, peps);
                }
                break;
            }
        } while (ep->rcv_urb != NULL);
    }
#endif
}

void
fx2lp_req_short_packet (PFX2LP_PCD pfx2lp, PEP_STATUS peps)
{
    if ((peps->outinfo.shortpend >= 0) && (peps->outinfo.outavail == 0))
    {
        peps->outinfo.rcd_pktlen = peps->outinfo.shortpend;
        peps->outinfo.shortpend = -1;
        fx2lp_schedule_ready_event(pfx2lp, 
            fx2lp_epx_info[peps->epx].shortack_event);
    }
}

/*==========================================================================

FUNCTION:  fx2lp_do_out_data_ep26_bh

DESCRIPTION:
    Bottom half processing for EP2 or 6 OUT data.

    Copy data as long as there's a rcv_urb and an available OUT packet.

ARGUMENTS PASSED:
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    peps->bh_active is set

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    Compiled only if FX2LP_OUT_AS_BH.

==========================================================================*/
#if FX2LP_OUT_AS_BH
static void 
fx2lp_do_out_data_ep26_bh(
    struct usbd_endpoint_instance *ep)
{
    PEP_STATUS          peps = (PEP_STATUS)ep->privdata;
    struct usbd_urb     *rcv_urb = ep->rcv_urb;
    pu16                pbuf, buffer_stop;
    short               packetlen;
    volatile u16        *pfifo_addr;
    unsigned long flags;
    PFX2LP_PCD pfx2lp;
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_LOOPCTS)
    int                 reps = 0;
#endif

    local_irq_save (flags);

    if ((ep->rcv_urb != NULL) && (peps->outinfo.outavail > 0))
    {
        pfx2lp = PCD_PFX2LP((struct pcd_instance *) ep->rcv_urb->bus->privdata);
        do
        {
#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_LOOPCTS)
            ++reps;
#endif
            packetlen = peps->outinfo.rcd_pktlen;
            peps->outinfo.rcd_pktlen = peps->pkt_size_assigned;

            if (--peps->outinfo.outavail == 0)
            {
                /* this is the last packet available */
                if (peps->outinfo.shortpend >= 0)
                {
                    /* ask for following short packet */
                    fx2lp_req_short_packet(
                        PCD_PFX2LP(
                            (struct pcd_instance *) ep->rcv_urb->bus->privdata),
                        peps);
                }
            }

            local_irq_restore (flags);
            fx2lp_do_out_26_packet (ep, packetlen);
            local_irq_save (flags);

            pcd_rcv_complete_irq (ep, packetlen, 0);

        } while ((ep->rcv_urb != NULL) && (peps->outinfo.outavail > 0));
    }

    peps->bh_active = FALSE;
    local_irq_restore (flags);

#if (FX2LP_DETAIL_KPF & FX2LP_DETAIL_LOOPCTS)
    {
        static int c[4], passes;

        reps = MIN(reps, ((sizeof c)/(sizeof (c[0]))-1));
        c[reps]++;
        if (++passes > 50)
        {
            printk ("  --OUT %d %d %d %d\n", c[0], c[1], c[2], c[3]);
            passes = 0;
            memset (c, 0, sizeof (c));
        }
    }
#endif
}
#endif

/*==========================================================================

FUNCTION:  fx2lp_handle_cmd_out

DESCRIPTION:
    Handle interrupt saying there's EP0 OUT data.  Most work is done
    in fx2lp_do_out_0().

ARGUMENTS PASSED:
    pfx2lp

RETURN VALUE:
    None

PRE-CONDITIONS:
    None.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
static void
fx2lp_handle_cmd_out(PFX2LP_PCD pfx2lp)
{
    PEP_STATUS     peps = fx2lp_get_ep_status(pfx2lp, FX2LP_EP0);

    TRACE_MSG0(PCD,"Enter fx2lp_handle_cmd_out\n");

    peps->outinfo.outavail = 1;

    if (peps->endpoint_inst->rcv_urb != NULL)
    {
        fx2lp_do_out_0 (pfx2lp, peps->endpoint_inst);
    }
}

/*==========================================================================

FUNCTION:  fx2lp_do_out_0

DESCRIPTION:
    Copy data if there's a received EP0 packet and an rcv_urb to put
    it into.

ARGUMENTS PASSED:
    pfx2lp
    endpoint

RETURN VALUE:
    None

PRE-CONDITIONS:
    We can assume that outinfo.outavail > 0.  It's checked before this
    is called.

POST-CONDITIONS:
    None.

IMPORTANT NOTES:
    None.

==========================================================================*/
static void
fx2lp_do_out_0 (PFX2LP_PCD pfx2lp, struct usbd_endpoint_instance *endpoint)
{
    struct usbd_urb     *rcv_urb;
    pu16                pbuf, buffer_stop;
    volatile u16        *pfifo_addr;
    u8                  cmd_id;

    u16               val, buffer_length;
    
    TRACE_MSG0(PCD,"Enter");

    if ((rcv_urb = endpoint->rcv_urb) == NULL)
    {
        return;
    }

    pfifo_addr = pfx2lp->pcmd_out_addr;
    val = FX2LP_WORD_SWAP(*pfifo_addr);
    cmd_id = LOBYTE(val);

    if(cmd_id == FX2LP_PLUS_OUT_PHASE_CMD)
    {
        pfx2lp->pep_status[FX2LP_EP0].outinfo.outavail = 0;
        buffer_length = HIBYTE(val);

        pbuf = (pu16)(rcv_urb->buffer + rcv_urb->actual_length);
        buffer_stop = pbuf + FULL_WORD_COUNT(buffer_length);

        while (pbuf < buffer_stop)
        {
            *pbuf++ = *pfifo_addr;
        }

        if (buffer_length < FX2LP_MAX_COMMAND_LENGTH)
        { 
             TRACE_MSG0(PCD,"Scheduling setup complete");
            fx2lp_schedule_ready_event(pfx2lp, FX2LP_RDY_EV_N_SETUP_CPLT);
        }
        rcv_urb = pcd_rcv_complete_irq (endpoint, buffer_length, 0);
    }
        
}

