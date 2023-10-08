/*
 * Copyright 2005 Freescale Semiconductor, Inc.
 * Copyright (C) 2006-2008 Motorola, Inc.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Date     Author    Comment
 * 10/2006  Motorola  Added power-up logo support, additional pixel packing
 *                    formats, and functions for controlling the pixel clock.
 * 11/2006  Motorola  Updated ipu_enable_pixel_clock functions for ESD recovery.
 * 07/2007  Motorola  Added  Dynamic AP Clock Gating for IPU clock.
 * 08/2007  Motorola  Use mdelay to replace msleep in ipu_disable_channel
 * 09/2007  Motorola  Fix problem that camera VF got tearing
 * 09/2007  Motorola  Add comments.
 * 10/2007  Motorola  Remove memory reserved by IPU camera
 * 10/2007  Motorola  Add ipu irq support for camera still caputre.
 * 10/2007  Motorola  fix some logic about IPU_CONF register.
 * 10/2007  Motorola  FIQ related modified.
 * 11/2007  Motorola  Added Dynamic AP Clock Gating for IPU clock.
 * 11/2007  Motorola  Flash in display is observed during phone powering up, add wait for BG EOF in ipu_update_channel_buffer.
 * 12/2007  Motorola  Added function pointer check before call it.
 * 12/2007  Motorola  Remove ipu csi mclk disable from ipu_go_to_sleep, this is done in camera omx 
 * 12/2007  Motorola  Removed Dynamic AP Clock Gating due to the introduced issue that phone resets
 * 01/2008  Motorola  Added unlock operation before error return in ipu_link_channel
 * 03/2008  Motorola  Add support for new keypad 
 * 03/2008  Motorola  Fix red screen issue
 * 04/2008  Motorola  Add code for new display
 * 05/2008  Motorola  Add new channel init
 * 07/2008  Motorola  Modify disable and uninit
 * 08/2008  Motorola  Add 2 fuctions which set/restore m3if register priority
 */
/*!
 * @file ipu_common.c
 *
 * @brief This file contains the IPU driver common API functions.
 *
 * @ingroup IPU
 *
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
#include <asm/setup.h>
#endif  /* CONFIG_MOT_FEAT_POWERUP_LOGO */
#if defined(CONFIG_MACH_ARGONLVPHONE)
#include <linux/mpm.h>
#endif
#include "ipu.h"
#include "ipu_prv.h"
#include "ipu_regs.h"
#include "ipu_param_mem.h"
#include "asm-arm/io.h"
#include <asm/arch/board.h>
#include "asm/arch/clock.h"

#if defined(CONFIG_MACH_ARGONLVPHONE)
static uint32_t ipu_in_sleep_flag = 0;
static uint32_t ipu_mpm_num = 0;
static uint32_t ipu_int_ctrl[5];

void ipu_wake_from_sleep(void);
void ipu_go_to_sleep(void);
#endif

#define USE_FIQ_HANDLER 1

#define M3IFBASE IO_ADDRESS(M3IF_BASE_ADDR)

/*
 * This type definition is used to define a node in the GPIO interrupt queue for
 * registered interrupts for GPIO pins. Each node contains the GPIO signal number
 * associated with the ISR and the actual ISR function pointer.
 */
struct ipu_irq_node {
    irqreturn_t(*handler) (int, void *, struct pt_regs *);  /*!< the ISR */
    const char *name;   /*!< device associated with the interrupt */
    void *dev_id;       /*!< some unique information for the ISR */
    __u32 flags;        /*!< not used */
};

/* Globals */
int32_t gOpenCount;     /* Counter to track opens. */
uint32_t g_ipu_clk;
uint32_t g_ipu_csi_clk;
int g_ipu_irq[2];
int g_ipu_hw_rev;
bool gSecChanEn[21];
#define DBG_INT_CTRL 1

static uint32_t gCtlReg1 = 0;
static uint32_t gCtlReg2 = 0;
static uint32_t gCtlReg3 = 0;
static uint32_t gCtlReg4 = 0;
static uint32_t gCtlReg5 = 0;

uint32_t gChannelInitMask;
DEFINE_RAW_SPINLOCK(ipu_lock);

static struct ipu_irq_node ipu_irq_list[IPU_IRQ_COUNT];
static const char driver_name[] = "mxc_ipu";

/* Static functions */
static irqreturn_t ipu_irq_handler(int irq, void *desc, struct pt_regs *regs);
static void _ipu_pf_init(ipu_channel_params_t * params);
static void _ipu_pf_uninit(void);

inline static uint32_t channel_2_dma(ipu_channel_t ch, ipu_buffer_t type)
{
    return ((type == IPU_INPUT_BUFFER) ? ((uint32_t) ch & 0xFF) :
        ((type ==
          IPU_OUTPUT_BUFFER) ? (((uint32_t) ch >> 8) & 0xFF)
         : (((uint32_t) ch >> 16) & 0xFF)));
};

inline static uint32_t DMAParamAddr(uint32_t dma_ch)
{
    return (0x10000 | (dma_ch << 4));
};

#ifdef CONFIG_MOT_FEAT_POWERUP_LOGO
u32 mot_mbm_is_ipu_initialized = 0x0UL;
u32 mot_mbm_ipu_buffer_address = 0x0UL;
u32 mot_mbm_gpu_context = 0x0UL;

static int __init parse_tag_ipu_buffer_address(const struct tag *tag)
{
    mot_mbm_ipu_buffer_address = tag->u.ipu_buffer_address.ipu_buffer_address;
    DPRINTK(KERN_WARNING "%s: mot_mbm_ipu_buffer_address = 0x%08x\n",
                __FUNCTION__, mot_mbm_ipu_buffer_address);
    return 0;
}

__tagtable(ATAG_IPU_BUFFER_ADDRESS, parse_tag_ipu_buffer_address);

static int __init parse_tag_is_ipu_initialized(const struct tag *tag)
{
    mot_mbm_is_ipu_initialized = tag->u.is_ipu_initialized.is_ipu_initialized;
    DPRINTK(KERN_WARNING "%s: mot_mbm_is_ipu_initialized = 0x%08x\n",
                __FUNCTION__, mot_mbm_is_ipu_initialized);
    return 0;
}

__tagtable(ATAG_IS_IPU_INITIALIZED, parse_tag_is_ipu_initialized);

static int __init parse_tag_gpu_context(const struct tag *tag)
{
    mot_mbm_gpu_context = tag->u.gpu_context.gpu_context;
    DPRINTK(KERN_WARNING "%s: mot_mbm_gpu_context = 0x%08x\n",
                __FUNCTION__, mot_mbm_gpu_context);
    return 0;
}

__tagtable(ATAG_GPU_CONTEXT, parse_tag_gpu_context);

#endif  /* CONFIG_MOT_FEAT_POWERUP_LOGO */

/*!
 * This function is called by the driver framework to initialize the IPU
 * hardware.
 *
 * @param       dev       The device structure for the IPU passed in by the framework.
 *
 * @return      This function returns 0 on success or negative error code on error
 */
static
int ipu_probe(struct device *dev)
{
    struct platform_device *pdev = to_platform_device(dev);
    struct mxc_ipu_config *ipu_conf = pdev->dev.platform_data;
    FUNC_START;

    spin_lock_init(&ipu_lock);

    if (ipu_pool_initialize(MXCIPU_MEM_ADDRESS, MXCIPU_MEM_SIZE, SZ_4K)) {
        return -EBUSY;
    }

    g_ipu_hw_rev = ipu_conf->rev;

    /* Register IPU interrupts */
    g_ipu_irq[0] = platform_get_irq(pdev, 0);
    if (!g_ipu_irq[0])
        return -EINVAL;

    if (request_irq(g_ipu_irq[0], ipu_irq_handler, 0, driver_name, 0) != 0) {
        printk("request SYNC interrupt failed\n");
        return -EBUSY;
    }
    /* Some platforms have 2 IPU interrupts */
    g_ipu_irq[1] = platform_get_irq(pdev, 1);
    if (g_ipu_irq[1]) {
        if (request_irq
            (g_ipu_irq[1], ipu_irq_handler, 0, driver_name, 0) != 0) {
            printk("request ERR interrupt failed\n");
            return -EBUSY;
        }
    }

    /* Enable IPU and CSI clocks */
    /* Get IPU clock freq */
    mxc_clks_enable(IPU_CLK);
    g_ipu_clk = mxc_get_clocks(IPU_CLK);
    printk("g_ipu_clk = %d\n", g_ipu_clk);

    __raw_writel(0x00100010L, DI_HSP_CLK_PER);

    /* Set SDC refresh channels as high priority */
    __raw_writel(0x0000C000L, IDMAC_CHA_PRI);

    /* Set to max back to back burst requests */
    __raw_writel(0x00000070L, IDMAC_CONF);

#if defined(CONFIG_MACH_ARGONLVPHONE)
        if((ipu_mpm_num = mpm_register_with_mpm("mxc_ipu")) < 0)
        {
            printk(KERN_ALERT"ihal.ko: mpm_register_with_mpm failed \n");

        }
        printk(KERN_ALERT"ihal.ko: mpm_register_with_mpm ipu_mpm_num:%d\n",ipu_mpm_num);
#endif

    FUNC_END;
    return 0;
}

static void ipu_remove(void)
{
    free_irq(g_ipu_irq[0], 0);
    if (g_ipu_irq[1]) {
        free_irq(g_ipu_irq[1], 0);
    }

    __raw_writel(0x00000000L, IPU_CONF);

    mxc_clks_disable(IPU_CLK);
}

/*!
 * This function is called to initialize a logical IPU channel.
 *
 * @param       channel Input parameter for the logical channel ID to initalize.
 *
 * @param       params  Input parameter containing union of channel initialization
 *                      parameters.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */

extern unsigned long ipu_reserved_buff_fiq[1201];
extern void disable_fiq(int fiq);
extern void enable_fiq(int fiq);
extern int mxc_fiq_irq_switch(int vector, int irq2fiq);

int32_t ipu_init_channel(ipu_channel_t channel, ipu_channel_params_t * params)
{
    uint32_t ipu_conf;
    uint32_t reg;
    uint32_t lock_flags;

    FUNC_START;
    DPRINTK("channel = %d\n", IPU_CHAN_ID(channel));

    if ((channel != MEM_SDC_BG) && (channel != MEM_SDC_FG) &&
        (channel != MEM_ROT_ENC_MEM) && (channel != MEM_ROT_VF_MEM) &&
        (channel != MEM_ROT_PP_MEM) && (channel != CSI_MEM)
        && (params == NULL)) {
        return -EINVAL;
    }

#if defined(CONFIG_MACH_ARGONLVPHONE)
        if(gChannelInitMask == 0){
            mpm_driver_advise(ipu_mpm_num, MPM_ADVICE_DRIVER_IS_BUSY);
            ipu_wake_from_sleep();
        }
#endif

    spin_lock_irqsave(&ipu_lock, lock_flags);


    if (gChannelInitMask & (1L << IPU_CHAN_ID(channel))) {
        DPRINTK("Warning: channel already initialized %d\n", IPU_CHAN_ID(channel));
    }


    switch (channel) {
    case CSI_PRP_VF_MEM:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg & ~FS_VF_IN_VALID, IPU_FS_PROC_FLOW);

        if (params->mem_prp_vf_mem.graphics_combine_en)
            gSecChanEn[IPU_CHAN_ID(channel)] = true;

        _ipu_ic_init_prpvf(params, true);
        break;
    case CSI_PRP_VF_ADC:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg | (FS_DEST_ADC << FS_PRPVF_DEST_SEL_OFFSET),
                 IPU_FS_PROC_FLOW);

        _ipu_adc_init_channel(CSI_PRP_VF_ADC,
                      params->csi_prp_vf_adc.disp,
                      WriteTemplateNonSeq,
                      params->csi_prp_vf_adc.out_left,
                      params->csi_prp_vf_adc.out_top);

        _ipu_ic_init_prpvf(params, true);
        break;
    case MEM_PRP_VF_MEM:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg | FS_VF_IN_VALID, IPU_FS_PROC_FLOW);

        if (params->mem_prp_vf_mem.graphics_combine_en)
            gSecChanEn[IPU_CHAN_ID(channel)] = true;

        _ipu_ic_init_prpvf(params, false);
        break;
    case MEM_ROT_VF_MEM:
        _ipu_ic_init_rotate_vf(params);
        break;
    case CSI_PRP_ENC_MEM:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg & ~FS_ENC_IN_VALID, IPU_FS_PROC_FLOW);
        _ipu_ic_init_prpenc(params, true);
        break;
    case MEM_PRP_ENC_MEM:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg | FS_ENC_IN_VALID, IPU_FS_PROC_FLOW);
        _ipu_ic_init_prpenc(params, false);
        break;
    case MEM_ROT_ENC_MEM:
        _ipu_ic_init_rotate_enc(params);
        break;
    case MEM_PP_ADC:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg | (FS_DEST_ADC << FS_PP_DEST_SEL_OFFSET),
                 IPU_FS_PROC_FLOW);

        _ipu_adc_init_channel(MEM_PP_ADC, params->mem_pp_adc.disp,
                      WriteTemplateNonSeq,
                      params->mem_pp_adc.out_left,
                      params->mem_pp_adc.out_top);

        if (params->mem_pp_adc.graphics_combine_en)
            gSecChanEn[IPU_CHAN_ID(channel)] = true;

        _ipu_ic_init_pp(params);
        break;
    case MEM_PP_MEM:
        if (params->mem_pp_mem.graphics_combine_en)
            gSecChanEn[IPU_CHAN_ID(channel)] = true;

        _ipu_ic_init_pp(params);
        break;
    case MEM_ROT_PP_MEM:
        _ipu_ic_init_rotate_pp(params);
        break;
    case CSI_MEM:
        _ipu_ic_init_csi(params);
        break;

    case MEM_PF_Y_MEM:
    case MEM_PF_U_MEM:
    case MEM_PF_V_MEM:
        /* Enable PF block */
        _ipu_pf_init(params);
        break;

    case MEM_SDC_BG:
        DPRINTK("Initializing SDC BG\n");

        /*_ipu_sdc_bg_init(params); */
        break;
    case MEM_SDC_FG:
        DPRINTK("Initializing SDC FG\n");

        /*_ipu_sdc_fg_init(params); */
        break;
    case ADC_SYS1:
        _ipu_adc_init_channel(ADC_SYS1, params->adc_sys1.disp,
                      params->adc_sys1.ch_mode,
                      params->adc_sys1.out_left,
                      params->adc_sys1.out_top);
        break;
    case ADC_SYS2:
        _ipu_adc_init_channel(ADC_SYS2, params->adc_sys2.disp,
                      params->adc_sys2.ch_mode,
                      params->adc_sys2.out_left,
                      params->adc_sys2.out_top);
        break;
    default:
        printk("Missing channel initialization\n");
        break;
    }




    gChannelInitMask |= 1L << IPU_CHAN_ID(channel);

    /* Enable IPU sub module */
    ipu_conf = __raw_readl(IPU_CONF);
    if (gChannelInitMask & 0x00000066L) /*CSI */
    {
        ipu_conf |= IPU_CONF_CSI_EN;
    }
    if (gChannelInitMask & 0x00001FFFL) /*IC */
    {
        ipu_conf |= IPU_CONF_IC_EN;
    }
    if (gChannelInitMask & 0x00000A10L) /*ROT */
    {
        ipu_conf |= IPU_CONF_ROT_EN;
    }
    if (gChannelInitMask & 0x0001C000L) /*SDC */
    {
        ipu_conf |= IPU_CONF_SDC_EN | IPU_CONF_DI_EN;
    }
    if (gChannelInitMask & 0x00061140L) /*ADC */
    {
        ipu_conf |= IPU_CONF_ADC_EN | IPU_CONF_DI_EN;
    }
    if (gChannelInitMask & 0x00380000L) /*PF */
    {
        ipu_conf |= IPU_CONF_PF_EN;
    }
    __raw_writel(ipu_conf, IPU_CONF);


    spin_unlock_irqrestore(&ipu_lock, lock_flags);

    FUNC_END;
    return 0;
}

/*!
 * This function is called to uninitialize a logical IPU channel.
 *
 * @param       channel Input parameter for the logical channel ID to uninitalize.
 */
void ipu_uninit_channel(ipu_channel_t channel)
{
    uint32_t lock_flags;
    uint32_t reg;
    uint32_t dma, mask = 0;
    uint32_t ipu_conf;

    FUNC_START;


    spin_lock_irqsave(&ipu_lock, lock_flags);

    if ((gChannelInitMask & (1L << IPU_CHAN_ID(channel))) == 0) {
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
        DPRINTK("Channel already uninitialized\n");
        return;
    }


#if defined(CONFIG_MACH_NEVIS)
    /* Make sure channel is disabled */

    /* Get input and output dma channels */
    dma = channel_2_dma(channel, IPU_OUTPUT_BUFFER);
    if (dma != IDMA_CHAN_INVALID)
        mask |= 1UL << dma;
    dma = channel_2_dma(channel, IPU_INPUT_BUFFER);
    if (dma != IDMA_CHAN_INVALID)
        mask |= 1UL << dma;
    /* Get secondary input dma channel */
    if (gSecChanEn[IPU_CHAN_ID(channel)]) {
        dma = channel_2_dma(channel, IPU_SEC_INPUT_BUFFER);
        if (dma != IDMA_CHAN_INVALID) {
            mask |= 1UL << dma;
        }
    }

    if (mask & __raw_readl(IDMAC_CHA_EN)) {
        DPRINTK("Channel %d is not disabled, disable first\n",IPU_CHAN_ID(channel));
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
        return;
    }
#endif

    gSecChanEn[IPU_CHAN_ID(channel)] = false;



    /* ipu_disable_channel(channel, false); */

    switch (channel) {
    case CSI_MEM:
        _ipu_ic_uninit_csi();
        break;
    case CSI_PRP_VF_ADC:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg & ~FS_PRPVF_DEST_SEL_MASK, IPU_FS_PROC_FLOW);

        _ipu_adc_uninit_channel(CSI_PRP_VF_ADC);

    /* Fall thru */
    case CSI_PRP_VF_MEM:
    case MEM_PRP_VF_MEM:
        _ipu_ic_uninit_prpvf();
        break;
    case MEM_PRP_VF_ADC:
        break;
    case MEM_ROT_VF_MEM:
        _ipu_ic_uninit_rotate_vf();
        break;
    case CSI_PRP_ENC_MEM:
    case MEM_PRP_ENC_MEM:
        _ipu_ic_uninit_prpenc();
        break;
    case MEM_ROT_ENC_MEM:
        _ipu_ic_uninit_rotate_enc();
        break;
    case MEM_PP_ADC:
        reg = __raw_readl(IPU_FS_PROC_FLOW);
        __raw_writel(reg & ~FS_PP_DEST_SEL_MASK, IPU_FS_PROC_FLOW);

        _ipu_adc_uninit_channel(MEM_PP_ADC);

    /* Fall thru */
    case MEM_PP_MEM:
        _ipu_ic_uninit_pp();
        break;
    case MEM_ROT_PP_MEM:
        _ipu_ic_uninit_rotate_pp();
        break;

    case MEM_PF_Y_MEM:
        _ipu_pf_uninit();
        break;
    case MEM_PF_U_MEM:
    case MEM_PF_V_MEM:
        break;

    case MEM_SDC_BG:
        /*_ipu_sdc_bg_uninit(); */
        break;
    case MEM_SDC_FG:
        /*_ipu_sdc_fg_uninit(); */
        break;
    case ADC_SYS1:
        _ipu_adc_uninit_channel(ADC_SYS1);
        break;
    case ADC_SYS2:
        _ipu_adc_uninit_channel(ADC_SYS2);
        break;
    case MEM_SDC_MASK:
    case CHAN_NONE:
        break;
    }



    gChannelInitMask &= ~(1L << IPU_CHAN_ID(channel));

    ipu_conf = __raw_readl(IPU_CONF);
    if ((gChannelInitMask & 0x00000066L) == 0)	/*CSI */
    {
        ipu_conf &= ~IPU_CONF_CSI_EN;
    }
    if ((gChannelInitMask & 0x00001FFFL) == 0)	/*IC */
    {
        ipu_conf &= ~IPU_CONF_IC_EN;
    }
    if ((gChannelInitMask & 0x00000A10L) == 0)	/*ROT */
    {
        ipu_conf &= ~IPU_CONF_ROT_EN;
    }
    if ((gChannelInitMask & 0x0001C000L) == 0)	/*SDC */
    {
        ipu_conf &= ~IPU_CONF_SDC_EN;
    }
    if ((gChannelInitMask & 0x00061140L) == 0)	/*ADC */
    {
        ipu_conf &= ~IPU_CONF_ADC_EN;
    }
    if ((gChannelInitMask & 0x0007D140L) == 0)	/*DI */
    {
        ipu_conf &= ~IPU_CONF_DI_EN;
    }
    if ((gChannelInitMask & 0x00380000L) == 0)	/*PF */
    {
        ipu_conf &= ~IPU_CONF_PF_EN;
    }
    __raw_writel(ipu_conf, IPU_CONF);





#if defined(CONFIG_MACH_ARGONLVPHONE)
        if(gChannelInitMask == 0){
            ipu_go_to_sleep();
            mpm_driver_advise(ipu_mpm_num, MPM_ADVICE_DRIVER_IS_NOT_BUSY);
        }
#endif


        spin_unlock_irqrestore(&ipu_lock, lock_flags);
    FUNC_END;
}

static irqreturn_t ipu_4k_test_handler(int irq,void *dev_id,struct pt_regs *preg)
{
    volatile unsigned long buf_num =  ipu_reserved_buff_fiq[1200];
    volatile unsigned long reg=0;
    //disable SENSOR_EOF interrupts
    reg = __raw_readl(IPU_INT_CTRL_1)&(~0x00000080);
    __raw_writel(reg,IPU_INT_CTRL_1);

    // clear interrupt state for IDMA 7 EOF
    __raw_writel(0x00000080,IPU_INT_STAT_1);

    if(1)
    {
        if((buf_num < 1200) &&(buf_num >= 0) )
        {
            if(ipu_reserved_buff_fiq[buf_num] != NULL)
            {
                printk("buf_num=%d,buff=0x%x\n",buf_num,ipu_reserved_buff_fiq[buf_num]);
                if ((buf_num%2) == 0)
                {
#if 1
                    /* update physical address for idma buffer 0 */
                    __raw_writel((unsigned long)0x10078, IPU_IMA_ADDR);
                    __raw_writel((uint32_t) ipu_reserved_buff_fiq[buf_num], IPU_IMA_DATA);
#endif
                    /* set buffer 0 ready bit */
                    __raw_writel((unsigned long)0x00000080, IPU_CHA_BUF0_RDY);
                }
                else
                {
                    /* update physical address for idma buffer 1 */
#if 1
                    __raw_writel((unsigned long)0x10079, IPU_IMA_ADDR);
                    __raw_writel((uint32_t) ipu_reserved_buff_fiq[buf_num], IPU_IMA_DATA);

#endif
                    /* set buffer 1 ready bit */
                    __raw_writel((unsigned long)0x00000080, IPU_CHA_BUF1_RDY);
                }
                ipu_reserved_buff_fiq[1200] ++;
            }
            else
            {
                printk("!!!no physical buffer anymore\n");
            }
        }
        else
        {
            printk("!!!out of buf_num=%d\n",buf_num);
        }
    }
    else
    {
        printk("!!!!not expected interrupts\n");
    }
    //enable interrupt
    reg = __raw_readl(IPU_INT_CTRL_1)|(0x00000080);
    __raw_writel(reg,IPU_INT_CTRL_1);
    return IRQ_HANDLED;
}

/*!
 * This function is called to initialize a buffer for logical IPU channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       type            Input parameter which buffer to initialize.
 *
 * @param       pixel_fmt       Input parameter for pixel format of buffer. Pixel
 *                              format is a FOURCC ASCII code.
 *
 * @param       width           Input parameter for width of buffer in pixels.
 *
 * @param       height          Input parameter for height of buffer in pixels.
 *
 * @param       stride          Input parameter for stride length of buffer
 *                              in pixels.
 *
 * @param       rot_mode        Input parameter for rotation setting of buffer.
 *                              A rotation setting other than \b IPU_ROTATE_VERT_FLIP
 *                              should only be used for input buffers of rotation
 *                              channels.
 *
 * @param       phyaddr_0       Input parameter buffer 0 physical address.
 *
 * @param       phyaddr_1       Input parameter buffer 1 physical address.
 *                              Setting this to a value other than NULL enables
 *                              double buffering mode.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */
int32_t ipu_init_channel_buffer(ipu_channel_t channel, ipu_buffer_t type,
                uint32_t pixel_fmt,
                uint16_t width, uint16_t height,
                uint32_t stride,
                ipu_rotate_mode_t rot_mode,
                void *phyaddr_0, void *phyaddr_1)
{
    uint32_t params[10];
    uint32_t lock_flags;
    uint32_t reg;
    uint32_t dma_chan;
    uint32_t stride_bytes;

    FUNC_START;

    dma_chan = channel_2_dma(channel, type);
    stride_bytes = stride * bytes_per_pixel(pixel_fmt);

    if (dma_chan == IDMA_CHAN_INVALID)
        return -EINVAL;

    if (stride_bytes % 4) {
        printk
            ("Error: Stride length must be 32-bit aligned, stride = %d, bytes = %d\n",
             stride, stride_bytes);
        return -EINVAL;
    }
    /* IC channels' stride must be multiple of 8 pixels */
    if ((dma_chan <= 13) && (stride % 8)) {
        printk("Error: Stride must be 8 pixel multiple\n");
        return -EINVAL;
    }
    /* Build parameter memory data for DMA channel */
    _ipu_ch_param_set_size(params, pixel_fmt, width, height, stride_bytes);
    _ipu_ch_param_set_buffer(params, phyaddr_0, phyaddr_1);
    _ipu_ch_param_set_rotation(params, rot_mode);
    /* Some channels (rotation) have restriction on burst length */
    if ((dma_chan == 10) || (dma_chan == 11) || (dma_chan == 13)) {
        _ipu_ch_param_set_burst_size(params, 8);
    } else if (dma_chan == 24)  /* PF QP channel */
    {
        _ipu_ch_param_set_burst_size(params, 4);
    } else if (dma_chan == 25)  /* PF H264 BS channel */
    {
        _ipu_ch_param_set_burst_size(params, 16);
    } else if (((dma_chan == 14) || (dma_chan == 15)) &&
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
        /* The pixel packing definition for RGB565 is reversed */
        pixel_fmt == IPU_PIX_FMT_BGR565) {
#else /* defined(CONFIG_FB_MXC_BGR) */
        pixel_fmt == IPU_PIX_FMT_RGB565) {
#endif
        _ipu_ch_param_set_burst_size(params, 16);
    }
    else if(((dma_chan == 18) || (dma_chan == 19)) && pixel_fmt == IPU_PIX_FMT_GENERIC_16)
    {
       _ipu_ch_param_set_burst_size(params, 16);
    }

    spin_lock_irqsave(&ipu_lock, lock_flags);

    _ipu_write_param_mem(DMAParamAddr(dma_chan), params, 10);

    reg = __raw_readl(IPU_CHA_DB_MODE_SEL);
    if (phyaddr_1 != NULL) {
        reg |= 1UL << dma_chan;
    } else {
        reg &= ~(1UL << dma_chan);
    }
    __raw_writel(reg, IPU_CHA_DB_MODE_SEL);

    /* Reset to buffer 0 */
    __raw_writel(1UL << dma_chan, IPU_CHA_CUR_BUF);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);

/* for nevis using ipu reserved memory (CONFIG_MOT_FEAT_DISPLAY_EPSON is on) */
#ifndef CONFIG_MOT_FEAT_DISPLAY_EPSON
printk("channel=%d,ipu_init_channel_buffer\n",channel);
    if(channel == CSI_MEM)
    {
#if DBG_INT_CTRL
        spin_lock_irqsave(&ipu_lock, lock_flags);
        // clear interrupt state
        __raw_writel(0xFFFFFFFF, IPU_INT_STAT_1);
        __raw_writel(0xFFFFFFFF, IPU_INT_STAT_2);
        __raw_writel(0xFFFFFFFF, IPU_INT_STAT_3);
        __raw_writel(0xFFFFFFFF, IPU_INT_STAT_4);
        __raw_writel(0xFFFFFFFF, IPU_INT_STAT_5);

        //store and disable all of interrupts
        gCtlReg1 = __raw_readl(IPU_INT_CTRL_1);
        __raw_writel(0x00000000, IPU_INT_CTRL_1);
        gCtlReg2 = __raw_readl(IPU_INT_CTRL_2);
        __raw_writel(0x00000000, IPU_INT_CTRL_2);
        gCtlReg3 = __raw_readl(IPU_INT_CTRL_3);
        __raw_writel(0x00000000, IPU_INT_CTRL_3);
        gCtlReg4 = __raw_readl(IPU_INT_CTRL_4);
        __raw_writel(0x00000000, IPU_INT_CTRL_4);
        gCtlReg5 = __raw_readl(IPU_INT_CTRL_5);
        __raw_writel(0x00000000, IPU_INT_CTRL_5);

        spin_unlock_irqrestore(&ipu_lock, lock_flags);
#endif

                printk("ipu_reserved_buff_fiq=0x%x,%x,%x,%x,%x\n",ipu_reserved_buff_fiq,ipu_reserved_buff_fiq[0],ipu_reserved_buff_fiq[1],ipu_reserved_buff_fiq[2],ipu_reserved_buff_fiq[3]);
                
        //request irq&enable irq
        printk("free handler first\n");
        ipu_free_irq(IPU_IRQ_SENSOR_OUT_EOF,NULL);
        printk("request 4k handler\n");
        ipu_request_irq(IPU_IRQ_SENSOR_OUT_EOF,ipu_4k_test_handler,0,NULL,NULL);
#ifdef USE_FIQ_HANDLER
        printk("!!switch to fiq,disable->switch->enable\n");

        disable_irq(42);
        mxc_fiq_irq_switch(42, 1);
        enable_fiq(42);
#endif
    }
#endif /*CONFIG_MOT_FEAT_DISPLAY_EPSON*/

    FUNC_END;
    return 0;
}

/*!
 * This function is called to update the physical address of a buffer for
 * a logical IPU channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       type            Input parameter which buffer to initialize.
 *
 * @param       bufNum          Input parameter for which buffer number to update.
 *                              0 or 1 are the only valid values.
 *
 * @param       phyaddr         Input parameter buffer physical address.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail. This function will fail if the buffer is set to ready.
 */
int32_t ipu_update_channel_buffer(ipu_channel_t channel, ipu_buffer_t type,
                  uint32_t bufNum, void *phyaddr)
{
    uint32_t reg;
    uint32_t lock_flags;
    uint32_t dma_chan = channel_2_dma(channel, type);
        uint32_t timeout = 2500;

    FUNC_START;

    if (dma_chan == IDMA_CHAN_INVALID)
        return -EINVAL;

        /* wait for EOF for SDC BG or FG first */
    if (channel == MEM_SDC_BG) {
        spin_lock_irqsave(&ipu_lock, lock_flags);
        ipu_clear_irq(IPU_IRQ_SDC_BG_EOF);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);

        while (!ipu_get_irq_status(IPU_IRQ_SDC_BG_EOF)) {
                        udelay(10);
                        timeout--;
            if (timeout == 0)
                break;
        }
        /* wait for IPU_IRQ_SDC_FG_EOF first */
    } else if (channel == MEM_SDC_FG) {
        spin_lock_irqsave(&ipu_lock, lock_flags);
        ipu_clear_irq(IPU_IRQ_SDC_FG_EOF);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);

        while (!ipu_get_irq_status(IPU_IRQ_SDC_FG_EOF)) {
            udelay(10);
            timeout--;
            if (timeout == 0)
                break;
        }
    }

        spin_lock_irqsave(&ipu_lock, lock_flags);

    if (bufNum == 0) {
        reg = __raw_readl(IPU_CHA_BUF0_RDY);
        if (reg & (1UL << dma_chan)) {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);
            return -EACCES;
        }
        __raw_writel(DMAParamAddr(dma_chan) + 0x0008UL, IPU_IMA_ADDR);
        __raw_writel((uint32_t) phyaddr, IPU_IMA_DATA);
    } else {
        reg = __raw_readl(IPU_CHA_BUF1_RDY);
        if (reg & (1UL << dma_chan)) {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);
            return -EACCES;
        }
        __raw_writel(DMAParamAddr(dma_chan) + 0x0009UL, IPU_IMA_ADDR);
        __raw_writel((uint32_t) phyaddr, IPU_IMA_DATA);
    }

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    DPRINTK("IPU: update IDMA ch %d buf %d = 0x%08X\n", dma_chan, bufNum,
        phyaddr);
    return 0;
}

/*!
 * This function is called to update the physical address of a buffer for
 * a logical IPU channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       type            Input parameter which buffer to initialize.
 *
 * @param       bufNum          Input parameter for which buffer number to update.
 *                              0 or 1 are the only valid values.
 *
 * @param       phyaddr         Input parameter buffer physical address.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail. This function will fail if the buffer is set to ready.
 */
int32_t ipu_update_channel_yuvoffset_buffer(ipu_channel_t channel, ipu_buffer_t type, uint16_t u_offset, uint16_t v_offset)
{
    uint32_t lock_flags;
    uint32_t dma_chan = channel_2_dma(channel, type);
    uint32_t params[4];

    if (dma_chan == IDMA_CHAN_INVALID)
      return -EINVAL;

    spin_lock_irqsave(&ipu_lock, lock_flags);
    printk("u and v update u %d, v%d,\n", u_offset, v_offset);
    /* read from parameter memory */
    __raw_writel(DMAParamAddr(dma_chan) + 0x0001UL, IPU_IMA_ADDR);
    params[1] = __raw_readl(IPU_IMA_DATA);
    __raw_writel(DMAParamAddr(dma_chan) + 0x0002UL, IPU_IMA_ADDR);
    params[2] = __raw_readl(IPU_IMA_DATA);
    __raw_writel(DMAParamAddr(dma_chan) + 0x0003UL, IPU_IMA_ADDR);
    params[3] = __raw_readl(IPU_IMA_DATA);

    params[1]  &= 0x001fffff;
    params[2]  = 0;
    params[3]  &= 0xffffffe0;

    /* fill the offset */
    params[1]  |= u_offset << (53 - 32);
    params[2]  = u_offset >> (64 - 53);
    params[2]  |= v_offset << (79 - 64);
    params[3]  |= v_offset >> (96 - 79);

    /* write to parameter memory */
    __raw_writel(DMAParamAddr(dma_chan) + 0x0001UL, IPU_IMA_ADDR);
    __raw_writel(params[1], IPU_IMA_DATA);
    __raw_writel(DMAParamAddr(dma_chan) + 0x0002UL, IPU_IMA_ADDR);
    __raw_writel(params[2], IPU_IMA_DATA);
    __raw_writel(DMAParamAddr(dma_chan) + 0x0003UL, IPU_IMA_ADDR);
    __raw_writel(params[3], IPU_IMA_DATA);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    return 0;
}

/*!
 * This function is called to set a channel's buffer as ready.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       type            Input parameter which buffer to initialize.
 *
 * @param       bufNum          Input parameter for which buffer number set to
 *                              ready state.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */
int32_t ipu_select_buffer(ipu_channel_t channel, ipu_buffer_t type,
              uint32_t bufNum)
{
    uint32_t dma_chan = channel_2_dma(channel, type);

    FUNC_START;

    if (dma_chan == IDMA_CHAN_INVALID)
        return -EINVAL;

    if (bufNum == 0) {
        /*Mark buffer 0 as ready. */
        __raw_writel(1UL << dma_chan, IPU_CHA_BUF0_RDY);
    } else {
        /*Mark buffer 1 as ready. */
        __raw_writel(1UL << dma_chan, IPU_CHA_BUF1_RDY);
    }

    FUNC_END;
    return 0;
}

/*!
 * This function links 2 channels together for automatic frame
 * synchronization. The output of the source channel is linked to the input of
 * the destination channel.
 *
 * @param       src_ch          Input parameter for the logical channel ID of
 *                              the source channel.
 *
 * @param       dest_ch         Input parameter for the logical channel ID of
 *                              the destination channel.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int32_t ipu_link_channels(ipu_channel_t src_ch, ipu_channel_t dest_ch)
{
    uint32_t lock_flags;
    uint32_t out_dma;
    uint32_t in_dma;
    bool isProc;
    uint32_t value;
    uint32_t mask;
    uint32_t offset;
    uint32_t fs_proc_flow;
    uint32_t fs_disp_flow;

    FUNC_START;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    fs_proc_flow = __raw_readl(IPU_FS_PROC_FLOW);
    fs_disp_flow = __raw_readl(IPU_FS_DISP_FLOW);

    out_dma = (1UL << channel_2_dma(src_ch, IPU_OUTPUT_BUFFER));
    in_dma = (1UL << channel_2_dma(dest_ch, IPU_INPUT_BUFFER));

    /* PROCESS THE OUTPUT DMA CH */
    switch (out_dma) {
        /*VF-> */
    case IDMA_IC_1:
        PRINTK("Link VF->");
        isProc = true;
        mask = FS_PRPVF_DEST_SEL_MASK;
        offset = FS_PRPVF_DEST_SEL_OFFSET;
        value = (in_dma == IDMA_IC_11) ? FS_DEST_ROT :  /*->VF_ROT */
            (in_dma == IDMA_ADC_SYS1_WR) ? FS_DEST_ADC1 :   /* ->ADC1 */
            (in_dma == IDMA_ADC_SYS2_WR) ? FS_DEST_ADC2 :   /* ->ADC2 */
            (in_dma == IDMA_SDC_BG) ? FS_DEST_SDC_BG :  /*->SDC_BG */
            (in_dma == IDMA_SDC_FG) ? FS_DEST_SDC_FG :  /*->SDC_FG */
            (in_dma == IDMA_ADC_SYS1_WR) ? FS_DEST_ADC1 :   /*->ADC1 */
            /* ->ADCDirect */
            0;
        break;

        /*VF_ROT-> */
    case IDMA_IC_9:
        PRINTK("Link VF_ROT->");
        isProc = true;
        mask = FS_PRPVF_ROT_DEST_SEL_MASK;
        offset = FS_PRPVF_ROT_DEST_SEL_OFFSET;
        value = (in_dma == IDMA_ADC_SYS1_WR) ? FS_DEST_ADC1 :   /*->ADC1 */
            (in_dma == IDMA_ADC_SYS2_WR) ? FS_DEST_ADC2 :   /* ->ADC2 */
            (in_dma == IDMA_SDC_BG) ? FS_DEST_SDC_BG :  /*->SDC_BG */
            (in_dma == IDMA_SDC_FG) ? FS_DEST_SDC_FG :  /*->SDC_FG */
            0;
        break;

        /*ENC-> */
    case IDMA_IC_0:
        PRINTK("Link ENC->");
        isProc = true;
        mask = 0;
        offset = 0;
        value = (in_dma == IDMA_IC_10) ? FS_PRPENC_DEST_SEL :   /*->ENC_ROT */
            0;
        break;

        /*PP-> */
    case IDMA_IC_2:
        PRINTK("Link PP->");
        isProc = true;
        mask = FS_PP_DEST_SEL_MASK;
        offset = FS_PP_DEST_SEL_OFFSET;
        value = (in_dma == IDMA_IC_13) ? FS_DEST_ROT :  /*->PP_ROT */
            (in_dma == IDMA_ADC_SYS1_WR) ? FS_DEST_ADC1 :   /* ->ADC1 */
            (in_dma == IDMA_ADC_SYS2_WR) ? FS_DEST_ADC2 :   /* ->ADC2 */
            (in_dma == IDMA_SDC_BG) ? FS_DEST_SDC_BG :  /*->SDC_BG */
            (in_dma == IDMA_SDC_FG) ? FS_DEST_SDC_FG :  /*->SDC_FG */
            /* ->ADCDirect */
            0;
        break;

        /*PP_ROT-> */
    case IDMA_IC_12:
        PRINTK("Link PP_ROT->");
        isProc = true;
        mask = FS_PP_ROT_DEST_SEL_MASK;
        offset = FS_PP_ROT_DEST_SEL_OFFSET;
        value = (in_dma == IDMA_IC_5) ? FS_DEST_ROT :   /*->PP */
            (in_dma == IDMA_ADC_SYS1_WR) ? FS_DEST_ADC1 :   /* ->ADC1 */
            (in_dma == IDMA_ADC_SYS2_WR) ? FS_DEST_ADC2 :   /* ->ADC2 */
            (in_dma == IDMA_SDC_BG) ? FS_DEST_SDC_BG :  /*->SDC_BG */
            (in_dma == IDMA_SDC_FG) ? FS_DEST_SDC_FG :  /*->SDC_FG */
            0;
        break;

        /*PF-> */
    case IDMA_PF_Y_OUT:
    case IDMA_PF_U_OUT:
    case IDMA_PF_V_OUT:
        PRINTK("Link PF->");
        isProc = true;
        mask = FS_PF_DEST_SEL_MASK;
        offset = FS_PF_DEST_SEL_OFFSET;
        value = (in_dma == IDMA_IC_5) ? FS_PF_DEST_PP :
            (in_dma == IDMA_IC_13) ? FS_PF_DEST_ROT : 0;
        break;

        /* Invalid Chainings: ENC_ROT-> */
    default:
        PRINTK("Link Invalid->");
        value = 0;
        break;

    }

    if (value) {
        if (isProc) {
            fs_proc_flow &= ~mask;
            fs_proc_flow |= (value << offset);
        } else {
            fs_disp_flow &= ~mask;
            fs_disp_flow |= (value << offset);
        }
    } else {
        PRINTK("Invalid channel chaining %d -> %d\n", out_dma, in_dma);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
        return -EINVAL;
    }

    /* PROCESS THE INPUT DMA CH */
    switch (in_dma) {
        /* ->VF_ROT */
    case IDMA_IC_11:
        PRINTK("VF_ROT\n");
        isProc = true;
        mask = 0;
        offset = 0;
        value = (out_dma == IDMA_IC_1) ? FS_PRPVF_ROT_SRC_SEL : /*VF-> */
            0;
        break;

        /* ->ENC_ROT */
    case IDMA_IC_10:
        PRINTK("ENC_ROT\n");
        isProc = true;
        mask = 0;
        offset = 0;
        value = (out_dma == IDMA_IC_0) ? FS_PRPENC_ROT_SRC_SEL :    /*ENC-> */
            0;
        break;

        /* ->PP */
    case IDMA_IC_5:
        PRINTK("PP\n");
        isProc = true;
        mask = FS_PP_SRC_SEL_MASK;
        offset = FS_PP_SRC_SEL_OFFSET;
        value = (out_dma == IDMA_PF_Y_OUT) ? FS_PP_SRC_PF : /*PF-> */
            (out_dma == IDMA_PF_U_OUT) ? FS_PP_SRC_PF : /*PF-> */
            (out_dma == IDMA_PF_V_OUT) ? FS_PP_SRC_PF : /*PF-> */
            (out_dma == IDMA_IC_12) ? FS_PP_SRC_ROT :   /*PP_ROT-> */
            0;
        break;

        /* ->PP_ROT */
    case IDMA_IC_13:
        PRINTK("PP_ROT\n");
        isProc = true;
        mask = FS_PP_ROT_SRC_SEL_MASK;
        offset = FS_PP_ROT_SRC_SEL_OFFSET;
        value = (out_dma == IDMA_PF_Y_OUT) ? FS_PP_SRC_PF : /*PF-> */
            (out_dma == IDMA_PF_U_OUT) ? FS_PP_SRC_PF : /*PF-> */
            (out_dma == IDMA_PF_V_OUT) ? FS_PP_SRC_PF : /*PF-> */
            (out_dma == IDMA_IC_2) ? FS_ROT_SRC_PP :    /*PP-> */
            0;
        break;

        /* ->SDC_BG */
    case IDMA_SDC_BG:
        PRINTK("SDC_BG\n");
        isProc = false;
        mask = FS_SDC_BG_SRC_SEL_MASK;
        offset = FS_SDC_BG_SRC_SEL_OFFSET;
        value = (out_dma == IDMA_IC_9) ? FS_SRC_ROT_VF :    /*VF_ROT-> */
            (out_dma == IDMA_IC_12) ? FS_SRC_ROT_PP :   /*PP_ROT-> */
            (out_dma == IDMA_IC_1) ? FS_SRC_VF :    /*VF-> */
            (out_dma == IDMA_IC_2) ? FS_SRC_PP :    /*PP-> */
            0;
        break;

        /* ->SDC_FG */
    case IDMA_SDC_FG:
        PRINTK("SDC_FG\n");
        isProc = false;
        mask = FS_SDC_FG_SRC_SEL_MASK;
        offset = FS_SDC_FG_SRC_SEL_OFFSET;
        value = (out_dma == IDMA_IC_9) ? FS_SRC_ROT_VF :    /*VF_ROT-> */
            (out_dma == IDMA_IC_12) ? FS_SRC_ROT_PP :   /*PP_ROT-> */
            (out_dma == IDMA_IC_1) ? FS_SRC_VF :    /*VF-> */
            (out_dma == IDMA_IC_2) ? FS_SRC_PP :    /*PP-> */
            0;
        break;

        /* ->ADC1 */
    case IDMA_ADC_SYS1_WR:
        PRINTK("ADC_SYS1\n");
        isProc = false;
        mask = FS_ADC1_SRC_SEL_MASK;
        offset = FS_ADC1_SRC_SEL_OFFSET;
        value = (out_dma == IDMA_IC_9) ? FS_SRC_ROT_VF :    /*VF_ROT-> */
            (out_dma == IDMA_IC_12) ? FS_SRC_ROT_PP :   /*PP_ROT-> */
            (out_dma == IDMA_IC_1) ? FS_SRC_VF :    /*VF-> */
            (out_dma == IDMA_IC_2) ? FS_SRC_PP :    /*PP-> */
            0;
        break;

        /* ->ADC2 */
    case IDMA_ADC_SYS2_WR:
        PRINTK("ADC_SYS2\n");
        isProc = false;
        mask = FS_ADC2_SRC_SEL_MASK;
        offset = FS_ADC2_SRC_SEL_OFFSET;
        value = (out_dma == IDMA_IC_9) ? FS_SRC_ROT_VF :    /*VF_ROT-> */
            (out_dma == IDMA_IC_12) ? FS_SRC_ROT_PP :   /*PP_ROT-> */
            (out_dma == IDMA_IC_1) ? FS_SRC_VF :    /*VF-> */
            (out_dma == IDMA_IC_2) ? FS_SRC_PP :    /*PP-> */
            0;
        break;

        /*Invalid chains: */
        /* ->ENC, ->VF, ->PF, ->VF_COMBINE, ->PP_COMBINE*/
    default:
        PRINTK("Invalid\n");
        value = 0;
        break;

    }

    if (value) {
        if (isProc) {
            fs_proc_flow &= ~mask;
            fs_proc_flow |= (value << offset);
        } else {
            fs_disp_flow &= ~mask;
            fs_disp_flow |= (value << offset);
        }
    } else {
        DPRINTK("Invalid channel chaining %d -> %d\n", out_dma, in_dma);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
        return -EINVAL;
    }

    __raw_writel(fs_proc_flow, IPU_FS_PROC_FLOW);
    __raw_writel(fs_disp_flow, IPU_FS_DISP_FLOW);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    FUNC_END;
    return 0;
}

/*!
 * This function unlinks 2 channels and disables automatic frame
 * synchronization.
 *
 * @param       src_ch          Input parameter for the logical channel ID of
 *                              the source channel.
 *
 * @param       dest_ch         Input parameter for the logical channel ID of
 *                              the destination channel.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int32_t ipu_unlink_channels(ipu_channel_t src_ch, ipu_channel_t dest_ch)
{
    uint32_t lock_flags;
    uint32_t out_dma;
    uint32_t in_dma;
    uint32_t fs_proc_flow;
    uint32_t fs_disp_flow;

    FUNC_START;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    fs_proc_flow = __raw_readl(IPU_FS_PROC_FLOW);
    fs_disp_flow = __raw_readl(IPU_FS_DISP_FLOW);

    out_dma = (1UL << channel_2_dma(src_ch, IPU_OUTPUT_BUFFER));
    in_dma = (1UL << channel_2_dma(dest_ch, IPU_INPUT_BUFFER));

    /*clear the src_ch's output destination */
    switch (out_dma) {
        /*VF-> */
    case IDMA_IC_1:
        PRINTK("Unlink VF->");
        fs_proc_flow &= ~FS_PRPVF_DEST_SEL_MASK;
        break;

        /*VF_ROT-> */
    case IDMA_IC_9:
        PRINTK("Unlink VF_Rot->");
        fs_proc_flow &= ~FS_PRPVF_ROT_DEST_SEL_MASK;
        break;

        /*ENC-> */
    case IDMA_IC_0:
        PRINTK("Unlink ENC->");
        fs_proc_flow &= ~FS_PRPENC_DEST_SEL;
        break;

        /*PP-> */
    case IDMA_IC_2:
        PRINTK("Unlink PP->");
        fs_proc_flow &= ~FS_PP_DEST_SEL_MASK;
        break;

        /*PP_ROT-> */
    case IDMA_IC_12:
        PRINTK("Unlink PP_ROT->");
        fs_proc_flow &= ~FS_PP_ROT_DEST_SEL_MASK;
        break;

        /*PF-> */
    case IDMA_PF_Y_OUT:
    case IDMA_PF_U_OUT:
    case IDMA_PF_V_OUT:
        PRINTK("Unlink PF->");
        fs_proc_flow &= ~FS_PF_DEST_SEL_MASK;
        break;

    default:        /*ENC_ROT-> */
        PRINTK("Unlink Invalid->");
        break;
    }

    /*clear the dest_ch's input source */
    switch (in_dma) {
    /*->VF_ROT*/
    case IDMA_IC_11:
        PRINTK("VF_ROT\n");
        fs_proc_flow &= ~FS_PRPVF_ROT_SRC_SEL;
        break;

    /*->Enc_ROT*/
    case IDMA_IC_10:
        PRINTK("ENC_ROT\n");
        fs_proc_flow &= ~FS_PRPENC_ROT_SRC_SEL;
        break;

    /*->PP*/
    case IDMA_IC_5:
        PRINTK("PP\n");
        fs_proc_flow &= ~FS_PP_SRC_SEL_MASK;
        break;

    /*->PP_ROT*/
    case IDMA_IC_13:
        PRINTK("PP_ROT\n");
        fs_proc_flow &= ~FS_PP_ROT_SRC_SEL_MASK;
        break;

    /*->SDC_FG*/
    case IDMA_SDC_FG:
        PRINTK("SDC_FG\n");
        fs_disp_flow &= ~FS_SDC_FG_SRC_SEL_MASK;
        break;

    /*->SDC_BG*/
    case IDMA_SDC_BG:
        PRINTK("SDC_BG\n");
        fs_disp_flow &= ~FS_SDC_BG_SRC_SEL_MASK;
        break;

    /*->ADC1*/
    case IDMA_ADC_SYS1_WR:
        PRINTK("ADC_SYS1\n");
        fs_disp_flow &= ~FS_ADC1_SRC_SEL_MASK;
        break;

    /*->ADC2*/
    case IDMA_ADC_SYS2_WR:
        PRINTK("ADC_SYS2\n");
        fs_disp_flow &= ~FS_ADC2_SRC_SEL_MASK;
        break;

    default:        /*->VF, ->ENC, ->VF_COMBINE, ->PP_COMBINE, ->PF*/
        PRINTK("Invalid\n");
        break;
    }

    __raw_writel(fs_proc_flow, IPU_FS_PROC_FLOW);
    __raw_writel(fs_disp_flow, IPU_FS_DISP_FLOW);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    FUNC_END;
    return 0;
}

/*!
 * This function enables a logical channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int32_t ipu_enable_channel(ipu_channel_t channel)
{
    uint32_t reg;
    uint32_t lock_flags;
    uint32_t in_dma;
    uint32_t sec_dma;
    uint32_t out_dma;
    uint32_t chan_mask = 0;

    FUNC_START;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    reg = __raw_readl(IDMAC_CHA_EN);

    /* Get input and output dma channels */
    out_dma = channel_2_dma(channel, IPU_OUTPUT_BUFFER);
    if (out_dma != IDMA_CHAN_INVALID)
        reg |= 1UL << out_dma;
    in_dma = channel_2_dma(channel, IPU_INPUT_BUFFER);
    if (in_dma != IDMA_CHAN_INVALID)
        reg |= 1UL << in_dma;

    /* Get secondary input dma channel */
    if (gSecChanEn[IPU_CHAN_ID(channel)]) {
        sec_dma = channel_2_dma(channel, IPU_SEC_INPUT_BUFFER);
        if (sec_dma != IDMA_CHAN_INVALID) {
            reg |= 1UL << sec_dma;
        }
    }

    __raw_writel(reg | chan_mask, IDMAC_CHA_EN);

// need move to funct _ipu_ic_enable_task in ipu_ic.c
    uint32_t ic_conf, adc_conf;

    ic_conf = __raw_readl(IC_CONF);
    adc_conf = __raw_readl(ADC_CONF);
    
    switch (channel) {
        case CSI_PRP_VF_ADC:
        case CSI_PRP_VF_MEM:
        case MEM_PRP_VF_MEM:
            ic_conf |= IC_CONF_PRPVF_EN;
            break;
        case MEM_ROT_VF_MEM:
            ic_conf |= IC_CONF_PRPVF_ROT_EN;
            break;
        case CSI_PRP_ENC_MEM:
            ic_conf |= IC_CONF_PRPENC_EN;
            break;
        case MEM_PRP_ENC_MEM:
            ic_conf |= IC_CONF_PRPENC_EN;
            break;
        case MEM_ROT_ENC_MEM:
            ic_conf |= IC_CONF_PRPENC_ROT_EN;
            break;
        case MEM_PP_ADC:
        case MEM_PP_MEM:
            ic_conf |= IC_CONF_PP_EN;
            break;
        case MEM_ROT_PP_MEM:
            ic_conf |= IC_CONF_PP_ROT_EN;
            break;
        case CSI_MEM:
            ic_conf |= (IC_CONF_RWS_EN | IC_CONF_PRPENC_EN);
            break;

        case MEM_PF_Y_MEM:
        case MEM_PF_U_MEM:
        case MEM_PF_V_MEM:
            break;

        case MEM_SDC_BG:
            DPRINTK("Initializing SDC BG\n");
            _ipu_sdc_bg_init(NULL);
            break;
        case MEM_SDC_FG:
            DPRINTK("Initializing SDC FG\n");
            _ipu_sdc_fg_init(NULL);
            break;
        case ADC_SYS1:
/*            adc_conf |= 0x00030000; */
            break;
        case ADC_SYS2:
/*            adc_conf |= 0x03000000; */
            break;
        default:
            printk("Missing channel initialization\n");
            break;
            }
    __raw_writel(ic_conf, IC_CONF);
/*	__raw_writel(adc_conf, ADC_CONF);*/

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    FUNC_END;
    return 0;
}

/*!
 * This function disables a logical channel.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       wait_for_stop   Flag to set whether to wait for channel end
 *                              of frame or return immediately.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int32_t ipu_disable_channel(ipu_channel_t channel, bool wait_for_stop)
{
    uint32_t reg;
    uint32_t lock_flags;
    uint32_t sec_dma;
    uint32_t in_dma;
    uint32_t out_dma;
    uint32_t chan_mask = 0;
    uint32_t ipu_conf;
    uint32_t ipu_conf1;

    FUNC_START;

    /* Get input and output dma channels */
    out_dma = channel_2_dma(channel, IPU_OUTPUT_BUFFER);
    if (out_dma != IDMA_CHAN_INVALID)
        chan_mask = 1UL << out_dma;
    in_dma = channel_2_dma(channel, IPU_INPUT_BUFFER);
    if (in_dma != IDMA_CHAN_INVALID)
        chan_mask |= 1UL << in_dma;
    sec_dma = channel_2_dma(channel, IPU_SEC_INPUT_BUFFER);
    if (sec_dma != IDMA_CHAN_INVALID)
        chan_mask |= 1UL << sec_dma;

    if (wait_for_stop && channel != MEM_SDC_FG && channel != MEM_SDC_BG && channel != ADC_SYS2) {
        uint32_t timeout = 40;
        while ((__raw_readl(IDMAC_CHA_BUSY) & chan_mask) ||
               (_ipu_channel_status(channel) == TASK_STAT_ACTIVE)) {
            timeout--;
            msleep(10);
            if (timeout == 0) {
                printk
                    ("MXC IPU: Warning - timeout waiting for channel 0x%08x to stop,\n"
                     "\tbuf0_rdy = 0x%08X, buf1_rdy = 0x%08X\n"
                     "\tbusy = 0x%08X, tstat = 0x%08X\n\tmask = 0x%08X\n",
                     channel,
                     __raw_readl(IPU_CHA_BUF0_RDY),
                     __raw_readl(IPU_CHA_BUF1_RDY),
                     __raw_readl(IDMAC_CHA_BUSY),
                     __raw_readl(IPU_TASKS_STAT), chan_mask);
                break;
            }
        }
        DPRINTK("timeout = %d * 10ms\n", 40 - timeout);
    }
    /* SDC BG and FG must be disabled before DMA is disabled */
    if (channel == MEM_SDC_BG) {
        uint32_t timeout = 5;
        spin_lock_irqsave(&ipu_lock, lock_flags);
        ipu_clear_irq(IPU_IRQ_SDC_BG_EOF);
        if (_ipu_sdc_bg_uninit() && wait_for_stop) {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);

            while (!ipu_get_irq_status(IPU_IRQ_SDC_BG_EOF)) {
                mdelay(5);
                timeout--;
                if (timeout == 0)
                    break;
            }
        } else {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);
        }
    } else if (channel == MEM_SDC_FG) {
        uint32_t timeout = 5;
        spin_lock_irqsave(&ipu_lock, lock_flags);
        ipu_clear_irq(IPU_IRQ_SDC_FG_EOF);
        if (_ipu_sdc_fg_uninit() && wait_for_stop) {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);

            while (!ipu_get_irq_status(IPU_IRQ_SDC_FG_EOF)) {
                mdelay(5);
                timeout--;
                if (timeout == 0)
                    break;
            }
        } else {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);
        }
    } else if (channel == ADC_SYS2) {
        /* Wait for EOF before disable DMA */
        uint32_t timeout = 5;
        spin_lock_irqsave(&ipu_lock, lock_flags);
        ipu_clear_irq(IPU_IRQ_ADC_SYS2_EOF);
        if ( wait_for_stop) {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);

            while (!ipu_get_irq_status(IPU_IRQ_ADC_SYS2_EOF)) {
                mdelay(5);
                timeout--;
                if (timeout == 0){
                break;
                }
            }
        } else {
            spin_unlock_irqrestore(&ipu_lock, lock_flags);
        }
    }


    spin_lock_irqsave(&ipu_lock, lock_flags);

    /* Disable IC task */
    if (IPU_CHAN_ID(channel) <= IPU_CHAN_ID(MEM_PP_ADC)) {
        _ipu_ic_disable_task(channel);
    }


    /* Disable DMA channel(s) */
    reg = __raw_readl(IDMAC_CHA_EN);
    __raw_writel(reg & ~chan_mask, IDMAC_CHA_EN);



    /* Reset the double buffer */
    reg = __raw_readl(IPU_CHA_DB_MODE_SEL);
    __raw_writel(reg & ~chan_mask, IPU_CHA_DB_MODE_SEL);

    /* Reset to buffer 0 */
    __raw_writel(chan_mask, IPU_CHA_CUR_BUF);



    /* Clear DMA related interrupts */
    __raw_writel(chan_mask, IPU_INT_STAT_1);
    __raw_writel(chan_mask, IPU_INT_STAT_2);
    __raw_writel(chan_mask, IPU_INT_STAT_4);



    spin_unlock_irqrestore(&ipu_lock, lock_flags);

/* for nevis using ipu reserved memory (CONFIG_MOT_FEAT_DISPLAY_EPSON is on) */
#ifndef CONFIG_MOT_FEAT_DISPLAY_EPSON
#if DBG_INT_CTRL
    if(channel == CSI_MEM)
    {
#ifdef USE_FIQ_HANDLER
        printk("switch back to irq,disable->switch->enable\n");
        disable_fiq(42);
        mxc_fiq_irq_switch(42, 0);
        enable_irq(42);
#endif
        printk("free IPU_IRQ_SENSOR_OUT_EOF\n ");
        ipu_free_irq(IPU_IRQ_SENSOR_OUT_EOF,NULL);

        printk("!!restore Ctl Reg\n");
        spin_lock_irqsave(&ipu_lock, lock_flags);
        __raw_writel(gCtlReg1, IPU_INT_CTRL_1);
        __raw_writel(gCtlReg2, IPU_INT_CTRL_2);
        __raw_writel(gCtlReg3, IPU_INT_CTRL_3);
        __raw_writel(gCtlReg4, IPU_INT_CTRL_4);
        __raw_writel(gCtlReg5, IPU_INT_CTRL_5);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
    }
#endif
#endif /*CONFIG_MOT_FEAT_DISPLAY_EPSON*/

    /* Freescale suggest seq to clear ADC DI_ADC_LOCK_ERR */
    if(channel == ADC_SYS2 &&
       (_ipu_channel_status(channel) == TASK_STAT_ACTIVE) )
    {
        printk("ADC2 task active after disable, try to recover\n ");
        printk("tsat:0x%08X buf0_rdy:0x%08X ipu_conf:0x%08X\n",
                __raw_readl(IPU_TASKS_STAT), __raw_readl(IPU_CHA_BUF0_RDY), __raw_readl(IPU_CONF));
        spin_lock_irqsave(&ipu_lock, lock_flags);
        __raw_writel(((__raw_readl(IPU_CHA_BUF0_RDY)) & ~(chan_mask)), IPU_CHA_BUF0_RDY);
        ipu_conf = __raw_readl(IPU_CONF);
        ipu_conf1 = ipu_conf;
        ipu_conf &= ~IPU_CONF_ADC_EN;
        ipu_conf &= ~IPU_CONF_DI_EN;
        __raw_writel(ipu_conf, IPU_CONF);
        ipu_conf |= IPU_CONF_ADC_EN | IPU_CONF_DI_EN;
        __raw_writel(ipu_conf, IPU_CONF);
        __raw_writel(ipu_conf1, IPU_CONF); // restore org settings
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
        printk("ADC2 recovery done.");
        printk("tsat:0x%08X buf0_rdy:0x%08X ipu_conf:0x%08X\n",
                __raw_readl(IPU_TASKS_STAT), __raw_readl(IPU_CHA_BUF0_RDY), __raw_readl(IPU_CONF));
    }

    FUNC_END;
    return 0;
}

static
irqreturn_t ipu_irq_handler(int irq, void *desc, struct pt_regs *regs)
{
    uint32_t line_base = 0;
    uint32_t line;
    irqreturn_t result = IRQ_HANDLED;
    uint32_t int_stat;

    FUNC_START;

    if (g_ipu_irq[1]) {
        disable_irq(g_ipu_irq[0]);
        disable_irq(g_ipu_irq[1]);
    }

    int_stat = __raw_readl(IPU_INT_STAT_1);
    int_stat &= __raw_readl(IPU_INT_CTRL_1);
    // unclear interrupt state for IDMA 7 EOF, it will be cleared in 4k handler
        
#if defined(CONFIG_MACH_ELBA) || defined(CONFIG_MACH_PIANOSA)
    __raw_writel(int_stat & (~0x00000080), IPU_INT_STAT_1);
#else
    __raw_writel(int_stat, IPU_INT_STAT_1);
#endif
    while ((line = ffs(int_stat)) != 0) {
        int_stat &= ~(1UL << (line - 1));
        line += line_base - 1;
        DPRINTK("intr on line %d\n", line);
                if (NULL != ipu_irq_list[line].handler)
                {
            result |=
                ipu_irq_list[line].handler(line, ipu_irq_list[line].dev_id,
                               regs);
                }
    }

    line_base = 32;
    int_stat = __raw_readl(IPU_INT_STAT_2);
    int_stat &= __raw_readl(IPU_INT_CTRL_2);
    __raw_writel(int_stat, IPU_INT_STAT_2);
    while ((line = ffs(int_stat)) != 0) {
        int_stat &= ~(1UL << (line - 1));
        line += line_base - 1;
        DPRINTK("intr on line %d\n", line);
                if (NULL != ipu_irq_list[line].handler)
                {
            result |=
                ipu_irq_list[line].handler(line, ipu_irq_list[line].dev_id,
                               regs);
                }
    }

    line_base = 64;
    int_stat = __raw_readl(IPU_INT_STAT_3);
    int_stat &= __raw_readl(IPU_INT_CTRL_3);
    __raw_writel(int_stat, IPU_INT_STAT_3);
    while ((line = ffs(int_stat)) != 0) {
        int_stat &= ~(1UL << (line - 1));
        line += line_base - 1;
        DPRINTK("intr on line %d\n", line);
                if (NULL != ipu_irq_list[line].handler)
                {
            result |=
                ipu_irq_list[line].handler(line, ipu_irq_list[line].dev_id,
                               regs);
                }
    }

    line_base = 96;
    int_stat = __raw_readl(IPU_INT_STAT_4);
    int_stat &= __raw_readl(IPU_INT_CTRL_4);
    __raw_writel(int_stat, IPU_INT_STAT_4);
    while ((line = ffs(int_stat)) != 0) {
        int_stat &= ~(1UL << (line - 1));
        line += line_base - 1;
        DPRINTK("intr on line %d\n", line);
                if (NULL != ipu_irq_list[line].handler)
                {
            result |=
                ipu_irq_list[line].handler(line, ipu_irq_list[line].dev_id,
                               regs);
                }
    }

    line_base = 128;
    int_stat = __raw_readl(IPU_INT_STAT_5);
    int_stat &= __raw_readl(IPU_INT_CTRL_5);
    __raw_writel(int_stat, IPU_INT_STAT_5);
    while ((line = ffs(int_stat)) != 0) {
        int_stat &= ~(1UL << (line - 1));
        line += line_base - 1;
        DPRINTK("intr on line %d\n", line);
                if (NULL != ipu_irq_list[line].handler)
                {
            result |=
                ipu_irq_list[line].handler(line, ipu_irq_list[line].dev_id,
                               regs);
                }
    }
    FUNC_END;

#if !defined(CONFIG_ARCH_MXC91331) && !defined(CONFIG_ARCH_MXC91321)
    enable_irq(INT_IPU_SYN);
    enable_irq(INT_IPU_ERR);
#endif

    return result;
}

/*!
 * This function enables the interrupt for the specified interrupt line.
 * The interrupt lines are defined in \b ipu_irq_line enum.
 *
 * @param       irq             Interrupt line to enable interrupt for.
 *
 */
void ipu_enable_irq(uint32_t irq)
{
    uint32_t reg;
    uint32_t lock_flags;

    FUNC_START;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    reg = __raw_readl(IPUIRQ_2_CTRLREG(irq));
    reg |= IPUIRQ_2_MASK(irq);
    __raw_writel(reg, IPUIRQ_2_CTRLREG(irq));

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    FUNC_END;
}

/*!
 * This function disables the interrupt for the specified interrupt line.
 * The interrupt lines are defined in \b ipu_irq_line enum.
 *
 * @param       irq             Interrupt line to disable interrupt for.
 *
 */
void ipu_disable_irq(uint32_t irq)
{
    uint32_t reg;
    uint32_t lock_flags;

    FUNC_START;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    reg = __raw_readl(IPUIRQ_2_CTRLREG(irq));
    reg &= ~IPUIRQ_2_MASK(irq);
    __raw_writel(reg, IPUIRQ_2_CTRLREG(irq));

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    FUNC_END;
}

/*!
 * This function clears the interrupt for the specified interrupt line.
 * The interrupt lines are defined in \b ipu_irq_line enum.
 *
 * @param       irq             Interrupt line to clear interrupt for.
 *
 */
void ipu_clear_irq(uint32_t irq)
{
    FUNC_START;
    __raw_writel(IPUIRQ_2_MASK(irq), IPUIRQ_2_STATREG(irq));
    FUNC_END;
}

/*!
 * This function returns the current interrupt status for the specified interrupt
 * line. The interrupt lines are defined in \b ipu_irq_line enum.
 *
 * @param       irq             Interrupt line to get status for.
 *
 * @return      Returns true if the interrupt is pending/asserted or false if
 *              the interrupt is not pending.
 */
bool ipu_get_irq_status(uint32_t irq)
{
    uint32_t reg = __raw_readl(IPUIRQ_2_STATREG(irq));

    FUNC_START;

    if (reg & IPUIRQ_2_MASK(irq))
        return true;
    else
        return false;
}

/*!
 * This function registers an interrupt handler function for the specified
 * interrupt line. The interrupt lines are defined in \b ipu_irq_line enum.
 *
 * @param       irq             Interrupt line to get status for.
 *
 * @param       handler         Input parameter for address of the handler
 *                              function.
 *
 * @param       irq_flags       Flags for interrupt mode. Currently not used.
 *
 * @param       devname         Input parameter for string name of driver
 *                              registering the handler.
 *
 * @param       dev_id          Input parameter for pointer of data to be passed
 *                              to the handler.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 */
int ipu_request_irq(uint32_t irq,
            irqreturn_t(*handler) (int, void *, struct pt_regs *),
            uint32_t irq_flags, const char *devname, void *dev_id)
{
    uint32_t lock_flags;

    FUNC_START;

    MXC_ERR_CHK(irq >= IPU_IRQ_COUNT);

    spin_lock_irqsave(&ipu_lock, lock_flags);

    if (ipu_irq_list[irq].handler != NULL) {
        printk
            ("ipu_request_irq - handler already installed on irq %d\n",
             irq);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
        return -EINVAL;
    }

    ipu_irq_list[irq].handler = handler;
    ipu_irq_list[irq].flags = irq_flags;
    ipu_irq_list[irq].dev_id = dev_id;
    ipu_irq_list[irq].name = devname;

    spin_unlock_irqrestore(&ipu_lock, lock_flags);

    ipu_enable_irq(irq);    /* enable the interrupt */

    FUNC_END;
    return 0;
}

/*!
 * This function unregisters an interrupt handler for the specified interrupt
 * line. The interrupt lines are defined in \b ipu_irq_line enum.
 *
 * @param       irq             Interrupt line to get status for.
 *
 * @param       dev_id          Input parameter for pointer of data to be passed
 *                              to the handler. This must match value passed to
 *                              ipu_request_irq().
 *
 */
void ipu_free_irq(uint32_t irq, void *dev_id)
{
    FUNC_START;

    ipu_disable_irq(irq);   /* disable the interrupt */

    if (ipu_irq_list[irq].dev_id == dev_id) {
        ipu_irq_list[irq].handler = NULL;
    }
    FUNC_END;
}

/*!
 * This function sets the post-filter pause row for h.264 mode.
 *
 * @param       pause_row       The last row to process before pausing.
 *
 * @return      This function returns 0 on success or negative error code on
 *              fail.
 *
 */
int32_t ipu_pf_set_pause_row(uint32_t pause_row)
{
    int32_t retval = 0;
    uint32_t timeout = 5;
    uint32_t lock_flags;
    uint32_t reg;

    FUNC_START;

    reg = __raw_readl(IPU_TASKS_STAT);
    while ((reg & TSTAT_PF_MASK) && ((reg & TSTAT_PF_H264_PAUSE) == 0)) {
        timeout--;
        msleep(5);
        if (timeout == 0) {
            printk("PF Timeout - tstat = 0x%08X\n",
                   __raw_readl(IPU_TASKS_STAT));
            retval = -ETIMEDOUT;
            goto err0;
        }
    }

    spin_lock_irqsave(&ipu_lock, lock_flags);

    reg = __raw_readl(PF_CONF);

    /* Set the pause row */
    if (pause_row) {
        reg &= ~PF_CONF_PAUSE_ROW_MASK;
        reg |= PF_CONF_PAUSE_EN | pause_row << PF_CONF_PAUSE_ROW_SHIFT;
    } else {
        reg &= ~(PF_CONF_PAUSE_EN | PF_CONF_PAUSE_ROW_MASK);
    }
    __raw_writel(reg, PF_CONF);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
      err0:
    return retval;
}

/* Private functions */
void _ipu_write_param_mem(uint32_t addr, uint32_t * data, uint32_t numWords)
{
    for (; numWords > 0; numWords--) {
        DPRINTK("IPU WriteParamMem() - addr = 0x%08X, data = 0x%08X\n",
            addr, *data);
        __raw_writel(addr, IPU_IMA_ADDR);
        __raw_writel(*data++, IPU_IMA_DATA);
        addr++;
        if ((addr & 0x7) == 5) {
            addr &= ~0x7;   /* set to word 0 */
            addr += 8;  /* increment to next row */
        }
    }
}

static void _ipu_pf_init(ipu_channel_params_t * params)
{
    uint32_t reg;

    /*Setup the type of filtering required */
    switch (params->mem_pf_mem.operation) {
    case PF_MPEG4_DEBLOCK:
    case PF_MPEG4_DERING:
    case PF_MPEG4_DEBLOCK_DERING:
        gSecChanEn[IPU_CHAN_ID(MEM_PF_Y_MEM)] = true;
        gSecChanEn[IPU_CHAN_ID(MEM_PF_U_MEM)] = false;
        break;
    case PF_H264_DEBLOCK:
        gSecChanEn[IPU_CHAN_ID(MEM_PF_Y_MEM)] = true;
        gSecChanEn[IPU_CHAN_ID(MEM_PF_U_MEM)] = true;
        break;
    default:
        gSecChanEn[IPU_CHAN_ID(MEM_PF_Y_MEM)] = false;
        gSecChanEn[IPU_CHAN_ID(MEM_PF_U_MEM)] = false;
        return;
        break;
    }
    reg = params->mem_pf_mem.operation;
    __raw_writel(reg, PF_CONF);
}

static void _ipu_pf_uninit(void)
{
    __raw_writel(0x0L, PF_CONF);
    gSecChanEn[IPU_CHAN_ID(MEM_PF_Y_MEM)] = false;
    gSecChanEn[IPU_CHAN_ID(MEM_PF_U_MEM)] = false;
}

uint32_t _ipu_channel_status(ipu_channel_t channel)
{
    uint32_t stat = 0;
    uint32_t task_stat_reg = __raw_readl(IPU_TASKS_STAT);

    switch (channel) {
    case CSI_MEM:
        stat =
            (task_stat_reg & TSTAT_CSI2MEM_MASK) >>
            TSTAT_CSI2MEM_OFFSET;
        break;
    case CSI_PRP_VF_ADC:
    case CSI_PRP_VF_MEM:
    case MEM_PRP_VF_ADC:
    case MEM_PRP_VF_MEM:
        stat = (task_stat_reg & TSTAT_VF_MASK) >> TSTAT_VF_OFFSET;
        break;
    case MEM_ROT_VF_MEM:
        stat =
            (task_stat_reg & TSTAT_VF_ROT_MASK) >> TSTAT_VF_ROT_OFFSET;
        break;
    case CSI_PRP_ENC_MEM:
    case MEM_PRP_ENC_MEM:
        stat = (task_stat_reg & TSTAT_ENC_MASK) >> TSTAT_ENC_OFFSET;
        break;
    case MEM_ROT_ENC_MEM:
        stat =
            (task_stat_reg & TSTAT_ENC_ROT_MASK) >>
            TSTAT_ENC_ROT_OFFSET;
        break;
    case MEM_PP_ADC:
    case MEM_PP_MEM:
        stat = (task_stat_reg & TSTAT_PP_MASK) >> TSTAT_PP_OFFSET;
        break;
    case MEM_ROT_PP_MEM:
        stat =
            (task_stat_reg & TSTAT_PP_ROT_MASK) >> TSTAT_PP_ROT_OFFSET;
        break;

    case MEM_PF_Y_MEM:
    case MEM_PF_U_MEM:
    case MEM_PF_V_MEM:
        stat = (task_stat_reg & TSTAT_PF_MASK) >> TSTAT_PF_OFFSET;
        break;
    case MEM_SDC_BG:
        break;
    case MEM_SDC_FG:
        break;
    case ADC_SYS1:
        stat =
            (task_stat_reg & TSTAT_ADCSYS1_MASK) >>
            TSTAT_ADCSYS1_OFFSET;
        break;
    case ADC_SYS2:
        stat =
            (task_stat_reg & TSTAT_ADCSYS2_MASK) >>
            TSTAT_ADCSYS2_OFFSET;
        break;
    case MEM_SDC_MASK:
    default:
        stat = TASK_STAT_IDLE;
        break;
    }
    return stat;
}

uint32_t bytes_per_pixel(uint32_t fmt)
{
    switch (fmt) {
    case IPU_PIX_FMT_GENERIC:   /*generic data */
    case IPU_PIX_FMT_RGB332:
        return 1;
        break;
    case IPU_PIX_FMT_RGB565:
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
    case IPU_PIX_FMT_BGR565:
#endif
    case IPU_PIX_FMT_YUYV:
    case IPU_PIX_FMT_UYVY:
    case IPU_PIX_FMT_GENERIC_16:
        return 2;
        break;
    case IPU_PIX_FMT_BGR24:
    case IPU_PIX_FMT_RGB24:
    case IPU_PIX_FMT_BGRA6666:
        return 3;
        break;
    case IPU_PIX_FMT_GENERIC_32:    /*generic data */
    case IPU_PIX_FMT_BGR32:
    case IPU_PIX_FMT_RGB32:
    case IPU_PIX_FMT_ABGR32:
    case IPU_PIX_FMT_RGBA32:
    case IPU_PIX_FMT_BGRA32:
        return 4;
        break;
    default:
        return 1;
        break;
    }
    return 0;
}

ipu_color_space_t format_to_colorspace(uint32_t fmt)
{
    switch (fmt) {
    case IPU_PIX_FMT_RGB565:
#if defined(CONFIG_MOT_FEAT_FB_MXC_RGB)
    case IPU_PIX_FMT_BGR565:
#endif
    case IPU_PIX_FMT_BGR24:
    case IPU_PIX_FMT_BGRA6666:
    case IPU_PIX_FMT_RGB24:
    case IPU_PIX_FMT_BGR32:
    case IPU_PIX_FMT_RGB32:
        return RGB;
        break;

    default:
        return YCbCr;
        break;
    }
    return RGB;

}

#if defined(CONFIG_MACH_ARGONLVPHONE)
static void clear_int(void)
{
        /* save interrupt control register*/
        ipu_int_ctrl[0] = __raw_readl(IPU_INT_CTRL_1);
        ipu_int_ctrl[1] = __raw_readl(IPU_INT_CTRL_2);
        ipu_int_ctrl[2] = __raw_readl(IPU_INT_CTRL_3);
        ipu_int_ctrl[3] = __raw_readl(IPU_INT_CTRL_4);
        ipu_int_ctrl[4] = __raw_readl(IPU_INT_CTRL_5);

        /* disable all interrupt lines */
         __raw_writel(0, IPU_INT_CTRL_1);
         __raw_writel(0, IPU_INT_CTRL_2);
         __raw_writel(0, IPU_INT_CTRL_3);
         __raw_writel(0, IPU_INT_CTRL_4);
         __raw_writel(0, IPU_INT_CTRL_5);

        /*  clear interrupt state bit*/
         __raw_writel(0xffffffff, IPU_INT_STAT_1);
         __raw_writel(0xffffffff, IPU_INT_STAT_2);
         __raw_writel(0xffffffff, IPU_INT_STAT_3);
         __raw_writel(0xffffffff, IPU_INT_STAT_4);
         __raw_writel(0xffffffff, IPU_INT_STAT_5);

}

static void restore_int(void)
{
        /*  clear interrupt state bit*/
         __raw_writel(0xffffffff, IPU_INT_STAT_1);
         __raw_writel(0xffffffff, IPU_INT_STAT_2);
         __raw_writel(0xffffffff, IPU_INT_STAT_3);
         __raw_writel(0xffffffff, IPU_INT_STAT_4);
         __raw_writel(0xffffffff, IPU_INT_STAT_5);

        /* enable interrupt lines */
         __raw_writel(ipu_int_ctrl[0], IPU_INT_CTRL_1);
         __raw_writel(ipu_int_ctrl[1], IPU_INT_CTRL_2);
         __raw_writel(ipu_int_ctrl[2], IPU_INT_CTRL_3);
         __raw_writel(ipu_int_ctrl[3], IPU_INT_CTRL_4);
         __raw_writel(ipu_int_ctrl[4], IPU_INT_CTRL_5);
}

void ipu_go_to_sleep(void)
{
    uint32_t lock_flags;
    if(gChannelInitMask != 0)
    {
        DPRINTK(KERN_ALERT"ipu_common: ipu is in use, can't go to sleep!!!! \n");
        return;
    }
    if(ipu_in_sleep_flag == 1)
    {
        DPRINTK(KERN_ALERT"ipu_common: ipu has been sleepinp!!!! \n");
        return;
    }
   
    printk(KERN_ALERT"ipu_common: ipu sleeping -_- -_- -_- -_- -_- ~~~~~~~~ \n");

    spin_lock_irqsave(&ipu_lock, lock_flags);
    ipu_in_sleep_flag = 1;

    clear_int();
    // just to ensure CSI clock has been disabled.
    //ipu_csi_enable_mclk(CSI_MCLK_VF|CSI_MCLK_ENC|CSI_MCLK_RAW|CSI_MCLK_I2C, false, 1);

    mxc_clks_disable(IPU_CLK);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);

    return;
}

void ipu_wake_from_sleep(void)
{

    uint32_t lock_flags;

    if(ipu_in_sleep_flag == 0)
    {
        DPRINTK(KERN_ALERT" ipu_common: ipu is awake!!!!!!!!\n");
        return;
    }

    printk(KERN_ALERT"ipu_common: wakeup from sleeping *_* *_* *_* *_* *_* \n");

    spin_lock_irqsave(&ipu_lock, lock_flags);

    mxc_clks_enable(IPU_CLK);

    restore_int();

    ipu_in_sleep_flag = 0;

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
    return;
}

#endif

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxcipu_driver = {
    .name = "mxc_ipu",
    .bus = &platform_bus_type,
    .probe = ipu_probe,
};

/* Turn off SDC component in the IPU.
 * Turning off SDC when display is blanked saves an additional
 * 2mA current drain. These routines are called from display
 * blank and unblank routines
 * */
void ipu_disable_sdc(void)
{
    uint32_t lock_flags;
    uint32_t ipu_conf;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    ipu_conf = __raw_readl(IPU_CONF);
    ipu_conf &= ~IPU_CONF_SDC_EN;
    __raw_writel(ipu_conf, IPU_CONF);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

void ipu_enable_sdc(void)
{
    uint32_t lock_flags;
    uint32_t ipu_conf;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    ipu_conf = __raw_readl(IPU_CONF);
    ipu_conf |= IPU_CONF_SDC_EN;
    __raw_writel(ipu_conf, IPU_CONF);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

/* Turn the Display-Interface (DI) component off/on
 * to disable/enable pixel clock by writing to IPU_CONF
 * DI_EN is 7th bit in the least-significant byte of the
 * IPU register
 * */
void disable_ipu_pixel_clock(void)
{
    uint32_t ipu_conf;
    uint32_t lock_flags;
    spin_lock_irqsave(&ipu_lock, lock_flags);
    ipu_conf = readl(IPU_CONF);
    ipu_conf &= ~(0x00000040);
    writel(ipu_conf, IPU_CONF);
    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

void enable_ipu_pixel_clock(void)
{
    uint32_t ipu_conf;
    uint32_t lock_flags;
    spin_lock_irqsave(&ipu_lock, lock_flags);
    ipu_conf = readl(IPU_CONF);
    ipu_conf |= 0x00000040;
    writel(ipu_conf, IPU_CONF);
    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

/*Pull the output of SDC IPU signals low/high for HSYNCH, VSYNCH, DOTCLK by changing polarity
dotclk 0 Straight polarity.
       1 Inverse polarity.
vsynch 0 Active low.
       1 Active high.
hsynch 0 Active low.
       1 Active high.*/
void change_polarity_ipu_sdc_signals(int dotclk, int vsynch, int hsynch)
{
    uint32_t ipu_conf;
    uint32_t lock_flags;
    spin_lock_irqsave(&ipu_lock, lock_flags);
    ipu_conf = readl(DI_DISP_SIG_POL);
    if(dotclk==1)
    {
    ipu_conf &=~(0x02000000);/*dotclk low*/
    }
    else
    {
     ipu_conf |=(0x02000000);/*dotclk high, original setting in ini*/
    }
    if(vsynch==1)
    {
    ipu_conf |= (0x10000000);/*vsynch low*/
    }
    else
    {
    ipu_conf &=~(0x10000000);/*vsynch high, original setting in ini*/
    }
    if(hsynch==1)
    {
    ipu_conf |=(0x08000000);/*hsynch low*/
    }
    else
    {
     ipu_conf &=~(0x08000000);/*hsynch high, original setting in ini*/
    }
    writel(ipu_conf, DI_DISP_SIG_POL);
    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}


/* Locked version of _ipu_sdc_fg_uninit */
void ipu_sdc_fg_uninit(void)
{
        uint32_t lock_flags;
        uint32_t reg;
        spin_lock_irqsave(&ipu_lock, lock_flags);
        /* Disable FG channel */
        reg = __raw_readl(SDC_COM_CONF);
        __raw_writel(reg & ~SDC_COM_FG_EN, SDC_COM_CONF);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

/* Locked version of _ipu_sdc_fg_init */
void ipu_sdc_fg_init(void)
{
        uint32_t lock_flags;
        uint32_t reg;
        spin_lock_irqsave(&ipu_lock, lock_flags);
        /* Enable FG channel */
        reg = __raw_readl(SDC_COM_CONF);
        __raw_writel(reg | SDC_COM_FG_EN, SDC_COM_CONF);
        spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

#ifdef CONFIG_MOT_FEAT_2MP_CAMERA_WRKARND
/* IPU configuration for 2MP capture */
static unsigned long priority_setting;
void setup_dma_chan_priority(void)
{
  uint32_t lock_flags;
  spin_lock_irqsave(&ipu_lock, lock_flags);
  /* Set DMA7 channel as high priority */
  priority_setting = __raw_readl(IDMAC_CHA_PRI);
  __raw_writel(0x00000080L, IDMAC_CHA_PRI);
  spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

void restore_dma_chan_priority(void)
{
  uint32_t lock_flags;
  spin_lock_irqsave(&ipu_lock, lock_flags);
  /* Restore priority setting */
  __raw_writel(priority_setting, IDMAC_CHA_PRI);
  spin_unlock_irqrestore(&ipu_lock, lock_flags);
}
/* End of IPU configuration for 2MP capture */
#endif

int32_t setup_ipu_access_memory_priority(void)
{
    uint32_t m3ifctl_conf_reg_val;
    uint32_t lock_flags;
    
    spin_lock_irqsave(&ipu_lock, lock_flags);
    
    m3ifctl_conf_reg_val = __raw_readl(M3IFBASE);
    printk(KERN_ALERT " M3IFCTL =%08X \n",  m3ifctl_conf_reg_val);
    
    m3ifctl_conf_reg_val |=  0x04;
    __raw_writel(m3ifctl_conf_reg_val, M3IFBASE);
    printk(KERN_ALERT "Setup M3IFCTL =%08X \n", __raw_readl(M3IFBASE));
    
    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

int32_t restore_ipu_access_memory_priority(void)
{
    uint32_t m3ifctl_conf_reg_val;
    uint32_t lock_flags;
    
    spin_lock_irqsave(&ipu_lock, lock_flags);
    
    m3ifctl_conf_reg_val = __raw_readl(M3IFBASE);
    printk(KERN_ALERT " M3IFCTL =%08X \n",  m3ifctl_conf_reg_val);
    
    m3ifctl_conf_reg_val &= 0xFFFFFF00;
    __raw_writel(m3ifctl_conf_reg_val, M3IFBASE);
    printk(KERN_ALERT "Restore M3IFCTL =%08X \n", __raw_readl(M3IFBASE));
    
    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}


/* TEST DEBUG ONLY */
/*!
 * This function is called to force write 0 to a channel's buffer ready reg.
 *
 * @param       channel         Input parameter for the logical channel ID.
 *
 * @param       type            Input parameter which buffer to initialize.
 *
 * @param       bufNum          Input parameter for which buffer number set to
 *                              ready state.
 *
 * @return      This function returns 0 on success or negative error code on fail
 */
int32_t ipu_test_unselect_buffer(ipu_channel_t channel, ipu_buffer_t type,
              uint32_t bufNum)
{
    uint32_t dma_chan = channel_2_dma(channel, type);

    FUNC_START;

    if (dma_chan == IDMA_CHAN_INVALID)
        return -EINVAL;

    if (bufNum == 0) {
        /*Mark buffer 0 as not ready. */
        __raw_writel(((__raw_readl(IPU_CHA_BUF0_RDY)) & ~(1UL << dma_chan)), IPU_CHA_BUF0_RDY);
    } else {
        /*Mark buffer 1 as not ready. */
        __raw_writel(((__raw_readl(IPU_CHA_BUF1_RDY)) & ~(1UL << dma_chan)), IPU_CHA_BUF1_RDY);
    }

    FUNC_END;
    return 0;
}

void ipu_test_adc_enable(int enable)
{
    uint32_t lock_flags;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    if(enable)
        __raw_writel(((__raw_readl(IPU_CONF)) | IPU_CONF_ADC_EN), IPU_CONF);
    else
        __raw_writel(((__raw_readl(IPU_CONF)) & ~IPU_CONF_ADC_EN), IPU_CONF);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}

void ipu_test_di_enable(int enable)
{
    uint32_t lock_flags;

    spin_lock_irqsave(&ipu_lock, lock_flags);

    if(enable)
        __raw_writel(((__raw_readl(IPU_CONF)) | IPU_CONF_DI_EN), IPU_CONF);
    else
        __raw_writel(((__raw_readl(IPU_CONF)) & ~IPU_CONF_DI_EN), IPU_CONF);

    spin_unlock_irqrestore(&ipu_lock, lock_flags);
}
/* TEST DEBUG ONLY */

int32_t __init ipu_gen_init(void)
{
    int32_t ret;

    FUNC_START;

    ret = driver_register(&mxcipu_driver);

    FUNC_END;
    return 0;
}

subsys_initcall(ipu_gen_init);

static void __exit ipu_gen_uninit(void)
{
    FUNC_START;
#if defined(CONFIG_ARCH_MXC91331) || defined(CONFIG_ARCH_MXC91321)
    free_irq(INT_IPU, 0);
#else
    free_irq(INT_IPU_SYN, 0);
    free_irq(INT_IPU_ERR, 0);
#endif
    FUNC_END;
}

module_exit(ipu_gen_uninit);

EXPORT_SYMBOL(ipu_init_channel);
EXPORT_SYMBOL(ipu_uninit_channel);
EXPORT_SYMBOL(ipu_init_channel_buffer);
EXPORT_SYMBOL(ipu_unlink_channels);
EXPORT_SYMBOL(ipu_update_channel_buffer);
EXPORT_SYMBOL(ipu_update_channel_yuvoffset_buffer);
EXPORT_SYMBOL(ipu_select_buffer);
EXPORT_SYMBOL(ipu_link_channels);
EXPORT_SYMBOL(ipu_enable_channel);
EXPORT_SYMBOL(ipu_disable_channel);
EXPORT_SYMBOL(ipu_enable_irq);
EXPORT_SYMBOL(ipu_disable_irq);
EXPORT_SYMBOL(ipu_clear_irq);
EXPORT_SYMBOL(ipu_get_irq_status);
EXPORT_SYMBOL(ipu_request_irq);
EXPORT_SYMBOL(ipu_free_irq);
EXPORT_SYMBOL(ipu_pf_set_pause_row);
EXPORT_SYMBOL(ipu_sdc_fg_uninit);
EXPORT_SYMBOL(ipu_sdc_fg_init);
EXPORT_SYMBOL(setup_ipu_access_memory_priority);
EXPORT_SYMBOL(restore_ipu_access_memory_priority);
#ifdef CONFIG_MOT_FEAT_2MP_CAMERA_WRKARND
EXPORT_SYMBOL(setup_dma_chan_priority);
EXPORT_SYMBOL(restore_dma_chan_priority);
#endif
#if defined(CONFIG_MACH_ARGONLVPHONE)
EXPORT_SYMBOL(ipu_go_to_sleep);
EXPORT_SYMBOL(ipu_wake_from_sleep);
#endif

/* TEST DEBUG ONLY */
EXPORT_SYMBOL(ipu_test_unselect_buffer);
EXPORT_SYMBOL(ipu_test_adc_enable);
EXPORT_SYMBOL(ipu_test_di_enable);

