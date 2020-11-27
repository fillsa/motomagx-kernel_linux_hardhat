/*
 * otg/isotest_fd/test.h
 *
 *      Copyright (c) 2003-2004 Belcarra
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>, 
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

#if defined(CONFIG_ARCH_SAMSUNG)
#ifndef CONFIG_OTG_SMDK2500_BCLOCK 
#define CONFIG_OTG_SMDK2500_BCLOCK 66
#endif
#endif

#define NUSBD_FRAMENUM             0xB0200038


struct isotest_stats {

        u16     count;

        u16     iso_transfer_number;
        u16     total_size;
        u16     packet_size;
        u16     total_frames;
        u16     last_packet;


        u16     framenum;
        u16     received;
        u16     missed;
        u16     skipped;
        u16     errors;

        u32     ok;

};





typedef struct iso_trace_types {
#if defined(CONFIG_ARCH_SA1100) || defined (CONFIG_ARCH_PXA)
        u32     ocsr;
#elif defined(CONFIG_MIPS_AU1000) || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
        u32     cp0_count;
#elif defined(CONFIG_ARCH_SAMSUNG)
        u32     tcnt1;
#else
        u64     jiffies;
#endif
        u16     iso_transfer_number;

        u16     expected;
        u16     received;
        u16     errors;
        u16     missed;
        u16     skipped;

        u32     ok;

} iso_trace_t;


#define ISO_TRACE_MAX       3000

extern int iso_trace_first;
extern int iso_trace_next;

extern iso_trace_t *iso_traces;

static __inline__ iso_trace_t *ISO_TRACE_NEXT(void)
{
        iso_trace_t *p;

        p = iso_traces + iso_trace_next;

#if defined(CONFIG_ARCH_SA1100) || defined (CONFIG_ARCH_PXA)
        p->ocsr = OSCR;
#elif defined(CONFIG_MIPS_AU1000) || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
        p->cp0_count = read_c0_count();
#elif defined(CONFIG_ARCH_SAMSUNG)
        p->tcnt1 = *(volatile u32 *)TCNT1;
#else
        p->jiffies = jiffies;
#endif
//#if defined(CONFIG_MIPS_AU1000) || defined(CONFIG_MIPS_PB1500) || defined(CONFIG_MIPS_PB1100)
//        p->iso_transfer_number = au_readl(NUSBD_FRAMENUM);
//#endif

        iso_trace_next++;
        iso_trace_next = (iso_trace_next == ISO_TRACE_MAX) ? 0 : iso_trace_next;

        if (iso_trace_next == iso_trace_first) {
                iso_trace_first++;
                iso_trace_first = (iso_trace_first == ISO_TRACE_MAX) ? 0 : iso_trace_first;
        }

        return p;
}

static __inline__ void TRACE_ISO(u16 iso_transfer_number, u16 expected, u16 received, u16 errors, u16 missed, u16 skipped, u32 ok)
{
        iso_trace_t *p = NULL;

        //printk(KERN_INFO"%s: %d %d %d %d %d first: %d next: %d traces: %p\n", __FUNCTION__, 
        //                expected, received, errors, missed, skipped, iso_trace_first, iso_trace_next, traces);

        if (!iso_traces) return;

        p = ISO_TRACE_NEXT();
        p->iso_transfer_number = iso_transfer_number;
        p->expected = expected;
        p->received = received;
        p->errors = errors;
        p->missed = missed;
        p->skipped = skipped;
        p->ok = ok;
}

void iso_trace_recv_data (struct isotest_stats *stats, u8 *cp, int length, int framenum);
int iso_trace_init (char *str);
void iso_trace_exit (char *str);

