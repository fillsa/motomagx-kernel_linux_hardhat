#
# Makefile for the linux pram-filesystem routines.
#

obj-$(CONFIG_PRAMFS) += pramfs.o

pramfs-objs := balloc.o dir.o file.o fsync.o inode.o ioctl.o namei.o \
		super.o symlink.o
ifndef CONFIG_PRAMFS_NOWP
pramfs-objs += wprotect.o
endif
