/*
 *  fs/fshook.c
 *
 *  Copyright 2005 McAfee, Inc. All rights reserved.  All rights reserved.
 *
 *  File content inspection hooking
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/signal.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/random.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/rwsem.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/atomic.h>

#include <linux/fshook.h>



/*
 * Define this to value from 1 to 10 to enable progressively verbose
 * kernel logging.
 */
#define DEBUG	0

#if DEBUG
#define DPRINTK(x)	printk x
#define DNPRINTK(n, x)	do { if ((n) <= DEBUG && !in_atomic()) printk x; } while (0)
#else
#define DPRINTK(x)
#define DNPRINTK(n, x)	(void) 0
#endif

/*
 * Define this to 1 to enable slab allocation debugging. Note that this
 * significantly increases the required memory.
 */
#define DEBUG_CAI	0

#if DEBUG_CAI
#define CAI_SLAB_DEBUG	(SLAB_DEBUG_FREE | SLAB_RED_ZONE /* | SLAB_POISON */)
#else
#define CAI_SLAB_DEBUG	0
#endif

/*
 * Size of the event cache. Every time an application's I/O is intercepted,
 * an event is placed inside the event queue. This is the maximum number of
 * pending events to be examined by the userspace helper.
 */
#define FSH_EVENT_CACHE_SIZE	256

/*
 * Maximum number of entries in the name cache, and maximum number of bits
 * of the name cache hash.
 */
#define FSH_CACHE_SIZE	256
#define FSH_MAX_HASH_BITS	16

/*
 * Maximum number of "I/O interception bypass tasks".
 */
#define FSH_MAX_BYPASS_TASKS	16

/*
 * Default timeout that the kernel module will wait for the event to be
 * processed by the userspace helper (-1 == infinite).
 */
#define FSH_STD_SCAN_TIMEO	-1

/*
 * Flags for the "flags" member of the "struct fshtbypass" structure.
 */
#define FSH_BYPF_IS_HELPER (1 << 0)

/*
 * This comes from the now (2.6) standard include/linux/hash.h
 */
#if BITS_PER_LONG == 32
/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define FSH_GOLDEN_RATIO_PRIME 0x9e370001UL
#elif BITS_PER_LONG == 64
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define FSH_GOLDEN_RATIO_PRIME 0x9e37fffffffc0001UL
#else
#error Define FSH_GOLDEN_RATIO_PRIME for your wordsize.
#endif

/*
 * Status codes returned by the userspace scan daemon.
 */
#define FSH_STATUS_BLOCK 0
#define FSH_STATUS_PASS 1
#define FSH_STATUS_NOCACHE 2

#define LIST_FIRST(h)	(list_empty(h) ? NULL: (h)->next)
#define LIST_LAST(h)	(list_empty(h) ? NULL: (h)->prev)

/*
 * Flags definitions for the "flags" member of the "struct fsevent".
 */
#define FSH_EVF_WQHOT (1 << 0)

/*
 * Structure defining an entry in the "path name exclude list".
 */
struct fsexname {
	struct list_head ll_lnk;
	int len;
	char fname[0];
};

/*
 * Structure defining an entry in the name cache.
 */
struct cacheitem {
	struct list_head ll_hash;
	struct list_head ll_lnk;
	char *fname;
};

/*
 * Structure defining an entry in the even hash/list.
 */
struct fsevent {
	struct list_head ll_hash;
	struct list_head ll_lnk;
	int status;
	unsigned long flags;
	wait_queue_head_t wq;
	wait_queue_t wait;
	unsigned long event_id;
	char *fname;
};

struct fshtbypass {
	struct task_struct *tsk;
	unsigned long flags;
};

/*
 * Main structure for the hook device.
 */
struct fshook {
	atomic_t usecnt;
	int enabled;
	struct rw_semaphore sem;
	wait_queue_head_t wq;
	wait_queue_head_t poll_wq;
	struct list_head event_list;
	struct list_head event_hot_list;
	unsigned long event_id;
	struct list_head *event_hash;
	unsigned int event_hbits;
	unsigned int event_hmask;
	struct list_head cache_list;
	int cache_items, cache_size;
	struct list_head *cache_hash;
	unsigned int cache_hbits;
	unsigned int cache_hmask;
	struct list_head cache_excl_list;
	int nbtasks;
	struct fshtbypass btasks[FSH_MAX_BYPASS_TASKS];
	struct list_head exname_list;
	long scan_timeo;
};




static struct fsexname *fsh_namelist_match(struct list_head *lsthead, char const *fname);
static struct fsexname *fsh_namelist_add(struct list_head *lsthead, char const *fname);
static int fsh_btask_add(struct fshook *fsh, struct task_struct *tsk,
			 unsigned long flags);
static int fsh_btask_del(struct fshook *fsh, struct task_struct *tsk);
static int fsh_btask_find(struct fshook *fsh, struct task_struct *tsk);
static int fsh_btask_active(struct fshook *fsh);
static int fsh_event_init(struct fshook *fsh, char *fname, struct fsevent *fsev);
static void fsh_event_cleanup(struct fshook *fsh, struct fsevent *fsev, int status);
static unsigned long fsh_str_hash(char const *str);
static struct cacheitem *fsh_cacheitem_alloc(char const *fname);
static void fsh_cacheitem_free(struct cacheitem *cai);
static int fsh_cache_add(struct fshook *fsh, char const *fname);
static inline unsigned long fsh_hash_long(unsigned long val, unsigned int bits);
static inline int fsh_log2(unsigned long num);
static int fsh_init(struct fshook *fsh);
static void fsh_free(struct fshook *fsh);
static int fsh_namelist_clear(struct list_head *lsthead);
static int fsh_events_release(struct fshook *fsh);
static int fsh_cache_clear(struct fshook *fsh);
static struct cacheitem *fsh_cache_find(struct fshook *fsh, char const *fname, int lru);
static int fsh_event_fetch(struct fshook *fsh, long timeout, struct fsevent **pfsev);
static int fsh_event_send(struct fshook *fsh, struct fsh_event *fse);
static int fsh_event_release(struct fshook *fsh, struct fsh_release *fsr);
static int open_fshook(struct inode *inode, struct file *file);
static int close_fshook(struct inode *inode, struct file *file);
static unsigned int poll_fshook(struct file *file, poll_table *wait);
static int ioctl_fshook(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg);
static struct fshook *fsh_get_global(void);
static void fsh_release_global(struct fshook *fsh);
static int fsh_request_scan(struct fshook *fsh, char *fname, long timeout, int *status);
static int fsh_hit_file(struct nameidata *nd);
static int fsh_path_lookup(const char *path, unsigned flags, struct nameidata *nd);
static char *fsh_getname(char const *uname, int kname);
static void fsh_putname(char *fname);


/*
 * There will be only one device main structure in the system. If
 * multiple tasks will use the hook device, they will share the common
 * structure. The "gfs_sem" is used to serialize the setting of the
 * global "struct fshook" "gfsh".
 */
static struct rw_semaphore gfs_sem;
static struct fshook *gfsh = NULL;

/*
 * Slab allocator for name cache items.
 */
static kmem_cache_t *cai_cache;

/*
 * File operations handling the hook file device.
 */
static struct file_operations fshook_fops = {
	.ioctl =	ioctl_fshook,
		.open =		open_fshook,
		.release =	close_fshook,
		.poll =		poll_fshook
};

static struct miscdevice fshook = {
	FSHOOK_MINOR, "fshook", &fshook_fops
};

/*
 * Searches the the list of "struct fsexname" items, to see if the passed
 * path name "fname" starts with any of the stored in the list. Returns the
 * structure "struct fsexname" that matches "fname", or NULL.
 */
static struct fsexname *fsh_namelist_match(struct list_head *lsthead, char const *fname)
{
	struct fsexname *fsxn;
	struct list_head *lnk;

	list_for_each(lnk, lsthead) {
		fsxn = list_entry(lnk, struct fsexname, ll_lnk);

		if (!strncmp(fname, fsxn->fname, fsxn->len))
			return fsxn;
	}
	return NULL;
}


/*
 * Adds an entry in the name list.
 */
static struct fsexname *fsh_namelist_add(struct list_head *lsthead, char const *fname)
{
	int len = strlen(fname);
	struct fsexname *fsxn;
	struct list_head *lnk;

	list_for_each(lnk, lsthead) {
		fsxn = list_entry(lnk, struct fsexname, ll_lnk);

		if (!strncmp(fname, fsxn->fname, fsxn->len))
			return fsxn;
	}

	if (!(fsxn = (struct fsexname *) kmalloc(sizeof(struct fsexname) + len + 1,
						 GFP_KERNEL)))
		return NULL;

	fsxn->len = len;
	memcpy(fsxn->fname, fname, len + 1);
	list_add_tail(&fsxn->ll_lnk, lsthead);

	return fsxn;
}

/*
 * Adds an entry in the "exclude path list".
 */
static int fsh_btask_add(struct fshook *fsh, struct task_struct *tsk,
			 unsigned long flags)
{
	int i = fsh_btask_find(fsh, tsk);

	if (i >= 0)
		return i;

	if (fsh->nbtasks >= FSH_MAX_BYPASS_TASKS)
		return -1;
	fsh->btasks[fsh->nbtasks].tsk = tsk;
	fsh->btasks[fsh->nbtasks++].flags = flags;
	return fsh->nbtasks - 1;
}


/*
 * Removes an entry from the "exclude path list".
 */
static int fsh_btask_del(struct fshook *fsh, struct task_struct *tsk)
{
	int i = fsh_btask_find(fsh, tsk);

	if (i >= 0) {
		fsh->nbtasks--;
		if (i < fsh->nbtasks)
			memcpy(&fsh->btasks[i], &fsh->btasks[i + 1],
			       (fsh->nbtasks - i) * sizeof(fsh->btasks[0]));
	}
	return i;
}


/*
 * Find a task in the "exclude path list". Returns a number >= 0 (index of the array)
 * if found, or -1 if not found.
 */
static int fsh_btask_find(struct fshook *fsh, struct task_struct *tsk)
{
	int i;
	struct fshtbypass *btasks;

	for (i = fsh->nbtasks - 1, btasks = &fsh->btasks[i]; i >= 0; i--, btasks--)
		if (btasks->tsk == tsk)
			return i;
	return -1;
}


/*
 * When an I/O is intercepted by the hook device, we have to know if exist
 * at least one task helper that is not stopped. This because, expecially
 * when power supend modes are active, events may occur before the helper
 * task will be wake up.
 */
static int fsh_btask_active(struct fshook *fsh)
{
	int i;
	struct fshtbypass *btasks;

	for (i = fsh->nbtasks - 1, btasks = &fsh->btasks[i]; i >= 0; i--, btasks--)
		if ((btasks->flags & FSH_BYPF_IS_HELPER) &&
		    btasks->tsk->state != TASK_STOPPED)
			return 1;

	return 0;
}


/*
 * Initializes an event. This includes initializing the wait queue head and entry,
 * and adding the current task inside the wait queue head. Then the vent is linked
 * to both the event list and the event hash. The status of the event is set to -1
 * to indicate that the event is still pending.
 */
static int fsh_event_init(struct fshook *fsh, char *fname, struct fsevent *fsev)
{
	unsigned long hidx;

	memset(fsev, 0, sizeof(*fsev));
	init_waitqueue_head(&fsev->wq);
	init_waitqueue_entry(&fsev->wait, current);
	add_wait_queue(&fsev->wq, &fsev->wait);
	fsev->status = -1;
	fsev->fname = fname;
	fsev->flags = FSH_EVF_WQHOT;

	fsev->event_id = fsh->event_id++;
	hidx = fsh_hash_long(fsev->event_id, fsh->event_hbits);
	list_add(&fsev->ll_hash, &fsh->event_hash[hidx]);
	list_add_tail(&fsev->ll_lnk, &fsh->event_list);

	return 0;
}


/*
 * Cleanup the event by unlinking the event structure from the event list and hash.
 * Also, any eventual waiters are woken up, and the wait queue entry is removed from
 * the wait queue head.
 */
static void fsh_event_cleanup(struct fshook *fsh, struct fsevent *fsev, int status)
{

	fsev->status = status;
	if (!list_empty(&fsev->ll_hash))
		list_del_init(&fsev->ll_hash);
	if (!list_empty(&fsev->ll_lnk))
		list_del_init(&fsev->ll_lnk);

	if (waitqueue_active(&fsev->wq))
		wake_up(&fsev->wq);
	if (fsev->flags & FSH_EVF_WQHOT) {
		fsev->flags &= ~FSH_EVF_WQHOT;
		remove_wait_queue(&fsev->wq, &fsev->wait);
	}
}


/*
 * Given a string (typically a file path) returns an unsigned long hash value.
 */
static unsigned long fsh_str_hash(char const *str)
{
	unsigned long sh = 0x09041934;
	unsigned char const *ptr = (unsigned char const *) str;

	for (; *ptr; ptr++) {
		sh += (sh << 3);
		sh ^= (unsigned long) *ptr;
	}
	return sh;
}


/*
 * Allocates an entry for the file name cache database.
 */
static struct cacheitem *fsh_cacheitem_alloc(char const *fname)
{
	int len = strlen(fname);
	struct cacheitem *cai;

	if (!(cai = (struct cacheitem *) kmem_cache_alloc(cai_cache, SLAB_KERNEL)))
		return NULL;
	if (!(cai->fname = (char *) kmalloc(len + 1, GFP_KERNEL))) {
		kmem_cache_free(cai_cache, cai);
		return NULL;
	}
	INIT_LIST_HEAD(&cai->ll_hash);
	INIT_LIST_HEAD(&cai->ll_lnk);
	memcpy(cai->fname, fname, len + 1);

	return cai;
}


/*
 * Frees a file name cache entry.
 */
static void fsh_cacheitem_free(struct cacheitem *cai)
{

	if (!list_empty(&cai->ll_hash))
		list_del_init(&cai->ll_hash);
	if (!list_empty(&cai->ll_lnk))
		list_del_init(&cai->ll_lnk);
	kfree(cai->fname);
	kmem_cache_free(cai_cache, cai);
}


/*
 * Adds a new entry to the file name cache. If the number of allocated items
 * grows over our limits, the tail of the LRU queue is freed. The entry is then
 * added to both the name cache list and hash.
 */
static int fsh_cache_add(struct fshook *fsh, char const *fname)
{
	unsigned long hidx;
	struct cacheitem *cai, *rcai;

	if (!(cai = fsh_cacheitem_alloc(fname)))
		return -ENOMEM;

	hidx = fsh_hash_long(fsh_str_hash(fname), fsh->cache_hbits);

	down_write(&fsh->sem);
	if (fsh->cache_items >= fsh->cache_size) {
		struct list_head *lnk = LIST_LAST(&fsh->cache_list);

		if (lnk) {
			rcai = list_entry(lnk, struct cacheitem, ll_lnk);

			fsh_cacheitem_free(rcai);
			fsh->cache_items--;
		}
	}
	list_add(&cai->ll_hash, &fsh->cache_hash[hidx]);
	list_add(&cai->ll_lnk, &fsh->cache_list);
	fsh->cache_items++;
	up_write(&fsh->sem);

	return 0;
}


/*
 * Given an unsigned long value, returns an hash index for a "bits"
 * sized hash table.
 */
static inline unsigned long fsh_hash_long(unsigned long val, unsigned int bits)
{

	return (val * FSH_GOLDEN_RATIO_PRIME) >> (BITS_PER_LONG - bits);
}


/*
 * Calculates the log2 value of "num".
 */
static inline int fsh_log2(unsigned long num)
{
	int l;

	for (l = 0; num > 1; num >>= 1, l++);
	return l;
}


/*
 * Initializes the main structure for the hook device.
 */
static int fsh_init(struct fshook *fsh)
{
	int i, hentries;

	fsh->enabled = 0;
	atomic_set(&fsh->usecnt, 1);
	init_rwsem(&fsh->sem);
	init_waitqueue_head(&fsh->wq);
	init_waitqueue_head(&fsh->poll_wq);
	fsh->nbtasks = 0;
	INIT_LIST_HEAD(&fsh->exname_list);
	fsh->scan_timeo = FSH_STD_SCAN_TIMEO;

	INIT_LIST_HEAD(&fsh->event_list);
	INIT_LIST_HEAD(&fsh->event_hot_list);
	fsh->event_id = 0;
	fsh->event_hbits = fsh_log2(FSH_EVENT_CACHE_SIZE);
	fsh->event_hmask = (1 << fsh->event_hbits) - 1;
	hentries = fsh->event_hmask + 1;
	if (!(fsh->event_hash = (struct list_head *) vmalloc(hentries * sizeof(struct list_head))))
		return -ENOMEM;
	for (i = 0; i < hentries; i++)
		INIT_LIST_HEAD(&fsh->event_hash[i]);

	INIT_LIST_HEAD(&fsh->cache_list);
	fsh->cache_items = 0;
	fsh->cache_size = FSH_CACHE_SIZE;
	fsh->cache_hbits = fsh_log2(fsh->cache_size);
	fsh->cache_hmask = (1 << fsh->cache_hbits) - 1;
	hentries = fsh->cache_hmask + 1;
	if (!(fsh->cache_hash = (struct list_head *) vmalloc(hentries * sizeof(struct list_head)))) {
		vfree(fsh->event_hash);
		return -ENOMEM;
	}
	for (i = 0; i < hentries; i++)
		INIT_LIST_HEAD(&fsh->cache_hash[i]);
	INIT_LIST_HEAD(&fsh->cache_excl_list);

	return 0;
}


/*
 * Free the main structure for the hook device, by releasing, among other things,
 * all pending I/O events currently queued.
 */
static void fsh_free(struct fshook *fsh)
{

	down_write(&fsh->sem);
	fsh_cache_clear(fsh);
	fsh_events_release(fsh);
	fsh_namelist_clear(&fsh->exname_list);
	fsh_namelist_clear(&fsh->cache_excl_list);
	up_write(&fsh->sem);

	vfree(fsh->cache_hash);
	vfree(fsh->event_hash);
}


/*
 * Cleanup a list containing "struct fsexname" items. These lists are used
 * for the scan-exclusion list, and for the name-cache-exclusion list.
 */
static int fsh_namelist_clear(struct list_head *lsthead)
{
	int names = 0;
	struct list_head *lnk;

	while ((lnk = LIST_FIRST(lsthead))) {
		struct fsexname *fsxn = list_entry(lnk, struct fsexname, ll_lnk);

		list_del(lnk);
		kfree(fsxn);
		names++;
	}
	return names;
}


/*
 * Releases all pending queued events.
 */
static int fsh_events_release(struct fshook *fsh)
{
	int events = 0;
	struct list_head *lnk;

	while ((lnk = LIST_FIRST(&fsh->event_list))) {
		struct fsevent *fsev = list_entry(lnk, struct fsevent, ll_lnk);

		fsh_event_cleanup(fsh, fsev, 1);
		events++;
	}
	while ((lnk = LIST_FIRST(&fsh->event_hot_list))) {
		struct fsevent *fsev = list_entry(lnk, struct fsevent, ll_lnk);

		fsh_event_cleanup(fsh, fsev, 1);
		events++;
	}

	return events;
}


/*
 * Free the file name cache.
 */
static int fsh_cache_clear(struct fshook *fsh)
{
	int items = 0;
	struct list_head *lnk;

	while ((lnk = LIST_FIRST(&fsh->cache_list))) {
		struct cacheitem *cai = list_entry(lnk, struct cacheitem, ll_lnk);

		fsh_cacheitem_free(cai);
		fsh->cache_items--;
		items++;
	}
	return items;
}


/*
 * Tries to lookup a file name inside the cache. If the "lru" parameter
 * is different from 0, an LRU transformation is performed.
 */
static struct cacheitem *fsh_cache_find(struct fshook *fsh, char const *fname, int lru)
{
	unsigned long hidx = fsh_hash_long(fsh_str_hash(fname), fsh->cache_hbits);
	struct cacheitem *cai = NULL;
	struct list_head *lsthead, *lnk;

	lsthead = &fsh->cache_hash[hidx];
	list_for_each(lnk, lsthead) {
		cai = list_entry(lnk, struct cacheitem, ll_hash);

		if (!strcmp(fname, cai->fname)) break;
		cai = NULL;
	}

	if (cai && lru) {
		list_del(&cai->ll_lnk);
		list_add(&cai->ll_lnk, &fsh->cache_list);
	}

	DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: fsh_cache_find(%s) -> %p\n",
		     current, fname, cai));

	return cai;
}


/*
 * Fetches an I/O event from the event queue, and sleeps if no events are
 * present (up to the "timeout" ms parameter). If an event is fetched, the
 * hook device semaphore is held locked, and the caller will be responsible
 * for unlocking. The event, if fetched, is also removed from the event list
 * and put in the event hot list.
 */
static int fsh_event_fetch(struct fshook *fsh, long timeout, struct fsevent **pfsev)
{
	int res = 0;
	long jtimeout;
	wait_queue_t wait;

	jtimeout = timeout == -1 || timeout > (MAX_SCHEDULE_TIMEOUT - 1000) / HZ ?
		MAX_SCHEDULE_TIMEOUT: (timeout * HZ + 999) / 1000;

	down_write(&fsh->sem);

	retry:
	if (list_empty(&fsh->event_list)) {
		init_waitqueue_entry(&wait, current);
		add_wait_queue(&fsh->wq, &wait);

		for (;;) {
			set_current_state(TASK_INTERRUPTIBLE);
			if (!list_empty(&fsh->event_list) || !jtimeout)
				break;
			if (signal_pending(current)) {
				res = -EINTR;
				break;
			}

			up_write(&fsh->sem);
			jtimeout = schedule_timeout(jtimeout);
			down_write(&fsh->sem);
		}
		remove_wait_queue(&fsh->wq, &wait);

		set_current_state(TASK_RUNNING);
	}

	if (!res) {
		struct list_head *lnk;

		if ((lnk = LIST_FIRST(&fsh->event_list))) {
			struct fsevent *fsev = list_entry(lnk, struct fsevent, ll_lnk);

			list_del(&fsev->ll_lnk);
			list_add_tail(&fsev->ll_lnk, &fsh->event_hot_list);
			*pfsev = fsev;
			res = 1;
		} else if (jtimeout)
				goto retry;
	}

	if (res <= 0)
		up_write(&fsh->sem);

	return res;
}


/*
 * Fetch an event and fill up the structure that will be returned to
 * userspace as response to an ioctl(FSH_EVENT).
 */
static int fsh_event_send(struct fshook *fsh, struct fsh_event *fse)
{
	int res, len;
	struct fsevent *fsev;

	if ((res = fsh_event_fetch(fsh, fse->fse_timeout, &fsev)) <= 0)
		return res;

	len = strlen(fsev->fname);
	if (len + 1 > fse->fse_name.fst_len ||
	    copy_to_user(fse->fse_name.fst_data, fsev->fname, len + 1)) {
		list_del(&fsev->ll_lnk);
		list_add_tail(&fsev->ll_lnk, &fsh->event_list);
		up_write(&fsh->sem);
		return -EFAULT;
	}
	fse->fse_id = fsev->event_id;
	up_write(&fsh->sem);

	return 1;
}


/*
 * Handles the release of an event, activated by an ioctl(FSH_RELEASE). The
 * kernel side event structure is set up with the response code coming from
 * the userspace helper, and all the waiter will be woken up.
 */
static int fsh_event_release(struct fshook *fsh, struct fsh_release *fsr)
{
	unsigned long hidx = fsh_hash_long(fsr->fsr_id, fsh->event_hbits);
	struct list_head *lsthead, *lnk;

	lsthead = &fsh->event_hash[hidx];
	list_for_each(lnk, lsthead) {
		struct fsevent *fsev = list_entry(lnk, struct fsevent, ll_hash);

		if (fsev->event_id == fsr->fsr_id) {
			fsev->status = fsr->fsr_status;
			list_del_init(&fsev->ll_hash);
			list_del_init(&fsev->ll_lnk);
			if (waitqueue_active(&fsev->wq))
				wake_up(&fsev->wq);

			return 1;
		}
	}

	return 0;
}


/*
 * Kernel side function that handles the open of the file device. There
 * is at most one "struct fshook" present in the system, and all the
 * instances of the hook device will refer to that. The open function
 * either allocates a new one if it is the first hook device to be opened,
 * or uses the existing one by increasing the internal use count of the
 * structure itself.
 */
static int open_fshook(struct inode *inode, struct file *file)
{
	int res;
	struct fshook *fsh;
	
	down_write(&gfs_sem);
	if (gfsh) {
		down_write(&gfsh->sem);
		atomic_inc(&gfsh->usecnt);
		fsh_btask_add(gfsh, current, FSH_BYPF_IS_HELPER);
		up_write(&gfsh->sem);
		fsh = gfsh;
	} else {
		if (!(fsh = kmalloc(sizeof(struct fshook), GFP_KERNEL))) {
			up_write(&gfs_sem);
			return -ENOMEM;
		}

		memset(fsh, 0, sizeof(*fsh));
		if ((res = fsh_init(fsh))) {
			kfree(fsh);
			up_write(&gfs_sem);
			return res;
		}
		fsh_btask_add(fsh, current, FSH_BYPF_IS_HELPER);
		gfsh = fsh;
	}

	file->private_data = fsh;

	up_write(&gfs_sem);

	DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: open() fsh=%p\n", current, fsh));
	return 0;
}


/*
 * Kernel side function that handles the close of the hook device.
 * If the internal use count of the stucture goes to zero, the structure
 * itself is freed.
 */
static int close_fshook(struct inode *inode, struct file *file)
{
	struct fshook *fsh = file->private_data;

	down_write(&fsh->sem);
	fsh_btask_del(fsh, current);
	up_write(&fsh->sem);

	down_write(&gfs_sem);
	if (atomic_dec_and_test(&fsh->usecnt)) {
		fsh_free(fsh);
		kfree(fsh);
		gfsh = NULL;
	}
	up_write(&gfs_sem);

	DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: close() fsh=%p\n", current, fsh));
	return 0;
}


/*
 * This function is the device helper for the poll(2)/select(2) system calls.
 * It enables the userspace helper to have multiplexed event handling inside
 * a single task, without using threads.
 */
static unsigned int poll_fshook(struct file *file, poll_table *wait)
{
	unsigned int pollflags = 0;
	struct fshook *fsh = file->private_data;

	poll_wait(file, &fsh->poll_wq, wait);

	down_read(&fsh->sem);
	if (!list_empty(&fsh->event_list))
		pollflags = POLLIN | POLLRDNORM;
	up_read(&fsh->sem);

	return pollflags;
}


/*
 * Kernel side function that handles the ioctl on the hook device.
 */
static int ioctl_fshook(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	int res;
	char *fname;
	struct fshook *fsh = file->private_data;
	struct task_struct *tsk;
	struct fsh_event fse;
	struct fsh_release fsr;

	switch (cmd) {
	case FSH_S_CACHE_SIZE:

		return 0;

	case FSH_DISABLE_PID:
		down_write(&fsh->sem);
		read_lock(&tasklist_lock);
		tsk = find_task_by_pid(arg);
		res = -ESRCH;
		if (tsk) {
			fsh_btask_add(fsh, tsk, 0);
			res = 0;
		}
		read_unlock(&tasklist_lock);
		up_write(&fsh->sem);

		return res;

	case FSH_ENABLE_PID:
		down_write(&fsh->sem);
		read_lock(&tasklist_lock);
		tsk = find_task_by_pid(arg);
		res = -ESRCH;
		if (tsk && fsh_btask_del(fsh, tsk) >= 0)
			res = 0;
		read_unlock(&tasklist_lock);
		up_write(&fsh->sem);

		return res;

	case FSH_TASK_ATTACH:
		down_write(&fsh->sem);
		fsh_btask_add(fsh, current, FSH_BYPF_IS_HELPER);
		up_write(&fsh->sem);

		return 0;

	case FSH_EXNAME_ADD:
		fname = getname((void *) arg);
		res = PTR_ERR(fname);
		if (!IS_ERR(fname)) {
			down_write(&fsh->sem);
			res = fsh_namelist_add(&fsh->exname_list, fname) ? 0: -ENOMEM;
			up_write(&fsh->sem);

			putname(fname);
		}

		DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: ioctl(%p, FSH_EXNAME_ADD, %p) == %d\n",
			     current, fsh, (void *) arg, res));

		return res;

	case FSH_CACHE_EXNAME_ADD:
		fname = getname((void *) arg);
		res = PTR_ERR(fname);
		if (!IS_ERR(fname)) {
			down_write(&fsh->sem);
			res = fsh_namelist_add(&fsh->cache_excl_list, fname) ? 0: -ENOMEM;
			up_write(&fsh->sem);

			putname(fname);
		}

		DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: ioctl(%p, FSH_CACHE_EXNAME_ADD, %p) == %d\n",
			     current, fsh, (void *) arg, res));

		return res;

	case FSH_START:
		down_write(&fsh->sem);
		fsh->enabled++;
		up_write(&fsh->sem);

		DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: ioctl(%p, FSH_START) == 0\n",
			     current, fsh));
		return 0;

	case FSH_STOP:
		down_write(&fsh->sem);
		if (fsh->enabled > 0 && !--fsh->enabled)
			fsh_events_release(fsh);
		up_write(&fsh->sem);

		DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: ioctl(%p, FSH_STOP) == 0\n",
			     current, fsh));
		return 0;

	case FSH_CACHE_CLEAR:
		down_write(&fsh->sem);
		fsh_cache_clear(fsh);
		up_write(&fsh->sem);

		DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: ioctl(%p, FSH_CACHE_CLEAR) == 0\n",
			     current, fsh));
		return 0;

	case FSH_EVENT:
		if (copy_from_user(&fse, (void *) arg, sizeof(fse)))
			return -EFAULT;

		res = fsh_event_send(fsh, &fse);

		if (res > 0 && copy_to_user((void *) arg, &fse, sizeof(fse)))
			return -EFAULT;

		DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: ioctl(%p, FSH_EVENT, %lu) == %d\n",
			     current, fsh, fse.fse_id, res));
		return res;

	case FSH_RELEASE:
		if (copy_from_user(&fsr, (void *) arg, sizeof(fsr)))
			return -EFAULT;

		down_write(&fsh->sem);
		res = fsh_event_release(fsh, &fsr);
		up_write(&fsh->sem);

		DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook: ioctl(%p, FSH_RELEASE, %lu) == %d\n",
			     current, fsh, fsr.fsr_id, res));
		return res;
	}

	return -EINVAL;
}


/*
 * Detaches a give task from the internal bypass list of the hook device.
 * This is used during the kernel side of a task exits, to handle cases
 * where the application quits without having either close the device or
 * forgot to deregister itself from the task exclusion list.
 */
int fsh_task_detach(struct task_struct *tsk)
{
	int res = 0;

	if (!gfsh)
		goto out;
	down_write(&gfs_sem);
	if (!gfsh)
		goto out_uw;
	down_write(&gfsh->sem);
	res = fsh_btask_del(gfsh, tsk) >= 0;
	up_write(&gfsh->sem);
	out_uw:
	up_write(&gfs_sem);
	out:
	return res;
}


/*
 * Get a valid reference to the main hook device structure. For a valid
 * pointer to be returned, at least one instance of the device must be
 * opened, the I/O interception must be enabled, the calling task should
 * not be present inside the task exclusion list, and at least one of the
 * userspace helpers must be in non-suspended mode.
 */
static struct fshook *fsh_get_global(void)
{
	struct fshook *fsh = NULL;

	if (!gfsh)
		return NULL;
	down_write(&gfs_sem);
	if (!gfsh || !gfsh->enabled) {
		up_write(&gfs_sem);
		return NULL;
	}
	down_write(&gfsh->sem);
	if (fsh_btask_find(gfsh, current) < 0 &&
	    fsh_btask_active(gfsh)) {
		atomic_inc(&gfsh->usecnt);
		fsh = gfsh;
	}
	up_write(&gfsh->sem);
	up_write(&gfs_sem);
	return fsh;
}


/*
 * Releases the instance of the hook device structure previously acquired
 * using the fsh_get_global() function.
 */
static void fsh_release_global(struct fshook *fsh)
{

	down_write(&gfs_sem);
	if (atomic_dec_and_test(&fsh->usecnt)) {
		fsh_free(fsh);
		kfree(fsh);
		gfsh = NULL;
	}
	up_write(&gfs_sem);
}


/*
 * This is the kernel side code that will handle I/O events. In a monolithic
 * kernel like Linux, an application has the userspace part and the corresponding
 * kernel part. If the kernel part suspend itself, like the following function
 * does, the associated userspace application will be suspended as a consequence.
 * The following function wakes up userspace helpers, is waiting on ioctl(FSH_EVENT).
 */
static int fsh_request_scan(struct fshook *fsh, char *fname, long timeout, int *status)
{
	int res;
	long jtimeout;
	struct fsevent fsev;

	down_write(&fsh->sem);
	if ((res = fsh_event_init(fsh, fname, &fsev)) < 0) {
		up_write(&fsh->sem);
		return res;
	}

	if (waitqueue_active(&fsh->wq))
		wake_up(&fsh->wq);
	if (waitqueue_active(&fsh->poll_wq))
		wake_up(&fsh->poll_wq);

	jtimeout = timeout == -1 || timeout > (MAX_SCHEDULE_TIMEOUT - 1000) / HZ ?
		MAX_SCHEDULE_TIMEOUT: (timeout * HZ + 999) / 1000;

	do {
		set_current_state(TASK_UNINTERRUPTIBLE);

		up_write(&fsh->sem);
		jtimeout = schedule_timeout(jtimeout);
		down_write(&fsh->sem);

		set_current_state(TASK_RUNNING);
	} while (jtimeout && fsev.status < 0);

	*status = fsev.status;
	fsh_event_cleanup(fsh, &fsev, 0);

	up_write(&fsh->sem);

	return 0;
}


/*
 * Tells us if the file type matches the ones we want to scan (regular files).
 */
static int fsh_hit_file(struct nameidata *nd)
{

	if (!nd->dentry || !nd->dentry->d_inode ||
	    !S_ISREG(nd->dentry->d_inode->i_mode))
		return 0;

	return 1;
}


/*
 * Perform path name lookup and fill the "struct nameidata" structure
 * will all file system information needed to normalize the path name.
 */
static int fsh_path_lookup(const char *path, unsigned flags, struct nameidata *nd)
{
	return path_lookup(path, flags, nd);
}


/*
 * Given a file path "uname", in user address space, perform the name lookup
 * and return a normalized version of the name. Also, checks if the file type
 * is one that we want to scan (using the fsh_hit_file() function).
 */
static char *fsh_getname(char const *uname, int kname)
{
	char *fname, *path;
	struct nameidata nd;

	fname = (char *) __get_free_page(GFP_USER);
	if (!fname)
		return NULL;
	if (!kname) {
		if (strncpy_from_user(fname, uname, PAGE_SIZE) < 0) {
			fsh_putname(fname);
			return NULL;
		}
	} else
		strncpy(fname, uname, PAGE_SIZE);

	if (fsh_path_lookup(fname, LOOKUP_FOLLOW, &nd)) {
		fsh_putname(fname);
		return NULL;
	}

	if (!fsh_hit_file(&nd)) {
		path_release(&nd);
		fsh_putname(fname);
		return NULL;
	}

	path = d_path(nd.dentry, nd.mnt, fname, PAGE_SIZE);
	path_release(&nd);

	if (IS_ERR(path)) {
		fsh_putname(fname);
		return NULL;
	}

	return path;
}


/*
 * Frees a file path returned by the fsh_getname() function.
 */
static void fsh_putname(char *fname)
{

	free_page(((unsigned long) fname) & ~(PAGE_SIZE - 1));
}


/*
 * Handles the "rename" (sys_rename) event by cleaning up the "nname"
 * name cache entry, if present.
 */
asmlinkage long fsh_sys_rename(const char *oname, const char *nname)
{
	struct fshook *fsh;
	char *fname;
	struct cacheitem *cai;

	if (!(fsh = fsh_get_global()))
		return 0;

	fname = fsh_getname(nname, 0);
	if (fname) {
		down_write(&fsh->sem);
		if ((cai = fsh_cache_find(fsh, fname, 0)) != NULL) {
			fsh_cacheitem_free(cai);
			fsh->cache_items--;
		}
		up_write(&fsh->sem);

		fsh_putname(fname);
	}

	fsh_release_global(fsh);
	return 0;
}


/*
 * Handles the "unlink" (sys_unlink) event by cleaning up the name
 * cache entry, if present.
 */
asmlinkage long fsh_sys_unlink(const char *filename)
{
	struct fshook *fsh;
	char *fname;
	struct cacheitem *cai;

	if (!(fsh = fsh_get_global()))
		return 0;

	fname = fsh_getname(filename, 0);
	if (fname) {
		down_write(&fsh->sem);
		if ((cai = fsh_cache_find(fsh, fname, 0)) != NULL) {
			fsh_cacheitem_free(cai);
			fsh->cache_items--;
		}
		up_write(&fsh->sem);

		fsh_putname(fname);
	}

	fsh_release_global(fsh);
	return 0;
}


/*
 * Handles the interception of the userspace I/O done on files. The
 * first step is to retrieve a valid hook device structure. If no
 * hook device are opened, or if hook devices are opened but the
 * corresponding userspace helper are all in suspended state, the
 * function will simply return 0 and the system call will pass though
 * and reach the real kernel processing.
 */
int fsh_check_file(const char *filename, int kname, int flags)
{
	int res = 0, cache_add, status;
	struct fshook *fsh;
	char *fname;
	struct cacheitem *cai;

	if (!(fsh = fsh_get_global()))
		goto out;
	fname = fsh_getname(filename, kname);
	if (!fname)
		goto out_grel;
	down_write(&fsh->sem);

	/*
	 * Searches the "exclude path list" to see if the passed path name "fname"
	 * starts with any of the stored exclusion paths. Exclusion paths are
	 * typically reserved for things like "/dev" or "/proc". The function
	 * returns the structure "struct fsexname" that matches "fname", or NULL.
	 */
	if (fsh_namelist_match(&fsh->exname_list, fname)) {
		up_write(&fsh->sem);
		goto out_pn;
	}
	if ((flags & O_WRONLY) == O_WRONLY) {
		if ((cai = fsh_cache_find(fsh, fname, 0)) != NULL) {
			fsh_cacheitem_free(cai);
			fsh->cache_items--;
		}
		up_write(&fsh->sem);
		goto out_pn;
	} else if ((flags & O_RDWR) == O_RDWR) {
		if ((cai = fsh_cache_find(fsh, fname, 0)) != NULL) {
			fsh_cacheitem_free(cai);
			fsh->cache_items--;
		}
		up_write(&fsh->sem);
		if (cai)
			goto out_pn;
		cache_add = 0;
	} else if ((flags & O_RDONLY) == O_RDONLY) {
		cai = fsh_cache_find(fsh, fname, 1);
		up_write(&fsh->sem);
		if (cai)
			goto out_pn;
		cache_add = 1;
	}

	if (fsh_request_scan(fsh, fname, fsh->scan_timeo, &status) == 0) {
		if (status == FSH_STATUS_BLOCK) {
			res = -EACCES;
			goto out_pn;
		} else if (status == FSH_STATUS_PASS) {

		} else if (status == FSH_STATUS_NOCACHE) {
			cache_add = 0;
		} else if (status < 0) {
			DNPRINTK(1, (KERN_INFO "[%p] /dev/fshook: scan timeout: file='%s' pid=%u\n",
				     current, fname, current->pid));
			goto out_pn;
		}
		if (cache_add &&
		    !fsh_namelist_match(&fsh->cache_excl_list, fname))
			fsh_cache_add(fsh, fname);
	}

	out_pn:
	fsh_putname(fname);
	out_grel:
	fsh_release_global(fsh);
	out:
	return res;
}


/*
 * Handles the "open" (sys_open) event by uploading all the work to the
 * fsh_check_file() function.
 */
asmlinkage long fsh_sys_open(const char *filename, int kname, int flags, int mode)
{
	int res;

	if ((res = fsh_check_file(filename, kname, flags)) < 0)
		return res;

	return 0;
}


/*
 * Handles the "close" (sys_close) event. At the moment, no processing is done
 * on close but it is likely to be required for future imminent versions.
 */
asmlinkage long fsh_sys_close(int fd)
{

	return 0;
}


/*
 * This is the kernel module init function. It is loaded when the kernel
 * starts up.
 */
int __init fshook_init(void)
{
	int error;

	init_rwsem(&gfs_sem);

	error = -ENOMEM;
	cai_cache = kmem_cache_create("fshook",
				      sizeof(struct cacheitem),
				      __alignof__(struct cacheitem),
				      CAI_SLAB_DEBUG, NULL, NULL);
	if (!cai_cache)
		goto error_exit;

	misc_register(&fshook);

	DNPRINTK(3, (KERN_INFO "[%p] /dev/fshook ready\n",
		     current));

	return 0;

	error_exit:
	if (cai_cache)
		kmem_cache_destroy(cai_cache);
	return error;
}


module_init(fshook_init);

MODULE_LICENSE("GPL");

