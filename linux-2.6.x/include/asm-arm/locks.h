/*
 *  linux/include/asm-arm/locks.h
 *
 *  Copyright (C) 2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Interrupt safe locking assembler. 
 */

/*
 *  Copyright (C) 2006-2007 Motorola, Inc.
 *
 * Date         Author          Comment
 * 10/2006      Motorola        Added __down_op_ret_timeout
 * 07/2007      Motorola        Fix bugs in up() function
 * 08/2007      Motorola        Add comments.
 */

#ifndef __ASM_PROC_LOCKS_H
#define __ASM_PROC_LOCKS_H

#if __LINUX_ARM_ARCH__ >= 6

#define __compat_down_op(ptr,fail)		\
	({					\
	__asm__ __volatile__(			\
	"@ compat_down_op\n"			\
"1:	ldrex	lr, [%0]\n"			\
"	sub	lr, lr, %1\n"			\
"	strex	ip, lr, [%0]\n"			\
"	teq	ip, #0\n"			\
"	bne	1b\n"				\
"	teq	lr, #0\n"			\
"	movmi	ip, %0\n"			\
"	blmi	" #fail				\
	:					\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	})

#define __compat_down_op_ret(ptr,fail)		\
	({					\
		unsigned int ret;		\
	__asm__ __volatile__(			\
	"@ compat_down_op_ret\n"		\
"1:	ldrex	lr, [%1]\n"			\
"	sub	lr, lr, %2\n"			\
"	strex	ip, lr, [%1]\n"			\
"	teq	ip, #0\n"			\
"	bne	1b\n"				\
"	teq	lr, #0\n"			\
"	movmi	ip, %1\n"			\
"	movpl	ip, #0\n"			\
"	blmi	" #fail "\n"			\
"	mov	%0, ip"				\
	: "=&r" (ret)				\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	ret;					\
	})

#ifdef CONFIG_MOT_FEAT_DOWN_TIMEOUT
#define __down_op_ret_timeout(ptr,tmo,fail)	\
	({					\
		unsigned int ret;		\
	__asm__ __volatile__(			\
	"@ __down_op_ret_timeout\n"		\
"1:	ldrex	lr, [%1]\n"			\
"	sub	lr, lr, %3\n"			\
"	strex	ip, lr, [%1]\n"			\
"	teq	ip, #0\n"			\
"	bne	1b\n"				\
"	mov	ip, #0\n"			\
"	teq	lr, #0\n"			\
"	bpl	2f\n"				\
"       stmfd   sp!, {r0 - r3}\n"		\
"       mov     r0, %1\n"			\
"       mov     r1, %2\n"			\
"	bl	" #fail "\n"			\
"       mov     ip, r0\n"			\
"       ldmfd   sp!, {r0 - r3}\n"		\
"2:	mov	%0, ip"				\
	: "=&r" (ret)				\
	: "r" (ptr), "r" (tmo), "I" (1)		\
	: "ip", "lr", "cc", "memory");		\
	ret;					\
	})
#endif

#ifdef CONFIG_MOT_WFN495
#define __compat_up_op(ptr,wake)		\
	({					\
	__asm__ __volatile__(			\
	"@ compat_up_op\n"			\
"1:	ldrex	lr, [%0]\n"			\
"	add	lr, lr, %1\n"			\
"	strex	ip, lr, [%0]\n"			\
"	teq	ip, #0\n"			\
"	bne	1b\n"				\
"	cmp	lr, #0\n"			\
"	movle	ip, %0\n"			\
"	blle	" #wake				\
	:					\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	})
#else
#define __compat_up_op(ptr,wake)                \
	({                                      \
	__asm__ __volatile__(                   \
	"@ compat_up_op\n"                      \
"1:	ldrex   lr, [%0]\n"                     \
"	add     lr, lr, %1\n"                   \
"	strex   ip, lr, [%0]\n"                 \
"	teq     ip, #0\n"                       \
"	bne     1b\n"                           \
"	teq     lr, #0\n"                       \
"	movle   ip, %0\n"                       \
"	blle    " #wake                         \
	:                                       \
	: "r" (ptr), "I" (1)                    \
	: "ip", "lr", "cc", "memory");          \
	})
#endif /* CONFIG_MOT_WFN495 */



/*
 * The value 0x01000000 supports up to 128 processors and
 * lots of processes.  BIAS must be chosen such that sub'ing
 * BIAS once per CPU will result in the long remaining
 * negative.
 */
#define RW_LOCK_BIAS      0x01000000
#define RW_LOCK_BIAS_STR "0x01000000"

#define __compat_down_op_write(ptr,fail)	\
	({					\
	__asm__ __volatile__(			\
	"@ compat_down_op_write\n"		\
"1:	ldrex	lr, [%0]\n"			\
"	sub	lr, lr, %1\n"			\
"	strex	ip, lr, [%0]\n"			\
"	teq	ip, #0\n"			\
"	bne	1b\n"				\
"	teq	lr, #0\n"			\
"	movne	ip, %0\n"			\
"	blne	" #fail				\
	:					\
	: "r" (ptr), "I" (RW_LOCK_BIAS)		\
	: "ip", "lr", "cc", "memory");		\
	})

#ifdef CONFIG_MOT_WFN495
#define __comapat_up_op_write(ptr,wake)		\
	({					\
	__asm__ __volatile__(			\
	"@ compat_up_op_read\n"			\
"1:	ldrex	lr, [%0]\n"			\
"	adds	lr, lr, %1\n"			\
"	strex	ip, lr, [%0]\n"			\
"	teq	ip, #0\n"			\
"	bne	1b\n"				\
"	movcs	ip, %0\n"			\
"	blcs	" #wake				\
	:					\
	: "r" (ptr), "I" (RW_LOCK_BIAS)		\
	: "ip", "lr", "cc", "memory");		\
	})
#else
#define __comapat_up_op_write(ptr,wake)         \
        ({                                      \
        __asm__ __volatile__(                   \
        "@ compat_up_op_read\n"                 \
"1:	ldrex   lr, [%0]\n"                     \
"	add     lr, lr, %1\n"                   \
"	strex   ip, lr, [%0]\n"                 \
"	teq     ip, #0\n"                       \
"	bne     1b\n"                           \
"	movcs   ip, %0\n"                       \
"	blcs    " #wake                         \
	:                                       \
	: "r" (ptr), "I" (RW_LOCK_BIAS)         \
	: "ip", "lr", "cc", "memory");          \
	})
#endif /* CONFIG_MOT_WFN495 */

#define __compat_down_op_read(ptr,fail)		\
	__compat_down_op(ptr, fail)

#define __compat_up_op_read(ptr,wake)		\
	({					\
	__asm__ __volatile__(			\
	"@ compat_up_op_read\n"			\
"1:	ldrex	lr, [%0]\n"			\
"	add	lr, lr, %1\n"			\
"	strex	ip, lr, [%0]\n"			\
"	teq	ip, #0\n"			\
"	bne	1b\n"				\
"	teq	lr, #0\n"			\
"	moveq	ip, %0\n"			\
"	bleq	" #wake				\
	:					\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	})

#else

#define __compat_down_op(ptr,fail)			\
	({					\
	__asm__ __volatile__(			\
	"@ compat_down_op\n"				\
"	mrs	ip, cpsr\n"			\
"	orr	lr, ip, #128\n"			\
"	msr	cpsr_c, lr\n"			\
"	ldr	lr, [%0]\n"			\
"	subs	lr, lr, %1\n"			\
"	str	lr, [%0]\n"			\
"	msr	cpsr_c, ip\n"			\
"	movmi	ip, %0\n"			\
"	blmi	" #fail				\
	:					\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	})

#define __compat_down_op_ret(ptr,fail)		\
	({					\
		unsigned int ret;		\
	__asm__ __volatile__(			\
	"@ compat_down_op_ret\n"		\
"	mrs	ip, cpsr\n"			\
"	orr	lr, ip, #128\n"			\
"	msr	cpsr_c, lr\n"			\
"	ldr	lr, [%1]\n"			\
"	subs	lr, lr, %2\n"			\
"	str	lr, [%1]\n"			\
"	msr	cpsr_c, ip\n"			\
"	movmi	ip, %1\n"			\
"	movpl	ip, #0\n"			\
"	blmi	" #fail "\n"			\
"	mov	%0, ip"				\
	: "=&r" (ret)				\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	ret;					\
	})

#define __compat_up_op(ptr,wake)		\
	({					\
	__asm__ __volatile__(			\
	"@ compat_compat_up_op\n"		\
"	mrs	ip, cpsr\n"			\
"	orr	lr, ip, #128\n"			\
"	msr	cpsr_c, lr\n"			\
"	ldr	lr, [%0]\n"			\
"	adds	lr, lr, %1\n"			\
"	str	lr, [%0]\n"			\
"	msr	cpsr_c, ip\n"			\
"	movle	ip, %0\n"			\
"	blle	" #wake				\
	:					\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	})

/*
 * The value 0x01000000 supports up to 128 processors and
 * lots of processes.  BIAS must be chosen such that sub'ing
 * BIAS once per CPU will result in the long remaining
 * negative.
 */
#define RW_LOCK_BIAS      0x01000000
#define RW_LOCK_BIAS_STR "0x01000000"

#define __compat_down_op_write(ptr,fail)	\
	({					\
	__asm__ __volatile__(			\
	"@ compat_down_op_write\n"			\
"	mrs	ip, cpsr\n"			\
"	orr	lr, ip, #128\n"			\
"	msr	cpsr_c, lr\n"			\
"	ldr	lr, [%0]\n"			\
"	subs	lr, lr, %1\n"			\
"	str	lr, [%0]\n"			\
"	msr	cpsr_c, ip\n"			\
"	movne	ip, %0\n"			\
"	blne	" #fail				\
	:					\
	: "r" (ptr), "I" (RW_LOCK_BIAS)		\
	: "ip", "lr", "cc", "memory");		\
	})

#define __compat_up_op_write(ptr,wake)		\
	({					\
	__asm__ __volatile__(			\
	"@ compat_up_op_read\n"			\
"	mrs	ip, cpsr\n"			\
"	orr	lr, ip, #128\n"			\
"	msr	cpsr_c, lr\n"			\
"	ldr	lr, [%0]\n"			\
"	adds	lr, lr, %1\n"			\
"	str	lr, [%0]\n"			\
"	msr	cpsr_c, ip\n"			\
"	movcs	ip, %0\n"			\
"	blcs	" #wake				\
	:					\
	: "r" (ptr), "I" (RW_LOCK_BIAS)		\
	: "ip", "lr", "cc", "memory");		\
	})

#define __compat_down_op_read(ptr,fail)		\
	__compat_down_op(ptr, fail)

#define __compat_up_op_read(ptr,wake)			\
	({					\
	__asm__ __volatile__(			\
	"@ compat_up_op_read\n"			\
"	mrs	ip, cpsr\n"			\
"	orr	lr, ip, #128\n"			\
"	msr	cpsr_c, lr\n"			\
"	ldr	lr, [%0]\n"			\
"	adds	lr, lr, %1\n"			\
"	str	lr, [%0]\n"			\
"	msr	cpsr_c, ip\n"			\
"	moveq	ip, %0\n"			\
"	bleq	" #wake				\
	:					\
	: "r" (ptr), "I" (1)			\
	: "ip", "lr", "cc", "memory");		\
	})


#endif
#endif
