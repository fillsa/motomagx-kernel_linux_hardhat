/*
 *  linux/include/asm-arm/arch-pnx4008/dma.h
 *
 *  PNX4008 DMA header file
 *
 *  Author:	Vitaly Wool
 *  Copyright:	MontaVista Software Inc. (c) 2005
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

#include "platform.h"

#define MAX_DMA_ADDRESS		0xffffffff

#define MAX_DMA_CHANNELS	8

#define DMAC_BASE		IO_ADDRESS(PNX4008_DMA_CONFIG_BASE)
#define DMAC_INT_STAT		(DMAC_BASE + 0x0000)
#define DMAC_INT_TC_STAT	(DMAC_BASE + 0x0004)
#define DMAC_INT_TC_CLEAR	(DMAC_BASE + 0x0008)
#define DMAC_INT_ERR_STAT	(DMAC_BASE + 0x000c)
#define DMAC_INT_ERR_CLEAR	(DMAC_BASE + 0x0010)
#define DMAC_SOFT_SREQ		(DMAC_BASE + 0x0024)
#define DMAC_CONFIG		(DMAC_BASE + 0x0030)
#define DMAC_Cx_SRC_ADDR(c)	(DMAC_BASE + 0x0100 + (c) * 0x20)
#define DMAC_Cx_DEST_ADDR(c)	(DMAC_BASE + 0x0104 + (c) * 0x20)
#define DMAC_Cx_LLI(c)		(DMAC_BASE + 0x0108 + (c) * 0x20)
#define DMAC_Cx_CONTROL(c)	(DMAC_BASE + 0x010c + (c) * 0x20)
#define DMAC_Cx_CONFIG(c)	(DMAC_BASE + 0x0110 + (c) * 0x20)

enum {
	WIDTH_BYTE = 0,
	WIDTH_HWORD,
	WIDTH_WORD
};

enum {
	FC_MEM2MEM_DMA,
	FC_MEM2PER_DMA,
	FC_PER2MEM_DMA,
	FC_PER2PER_DMA,
	FC_PER2PER_DPER,
	FC_MEM2PER_PER,
	FC_PER2MEM_PER,
	FC_PER2PER_SPER
};

enum {
	DMA_INT_UNKNOWN = 0,
	DMA_ERR_INT = 1,
	DMA_TC_INT = 2,
};

enum {
	DMA_BUFFER_ALLOCATED = 1,
	DMA_HAS_LL = 2,
};

enum {
	PER_CAM_DMA_1 = 0,
	PER_NDF_FLASH = 1,
	PER_MBX_SLAVE_FIFO = 2,
	PER_SPI2_REC_XMIT = 3,
	PER_MS_SD_RX_XMIT = 4,
	PER_HS_UART_1_XMIT = 5,
	PER_HS_UART_1_RX = 6,
	PER_HS_UART_2_XMIT = 7,
	PER_HS_UART_2_RX = 8,
	PER_HS_UART_7_XMIT = 9,
	PER_HS_UART_7_RX = 10,
	PER_SPI1_REC_XMIT = 11,
	PER_MLC_NDF_SREC = 12,
	PER_CAM_DMA_2 = 13,
	PER_PRNG_INFIFO = 14,
	PER_PRNG_OUTFIFO = 15,
};

typedef struct pnx4008_dma_ch_ctrl {
	int tc_mask;
	int cacheable;
	int bufferable;
	int priv_mode;
	int di;
	int si;
	int dest_ahb1;
	int src_ahb1;
	int dwidth;
	int swidth;
	int dbsize;
	int sbsize;
	int tr_size;
} pnx4008_dma_ch_ctrl_t;

typedef struct pnx4008_dma_ch_config {
	int halt;
	int active;
	int lock;
	int itc;
	int ie;
	int flow_cntrl;
	int dest_per;
	int src_per;
} pnx4008_dma_ch_config_t;

typedef struct pnx4008_dma_ll {
	unsigned long src_addr;
	unsigned long dest_addr;
	u32 next_dma;
	unsigned long ch_ctrl;
	struct pnx4008_dma_ll *next;
	int flags;
	void *alloc_data;
	int (*free) (void *);
} pnx4008_dma_ll_t;

typedef struct pnx4008_dma_config {
	int is_ll;
	unsigned long src_addr;
	unsigned long dest_addr;
	unsigned long ch_ctrl;
	unsigned long ch_cfg;
	pnx4008_dma_ll_t *ll;
	u32 ll_dma;
	int flags;
	void *alloc_data;
	int (*free) (void *);
} pnx4008_dma_config_t;

extern pnx4008_dma_ll_t *pnx4008_alloc_ll_entry(dma_addr_t *);
extern void pnx4008_free_ll_entry(pnx4008_dma_ll_t *, dma_addr_t);
extern void pnx4008_free_ll(u32 ll_dma, pnx4008_dma_ll_t *);

extern int pnx4008_request_channel(char *, int,
				   void (*)(int, int, void *, struct pt_regs *),
				   void *);
extern void pnx4008_free_channel(int);
extern int pnx4008_config_dma(int, int, int);
extern int pnx4008_dma_pack_control(const pnx4008_dma_ch_ctrl_t *,
				    unsigned long *);
extern int pnx4008_dma_parse_control(unsigned long, pnx4008_dma_ch_ctrl_t *);
extern int pnx4008_dma_pack_config(const pnx4008_dma_ch_config_t *,
				   unsigned long *);
extern int pnx4008_dma_parse_config(unsigned long, pnx4008_dma_ch_config_t *);
extern int pnx4008_config_channel(int, pnx4008_dma_config_t *);
extern int pnx4008_channel_get_config(int, pnx4008_dma_config_t *);
extern int pnx4008_dma_ch_enable(int);
extern int pnx4008_dma_ch_disable(int);
extern int pnx4008_dma_ch_enabled(int);
extern void pnx4008_dma_split_head_entry(pnx4008_dma_config_t *,
					 pnx4008_dma_ch_ctrl_t *);
extern void pnx4008_dma_split_ll_entry(pnx4008_dma_ll_t *,
				       pnx4008_dma_ch_ctrl_t *);

#endif				/* _ASM_ARCH_DMA_H */
