/*
 *  linux/fs/exec.c
 *
 *  Copyright (C) 2006-2008 Motorola Inc.
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 */

/* ChangeLog:
 * (mm-dd-yyyy) Author    Comment
 * 06-27-2006   Motorola  Add execve hooks for antivirus protection
 *                        Add execve hooks for aplogger
 *                        Add hooks to perform mot specific coredump
 *                        If execve fails, remove secure_clock capability
 *                          and set current process' security back to 0
 * 04-26-2007   Motorola  Enable full coredump for non-security related application.
 * 02-27-2008   Motorola  Fix full coredump pattern issue
 * 04-15-2008   Motorola  Enable full coredump for applications which will set[u/g]id
 * 05-09-2008   Motorola  Enable aplog backup during full core dump
 * 06-24-2008   Motorola  Make aplog do coredump to MMC/SD card if it is available
 * 07-10-2008   Motorola  Add build label info in full coredump name.
 * 07-15-2008   Motorola  Prompt when coredump generated.
 *
 */

/*
 * #!-checking implemented by tytso.
 */
/*
 * Demand-loading implemented 01.12.91 - no need to read anything but
 * the header into memory. The inode of the executable is put into
 * "current->executable", and page faults do the actual loading. Clean.
 *
 * Once more I can proudly say that linux stood up to being changed: it
 * was less than 2 hours work to get demand-loading completely implemented.
 *
 * Demand loading changed July 1993 by Eric Youngdale.   Use mmap instead,
 * current->executable is only used by the procfs.  This allows a dispatch
 * table to check for several different types  of binary formats.  We keep
 * trying until we recognize the file or we run out of supported binary
 * formats. 
 */

#include <linux/config.h>
#include <linux/slab.h>
#include <linux/file.h>
#include <linux/mman.h>
#include <linux/a.out.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/spinlock.h>
#include <linux/key.h>
#include <linux/personality.h>
#include <linux/binfmts.h>
#include <linux/swap.h>
#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/namei.h>
#include <linux/proc_fs.h>
#include <linux/ptrace.h>
#include <linux/mount.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/rmap.h>

#ifdef CONFIG_MOT_FEAT_ANTIVIRUS_HOOKS
#include <linux/fshook.h>
#endif

#include <linux/ltt-events.h>

#include <asm/uaccess.h>
#include <asm/mmu_context.h>

#ifdef CONFIG_KMOD
#include <linux/kmod.h>
#endif

#ifdef CONFIG_APLOGGER
#include <linux/aplogger_hook.h>
#endif

#ifdef CONFIG_MOT_FEAT_APP_DUMP
#include <linux/ks_otp_api.h>
#ifdef CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY
#include <linux/motfb.h>
#endif /* CNFIG_MOT_FEAT_APP_COREDUMP_DISPLAY */
#endif /* CONFIG_MOT_FEAT_APP_DUMP */

#ifdef CONFIG_MOT_FEAT_BOOTINFO
#include <asm/bootinfo.h>
#endif /* CONFIG_MOT_FEAT_BOOTINFO */

#ifdef CONFIG_MOT_FEAT_DRM_COREDUMP
extern int mot_security_coredump(void);
#endif /* CONFIG_MOT_FEAT_DRM_COREDUMP */

int core_uses_pid;
char core_pattern[65] = "core";
/* The maximal length of core_pattern is also specified in sysctl.c */

static struct linux_binfmt *formats;
static DEFINE_RWLOCK(binfmt_lock);

#ifdef CONFIG_MOT_FEAT_APP_DUMP
volatile int (*aplog_do_coredump)(long, struct pt_regs *, int, int) = NULL;
DECLARE_RWSEM(aplog_do_coredump_lock);
EXPORT_SYMBOL(aplog_do_coredump);
EXPORT_SYMBOL(aplog_do_coredump_lock);
#ifdef CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY 
extern int fb_panic_text(struct fb_info * fbi, const char * panic_text,
                         long panic_len, int do_timer);
#endif /* CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY */
#endif /* CONFIG_MOT_FEAT_APP_DUMP */

int register_binfmt(struct linux_binfmt * fmt)
{
	struct linux_binfmt ** tmp = &formats;

	if (!fmt)
		return -EINVAL;
	if (fmt->next)
		return -EBUSY;
	write_lock(&binfmt_lock);
	while (*tmp) {
		if (fmt == *tmp) {
			write_unlock(&binfmt_lock);
			return -EBUSY;
		}
		tmp = &(*tmp)->next;
	}
	fmt->next = formats;
	formats = fmt;
	write_unlock(&binfmt_lock);
	return 0;	
}

EXPORT_SYMBOL(register_binfmt);

int unregister_binfmt(struct linux_binfmt * fmt)
{
	struct linux_binfmt ** tmp = &formats;

	write_lock(&binfmt_lock);
	while (*tmp) {
		if (fmt == *tmp) {
			*tmp = fmt->next;
			write_unlock(&binfmt_lock);
			return 0;
		}
		tmp = &(*tmp)->next;
	}
	write_unlock(&binfmt_lock);
	return -EINVAL;
}

EXPORT_SYMBOL(unregister_binfmt);

static inline void put_binfmt(struct linux_binfmt * fmt)
{
	module_put(fmt->module);
}

/*
 * Note that a shared library must be both readable and executable due to
 * security reasons.
 *
 * Also note that we take the address to load from from the file itself.
 */
asmlinkage long sys_uselib(const char __user * library)
{
	struct file * file;
	struct nameidata nd;
	int error;

	nd.intent.open.flags = FMODE_READ;
	error = __user_walk(library, LOOKUP_FOLLOW|LOOKUP_OPEN, &nd);
	if (error)
		goto out;

	error = -EINVAL;
	if (!S_ISREG(nd.dentry->d_inode->i_mode))
		goto exit;

	error = permission(nd.dentry->d_inode, MAY_READ | MAY_EXEC, &nd);
	if (error)
		goto exit;

	file = dentry_open(nd.dentry, nd.mnt, O_RDONLY);
	error = PTR_ERR(file);
	if (IS_ERR(file))
		goto out;

	error = -ENOEXEC;
	if(file->f_op) {
		struct linux_binfmt * fmt;

		read_lock(&binfmt_lock);
		for (fmt = formats ; fmt ; fmt = fmt->next) {
			if (!fmt->load_shlib)
				continue;
			if (!try_module_get(fmt->module))
				continue;
			read_unlock(&binfmt_lock);
			error = fmt->load_shlib(file);
			read_lock(&binfmt_lock);
			put_binfmt(fmt);
			if (error != -ENOEXEC)
				break;
		}
		read_unlock(&binfmt_lock);
	}
	fput(file);
out:
  	return error;
exit:
	path_release(&nd);
	goto out;
}

/*
 * count() counts the number of strings in array ARGV.
 */
static int count(char __user * __user * argv, int max)
{
	int i = 0;

	if (argv != NULL) {
		for (;;) {
			char __user * p;

			if (get_user(p, argv))
				return -EFAULT;
			if (!p)
				break;
			argv++;
			if(++i > max)
				return -E2BIG;
			cond_resched();
		}
	}
	return i;
}

/*
 * 'copy_strings()' copies argument/environment strings from user
 * memory to free pages in kernel mem. These are in a format ready
 * to be put directly into the top of new user memory.
 */
int copy_strings(int argc,char __user * __user * argv, struct linux_binprm *bprm)
{
	struct page *kmapped_page = NULL;
	char *kaddr = NULL;
	int ret;

	while (argc-- > 0) {
		char __user *str;
		int len;
		unsigned long pos;

		if (get_user(str, argv+argc) ||
				!(len = strnlen_user(str, bprm->p))) {
			ret = -EFAULT;
			goto out;
		}

		if (bprm->p < len)  {
			ret = -E2BIG;
			goto out;
		}

		bprm->p -= len;
		/* XXX: add architecture specific overflow check here. */
		pos = bprm->p;

		while (len > 0) {
			int i, new, err;
			int offset, bytes_to_copy;
			struct page *page;

			offset = pos % PAGE_SIZE;
			i = pos/PAGE_SIZE;
			page = bprm->page[i];
			new = 0;
			if (!page) {
				page = alloc_page(GFP_HIGHUSER);
				bprm->page[i] = page;
				if (!page) {
					ret = -ENOMEM;
					goto out;
				}
				new = 1;
			}

			if (page != kmapped_page) {
				if (kmapped_page)
					kunmap(kmapped_page);
				kmapped_page = page;
				kaddr = kmap(kmapped_page);
			}
			if (new && offset)
				memset(kaddr, 0, offset);
			bytes_to_copy = PAGE_SIZE - offset;
			if (bytes_to_copy > len) {
				bytes_to_copy = len;
				if (new)
					memset(kaddr+offset+len, 0,
						PAGE_SIZE-offset-len);
			}
			err = copy_from_user(kaddr+offset, str, bytes_to_copy);
			if (err) {
				ret = -EFAULT;
				goto out;
			}

			pos += bytes_to_copy;
			str += bytes_to_copy;
			len -= bytes_to_copy;
		}
	}
	ret = 0;
out:
	if (kmapped_page)
		kunmap(kmapped_page);
	return ret;
}

/*
 * Like copy_strings, but get argv and its values from kernel memory.
 */
int copy_strings_kernel(int argc,char ** argv, struct linux_binprm *bprm)
{
	int r;
	mm_segment_t oldfs = get_fs();
	set_fs(KERNEL_DS);
	r = copy_strings(argc, (char __user * __user *)argv, bprm);
	set_fs(oldfs);
	return r;
}

EXPORT_SYMBOL(copy_strings_kernel);

#ifdef CONFIG_MMU
/*
 * This routine is used to map in a page into an address space: needed by
 * execve() for the initial stack and environment pages.
 *
 * vma->vm_mm->mmap_sem is held for writing.
 */
void install_arg_page(struct vm_area_struct *vma,
			struct page *page, unsigned long address)
{
	struct mm_struct *mm = vma->vm_mm;
	pgd_t * pgd;
	pmd_t * pmd;
	pte_t * pte;

	if (unlikely(anon_vma_prepare(vma)))
		goto out_sig;

	flush_dcache_page(page);
	pgd = pgd_offset(mm, address);

	spin_lock(&mm->page_table_lock);
	pmd = pmd_alloc(mm, pgd, address);
	if (!pmd)
		goto out;
	pte = pte_alloc_map(mm, pmd, address);
	if (!pte)
		goto out;
	if (!pte_none(*pte)) {
		pte_unmap(pte);
		goto out;
	}
	mm->rss++;
	lru_cache_add_active(page);
	set_pte(pte, pte_mkdirty(pte_mkwrite(mk_pte(
					page, vma->vm_page_prot))));
	page_add_anon_rmap(page, vma, address);
	pte_unmap(pte);
	spin_unlock(&mm->page_table_lock);

	/* no need for flush_tlb */
	return;
out:
	spin_unlock(&mm->page_table_lock);
out_sig:
	__free_page(page);
	force_sig(SIGKILL, current);
}

#define EXTRA_STACK_VM_PAGES	20	/* random */

int setup_arg_pages(struct linux_binprm *bprm, int executable_stack)
{
	unsigned long stack_base;
	struct vm_area_struct *mpnt;
	struct mm_struct *mm = current->mm;
	int i, ret;
	long arg_size;

#ifdef CONFIG_STACK_GROWSUP
	/* Move the argument and environment strings to the bottom of the
	 * stack space.
	 */
	int offset, j;
	char *to, *from;

	/* Start by shifting all the pages down */
	i = 0;
	for (j = 0; j < MAX_ARG_PAGES; j++) {
		struct page *page = bprm->page[j];
		if (!page)
			continue;
		bprm->page[i++] = page;
	}

	/* Now move them within their pages */
	offset = bprm->p % PAGE_SIZE;
	to = kmap(bprm->page[0]);
	for (j = 1; j < i; j++) {
		memmove(to, to + offset, PAGE_SIZE - offset);
		from = kmap(bprm->page[j]);
		memcpy(to + PAGE_SIZE - offset, from, offset);
		kunmap(bprm->page[j - 1]);
		to = from;
	}
	memmove(to, to + offset, PAGE_SIZE - offset);
	kunmap(bprm->page[j - 1]);

	/* Limit stack size to 1GB */
	stack_base = current->signal->rlim[RLIMIT_STACK].rlim_max;
	if (stack_base > (1 << 30))
		stack_base = 1 << 30;
	stack_base = PAGE_ALIGN(STACK_TOP - stack_base);

	/* Adjust bprm->p to point to the end of the strings. */
	bprm->p = stack_base + PAGE_SIZE * i - offset;

	mm->arg_start = stack_base;
	arg_size = i << PAGE_SHIFT;

	/* zero pages that were copied above */
	while (i < MAX_ARG_PAGES)
		bprm->page[i++] = NULL;
#else
	stack_base = STACK_TOP - MAX_ARG_PAGES * PAGE_SIZE;
	bprm->p += stack_base;
	mm->arg_start = bprm->p;
	arg_size = STACK_TOP - (PAGE_MASK & (unsigned long) mm->arg_start);
#endif

	arg_size += EXTRA_STACK_VM_PAGES * PAGE_SIZE;

	if (bprm->loader)
		bprm->loader += stack_base;
	bprm->exec += stack_base;

	mpnt = kmem_cache_alloc(vm_area_cachep, SLAB_KERNEL);
	if (!mpnt)
		return -ENOMEM;

	if (security_vm_enough_memory(arg_size >> PAGE_SHIFT)) {
		kmem_cache_free(vm_area_cachep, mpnt);
		return -ENOMEM;
	}

	memset(mpnt, 0, sizeof(*mpnt));

	down_write(&mm->mmap_sem);
	{
		mpnt->vm_mm = mm;
#ifdef CONFIG_STACK_GROWSUP
		mpnt->vm_start = stack_base;
		mpnt->vm_end = stack_base + arg_size;
#else
		mpnt->vm_end = STACK_TOP;
		mpnt->vm_start = mpnt->vm_end - arg_size;
#endif
		/* Adjust stack execute permissions; explicitly enable
		 * for EXSTACK_ENABLE_X, disable for EXSTACK_DISABLE_X
		 * and leave alone (arch default) otherwise. */
		if (unlikely(executable_stack == EXSTACK_ENABLE_X))
			mpnt->vm_flags = VM_STACK_FLAGS |  VM_EXEC;
		else if (executable_stack == EXSTACK_DISABLE_X)
			mpnt->vm_flags = VM_STACK_FLAGS & ~VM_EXEC;
		else
			mpnt->vm_flags = VM_STACK_FLAGS;
		mpnt->vm_flags |= mm->def_flags;
		mpnt->vm_page_prot = protection_map[mpnt->vm_flags & 0x7];
		if ((ret = insert_vm_struct(mm, mpnt))) {
			up_write(&mm->mmap_sem);
			kmem_cache_free(vm_area_cachep, mpnt);
			return ret;
		}
		mm->stack_vm = mm->total_vm = vma_pages(mpnt);
	}

	for (i = 0 ; i < MAX_ARG_PAGES ; i++) {
		struct page *page = bprm->page[i];
		if (page) {
			bprm->page[i] = NULL;
			install_arg_page(mpnt, page, stack_base);
		}
		stack_base += PAGE_SIZE;
	}
	up_write(&mm->mmap_sem);
	
	return 0;
}

EXPORT_SYMBOL(setup_arg_pages);

#define free_arg_pages(bprm) do { } while (0)

#else

static inline void free_arg_pages(struct linux_binprm *bprm)
{
	int i;

	for (i = 0; i < MAX_ARG_PAGES; i++) {
		if (bprm->page[i])
			__free_page(bprm->page[i]);
		bprm->page[i] = NULL;
	}
}

#endif /* CONFIG_MMU */

struct file *open_exec(const char *name)
{
	struct nameidata nd;
	int err;
	struct file *file;

	nd.intent.open.flags = FMODE_READ;
	err = path_lookup(name, LOOKUP_FOLLOW|LOOKUP_OPEN, &nd);
	file = ERR_PTR(err);

	if (!err) {
		struct inode *inode = nd.dentry->d_inode;
		file = ERR_PTR(-EACCES);
		if (!(nd.mnt->mnt_flags & MNT_NOEXEC) &&
		    S_ISREG(inode->i_mode)) {
			int err = permission(inode, MAY_EXEC, &nd);
			if (!err && !(inode->i_mode & 0111))
				err = -EACCES;
			file = ERR_PTR(err);
			if (!err) {
				file = dentry_open(nd.dentry, nd.mnt, O_RDONLY);
				if (!IS_ERR(file)) {
					err = deny_write_access(file);
					if (err) {
						fput(file);
						file = ERR_PTR(err);
					}
				}
out:
				return file;
			}
		}
		path_release(&nd);
	}
	goto out;
}

EXPORT_SYMBOL(open_exec);

int kernel_read(struct file *file, unsigned long offset,
	char *addr, unsigned long count)
{
	mm_segment_t old_fs;
	loff_t pos = offset;
	int result;

	old_fs = get_fs();
	set_fs(get_ds());
	/* The cast to a user pointer is valid due to the set_fs() */
	result = vfs_read(file, (void __user *)addr, count, &pos);
	set_fs(old_fs);
	return result;
}

EXPORT_SYMBOL(kernel_read);

static int exec_mmap(struct mm_struct *mm)
{
	struct task_struct *tsk;
	struct mm_struct * old_mm, *active_mm;

	/* Notify parent that we're no longer interested in the old VM */
	tsk = current;
	old_mm = current->mm;
	mm_release(tsk, old_mm);

	if (old_mm) {
		/*
		 * Make sure that if there is a core dump in progress
		 * for the old mm, we get out and die instead of going
		 * through with the exec.  We must hold mmap_sem around
		 * checking core_waiters and changing tsk->mm.  The
		 * core-inducing thread will increment core_waiters for
		 * each thread whose ->mm == old_mm.
		 */
		down_read(&old_mm->mmap_sem);
		if (unlikely(old_mm->core_waiters)) {
			up_read(&old_mm->mmap_sem);
			return -EINTR;
		}
	}
	task_lock(tsk);

	local_irq_disable(); // FIXME
	active_mm = tsk->active_mm;
	activate_mm(active_mm, mm);
	tsk->mm = mm;
	tsk->active_mm = mm;
	local_irq_enable();

	task_unlock(tsk);

	arch_pick_mmap_layout(mm);
	if (old_mm) {
		up_read(&old_mm->mmap_sem);
		if (active_mm != old_mm) BUG();
		mmput(old_mm);
		return 0;
	}
	mmdrop(active_mm);
	return 0;
}

/*
 * This function makes sure the current process has its own signal table,
 * so that flush_signal_handlers can later reset the handlers without
 * disturbing other processes.  (Other processes might share the signal
 * table via the CLONE_SIGHAND option to clone().)
 */
static inline int de_thread(struct task_struct *tsk)
{
	struct signal_struct *sig = tsk->signal;
	struct sighand_struct *newsighand, *oldsighand = tsk->sighand;
	spinlock_t *lock = &oldsighand->siglock;
	int count;

	/*
	 * If we don't share sighandlers, then we aren't sharing anything
	 * and we can just re-use it all.
	 */
	if (atomic_read(&oldsighand->count) <= 1) {
		BUG_ON(atomic_read(&sig->count) != 1);
		exit_itimers(sig);
		return 0;
	}

	newsighand = kmem_cache_alloc(sighand_cachep, GFP_KERNEL);
	if (!newsighand)
		return -ENOMEM;

	if (thread_group_empty(current))
		goto no_thread_group;

	/*
	 * Kill all other threads in the thread group.
	 * We must hold tasklist_lock to call zap_other_threads.
	 */
	read_lock(&tasklist_lock);
	spin_lock_irq(lock);
	if (sig->group_exit) {
		/*
		 * Another group action in progress, just
		 * return so that the signal is processed.
		 */
		spin_unlock_irq(lock);
		read_unlock(&tasklist_lock);
		kmem_cache_free(sighand_cachep, newsighand);
		return -EAGAIN;
	}
	sig->group_exit = 1;
	zap_other_threads(current);
	read_unlock(&tasklist_lock);

	/*
	 * Account for the thread group leader hanging around:
	 */
	count = 2;
	if (thread_group_leader(current))
		count = 1;
	while (atomic_read(&sig->count) > count) {
		sig->group_exit_task = current;
		sig->notify_count = count;
		__set_current_state(TASK_UNINTERRUPTIBLE);
		spin_unlock_irq(lock);
		schedule();
		spin_lock_irq(lock);
	}
	sig->group_exit_task = NULL;
	sig->notify_count = 0;
	spin_unlock_irq(lock);

	/*
	 * At this point all other threads have exited, all we have to
	 * do is to wait for the thread group leader to become inactive,
	 * and to assume its PID:
	 */
	if (!thread_group_leader(current)) {
		struct task_struct *leader = current->group_leader, *parent;
		struct dentry *proc_dentry1, *proc_dentry2;
		unsigned long exit_state, ptrace;

		/*
		 * Wait for the thread group leader to be a zombie.
		 * It should already be zombie at this point, most
		 * of the time.
		 */
		while (leader->exit_state != EXIT_ZOMBIE)
			msleep(1);

		spin_lock(&leader->proc_lock);
		spin_lock(&current->proc_lock);
		proc_dentry1 = proc_pid_unhash(current);
		proc_dentry2 = proc_pid_unhash(leader);
		write_lock_irq(&tasklist_lock);

		if (leader->tgid != current->tgid)
			BUG();
		if (current->pid == current->tgid)
			BUG();
		/*
		 * An exec() starts a new thread group with the
		 * TGID of the previous thread group. Rehash the
		 * two threads with a switched PID, and release
		 * the former thread group leader:
		 */
		ptrace = leader->ptrace;
		parent = leader->parent;
		if (unlikely(ptrace) && unlikely(parent == current)) {
			/*
			 * Joker was ptracing his own group leader,
			 * and now he wants to be his own parent!
			 * We can't have that.
			 */
			ptrace = 0;
		}

		ptrace_unlink(current);
		ptrace_unlink(leader);
		remove_parent(current);
		remove_parent(leader);

		switch_exec_pids(leader, current);

		current->parent = current->real_parent = leader->real_parent;
		leader->parent = leader->real_parent = child_reaper;
		current->group_leader = current;
		leader->group_leader = leader;

		add_parent(current, current->parent);
		add_parent(leader, leader->parent);
		if (ptrace) {
			current->ptrace = ptrace;
			__ptrace_link(current, parent);
		}

		list_del(&current->tasks);
		list_add_tail(&current->tasks, &init_task.tasks);
		current->exit_signal = SIGCHLD;
		exit_state = leader->exit_state;

		write_unlock_irq(&tasklist_lock);
		spin_unlock(&leader->proc_lock);
		spin_unlock(&current->proc_lock);
		proc_pid_flush(proc_dentry1);
		proc_pid_flush(proc_dentry2);

		if (exit_state != EXIT_ZOMBIE)
			BUG();
		release_task(leader);
        }

	/*
	 * Now there are really no other threads at all,
	 * so it's safe to stop telling them to kill themselves.
	 */
	sig->group_exit = 0;

no_thread_group:
	BUG_ON(atomic_read(&sig->count) != 1);
	exit_itimers(sig);

	if (atomic_read(&oldsighand->count) == 1) {
		/*
		 * Now that we nuked the rest of the thread group,
		 * it turns out we are not sharing sighand any more either.
		 * So we can just keep it.
		 */
		kmem_cache_free(sighand_cachep, newsighand);
	} else {
		/*
		 * Move our state over to newsighand and switch it in.
		 */
		spin_lock_init(&newsighand->siglock);
		atomic_set(&newsighand->count, 1);
		memcpy(newsighand->action, oldsighand->action,
		       sizeof(newsighand->action));

		write_lock_irq(&tasklist_lock);
		spin_lock(&oldsighand->siglock);
		spin_lock(&newsighand->siglock);

		current->sighand = newsighand;
		recalc_sigpending();

		spin_unlock(&newsighand->siglock);
		spin_unlock(&oldsighand->siglock);
		write_unlock_irq(&tasklist_lock);

		if (atomic_dec_and_test(&oldsighand->count))
			kmem_cache_free(sighand_cachep, oldsighand);
	}

	if (!thread_group_empty(current))
		BUG();
	if (!thread_group_leader(current))
		BUG();
	return 0;
}
	
/*
 * These functions flushes out all traces of the currently running executable
 * so that a new one can be started
 */

static inline void flush_old_files(struct files_struct * files)
{
	long j = -1;

	spin_lock(&files->file_lock);
	for (;;) {
		unsigned long set, i;

		j++;
		i = j * __NFDBITS;
		if (i >= files->max_fds || i >= files->max_fdset)
			break;
		set = files->close_on_exec->fds_bits[j];
		if (!set)
			continue;
		files->close_on_exec->fds_bits[j] = 0;
		spin_unlock(&files->file_lock);
		for ( ; set ; i++,set >>= 1) {
			if (set & 1) {
				sys_close(i);
			}
		}
		spin_lock(&files->file_lock);

	}
	spin_unlock(&files->file_lock);
}

void get_task_comm(char *buf, struct task_struct *tsk)
{
	/* buf must be at least sizeof(tsk->comm) in size */
	task_lock(tsk);
	memcpy(buf, tsk->comm, sizeof(tsk->comm));
	task_unlock(tsk);
}

void set_task_comm(struct task_struct *tsk, char *buf)
{
	task_lock(tsk);
	strlcpy(tsk->comm, buf, sizeof(tsk->comm));
	task_unlock(tsk);
}

int flush_old_exec(struct linux_binprm * bprm)
{
	char * name;
	int i, ch, retval;
	struct files_struct *files;
	char tcomm[sizeof(current->comm)];

	/*
	 * Make sure we have a private signal table and that
	 * we are unassociated from the previous thread group.
	 */
	retval = de_thread(current);
	if (retval)
		goto out;

	/*
	 * Make sure we have private file handles. Ask the
	 * fork helper to do the work for us and the exit
	 * helper to do the cleanup of the old one.
	 */
	files = current->files;		/* refcounted so safe to hold */
	retval = unshare_files();
	if (retval)
		goto out;
	/*
	 * Release all of the old mmap stuff
	 */
	retval = exec_mmap(bprm->mm);
	if (retval)
		goto mmap_failed;

	bprm->mm = NULL;		/* We're using it now */

	/* This is the point of no return */
	steal_locks(files);
	put_files_struct(files);

	current->sas_ss_sp = current->sas_ss_size = 0;

	if (current->euid == current->uid && current->egid == current->gid)
		current->mm->dumpable = 1;
	name = bprm->filename;
	for (i=0; (ch = *(name++)) != '\0';) {
		if (ch == '/')
			i = 0;
		else
			if (i < (sizeof(tcomm) - 1))
				tcomm[i++] = ch;
	}
	tcomm[i] = '\0';
	set_task_comm(current, tcomm);

	flush_thread();

	if (bprm->e_uid != current->euid || bprm->e_gid != current->egid || 
	    permission(bprm->file->f_dentry->d_inode,MAY_READ, NULL) ||
	    (bprm->interp_flags & BINPRM_FLAGS_ENFORCE_NONDUMP)) {
		suid_keys(current);
		current->mm->dumpable = 0;
	}

	/* An exec changes our domain. We are no longer part of the thread
	   group */

	current->self_exec_id++;
			
	flush_signal_handlers(current, 0);
	flush_old_files(current->files);

	return 0;

mmap_failed:
	put_files_struct(current->files);
	current->files = files;
out:
	return retval;
}

EXPORT_SYMBOL(flush_old_exec);

/* 
 * Fill the binprm structure from the inode. 
 * Check permissions, then read the first 128 (BINPRM_BUF_SIZE) bytes
 */
int prepare_binprm(struct linux_binprm *bprm)
{
	int mode;
	struct inode * inode = bprm->file->f_dentry->d_inode;
	int retval;

	mode = inode->i_mode;
	/*
	 * Check execute perms again - if the caller has CAP_DAC_OVERRIDE,
	 * generic_permission lets a non-executable through
	 */
	if (!(mode & 0111))	/* with at least _one_ execute bit set */
		return -EACCES;
	if (bprm->file->f_op == NULL)
		return -EACCES;

	bprm->e_uid = current->euid;
	bprm->e_gid = current->egid;

	if(!(bprm->file->f_vfsmnt->mnt_flags & MNT_NOSUID)) {
		/* Set-uid? */
		if (mode & S_ISUID) {
			current->personality &= ~PER_CLEAR_ON_SETID;
			bprm->e_uid = inode->i_uid;
		}

		/* Set-gid? */
		/*
		 * If setgid is set but no group execute bit then this
		 * is a candidate for mandatory locking, not a setgid
		 * executable.
		 */
		if ((mode & (S_ISGID | S_IXGRP)) == (S_ISGID | S_IXGRP)) {
			current->personality &= ~PER_CLEAR_ON_SETID;
			bprm->e_gid = inode->i_gid;
		}
	}

	/* fill in binprm security blob */
	retval = security_bprm_set(bprm);
	if (retval)
		return retval;

	memset(bprm->buf,0,BINPRM_BUF_SIZE);
	return kernel_read(bprm->file,0,bprm->buf,BINPRM_BUF_SIZE);
}

EXPORT_SYMBOL(prepare_binprm);

static inline int unsafe_exec(struct task_struct *p)
{
	int unsafe = 0;
	if (p->ptrace & PT_PTRACED) {
		if (p->ptrace & PT_PTRACE_CAP)
			unsafe |= LSM_UNSAFE_PTRACE_CAP;
		else
			unsafe |= LSM_UNSAFE_PTRACE;
	}
	if (atomic_read(&p->fs->count) > 1 ||
	    atomic_read(&p->files->count) > 1 ||
	    atomic_read(&p->sighand->count) > 1)
		unsafe |= LSM_UNSAFE_SHARE;

	return unsafe;
}

void compute_creds(struct linux_binprm *bprm)
{
	int unsafe;

	if (bprm->e_uid != current->uid)
		suid_keys(current);
	exec_keys(current);

	task_lock(current);
	unsafe = unsafe_exec(current);
	security_bprm_apply_creds(bprm, unsafe);
	task_unlock(current);
}

EXPORT_SYMBOL(compute_creds);

void remove_arg_zero(struct linux_binprm *bprm)
{
	if (bprm->argc) {
		unsigned long offset;
		char * kaddr;
		struct page *page;

		offset = bprm->p % PAGE_SIZE;
		goto inside;

		while (bprm->p++, *(kaddr+offset++)) {
			if (offset != PAGE_SIZE)
				continue;
			offset = 0;
			kunmap_atomic(kaddr, KM_USER0);
inside:
			page = bprm->page[bprm->p/PAGE_SIZE];
			kaddr = kmap_atomic(page, KM_USER0);
		}
		kunmap_atomic(kaddr, KM_USER0);
		bprm->argc--;
	}
}

EXPORT_SYMBOL(remove_arg_zero);

/*
 * cycle the list of binary formats handler, until one recognizes the image
 */
int search_binary_handler(struct linux_binprm *bprm,struct pt_regs *regs)
{
	int try,retval;
	struct linux_binfmt *fmt;
#ifdef __alpha__
	/* handle /sbin/loader.. */
	{
	    struct exec * eh = (struct exec *) bprm->buf;

	    if (!bprm->loader && eh->fh.f_magic == 0x183 &&
		(eh->fh.f_flags & 0x3000) == 0x3000)
	    {
		struct file * file;
		unsigned long loader;

		allow_write_access(bprm->file);
		fput(bprm->file);
		bprm->file = NULL;

	        loader = PAGE_SIZE*MAX_ARG_PAGES-sizeof(void *);

		file = open_exec("/sbin/loader");
		retval = PTR_ERR(file);
		if (IS_ERR(file))
			return retval;

		/* Remember if the application is TASO.  */
		bprm->sh_bang = eh->ah.entry < 0x100000000UL;

		bprm->file = file;
		bprm->loader = loader;
		retval = prepare_binprm(bprm);
		if (retval<0)
			return retval;
		/* should call search_binary_handler recursively here,
		   but it does not matter */
	    }
	}
#endif
	retval = security_bprm_check(bprm);
	if (retval)
		return retval;

	/* kernel module loader fixup */
	/* so we don't try to load run modprobe in kernel space. */
	set_fs(USER_DS);
	retval = -ENOENT;
	for (try=0; try<2; try++) {
		read_lock(&binfmt_lock);
		for (fmt = formats ; fmt ; fmt = fmt->next) {
			int (*fn)(struct linux_binprm *, struct pt_regs *) = fmt->load_binary;
			if (!fn)
				continue;
			if (!try_module_get(fmt->module))
				continue;
			read_unlock(&binfmt_lock);
			retval = fn(bprm, regs);
			if (retval >= 0) {
				put_binfmt(fmt);
				allow_write_access(bprm->file);
				if (bprm->file)
					fput(bprm->file);
				bprm->file = NULL;
				current->did_exec = 1;
				return retval;
			}
			read_lock(&binfmt_lock);
			put_binfmt(fmt);
			if (retval != -ENOEXEC || bprm->mm == NULL)
				break;
			if (!bprm->file) {
				read_unlock(&binfmt_lock);
				return retval;
			}
		}
		read_unlock(&binfmt_lock);
		if (retval != -ENOEXEC || bprm->mm == NULL) {
			break;
#ifdef CONFIG_KMOD
		}else{
#define printable(c) (((c)=='\t') || ((c)=='\n') || (0x20<=(c) && (c)<=0x7e))
			if (printable(bprm->buf[0]) &&
			    printable(bprm->buf[1]) &&
			    printable(bprm->buf[2]) &&
			    printable(bprm->buf[3]))
				break; /* -ENOEXEC */
			request_module("binfmt-%04x", *(unsigned short *)(&bprm->buf[2]));
#endif
		}
	}
	return retval;
}

EXPORT_SYMBOL(search_binary_handler);

/*
 * sys_execve() executes a new program.
 */
int do_execve(char * filename,
	char __user *__user *argv,
	char __user *__user *envp,
	struct pt_regs * regs)
{
	struct linux_binprm *bprm;
	struct file *file;
	int retval;
	int i;

	retval = -ENOMEM;
	bprm = kmalloc(sizeof(*bprm), GFP_KERNEL);
	if (!bprm)
		goto out_ret;
	memset(bprm, 0, sizeof(*bprm));

#ifdef CONFIG_MOT_FEAT_ANTIVIRUS_HOOKS
	if ((retval = fsh_check_file(filename, 1, O_RDONLY)) < 0)
	        goto out_kfree;	
#endif

	file = open_exec(filename);
	retval = PTR_ERR(file);
	if (IS_ERR(file))
		goto out_kfree;
	
#ifdef CONFIG_APLOGGER
    if (logger_exec_log_ptr != NULL)
    {
        down_read(&logger_exec_ptr_lock);
        if (logger_exec_log_ptr != NULL)
            logger_exec_log_ptr(filename, argv, envp);
        up_read(&logger_exec_ptr_lock);
    }
#endif

	ltt_ev_file_system(LTT_EV_FILE_SYSTEM_EXEC,
			   0,
			   file->f_dentry->d_name.len,
			   file->f_dentry->d_name.name);

	sched_exec();

	bprm->p = PAGE_SIZE*MAX_ARG_PAGES-sizeof(void *);

	bprm->file = file;
	bprm->filename = filename;
	bprm->interp = filename;
	bprm->mm = mm_alloc();
	retval = -ENOMEM;
	if (!bprm->mm)
		goto out_file;

	retval = init_new_context(current, bprm->mm);
	if (retval < 0)
		goto out_mm;

	bprm->argc = count(argv, bprm->p / sizeof(void *));
	if ((retval = bprm->argc) < 0)
		goto out_mm;

	bprm->envc = count(envp, bprm->p / sizeof(void *));
	if ((retval = bprm->envc) < 0)
		goto out_mm;

	retval = security_bprm_alloc(bprm);
	if (retval)
		goto out;

	retval = prepare_binprm(bprm);
	if (retval < 0)
		goto out;

	retval = copy_strings_kernel(1, &bprm->filename, bprm);
	if (retval < 0)
		goto out;

	bprm->exec = bprm->p;
	retval = copy_strings(bprm->envc, envp, bprm);
	if (retval < 0)
		goto out;

	retval = copy_strings(bprm->argc, argv, bprm);
	if (retval < 0)
		goto out;

	retval = search_binary_handler(bprm,regs);
	if (retval >= 0) {
		free_arg_pages(bprm);

		/* execve success */
		security_bprm_free(bprm);
		kfree(bprm);
		return retval;
	}

out:
	/* Something went wrong, return the inode and free the argument pages*/
	for (i = 0 ; i < MAX_ARG_PAGES ; i++) {
		struct page * page = bprm->page[i];
		if (page)
			__free_page(page);
	}

	if (bprm->security)
		security_bprm_free(bprm);

#ifdef CONFIG_MOT_FEAT_SECURE_CLOCK
	/* If the execve() failed, make sure to lower CAP_SECURE_CLOCK 
	   from current's cap_permitted & cap_inheritable! */

	if (cap_raised(current->cap_permitted, CAP_SECURE_CLOCK)) 
	{
		cap_lower(current->cap_permitted, CAP_SECURE_CLOCK);
	}

	if (cap_raised(current->cap_inheritable, CAP_SECURE_CLOCK))
	{
		cap_lower(current->cap_inheritable, CAP_SECURE_CLOCK);
	}
#endif /* CONFIG_MOT_FEAT_SECURE_CLOCK */
#ifdef CONFIG_MOT_FEAT_SECURE_DRM
	if (unlikely(current->security != 0))
	{
		current->security = 0;
	}
#endif /* CONFIG_MOT_FEAT_SECURE_DRM */

out_mm:
	if (bprm->mm)
		mmdrop(bprm->mm);

out_file:
	if (bprm->file) {
		allow_write_access(bprm->file);
		fput(bprm->file);
	}

out_kfree:
	kfree(bprm);

out_ret:
	return retval;
}

int set_binfmt(struct linux_binfmt *new)
{
	struct linux_binfmt *old = current->binfmt;

	if (new) {
		if (!try_module_get(new->module))
			return -1;
	}
	current->binfmt = new;
	if (old)
		module_put(old->module);
	return 0;
}

EXPORT_SYMBOL(set_binfmt);

#ifdef CONFIG_MOT_FEAT_BOOTINFO
#define CORENAME_MAX_SIZE 128
#else
#define CORENAME_MAX_SIZE 64
#endif
/* format_corename will inspect the pattern parameter, and output a
 * name into corename, which must have space for at least
 * CORENAME_MAX_SIZE bytes plus one byte for the zero terminator.
 */
static void format_corename(char *corename, const char *pattern, long signr)
{
	const char *pat_ptr = pattern;
	char *out_ptr = corename;
	char *const out_end = corename + CORENAME_MAX_SIZE;
	int rc;
	int pid_in_pattern = 0;
#ifdef CONFIG_MOT_FEAT_BOOTINFO
	char *moto_bldlabel = NULL;
#endif

	/* Repeat as long as we have more pattern to process and more output
	   space */
	while (*pat_ptr) {
		if (*pat_ptr != '%') {
			if (out_ptr == out_end)
				goto out;
			*out_ptr++ = *pat_ptr++;
		} else {
			switch (*++pat_ptr) {
			case 0:
				goto out;
			/* Double percent, output one percent */
			case '%':
				if (out_ptr == out_end)
					goto out;
				*out_ptr++ = '%';
				break;
			/* pid */
			case 'p':
				pid_in_pattern = 1;
				rc = snprintf(out_ptr, out_end - out_ptr,
					      "%d", current->tgid);
				if (rc > out_end - out_ptr)
					goto out;
				out_ptr += rc;
				break;
			/* uid */
			case 'u':
				rc = snprintf(out_ptr, out_end - out_ptr,
					      "%d", current->uid);
				if (rc > out_end - out_ptr)
					goto out;
				out_ptr += rc;
				break;
			/* gid */
			case 'g':
				rc = snprintf(out_ptr, out_end - out_ptr,
					      "%d", current->gid);
				if (rc > out_end - out_ptr)
					goto out;
				out_ptr += rc;
				break;
			/* signal that caused the coredump */
			case 's':
				rc = snprintf(out_ptr, out_end - out_ptr,
					      "%ld", signr);
				if (rc > out_end - out_ptr)
					goto out;
				out_ptr += rc;
				break;
			/* UNIX time of coredump */
			case 't': {
				struct timeval tv;
				do_gettimeofday(&tv);
				rc = snprintf(out_ptr, out_end - out_ptr,
					      "%lu", tv.tv_sec);
				if (rc > out_end - out_ptr)
					goto out;
				out_ptr += rc;
				break;
			}
			/* hostname */
			case 'h':
				down_read(&uts_sem);
				rc = snprintf(out_ptr, out_end - out_ptr,
					      "%s", system_utsname.nodename);
				up_read(&uts_sem);
				if (rc > out_end - out_ptr)
					goto out;
				out_ptr += rc;
				break;
			/* executable */
			case 'e':
				rc = snprintf(out_ptr, out_end - out_ptr,
					      "%s", current->comm);
				if (rc > out_end - out_ptr)
					goto out;
				out_ptr += rc;
				break;
#ifdef CONFIG_MOT_FEAT_BOOTINFO
			/* build label */
			case 'b':
				moto_bldlabel = (char *)mot_build_label();
				/* if build label is too long, use the tailed string */
				rc = strlen(moto_bldlabel) - (out_end - out_ptr);
				if(rc > 0)
				{
					rc = snprintf(out_ptr, out_end - out_ptr,
						      "%s", moto_bldlabel + rc + 1);
				}else{
					rc = snprintf(out_ptr, out_end - out_ptr,
						      "%s", moto_bldlabel);
				}
				out_ptr += rc;
				break;
#endif
			default:
				break;
			}
			++pat_ptr;
		}
	}
	/* Backward compatibility with core_uses_pid:
	 *
	 * If core_pattern does not include a %p (as is the default)
	 * and core_uses_pid is set, then .%pid will be appended to
	 * the filename */
	if (!pid_in_pattern
            && (core_uses_pid || atomic_read(&current->mm->mm_users) != 1)) {
		rc = snprintf(out_ptr, out_end - out_ptr,
			      ".%d", current->tgid);
		if (rc > out_end - out_ptr)
			goto out;
		out_ptr += rc;
	}
      out:
	*out_ptr = 0;
}

static void zap_threads (struct mm_struct *mm)
{
	struct task_struct *g, *p;
	struct task_struct *tsk = current;
	struct completion *vfork_done = tsk->vfork_done;
	int traced = 0;

	/*
	 * Make sure nobody is waiting for us to release the VM,
	 * otherwise we can deadlock when we wait on each other
	 */
	if (vfork_done) {
		tsk->vfork_done = NULL;
		complete(vfork_done);
	}

	read_lock(&tasklist_lock);
	do_each_thread(g,p)
		if (mm == p->mm && p != tsk) {
			force_sig_specific(SIGKILL, p);
			mm->core_waiters++;
			if (unlikely(p->ptrace) &&
			    unlikely(p->parent->mm == mm))
				traced = 1;
		}
	while_each_thread(g,p);

	read_unlock(&tasklist_lock);

	if (unlikely(traced)) {
		/*
		 * We are zapping a thread and the thread it ptraces.
		 * If the tracee went into a ptrace stop for exit tracing,
		 * we could deadlock since the tracer is waiting for this
		 * coredump to finish.  Detach them so they can both die.
		 */
		write_lock_irq(&tasklist_lock);
		do_each_thread(g,p) {
			if (mm == p->mm && p != tsk &&
			    p->ptrace && p->parent->mm == mm) {
				__ptrace_unlink(p);
			}
		} while_each_thread(g,p);
		write_unlock_irq(&tasklist_lock);
	}
}

static void coredump_wait(struct mm_struct *mm)
{
	DECLARE_COMPLETION(startup_done);

	mm->core_waiters++; /* let other threads block */
	mm->core_startup_done = &startup_done;

	zap_threads(mm);
	if (--mm->core_waiters) {
		up_write(&mm->mmap_sem);
		wait_for_completion(&startup_done);
	} else
		up_write(&mm->mmap_sem);
	BUG_ON(mm->core_waiters);
}

#ifdef CONFIG_MOT_FEAT_APP_DUMP
/*
 * Determines the current mode of the phone and returns
 * the level of coredump to be generated.
 *
 * Return Values:
 * 	0 == No Linux Coredump && No EZX Coredump
 * 	1 == No Linux Coredump && Limited EZX Coredump
 * 	2 == No Linux Coredump && Full EZX Coredump
 * 	3 == Full Linux Coredump && Full EZX Coredump
 */
static int get_core_dump_level()
{
	/* setting the default values to be most restrictive */
	unsigned char security_mode = MOT_SECURITY_MODE_PRODUCTION;
	unsigned char production_state = LAUNCH_SHIP;
	unsigned char bound_signature_state = BS_DIS_DISABLED;
	int core_dump_level = 0;

#ifdef CONFIG_MOT_FEAT_DRM_COREDUMP
	/* dump full core if not a DRM_READ privileged application */
	if(mot_security_coredump() == 0) {
		return 3;
	}
#endif /* CONFIG_MOT_FEAT_DRM_COREDUMP */

	/* dump the most restrictive version(s) of the core if reading any of the efuses returned an error */
	if ((mot_otp_get_mode(&security_mode, NULL) == -1) ||
	    (mot_otp_get_productstate(&production_state, NULL) == -1) ||
	    (mot_otp_get_boundsignaturestate(&bound_signature_state, NULL) == -1)) {
		printk(KERN_EMERG "Error: Could not read the OTP efuse bits. Production-Launched mode of the phone will be assumed.\n");
		return core_dump_level;
	}
	if ((security_mode == MOT_SECURITY_MODE_ENGINEERING) ||
	    (security_mode == MOT_SECURITY_MODE_NO_SECURITY)) {
		core_dump_level = 3;
	}
	else {
		if (production_state == PRE_ACCEPTANCE_ACCEPTANCE) {
			/* production DRM key is present */
			if (bound_signature_state == BS_DIS_DISABLED) {
				core_dump_level = 1;
			}
			/* no production DRM key is present */
			else {
				core_dump_level = 2;
			}
		}
	}
	return core_dump_level;

}

#define SD_MOUNT_PT     "/mmc/mmca1"
#define SD_DUMP_DIR     "/mmc/mmca1/app_dump"
#ifdef CONFIG_MOT_FEAT_BOOTINFO
#define SD_FULL_DUMP_PATTERN "/core.%e.%p.%t.%b"
#else
#define SD_FULL_DUMP_PATTERN "/core.%e.%p.%t"
#endif
#define SD_DEV_MAJOR    243
int sd_exist = 0;

/*
 * check_sd_card() - Check whether SD card is inserted, and mkdir "app_dump"
 * in SD card to save full coredump file.
 */
static int check_sd_card(void)
{
        struct nameidata nd;
        struct dentry *dentry;
        struct kstat dir_stat;
        mm_segment_t old_fs = get_fs();
        int ret = 0;

        /* test if SD card is properly mounted and ready to     be used*/
        if ((path_lookup(SD_MOUNT_PT, LOOKUP_FOLLOW, &nd) < 0) ||
                                (MAJOR(nd.mnt->mnt_sb->s_dev) != SD_DEV_MAJOR))         {
                    printk(KERN_WARNING"SD card not found, no full coredump.\n");
                    return -1;
        }
        path_release(&nd);

        set_fs(get_ds());

       /* creat the directories */
        /* check if the directory already exist */
        ret = vfs_stat(SD_DUMP_DIR, &dir_stat);
        if (ret != 0) {
                    printk("vfs_stat gives ret %i, -2 we create, -5 io error\n", ret);
                ret = path_lookup(SD_DUMP_DIR, LOOKUP_PARENT, &nd);
                if (ret)
                {
                        set_fs(old_fs);
                        return ret;
                }
                dentry = lookup_create(&nd, 1);

                if (!IS_POSIXACL(nd.dentry->d_inode))
                        ret = vfs_mkdir(nd.dentry->d_inode, dentry, 0777);

                dput(dentry);
                up(&nd.dentry->d_inode->i_sem);
                path_release(&nd);

                /* ret equal to -EEXIST when the directory already exist */
                if (ret != 0 && ret != -EEXIST) {
                        printk("sys_mkdir returns %i \n", ret);
                        set_fs(old_fs);
                        return ret;
                }
        }
        set_fs(old_fs);

        return 0;
}


struct aplog_coredump_cfg{
    unsigned int maxcorefilesize;
    unsigned int maxcorefilesperapp;
};
static struct aplog_coredump_cfg apcore = {10240, 4};

void (*get_aplog_coredump_cfg)(struct aplog_coredump_cfg * apcore) = NULL;
EXPORT_SYMBOL(get_aplog_coredump_cfg);
DECLARE_RWSEM(get_aplog_coredump_cfg_lock);
EXPORT_SYMBOL(get_aplog_coredump_cfg_lock);


/*
 * A function to call a user space script to do the synchronizing work between
 * MMC card and /ezxlocal for aplog core dump, and back up aplog and system info
 * for linux full core dump.
 * Will be called during do_coredump.
 * 
 */
static int call_post_dump_script(int dump_level, char * dump_name)
{
        static char * envp[] = {"HOME=/", "PATH=/usr/bin:/bin", NULL};        
        char * argv[] = {"/etc/log_coredump.sh", NULL, NULL, NULL, NULL, NULL};
        char level_str[2];
        char size_str[10];
        char file_str[4];

        snprintf(level_str, 2, "%d", dump_level);
        
        if (get_aplog_coredump_cfg != NULL) {
                down_read(&get_aplog_coredump_cfg_lock);
                (*get_aplog_coredump_cfg)(&apcore);
                up_read(&get_aplog_coredump_cfg_lock);
        }
        snprintf(size_str, 10, "%d", apcore.maxcorefilesize);
        snprintf(file_str, 4, "%d", apcore.maxcorefilesperapp);

        argv[1] = level_str;
        argv[2] = size_str;
        argv[3] = file_str;
        argv[4] = dump_name;
        
        return call_usermodehelper(argv[0], argv, envp, 0);
}

#endif /* CONFIG_MOT_FEAT_APP_DUMP */

int do_coredump(long signr, int exit_code, struct pt_regs * regs)
{
	char corename[CORENAME_MAX_SIZE + 1];
	struct mm_struct *mm = current->mm;
	struct linux_binfmt * binfmt;
	struct inode * inode;
	struct file * file;
	int retval = 0;
#ifdef CONFIG_MOT_FEAT_APP_DUMP
	int dump_level;
#ifdef CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY
	char fb_coredump_text_buf[PANIC_MAX_STR_LEN];
	const char core_text[] = " coredump";
#endif /* CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY */
#endif /* CONFIG_MOT_FEAT_APP_DUMP */

#ifdef CONFIG_MOT_FEAT_APP_DUMP
	printk(
		   KERN_WARNING "Process '%s' (pid: %d) was terminated with signal: %d.\n",
		   current->comm,
		   current->pid,
		   signr
		  );

	dump_level = get_core_dump_level();

        if( dump_level == 3 && check_sd_card() == 0){
                sd_exist = 1;
        }
    
	/* continue system coredump after aplog_do_coredump, if necessary */
	if (aplog_do_coredump != NULL) {
		down_read(&aplog_do_coredump_lock);
		if (aplog_do_coredump != NULL) {
			/*
			 * The reason why dump_level is decremented to 2 if it is equal to 3 is b/c for
			 * aplog_do_coredump() function, dump_level of 2 and 3 mean exactly the same
			 * thing. The return value is also purposely being ignored.
			 */
			aplog_do_coredump(signr, regs, (dump_level == 3 ? dump_level-1 : dump_level), sd_exist);
		}
		up_read(&aplog_do_coredump_lock);
	}
	/* skip the process of dumping the standard Linux coredump */
	if (dump_level < 3) {
		goto fail;
	}
                                                                                
#ifdef CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY
	strncpy(fb_coredump_text_buf, current->comm, sizeof(current->comm));
	strncat(fb_coredump_text_buf, core_text, 10);
        /* display coredump text to the screen */
        fb_panic_text(NULL, fb_coredump_text_buf,
                       strnlen(fb_coredump_text_buf, PANIC_MAX_STR_LEN), true);
#endif /* CONFIG_MOT_FEAT_APP_COREDUMP_DISPLAY */

       elf_clear_last_dump_status();



#endif /* CONFIG_MOT_FEAT_APP_DUMP */

	binfmt = current->binfmt;
	if (!binfmt || !binfmt->core_dump)
		goto fail;

	down_write(&mm->mmap_sem);
        mm->dumpable = 1;
	if (!mm->dumpable) {
		up_write(&mm->mmap_sem);
		goto fail;
	}
	mm->dumpable = 0;
	init_completion(&mm->core_done);
	current->signal->group_exit = 1;
	current->signal->group_exit_code = exit_code;
	coredump_wait(mm);

	if (current->signal->rlim[RLIMIT_CORE].rlim_cur < binfmt->min_coredump)
		goto fail_unlock;

	/*
	 * lock_kernel() because format_corename() is controlled by sysctl, which
	 * uses lock_kernel()
	 */
 	lock_kernel();

#ifdef CONFIG_MOT_FEAT_APP_DUMP
       if(sd_exist == 1)
        {
            strcpy(core_pattern, SD_DUMP_DIR);
            strcat(core_pattern, SD_FULL_DUMP_PATTERN);
        }
#endif /* CONFIG_MOT_FEAT_APP_DUMP */


	format_corename(corename, core_pattern, signr);
	unlock_kernel();
	file = filp_open(corename, O_CREAT | 2 | O_NOFOLLOW | O_LARGEFILE, 0600);
	if (IS_ERR(file))
		goto fail_unlock;
	inode = file->f_dentry->d_inode;
	if (inode->i_nlink > 1)
		goto close_fail;	/* multiple links - don't dump */
	if (d_unhashed(file->f_dentry))
		goto close_fail;

	if (!S_ISREG(inode->i_mode))
		goto close_fail;
	if (!file->f_op)
		goto close_fail;
	if (!file->f_op->write)
		goto close_fail;
	if (do_truncate(file->f_dentry, 0) != 0)
		goto close_fail;

	retval = binfmt->core_dump(signr, regs, file);

#ifdef CONFIG_MOT_FEAT_APP_DUMP
        if(elf_get_last_dump_status())
        {
                printk("There is not enough space in SD for full dump.\n");

                /* Make coredump file size to zero. */
                do_truncate(file->f_dentry, 0);
        }
        else {
                printk("Full coredump: %s.\n", corename);
        }
        
        /* When we come here, dump level is already 3 */
        if (sd_exist == 1) { 
                printk("Call user space script for backing up...return (%d)\n",
                   call_post_dump_script(dump_level, corename));
        }
        
        
#endif /* CONFIG_MOT_FEAT_APP_DUMP */



	if (retval)
		current->signal->group_exit_code |= 0x80;
close_fail:
	filp_close(file, NULL);
fail_unlock:
	complete_all(&mm->core_done);
fail:
	return retval;
}
