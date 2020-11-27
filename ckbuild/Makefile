# This file is derived from the Linux kernel's toplevel Makefile.
#
# Copyright 2006 Motorola
#
# Motorola  2006-Nov-13 - Adapted for clearmake-compliant build of external
#                         modules.

# CKBUILD : perform sanity checks
ifeq ($(KBUILD_SRC),)
CKBUILD_MISSING += KBUILD_SRC
endif
ifeq ($(MODLIB),)
CKBUILD_MISSING += MODLIB
endif
ifeq ($(CKBUILD_SRC),)
CKBUILD_MISSING += CKBUILD_SRC
endif

# CKBUILD : if any variables are missing, bail out
ifneq ($(CKBUILD_MISSING),)
default %:
	@echo The following variables need to be set : $(CKBUILD_MISSING)
	exit 1
else

export CKBUILD_SRC
cmif := $(CKBUILD_SRC)/scripts/if.sh

# Call sparse as part of compilation of C files
# Use 'make C=1' to enable sparse checking

ifdef C
  ifeq ("$(origin C)", "command line")
    KBUILD_CHECKSRC = $(C)
  endif
endif
ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

# Use make M=dir to specify directory of external module to build
# Old syntax make ... SUBDIRS=$PWD is still supported
# Setting the environment variable KBUILD_EXTMOD take precedence
ifdef SUBDIRS
  KBUILD_EXTMOD ?= $(SUBDIRS)
endif
ifdef M
  ifeq ("$(origin M)", "command line")
    KBUILD_EXTMOD := $(M)
  endif
endif

srctree         := $(KBUILD_SRC)
objtree         := $(CURDIR)

export srctree objtree

# We are only building modules
KBUILD_MODULES := 1
KBUILD_BUILTIN :=

export KBUILD_MODULES KBUILD_BUILTIN
export KBUILD_CHECKSRC KBUILD_SRC KBUILD_EXTMOD

######
# cc support functions to be used (only) in arch/$(ARCH)/Makefile
# See documentation in Documentation/kbuild/makefiles.txt

# cc-option
# Usage: cflags-y += $(shell $(cc-option) "-march=winchip-c6" "-march=i586")

cc-option = $(CKBUILD_SRC)/scripts/cc-option.sh "$(CC)" "$(CFLAGS)"

# cc-option-yn
# Usage: flag := $(shell $(cc-option-yn) "-march=winchip-c6")
cc-option-yn = $(CKBUILD_SRC)/scripts/cc-option-yn.sh "$(CC)" "$(CFLAGS)"

# cc-option-align
# Prefix align with either -falign or -malign
cc-option-align = $(subst -functions=0,,\
	$(shell $(cc-option) -falign-functions=0 -malign-functions=0))


## FROM KERNEL MAKEFILE
AS              = $(CROSS_COMPILE)as
LD              = $(CROSS_COMPILE)ld
CC              = $(CROSS_COMPILE)gcc
CPP             = $(CC) -E
AR              = $(CROSS_COMPILE)ar
NM              = $(CROSS_COMPILE)nm
STRIP           = $(CROSS_COMPILE)strip
OBJCOPY         = $(CROSS_COMPILE)objcopy
OBJDUMP         = $(CROSS_COMPILE)objdump
AWK             = awk
GENKSYMS        = scripts/genksyms/genksyms
DEPMOD          = $(TOOLDIR)depmod
KALLSYMS        = scripts/kallsyms
PERL            = perl
CHECK           = sparse
CHECKFLAGS     := -D__linux__ -Dlinux -D__STDC__ -Dunix -D__unix__
MODFLAGS        = -DMODULE
CFLAGS_MODULE   = $(MODFLAGS)
AFLAGS_MODULE   = $(MODFLAGS)
LDFLAGS_MODULE  = -r
CFLAGS_KERNEL   =
AFLAGS_KERNEL   =

NOSTDINC_FLAGS  = -nostdinc -iwithprefix include

LINUXINCLUDE    := -Iinclude -Iinclude2 -I$(srctree)/include

CPPFLAGS        := -D__KERNEL__ $(LINUXINCLUDE)

CFLAGS          := -Wall -Wstrict-prototypes -Wno-trigraphs \
                   -fno-strict-aliasing -fno-common
AFLAGS          := -D__ASSEMBLY__

export  VERSION PATCHLEVEL SUBLEVEL EXTRAVERSION LOCALVERSION KERNELRELEASE \
        ARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC \
        CPP AR NM STRIP OBJCOPY OBJDUMP MAKE AWK GENKSYMS PERL UTS_MACHINE \
        HOSTCXX HOSTCXXFLAGS LDFLAGS_MODULE CHECK CHECKFLAGS

export CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS LDFLAGS
export CFLAGS CFLAGS_KERNEL CFLAGS_MODULE
export AFLAGS AFLAGS_KERNEL AFLAGS_MODULE

# When compiling out-of-tree modules, put MODVERDIR in the module
# tree rather than in the kernel tree. The kernel tree might
# even be read-only.
export MODVERDIR := $(firstword $(KBUILD_EXTMOD))/.tmp_versions

export MODLIB

# In this section, we need .config
include .config

ifdef CONFIG_CC_OPTIMIZE_FOR_SIZE
CFLAGS          += -Os
else
CFLAGS          += -O2
endif

#Add align options if CONFIG_CC_* is not equal to 0
CFLAGS          += $(shell $(cmif) "$(filter-out 0,$(CONFIG_CC_ALIGN_FUNCTIONS))" "$(cc-option-align)-functions=$(CONFIG_CC_ALIGN_FUNCTIONS)")
CFLAGS          += $(shell $(cmif) "$(filter-out 0,$(CONFIG_CC_ALIGN_LABELS))" "$(cc-option-align)-labels=$(CONFIG_CC_ALIGN_LABELS)")
CFLAGS          += $(shell $(cmif) "$(filter-out 0,$(CONFIG_CC_ALIGN_LOOPS))" "$(cc-option-align)-loops=$(CONFIG_CC_ALIGN_LOOPS)")
CFLAGS          += $(shell $(cmif) "$(filter-out 0,$(CONFIG_CC_ALIGN_JUMPS))" "$(cc-option-align)-jumps=$(CONFIG_CC_ALIGN_JUMPS)")

ifdef CONFIG_MCOUNT
CFLAGS          += -pg -fno-omit-frame-pointer
else
 ifdef CONFIG_FRAME_POINTER
 CFLAGS         += -fno-omit-frame-pointer
 else
 CFLAGS         += -fomit-frame-pointer
 endif
endif

ifdef CONFIG_KFI
CFLAGS += -finstrument-functions
endif

ifdef CONFIG_DEBUG_INFO
CFLAGS          += -g
endif

include $(CKBUILD_SRC)/arch/$(ARCH)/Makefile

# warn about C99 declaration after statement
CFLAGS += $(shell $(cc-option) -Wdeclaration-after-statement)


# Shorthand for $(Q)$(MAKE) -f scripts/Makefile.build obj=dir
# Usage:
# $(Q)$(MAKE) $(build)=dir
build := -f $(CKBUILD_SRC)/scripts/Makefile.build obj

.PHONY: crmodverdir
crmodverdir:
	$(Q)mkdir -p $(MODVERDIR)

module-dirs := $(addprefix _module_,$(KBUILD_EXTMOD))
.PHONY: $(module-dirs) modules
$(module-dirs): crmodverdir
	$(Q)$(MAKE) $(build)=$(patsubst _module_%,%,$@)

modules: $(module-dirs)
	@echo '  Building modules, stage 2.';
	$(Q)$(MAKE) -f $(CKBUILD_SRC)/scripts/Makefile.modpost

.PHONY: modules_install
modules_install:
	$(Q)$(MAKE) -rR -f $(CKBUILD_SRC)/scripts/Makefile.modinst

endif
