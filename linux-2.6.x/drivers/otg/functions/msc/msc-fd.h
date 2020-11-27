/*
 * otg/msc_fd/msc.h - Mass Storage Class
 * @(#) balden@seth2.belcarratech.com|otg/functions/msc/msc-fd.h|20051116204958|27923
 *
 *      Copyright (c) 2003-2004 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright (c) 2005-2007 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 08/04/2005         Motorola         Initial distribution 
 * 12/30/2005         Motorola         Added variables for multiple LUN 
 *                                       support 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/11/2006         Motorola         Changes for Open src compliance.
 * 03/07/2007         Motorola         Added function msc_start_recv_urb_for_write.
 *
 * This Program is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of
 * MERCHANTIBILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
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

#ifndef MSC_FD_H
#define MSC_FD_H 1

extern int msc_dispatch_query_urb(struct usbd_function_instance *, struct usbd_urb *, int , int );
extern int msc_start_sending_csw(struct usbd_function_instance *, u8 );
extern int msc_dispatch_query_urb_zlp(struct usbd_function_instance *, u32 sensedata, u32 info, int status);
extern int msc_start_sending_csw_failed(struct usbd_function_instance *, u32 sensedata, u32 info, int status);
extern int msc_start_recv_urb(struct usbd_function_instance *, int size);
extern int msc_start_recv_urb_for_write(struct usbd_function_instance *, int size);


#define NOCHK 0
#define NOZLP 1
#define SENDZLP 2

#if 1
static __inline__ void TRACE_SENSE(unsigned int sense, unsigned int info)
{
        TRACE_MSG2(MSC, "-->             SENSE: %06x INFO: %08x", sense, info);
}
static __inline__ void TRACE_RLBA(unsigned int lba, unsigned int crc)
{
        TRACE_MSG2(MSC, "<--             rlba [%8x %08x]", lba, crc);
}
static __inline__ void TRACE_SLBA(unsigned int lba, unsigned int crc)
{
        TRACE_MSG2(MSC, "-->             slba [%8x %08x]", lba, crc);
}
static __inline__ void TRACE_TLBA(unsigned int lba, unsigned int crc)
{
        TRACE_MSG2(MSC, "-->             tlba [%8x %08x]", lba, crc);
}
static __inline__ void TRACE_TAG(unsigned int tag, unsigned int frame)
{
        TRACE_MSG2(MSC, "-->             TAG: %8x FRAME: %03x", tag, frame);
}
static __inline__ void TRACE_CBW(char *msg, int val, int check)
{
        TRACE_MSG3(MSC,  " -->            %s %02x check: %d", msg, val, check);
}
static __inline__ void TRACE_RECV(unsigned char *cp)
{
        TRACE_MSG8(MSC, "<--             recv [%02x %02x %02x %02x %02x %02x %02x %02x]",
                        cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
}
static __inline__ void TRACE_SENT(unsigned char *cp)
{
        TRACE_MSG8(MSC, "-->             sent [%02x %02x %02x %02x %02x %02x %02x %02x]",
                        cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
}

#endif


#endif /* MSC_H */
