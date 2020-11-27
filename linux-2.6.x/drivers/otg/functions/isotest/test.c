/*
 * otg/isotest_fd/test.c
 * @(#) balden@seth2.belcarratech.com|otg/functions/isotest/test.c|20051116204957|25624
 *
 *      Copyright (c) 2003-2004 Belcarra
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *
 *      Copyright (c) 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         dn Initial distribution
 * 10/18/2006         Motorola         ma sk pw Add Open Src Software language
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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/version.h>

#include <linux/kernel.h>
//#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
//#include <linux/poll.h>
#include <linux/init.h>

#include <asm/system.h>
#include <asm/atomic.h>
//#include <linux/interrupt.h>
//#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <linux/proc_fs.h>
#include <linux/vmalloc.h>

#include <asm/io.h>
#include <asm/string.h>

#include <linux/proc_fs.h>

#include <linux/netdevice.h>
#include <linux/cache.h>


#if defined(CONFIG_ARCH_SA1100) || defined (CONFIG_ARCH_PXA)
#include <asm/dma.h>
#include <asm/mach/dma.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/types.h>
#endif

#if defined(CONFIG_MIPS_AU1000) || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
#include <asm/au1000.h>
#include <asm/au1000_dma.h>
#include <asm/mipsregs.h>
#endif

#if defined(CONFIG_ARCH_SAMSUNG)
#include <asm/arch/timers.h>
#include <asm/arch/hardware.h>
#endif

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include "test.h"


int iso_trace_first;
int iso_trace_next;
iso_trace_t *iso_traces;


/* ******************************************************************************************* */

void dump_stats(struct isotest_stats *stats)
{
        if (stats->errors || ((stats->ok % 100) == 99)) {
                TRACE_ISO(stats->iso_transfer_number, stats->total_frames,
                                stats->received, stats->errors, stats->missed, stats->skipped, stats->ok);
        }
        if (stats->errors) {
                memset(stats, 0, sizeof(struct isotest_stats));
        }
        else {
                stats->ok++;
                stats->iso_transfer_number++;
                stats->last_packet = 0;
        }
}

u16 getu16(u8 **cp)
{
        u16     val = 0;

        val = *(*cp)++;
        val |= *(*cp)++ << 8;

        return val;
}

u16 getu32(u8 **cp)
{
        u32     val = 0;

        val = *(*cp)++;
        val |= *(*cp)++ << 8;
        val |= *(*cp)++ << 16;
        val |= *(*cp)++ << 24;

        return val;
}

static int iso_in_received;

void iso_trace_recv_data (struct isotest_stats *stats, u8 *cp, int length, int framenum)
{
        //struct usb_device_instance *device = urb->device;

        u16     iso_transfer_number;
        u16     total_size;
        u16     packet_size;
        u16     total_frames;
        u16     this_packet;
        u8      frames;
        
        
        iso_transfer_number = getu32(&cp);
        total_size = getu16(&cp);
        packet_size = getu16(&cp);
        total_frames = getu16(&cp);
        this_packet = getu16(&cp);

                
        // did we miss the end of the last ISO transfer?
        if (stats->iso_transfer_number && (stats->iso_transfer_number != iso_transfer_number)) {
                stats->errors++;
                stats->missed++;
                //printk(KERN_INFO"%s: ERROR bad iso number: %x %x %x %x %x expecting: %x\n", __FUNCTION__, iso_transfer_number,
                //                total_size, packet_size, total_frames, this_packet, stats->iso_transfer_number);
                dump_stats(stats);
                return;
        }
                
        if (!stats->iso_transfer_number) {
                stats->framenum = framenum;
                stats->iso_transfer_number = iso_transfer_number;
                stats->total_frames = total_frames;
        }

        // is this the packet we are expecting?
        if (stats->last_packet && ((stats->last_packet + 1) != this_packet)) {
                stats->errors++;
                stats->missed++;
                //printk(KERN_INFO"%s: ERROR bad packet: %x %x %x %x %x expecting: %x\n", __FUNCTION__, iso_transfer_number,
                //                total_size, packet_size, total_frames, this_packet, stats->last_packet + 1);
        }
        
        // did we miss a frame? 
        frames = (stats->framenum < framenum) ?  (framenum - stats->framenum) : (stats->framenum - framenum);
        
        if (frames > 1) {
                stats->errors++;
                stats->skipped++;
                //printk(KERN_INFO"%s: SKIPPED %x %x %x %x %x\n", __FUNCTION__, iso_transfer_number,
                //                total_size, packet_size, total_frames, this_packet);
        }

        // update stats
        stats->received++;
        stats->last_packet = this_packet;

        // last packet?
        if (stats->total_frames == this_packet) 
                dump_stats(stats);
}



/* Proc Filesystem *************************************************************************** */
        
/* *    
 * iso_trace_proc_read - implement proc file system read.
 * @file        
 * @buf         
 * @count
 * @pos 
 *      
 * Standard proc file system read function.
 */         
static ssize_t iso_trace_proc_read (struct file *file, char *buf, size_t count, loff_t * pos)
{                                  
        unsigned long page;
        int len = 0;
        int index;
        int oindex;
        int previous;

        // XXX MOD_INC_USE_COUNT;
        //printk(KERN_INFO"%s: GET_USE_COUNT: %d\n", __FUNCTION__, GET_USE_COUNT(THIS_MODULE));
        // get a page, max 4095 bytes of data...
        #if 0
        if (!(page = get_free_page (GFP_KERNEL))) {
                // XXX MOD_DEC_USE_COUNT;
                //printk(KERN_INFO"%s: GET_USE_COUNT: %d\n", __FUNCTION__, GET_USE_COUNT(THIS_MODULE));
                return -ENOMEM;
        }
        #endif

        len = 0;
        oindex = index = (*pos)++;

        if (index == 0) {
              //len += sprintf ((char *) page + len, "      uS              Exp  Rcv   Err  Miss  Skip  OK\n");
              //len += sprintf ((char *) page + len, "      uS            Exp  Rcv   Err  Miss  Skip  Xfers\n");
                len += sprintf ((char *) page + len, "      uS           Tr   Exp   Rcv   Err  Miss  Skip  Xfers\n");

//      uS            Exp  Rcv   Err  Miss  Skip  Xfers
//      uS          Tr     Exp  Rcv   Err  Miss  Skip  Xfers
//        0  |      100     9   900     0     0     0    99

        }       
                         
        index += iso_trace_first;
        if (index >= ISO_TRACE_MAX) {
                index -= ISO_TRACE_MAX;
        }
        previous = (index) ? (index - 1) : (ISO_TRACE_MAX - 1);

        //printk(KERN_INFO"first: %d next: %d index: %d %d prev: %d\n", iso_trace_first, iso_trace_next, oindex, index, previous);

        if (
                        ((iso_trace_first < iso_trace_next) && (index >= iso_trace_first) && (index < iso_trace_next)) ||
                        ((iso_trace_first > iso_trace_next) && ((index < iso_trace_next) || (index >= iso_trace_first)))
           )
        {

#if defined(CONFIG_ARCH_SA1100) || defined (CONFIG_ARCH_PXA) || defined(CONFIG_MIPS_AU1000)  || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
                u32 ticks = 0;
#elif defined(CONFIG_ARCH_SAMSUNG)
                u32 ticks = 0;
#else
                u64 jifs = 0;
#endif
                iso_trace_t *p = iso_traces + index;
                //unsigned char *cp;
                //int skip = 0;

                if (oindex > 0) {
                        iso_trace_t *o = iso_traces + previous;

#if defined(CONFIG_ARCH_SA1100) || defined (CONFIG_ARCH_PXA)
                        /*
                         * oscr is 3.6864 Mhz free running counter, 
                         *
                         *      1/3.6864 = .2712
                         *      60/221   = .2714
                         *
                         */
                        if (o->ocsr) {
                                ticks = (p->ocsr > o->ocsr) ? (p->ocsr - o->ocsr) : (o->ocsr - p->ocsr) ;
                                ticks = (ticks * 60) / 221;
                        }

#elif defined(CONFIG_MIPS_AU1000)  || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
                        /*
                         * cp0_count is incrementing timer at system clock
                         */
                        if (o->cp0_count) {
                                //ticks = (p->cp0_count > o->cp0_count) ? 
                                //        (p->cp0_count - o->cp0_count) : (o->cp0_count - p->cp0_count) ;
                                ticks = (p->cp0_count - o->cp0_count) ;
                                ticks = ticks / CONFIG_OTG_AU1X00_SCLOCK;
                        }

#elif defined(CONFIG_ARCH_SAMSUNG)
                        /*
                         * tcnt1 is a count-down timer running at the system bus clock
                         * The divisor must be set as a configuration value, typically 66 or 133.
                         */
                        if (o->tcnt1) {
                                ticks = (p->tcnt1 < o->tcnt1) ?  (o->tcnt1 - p->tcnt1) : (p->tcnt1 - o->tcnt1) ;
                                ticks /= CONFIG_OTG_SMDK2500_BCLOCK;
                        }
#else
                        if (o->jiffies) {
                                jifs = p->jiffies - iso_traces[previous].jiffies;
                        }
#endif

                        //if (o->interrupts != p->interrupts) {
                        //        skip++;
                        //}
                }
                

#if defined(CONFIG_ARCH_SA1100) || defined (CONFIG_ARCH_PXA) || defined(CONFIG_MIPS_AU1000) || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
                len += sprintf ((char *) page + len, "%9d  ", ticks);
#elif defined(CONFIG_ARCH_SAMSUNG)
                //len += sprintf ((char *) page + len, "%8u ", p->jiffies);
                //len += sprintf ((char *) page + len, "%8u ", p->tcnt0);
                len += sprintf ((char *) page + len, "%8u ", p->tcnt1);
                if (ticks > 1024*1024) {
                        len += sprintf ((char *) page + len, "%8dM ", ticks>>20);
                }
                else {
                        len += sprintf ((char *) page + len, "%8d  ", ticks);
                }
#else
                if (jifs > 1024) {
                        len += sprintf ((char *) page + len, "%4dK", (int)jifs>>20);
                }
                else {
                        len += sprintf ((char *) page + len, "%4d  ", (int)jifs);
                }
#endif

                len += sprintf ((char *) page + len, "| %8d", p->iso_transfer_number);
                len += sprintf ((char *) page + len, "%6d", p->expected);
                len += sprintf ((char *) page + len, "%6d", p->received);
                len += sprintf ((char *) page + len, "%6d", p->errors);
                len += sprintf ((char *) page + len, "%6d", p->missed);
                len += sprintf ((char *) page + len, "%6d", p->skipped);
                len += sprintf ((char *) page + len, "%6d", p->ok);
                len += sprintf ((char *) page + len, "\n");
        }

#if 1
        if ((len > count) ) {
                printk(KERN_INFO"%s: ((len > count)  count=%d len=%d\n", __FUNCTION__, count, len);
                len = -EINVAL;
        } 

#else
        if ((len > count) || (len == 0)) {
                printk(KERN_INFO"%s: ((len > count) || (len == 0)) count=%d len=%d\n", __FUNCTION__, count, len);
                len = -EINVAL;
        } 
#endif
        else if (len > 0 && copy_to_user (buf, (char *) page, len)) {
                printk(KERN_INFO"%s: (len > 0 && copy_to_user (buf, (char *) page, len) \n", __FUNCTION__);
                len = -EFAULT;
        }
        free_page (page);
        // XXX MOD_DEC_USE_COUNT;
        //printk(KERN_INFO"%s: GET_USE_COUNT: %d\n", __FUNCTION__, GET_USE_COUNT(THIS_MODULE));
        return len;
}

void iso_start_in(int count);
void iso_start_out(int count);

/* *
 * iso_trace_proc_write - implement proc file system write.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Proc file system write function, used to signal monitor actions complete.
 * (Hotplug script (or whatever) writes to the file to signal the completion
 * of the script.)  An ugly hack.
 */
static ssize_t iso_trace_proc_write (struct file *file, const char *buf, size_t count, loff_t * pos)
{
        //struct usb_device_instance *device;
        size_t n = count;
        char command[64];
        char *cp = command;
        int i = 0;

        // XXX MOD_INC_USE_COUNT;
        //printk(KERN_INFO"%s: GET_USE_COUNT: %d\n", __FUNCTION__, GET_USE_COUNT(THIS_MODULE));
        //printk(KERN_DEBUG "%s: count=%u\n",__FUNCTION__,count);
        while ((n > 0) && (i < 64)) {
                // Not too efficient, but it shouldn't matter
                if (copy_from_user (cp++, buf + (count - n), 1)) {
                        count = -EFAULT;
                        break;
                }
                *cp = '\0';
                i++;
                n -= 1;
                //printk(KERN_DEBUG "%s: %u/%u %02x\n",__FUNCTION__,count-n,count,c);
        }
        if (!strncmp (command, "in", 2)) {
                iso_start_in (10);
        } 
        else if (!strncmp (command, "out", 3)) {
                iso_start_out (10);
        } 
        // XXX need to be able to set serial number here
        // XXX MOD_DEC_USE_COUNT;
        //printk(KERN_INFO"%s: GET_USE_COUNT: %d\n", __FUNCTION__, GET_USE_COUNT(THIS_MODULE));
        return (count);
}

static struct file_operations iso_trace_proc_operations_functions = {
read:iso_trace_proc_read,
write:iso_trace_proc_write,
};

#if defined(CONFIG_ARCH_SAMSUNG)
#endif

/**
 * iso_trace_init
 *
 * Return non-zero if not successful.
 */
int iso_trace_init (char *name)
{
        printk(KERN_INFO"%s:\n", __FUNCTION__);
        if (!(iso_traces = vmalloc(sizeof(iso_trace_t) * ISO_TRACE_MAX))) {
                printk(KERN_ERR"%s: malloc failed %p %d\n", __FUNCTION__, iso_traces, sizeof(iso_trace_t) * ISO_TRACE_MAX);
                return -EINVAL;
        }
        memset(iso_traces, 0, sizeof(iso_trace_t) * ISO_TRACE_MAX);

        {
                struct proc_dir_entry *p;

                // create proc filesystem entries
                if ((p = create_proc_entry (name, 0, 0)) == NULL) {
                        printk(KERN_INFO"BITRACE PROC FS failed\n");
                }
                else {
                        p->proc_fops = &iso_trace_proc_operations_functions;
                }
        }
#if defined(CONFIG_ARCH_SAMSUNG)
        *(volatile u32 *)TMOD |= 0x3 << 3;
#endif
        printk(KERN_INFO"%s: OK\n", __FUNCTION__);
        return 0;
}

/**
 * udc_release_io - release UDC io region
 */
void iso_trace_exit (char *name)
{
        {
                unsigned long flags;
                local_irq_save (flags);
                remove_proc_entry (name, NULL);
                if (iso_traces) {
                        iso_trace_t *p = iso_traces;
                        iso_traces = 0;
                        vfree(p);
                }
                local_irq_restore (flags);
        }
}


/* End of FILE */

