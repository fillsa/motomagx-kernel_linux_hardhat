/*
 * fs/proc/task_mmu.c
 *
 * Copyright 2007 Motorola, Inc
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Date         Author          Comment
 * ===========  ==============  ==============================================
 * 05-23-2007   Motorola   	Fix the deadlock issue when process access 
 *                              /proc/[pid]/memmap.
 */

#include <linux/mm.h>
#include <linux/hugetlb.h>
#include <linux/seq_file.h>
#include <asm/elf.h>
#include <asm/uaccess.h>

extern void count_physical_pages(struct mm_struct *, struct vm_area_struct *,
				 signed char *, int);


char *task_mem(struct mm_struct *mm, char *buffer)
{
	unsigned long data, text, lib;

	data = mm->total_vm - mm->shared_vm - mm->stack_vm;
	text = (PAGE_ALIGN(mm->end_code) - (mm->start_code & PAGE_MASK)) >> 10;
	lib = (mm->exec_vm << (PAGE_SHIFT-10)) - text;
	buffer += sprintf(buffer,
		"VmSize:\t%8lu kB\n"
		"VmLck:\t%8lu kB\n"
		"VmRSS:\t%8lu kB\n"
		"VmData:\t%8lu kB\n"
		"VmStk:\t%8lu kB\n"
		"VmExe:\t%8lu kB\n"
		"VmLib:\t%8lu kB\n"
		"VmPTE:\t%8lu kB\n",
		(mm->total_vm - mm->reserved_vm) << (PAGE_SHIFT-10),
		mm->locked_vm << (PAGE_SHIFT-10),
		mm->rss << (PAGE_SHIFT-10),
		data << (PAGE_SHIFT-10),
		mm->stack_vm << (PAGE_SHIFT-10), text, lib,
		(PTRS_PER_PTE*sizeof(pte_t)*mm->nr_ptes) >> 10);
	return buffer;
}

unsigned long task_vsize(struct mm_struct *mm)
{
	return PAGE_SIZE * mm->total_vm;
}

int task_statm(struct mm_struct *mm, int *shared, int *text,
	       int *data, int *resident)
{
	*shared = mm->rss - mm->anon_rss;
	*text = (PAGE_ALIGN(mm->end_code) - (mm->start_code & PAGE_MASK))
								>> PAGE_SHIFT;
	*data = mm->total_vm - mm->shared_vm;
	*resident = mm->rss;
	return mm->total_vm;
}

static int show_map(struct seq_file *m, void *v)
{
	struct vm_area_struct *map = v;
	struct file *file = map->vm_file;
	int flags = map->vm_flags;
	unsigned long ino = 0;
	dev_t dev = 0;
	int len;

	if (file) {
		struct inode *inode = map->vm_file->f_dentry->d_inode;
		dev = inode->i_sb->s_dev;
		ino = inode->i_ino;
	}

	seq_printf(m, "%08lx-%08lx %c%c%c%c %08lx %02x:%02x %lu %n",
			map->vm_start,
			map->vm_end,
			flags & VM_READ ? 'r' : '-',
			flags & VM_WRITE ? 'w' : '-',
			flags & VM_EXEC ? 'x' : '-',
			flags & VM_MAYSHARE ? 's' : 'p',
			map->vm_pgoff << PAGE_SHIFT,
			MAJOR(dev), MINOR(dev), ino, &len);

	if (map->vm_file) {
		len = 25 + sizeof(void*) * 6 - len;
		if (len < 1)
			len = 1;
		seq_printf(m, "%*c", len, ' ');
		seq_path(m, file->f_vfsmnt, file->f_dentry, "");
	}
	seq_putc(m, '\n');
	return 0;
}

static int show_mem_node_map(struct seq_file *m,
			     struct vm_area_struct * map,
			     int node_map)
{
	struct task_struct *task = m->private;
	struct mm_struct *mm;
	char * memmapbuf;
	signed char *data_buf;
	long retval = 0;
	int index, pages=0, len=0;
	
	pages = (map->vm_end - map->vm_start) / PAGE_SIZE;
	
	retval = -ENOMEM;
	if ((data_buf = (signed char *)kmalloc(pages, GFP_KERNEL)) == NULL) {
		return retval;
	}
	for (index = 0; index < pages; index++) {
		data_buf[index] = -1;
	}
	if ((memmapbuf = (char *)kmalloc(2*pages+1, GFP_KERNEL)) == NULL)
		goto out_free_databuf;
	
	task_lock(task);
	mm = task->mm;
	if (mm)
		atomic_inc(&mm->mm_users);
	task_unlock(task);
	retval = 0;
	if (!mm)
		goto out_free_mmb;
	
#ifndef CONFIG_MOT_WFN493
	down_read(&mm->mmap_sem);
#endif
	retval = 0;
	
	count_physical_pages(map->vm_mm, map, data_buf, node_map);
	
	for (index = 0; index < pages; index++) {
		int val = data_buf[index];
		if (val < 0)
			len += sprintf(&memmapbuf[len],node_map ? " -" : " 0");
		else
			len += sprintf(&memmapbuf[len], "%2d", val);
		if (index == pages-1)
			len += sprintf(&memmapbuf[len], "\n");
	}
	
	seq_printf(m, memmapbuf);
#ifndef CONFIG_MOT_WFN493
	up_read(&mm->mmap_sem);
#endif
 out_free_mmb:
	mmput(mm);
	kfree(memmapbuf);
 out_free_databuf:
	kfree(data_buf);
	return retval;
}

static int show_memmap(struct seq_file *m, void *v)
{
	return show_mem_node_map(m, (struct vm_area_struct *)v, 0);
}

static int show_nodemap(struct seq_file *m, void *v)
{
	return show_mem_node_map(m, (struct vm_area_struct *)v, 1);
}

static void *m_start(struct seq_file *m, loff_t *pos)
{
	struct task_struct *task = m->private;
	struct mm_struct *mm = get_task_mm(task);
	struct vm_area_struct * map;
	loff_t l = *pos;

	if (!mm)
		return NULL;

	down_read(&mm->mmap_sem);
	map = mm->mmap;
	while (l-- && map) {
		map = map->vm_next;
		cond_resched();
	}
	if (!map) {
		up_read(&mm->mmap_sem);
		mmput(mm);
		if (l == -1)
			map = get_gate_vma(task);
	}
	return map;
}

static void m_stop(struct seq_file *m, void *v)
{
	struct task_struct *task = m->private;
	struct vm_area_struct *map = v;
	if (map && map != get_gate_vma(task)) {
		struct mm_struct *mm = map->vm_mm;
		up_read(&mm->mmap_sem);
		mmput(mm);
	}
}

static void *m_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct task_struct *task = m->private;
	struct vm_area_struct *map = v;
	(*pos)++;
	if (map->vm_next)
		return map->vm_next;
	m_stop(m, v);
	if (map != get_gate_vma(task))
		return get_gate_vma(task);
	return NULL;
}

struct seq_operations proc_pid_maps_op = {
	.start	= m_start,
	.next	= m_next,
	.stop	= m_stop,
	.show	= show_map
};

struct seq_operations proc_pid_memmap_op = {
	.start  = m_start,
	.next   = m_next,
	.stop   = m_stop,
	.show   = show_memmap
};

struct seq_operations proc_pid_nodemap_op = {
	.start  = m_start,
	.next   = m_next,
	.stop   = m_stop,
	.show   = show_nodemap
};
