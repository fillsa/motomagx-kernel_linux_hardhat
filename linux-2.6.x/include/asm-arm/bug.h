#ifndef _ASMARM_BUG_H
#define _ASMARM_BUG_H

/* Copyright (C) 2007 Motorola, Inc. 
 * 
 * Date         Author         Comment
 * ==========   ===========    ===================================
 * 06/01/2007   Motorola       Define CONFIG_MOT_FEAT_CHKSUM.             
 */

#include <linux/config.h>

#ifdef CONFIG_DEBUG_BUGVERBOSE
extern volatile void __bug(const char *file, int line, void *data);

/* give file/line information */
#define BUG()		__bug(__FILE__, __LINE__, NULL)

#ifdef CONFIG_MOT_FEAT_CHKSUM
#define WARN_ON(condition) do { \
	if (unlikely((condition)!=0)) { \
		printk("%s/%d: BUG in %s at %s:%d\n", current->comm, current->pid,__FUNCTION__, __FILE__, __LINE__); \
		dump_stack(); \
	} \
} while (0)
#endif

#else

/* this just causes an oops */
#define BUG()		(*(int *)0 = 0)
#ifdef CONFIG_MOT_FEAT_CHKSUM
#define WARN_ON(condition) do { \
	if (unlikely((condition)!=0)) { \
		printk("%s/%d: BUG in %s at %s:%d\n", current->comm, current->pid,__FUNCTION__, "FILE", __LINE__); \
	}\
} while (0)

#endif
#endif

#define HAVE_ARCH_BUG
#ifdef CONFIG_MOT_FEAT_CHKSUM
#define HAVE_ARCH_WARN_ON
#endif
#include <asm-generic/bug.h>

#endif
