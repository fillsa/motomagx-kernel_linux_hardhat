/*
 * otg/otgcore/otg-trace.c
 * @(#) balden@seth2.belcarratech.com|otg/otgcore/otg-trace.c|20051116205002|51714
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@belcarra.com>, 
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 01/27/2006         Motorola         ACM patch work 
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
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
 * @file otg/otgcore/otg-trace.c
 * @brief OTG Core Debug Trace Facility
 *
 * This implements the OTG Core Trace Facility. 
 *
 * @ingroup OTGCore
 */

#include <otg/otg-compat.h>
#include <otg/otg-module.h>

#if defined(OTG_LINUX)
#include <linux/vmalloc.h>
#include <stdarg.h>
#endif /* defined(OTG_LINUX) */

#include <otg/usbp-chap9.h>
#include <otg/usbp-func.h>
#include <otg/usbp-bus.h>

#include <otg/otg-trace.h>
#include <otg/otg-api.h>

#define STATIC static

#if defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE)

DECLARE_MUTEX(otg_trace_sem);

static int otg_trace_exiting;

#define OTG_TRACE_NAME "trace_otg"

typedef struct otg_trace_snapshot {
        int first;
        int next;
        int total;
        otg_trace_t *traces;
} otg_trace_snapshot;

otg_trace_snapshot otg_snapshot1;
otg_trace_snapshot otg_snapshot2;

static otg_trace_snapshot *otg_trace_current_traces;
static otg_trace_snapshot *otg_trace_other_traces;

STATIC otg_trace_snapshot *rel_snapshot(otg_trace_snapshot *tss)
{
        RETURN_NULL_UNLESS(tss);
        if (tss->traces) {
                vfree(tss->traces);
                tss->traces = NULL;
        }
        //vfree(tss);
        return NULL;
}

STATIC void init_snapshot(otg_trace_snapshot *tss)
{
        tss->first = tss->next = tss->total = 0;
        memset(tss->traces,0,sizeof(otg_trace_t) * TRACE_MAX);
}

STATIC otg_trace_snapshot *get_snapshot(otg_trace_snapshot *tss)
{
        //otg_trace_snapshot *tss;
        unsigned long flags;
        //UNLESS ((tss = vmalloc(sizeof(otg_trace_snapshot)))) {
        //        printk(KERN_ERR "Cannot allocate memory for OTG trace snapshot.\n");
        //        return NULL;
        //}
        
        UNLESS ((tss->traces = vmalloc(sizeof(otg_trace_t) * TRACE_MAX))) {
                printk(KERN_ERR "Cannot allocate memory for OTG trace log.\n");
                return rel_snapshot(tss);
        }
        init_snapshot(tss);
        return tss;
}

/*! otg_trace_get_next
 */
STATIC otg_trace_t *otg_trace_get_next(otg_trace_snapshot *tss, otg_tag_t tag, const char *fn, otg_trace_types_t otg_trace_type, int n)
{
        otg_trace_t *p;

        // Get the next (n) trace slots.

        p = tss->traces + tss->next;
        
        while (n-- > 0) {
                tss->next = (tss->next + 1) & TRACE_MASK;
                if (tss->next == tss->first) {
                        // We have wraparound, bump tss->first
                        tss->first = (tss->first + 1) & TRACE_MASK;
                }
                tss->total++;
        }
        // End of next trace slot.
        
        otg_get_trace_info(p);
        p->otg_trace_type = otg_trace_type;
        p->tag = tag;
        p->va.s.function = fn;

        return p;
}

/* otg_trace_next_va
 */
STATIC otg_trace_t *otg_trace_next_va(otg_trace_snapshot *tss, otg_trace_t *p)
{
        /* Return the next trace slot in a va group.
           No need to be atomic, just worry about wraparound. */
        int nxt = (p - tss->traces);
        nxt = (nxt + 1) & TRACE_MASK;
        p = tss->traces + nxt;
        return(p);
}

/* otg_trace_setup
 */
void otg_trace_setup(otg_tag_t tag, const char *fn, void *setup)
{
        otg_trace_t *p;
        otg_trace_snapshot *tss;
        unsigned long flags;
        local_irq_save(flags);
        UNLESS ((tss = otg_trace_current_traces)) {
                local_irq_restore(flags);
                return;
        }
        // Reserve the trace slots.
        UNLESS ((p = otg_trace_get_next(tss,tag,fn,otg_trace_setup_n,1))) {
                local_irq_restore(flags);
                return;
        }
        p->va_num_args = 0;
        memcpy(&p->va.s.trace.setup, setup, sizeof(p->va.s.trace.setup) /*sizeof(struct usbd_device_request)*/);
        local_irq_restore(flags);
}

/* otg_trace_msg
 */
void otg_trace_msg(otg_tag_t tag, const char *fn, u8 nargs, char *fmt, ...)
{
        int n;
        otg_trace_t *p;
        otg_trace_snapshot *tss;
        unsigned long flags;

        // Figure out how many trace slots we're going to need.
        n = 1;
        if (nargs > 1) {
                n = 2;
                // Too many arguments, bail.
                RETURN_IF (nargs > (OTG_TRACE_MAX_IN_VA+1));
        }

        local_irq_save(flags);
        UNLESS ((tss = otg_trace_current_traces)) {
                local_irq_restore(flags);
                return;
        }
        // Reserve the trace slots.
        UNLESS((p = otg_trace_get_next(tss,tag,fn,otg_trace_msg_va_start_n,n))) {
                local_irq_restore(flags);
                return;
        }
        p->va.s.trace.msg32.msg = fmt;
        p->va_num_args = nargs;
        if (nargs > 0) {
                va_list ap;
                va_start(ap,fmt);
                p->va.s.trace.msg32.val = va_arg(ap,u32);
                if (--nargs > 0) {
                        // We need the next trace slot.
                        int i;
                        while (nargs >= 8) {
                                p = otg_trace_next_va(tss,p);
                                p->otg_trace_type = otg_trace_msg_va_list_n;
                                for (i = 0; i < 8; i++) p->va.l.val[i] = va_arg(ap,u32);
                                nargs -= 8;
                        }
                        p = otg_trace_next_va(tss,p);
                        p->otg_trace_type = otg_trace_msg_va_list_n;
                        for (i = 0; i < nargs; i++) p->va.l.val[i] = va_arg(ap,u32);
                }
                va_end(ap);
        }
        local_irq_restore(flags);
}

/* Proc Filesystem *************************************************************************** */

/* int slot_in_data
 */
STATIC int slot_in_data(otg_trace_snapshot *tss, int slot)
{
        // Return 1 if slot is in the valid data region, 0 otherwise.
        return (((tss->first < tss->next) && (slot >= tss->first) && (slot < tss->next)) ||
                ((tss->first > tss->next) && ((slot < tss->next) || (slot >= tss->first))));

}

/*! otg_trace_read_slot
 *
 * Place a slot pointer in *ps (and possibly *pv), with the older entry (if any) in *po.
 * Return: -1 at end of data
 *          0 if not a valid enty
 *          1 if entry valid but no older valid entry
 *          2 if both entry and older are valid.
 */
STATIC int otg_trace_read_slot(otg_trace_snapshot *tss, int index, otg_trace_t **ps, otg_trace_t **po)
{
        int res;
        int previous;
        otg_trace_t *s,*o;

        *ps = *po = NULL;
        index = (tss->first + index) & TRACE_MASK;
        // Are we at the end of the data?
        if (!slot_in_data(tss,index)) {
                // End of data.
                return(-1);
        }
        /* Nope, there's data to show. */
        s = tss->traces + index;
        if (!s->tag ||
            s->otg_trace_type == otg_trace_msg_invalid_n ||
            s->otg_trace_type == otg_trace_msg_va_list_n) {
                /* Ignore this slot, it's not valid, or is part
                   of a previously processed varargs. */
                return(0);
        }
        *ps = s;
        res = 1;
        // Is there a previous event (for "ticks" calculation)?
        previous = (index - 1) & TRACE_MASK;
        if (previous != tss->next && tss->total > 1) {
                // There is a valid previous event.
                res = 2;
                o = tss->traces + previous;
                /* If the previous event was a varargs event, we want
                   a copy of the first slot, not the last. */
                while (o->otg_trace_type == otg_trace_msg_va_list_n) {
                        previous = (previous - 1) & TRACE_MASK;
                        if (previous == tss->next) {
                                res = 1;
                                o = NULL;
                                break;
                        }
                        o = tss->traces + previous;
                }
                *po = o;
        }
        return(res);
}


/*! otg_trace_proc_read - implement proc file system read.
 *      
 * Standard proc file system read function.
 * @file        
 * @buf         
 * @count
 * @pos 
 */         
int otg_trace_proc_read(char *page, int count, int * pos)
{                                  
        int len = 0;
        int index;
        int oindex;
        int rc;

        u64 ticks = 0;

        unsigned char *cp;
        int skip = 0;
        char hcd_pcd;

        otg_trace_t *o;
        otg_trace_t *s;
        otg_trace_snapshot *tss;

        DOWN(&otg_trace_sem);

        /* Grab the current snapshot, and replace it with the other.  This
         * needs to be atomic WRT otg_trace_msg() calls.
         */
        UNLESS (*pos) {
                unsigned long flags;
                tss = otg_trace_other_traces;
                init_snapshot(tss);                                     // clear previous
                local_irq_save(flags);
                otg_trace_other_traces = otg_trace_current_traces;      // swap snapshots
                otg_trace_current_traces = tss;
                local_irq_restore(flags);
        } 

        tss = otg_trace_other_traces;

        /* Get the index of the entry to read - this needs to be atomic WRT tag operations. 
         */
        s = o = NULL;
        oindex = index = (*pos)++;
        do {
                rc = otg_trace_read_slot(tss,index,&s,&o);
                switch (rc) {
                case -1: // End of data.
                        UP(&otg_trace_sem);
                        return(0);

                case 0: // Invalid slot, skip it and try the next.
                        index = (*pos)++;
                        break;

                case 1: // s valid, o NULL
                case 2: // s and o valid
                        break;
                }
        } while (rc == 0);

        len = 0;

        UNLESS (oindex) 
                //len += sprintf((char *) page + len, "Tag  Index     Ints Framenum   Ticks\n");
                len += sprintf((char *) page + len, "Tag  Ints Framenum   Ticks\n");


        /* If there is a previous trace event, we want to calculate how many
         * ticks have elapsed siince it happened.  Unfortunately, determining
         * if there _is_ a previous event isn't obvious, since we have to watch
         * out for startup, varargs and wraparound. 
         */
        if (o) {
        
                ticks = otg_tmr_elapsed(&s->va.s.ticks, &o->va.s.ticks);

                if ((o->va.s.interrupts != s->va.s.interrupts) || (o->tag != s->tag) || (o->in_interrupt != s->in_interrupt)) 
                        skip++;
        }

        switch (s->hcd_pcd) {
        default:
        case 0: hcd_pcd = ' '; break;
        case 1: hcd_pcd = 'H'; break;
        case 2: hcd_pcd = 'P'; break;
        case 3: hcd_pcd = 'B'; break;
        }

        len += sprintf((char *) page + len, "%s%02x %c%c %6x ", skip?"\n":"", 
                        s->tag, s->id_gnd ? 'A' : 'B', hcd_pcd, s->va.s.interrupts);

        len += sprintf((char *) page + len, "%03x %03x  ", s->va.s.h_framenum & 0x7ff, s->va.s.p_framenum & 0x7ff);


        if (ticks < 10000)
                len += sprintf((char *) page + len, "%6duS", ticks);
        else if (ticks < 10000000)
                len += sprintf((char *) page + len, "%6dmS", do_div(ticks, 1000));
        else 
                len += sprintf((char *) page + len, "%6dS ", do_div(ticks, 1000000));

        ticks = 0LL;


        ticks = otg_tmr_elapsed(&ticks, &s->va.s.ticks);

        len += sprintf((char *) page + len, "%6dmS", do_div(ticks, 1000) & 0xffff);


        len += sprintf((char *) page + len, " %s ", s->in_interrupt ? "--" : "==");
        len += sprintf((char *) page + len, "%-24s: ",s->va.s.function);


        switch (s->otg_trace_type) {
        case otg_trace_msg_invalid_n:
        case otg_trace_msg_va_list_n:
                len += sprintf((char *) page + len, " --          N/A");
                break;

        case otg_trace_msg_va_start_n:
                if (s->va_num_args <= 1) 
                        len += sprintf((char *) page + len, s->va.s.trace.msg32.msg, s->va.s.trace.msg32.val);
                
                else {
                        otg_trace_t *v = otg_trace_next_va(tss,s);
                        len += sprintf((char *) page + len, s->va.s.trace.msg32.msg, s->va.s.trace.msg32.val,
                                       v->va.l.val[0], v->va.l.val[1], v->va.l.val[2], v->va.l.val[3],
                                       v->va.l.val[4], v->va.l.val[5], v->va.l.val[6]);
                }
                break;

        case otg_trace_msg_n:
                len += sprintf((char *) page + len, s->va.s.trace.msg.msg);
                break;

        case otg_trace_msg32_n:
                len += sprintf((char *) page + len, s->va.s.trace.msg32.msg, s->va.s.trace.msg32.val);
                break;

        case otg_trace_msg16_n:
                len += sprintf((char *) page + len, s->va.s.trace.msg16.msg, s->va.s.trace.msg16.val0, s->va.s.trace.msg16.val1);
                break;

        case otg_trace_msg8_n:
                len += sprintf((char *) page + len, s->va.s.trace.msg8.msg, 
                               s->va.s.trace.msg8.val0, s->va.s.trace.msg8.val1, s->va.s.trace.msg8.val2, 
                               s->va.s.trace.msg8.val3);
                break;

        case otg_trace_setup_n:
                cp = (unsigned char *)&s->va.s.trace.setup;
                len += sprintf((char *) page + len, 
                               " --      request [%02x %02x %02x %02x %02x %02x %02x %02x]", 
                               cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
                break;
        }
        len += sprintf((char *) page + len, "\n");

        UP(&otg_trace_sem);
        return len;
}

/*! otg_trace_proc_write - implement proc file system write.
 * @file
 * @buf
 * @count
 * @pos
 *
 * Proc file system write function.
 */
int otg_trace_proc_write (const char *buf, int count, int * pos)
{
#define MAX_TRACE_CMD_LEN  64
        char command[MAX_TRACE_CMD_LEN+1];
        size_t n = count;
        size_t l;
        char c;

        if (n > 0) {
                l = MIN(n,MAX_TRACE_CMD_LEN);
                if (copy_from_user (command, buf, l)) 
                        count = -EFAULT;
                else {
                        n -= l;
                        while (n > 0) {
                                if (copy_from_user (&c, buf + (count - n), 1)) {
                                        count = -EFAULT;
                                        break;
                                }
                                n -= 1;
                        }
                        // Terminate command[]
                        if (l > 0 && command[l-1] == '\n') {
                                l -= 1;
                        }
                        command[l] = 0;
                }
        }

        if (0 >= count) {
                return count;
        }


        return count;
}

static u32 otg_tags_mask = 0x0;
extern void otg_trace_exit (void);

/*! otg_trace_obtain_tag(void)
 */
otg_tag_t otg_trace_obtain_tag(void)
{
        /* Return the next unused tag value [1..32] if one is available,
         * return 0 if none is available. 
         */
        otg_tag_t tag = 0;
        DOWN(&otg_trace_sem);
        if (otg_tags_mask == 0xffffffff || otg_trace_exiting) {
                // They're all in use.
                UP(&otg_trace_sem);
                return tag;
        }
        if (otg_tags_mask != 0x00000000) {
                // 2nd or later tag, search for an unused one
                while (otg_tags_mask & (0x1 << tag)) {
                        tag += 1;
                }
        } 
        
        // Successful 1st or later tag.
        otg_tags_mask |= 0x1 << tag;
        UP(&otg_trace_sem);
        return tag + 1;
}

/*! otg_trace_invalidate_tag(otg_tag_t tag)
 */
otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag)
{
        otg_trace_t *p,*e;
        otg_trace_snapshot *css,*oss;
        unsigned long flags;

        // Nothing to do
        RETURN_ZERO_IF ((0 >= tag) || (tag > 32));
        
        // Grab a local copy of the snapshot pointers.
        local_irq_save(flags);
        css = otg_trace_current_traces;
        oss = otg_trace_other_traces;
        local_irq_restore(flags);


        // Run through all the trace messages, and invalidate any with the given tag.

        DOWN(&otg_trace_sem);
        if (css)
                for (e = TRACE_MAX + (p = css->traces); p < e; p++) {
                        if (p->tag == tag) {
                                p->otg_trace_type = otg_trace_msg_invalid_n;
                                p->tag = 0;
                        }
                }
        if (oss)
                for (e = TRACE_MAX + (p = oss->traces); p < e; p++) {
                        if (p->tag == tag) {
                                p->otg_trace_type = otg_trace_msg_invalid_n;
                                p->tag = 0;
                        }
                }
        
        // Turn off the tag
        otg_tags_mask &= ~(0x1 << (tag-1));
        if (otg_tags_mask == 0x00000000) { /* otg_trace_exit (); */ }
        UP(&otg_trace_sem);
        return 0;
}



/* Module init ************************************************************** */

#define OT otg_trace_init_test
otg_tag_t OT;

/*! otg_trace_init
 *
 * Return non-zero if not successful.
 */
int otg_trace_init (void)
{
        otg_trace_exiting = 0;
        otg_trace_current_traces = otg_trace_other_traces = NULL;
        UNLESS ((otg_trace_current_traces = get_snapshot(&otg_snapshot1)) && 
                        (otg_trace_other_traces = get_snapshot(&otg_snapshot2))) 
        {
                printk(KERN_ERR"%s: malloc failed (2x) %d\n", __FUNCTION__, sizeof(otg_trace_t) * TRACE_MAX);
                otg_trace_current_traces = rel_snapshot(otg_trace_current_traces);
                otg_trace_other_traces = rel_snapshot(otg_trace_other_traces);
                return -ENOMEM;
        }
        OT = otg_trace_obtain_tag();
        RETURN_ENOMEM_IF(!OT);
        TRACE_MSG0(OT,"--");
        return 0;
}

/*! otg_trace_exit - remove procfs entry, free trace data space.
 */
void otg_trace_exit (void)
{
        otg_trace_t *p;
        otg_tag_t tag;

        unsigned long flags;
        otg_trace_snapshot *c,*o;

        otg_trace_invalidate_tag(OT);
        otg_trace_exiting = 1;

        local_irq_save (flags);

        /* Look for any outstanding tags. */
        for (tag = 32; otg_tags_mask && tag > 0; tag--) 
                if (otg_tags_mask & (0x1 << (tag-1))) 
                        otg_trace_invalidate_tag(tag);

        c = otg_trace_current_traces;
        o = otg_trace_other_traces;
        otg_trace_current_traces = otg_trace_other_traces = NULL;
        local_irq_restore (flags);

        c = rel_snapshot(c);
        o = rel_snapshot(o);
}

#else /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */
int otg_trace_init (void) {return 0;}
void otg_trace_exit (void) {}
otg_tag_t otg_trace_obtain_tag(void) { return 0; }
void otg_trace_setup(otg_tag_t tag, const char *fn, void *setup) { }
void otg_trace_msg(otg_tag_t tag, const char *fn, u8 nargs, char *fmt, ...) { }
otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag) { return 0; }
#endif /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */

OTG_EXPORT_SYMBOL(otg_trace_setup);
OTG_EXPORT_SYMBOL(otg_trace_msg);
OTG_EXPORT_SYMBOL(otg_trace_obtain_tag);
OTG_EXPORT_SYMBOL(otg_trace_invalidate_tag);

