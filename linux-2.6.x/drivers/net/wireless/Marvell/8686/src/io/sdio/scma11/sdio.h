/** @file sdio.h
 *  @brief This file contains the structure definations for the low level driver
 *         And the error response related code
 * 
 * (c) Copyright © 2003-2006, Marvell International Ltd. 
 *
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 * (the "License").  You may use, redistribute and/or modify this File in 
 * accordance with the terms and conditions of the License, a copy of which 
 * is available along with the File in the gpl.txt file or by writing to 
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 *
 */

#ifndef __SDIO_H__
#define __SDIO_H__

#include	<linux/spinlock.h>      /* For read write semaphores */
#include	<linux/completion.h>
#include	<asm/semaphore.h>
#include	<linux/completion.h>
#include 	<asm/arch/spba.h>
#include	<asm/arch/gpio.h>
#include 	<asm/arch/clock.h>
#include	<asm/dma.h>
#ifdef CONFIG_PROC_FS
#include 	<linux/proc_fs.h>
#endif

#include 	<sdio_spec.h>
#include 	<sdio_error.h>

#include 	"os_defs.h"

#define	_ERROR(x...)		printk(KERN_ERR x)

#ifdef	DEBUG_SDIO_LEVEL1
#ifndef DEBUG_SDIO_LEVEL0
#define DEBUG_SDIO_LEVEL0
#endif
#define	_ENTER() printk(KERN_DEBUG "Enter: %s, %s linux %i\n", __FUNCTION__, __FILE__, __LINE__)
#define	_LEAVE() printk(KERN_DEBUG "Leave: %s, %s linux %i\n", __FUNCTION__, __FILE__, __LINE__)
#else
#define _ENTER()
#define _LEAVE()
#endif

#ifdef DEBUG_SDIO_LEVEL0
#define	_DBG(x...)		printk(x)
#else
#define	_DBG(x...)
#endif

#define MAJOR_NUMBER	242

#ifdef MOTO_PLATFORM
#define SAVED_SDHC_CLK_RATE     (ctrller->saved_sdhc_clk_rate)
#define SAVED_SDHC_CMD_DAT_CONT (ctrller->saved_sdhc_cmd_dat_cont)
#define SAVED_SDHC_RES_TO       (ctrller->saved_sdhc_res_to)
#define SAVED_SDHC_READ_TO      (ctrller->saved_sdhc_read_to)
#define SAVED_SDHC_BLK_LEN      (ctrller->saved_sdhc_blk_len)
#define SAVED_SDHC_NOB          (ctrller->saved_sdhc_nob)
#define SAVED_SDHC_INT_CNTR     (ctrller->saved_sdhc_int_cntr)
#endif

#ifdef DEBUG_USEC
static inline unsigned long
get_usecond(void)
{
    struct timeval t;
    unsigned long usec;

    do_gettimeofday(&t);
    usec = (unsigned long) t.tv_sec * 1000000 + ((unsigned long) t.tv_usec);
    return usec;
}
#endif

typedef struct _card_capability
{
    u8 num_of_io_funcs;         /* Number of i/o functions */
    u8 memory_yes;              /* Memory present ? */
    u16 rca;                    /* Relative Card Address */
    u32 ocr;                    /* Operation Condition register */
    u16 fnblksz[8];
    u32 cisptr[8];
} card_capability;

typedef struct _dummy_tmpl
{
    int irq_line;
} dummy_tmpl;

typedef struct _mmc_card_rec
{
    u8 chiprev;
    u8 block_size_512;
    u8 async_int_mode;
    card_capability info;
    struct _sdio_host *ctrlr;
} mmc_card_rec;

typedef struct _mmc_card_rec *mmc_card_t;

typedef struct _sdio_host *mmc_controller_t;

typedef enum _sdio_fsm
{
    SDIO_FSM_IDLE = 1,
    SDIO_FSM_CLK_OFF,
    SDIO_FSM_END_CMD,
    SDIO_FSM_BUFFER_IN_TRANSIT,
    SDIO_FSM_END_BUFFER,
    SDIO_FSM_END_IO,
    SDIO_FSM_END_PRG,
    SDIO_FSM_ERROR
} sdio_fsm_state;

#define SDIO_STATE_LABEL( state ) ( \
	(state == SDIO_FSM_IDLE) ? "IDLE" : \
	(state == SDIO_FSM_CLK_OFF) ? "CLK_OFF" : \
	(state == SDIO_FSM_END_CMD) ? "END_CMD" : \
	(state == SDIO_FSM_BUFFER_IN_TRANSIT) ? "IN_TRANSIT" : \
	(state == SDIO_FSM_END_BUFFER) ? "END_BUFFER" : \
	(state == SDIO_FSM_END_IO) ? "END_IO" : \
	(state == SDIO_FSM_END_PRG) ? "END_PRG" : "UNKNOWN" )

/*!
 * This structure is a way for the low level driver to define their own
 * \b mmc_host structure. This structure includes the core \b mmc_host
 * structure that is provided by Linux MMC/SD Bus protocol driver as an
 * element and has other elements that are specifically required by this
 * low-level driver.
 */
typedef struct _sdio_host
{
    /*!
     * The mmc structure holds all the information about the device
     * structure, current SDHC io bus settings, the current OCR setting,
     * devices attached to this host, and so on.
     */
    struct mmc_host *mmc;

    /*!
     * This variable is used for locking the host data structure from
     * multiple access.
     */
    spinlock_t lock;

    /*!
     * Resource structure, which will maintain base addresses and IRQs.
     */
    struct resource *res;

    /*!
     * Base address of SDHC, used in readl and writel.
     */
    void *base;

    /*!
     * SDHC IRQ number.
     */
    int irq;

    /*!
     * Clock id to hold ipg_perclk.
     */
    enum mxc_clocks clock_id;

    /*!
     * DMA Event number.
     */
    int event_id;

    /*!
     * DMA channel number.
     */
    int dma;

    /*!
     * Holds the Interrupt Control Register value.
     */
    unsigned int icntr;

    /*!
     * Pointer to hold MMC/SD request.
     */
    struct mmc_request *req;

    /*!
     * Pointer to hold MMC/SD command.
     */
    struct mmc_command *cmd;

    /*!
     * Pointer to hold MMC/SD data.
     */
    struct mmc_data *data;

    /*!
     * Holds the number of bytes to transfer using SDMA.
     */
    unsigned int dma_size;

    /*!
     * Value to store in Command and Data Control Register 
     * - currently unused
     */
    unsigned int cmdat;

    /*!
     * Power mode - currently unused
     */
    unsigned int power_mode;

    /*!
     * DMA address for scatter-gather transfers
     */
    dma_addr_t sg_dma;

    /*!
     * Length of the scatter-gather list
     */
    unsigned int dma_len;

    /*!
     * Holds the direction of data transfer.
     */
    unsigned int dma_dir;

    /*!
     * Card-specific informat (deprecated?)
     */
    mmc_card_t card;

    u16 manf_id;

    u16 dev_id;

    int bus_width;

    int irq_line;

    struct _dummy_tmpl *tmpl;

    int card_int_ready;

    struct completion completion;       /* completion                */

    struct semaphore io_sem;

    u32 num_ofcmd52;

    u32 num_ofcmd53;

    int busy;                   /* atomic busy flag               */

    u32 mmc_i_reg;              /* interrupt last requested       */

    u32 mmc_stat;               /* status register at last intr   */

    sdio_fsm_state state;

    card_capability card_capability;

    // char                 *iodata;

    u8 mmc_res[8];

    u8 io_reset;                /* IO reset */

#ifdef MOTO_PLATFORM
    /* used to store SDHC registers while in DSM */
    unsigned long saved_sdhc_clk_rate;
    unsigned long saved_sdhc_cmd_dat_cont;
    unsigned long saved_sdhc_res_to;
    unsigned long saved_sdhc_read_to;
    unsigned long saved_sdhc_blk_len;
    unsigned long saved_sdhc_nob;
    unsigned long saved_sdhc_int_cntr;
#endif

} __attribute__ ((aligned)) sdio_ctrller;

#define MMC_RESPONSE(ctrlr, idx)    ((ctrlr->mmc_res)[idx])

typedef struct _sdio_operations
{
    char name[16];
} sdio_operations;

typedef struct _iorw_extended_t
{
    u8 rw_flag;          /** If 0 command is READ; else if 1 command is WRITE */
    u8 func_num;
    u8 blkmode;
    u8 op_code;
    u32 reg_addr;
    u32 byte_cnt;
    u32 blk_size;
    u8 *buf;
} iorw_extended_t;

typedef struct _ioreg
{
    u8 read_or_write;
    u8 function_num;
    u32 reg_addr;
    u8 dat;
} ioreg_t;

typedef struct _mmc_notifier_rec mmc_notifier_rec_t;
typedef struct _mmc_notifier_rec *mmc_notifier_t;

typedef int (*mmc_notifier_fn_t) (mmc_card_t);

struct _mmc_notifier_rec
{
    mmc_notifier_fn_t add;
    mmc_notifier_fn_t remove;
};

typedef enum _mmc_response
{
    MMC_NORESPONSE = 1,
    MMC_R1,
    MMC_R2,
    MMC_R3,
    MMC_R4,
    MMC_R5,
    MMC_R6
} mmc_response;

enum _cmd53_rw
{
    IOMEM_READ = 0,
    IOMEM_WRITE = 1
};

enum _cmd53_mode
{
    BLOCK_MODE = 1,
    BYTE_MODE = 0
};

enum _cmd53_opcode
{
    FIXED_ADDRESS = 0,
    INCREMENTAL_ADDRESS = 1
};

#define SDIO_BUSWIDTH_1_BIT 1
#define SDIO_BUSWIDTH_4_BIT 4
#define ECSI_BIT	    0x20
#define BUSWIDTH_1_BIT	    0x00
#define BUSWIDTH_4_BIT	    0x02
#define	MMC_TYPE_HOST		1
#define	MMC_TYPE_CARD		2
#define	MMC_REG_TYPE_USER	3

#define	SDIO_READ		0
#define	SDIO_WRITE		1
#define	MRVL_MANFID		0x2df
#define	MRVL_DEVID		0x9103
#define	NO			0
#define	YES			1
#define	TRUE			1
#define	FALSE			0
#define	ENABLED 		1
#define	DISABLED 		0
#define	SIZE_OF_TUPLE 		255

/* Functions exported out for WLAN Layer */
void *sdio_register(int type, void *ops, int memory);
int sdio_unregister(int type, void *ops);
int sdio_release(struct inode *in, struct file *file);
int stop_clock(sdio_ctrller * ctrller);
int start_clock(sdio_ctrller * ctrller);
int sdio_write_ioreg(mmc_card_t card, u8 func, u32 reg, u8 dat);
int sdio_read_ioreg(mmc_card_t card, u8 func, u32 reg, u8 * dat);
int sdio_read_iomem(mmc_card_t card, u8 func, u32 reg, u8 blockmode,
                    u8 opcode, ssize_t cnt, ssize_t blksz, u8 * dat);
int sdio_write_iomem(mmc_card_t card, u8 func, u32 reg, u8 blockmode,
                     u8 opcode, ssize_t cnt, ssize_t blksz, u8 * dat);
void sdio_print_imask(mmc_controller_t);
int sdio_request_irq(mmc_card_t card,
                     IRQ_RET_TYPE(*handler) (int, void *, struct pt_regs *),
                     unsigned long irq_flags,
                     const char *devname, void *dev_id);
int sdio_free_irq(mmc_card_t card, void *dev_id);
int sdio_read_cisinfo(mmc_card_t card, u8 *);
void sdio_enable_SDIO_INT(void);
int sdio_get_vendor_id(mmc_card_t card);
void print_addresses(sdio_ctrller * ctrller);
int sdio_set_buswidth(mmc_card_t card, int mode);
void sdio_unmask(void);

int sdio_suspend(mmc_card_t card);
int sdio_resume(mmc_card_t card);

#ifdef HOTPLUG
extern char hotplug_path[];
extern int call_usermodehelper(char *path, char **argv, char **envp);
#endif

#endif /* __SDIO__H */
