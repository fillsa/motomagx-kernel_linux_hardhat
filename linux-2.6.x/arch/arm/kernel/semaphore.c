/*
 *  ARM semaphore implementation, taken from
 *
 *  i386 semaphore implementation.
 *
 *  (C) Copyright 1999 Linus Torvalds
 *  Copyright Motorola 2005
 *
 *  Modified for ARM by Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  ---------------------------
 * 09/26/2005   Motorola  Add 'down timeout' services 
 *
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/init.h>

#include <asm/semaphore.h>

/*
 * Semaphores are implemented using a two-way counter:
 * The "count" variable is decremented for each process
 * that tries to acquire the semaphore, while the "sleeping"
 * variable is a count of such acquires.
 *
 * Notably, the inline "up()" and "down()" functions can
 * efficiently test if they need to do any extra work (up
 * needs to do something only if count was negative before
 * the increment operation.
 *
 * "sleeping" and the contention routine ordering is
 * protected by the semaphore spinlock.
 *
 * Note that these functions are only called when there is
 * contention on the lock, and as such all this is the
 * "non-critical" part of the whole semaphore business. The
 * critical part is the inline stuff in <asm/semaphore.h>
 * where we want to avoid any extra jumps and calls.
 */

/*
 * Logic:
 *  - only on a boundary condition do we need to care. When we go
 *    from a negative count to a non-negative, we wake people up.
 *  - when we go from a non-negative count to a negative do we
 *    (a) synchronize with the "sleeper" count and (b) make sure
 *    that we're on the wakeup list before we synchronize so that
 *    we cannot lose wakeup events.
 */
fastcall void __compat_up(struct compat_semaphore *sem)
{
	wake_up(&sem->wait);
}

static DEFINE_RAW_SPINLOCK(semaphore_lock);

fastcall void __sched __compat_down(struct compat_semaphore * sem)
{
	struct task_struct *tsk = current;
	DECLARE_WAITQUEUE(wait, tsk);
	unsigned long flags;

	tsk->state = TASK_UNINTERRUPTIBLE;
	spin_lock_irqsave(&sem->wait.lock, flags);
	add_wait_queue_exclusive_locked(&sem->wait, &wait);

	sem->sleepers++;
	for (;;) {
		int sleepers = sem->sleepers;

		/*
		 * Add "everybody else" into it. They aren't
		 * playing, because we own the spinlock in
		 * the wait_queue_head.
		 */
		if (!atomic_add_negative(sleepers - 1, &sem->count)) {
			sem->sleepers = 0;
			break;
		}
		sem->sleepers = 1;	/* us - see -1 above */
		spin_unlock_irqrestore(&sem->wait.lock, flags);

		schedule();

		spin_lock_irqsave(&sem->wait.lock, flags);
		tsk->state = TASK_UNINTERRUPTIBLE;
	}
	remove_wait_queue_locked(&sem->wait, &wait);
	wake_up_locked(&sem->wait);
	spin_unlock_irqrestore(&sem->wait.lock, flags);
	tsk->state = TASK_RUNNING;
}

fastcall int __sched __compat_down_interruptible(struct compat_semaphore * sem)
{
	int retval = 0;
	struct task_struct *tsk = current;
	DECLARE_WAITQUEUE(wait, tsk);
	unsigned long flags;

	tsk->state = TASK_INTERRUPTIBLE;
	spin_lock_irqsave(&sem->wait.lock, flags);
	add_wait_queue_exclusive_locked(&sem->wait, &wait);

	sem->sleepers++;
	for (;;) {
		int sleepers = sem->sleepers;

		/*
		 * With signals pending, this turns into
		 * the trylock failure case - we won't be
		 * sleeping, and we* can't get the lock as
		 * it has contention. Just correct the count
		 * and exit.
		 */
		if (signal_pending(current)) {
			retval = -EINTR;
			sem->sleepers = 0;
			atomic_add(sleepers, &sem->count);
			break;
		}

		/*
		 * Add "everybody else" into it. They aren't
		 * playing, because we own the spinlock in
		 * wait_queue_head. The "-1" is because we're
		 * still hoping to get the semaphore.
		 */
		if (!atomic_add_negative(sleepers - 1, &sem->count)) {
			sem->sleepers = 0;
			break;
		}
		sem->sleepers = 1;	/* us - see -1 above */
		spin_unlock_irqrestore(&sem->wait.lock, flags);

		schedule();

		spin_lock_irqsave(&sem->wait.lock, flags);
		tsk->state = TASK_INTERRUPTIBLE;
	}
	remove_wait_queue_locked(&sem->wait, &wait);
	wake_up_locked(&sem->wait);
	spin_unlock_irqrestore(&sem->wait.lock, flags);

	tsk->state = TASK_RUNNING;
	return retval;
}

#ifdef CONFIG_MOT_FEAT_DOWN_TIMEOUT
int __sched __down_interruptible_timeout(struct semaphore * sem, signed long timeout)
{
	int retval = 0;
	struct task_struct *tsk = current;
	DECLARE_WAITQUEUE(wait, tsk);

	/* If no timeout is specified, just return since this function is only
	 * called when caller failed to acquire the semaphore.
	 */
	if (timeout <= 0)
	    return -EAGAIN;

	tsk->state = TASK_INTERRUPTIBLE;
	add_wait_queue_exclusive(&sem->wait, &wait);

	spin_lock_irq(&semaphore_lock);
	sem->sleepers ++;
	for (;;) {
		int sleepers = sem->sleepers;

		if (0 >= timeout) {
		    retval = -ETIMEDOUT;
		    sem->sleepers = 0;
		    atomic_add(sleepers, &sem->count);
		    break;
		}

		/*
		 * With signals pending, this turns into
		 * the trylock failure case - we won't be
		 * sleeping, and we can't get the lock as
		 * it has contention. Just correct the count
		 * and exit.
		 */
		if (signal_pending(current)) {
			retval = -EINTR;
			sem->sleepers = 0;
			atomic_add(sleepers, &sem->count);
			break;
		}

		/*
		 * Add "everybody else" into it. They aren't
		 * playing, because we own the spinlock. The
		 * "-1" is because we're still hoping to get
		 * the lock.
		 */
		if (!atomic_add_negative(sleepers - 1, &sem->count)) {
			sem->sleepers = 0;
			break;
		}
		sem->sleepers = 1;	/* us - see -1 above */
		spin_unlock_irq(&semaphore_lock);

		timeout = schedule_timeout(timeout);

		tsk->state = TASK_INTERRUPTIBLE;
		spin_lock_irq(&semaphore_lock);
	}
	spin_unlock_irq(&semaphore_lock);
	tsk->state = TASK_RUNNING;
	remove_wait_queue(&sem->wait, &wait);
	wake_up(&sem->wait);

	return retval;
}
EXPORT_SYMBOL(__down_interruptible_timeout);
#endif

/*
 * Trylock failed - make sure we correct for
 * having decremented the count.
 *
 * We could have done the trylock with a
 * single "cmpxchg" without failure cases,
 * but then it wouldn't work on a 386.
 */
fastcall int __compat_down_trylock(struct compat_semaphore * sem)
{
	int sleepers;
	unsigned long flags;

	spin_lock_irqsave(&sem->wait.lock, flags);
	sleepers = sem->sleepers + 1;
	sem->sleepers = 0;

	/*
	 * Add "everybody else" and us into it. They aren't
	 * playing, because we own the spinlock in the
	 * wait_queue_head.
	 */
	if (!atomic_add_negative(sleepers, &sem->count)) {
		wake_up_locked(&sem->wait);
	}

	spin_unlock_irqrestore(&sem->wait.lock, flags);
	return 1;
}

fastcall int compat_sem_is_locked(struct compat_semaphore *sem)
{
        return (int) atomic_read(&sem->count) < 0;
}
EXPORT_SYMBOL(compat_sem_is_locked);

/*
 * The semaphore operations have a special calling sequence that
 * allow us to do a simpler in-line version of them. These routines
 * need to convert that sequence back into the C sequence when
 * there is contention on the semaphore.
 *
 * ip contains the semaphore pointer on entry. Save the C-clobbered
 * registers (r0 to r3 and lr), but not ip, as we use it as a return
 * value in some cases..
 */
asm("	.section .sched.text,\"ax\"		\n\
	.align	5				\n\
	.globl	__compat_down_failed		\n\
__compat_down_failed:				\n\
	stmfd	sp!, {r0 - r3, lr}		\n\
	mov	r0, ip				\n\
	bl	__compat_down			\n\
	ldmfd	sp!, {r0 - r3, pc}		\n\
						\n\
	.align	5				\n\
	.globl	__compat_down_interruptible_failed \n\
__compat_down_interruptible_failed:			\n\
	stmfd	sp!, {r0 - r3, lr}		\n\
	mov	r0, ip				\n\
	bl	__compat_down_interruptible	\n\
	mov	ip, r0				\n\
	ldmfd	sp!, {r0 - r3, pc}		\n\
						\n\
	.align	5				\n\
	.globl	__compat_down_trylock_failed	\n\
__compat_down_trylock_failed:			\n\
	stmfd	sp!, {r0 - r3, lr}		\n\
	mov	r0, ip				\n\
	bl	__compat_down_trylock		\n\
	mov	ip, r0				\n\
	ldmfd	sp!, {r0 - r3, pc}		\n\
						\n\
	.align	5				\n\
	.globl	__compat_up_wakeup		\n\
__compat_up_wakeup:				\n\
	stmfd	sp!, {r0 - r3, lr}		\n\
	mov	r0, ip				\n\
	bl	__compat_up			\n\
	ldmfd	sp!, {r0 - r3, pc}		\n\
	");

EXPORT_SYMBOL(__compat_down_failed);
EXPORT_SYMBOL(__compat_down_interruptible_failed);
EXPORT_SYMBOL(__compat_down_trylock_failed);
EXPORT_SYMBOL(__compat_up_wakeup);
