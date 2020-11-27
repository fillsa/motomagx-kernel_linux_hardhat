#
# Makefile for the linux kernel.
#           Copyright Motorola 2005-2006
#
# Revision History:
#
# Date         Author    Comment
# ----------   --------  -------------------
# 11/16/2006   Motorola  Hardware Config Framework



AFLAGS_head.o := -DTEXTADDR=$(TEXTADDR) -DDATAADDR=$(DATAADDR)

# Object file lists.

obj-y		:= arch.o compat.o  entry-armv.o entry-common.o irq.o   \
		   process.o ptrace.o setup.o signal.o sys_arm.o \
		   time.o traps.o

ifndef CONFIG_ARCH_ARGON
obj-y		+= dma.o
endif

obj-$(CONFIG_ASM_SEMAPHORES)    += semaphore.o
obj-$(CONFIG_APM)		+= apm.o
obj-$(CONFIG_ARCH_ACORN)	+= ecard.o 
obj-$(CONFIG_FOOTBRIDGE)	+= isa.o
obj-$(CONFIG_FIQ)		+= fiq.o
obj-$(CONFIG_MODULES)		+= armksyms.o module.o
obj-$(CONFIG_ARTHUR)		+= arthur.o
obj-$(CONFIG_ISA_DMA)		+= dma-isa.o
obj-$(CONFIG_PCI)		+= bios32.o
obj-$(CONFIG_SMP)		+= smp.o
obj-$(CONFIG_OABI_COMPAT)	+= sys_oabi-compat.o
obj-$(CONFIG_KGDB)		+= kgdb.o kgdb-jmp.o
obj-$(CONFIG_VST)		+= vst.o
obj-$(CONFIG_MOT_FEAT_BRDREV)	+= boardrev.o
obj-$(CONFIG_MOT_FEAT_BOOTINFO)	+= bootinfo.o
obj-$(CONFIG_MOT_FEAT_DEVICE_TREE)	+= mothwcfg.o motflattree.o
obj-$(CONFIG_MOT_FEAT_KPANIC)	+= kpanic.o
obj-$(CONFIG_MOT_FEAT_FX2LP_I2C) += fx2lp_i2c.o
obj-$(CONFIG_MOT_FEAT_DEBUG_WDOG) += fiq.o

obj-$(CONFIG_IWMMXT)		+= iwmmxt.o
AFLAGS_iwmmxt.o			:= -Wa,-mcpu=iwmmxt

ifneq ($(CONFIG_ARCH_EBSA110),y)
  obj-y		+= io.o
endif

head-y			:= head.o
obj-$(CONFIG_DEBUG_LL)	+= debug.o

extra-y := $(head-y) init_task.o vmlinux.lds

# Spell out some dependencies that aren't automatically figured out
$(obj)/entry-armv.o: 	$(obj)/entry-header.S include/asm-arm/constants.h
$(obj)/entry-common.o: 	$(obj)/entry-header.S include/asm-arm/constants.h \
			$(obj)/calls.S
