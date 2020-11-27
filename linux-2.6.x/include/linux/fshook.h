/*
 *  include/linux/fshook.h
 *
 *  Copyright 2005 McAfee, Inc. All rights reserved.  All rights reserved.
 *
 *  File content inspection hooking
 */

#ifndef _LINUX_FSHOOK_H
#define _LINUX_FSHOOK_H


#define FSHOOK_MINOR	123

struct fsh_token {
	int fst_len;
	char *fst_data;
};

struct fsh_event {
	unsigned long fse_id;
	struct fsh_token fse_name;
	long fse_timeout;
};

struct fsh_release {
	unsigned long fsr_id;
	int fsr_status;
};


#define FSH_S_CACHE_SIZE	_IOR('F', 1, int)
#define FSH_START		_IO('F', 2)
#define FSH_STOP		_IO('F', 3)
#define FSH_CACHE_CLEAR		_IO('F', 4)
#define FSH_EVENT		_IOWR('F', 5, struct fsh_event)
#define FSH_RELEASE		_IOR('F', 6, struct fsh_release)
#define FSH_EXNAME_ADD		_IOR('F', 7, char *)
#define FSH_TASK_ATTACH		_IOR('F', 8, int)
#define FSH_DISABLE_PID		_IOR('F', 9, int)
#define FSH_ENABLE_PID		_IOR('F', 10, int)
#define FSH_CACHE_EXNAME_ADD	_IOR('F', 11, char *)


#ifdef __KERNEL__

int fsh_task_detach(struct task_struct *tsk);
int fsh_check_file(const char *filename, int kname, int flags);

asmlinkage long fsh_sys_rename(const char *oname, const char *nname);
asmlinkage long fsh_sys_unlink(const char *filename);
asmlinkage long fsh_sys_open(const char *filename, int kname, int flags, int mode);
asmlinkage long fsh_sys_close(int fd);

#endif

#endif

