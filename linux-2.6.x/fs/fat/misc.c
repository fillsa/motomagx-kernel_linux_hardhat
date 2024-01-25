/*
 *  linux/fs/fat/misc.c
 *
 *  Written 1992,1993 by Werner Almesberger
 *  22/11/2000 - Fixed fat_date_unix2dos for dates earlier than 01/01/1980
 *		 and date_dos2unix for date==0 by Igor Zhbanov(bsg@uniyar.ac.ru)
 */

/*
 * Copyright (C) 2007-2008 Motorola, Inc.
 *
 * Date         Author            Comment
 * ===========  ==========  ====================================
 * 11/01/2007   Motorola    Fix for TF write protected issue.
 * 11/26/2007   Motorola	Fix for TF write protected issue.
 * 11-15-2007   Motorola    Upmerge from 6.1 (Added conditional sync dirt mark for loop device)
 * 12-20-2007   Motorola	Added simple auto  repair FAT
 * 01-25-2008   Motorola	Remove the FS changes about repair FAT	   
 * 04-18-2008   Motorola	Upmerge from 6.3 to 7.4 (repairfat and fat sync)
 * 07-04-2008	Motorola	Remove changes for "simple auto repair FAT" in LJ7.4
 * 08-12-2008   Motorola        Add simple repair FAT
 */



#include <linux/module.h>
#include <linux/fs.h>
#include <linux/msdos_fs.h>
#include <linux/buffer_head.h>
#include <linux/blkdev.h>
#ifdef CONFIG_MOT_FEAT_FAT_SYNC
#include <linux/loop.h>
#endif
#include <asm-arm/atomic.h>
#include <linux/proc_fs.h>



/*
 * fat_fs_panic reports a severe file system problem and sets the file system
 * read-only. The file system can be made writable again by remounting it.
 */

static char panic_msg[512];

static void wakeup_repairfat(struct super_block *sb);
static void (*repairfat_callback)(struct super_block *sb) = NULL;
void register_repairfat_callback(void (*callback_fun)(struct super_block *sb))
{
	repairfat_callback = callback_fun;	
}

void unregister_repairfat_callback(void)
{
	repairfat_callback = NULL;
}

EXPORT_SYMBOL(register_repairfat_callback);
EXPORT_SYMBOL(unregister_repairfat_callback);

void fat_fs_panic(struct super_block *s, const char *fmt, ...)
{
	int not_ro;
	va_list args;

	va_start (args, fmt);
	vsnprintf (panic_msg, sizeof(panic_msg), fmt, args);
	va_end (args);

/*
 * It is the Not nessary,  and it just want to make sure 
 * all the data write to disk is right. In fact the other
 * files whose fat chain has some releationship with the 
 * chain of this inode is very small. 
 * 
 * M$ Windows do it as the following:
 *	return -EIO for the wrong files, other files are no effect.
 */
	not_ro = !(s->s_flags & MS_RDONLY);
	if (not_ro)
		s->s_flags |= MS_RDONLY;

	printk(KERN_ERR "FAT: Filesystem panic (dev %s)\n"
	       "    %s\n", s->s_id, panic_msg);
	if (not_ro)
		printk(KERN_ERR "    File system has been set read-only\n");

	if (repairfat_callback) {
		printk(KERN_INFO "FAT: mxclay call back\n");
		repairfat_callback(s);
	}
//08-12-2008   Motorola        Add simple repair FAT
#if defined(MAGX_N_06_19_60I) || defined(CONFIG_MACH_PEARL)
	wakeup_repairfat(s);
#endif
}

void lock_fat(struct super_block *sb)
{
	down(&(MSDOS_SB(sb)->fat_lock));
}

void unlock_fat(struct super_block *sb)
{
	up(&(MSDOS_SB(sb)->fat_lock));
}

/* Flushes the number of free clusters on FAT32 */
/* XXX: Need to write one per FSINFO block.  Currently only writes 1 */
void fat_clusters_flush(struct super_block *sb)
{
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	struct buffer_head *bh;
	struct fat_boot_fsinfo *fsinfo;
#ifdef CONFIG_MOT_FEAT_FAT_SYNC
        struct loop_device *lo;
        struct file *file;
#endif



	if (sbi->fat_bits != 32)
		return;

	bh = sb_bread(sb, sbi->fsinfo_sector);
	if (bh == NULL) {
		printk(KERN_ERR "FAT bread failed in fat_clusters_flush\n");
		return;
	}

	fsinfo = (struct fat_boot_fsinfo *)bh->b_data;
	/* Sanity check */
	if (!IS_FSINFO(fsinfo)) {
		printk(KERN_ERR "FAT: Did not find valid FSINFO signature.\n"
		       "     Found signature1 0x%08x signature2 0x%08x"
		       " (sector = %lu)\n",
		       le32_to_cpu(fsinfo->signature1),
		       le32_to_cpu(fsinfo->signature2),
		       sbi->fsinfo_sector);
	} else {
		if (sbi->free_clusters != -1)
			fsinfo->free_clusters = cpu_to_le32(sbi->free_clusters);
		if (sbi->prev_free != -1)
			fsinfo->next_cluster = cpu_to_le32(sbi->prev_free);
		mark_buffer_dirty(bh);
	}
	brelse(bh);

#ifdef CONFIG_MOT_FEAT_FAT_SYNC
        /* Check if the fat was mounted on a loop block device.
         * If so, we mark the mapped file system as dirty
         */
        if (MAJOR(sb->s_dev) == LOOP_MAJOR) {
                lo = sb->s_bdev->bd_disk->private_data;
                file = lo->lo_backing_file;
                file->f_dentry->d_sb->s_dirt = 1;
        }
#endif


}

/*
 * fat_add_cluster tries to allocate a new cluster and adds it to the
 * file represented by inode.
 */
int fat_add_cluster(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	int ret, count, limit, new_dclus, new_fclus, last;
	int cluster_bits = MSDOS_SB(sb)->cluster_bits;
	
	/* 
	 * We must locate the last cluster of the file to add this new
	 * one (new_dclus) to the end of the link list (the FAT).
	 *
	 * In order to confirm that the cluster chain is valid, we
	 * find out EOF first.
	 */
	last = new_fclus = 0;
	if (MSDOS_I(inode)->i_start) {
		int ret, fclus, dclus;

		ret = fat_get_cluster(inode, FAT_ENT_EOF, &fclus, &dclus);
		if (ret < 0)
			return ret;
		new_fclus = fclus + 1;
		last = dclus;
	}

	/* find free FAT entry */
	lock_fat(sb);
	
	if (MSDOS_SB(sb)->free_clusters == 0) {
		unlock_fat(sb);
		return -ENOSPC;
	}

	limit = MSDOS_SB(sb)->clusters + 2;
	new_dclus = MSDOS_SB(sb)->prev_free + 1;
	for (count = 0; count < MSDOS_SB(sb)->clusters; count++, new_dclus++) {
		new_dclus = new_dclus % limit;
		if (new_dclus < 2)
			new_dclus = 2;

		ret = fat_access(sb, new_dclus, -1);
		if (ret < 0) {
			unlock_fat(sb);
			return ret;
		} else if (ret == FAT_ENT_FREE)
			break;
	}
	if (count >= MSDOS_SB(sb)->clusters) {
		MSDOS_SB(sb)->free_clusters = 0;
		unlock_fat(sb);
		return -ENOSPC;
	}

	ret = fat_access(sb, new_dclus, FAT_ENT_EOF);
	if (ret < 0) {
		unlock_fat(sb);
		return ret;
	}

	MSDOS_SB(sb)->prev_free = new_dclus;
	if (MSDOS_SB(sb)->free_clusters != -1)
		MSDOS_SB(sb)->free_clusters--;
	fat_clusters_flush(sb);
	
	unlock_fat(sb);

	/* add new one to the last of the cluster chain */
	if (last) {
		ret = fat_access(sb, last, new_dclus);
		if (ret < 0)
			return ret;
//		fat_cache_add(inode, new_fclus, new_dclus);
	} else {
		MSDOS_I(inode)->i_start = new_dclus;
		MSDOS_I(inode)->i_logstart = new_dclus;
		mark_inode_dirty(inode);
	}
	if (new_fclus != (inode->i_blocks >> (cluster_bits - 9))) {
		fat_fs_panic(sb, "clusters badly computed (%d != %lu)",
			new_fclus, inode->i_blocks >> (cluster_bits - 9));
		fat_cache_inval_inode(inode);
	}
	inode->i_blocks += MSDOS_SB(sb)->cluster_size >> 9;

	return new_dclus;
}

struct buffer_head *fat_extend_dir(struct inode *inode)
{
	struct super_block *sb = inode->i_sb;
	struct buffer_head *bh, *res = NULL;
	int nr, sec_per_clus = MSDOS_SB(sb)->sec_per_clus;
	sector_t sector, last_sector;

	if (MSDOS_SB(sb)->fat_bits != 32) {
		if (inode->i_ino == MSDOS_ROOT_INO)
			return ERR_PTR(-ENOSPC);
	}

	nr = fat_add_cluster(inode);
	if (nr < 0)
		return ERR_PTR(nr);
	
	sector = ((sector_t)nr - 2) * sec_per_clus + MSDOS_SB(sb)->data_start;
	last_sector = sector + sec_per_clus;
	for ( ; sector < last_sector; sector++) {
		if ((bh = sb_getblk(sb, sector))) {
			memset(bh->b_data, 0, sb->s_blocksize);
			set_buffer_uptodate(bh);
			mark_buffer_dirty(bh);
			if (!res)
				res = bh;
			else
				brelse(bh);
		}
	}
	if (res == NULL)
		res = ERR_PTR(-EIO);
	if (inode->i_size & (sb->s_blocksize - 1)) {
		fat_fs_panic(sb, "Odd directory size");
		inode->i_size = (inode->i_size + sb->s_blocksize)
			& ~((loff_t)sb->s_blocksize - 1);
	}
	inode->i_size += MSDOS_SB(sb)->cluster_size;
	MSDOS_I(inode)->mmu_private += MSDOS_SB(sb)->cluster_size;

	return res;
}

/* Linear day numbers of the respective 1sts in non-leap years. */

static int day_n[] = { 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 };
		  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */


extern struct timezone sys_tz;


/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). */

int date_dos2unix(unsigned short time,unsigned short date)
{
	int month,year,secs;

	/* first subtract and mask after that... Otherwise, if
	   date == 0, bad things happen */
	month = ((date >> 5) - 1) & 15;
	year = date >> 9;
	secs = (time & 31)*2+60*((time >> 5) & 63)+(time >> 11)*3600+86400*
	    ((date & 31)-1+day_n[month]+(year/4)+year*365-((year & 3) == 0 &&
	    month < 2 ? 1 : 0)+3653);
			/* days since 1.1.70 plus 80's leap day */
	secs += sys_tz.tz_minuteswest*60;
	return secs;
}


/* Convert linear UNIX date to a MS-DOS time/date pair. */

void fat_date_unix2dos(int unix_date,__le16 *time, __le16 *date)
{
	int day,year,nl_day,month;

	unix_date -= sys_tz.tz_minuteswest*60;

	/* Jan 1 GMT 00:00:00 1980. But what about another time zone? */
	if (unix_date < 315532800)
		unix_date = 315532800;

	*time = cpu_to_le16((unix_date % 60)/2+(((unix_date/60) % 60) << 5)+
	    (((unix_date/3600) % 24) << 11));
	day = unix_date/86400-3652;
	year = day/365;
	if ((year+3)/4+365*year > day) year--;
	day -= (year+3)/4+365*year;
	if (day == 59 && !(year & 3)) {
		nl_day = day;
		month = 2;
	}
	else {
		nl_day = (year & 3) || day <= 59 ? day : day-1;
		for (month = 0; month < 12; month++)
			if (day_n[month] > nl_day) break;
	}
	*date = cpu_to_le16(nl_day-day_n[month-1]+1+(month << 5)+(year << 9));
}


/* Returns the inode number of the directory entry at offset pos. If bh is
   non-NULL, it is brelse'd before. Pos is incremented. The buffer header is
   returned in bh.
   AV. Most often we do it item-by-item. Makes sense to optimize.
   AV. OK, there we go: if both bh and de are non-NULL we assume that we just
   AV. want the next entry (took one explicit de=NULL in vfat/namei.c).
   AV. It's done in fat_get_entry() (inlined), here the slow case lives.
   AV. Additionally, when we return -1 (i.e. reached the end of directory)
   AV. we make bh NULL. 
 */

int fat__get_entry(struct inode *dir, loff_t *pos,struct buffer_head **bh,
		   struct msdos_dir_entry **de, loff_t *i_pos)
{
	struct super_block *sb = dir->i_sb;
	struct msdos_sb_info *sbi = MSDOS_SB(sb);
	sector_t phys, iblock;
	loff_t offset;
	int err;

next:
	offset = *pos;
	if (*bh)
		brelse(*bh);

	*bh = NULL;
	iblock = *pos >> sb->s_blocksize_bits;
	err = fat_bmap(dir, iblock, &phys);
	if (err || !phys)
		return -1;	/* beyond EOF or error */

	*bh = sb_bread(sb, phys);
	if (*bh == NULL) {
		printk(KERN_ERR "FAT: Directory bread(block %llu) failed\n",
		       (unsigned long long)phys);
		/* skip this block */
		*pos = (iblock + 1) << sb->s_blocksize_bits;
		goto next;
	}

	offset &= sb->s_blocksize - 1;
	*pos += sizeof(struct msdos_dir_entry);
	*de = (struct msdos_dir_entry *)((*bh)->b_data + offset);
	*i_pos = ((loff_t)phys << sbi->dir_per_block_bits) + (offset >> MSDOS_DIR_BITS);

	return 0;
}

#if defined(MAGX_N_06_19_60I)

#define BDEVNAME_SIZE    32      /* Largest string for a blockdev identifier */
static char device_name[BDEVNAME_SIZE];
static volatile int panic_script_running=0;
spinlock_t panic_script_lock=SPIN_LOCK_UNLOCKED;

static void call_repairfat(char *dev_name)
{
#define NUM_ENVP 32
	int i=0,ret;
	char *argv[3];
	char **envp=NULL;
	char script_path[]="/etc/repairfat.sh";

	envp = kmalloc(NUM_ENVP * sizeof (char *), GFP_KERNEL);
	if (!envp)
		return;
	memset (envp, 0x00, NUM_ENVP * sizeof (char *));

				
	spin_lock(&panic_script_lock);
	if( !panic_script_running )
		panic_script_running = 1;
	else
	{
		spin_unlock(&panic_script_lock);
		kfree(envp);
		return;
	}

	spin_unlock(&panic_script_lock);


	argv[0]= script_path;
	argv[1]= dev_name;
	argv[2]= NULL;

        envp [i++] = "HOME=/";
	envp [i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
	//envp [i++] = 
	ret = call_usermodehelper(argv[0], argv, envp, 0 );
	if( ret )
                printk( "%s: call repairfat.sh returned %d \n", __FUNCTION__,  ret);

	spin_lock(&panic_script_lock);
	panic_script_running=0;
	spin_unlock(&panic_script_lock);
	kfree(envp);
}
static void wakeup_repairfat(struct super_block *sb)
{
	int part = 0;
	struct block_device *panic_bdev = sb->s_bdev;
	printk("%s: mxclay repairfat \n", __FUNCTION__);

	if(panic_bdev && panic_bdev->bd_disk && 
                        !(strncmp("mmc", panic_bdev->bd_disk->devfs_name, 3))) {
		printk("wmnbmh:repairfat %s\n", panic_bdev->bd_disk->devfs_name);
		part = MINOR(panic_bdev->bd_dev) - panic_bdev->bd_disk->first_minor;
		snprintf(device_name, BDEVNAME_SIZE, "/dev/%s/part%d", 
				panic_bdev->bd_disk->devfs_name, part);
		printk("wmnbmh:repairfat device= %s\n", device_name);
								
		call_repairfat(device_name);	
		/*complete(&fatpanic_completion);*/
	}

//08-12-2008   Motorola        Add simple repair FAT
#else //  #if defined(MAGX_N_06_19_60I)
static atomic_t script_running = ATOMIC_INIT(-1);
/* This variable is used to confirm that the repair script has been finished */
static int fat_repairing = 0;

static void wakeup_repairfat(struct super_block *sb)
{
        int ret, part = 0;
	char device_name[BDEVNAME_SIZE];
        struct block_device *panic_bdev = sb->s_bdev;
	char *argv[3];
	char *envp[] = { "HOME=/",
			 "PATH=/sbin:/bin:/usr/sbin:/usr/bin",
			 NULL };

	/* avoid too many invoked */
	if (!atomic_inc_and_test(&script_running)) {
		atomic_dec(&script_running);
		return;
	}

	/* the variable will be cleared in repair script by ioctl */
	if(fat_repairing)
		goto repair_back;

        printk("%s: will repairfat \n", __FUNCTION__);
	if(panic_bdev && panic_bdev->bd_disk) {
		printk("wmnbmh:repairfat %s\n", panic_bdev->bd_disk->devfs_name);

		if(!(strncmp("cyan", panic_bdev->bd_disk->devfs_name, 4))) {
			part = MINOR(panic_bdev->bd_dev) - panic_bdev->bd_disk->first_minor;
	                snprintf(device_name, BDEVNAME_SIZE, "/dev/%s/part%d",
        	               	        panic_bdev->bd_disk->devfs_name, part);
		}
		else if(!(strncmp("loop", panic_bdev->bd_disk->devfs_name, 4))) {
			snprintf(device_name, BDEVNAME_SIZE, "/dev/%s",
	                       	        panic_bdev->bd_disk->devfs_name);
		}
		else {
			printk("Couldn't repair this device %s\n", panic_bdev->bd_disk->devfs_name);
			goto repair_back;
		}
                printk("wmnbmh:repairfat device= %s\n", device_name);

		argv[0] = "/etc/repairfat.sh";
		argv[1] = device_name;
		argv[2] = NULL;

		printk("mxclay: begin to call usermodehelper %s %s \n", argv[0], argv[1]);
		fat_repairing = 1;
        	ret = call_usermodehelper(argv[0], argv, envp, 0 );
	        printk( "%s: call repairfat.sh returned %d \n", __FUNCTION__,  ret);
        }

repair_back:
	atomic_dec(&script_running);
}

static int repairfat_proc_read(struct file *file, char *buf, int count, int * pos)
{
        int len = 0;
        int index;
        char page[128];

        len = 0;
        index = (*pos)++;

        switch(index) {
        case 0:
                len += sprintf ((char *) page + len, "repairfat \n");
                break;
        case 1:
                len += sprintf ((char *) page + len, " 1 enable fat panic repair, 0 disable \n");
        }

        if ((len > 0) && copy_to_user (buf, (char *) page, len)) {
                len = -EFAULT;
        }
        return len;
}

static int repairfat_proc_write (struct file *file, char *buf, int count, int * pos)
{
        char c;
        printk("%s %c %c ",__FUNCTION__, buf[0],buf[1]);

        c = buf[0];
        switch(c) {
        case '0':
		fat_repairing = 1;
                break;
        case '1':
		fat_repairing = 0;
                break;
        default:
                printk("%s get wrong cmd:%c\n",__FUNCTION__, c);
        }

        return count;
}

/*
   To confirm only one repair script in running after fat panic.
   After exec repair application, the script will send "1" by ioctl funtion.
   We use proc technology to communicate between kernel and user space.
 */
static struct file_operations repairfat_proc_fops =
{
    .read    = repairfat_proc_read,
    .write   = repairfat_proc_write,
};
#define PROC_FAT_DIR_NAME             "fat_repair"
static struct proc_dir_entry *proc_repairfat_dir, *proc_repairfat_file;

int __init fat_misc_init(void)
{
	/* create directory */
	proc_repairfat_dir = proc_mkdir(PROC_FAT_DIR_NAME, NULL);
	if(proc_repairfat_dir == NULL) {
		printk("%s:couldn't mkdir:%s\n", __FUNCTION__, PROC_FAT_DIR_NAME);
		return -ENOMEM;
	}
	proc_repairfat_file = create_proc_entry("repairfat", 0644, proc_repairfat_dir);
	if(proc_repairfat_file == NULL) {
		printk("%s:couldn't make file:repairfat\n", __FUNCTION__);
		remove_proc_entry(PROC_FAT_DIR_NAME, NULL);
		return -ENOMEM;
	}
	proc_repairfat_file->proc_fops = (struct file_operations *)&repairfat_proc_fops;

	return 0;
}

void __exit fat_misc_destroy(void)
{
	remove_proc_entry("repairfat",proc_repairfat_dir);
	remove_proc_entry(PROC_FAT_DIR_NAME, NULL);
}

#endif //  #if defined(MAGX_N_06_19_60I) #if defined(CONFIG_MACH_PEARL)

