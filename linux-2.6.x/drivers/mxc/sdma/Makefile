ifneq ($(KBUILD_SRC),)
CFLAGS	+= -I$(KBUILD_SRC)/drivers/mxc/sdma/iapi/include \
		-I$(KBUILD_SRC)/include/linux -DMCU -DOS=LINUX \
		-DL_I_T_T_L_E_ENDIAN=0 -DB_I_G_ENDIAN=1 \
		-DENDIANNESS=L_I_T_T_L_E_ENDIAN
else
CFLAGS	+= -Idrivers/mxc/sdma/iapi/include \
		-Iinclude/linux -DMCU -DOS=LINUX -DL_I_T_T_L_E_ENDIAN=0 \
		-DB_I_G_ENDIAN=1 -DENDIANNESS=L_I_T_T_L_E_ENDIAN
endif

obj-$(CONFIG_MXC_SDMA_API)		+= sdma.o
obj-$(CONFIG_MXC_SDMA_API)		+= iapi/
obj-$(CONFIG_MXC_SDMA_API)		+= sdma_malloc.o
