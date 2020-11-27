/*
 * otg/functions/network/net-l24-os.c - Network Function Driver
 * @(#) sl@whiskey.enposte.net|otg/functions/network/net-l24-os.c|20051116230343|35909
 *
 *      Copyright (c) 2002-2005 Belcarra Technologies
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005,2006,2008 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/08/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 10/19/2008         Motorola         Add a NULL check before pointer dereference
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
/*!
 * @file otg/functions/network/net-l24-os.c
 * @brief The Linux 2.4 OS specific upper edge (network interface) 
 * implementation for the Network Function Driver.
 *
 * This file implements a standard Linux network driver interface and
 * the standard Linux 2.4 module init and exit functions.
 *
 * If compiled into the kernel, this driver can be used with NFSROOT to
 * provide the ROOT filesystem. Please note that the kernel NFSROOT support
 * (circa 2.4.20) can have problems if there are multiple interfaces. So 
 * it is best to ensure that there are no other network interfaces compiled
 * in.
 *
 *
 * @ingroup NetworkFunction
 */

/* OS-specific #includes */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/ctype.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/utsname.h>
#include <linux/kmod.h>
#include <asm/uaccess.h>
#include <asm/system.h>

/* Networking #includes from OS */
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <net/arp.h>
#include <linux/rtnetlink.h>
#include <linux/atmdev.h>
#include <linux/pkt_sched.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <linux/in.h>
#include <linux/inetdevice.h>



/* Belcarra public interfaces */
#include <otg/otg-compat.h>
#include <otg/otg-module.h>
#include <otg/usbp-chap9.h>
//#include <otg/usbd-mem.h>
#include <otg/usbp-func.h>

/* Belcarra private interfaces SUBJECT TO CHANGE WITHOUT NOTICE */
#include "network.h"
#include "net-os.h"
#include "net-fd.h"

/* public node definition */
#include <public/otg-node.h>


MOD_AUTHOR ("sl@belcarra.com, tbr@belcarra.com, balden@belcarra.com");

EMBED_LICENSE();

MOD_DESCRIPTION ("USB Network Function");
EMBED_USBD_INFO ("network_fd 2.0-beta");

MOD_PARM_STR (local_dev_addr, "Local Device Address", NULL);
MOD_PARM_STR (remote_dev_addr, "Remote Device Address", NULL);

int blan_mod_init(void);
void blan_mod_exit(void);

int basic_mod_init(void);
void basic_mod_exit(void);

int basic2_mod_init(void);
void basic2_mod_exit(void);

int cdc_mod_init(void);
void cdc_mod_exit(void);

int safe_mod_init(void);
void safe_mod_exit(void);

#define NTT network_fd_trace_tag
otg_tag_t network_fd_trace_tag;
wait_queue_head_t usb_netif_wq;
#ifdef CONFIG_OTG_NET_NFS_SUPPORT
int usb_is_configured;
#endif

/* Module Parameters ************************************************************************* */


/* End of Module Parameters ****************************************************************** */

//u8 local_dev_addr[ETH_ALEN];
//u8 remote_dev_addr[ETH_ALEN];
static u8 zeros[ETH_ALEN];

extern u32 ip_addr;
extern u32 router_ip;
extern u32 network_mask;
extern u32 dns_server_ip;

/* Prevent overlapping of bus administrative functions:
 *
 *      network_function_enable
 *      network_function_disable
 *      network_hard_start_xmit
 */
DECLARE_MUTEX(usbd_network_sem);

struct net_device Network_net_device;
struct net_device_stats Network_net_device_stats;  /* network device statistics */

void notification_schedule_bh (void);
int network_urb_sent_bulk (struct usbd_urb *urb, int urb_rc);
int network_urb_sent_int (struct usbd_urb *urb, int urb_rc);

//_________________________________________________________________________________________________

/*
 * Synchronization
 *
 *
 * Notification bottom half
 *   
 *  This is a scheduled task that will send an interrupt notification. Because it
 *  is called from the task scheduler context it needs to verify that the device is
 *  still usable.
 *
 *      static int network_send_int_blan(struct usbd_simple_instance *, int )
 *      static void notification_bh (void *)
 *      void notification_schedule_bh (void)
 *
 *
 * Netdevice functions
 *
 *   These are called by the Linux network layer. They must be protected by irq locks
 *   if necessary to prevent interruption by IRQ level events.
 *
 *      int network_init (struct net_device *net_device)
 *      void network_uninit (struct net_device *net_device)
 *      int network_open (struct net_device *net_device)
 *      int network_stop (struct net_device *net_device)
 *      struct net_device_stats *network_get_stats (struct net_device *net_device)
 *      int network_set_mac_addr (struct net_device *net_device, void *p)
 *      void network_tx_timeout (struct net_device *net_device)
 *      int network_set_config (struct net_device *net_device, struct ifmap *map)
 *      int network_stop (struct net_device *net_device)
 *      int network_hard_start_xmit (struct sk_buff *skb, struct net_device *net_device)
 *      int network_do_ioctl (struct net_device *net_device, struct ifreq *rp, int cmd)
 *
 *
 * Data bottom half functions
 *
 *  These are called from the bus bottom half handler. 
 *
 *      static int network_recv (struct usb_network_private *, struct net_device *, struct sk_buff *)
 *      int network_recv_urb (struct usbd_urb *)
 *      int network_urb_sent (struct usbd_urb *, int )
 *      
 *
 * Hotplug bottom half:
 *
 *  This is a scheduled task that will send do a hotplug call. Because it is
 *  called from the task scheduler context it needs to verify that the
 *  device is still usable.
 *
 *      static int hotplug_attach (u32 ip, u32 mask, u32 router, int attach)
 *      static void hotplug_bh (void *data)
 *      void net_os_hotplug (void)
 *
 *
 * Irq level functions:
 *
 *   These are called at interrupt time do process or respond to USB setup
 *   commands. 
 *
 *      int network_device_request (struct usbd_device_request *)
 *      void network_event_handler (struct usbd_simple_instance *function, usbd_device_event_t event, int data)
 *
 *      
 * Enable and disable functions:
 *
 *      void network_function_enable (struct usbd_simple_instance *, struct usbd_simple_instance *)
 *      void network_function_disable (struct usbd_simple_instance *function)
 *
 *
 * Driver initialization and exit:
 *
 *      static  int network_create (struct usb_network_private *)
 *      static  void network_destroy (struct usb_network_private *)
 *
 *      int network_modinit (void)
 *      void network_modexit (void)
 */


//_______________________________USB part Functions_________________________________________

/*
 * net_os_mutex_enter - enter mutex region
 */
void net_os_mutex_enter(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        down(&usbd_network_sem);
}

/*
 * net_os_mutex_exit - exit mutex region
 */
void net_os_mutex_exit(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        up(&usbd_network_sem);
}

/* notification_bh - Bottom half handler to send a notification status 
 *
 * Send a notification with open/close status
 *
 * It should not be possible for this to be called more than once at a time
 * as it is only called via schedule_task() which protects against a second
 * invocation.
 */
STATIC void notification_bh (void *data)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) data;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        unsigned long flags;
        RETURN_UNLESS(npd);
        local_irq_save(flags);
        net_fd_send_int_notification(function_instance, npd->flags & NETWORK_OPEN, 0);
        local_irq_restore(flags);
}

/* notification_schedule_bh - schedule a call for notification_bh
 */
void net_os_send_notification_later(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd);
#ifdef LINUX24
        //LINUX 2.4 is the only supported OS to need the USE_COUNT
        //_MOD_INC_USE_COUNT;
        TRACE_MSG1(NTT, "INC: %d", MOD_IN_USE);
        if (!SCHEDULE_WORK(npd->notification_bh)) {
                //_MOD_DEC_USE_COUNT;
                //TRACE_MSG1(NTT, "DEC: %d", MOD_IN_USE);
        }
#else
        PREPARE_WORK_ITEM(npd->notification_bh, notification_bh,npd);
        SCHEDULE_WORK(npd->notification_bh);
        //XXX No provision for failure of schedule ....
#endif
}

/*! net_os_xmit_done - called from USB part when a transmit completes, good or bad.
 *                    tx_rc is USBD_URB_ERROR, USBD_URB_CANCELLED or...
 *                    data is the pointer passed to fd_ops->start_xmit().
 * @return non-zero only if network does not exist, ow 0.
 */
int net_os_xmit_done(struct usbd_function_instance *function_instance, void *data, int tx_rc)
{
        struct sk_buff *skb = (struct sk_buff *) data;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct net_device *net_device = npd ? npd->net_device : NULL;
        int rc = 0;

        TRACE_MSG4(NTT, "function: %x npd: %x skb: %x", function_instance, npd, skb, net_device);

        RETURN_ZERO_UNLESS(skb);
        dev_kfree_skb_any(skb);

        RETURN_ZERO_UNLESS(net_device);
        if ( netif_queue_stopped(net_device)) {
                netif_wake_queue(net_device);
        }

        RETURN_ZERO_UNLESS(npd);

        switch (tx_rc) {
        case USBD_URB_ERROR:
                Network_net_device_stats.tx_errors++;
                Network_net_device_stats.tx_dropped++;
                break;
        case USBD_URB_CANCELLED:
                Network_net_device_stats.tx_errors++;
                Network_net_device_stats.tx_carrier_errors++;
                break;
        default:
                break;
        }

        npd->avg_queue_entries += npd->queued_entries;
        npd->queued_entries--;
        npd->samples++;
        npd->jiffies += jiffies - *(time_t *) (&skb->cb);
        npd->queued_bytes -= skb->len;

        return 0;
}

/*! net_os_dealloc_buffer
 */
void net_os_dealloc_buffer(struct usbd_function_instance *function_instance, void *data)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct sk_buff *skb = data;
        RETURN_UNLESS(skb);
        dev_kfree_skb_any(skb);
        RETURN_UNLESS(npd);
        Network_net_device_stats.rx_dropped++;
}

/*! net_os_alloc_buffer
 */
void *net_os_alloc_buffer(struct usbd_function_instance *function_instance, u8 **cp, int n)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct sk_buff *skb;

        /* allocate skb of appropriate length, reserve 2 to align ip
         */
        RETURN_NULL_UNLESS ((skb = dev_alloc_skb(n+2)));
        TRACE_MSG1(NTT, "skb: %x", skb);

        skb_reserve(skb, 2);
        *cp = skb_put(skb, n);

        TRACE_MSG7(NTT, "skb: %x head: %x data: %x tail: %x end: %x len: %d tail-data: %d", 
                        skb, skb->head, skb->data, skb->tail, skb->end, skb->len, skb->tail - skb->data); 

        return (void*) skb;
}

/*! network_recv - function to process an received data URB
 *
 * Passes received data to the network layer. Passes skb to network layer.
 *
 * @return non-zero for failure.
 */
STATIC __inline__ int network_recv (struct usbd_function_instance *function_instance, 
                struct net_device *net_device, struct sk_buff *skb)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        int rc;

        RETURN_ZERO_UNLESS(npd);
#if 0
        printk(KERN_INFO"%s: len: %x head: %p data: %p tail: %p\n", __FUNCTION__, 
                        skb->len, skb->head, skb->data, skb->tail);
        {
                u8 *cp = skb->data;
                int i;
                for (i = 0; i < skb->len; i++) {
                        if ((i%32) == 0) {
                                printk("\nrx[%2x] ", i);
                        }
                        printk("%02x ", *cp++);
                }
                printk("\n");
        }
#endif

        /* refuse if no device present
         */
        if (!netif_device_present (net_device)) {
                TRACE_MSG0(NTT,"device not present");
                printk(KERN_INFO"%s: device not present\n", __FUNCTION__);
                return -EINVAL;
        }

        /* refuse if no carrier
         */
        if (!netif_carrier_ok (net_device)) {
                TRACE_MSG0(NTT,"no carrier");
                printk(KERN_INFO"%s: no carrier\n", __FUNCTION__);
                return -EINVAL;
        }

        /* refuse if the net device is down
         */
        if (!(net_device->flags & IFF_UP)) {
                TRACE_MSG1(NTT,"not up net_dev->flags: %x", net_device->flags);
                //printk(KERN_INFO"%s: not up net_dev->flags: %x\n", __FUNCTION__, net_device->flags);
                //npd->stats.rx_dropped++;
                return -EINVAL;
        }

        skb->dev = net_device;
        skb->pkt_type = PACKET_HOST;
        skb->protocol = eth_type_trans (skb, net_device);
        skb->ip_summed = CHECKSUM_UNNECESSARY;

        /* pass it up to kernel networking layer
         */
        if ((rc = netif_rx (skb))) 
                TRACE_MSG1(NTT,"netif_rx rc: %d", rc);
        
        Network_net_device_stats.rx_bytes += skb->len;
        Network_net_device_stats.rx_packets++;

        return 0;
}

/*! net_os_recv_buffer - forward a received URB, or clean up after a bad one.
 *      data == NULL --> just accumulate stats (count 1 bad buff)
 *      crc_bad != 0     --> count a crc error and free data/skb
 *      trim             --> amount to trim from valid skb
 */
int net_os_recv_buffer(struct usbd_function_instance *function_instance, void *data, int crc_bad, int trim)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct sk_buff *skb = (struct sk_buff *) data;

        RETURN_EAGAIN_UNLESS(npd);

        TRACE_MSG7(NTT, "skb: %x head: %x data: %x tail: %x end: %x len: %d tail-data: %d", 
                        skb, skb->head, skb->data, skb->tail, skb->end, skb->len, skb->tail - skb->data); 

        TRACE_MSG3(NTT, "npd: %x skb: %x flags: %04x", npd, skb, npd->flags);

        UNLESS (skb) {
                /* Lower layer never got around to allocating an skb, but
                 * needs to count a packet it can't forward. 
                 */
                Network_net_device_stats.rx_frame_errors++;
                Network_net_device_stats.rx_errors++;
                TRACE_MSG0(NTT, "NO SKB");
                return -EAGAIN;
        } 

        /* There is an skb, either forward it or free it.
         */
        if (crc_bad) {
                Network_net_device_stats.rx_crc_errors++;
                Network_net_device_stats.rx_errors++;
                TRACE_MSG0(NTT, "BAD CRC");
                return -EAGAIN;
        } 

        /* is the network up?
         */
        UNLESS (npd->net_device) {

                // Something wrong, free the skb
                //dev_kfree_skb_any (skb);
                // The received buffer didn't get forwarded, so...
                Network_net_device_stats.rx_dropped++;
                TRACE_MSG0(NTT, "NO NETWORK");
                return -EAGAIN;
        }

        /* Trim if necessary and pass up
         */
        if (trim) 
                skb_trim(skb, skb->len - trim);
        
        TRACE_MSG0(NTT, "OK");
        return network_recv(function_instance, npd->net_device, skb);
}

extern void config_bh (void *data);
extern void hotplug_bh (void *data);

/*! net_os_enable
 *
 */
void net_os_enable(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd);
        TRACE_MSG3(NTT,"npd: %p function: %p, f->p: %p", npd, function_instance, function_instance->privdata);

        memset(&Network_net_device_stats , 0, sizeof Network_net_device_stats);
        npd->net_device = &Network_net_device;
        npd->net_device->priv = function_instance;
#ifdef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG
        PREPARE_WORK_ITEM(npd->config_bh, config_bh, function_instance);
#endif /* CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG */
#ifdef CONFIG_OTG_NETWORK_HOTPLUG
        PREPARE_WORK_ITEM(npd->hotplug_bh, hotplug_bh, function_instance);
#endif /* CONFIG_OTG_NETWORK_HOTPLUG */
        PREPARE_WORK_ITEM(npd->notification_bh, notification_bh, npd);

        /* set the network device address from the local device address
         */
        memcpy(npd->net_device->dev_addr, npd->local_dev_addr, ETH_ALEN);
}

/*! net_os_disable
 * 
 */
extern void net_os_disable(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd && npd->net_device);
        npd->net_device->priv = NULL;
        npd->net_device = NULL;
}

/*
 *
 */
void net_os_carrier_on(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd);
        netif_carrier_on(npd->net_device);
        netif_wake_queue(npd->net_device);
#ifdef CONFIG_OTG_NET_NFS_SUPPORT
        RETURN_UNLESS (usb_is_configured);
        wake_up(&usb_netif_wq);
        usb_is_configured = 1;
#endif
}

/*
 *
 */
void net_os_carrier_off(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd);
        printk("%s check NULL npd->net_device %d\n ",__FUNCTION__, __LINE__);
        RETURN_UNLESS(npd->net_device);
        netif_stop_queue(npd->net_device);
        netif_carrier_off(npd->net_device);
}


//_______________________________Network Layer Functions____________________________________

/*
 * In general because the functions are called from an independant layer it is necessary
 * to verify that the device is still ok and to lock interrupts out to prevent in-advertant
 * closures while in progress.
 */

/* network_init -
 *
 * Initializes the specified network device.
 *
 * Returns non-zero for failure.
 */
STATIC int network_init (struct net_device *net_device)
{
        TRACE_MSG0(NTT,"no-op");
        return 0;
}


/* network_uninit -
 *
 * Uninitializes the specified network device.
 */
STATIC void network_uninit (struct net_device *net_device)
{
        TRACE_MSG0(NTT,"no-op");
        return;
}


/* network_open -
 *
 * Opens the specified network device.
 *
 * Returns non-zero for failure.
 */
STATIC int network_open (struct net_device *net_device)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) net_device->priv;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        unsigned long flags;
        RETURN_ZERO_UNLESS(function_instance);
        RETURN_ZERO_UNLESS(npd);

        if (npd)
                npd->flags |= NETWORK_OPEN;
        netif_wake_queue (net_device);

        local_irq_save(flags);
        net_fd_send_int_notification(function_instance, 1, 0);
        local_irq_restore(flags);

#ifdef CONFIG_OTG_NET_NFS_SUPPORT
        if (!usb_is_configured) {
                if (!in_interrupt()) {
                        printk(KERN_ERR"Please replug USB cable and then ifconfig host interface.\n");
                        interruptible_sleep_on(&usb_netif_wq);
                }
                else {
                        printk(KERN_ERR"Warning! In interrupt\n");
                }
        }
#endif
        return 0;
}


/* network_stop -
 *
 * Stops the specified network device.
 *
 * Returns non-zero for failure.
 */
STATIC int network_stop (struct net_device *net_device)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) net_device->priv;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        unsigned long flags;

        TRACE_MSG0(NTT,"-");
        
        RETURN_ZERO_UNLESS(npd);
        printk("%s set npd->flag  - %d\n ",__FUNCTION__, __LINE__);
        npd->flags &= ~NETWORK_OPEN;
        local_irq_save(flags);
        net_fd_send_int_notification(function_instance, 0, 0);
        local_irq_restore(flags);

        return 0;
}


/* network_get_stats -
 *
 * Gets statistics from the specified network device.
 *
 * Returns pointer to net_device_stats structure with the required information.
 */
STATIC struct net_device_stats *network_get_stats (struct net_device *net_device)
{
        //struct usbd_function_instance *function_instance = (struct usbd_function_instance *) net_device->priv;
        //struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        
        //if (npd)
                //return &npd->stats;
        //else
                return &Network_net_device_stats;  /* network device statistics */
}


/* network_set_mac_addr
 *
 * Sets the MAC address of the specified network device. Fails if the device is in use.
 *
 * Returns non-zero for failure.
 */
STATIC int network_set_mac_addr (struct net_device *net_device, void *p)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) net_device->priv;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        struct sockaddr *addr = p;
        unsigned long flags;

        TRACE_MSG0(NTT,"--");

        RETURN_EBUSY_IF(netif_running (net_device));
        RETURN_ZERO_UNLESS(npd);
        local_irq_save(flags);
        memcpy(net_device->dev_addr, addr->sa_data, net_device->addr_len);
        memcpy(npd->local_dev_addr, npd->net_device->dev_addr, ETH_ALEN);
        local_irq_restore(flags);
        return 0;
}


/* network_tx_timeout -
 *
 * Tells the specified network device that its current transmit attempt has timed out.
 */
STATIC void network_tx_timeout (struct net_device *net_device)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) net_device->priv;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

#if 0
        Network_net_device_stats.tx_errors++;
        Network_net_device_stats.tx_dropped++;
        usbd_cancel_urb_irq (npd->bus, NULL);  // QQSV
#endif
#if 0
        // XXX attempt to wakeup the host...
        if ((npd->network_type == network_blan) && (npd->flags & NETWORK_OPEN)) {
                notification_schedule_bh();
        }
#endif
}


/** network_set_config -
 *
 * Sets the specified network device's configuration. Fails if the device is in use.
 *
 * map           An ifmap structure containing configuration values.
 *                      Those values which are non-zero/non-null update the corresponding fields
 *                      in net_device.
 *
 * Returns non-zero for failure.
 */
STATIC int network_set_config (struct net_device *net_device, struct ifmap *map)
{
        RETURN_EBUSY_IF(netif_running (net_device));
        if (map->base_addr) 
                net_device->base_addr = map->base_addr;
        if (map->mem_start) 
                net_device->mem_start = map->mem_start;
        if (map->irq) 
                net_device->irq = map->irq;
        return 0;
}


/* network_change_mtu -
 *
 * Sets the specified network device's MTU. Fails if the new value is larger and
 * the device is in use.
 *
 * Returns non-zero for failure.
 */
STATIC int network_change_mtu (struct net_device *net_device, int mtu)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) net_device->priv;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        TRACE_MSG0(NTT,"-");

        RETURN_EBUSY_IF(netif_running (net_device));
        RETURN_ZERO_UNLESS(npd);
        RETURN_EBUSY_IF(mtu > npd->mtu);
        npd->mtu = mtu;
        return 0;
}

//_________________________________________________________________________________________________
//                                      network_hard_start_xmit

/*
 * net_os_start_xmit - start sending an skb, with usbd_network_sem already held.
 *
 * Return non-zero (1) if busy. QQSV - this code always returns 0.
 */
STATIC __inline__ int net_os_start_xmit (struct sk_buff *skb, struct net_device *net_device)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) net_device->priv;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        int rc;

        RETURN_ZERO_UNLESS(npd);

        if (!netif_carrier_ok(net_device)) {
                dev_kfree_skb_any (skb);
                Network_net_device_stats.tx_dropped++;
                return 0;
        }

        // stop queue, it will be restarted only when we are ready for another skb
        netif_stop_queue(net_device);
        //npd->stopped++;

        // Set the timestamp for tx timeout
        net_device->trans_start = jiffies;

        // XXX request IN, should start a timer to resend this. 
        // XXX net_fd_send_int_notification(function_instance, 0, 1);

        switch ((rc = net_fd_start_xmit(function_instance, skb->data, skb->len, (void*)skb))) {
        case 0:
                TRACE_MSG1(NTT,"OK: %d", rc);
                // update some stats
                Network_net_device_stats.tx_packets++;
                Network_net_device_stats.tx_bytes += skb->len;

                // Should we restart network queue
                if ((npd->queued_entries < npd->max_queue_entries) && (npd->queued_bytes < npd->max_queue_bytes)) 
                        netif_wake_queue (net_device);
                break;

        case -EINVAL:
        case -EUNATCH:
                TRACE_MSG1(NTT,"not attached, send failed: %d", rc);
                printk(KERN_ERR"%s: not attached, send failed: %d\n", __FUNCTION__, rc);
                Network_net_device_stats.tx_errors++;
                Network_net_device_stats.tx_carrier_errors++;
                netif_wake_queue(net_device);
                break;

        case -ENOMEM:
                TRACE_MSG1(NTT,"no mem, send failed: %d", rc);
                printk(KERN_ERR"%s: no mem, send failed: %d\n", __FUNCTION__, rc);
                Network_net_device_stats.tx_errors++;
                Network_net_device_stats.tx_fifo_errors++;
                netif_wake_queue(net_device);
                break;

        case -ECOMM:
                TRACE_MSG2(NTT,"comm failure, send failed: %d %p", rc, net_device);
                printk(KERN_ERR"%s: comm failure, send failed: %d %p\n", __FUNCTION__, rc, net_device);
                // Leave the IF queue stopped.
                Network_net_device_stats.tx_dropped++;
                break;

        }
        if (0 != rc) {
                dev_kfree_skb_any (skb);
                // XXX this is what we should do, blows up on some 2.4.20 kernels
                // return(NET_XMIT_DROP);
                return 0;
        }
        return 0;
}

/*
 * network_hard_start_xmit - start sending an skb.  Called by the OS network layer.
 *
 * Return non-zero (1) if busy. QQSV - this code always returns 0.
 */
STATIC int network_hard_start_xmit (struct sk_buff *skb, struct net_device *net_device)
{
        int rc = 0;
        down(&usbd_network_sem);
        rc = net_os_start_xmit(skb, net_device);
        up(&usbd_network_sem);
        return rc;
}


/* network_do_ioctl - perform an ioctl call 
 *
 * Carries out IOCTL commands for the specified network device.
 *
 * rp            Points to an ifreq structure containing the IOCTL parameter(s).
 * cmd           The IOCTL command.
 *
 * Returns non-zero for failure.
 */
STATIC int network_do_ioctl (struct net_device *net_device, struct ifreq *rp, int cmd)
{
        return -ENOIOCTLCMD;
}

//_________________________________________________________________________________________________

struct net_device Network_net_device = {
        .get_stats = network_get_stats,
        .tx_timeout = network_tx_timeout,
        .do_ioctl = network_do_ioctl,
        .set_config = network_set_config,
        .set_mac_address = network_set_mac_addr,
        .hard_start_xmit = network_hard_start_xmit,
        .change_mtu = network_change_mtu,
        .init = network_init,
        .uninit = network_uninit,
        .open = network_open,
        .stop = network_stop,
        .priv = NULL,
        .name =  NET_DEV_NAME,
};


//_________________________________________________________________________________________________

#ifdef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG
/* sock_ioctl - perform an ioctl call to inet device
 */
static int sock_ioctl(u32 cmd, struct ifreq *ifreq)
{
        int rc = 0;
        mm_segment_t save_get_fs = get_fs();
        TRACE_MSG1(NTT, "cmd: %x", cmd);
        set_fs(get_ds());
        rc = devinet_ioctl(cmd, ifreq);
        set_fs(save_get_fs);
        return rc;
}

/* sock_addr - setup a socket address for specified interface
 */
static int sock_addr(char * ifname, u32 cmd, u32 s_addr)
{
        struct ifreq ifreq;
        struct sockaddr_in *sin = (void *) &(ifreq.ifr_ifru.ifru_addr);

        TRACE_MSG2(NTT, "ifname: %s addr: %x", ifname, ntohl(s_addr));

        memset(&ifreq, 0, sizeof(ifreq));
        strcpy(ifreq.ifr_ifrn.ifrn_name, ifname); 

        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = s_addr;
        
        return sock_ioctl(cmd, &ifreq);
}


/* sock_flags - set flags for specified interface
 */
static int sock_flags(char * ifname, u16 oflags, u16 sflags, u16 rflags)
{
        int rc = 0;
        struct ifreq ifreq;

        TRACE_MSG4(NTT, "ifname: %s oflags: %x s_flags: %x r_flags: %x", ifname, oflags, sflags, rflags);

        memset(&ifreq, 0, sizeof(ifreq));
        strcpy(ifreq.ifr_ifrn.ifrn_name, ifname); 

        oflags |= sflags;
        oflags &= ~rflags;
        ifreq.ifr_flags = oflags;

        TRACE_MSG1(NTT, "-> ifr_flags: %x ", ifreq.ifr_flags);

        THROW_IF ((rc = sock_ioctl(SIOCSIFFLAGS, &ifreq)), error);

        TRACE_MSG1(NTT, "<- ifr_flags: %x ", ifreq.ifr_flags);

        CATCH(error) {
                TRACE_MSG1(NTT, "ifconfig: cannot get/set interface flags (%d)", rc);
                return rc;
        }
        return rc;
}

/* network_attach - configure interface
 *
 * This will use socket calls to configure the interface to the supplied
 * ip address and make it active.
 */
STATIC int network_attach(struct usbd_function_instance *function_instance, u32 ip, u32 mask, u32 router, int attach)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        int err = 0;

        RETURN_ZERO_UNLESS(npd);

        if (attach) {
                u16 oflags = (npd && npd->net_device) ? npd->net_device->flags : 0;
                TRACE_MSG5(NTT, "ATTACH npd: %x ip: %08x mask: %08x router: %08x attach: %d", npd, ip, mask, router, attach);

                if (npd->net_device)
                        memcpy(npd->net_device->dev_addr, npd->local_dev_addr, ETH_ALEN);

                /* setup ip address, netwask, and broadcast address */
                if (ip) {
                        THROW_IF ((err = sock_addr(NET_DEV_NAME, SIOCSIFADDR, htonl(ip))), error);
                        if (mask) {
                                THROW_IF ((err = sock_addr(NET_DEV_NAME, SIOCSIFNETMASK, htonl(mask))), error);
                                THROW_IF ((err = sock_addr(NET_DEV_NAME, SIOCSIFBRDADDR, htonl(ip | ~mask))), error);
                        }
                        /* bring the interface up */
                        THROW_IF ((err = sock_flags(NET_DEV_NAME, oflags, IFF_UP, 0)), error);
                }
        }
        else {
                u16 oflags = (npd && npd->net_device) ? npd->net_device->flags : 0;
                TRACE_MSG5(NTT, "DETACH npd: %x ip: %08x mask: %08x router: %08x attach: %d", npd, ip, mask, router, attach);
                /* bring the interface down */
                THROW_IF ((err = sock_flags(NET_DEV_NAME, oflags, 0, IFF_UP)), error);
        }

        CATCH(error) {
                TRACE_MSG1(NTT, "ifconfig: cannot configure interface (%d)", err);
                return err;
        }
        return 0;
}

/* config_bh() - configure network
 *
 * Check connected status and load/unload as appropriate.
 *
 */
void config_bh (void *data)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) data;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        RETURN_UNLESS(npd);

        TRACE_MSG5(NTT, "function: %x enabled: %x status: %x state: %x npd->config_status: %d", 
                        function_instance, 
                        npd->flags & NETWORK_ENABLED,
                        usbd_get_device_status(function_instance),
                        usbd_get_device_state(function_instance),
                        npd->config_status);

        if ((npd->flags & NETWORK_ENABLED ) && 
                        (function_instance && (USBD_OK == usbd_get_device_status(function_instance)) 
                         && (STATE_CONFIGURED == usbd_get_device_state(function_instance)) && ip_addr)) 
        {
                TRACE_MSG2(NTT,"BUS state: %d status: %d", usbd_get_device_state(function_instance), 
                                usbd_get_device_status(function_instance));
                if (config_attached != npd->config_status) {
                        TRACE_MSG0(NTT,"ATTACH");
                        npd->config_status = config_attached;
                        network_attach (function_instance, ip_addr, network_mask, router_ip, 1);
                }
                return;
        }

        if (config_detached != npd->config_status) {
                TRACE_MSG0(NTT,"DETACH");
                npd->config_status = config_detached;
                network_attach (function_instance, ip_addr, network_mask, router_ip, 0);
        }
}

/*
 * net_os_config - schedule a call to config bottom half
 */
void net_os_config(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        TRACE_MSG0(NTT, "CONFIG");
        RETURN_UNLESS(npd);
        SET_WORK_ARG(npd->config_bh, function_instance);
        SCHEDULE_WORK(npd->config_bh);
}
#else /* CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG */
void net_os_config(struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(NTT, "NOT CONFIGURED");
        // Do nothing, config not enabled.
}
#endif /* CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG */
//______________________________________ Hotplug Functions ________________________________________

#ifdef CONFIG_OTG_NETWORK_HOTPLUG

#define AGENT "network_fd"

/* hotplug_attach - call hotplug 
 */
STATIC int hotplug_attach(struct usbd_function_instance *function_instance, u32 ip, u32 mask, u32 router, int attach)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        static int count = 0;
        char *argv[3];
        char *envp[10];
        char ifname[20+12 + IFNAMSIZ];
        int i;
        char count_str[20];

        RETURN_UNLESS(npd);
        RETURN_EINVAL_IF(!hotplug_path[0]);

        argv[0] = hotplug_path;
        argv[1] = AGENT;
        argv[2] = 0;

        sprintf (ifname, "INTERFACE=%s", npd->net_device->name);
        sprintf (count_str, "COUNT=%d", count++);

        i = 0;
        envp[i++] = "HOME=/";
        envp[i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
        envp[i++] = ifname;


        if (attach) {
                unsigned char *cp;
                char ip_str[20+32];
                char mask_str[20+32];
                char router_str[20+32];
                u32 nh;

                nh = htonl(ip);
                cp = (unsigned char*) &nh;
                sprintf (ip_str, "IP=%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);

                nh = htonl(mask);
                cp = (unsigned char*) &nh;
                sprintf (mask_str, "MASK=%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);

                nh = htonl(router);
                cp = (unsigned char*) &nh;
                sprintf (router_str, "ROUTER=%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);

                //TRACE_MSG3(NTT, "attach %s %s %s", ifname, ip_str, count_str);

                envp[i++] = "ACTION=attach";
                envp[i++] = ip_str;
                envp[i++] = mask_str;
                envp[i++] = router_str;

                TRACE_MSG3(NTT, "attach ip: %08x mask: %08x router: %08x", ip, mask, router);
        }
        else {
                //TRACE_MSG2(NTT,"detach %s %s", ifname, count_str);
                envp[i++] = "ACTION=detach";
                TRACE_MSG3(NTT, "detach ip: %08x mask: %08x router: %08x", ip, mask, router);
        }

        envp[i++] = count_str;
        envp[i++] = 0;

        return call_usermodehelper (argv[0], argv, envp);
}


/* hotplug_bh - bottom half handler to call hotplug script to signal ATTACH or DETACH
 *
 * Check connected status and load/unload as appropriate.
 *
 * It should not be possible for this to be called more than once at a time
 * as it is only called via schedule_task() which protects against a second
 * invocation.
 */
STATIC void hotplug_bh (void *data)
{
        struct usbd_function_instance *function_instance = (struct usbd_function_instance *) data;
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);

        RETURN_UNLESS(npd);

        function_instance = (struct usbd_function_instance *)npd->function_instance;

        TRACE_MSG2(NTT, "function: %x npd->hotplug_status: %d", function_instance, npd->hotplug_status);


        if ((npd->flags & NETWORK_ENABLED ) && (function_instance && (USBD_OK == usbd_get_device_status(function_instance)) 
                         && (STATE_CONFIGURED == usbd_get_device_state(function_instance)))) 
        {
                TRACE_MSG2(NTT,"BUS state: %d status: %d", usbd_get_device_state(function_instance), 
                                usbd_get_device_status(function_instance));
                if (hotplug_attached != npd->hotplug_status) {
                        TRACE_MSG0(NTT,"ATTACH");
                        npd->hotplug_status = hotplug_attached;
                        hotplug_attach (function_instance, ip_addr, network_mask, router_ip, 1);
                }
                return;
        }

        if (hotplug_detached != npd->hotplug_status) {
                TRACE_MSG0(NTT,"DETACH");
                npd->hotplug_status = hotplug_detached;
                hotplug_attach (function_instance, ip_addr, network_mask, router_ip, 0);
        }
}

/*
 * net_os_hotplug - schedule a call to hotplug bottom half
 */
void net_os_hotplug(struct usbd_function_instance *function_instance)
{
        struct usb_network_private *npd = (struct usb_network_private *) (function_instance ? function_instance->privdata : NULL);
        RETURN_UNLESS(npd);
        SET_WORK_ARG(npd->hotplug_bh, function_instance);
        SCHEDULE_WORK(npd->hotplug_bh);
}
#else
void net_os_hotplug(struct usbd_function_instance *function_instance)
{
        TRACE_MSG0(NTT, "NOT CONFIGURED");
        // Do nothing, hotplug not configured.
}
#endif                  /* CONFIG_OTG_NETWORK_HOTPLUG */

//_________________________________________________________________________________________________

/* network_create - create and initialize network private structure
 *
 * Returns non-zero for failure.
 */
STATIC int network_create(void)
{

        u16 oflags =  0;

        TRACE_MSG0(NTT,"entered");

        // Set some fields to generic defaults and register the network device with the kernel networking code

        ether_setup(&Network_net_device);
        RETURN_EINVAL_IF (register_netdev(&Network_net_device));

        netif_stop_queue(&Network_net_device);
        netif_carrier_off(&Network_net_device);
        Network_net_device.flags &= ~IFF_UP;


        TRACE_MSG0(NTT,"finis");
        return 0;
}

/* network_destroy - destroy network private struture
 *
 * Destroys the network interface referenced by the global variable @c Network_net_device.
 */
STATIC void network_destroy(void)
{
        netif_stop_queue(&Network_net_device);
        netif_carrier_off(&Network_net_device);
        unregister_netdev(&Network_net_device);
}


//______________________________________module_init and module_exit________________________________
#ifdef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG

/*! network_proc_read_ip - implement proc file system read.
 * Standard proc file system read function.
 */
static ssize_t network_proc_read_ip (struct file *file, char *buf, size_t count, loff_t * pos)
{
        unsigned long page;
        int len = 0;
        char *bp;
        int index = (*pos)++;

        RETURN_ZERO_IF(index);

        // get a page, max 4095 bytes of data...
        RETURN_ENOMEM_UNLESS ((page = GET_KERNEL_PAGE()));

        bp = (char *) page;

        len = sprintf(bp, "%d.%d.%d.%d\n", 
                        (router_ip >> 24) & 0xff,
                        (router_ip >> 16) & 0xff,
                        (router_ip >> 8) & 0xff,
                        (router_ip) & 0xff
                        );

        if (copy_to_user(buf, (char *) page, len)) 
                len = -EFAULT;
        
        free_page (page);
        return len;
}

static struct file_operations otg_message_proc_switch_functions = {
      read: network_proc_read_ip,
};

#endif


/* network_modinit - driver intialization
 *
 * Returns non-zero for failure.
 */
STATIC int network_modinit (void)
{
        #if defined(CONFIG_PROC_FS) && defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG)
        struct proc_dir_entry *p;
        #endif /* defined(CONFIG_PROC_FS) && defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) */

        BOOL blan = FALSE, basic = FALSE, basic2 = FALSE, cdc = FALSE, eem = FALSE,
             safe = FALSE, net_fd = FALSE, rnddis = FALSE, procfs = FALSE;

        NTT = otg_trace_obtain_tag();
#if 0
        THROW_UNLESS (blan = BOOLEAN(!blan_mod_init()), error);
        THROW_UNLESS (basic = BOOLEAN(!basic_mod_init()), error);
        THROW_UNLESS (basic2 = BOOLEAN(!basic2_mod_init()), error);
        THROW_UNLESS (cdc = BOOLEAN(!cdc_mod_init()), error);
        THROW_UNLESS (safe = BOOLEAN(!safe_mod_init()), error);
        //THROW_UNLESS (eem = BOOLEAN(!eem_mod_init()), error);
        //THROW_UNLESS (rndis = BOOLEAN(!rndis_mod_init()), error);

#else
	UNLESS (blan = BOOLEAN(!blan_mod_init()))  blan_mod_exit();
        UNLESS (basic = BOOLEAN(!basic_mod_init())) basic_mod_exit();
        UNLESS (basic2 = BOOLEAN(!basic2_mod_init())) basic2_mod_exit();
        UNLESS (cdc = BOOLEAN(!cdc_mod_init())) cdc_mod_exit();
        UNLESS (safe = BOOLEAN(!safe_mod_init())) safe_mod_exit();
        //THROW_UNLESS (eem = BOOLEAN(!eem_mod_init()), error);
        //THROW_UNLESS (rndis = BOOLEAN(!rndis_mod_init()), error);

#endif
	THROW_UNLESS ((blan || basic || basic2 || cdc || safe),error);

  	THROW_UNLESS (net_fd = BOOLEAN(!net_fd_init("network", MODPARM(local_dev_addr), MODPARM(remote_dev_addr))), error);
	

        
        init_waitqueue_head(&usb_netif_wq);

        #if defined(CONFIG_PROC_FS) && defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG)
        if ((p = create_proc_entry (NET_PROCFS_ENTRY, 0666, 0))) 
                p->proc_fops = &otg_message_proc_switch_functions;
        procfs = TRUE;
        #endif /* defined(CONFIG_PROC_FS) && defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) */

        THROW_IF (network_create(), error);

        return 0;

        CATCH(error) {
                if (net_fd) net_fd_exit(); 

                #if defined(CONFIG_PROC_FS) && defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG)
                if (procfs) remove_proc_entry(NET_PROCFS_ENTRY, NULL);
                #endif /* defined(CONFIG_PROC_FS) && defined(CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG) */
#if 0
                if (blan) blan_mod_exit();
                if (basic) basic_mod_exit();
                if (basic2) basic2_mod_exit();
                if (cdc) cdc_mod_exit();
                if (safe) safe_mod_exit();
                //if (eem) eem_mod_exit();
                //if (rndis) rndis_mod_exit();
#endif

                otg_trace_invalidate_tag(NTT);
                return -EINVAL;
        }
}

//_____________________________________________________________________________

/* network_modexit - driver exit
 *
 * Cleans up the module. Deregisters the function driver and destroys the network object.
 */
STATIC void network_modexit (void)
{
        network_destroy();
        net_fd_exit(); 

        #ifdef CONFIG_PROC_FS
        remove_proc_entry(NET_PROCFS_ENTRY, NULL);
        #endif /* CONFIG_PROC_FS */

        blan_mod_exit();
        basic_mod_exit();
        basic2_mod_exit();
        cdc_mod_exit();
        safe_mod_exit();
        //eem_mod_exit();
        //rndis_mod_exit();


        otg_trace_invalidate_tag(NTT);
}

//_________________________________________________________________________________________________
#ifdef CONFIG_OTG_NFS
late_initcall (network_modinit);
#else
module_init (network_modinit);
#endif
module_exit (network_modexit);

