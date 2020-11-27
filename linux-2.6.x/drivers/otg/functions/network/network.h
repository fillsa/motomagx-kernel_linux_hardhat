/*
 * otg/functions/network/network.h - Network Function Driver
 * @(#) balden@seth2.belcarratech.com|otg/functions/network/network.h|20051116204958|64349
 *
 *      Copyright (c) 2002-2005 Belcarra
 *
 * By: 
 *      Chris Lynne <cl@belcarra.com>
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 06/08/2005         Motorola         Initial distribution 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
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
 */
/*!
 * @defgroup NetworkFunction Network
 * @ingroup functiongroup 
 */
/*!
 * @file otg/functions/network/network.h
 * @brief Network Function Driver private defines
 *
 * This defines the internal and private structures required
 * by the Network Function Driver.
 *
 * @ingroup NetworkFunction
 */

/*! This is a test
 */
#ifndef NETWORK_FD_H
#define NETWORK_FD_H 1

#include <otg/otg-trace.h>

#define NTT network_fd_trace_tag
extern otg_tag_t network_fd_trace_tag;

// Some platforms need to get rid of "static" when using GDB or looking at ksyms
//#define STATIC
#define STATIC static

typedef enum network_encapsulation {
        simple_net, simple_crc, 
} network_encapsulation_t;

typedef enum network_config_status {
        config_unkown,
        config_attached,
        config_detached
} network_config_status_t;

typedef enum network_hotplug_status {
        hotplug_unkown,
        hotplug_attached,
        hotplug_detached
} network_hotplug_status_t;

typedef enum network_type {
        network_unknown,
        network_blan,
        network_safe,
        network_cdc,
        network_basic,
        network_basic2,
        network_eem,
} network_type_t;

#if 0
struct usb_network_params {
        // enabling switchs
        u32 cdc;
        u32 basic;
        u32 basic2;
        u32 safe;
        u32 blan;
        // capability flags
        u32 cdc_capable;
        u32 basic_capable;
        u32 basic2_capable;
        u32 safe_capable;
        u32 blan_capable;
        // overrides
        u32 vendor_id;
        u32 product_id;
        char *local_mac_address_str;
        char *remote_mac_address_str;
        // other switches
        u32 ep0test;
        u32 zeroconf;
};
#endif

#define CONFIG_OTG_NETWORK_ALLOW_SETID           1

//#define NETWORK_CREATED         0x01
//#define NETWORK_REGISTERED      0x02
#define NETWORK_DESTROYING      0x04
#define NETWORK_ENABLED         0x08
#define NETWORK_CONFIGURED      0x10 
#define NETWORK_OPEN            0x20


#define MCCI_ENABLE_CRC         0x03
#define BELCARRA_GETMAC         0x04

#define BELCARRA_SETTIME        0x04
#define BELCARRA_SETIP          0x05
#define BELCARRA_SETMSK         0x06
#define BELCARRA_SETROUTER      0x07
#define BELCARRA_SETDNS         0x08
#define BELCARRA_PING           0x09
#define BELCARRA_SETFERMAT      0x0a
#define BELCARRA_HOSTNAME       0x0b


// RFC868 - seconds from midnight on 1 Jan 1900 GMT to Midnight 1 Jan 1970 GMT
#define RFC868_OFFSET_TO_EPOCH  0x83AA7E80

struct usb_network_private {

        struct net_device_stats stats;  /* network device statistics */

        int flags;
        struct net_device *net_device;
        struct usbd_function_instance *function_instance;
        struct usbd_simple_driver *simple_driver;
        //struct usb_network_params *params;
        //struct net_usb_services *fd_ops;
        unsigned int maxtransfer;
        rwlock_t rwlock;

        network_config_status_t config_status;
        network_hotplug_status_t hotplug_status;
        network_type_t network_type;

        int state;

        int mtu; 
        int crc;
#if defined(CONFIG_OTG_NETWORK_BLAN_FERMAT)
        int fermat;
#endif

        unsigned int stopped; 
        unsigned int restarts;

        unsigned int max_queue_entries;
        unsigned int max_queue_bytes;

        unsigned int queued_entries;
        unsigned int queued_bytes;

        time_t avg_queue_entries;

        time_t jiffies;
        unsigned long samples;

        int have_interrupt;

        int data_notify;

        struct usbd_urb *int_urb;

        network_encapsulation_t encapsulation;

        struct WORK_ITEM notification_bh;
#ifdef CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG
        struct WORK_ITEM config_bh;
#endif /* CONFIG_OTG_NETWORK_BLAN_AUTO_CONFIG */
#ifdef CONFIG_HOTPLUG
        struct WORK_ITEM hotplug_bh;
#endif /* CONFIG_HOTPLUG */
        __u8 local_dev_addr[ETH_ALEN];
        BOOL local_dev_set;
        __u8 remote_dev_addr[ETH_ALEN];
        BOOL remote_dev_set;

        void *privdata;
};

// XXX this needs to be co-ordinated with rndis.c maximum's
#define MAXFRAMESIZE 2000

#if !defined(CONFIG_OTG_MANUFACTURER)
        #define CONFIG_OTG_MANUFACTURER                "Belcarra"
#endif


#if !defined(CONFIG_OTG_SERIAL_NUMBER_STR)
        #define CONFIG_OTG_SERIAL_NUMBER_STR           ""
#endif

/*
 * Lineo specific 
 */

#define VENDOR_SPECIFIC_CLASS           0xff
#define VENDOR_SPECIFIC_SUBCLASS        0xff
#define VENDOR_SPECIFIC_PROTOCOL        0xff

/*
 * Lineo Classes
 */
#define LINEO_CLASS                     0xff

#define LINEO_SUBCLASS_BASIC_NET          0x01
#define LINEO_SUBCLASS_BASIC_SERIAL       0x02

/*
 * Lineo Protocols
 */
#define LINEO_BASIC_NET_CRC             0x01
#define LINEO_BASIC_NET_CRC_PADDED      0x02

#define LINEO_BASIC_SERIAL_CRC          0x01
#define LINEO_BASIC_SERIAL_CRC_PADDED   0x02


/*
 * endpoint and interface indexes
 */
#define BULK_OUT        0x00
#define BULK_IN         0x01
#define INT_IN          0x02
#define ENDPOINTS       0x03

#define COMM_INTF       0x00
#define DATA_INTF       0x01

extern struct usbd_endpoint_request net_fd_endpoint_requests[ENDPOINTS+1];
extern struct usbd_endpoint_request cdc_int_endpoint_requests[1+1];
extern struct usbd_endpoint_request cdc_data_endpoint_requests[2+1];
extern u8 net_fd_endpoint_index[ENDPOINTS];
extern u8 cdc_data_endpoint_index[2];
extern u8 cdc_int_endpoint_index[1];

/* bmDataCapabilities */
#define BMDATA_CRC                      0x01
#define BMDATA_PADBEFORE                0x02
#define BMDATA_PADAFTER                 0x04
#define BMDATA_FERMAT                   0x08
#define BMDATA_HOSTNAME                 0x10

/* bmNetworkCapabilities */
#define BMNETWORK_SET_PACKET_OK         0x01
#define BMNETWORK_NONBRIDGED            0x02
#define BMNETWORK_DATA_NOTIFY_OK        0x04


/*
 * BLAN Data Plane
 */
//#define CONFIG_OTG_NETWORK_PADBYTES  8
//#define CONFIG_OTG_NETWORK_PADAFTER  1
//#undef CONFIG_OTG_NETWORK_PADBEFORE  
//#define CONFIG_OTG_NETWORK_CRC       1


extern __u8 network_requested_endpoints[ENDPOINTS+1];
extern __u16 network_requested_transferSizes[ENDPOINTS+1];
//extern struct usb_network_private Usb_network_private;
//extern __u8 local_dev_addr[ETH_ALEN];
//extern __u8 remote_dev_addr[ETH_ALEN];

extern struct usbd_function_operations net_fd_function_ops;

struct usbd_class_safe_networking_mdlm_descriptor {
        __u8 bFunctionLength;           // 0x06
        __u8 bDescriptorType;           // 0x24
        __u8 bDescriptorSubtype;        // 0x13
        __u8 bGuidDescriptorType;       // 0x00
        __u8 bmNetworkCapabilities;
        __u8 bmDataCapabilities;
} __attribute__ ((packed));

struct usbd_class_blan_networking_mdlm_descriptor {
        __u8 bFunctionLength;           // 0x07
        __u8 bDescriptorType;           // 0x24
        __u8 bDescriptorSubtype;        // 0x13
        __u8 bGuidDescriptorType;       // 0x01
        __u8 bmNetworkCapabilities;
        __u8 bmDataCapabilities;
        __u8 bPad;
} __attribute__ ((packed));



#if 0
#define NETWORK_CREATED         0x01
#define NETWORK_REGISTERED      0x02
#define NETWORK_DESTROYING      0x04
#define NETWORK_ENABLED         0x08
#define NETWORK_ATTACHED        0x10
#define NETWORK_OPEN            0x20


#define MCCI_ENABLE_CRC         0x03
#define BELCARRA_GETMAC         0x04

#define BELCARRA_SETTIME        0x04
#define BELCARRA_SETIP          0x05
#define BELCARRA_SETMSK         0x06
#define BELCARRA_SETROUTER      0x07
#define BELCARRA_SETDNS         0x08
#define BELCARRA_PING           0x09
#define BELCARRA_SETFERMAT      0x0a
#define BELCARRA_HOSTNAME       0x0b
#define BELCARRA_DATA_NOTIFY    0x0c
#endif

#define RFC868_OFFSET_TO_EPOCH  0x83AA7E80      // RFC868 - seconds from midnight on 1 Jan 1900 GMT to Midnight 1 Jan 1970 GMT


extern __u32 *network_crc32_table;
                        
#define CRC32_INIT   0xffffffff      // Initial FCS value
#define CRC32_GOOD   0xdebb20e3      // Good final FCS value

#define CRC32_POLY   0xedb88320      // Polynomial for table generation
        
#define COMPUTE_FCS(val, c) (((val) >> 8) ^ network_crc32_table[((val) ^ (c)) & 0xff])


#if 0
#if !defined(CONFIG_OTG_NETWORK_CRC_DUFF4) && !defined(CONFIG_OTG_NETWORK_CRC_DUFF8)
/**
 * Copies a specified number of bytes, computing the 32-bit CRC FCS as it does so.
 *
 * dst   Pointer to the destination memory area.
 * src   Pointer to the source memory area.
 * len   Number of bytes to copy.
 * val   Starting value for the CRC FCS.
 *
 * Returns      Final value of the CRC FCS.
 *
 * @sa crc32_pad
 */
static __u32 __inline__ crc32_copy (__u8 *dst, __u8 *src, int len, __u32 val)
{
        for (; len-- > 0; val = COMPUTE_FCS (val, *dst++ = *src++));
        return val;
}

#endif /* DUFFn */
#endif


int net_fd_device_request (struct usbd_function_instance *function_instance, struct usbd_device_request *request);
void net_fd_event_handler (struct usbd_function_instance *function_instance, usbd_device_event_t event, int data);
void net_fd_endpoint_cleared (struct usbd_function_instance *function, int bEndpointAddress);
int net_fd_set_configuration (struct usbd_function_instance *function, int configuration);
int net_fd_set_interface (struct usbd_function_instance *function, int wIndex, int altsetting);
int net_fd_reset (struct usbd_function_instance *function);
int net_fd_suspended (struct usbd_function_instance *function);
int net_fd_resumed (struct usbd_function_instance *function);

int net_fd_function_enable (struct usbd_function_instance *, network_type_t);
void net_fd_function_disable (struct usbd_function_instance *);

int net_fd_start_xmit (struct usbd_function_instance *, u8 *, int , void *);

void net_fd_send_int_notification(struct usbd_function_instance *, int connected, int data);

void net_fd_exit(void);
int net_fd_init(char *info_str, char *, char *);
int net_fd_urb_sent_bulk (struct usbd_urb *urb, int urb_rc);



#endif /* NETWORK_FD_H */
