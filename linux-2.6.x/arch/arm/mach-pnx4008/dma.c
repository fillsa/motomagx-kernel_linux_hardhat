/*
 *  linux/arch/arm/mach-pnx4008/dma.c
 *
 *  PNX4008 DMA registration and IRQ dispatching
 *
 *  Author:	Vitaly Wool
 *  Copyright:	MontaVista Software Inc. (c) 2005
 *
 *  Based on the code from Nicolas Pitre
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>

#include <asm/system.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/dma.h>
#include <asm/dma-mapping.h>
#include <asm/io.h>
#include <asm/mach/dma.h>
#include <asm/hardware/clock.h>

static struct dma_channel {
	char *name;
	void (*irq_handler) (int, int, void *, struct pt_regs *);
	void *data;
	pnx4008_dma_ll_t *ll;
	u32 ll_dma;
	void *target_addr;
	int target_id;
} dma_channels[MAX_DMA_CHANNELS];

static struct ll_pool {
	void *vaddr;
	void *cur;
	dma_addr_t dma_addr;
	int count;
} ll_pool;

static raw_spinlock_t ll_lock = RAW_SPIN_LOCK_UNLOCKED;

pnx4008_dma_ll_t *pnx4008_alloc_ll_entry(dma_addr_t * ll_dma)
{
	pnx4008_dma_ll_t *ll = NULL;
	unsigned long flags;

	spin_lock_irqsave(&ll_lock, flags);
	if (ll_pool.count > 4) {
		ll = *(pnx4008_dma_ll_t **) ll_pool.cur;
		*ll_dma = ll_pool.dma_addr + ((void *)ll - ll_pool.vaddr);
		*(void **)ll_pool.cur = **(void ***)ll_pool.cur;
		memset(ll, 0, sizeof(*ll));
		ll_pool.count--;
	}
	spin_unlock_irqrestore(&ll_lock, flags);

	return ll;
}

EXPORT_SYMBOL_GPL(pnx4008_alloc_ll_entry);

void pnx4008_free_ll_entry(pnx4008_dma_ll_t * ll, dma_addr_t ll_dma)
{
	unsigned long flags;

	if (ll) {
		if ((unsigned long)((long)ll - (long)ll_pool.vaddr) > 0x4000) {
			printk(KERN_ERR "Trying to free entry not allocated by DMA\n");
			BUG();
		}

		if (ll->flags & DMA_BUFFER_ALLOCATED)
			ll->free(ll->alloc_data);

		spin_lock_irqsave(&ll_lock, flags);
		*(long *)ll = *(long *)ll_pool.cur;
		*(long *)ll_pool.cur = (long)ll;
		ll_pool.count++;
		spin_unlock_irqrestore(&ll_lock, flags);
	}
}

EXPORT_SYMBOL_GPL(pnx4008_free_ll_entry);

void pnx4008_free_ll(u32 ll_dma, pnx4008_dma_ll_t * ll)
{
	pnx4008_dma_ll_t *ptr;
	u32 dma;

	while (ll) {
		dma = ll->next_dma;
		ptr = ll->next;
		pnx4008_free_ll_entry(ll, ll_dma);

		ll_dma = dma;
		ll = ptr;
	}
}

EXPORT_SYMBOL_GPL(pnx4008_free_ll);

static int dma_channels_requested = 0;

static inline void dma_increment_usage(void)
{
	if (!dma_channels_requested++) {
		struct clk *clk = clk_get(0, "dma_ck");
		if (!IS_ERR(clk)) {
			clk_set_rate(clk, 1);
			clk_put(clk);
		}
		pnx4008_config_dma(-1, -1, 1);
	}
}
static inline void dma_decrement_usage(void)
{
	if (!--dma_channels_requested) {
		struct clk *clk = clk_get(0, "dma_ck");
		if (!IS_ERR(clk)) {
			clk_set_rate(clk, 0);
			clk_put(clk);
		}
		pnx4008_config_dma(-1, -1, 0);

	}
}

static raw_spinlock_t dma_lock = RAW_SPIN_LOCK_UNLOCKED;

static inline void pnx4008_dma_lock(void)
{
	spin_lock_irq(&dma_lock);
}

static inline void pnx4008_dma_unlock(void)
{
	spin_unlock_irq(&dma_lock);
}

#define VALID_CHANNEL(c)	(((c) >= 0) && ((c) < MAX_DMA_CHANNELS))

int pnx4008_request_channel(char *name, int ch,
			    void (*irq_handler) (int, int, void *,
						 struct pt_regs *), void *data)
{
	int i, found = 0;

	/* basic sanity checks */
	if (!name || (ch != -1 && !VALID_CHANNEL(ch)))
		return -EINVAL;

	pnx4008_dma_lock();

	/* try grabbing a DMA channel with the requested priority */
	for (i = MAX_DMA_CHANNELS - 1; i >= 0; i--) {
		if (!dma_channels[i].name && (ch == -1 || ch == i)) {
			found = 1;
			break;
		}
	}

	if (found) {
		dma_increment_usage();
		dma_channels[i].name = name;
		dma_channels[i].irq_handler = irq_handler;
		dma_channels[i].data = data;
		dma_channels[i].ll = NULL;
		dma_channels[i].ll_dma = 0;
	} else {
		printk(KERN_WARNING "No more available DMA channels for %s\n",
		       name);
		i = -ENODEV;
	}

	pnx4008_dma_unlock();
	return i;
}

EXPORT_SYMBOL_GPL(pnx4008_request_channel);

void pnx4008_free_channel(int ch)
{
	if (!dma_channels[ch].name) {
		printk(KERN_CRIT
		       "%s: trying to free channel %d which is already freed\n",
		       __FUNCTION__, ch);
		return;
	}

	pnx4008_dma_lock();
	pnx4008_free_ll(dma_channels[ch].ll_dma, dma_channels[ch].ll);
	dma_channels[ch].ll = NULL;
	dma_decrement_usage();

	dma_channels[ch].name = NULL;
	pnx4008_dma_unlock();
}

EXPORT_SYMBOL_GPL(pnx4008_free_channel);

int pnx4008_config_dma(int ahb_m1_be, int ahb_m2_be, int enable)
{
	unsigned long dma_cfg = __raw_readl(DMAC_CONFIG);

	switch (ahb_m1_be) {
	case 0:
		dma_cfg &= ~(1 << 1);
		break;
	case 1:
		dma_cfg |= (1 << 1);
		break;
	default:
		break;
	}

	switch (ahb_m2_be) {
	case 0:
		dma_cfg &= ~(1 << 2);
		break;
	case 1:
		dma_cfg |= (1 << 2);
		break;
	default:
		break;
	}

	switch (enable) {
	case 0:
		dma_cfg &= ~(1 << 0);
		break;
	case 1:
		dma_cfg |= (1 << 0);
		break;
	default:
		break;
	}

	pnx4008_dma_lock();
	__raw_writel(dma_cfg, DMAC_CONFIG);
	pnx4008_dma_unlock();

	return 0;
}

EXPORT_SYMBOL_GPL(pnx4008_config_dma);

int pnx4008_dma_pack_control(const pnx4008_dma_ch_ctrl_t * ch_ctrl,
			     unsigned long *ctrl)
{
	int i = 0, dbsize, sbsize, err = 0;

	if (!ctrl || !ch_ctrl) {
		err = -EINVAL;
		goto out;
	}

	*ctrl = 0;

	switch (ch_ctrl->tc_mask) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 31);
		break;

	default:
		err = -EINVAL;
		goto out;
	}

	switch (ch_ctrl->cacheable) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 30);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->bufferable) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 29);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->priv_mode) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 28);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->di) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 27);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->si) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 26);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->dest_ahb1) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 25);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->src_ahb1) {
	case 0:
		break;
	case 1:
		*ctrl |= (1 << 24);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->dwidth) {
	case WIDTH_BYTE:
		*ctrl &= ~(7 << 21);
		break;
	case WIDTH_HWORD:
		*ctrl &= ~(7 << 21);
		*ctrl |= (1 << 21);
		break;
	case WIDTH_WORD:
		*ctrl &= ~(7 << 21);
		*ctrl |= (2 << 21);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_ctrl->swidth) {
	case WIDTH_BYTE:
		*ctrl &= ~(7 << 18);
		break;
	case WIDTH_HWORD:
		*ctrl &= ~(7 << 18);
		*ctrl |= (1 << 18);
		break;
	case WIDTH_WORD:
		*ctrl &= ~(7 << 18);
		*ctrl |= (2 << 18);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	dbsize = ch_ctrl->dbsize;
	while (!(dbsize & 1)) {
		i++;
		dbsize >>= 1;
	}
	if (ch_ctrl->dbsize != 1 || i > 8 || i == 1) {
		err = -EINVAL;
		goto out;
	} else if (i > 1)
		i--;
	*ctrl &= ~(7 << 15);
	*ctrl |= (i << 15);

	sbsize = ch_ctrl->sbsize;
	while (!(sbsize & 1)) {
		i++;
		sbsize >>= 1;
	}
	if (ch_ctrl->sbsize != 1 || i > 8 || i == 1) {
		err = -EINVAL;
		goto out;
	} else if (i > 1)
		i--;
	*ctrl &= ~(7 << 12);
	*ctrl |= (i << 12);

	if (ch_ctrl->tr_size > 0x7ff) {
		err = -E2BIG;
		goto out;
	}
	*ctrl &= ~0x7ff;
	*ctrl |= ch_ctrl->tr_size & 0x7ff;

      out:
	return err;
}

EXPORT_SYMBOL_GPL(pnx4008_dma_pack_control);

int pnx4008_dma_parse_control(unsigned long ctrl,
			      pnx4008_dma_ch_ctrl_t * ch_ctrl)
{
	int err = 0;

	if (!ch_ctrl) {
		err = -EINVAL;
		goto out;
	}

	ch_ctrl->tr_size = ctrl & 0x7ff;
	ctrl >>= 12;

	ch_ctrl->sbsize = 1 << (ctrl & 7);
	if (ch_ctrl->sbsize > 1)
		ch_ctrl->sbsize <<= 1;
	ctrl >>= 3;

	ch_ctrl->dbsize = 1 << (ctrl & 7);
	if (ch_ctrl->dbsize > 1)
		ch_ctrl->dbsize <<= 1;
	ctrl >>= 3;

	switch (ctrl & 7) {
	case 0:
		ch_ctrl->swidth = WIDTH_BYTE;
		break;
	case 1:
		ch_ctrl->swidth = WIDTH_HWORD;
		break;
	case 2:
		ch_ctrl->swidth = WIDTH_WORD;
		break;
	default:
		err = -EINVAL;
		goto out;
	}
	ctrl >>= 3;

	switch (ctrl & 7) {
	case 0:
		ch_ctrl->dwidth = WIDTH_BYTE;
		break;
	case 1:
		ch_ctrl->dwidth = WIDTH_HWORD;
		break;
	case 2:
		ch_ctrl->dwidth = WIDTH_WORD;
		break;
	default:
		err = -EINVAL;
		goto out;
	}
	ctrl >>= 3;

	ch_ctrl->src_ahb1 = ctrl & 1;
	ctrl >>= 1;

	ch_ctrl->dest_ahb1 = ctrl & 1;
	ctrl >>= 1;

	ch_ctrl->si = ctrl & 1;
	ctrl >>= 1;

	ch_ctrl->di = ctrl & 1;
	ctrl >>= 1;

	ch_ctrl->priv_mode = ctrl & 1;
	ctrl >>= 1;

	ch_ctrl->bufferable = ctrl & 1;
	ctrl >>= 1;

	ch_ctrl->cacheable = ctrl & 1;
	ctrl >>= 1;

	ch_ctrl->tc_mask = ctrl & 1;

      out:
	return err;
}

EXPORT_SYMBOL_GPL(pnx4008_dma_parse_control);

int pnx4008_dma_pack_config(const pnx4008_dma_ch_config_t * ch_cfg,
			    unsigned long *cfg)
{
	int err = 0;

	if (!cfg || !ch_cfg) {
		err = -EINVAL;
		goto out;
	}

	*cfg = 0;

	switch (ch_cfg->halt) {
	case 0:
		break;
	case 1:
		*cfg |= (1 << 18);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_cfg->active) {
	case 0:
		break;
	case 1:
		*cfg |= (1 << 17);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_cfg->lock) {
	case 0:
		break;
	case 1:
		*cfg |= (1 << 16);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_cfg->itc) {
	case 0:
		break;
	case 1:
		*cfg |= (1 << 15);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_cfg->ie) {
	case 0:
		break;
	case 1:
		*cfg |= (1 << 14);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	switch (ch_cfg->flow_cntrl) {
	case FC_MEM2MEM_DMA:
		*cfg &= ~(7 << 11);
		break;
	case FC_MEM2PER_DMA:
		*cfg &= ~(7 << 11);
		*cfg |= (1 << 11);
		break;
	case FC_PER2MEM_DMA:
		*cfg &= ~(7 << 11);
		*cfg |= (2 << 11);
		break;
	case FC_PER2PER_DMA:
		*cfg &= ~(7 << 11);
		*cfg |= (3 << 11);
		break;
	case FC_PER2PER_DPER:
		*cfg &= ~(7 << 11);
		*cfg |= (4 << 11);
		break;
	case FC_MEM2PER_PER:
		*cfg &= ~(7 << 11);
		*cfg |= (5 << 11);
		break;
	case FC_PER2MEM_PER:
		*cfg &= ~(7 << 11);
		*cfg |= (6 << 11);
		break;
	case FC_PER2PER_SPER:
		*cfg |= (7 << 11);
		break;

	default:
		err = -EINVAL;
		goto out;
	}
	*cfg &= ~(0x1f << 6);
	*cfg |= ((ch_cfg->dest_per & 0x1f) << 6);

	*cfg &= ~(0x1f << 1);
	*cfg |= ((ch_cfg->src_per & 0x1f) << 1);

      out:
	return err;
}

EXPORT_SYMBOL_GPL(pnx4008_dma_pack_config);

int pnx4008_dma_parse_config(unsigned long cfg,
			     pnx4008_dma_ch_config_t * ch_cfg)
{
	int err = 0;

	if (!ch_cfg) {
		err = -EINVAL;
		goto out;
	}

	cfg >>= 1;

	ch_cfg->src_per = cfg & 0x1f;
	cfg >>= 5;

	ch_cfg->dest_per = cfg & 0x1f;
	cfg >>= 5;

	switch (cfg & 7) {
	case 0:
		ch_cfg->flow_cntrl = FC_MEM2MEM_DMA;
		break;
	case 1:
		ch_cfg->flow_cntrl = FC_MEM2PER_DMA;
		break;
	case 2:
		ch_cfg->flow_cntrl = FC_PER2MEM_DMA;
		break;
	case 3:
		ch_cfg->flow_cntrl = FC_PER2PER_DMA;
		break;
	case 4:
		ch_cfg->flow_cntrl = FC_PER2PER_DPER;
		break;
	case 5:
		ch_cfg->flow_cntrl = FC_MEM2PER_PER;
		break;
	case 6:
		ch_cfg->flow_cntrl = FC_PER2MEM_PER;
		break;
	case 7:
		ch_cfg->flow_cntrl = FC_PER2PER_SPER;
	}
	cfg >>= 3;

	ch_cfg->ie = cfg & 1;
	cfg >>= 1;

	ch_cfg->itc = cfg & 1;
	cfg >>= 1;

	ch_cfg->lock = cfg & 1;
	cfg >>= 1;

	ch_cfg->active = cfg & 1;
	cfg >>= 1;

	ch_cfg->halt = cfg & 1;

      out:
	return err;
}

EXPORT_SYMBOL_GPL(pnx4008_dma_parse_config);

void pnx4008_dma_split_head_entry(pnx4008_dma_config_t * config,
				  pnx4008_dma_ch_ctrl_t * ctrl)
{
	int new_len = ctrl->tr_size, num_entries = 0;
	int old_len = new_len;
	int src_width, dest_width, count = 1;

	switch (ctrl->swidth) {
	case WIDTH_BYTE:
		src_width = 1;
		break;
	case WIDTH_HWORD:
		src_width = 2;
		break;
	case WIDTH_WORD:
		src_width = 4;
		break;
	default:
		return;
	}

	switch (ctrl->dwidth) {
	case WIDTH_BYTE:
		dest_width = 1;
		break;
	case WIDTH_HWORD:
		dest_width = 2;
		break;
	case WIDTH_WORD:
		dest_width = 4;
		break;
	default:
		return;
	}

	while (new_len > 0x7FF) {
		num_entries++;
		new_len = (ctrl->tr_size + num_entries) / (num_entries + 1);
	}
	if (num_entries != 0) {
		pnx4008_dma_ll_t *ll = NULL;
		config->ch_ctrl &= ~0x7ff;
		config->ch_ctrl |= new_len;
		if (!config->is_ll) {
			config->is_ll = 1;
			while (num_entries) {
				if (!ll) {
					config->ll =
					    pnx4008_alloc_ll_entry(&config->
								   ll_dma);
					ll = config->ll;
				} else {
					ll->next =
					    pnx4008_alloc_ll_entry(&ll->
								   next_dma);
					ll = ll->next;
				}

				if (ctrl->si)
					ll->src_addr =
					    config->src_addr +
					    src_width * new_len * count;
				else
					ll->src_addr = config->src_addr;
				if (ctrl->di)
					ll->dest_addr =
					    config->dest_addr +
					    dest_width * new_len * count;
				else
					ll->dest_addr = config->dest_addr;
				ll->ch_ctrl = config->ch_ctrl & 0x7fffffff;
				ll->next_dma = 0;
				ll->next = NULL;
				num_entries--;
				count++;
			}
		} else {
			pnx4008_dma_ll_t *ll_old = config->ll;
			unsigned long ll_dma_old = config->ll_dma;
			while (num_entries) {
				if (!ll) {
					config->ll =
					    pnx4008_alloc_ll_entry(&config->
								   ll_dma);
					ll = config->ll;
				} else {
					ll->next =
					    pnx4008_alloc_ll_entry(&ll->
								   next_dma);
					ll = ll->next;
				}

				if (ctrl->si)
					ll->src_addr =
					    config->src_addr +
					    src_width * new_len * count;
				else
					ll->src_addr = config->src_addr;
				if (ctrl->di)
					ll->dest_addr =
					    config->dest_addr +
					    dest_width * new_len * count;
				else
					ll->dest_addr = config->dest_addr;
				ll->ch_ctrl = config->ch_ctrl & 0x7fffffff;
				ll->next_dma = 0;
				ll->next = NULL;
				num_entries--;
				count++;
			}
			ll->next_dma = ll_dma_old;
			ll->next = ll_old;
		}
		/* adjust last length/tc */
		ll->ch_ctrl = config->ch_ctrl & (~0x7ff);
		ll->ch_ctrl |= old_len - new_len * (count - 1);
		config->ch_ctrl &= 0x7fffffff;
	}
}

EXPORT_SYMBOL_GPL(pnx4008_dma_split_head_entry);

void pnx4008_dma_split_ll_entry(pnx4008_dma_ll_t * cur_ll,
				pnx4008_dma_ch_ctrl_t * ctrl)
{
	int new_len = ctrl->tr_size, num_entries = 0;
	int old_len = new_len;
	int src_width, dest_width, count = 1;

	switch (ctrl->swidth) {
	case WIDTH_BYTE:
		src_width = 1;
		break;
	case WIDTH_HWORD:
		src_width = 2;
		break;
	case WIDTH_WORD:
		src_width = 4;
		break;
	default:
		return;
	}

	switch (ctrl->dwidth) {
	case WIDTH_BYTE:
		dest_width = 1;
		break;
	case WIDTH_HWORD:
		dest_width = 2;
		break;
	case WIDTH_WORD:
		dest_width = 4;
		break;
	default:
		return;
	}

	while (new_len > 0x7FF) {
		num_entries++;
		new_len = (ctrl->tr_size + num_entries) / (num_entries + 1);
	}
	if (num_entries != 0) {
		pnx4008_dma_ll_t *ll = NULL;
		cur_ll->ch_ctrl &= ~0x7ff;
		cur_ll->ch_ctrl |= new_len;
		if (!cur_ll->next) {
			while (num_entries) {
				if (!ll) {
					cur_ll->next =
					    pnx4008_alloc_ll_entry(&cur_ll->
								   next_dma);
					ll = cur_ll->next;
				} else {
					ll->next =
					    pnx4008_alloc_ll_entry(&ll->
								   next_dma);
					ll = ll->next;
				}

				if (ctrl->si)
					ll->src_addr =
					    cur_ll->src_addr +
					    src_width * new_len * count;
				else
					ll->src_addr = cur_ll->src_addr;
				if (ctrl->di)
					ll->dest_addr =
					    cur_ll->dest_addr +
					    dest_width * new_len * count;
				else
					ll->dest_addr = cur_ll->dest_addr;
				ll->ch_ctrl = cur_ll->ch_ctrl & 0x7fffffff;
				ll->next_dma = 0;
				ll->next = NULL;
				num_entries--;
				count++;
			}
		} else {
			pnx4008_dma_ll_t *ll_old = cur_ll->next;
			unsigned long ll_dma_old = cur_ll->next_dma;
			while (num_entries) {
				if (!ll) {
					cur_ll->next =
					    pnx4008_alloc_ll_entry(&cur_ll->
								   next_dma);
					ll = cur_ll->next;
				} else {
					ll->next =
					    pnx4008_alloc_ll_entry(&ll->
								   next_dma);
					ll = ll->next;
				}

				if (ctrl->si)
					ll->src_addr =
					    cur_ll->src_addr +
					    src_width * new_len * count;
				else
					ll->src_addr = cur_ll->src_addr;
				if (ctrl->di)
					ll->dest_addr =
					    cur_ll->dest_addr +
					    dest_width * new_len * count;
				else
					ll->dest_addr = cur_ll->dest_addr;
				ll->ch_ctrl = cur_ll->ch_ctrl & 0x7fffffff;
				ll->next_dma = 0;
				ll->next = NULL;
				num_entries--;
				count++;
			}

			ll->next_dma = ll_dma_old;
			ll->next = ll_old;
		}
		/* adjust last length/tc */
		ll->ch_ctrl = cur_ll->ch_ctrl & (~0x7ff);
		ll->ch_ctrl |= old_len - new_len * (count - 1);
		cur_ll->ch_ctrl &= 0x7fffffff;
	}
}

EXPORT_SYMBOL_GPL(pnx4008_dma_split_ll_entry);

int pnx4008_config_channel(int ch, pnx4008_dma_config_t * config)
{
	if (!VALID_CHANNEL(ch) || !dma_channels[ch].name)
		return -EINVAL;

	pnx4008_dma_lock();
	__raw_writel(config->src_addr, DMAC_Cx_SRC_ADDR(ch));
	__raw_writel(config->dest_addr, DMAC_Cx_DEST_ADDR(ch));

	if (config->is_ll)
		__raw_writel(config->ll_dma, DMAC_Cx_LLI(ch));
	else
		__raw_writel(0, DMAC_Cx_LLI(ch));

	__raw_writel(config->ch_ctrl, DMAC_Cx_CONTROL(ch));
	__raw_writel(config->ch_cfg, DMAC_Cx_CONFIG(ch));
	pnx4008_dma_unlock();

	return 0;

}

EXPORT_SYMBOL_GPL(pnx4008_config_channel);

int pnx4008_channel_get_config(int ch, pnx4008_dma_config_t * config)
{
	if (!VALID_CHANNEL(ch) || !dma_channels[ch].name || !config)
		return -EINVAL;

	pnx4008_dma_lock();
	config->ch_cfg = __raw_readl(DMAC_Cx_CONFIG(ch));
	config->ch_ctrl = __raw_readl(DMAC_Cx_CONTROL(ch));

	config->ll_dma = __raw_readl(DMAC_Cx_LLI(ch));
	config->is_ll = config->ll_dma ? 1 : 0;

	config->src_addr = __raw_readl(DMAC_Cx_SRC_ADDR(ch));
	config->dest_addr = __raw_readl(DMAC_Cx_DEST_ADDR(ch));
	pnx4008_dma_unlock();

	return 0;
}

EXPORT_SYMBOL_GPL(pnx4008_channel_get_config);

int pnx4008_dma_ch_enable(int ch)
{
	unsigned long ch_cfg;

	if (!VALID_CHANNEL(ch) || !dma_channels[ch].name)
		return -EINVAL;

	pnx4008_dma_lock();
	ch_cfg = __raw_readl(DMAC_Cx_CONFIG(ch));
	ch_cfg |= 1;
	__raw_writel(ch_cfg, DMAC_Cx_CONFIG(ch));
	pnx4008_dma_unlock();

	return 0;
}

EXPORT_SYMBOL_GPL(pnx4008_dma_ch_enable);

int pnx4008_dma_ch_disable(int ch)
{
	unsigned long ch_cfg;

	if (!VALID_CHANNEL(ch) || !dma_channels[ch].name)
		return -EINVAL;

	pnx4008_dma_lock();
	ch_cfg = __raw_readl(DMAC_Cx_CONFIG(ch));
	ch_cfg &= ~1;
	__raw_writel(ch_cfg, DMAC_Cx_CONFIG(ch));
	pnx4008_dma_unlock();

	return 0;
}

EXPORT_SYMBOL_GPL(pnx4008_dma_ch_disable);

static u32 per_dma_addresses[16] = {
	0,			/* PER_CAM_DMA_1: not yet */
	0,			/* PER_NDF_FLASH: not yet */
	0,			/* PER_MBX_SLAVE_FIFO: not yet */
	PNX4008_SPI2_BASE + 0x14,	/* PER_SPI2_REC_XMIT */
	0,			/* PER_MS_SD_RX_XMIT: not yet */
	0,			/* PER_HS_UART_1_XMIT: not yet */
	0,			/* PER_HS_UART_1_RX: not yet */
	0,			/* PER_HS_UART_2_XMIT: not yet */
	0,			/* PER_HS_UART_2_RX: not yet */
	0,			/* PER_HS_UART_7_XMIT: not yet */
	0,			/* PER_HS_UART_7_RX: not yet */
	PNX4008_SPI1_BASE + 0x14,	/* PER_SPI1_REC_XMIT */
	PNX4008_FLASH_DATA,	/* PER_MLC_NDF_SREC */
	0,			/* PER_CAM_DMA_2: not yet */
	0,			/* PER_PRNG_INFIFO: not yet */
	0,			/* PER_PRNG_OUTFIFO: not yet */
};

static int pnx4008_reconfig_channel(dmach_t channel, dma_t * dma)
{
	pnx4008_dma_ch_ctrl_t ctrl;
	pnx4008_dma_ch_config_t config;
	pnx4008_dma_config_t cfg;
	pnx4008_dma_ll_t *prev = NULL, *ll = NULL;
	struct scatterlist *list;
	int res = 0, num_entries, item;
	u32 ll_dma;
	u8 periph_id = (channel & 0xF00) >> 8;

	channel &= 0x0F;

	pnx4008_channel_get_config(channel, &cfg);

	pnx4008_dma_parse_config(cfg.ch_cfg, &config);
	pnx4008_dma_parse_control(cfg.ch_ctrl, &ctrl);

	pnx4008_free_ll(dma_channels[channel].ll_dma, dma_channels[channel].ll);

	if (!dma->using_sg) {
		num_entries = 1;
		list = &dma->buf;
		cfg.is_ll = 0;
		ctrl.tc_mask = 1;
	} else {
		cfg.is_ll = 1;
		num_entries = dma->sgcount;
		list = dma->sg;
		ctrl.tc_mask = 0;
	}

	ctrl.cacheable = 0;
	ctrl.bufferable = 0;
	ctrl.priv_mode = 1;
	ctrl.di = 1;
	ctrl.si = 0;
	ctrl.dest_ahb1 = 0;	/* ? */
	ctrl.src_ahb1 = 0;	/* ? */
	ctrl.dbsize = 1;
	ctrl.sbsize = 1;

	for (prev = NULL, item = 0; item < num_entries; item++, list++) {
		if (item == 1)
			prev = ll = pnx4008_alloc_ll_entry(&ll_dma);
		else if (item > 1) {
			ll = pnx4008_alloc_ll_entry(&ll_dma);
			prev->next = ll;
			prev->next_dma = ll_dma;
		}
		if (item > 0 && ll == NULL) {
			res = -ENOMEM;
			goto end;
		}

		if ((dma->dma_mode & DMA_MODE_MASK) == DMA_MODE_READ) {
			if (item == 0) {
				cfg.dest_addr = (unsigned long)list->__address;
				cfg.src_addr = per_dma_addresses[periph_id];
			} else {
				ll->dest_addr = (unsigned long)list->__address;
				ll->src_addr = per_dma_addresses[periph_id];
			}
			ctrl.dwidth = WIDTH_WORD;
			ctrl.swidth = WIDTH_BYTE;
			ctrl.tr_size = list->length;
			config.dest_per = 0;
			config.src_per = 0;
			config.flow_cntrl = FC_PER2MEM_DMA;
			if (dma_channels[channel].target_addr) {
				cfg.src_addr =
				    (unsigned long)dma_channels[channel].
				    target_addr;
				config.src_per =
				    dma_channels[channel].target_id;
			}
		} else if ((dma->dma_mode & DMA_MODE_MASK) == DMA_MODE_WRITE) {
			if (item == 0) {
				cfg.dest_addr = 0;
				cfg.src_addr = (unsigned long)list->__address;
			} else {
				ll->dest_addr = 0;
				ll->src_addr = (unsigned long)list->__address;
			}
			ctrl.dwidth = WIDTH_BYTE;
			ctrl.swidth = WIDTH_WORD;	/* ? */
			ctrl.tr_size = ((list->length + 3) >> 2);
			config.dest_per = 0;
			config.src_per = 0;
			config.flow_cntrl = FC_MEM2PER_DMA;
			if (dma_channels[channel].target_addr) {
				if (item == 0)
					cfg.dest_addr = (unsigned long)
					    dma_channels[channel].target_addr;
				else
					ll->dest_addr = (unsigned long)
					    dma_channels[channel].target_addr;
				config.dest_per =
				    dma_channels[channel].target_id;
			}
		} else {
			printk(KERN_ERR "Not supported at %s:%d\n",
			       __FILE__, __LINE__);
			res = -ENOTSUPP;
			goto end;
		}
		if (item > 0) {
			res = pnx4008_dma_pack_control(&ctrl, &ll->ch_ctrl);
			if (res < 0) {
				if (res == -E2BIG) {
					pnx4008_dma_split_ll_entry(ll, &ctrl);
					while (ll) {
						prev = ll;
						ll = ll->next;
					}
				} else
					goto end;
			}
		}
	}

	if (dma->using_sg)	/* last entry's tc := 1 */
		ll->ch_ctrl |= 0x80000000;

	config.halt = 0;
	config.active = 1;
	config.lock = 0;
	config.itc = 1;
	config.ie = 1;

	pnx4008_dma_pack_config(&config, &cfg.ch_cfg);
	res = pnx4008_dma_pack_control(&ctrl, &cfg.ch_ctrl);
	if (res < 0) {
		if (res == -E2BIG)
			pnx4008_dma_split_head_entry(&cfg, &ctrl);
		else
			goto end;
	}

	if (cfg.is_ll) {
		dma_channels[channel].ll_dma = cfg.ll_dma;
		dma_channels[channel].ll = cfg.ll;
	}

	res = pnx4008_config_channel(channel, &cfg);
      end:
	return res;
}

int pnx4008_dma_ch_enabled(int ch)
{
	unsigned long ch_cfg;

	if (!VALID_CHANNEL(ch) || !dma_channels[ch].name)
		return -EINVAL;

	pnx4008_dma_lock();
	ch_cfg = __raw_readl(DMAC_Cx_CONFIG(ch));
	pnx4008_dma_unlock();

	return ch_cfg & 1;
}

EXPORT_SYMBOL_GPL(pnx4008_dma_ch_enabled);

static irqreturn_t dma_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	int i;
	unsigned long dint = __raw_readl(DMAC_INT_STAT);
	unsigned long tcint = __raw_readl(DMAC_INT_TC_STAT);
	unsigned long eint = __raw_readl(DMAC_INT_ERR_STAT);
	unsigned long i_bit;

	for (i = MAX_DMA_CHANNELS - 1; i >= 0; i--) {
		i_bit = 1 << i;
		if (dint & i_bit) {
			struct dma_channel *channel = &dma_channels[i];

			if (channel->name && channel->irq_handler) {
				int cause = 0;

				if (eint & i_bit)
					cause |= DMA_ERR_INT;
				if (tcint & i_bit)
					cause |= DMA_TC_INT;
				channel->irq_handler(i, cause, channel->data,
						     regs);
			} else {
				/*
				 * IRQ for an unregistered DMA channel
				 */
				printk(KERN_WARNING
				       "spurious IRQ for DMA channel %d\n", i);
			}
			if (tcint & i_bit)
				__raw_writel(i_bit, DMAC_INT_TC_CLEAR);
			if (eint & i_bit)
				__raw_writel(i_bit, DMAC_INT_ERR_CLEAR);
		}
	}
	return IRQ_HANDLED;
}

static int __init pnx4008_dma_init(void)
{
	int ret, i;

	ret = request_irq(DMA_INT, dma_irq_handler, 0, "DMA", NULL);
	if (ret) {
		printk(KERN_CRIT "Wow!  Can't register IRQ for DMA\n");
		goto out;
	}

	ll_pool.count = 0x4000 / sizeof(pnx4008_dma_ll_t);
	ll_pool.cur = ll_pool.vaddr =
	    dma_alloc_coherent(NULL, ll_pool.count * sizeof(pnx4008_dma_ll_t),
			       &ll_pool.dma_addr, GFP_KERNEL);

	if (!ll_pool.vaddr) {
		ret = -ENOMEM;
		free_irq(DMA_INT, NULL);
		goto out;
	}

	for (i = 0; i < ll_pool.count - 1; i++) {
		void **addr = ll_pool.vaddr + i * sizeof(pnx4008_dma_ll_t);
		*addr = (void *)addr + sizeof(pnx4008_dma_ll_t);
	}
	*(long *)(ll_pool.vaddr +
		  (ll_pool.count - 1) * sizeof(pnx4008_dma_ll_t)) =
	    (long)ll_pool.vaddr;

	__raw_writel(1, DMAC_CONFIG);

      out:
	return ret;
}

static void std_dma_handler(int ch, int cause, void *data, struct pt_regs *regs)
{
	/* empty handler to suppress warning at DMA_INT handler */
}

static int arch_dma_request(dmach_t channel, dma_t * dma)
{
	int res = 0;
	int ch = channel & 0xF;

	res = pnx4008_request_channel("std_dma", ch, std_dma_handler, dma);
	if (res < 0) {
		goto end;
	}
	res = pnx4008_reconfig_channel(res | (channel & 0xF00), dma);
	if (res < 0)
		pnx4008_free_channel(ch);
      end:
	return res;
}

static void arch_enable_dma(dmach_t channel, dma_t * dma)
{
	if (pnx4008_reconfig_channel(channel, dma) == 0)
		pnx4008_dma_ch_enable(channel & 0xF);
}

static void arch_disable_dma(dmach_t channel, dma_t * dma)
{
	pnx4008_dma_ch_disable(channel & 0xF);
}

static int arch_get_dma_residue(dmach_t channel, dma_t * dma)
{
	pnx4008_dma_config_t cfg;
	pnx4008_dma_ch_ctrl_t ctrl;
	int bytes_left = 0;

	if (pnx4008_channel_get_config(channel & 0xF, &cfg) < 0)
		goto out;
	if (pnx4008_dma_parse_control(cfg.ch_ctrl, &ctrl) < 0)
		goto out;
	bytes_left = ctrl.tr_size;
	if (cfg.is_ll) {
		pnx4008_dma_ll_t *ll = cfg.ll;
		while (ll) {
			pnx4008_dma_parse_control(ll->ch_ctrl, &ctrl);
			bytes_left += ctrl.tr_size;
			ll = ll->next;
		}
	}
      out:
	return bytes_left;
}

int pnx4008_set_dma_options(dmach_t channel, void *target_addr,
			    int peripheral_id)
{
	int err;
	int ch = channel & 0xF;

	if (VALID_CHANNEL(ch) && dma_channels[ch].name) {
		dma_channels[ch].target_addr = target_addr;
		dma_channels[ch].target_id = peripheral_id;
		err = 0;
	} else {
		err = -EFAULT;
	}
	return err;
}

EXPORT_SYMBOL_GPL(pnx4008_set_dma_options);

static struct dma_ops arch_dma_ops = {
	.type = "pnx4008",
	.request = arch_dma_request,
	.enable = arch_enable_dma,
	.disable = arch_disable_dma,
	.residue = arch_get_dma_residue,
};

void arch_dma_init(dma_t * dma)
{
	int i;
	for (i = 0; i < MAX_DMA_CHANNELS; i++)
		dma[i].d_ops = &arch_dma_ops;
}

arch_initcall(pnx4008_dma_init);
