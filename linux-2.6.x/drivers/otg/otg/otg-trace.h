/*
 * otg/otgcore/otg-trace.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-trace.h|20051116205002|02341
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
 *
 */
/*!
 * @file otg/otg/otg-trace.h
 * @brief Core Defines for USB OTG Core Layaer
 *
 * Fast Trace Utility
 * This set of definitions and code is meant to provide a _fast_ debugging facility
 * (much faster than printk) so that time critical code can be debugged by looking
 * at a trace of events provided by reading a file in the procfs without affecting
 * the timing of events in the critical code.
 *
 * The mechanism used it to allocate a (large) ring buffer of relatively small structures
 * that include location and high-res timestamp info, and up to 8 bytes of optional
 * data.  Values are stored and timestamps are taken as the critical code runs, but
 * data formatting and display are done during the procfs read, when more time is
 * available :).
 *
 * Note that there is usually some machine dependent code involved in getting the
 * high-res timestamp, and there may be other bits used just to keep the overall
 * time impact as low as possible.
 *
 * Varargs up to 9 arguments are now supported, but you have to supply the number
 * of args, since examining the format string for the number at trace event time
 * was deemed too expensive time-wise.
 * 
 *
 * @ingroup OTGCore
 */

#ifndef OTG_TRACE_H
#define OTG_TRACE_H 1

//#include <linux/interrupt.h>



typedef u8 otg_tag_t;  // 1-origin tags (0 ==> invalid/unused trace entry.  Max is 32.

#ifdef PRAGMAPACK
#pragma pack(push,1)
#endif /* PRAGMAPACK */
typedef PACKED_ENUM otg_trace_types {
        otg_trace_msg_invalid_n,
        otg_trace_msg_va_start_n,
        otg_trace_msg_va_list_n,
        otg_trace_setup_n,
        otg_trace_msg_n,
        otg_trace_msg32_n,
        otg_trace_msg16_n,
        otg_trace_msg8_n
} PACKED_ENUM_EXTRA otg_trace_types_t;
#ifdef PRAGMAPACK
#pragma pack(pop)
#endif /* PRAGMAPACK */

typedef struct otg_trace_msg {
        char    *msg;
} otg_trace_msg_t;

typedef struct otg_trace_msg32 {
        char    *msg;
        u32      val;
} otg_trace_msg32_t;

typedef struct otg_trace_msg16 {
        char    *msg;
        u16     val0;
        u16     val1;
} otg_trace_msg16_t;

typedef struct otg_trace_msg8 {
        char    *msg;
        u8      val0;
        u8      val1;
        u8      val2;
        u8      val3;
} otg_trace_msg8_t;

#define OTG_TRACE_MAX_IN_VA                  7
typedef struct otg_trace_msg_va_list {
        u32     val[OTG_TRACE_MAX_IN_VA];
} otg_trace_msg_va_list_t;

typedef struct otg_trace_msg_va_start {
        const char    *function;
        u32     interrupts;
        u64     ticks;
        u16     h_framenum;
        u16     p_framenum;

        union {
                otg_trace_msg_t        msg;
                otg_trace_msg8_t       msg8;
                otg_trace_msg16_t      msg16;
                otg_trace_msg32_t      msg32;
		unsigned char setup[8];

#if 0		
#if !defined(BELCARRA_DEBUG)
                struct usbd_device_request       setup;
#else
                unsigned char setup[8];
#endif /* !defined(BELCARRA_DEBUG) */
#endif

        } trace;
} PACKED2 otg_trace_msg_va_start_t;

typedef PACKED0 struct PACKED1 trace {
        otg_trace_types_t        otg_trace_type;
        otg_tag_t                tag;
        u8                       va_num_args;
        u8                       in_interrupt:1;
        u8                       id_gnd:1;
        u8                       hcd_pcd:2;
        union {
                otg_trace_msg_va_start_t s;
                otg_trace_msg_va_list_t l;
        } va;
} PACKED2 otg_trace_t;

#define TRACE_MAX_IS_2N 1
#define TRACE_MAX  0x00008000
#define TRACE_MASK 0x00007FFF
//#define TRACE_MAX  0x000080
//#define TRACE_MASK 0x00007F


#if defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE)

#if !defined(OLD_TRACE_API)

// The new API hides otg_trace_get_next(), and adds otg_trace_setup() as part of the core.

#if !defined(BELCARRA_DEBUG)
extern void otg_trace_setup(otg_tag_t tag, const char *fn, void *setup);
#define TRACE_SETUP(tag,setup)     otg_trace_setup(tag,__FUNCTION__,setup)
#endif
#define TRACE_MSG0(tag, msg)       otg_trace_msg(tag, __FUNCTION__, 0, msg,  0)                                
#define TRACE_FMSG0(tag, f, msg)   otg_trace_msg(tag, f, 0, msg,  0)                                

#else

extern int otg_trace_first;
extern int otg_trace_last_read;
extern int otg_trace_next;

#if 0

extern otg_trace_t *otg_traces;


otg_trace_t *otg_trace_get_next(otg_tag_t tag, const char *fn, otg_trace_types_t otg_trace_type, int n);

#if !defined(BELCARRA_DEBUG)
/*
 * Trace a setup packet.
 * The BELCARRA_DEBUG guard is so the rest of these functions can be used
 * outside of a USB context, such as debugging TTY flow control.
 */
static __inline__ void otg_trace_setup(otg_tag_t tag, const char *fn, struct usbd_device_request *setup)
{
        if (otg_traces) {
                otg_trace_t *p = otg_trace_get_next(tag,fn,otg_trace_setup_n,1);
                memcpy(&p->va.s.trace.setup, setup, sizeof(struct usbd_device_request));
        }
}
#define TRACE_SETUP(tag,setup) otg_trace_setup(tag,__FUNCTION__,setup)
#endif /* !defined(BELCARRA_DEBUG) */

/*
 * Trace a single non-parameterized message.
 */
static __inline__ void otg_trace_msg_0(otg_tag_t tag, const char *fn, char *msg)
{
        if (otg_traces) {
                otg_trace_t *p = otg_trace_get_next(tag,fn,otg_trace_msg_n,1);
                p->va.s.trace.msg.msg = msg;
        }
}
#define TRACE_MSG0(tag, msg) otg_trace_msg_0(tag, __FUNCTION__, msg)
#define TRACE_FMSG0(tag, f, msg) otg_trace_msg_0(tag, f, msg)

/*
 * Trace a message with a single 32-bit value.
 */
static __inline__ void otg_trace_msg_1xU32(otg_tag_t tag, const char *fn, char *fmt, u32 val)
{
        if (otg_traces) {
                otg_trace_t *p = otg_trace_get_next(tag,fn,otg_trace_msg32_n,1);
                p->va.s.trace.msg32.val = val;
                p->va.s.trace.msg32.msg = fmt;
        }
}
#define OTRACE_MSG1(tag,fmt,val) otg_trace_msg_1xU32(tag,__FUNCTION__,fmt,(u32)val)

/*
 * Trace a message with two 16-bit values.
 */
static __inline__ void otg_trace_msg_2xU16(otg_tag_t tag, const char *fn, char *fmt, u16 val0, u16 val1)
{
        if (otg_traces) {
                otg_trace_t *p = otg_trace_get_next(tag,fn,otg_trace_msg16_n,1);
                p->va.s.trace.msg16.val0 = val0;
                p->va.s.trace.msg16.val1 = val1;
                p->va.s.trace.msg16.msg = fmt;
        }
}
#define OTRACE_MSG2(tag,fmt,val0,val1) otg_trace_msg_2xU16(tag,__FUNCTION__,fmt,(u16)val0,(u16)val1)

/*
 * Trace a message with four 8-bit values.
 */
static __inline__ void otg_trace_msg_4xU8(otg_tag_t tag, const char *fn, char *fmt, u8 val0, u8 val1, u8 val2, u8 val3)
{
        if (otg_traces) {
                otg_trace_t *p = otg_trace_get_next(tag,fn,otg_trace_msg8_n,1);
                p->va.s.trace.msg8.val0 = val0;
                p->va.s.trace.msg8.val1 = val1;
                p->va.s.trace.msg8.val2 = val2;
                p->va.s.trace.msg8.val3 = val3;
                p->va.s.trace.msg8.msg = fmt;
        }
}
#define OTRACE_MSG4(tag,fmt,val0,val1,val2,val3) otg_trace_msg_4xU8(tag,__FUNCTION__,fmt,(u8)val0,(u8)val1,(u8)val2,(u8)val3)
#endif

#endif

extern void otg_trace_msg(otg_tag_t tag, const char *fn, u8 nargs, char *fmt, ...);

#define eprintf(format, args...)  printk (KERN_INFO format , ## args)

#define TRACE_MSG(tag, nargs, fmt, args...) otg_trace_msg(tag, __FUNCTION__, nargs, fmt, ## args)

#define TRACE_MSG1(tag, fmt, a1) \
        otg_trace_msg(tag, __FUNCTION__, 1, fmt,  a1)                                
#define TRACE_MSG2(tag, fmt, a1, a2) \
        otg_trace_msg(tag, __FUNCTION__, 2, fmt,  a1, a2)                            
#define TRACE_MSG3(tag, fmt, a1, a2, a3) \
        otg_trace_msg(tag, __FUNCTION__, 3, fmt,  a1, a2, a3)                        
#define TRACE_MSG4(tag, fmt, a1, a2, a3, a4) \
        otg_trace_msg(tag, __FUNCTION__, 4, fmt,  a1, a2, a3, a4)                    
#define TRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5) \
        otg_trace_msg(tag, __FUNCTION__, 5, fmt,  a1, a2, a3, a4, a5)                
#define TRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6) \
        otg_trace_msg(tag, __FUNCTION__, 6, fmt,  a1, a2, a3, a4, a5, a6)            
#define TRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7) \
        otg_trace_msg(tag, __FUNCTION__, 7, fmt,  a1, a2, a3, a4, a5, a6, a7)        
#define TRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
        otg_trace_msg(tag, __FUNCTION__, 8, fmt,  a1, a2, a3, a4, a5, a6, a7, a8)    
#if 0
#define TRACE_MSG9(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
        otg_trace_msg(tag, __FUNCTION__, 9, fmt,  a1, a2, a3, a4, a5, a6, a7, a8, a9)
#endif
#define TRACE_FMSG1(tag, f, fmt, a1) \
        otg_trace_msg(tag, f, 1, fmt,  a1)                                
#define TRACE_FMSG2(tag, f, fmt, a1, a2) \
        otg_trace_msg(tag, f, 2, fmt,  a1, a2)                            
#define TRACE_FMSG3(tag, f, fmt, a1, a2, a3) \
        otg_trace_msg(tag, f, 3, fmt,  a1, a2, a3)                        
#define TRACE_FMSG4(tag, f, fmt, a1, a2, a3, a4) \
        otg_trace_msg(tag, f, 4, fmt,  a1, a2, a3, a4)                    
#define TRACE_FMSG5(tag, f, fmt, a1, a2, a3, a4, a5) \
        otg_trace_msg(tag, f, 5, fmt,  a1, a2, a3, a4, a5)                
#define TRACE_FMSG6(tag, f, fmt, a1, a2, a3, a4, a5, a6) \
        otg_trace_msg(tag, f, 6, fmt,  a1, a2, a3, a4, a5, a6)            
#define TRACE_FMSG7(tag, f, fmt, a1, a2, a3, a4, a5, a6, a7) \
        otg_trace_msg(tag, f, 7, fmt,  a1, a2, a3, a4, a5, a6, a7)        
#define TRACE_FMSG8(tag, f, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
        otg_trace_msg(tag, f, 8, fmt,  a1, a2, a3, a4, a5, a6, a7, a8)    
#if 0
#define TRACE_FMSG9(tag, f, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
        otg_trace_msg(tag, f, 9, fmt,  a1, a2, a3, a4, a5, a6, a7, a8, a9)
#endif
extern otg_tag_t otg_trace_obtain_tag(void);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);

#elif defined(OTG_WINCE)

#define TRACE_MSG0(tag, msg) \
          DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s"), _T(msg)))

#define TRACE_MSG1(tag, fmt, a1) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d"), _T(fmt), a1))

#define TRACE_MSG2(tag, fmt, a1, a2) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d"), _T(fmt), a1, a2))

#define TRACE_MSG3(tag, fmt, a1, a2, a3) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d"), _T(fmt), a1, a2, a3));

#define TRACE_MSG4(tag, fmt, a1, a2, a3, a4) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d"), _T(fmt), a1, a2, a3, a4));

#define TRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5));

#define TRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5, a6));

#define TRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5, a6, a7));

#define TRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8) \
		  DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CORE - %s %d %d %d %d %d %d %d %d %d"), _T(fmt), a1, a2, a3, a4, a5, a6, a7, a8));

extern otg_tag_t otg_trace_obtain_tag(void);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);

/*
#define TRACE_OTG_CURRENT(o) \
          DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CURRENT - %s (%s) RESET: %08x SET: %08x"),\
                                otg_state_names[o->state], \
                                otg_state_names[o->previous], \
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs))\
						);

#define TRACE_OTG_CHANGE(o) \
          DEBUGMSG(ZONE_INIT,(_T("OTGCORE - CHANGE - %s (%s) RESET: %08x SET: %08x"),\
                                otg_state_names[o->state], \
                                otg_state_names[o->previous], \
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs))\
						);

#define TRACE_OTG_NEW(o) \
          DEBUGMSG(ZONE_INIT,(_T("OTGCORE - NEW - %s (%s) RESET: %08x SET: %08x"),\
                                otg_state_names[o->state], \
                                otg_state_names[o->previous], \
                                (u32)(~o->current_inputs), \
                                (u32)(o->current_inputs))\
						);

#define TRACE_OTG_INPUTS(u) \
          DEBUGMSG(ZONE_INIT,(_T("OTGCORE - INPUTS - RESET: %08x SET: %08x"),\
                                (u32)(u >> 32), \
                                (u32)(u & 0xffffffff))\
						);
*/

extern otg_tag_t otg_trace_obtain_tag(void);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);

#else /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */

#define TRACE_SETUP(tag,setup)

//#define TRACE_MSG(tag,nargs,fmt,...)
//#define TRACE_FMSG(tag, nargs,fmt,...)

#define TRACE_MSG0(tag, fmt) 
#define TRACE_MSG1(tag, fmt, a1) 
#define TRACE_MSG2(tag, fmt, a1, a2) 
#define TRACE_MSG3(tag, fmt, a1, a2, a3) 
#define TRACE_MSG4(tag, fmt, a1, a2, a3, a4) 
#define TRACE_MSG5(tag, fmt, a1, a2, a3, a4, a5) 
#define TRACE_MSG6(tag, fmt, a1, a2, a3, a4, a5, a6) 
#define TRACE_MSG7(tag, fmt, a1, a2, a3, a4, a5, a6, a7) 
#define TRACE_MSG8(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8) 
//#define TRACE_MSG9(tag, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9) 

#define TRACE_FMSG0(tag, f, fmt) 
#define TRACE_FMSG1(tag, f, fmt, a1) 
#define TRACE_FMSG2(tag, f, fmt, a1, a2) 
#define TRACE_FMSG3(tag, f, fmt, a1, a2, a3) 
#define TRACE_FMSG4(tag, f, fmt, a1, a2, a3, a4) 
#define TRACE_FMSG5(tag, f, fmt, a1, a2, a3, a4, a5) 
#define TRACE_FMSG6(tag, f, fmt, a1, a2, a3, a4, a5, a6) 
#define TRACE_FMSG7(tag, f, fmt, a1, a2, a3, a4, a5, a6, a7) 
#define TRACE_FMSG8(tag, f, fmt, a1, a2, a3, a4, a5, a6, a7, a8) 
//#define TRACE_FMSG9(tag, f, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9) 

//static inline otg_tag_t otg_trace_obtain_tag(void)
//{
//        return 0;
//}

//static inline otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag)
//{
//        return 0;
//}

extern otg_tag_t otg_trace_obtain_tag(void);
extern otg_tag_t otg_trace_invalidate_tag(otg_tag_t tag);

#endif /* defined(CONFIG_OTG_TRACE) || defined(CONFIG_OTG_TRACE_MODULE) */

#define USBD usbd_trace_tag
extern otg_tag_t USBD;

//#define ACM acm_trace_tag
//extern otg_tag_t ACM;

//#define NETWORK network_trace_tag
//extern otg_tag_t NETWORK;

#endif /* OTG_TRACE_H */
