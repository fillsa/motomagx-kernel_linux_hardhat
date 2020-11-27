/*
 * Copyright 2004 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_ir.c
 *
 * @brief Driver for the Freescale Semiconductor MXC FIRI.
 *
 * This driver is based on drivers/net/irda/sa1100_ir.c, by Russell King.
 *
 * @ingroup FIRI
 */

/*
 * Include Files
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>

#include <net/irda/irda.h>
#include <net/irda/wrapper.h>
#include <net/irda/irda_device.h>

#include <asm/irq.h>
#include <asm/dma.h>
#include <asm/hardware.h>
#include <asm/arch/platform.h>
#include <asm/arch/gpio.h>
#include <asm/arch/spba.h>
#include <asm/arch/mxc_uart.h>
#include <asm/arch/clock.h>
#include "mxc_ir.h"
#include "../../serial/mxc_uart_reg.h"

#define SIR_MODE 0x1
#define MIR_MODE 0x2
#define FIR_MODE 0x4
#define IS_SIR(mi)		( (mi)->speed < 576000 )
#define IS_MIR(mi)		( (mi)->speed < 4000000 && (mi)->speed >= 576000 )
#define IS_FIR(mi)		( (mi)->speed >= 4000000 )

#define ENABLE  1
#define DISABLE 0

#define IRDA_FRAME_SIZE_LIMIT   2047


#ifdef IRDA_DEBUG
#undef IRDA_DEBUG
#endif

#ifdef MESSAGE
#undef MESSAGE
#endif


//#define TEST
#ifdef TEST
char receive_fifo[2048];
int receive_number=0;
#define IRDA_DEBUG(n, args...) (printk(args))
#define MESSAGE(args...) (printk(args))
#else 
#define IRDA_DEBUG(n, args...)  
#define MESSAGE(args...)
#endif

/*!
 * This structure is a way for the low level driver to define their own
 * \b mxc_irda structure. This structure includes SK buffers, DMA buffers.
 * and has other elements that are specifically required by this driver.
 */
struct mxc_irda{
        /*!
         * This keeps track of device is running or not
         */
        unsigned char           open;

        /*!
         * This holds current FIRI communication speed
         */
        int                     speed;

        /*!
         * This holds FIRI communication speed for next packet
         */
        int                     newspeed;
        
        /*!
         * SK buffer for transmitter
         */
        struct sk_buff          *txskb;

        /*!
         * SK buffer for receiver 
         */
        struct sk_buff          *rxskb;

        unsigned char           *dma_rx_buff;
        unsigned char           *dma_tx_buff;

        /*!
         * DMA address for transmitter
         */
        dma_addr_t              dma_rx_buff_phy;

        /*!
         * DMA address for receiver
         */
        dma_addr_t              dma_tx_buff_phy;

        unsigned int            dma_tx_buff_len;

        /*!
         * DMA channel for transmitter
         */
        int                     txdma_ch;
       
        /*!
         * DMA channel for receiver
         */
        int                     rxdma_ch;

        /*!
         * IrDA network device statistics
         */
        struct net_device_stats stats;

        /*!
         * The device structure used to get FIRI information
         */
        struct device           *dev;

         /*!
          * Resource structure for UART, which will maintain base addresses and IRQs.
          */
        struct resource         *uart_res;

        /*!
         * Base address of UART, used in readl and writel.
         */
        void                    *uart_base;

         /*!
          * Resource structure for FIRI, which will maintain base addresses and IRQs.
          */
        struct resource         *firi_res;

        /*!
         * Base address of FIRI, used in readl and writel.
         */
        void                    *firi_base;

        /*!
         * UART IRQ number.
         */
        int                     uart_irq;

        unsigned int            uartclk;

        /*!
         * FIRI IRQ number.
         */
        int                     firi_irq;

        /*!
         * IrLAP layer instance
         */
        struct irlap_cb         *irlap;

        /*!
         * Driver supported baudrate capabilities
         */
        struct qos_info         qos;

        iobuff_t                tx_buff;
        iobuff_t                rx_buff;

};

extern void gpio_uart_active(int port, int no_irda);
extern void gpio_uart_inactive(int port, int no_irda);

static void mxc_irda_fir_dma_rx_irq(void *);
static void mxc_irda_fir_dma_tx_irq(void *);
static struct resource mxcir_resources[4];

/*!
 * This function allocates and maps the receive buffer, 
 * unless it is already allocated.
 *
 * @param   si   FIRI device specific structure.
 * @return  The function returns 0 on success and a non-zero value on
 *          failure.
 */
static int mxc_irda_rx_alloc(struct mxc_irda *si)
{
        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        if (si->rxskb) {
                return 0;
        }

        si->rxskb = alloc_skb(FIR_MAX_RXLEN + 1, GFP_ATOMIC);

        if (!si->rxskb) {
                ERROR("mxc_ir: out of memory for RX SKB\n");
                return -ENOMEM;
        }

        /*
         * Align any IP headers that may be contained
         * within the frame.
         */
        skb_reserve(si->rxskb, 1);

        si->dma_rx_buff_phy = dma_map_single(si->dev, si->rxskb->data,
                                       FIR_MAX_RXLEN, DMA_FROM_DEVICE);
        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return 0;
}

/*!
 * This function is called to configure and start RX DMA channel
 * This uses the existing buffer.
 *
 * @param   si   FIRI device specific structure.
 */
static void mxc_irda_fir_dma_rx_start(struct mxc_irda *si)
{
        dma_request_t dma_cfg;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);

        /* 
         * Enable the DMA, 
         * Configure DMA RX Channel for source and destination addresses,
         * Number of bytes in RX FIFO and Reception complete
         * callback function.
         */
        dma_cfg.sourceAddr = (__u8*)(FIRI_BASE_ADDR + 0x18);
        dma_cfg.destAddr = (__u8*)si->dma_rx_buff_phy;
        dma_cfg.count =   IRDA_FRAME_SIZE_LIMIT;
        //dma_cfg.callback = mxc_irda_fir_dma_rx_irq;
        mxc_dma_set_config(si->rxdma_ch, &dma_cfg, 0);
        mxc_dma_start(si->rxdma_ch);

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
}

static void mxc_irda_fir_dma_tx_start(struct mxc_irda *si)
{
        dma_request_t dma_cfg;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        /*
         * Configure DMA Tx Channel for source and destination addresses,
         * Number of bytes in SK buffer to transfer and Transfer complete
         * callback function.
         */
        dma_cfg.sourceAddr = (__u8*)si->dma_tx_buff_phy;
        dma_cfg.destAddr = (__u8*) (FIRI_BASE_ADDR + 0x14);
        dma_cfg.count =   si->dma_tx_buff_len;
        //dma_cfg.callback = mxc_irda_fir_dma_tx_irq;
        mxc_dma_set_config(si->txdma_ch, &dma_cfg, 0);
        mxc_dma_start(si->txdma_ch);

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
}



/*!
 * This function is called to set the IrDA communications speed.
 *
 * @param   si     FIRI specific structure.
 * @param   speed  new Speed to be configured for.
 *
 * @returns The function returns \b -EINVAL on failure, else it returns 0
 */
static int mxc_irda_set_speed(struct mxc_irda *si, int speed)
{
        unsigned long flags;
        int ret = -EINVAL;
        unsigned int num, denom, baud;
        unsigned int cr, sr1, sr2;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);

        /* clear TX and RX status registers */
        //FIRITSR = 0xff;
        //FIRIRSR = 0xff;

        /* Burst length = 32 bytes */
        //FIRICR = 32 << 5;
        //FIRITCR = FIRITCR_TCIE | FIRITCR_TPEIE | FIRITCR_TFUIE;
        //FIRITCTR = FIRITCTR_TPL;
        //FIRIRCR = FIRIRCR_RPEDE | FIRIRCR_RPEIE | FIRIRCR_RFOIE | FIRIRCR_PAIE;

        switch (speed) {
        case 9600:      case 19200:     case 38400:
        case 57600:     case 115200:
                baud = speed;
                /* stop RX DMA */
                if (IS_FIR(si))
                        mxc_dma_stop(si->rxdma_ch);
                local_irq_save(flags);

                //Disable Tx and Rx
                cr = readl(si->uart_base + MXC_UARTUCR2);
                cr &= ~(MXC_UARTUCR2_RXEN | MXC_UARTUCR2_TXEN);
                writel(cr, si->uart_base + MXC_UARTUCR2);

                /* Disable the Transmit complete interrupt bit */
                cr = readl(si->uart_base + MXC_UARTUCR4);
                cr &= ~MXC_UARTUCR4_TCEN;
                writel(cr, si->uart_base + MXC_UARTUCR4);

                //Disable Parity error interrupt.
                //Disable Frame error interrupt.
                cr = readl(si->uart_base + MXC_UARTUCR3);
                cr &= ~(MXC_UARTUCR3_PARERREN | MXC_UARTUCR3_FRAERREN);
                writel(cr, si->uart_base + MXC_UARTUCR3);

                //Disable Receive Overrun and Data Ready interrupts.
                cr = readl(si->uart_base + MXC_UARTUCR4);
                cr &= ~(MXC_UARTUCR4_OREN | MXC_UARTUCR4_DREN);
                writel(cr, si->uart_base + MXC_UARTUCR4);

                cr = readl(si->uart_base + MXC_UARTUCR2);
                cr &= ~MXC_UARTUCR2_SRST;
                writel(cr, si->uart_base + MXC_UARTUCR2);

                num = baud / 100 - 1;
                denom = si->uartclk / 1600 - 1;
                if ((denom < 65536) && (si->uartclk > 1600)) {
                       writel(num, si->uart_base + MXC_UARTUBIR);
                        writel(denom, si->uart_base + MXC_UARTUBMR);
                }

                printk("UBIR:%x\n", readl( si->uart_base + MXC_UARTUBIR));
                printk("UBMR:%x\n", readl( si->uart_base + MXC_UARTUBMR));
                printk("baud %d\n", baud);

                sr2 = readl(si->uart_base + MXC_UARTUSR2);
                sr2 |= MXC_UARTUSR2_ORE;
                writel(sr2, si->uart_base + MXC_UARTUSR2);

                sr1 = readl(si->uart_base + MXC_UARTUSR1);
                sr1 |= (MXC_UARTUSR1_FRAMERR | MXC_UARTUSR1_PARITYERR);
                writel(sr1, si->uart_base + MXC_UARTUSR1);

                //Enable Parity error interrupt.
                //Enable Frame error interrupt.
                cr = readl(si->uart_base + MXC_UARTUCR3);
                cr |= (MXC_UARTUCR3_PARERREN | MXC_UARTUCR3_FRAERREN);
                writel(cr, si->uart_base + MXC_UARTUCR3);

                //Enable Receive Overrun and Data Ready interrupts.
                cr = readl(si->uart_base + MXC_UARTUCR4);
                cr |= (MXC_UARTUCR4_OREN | MXC_UARTUCR4_DREN);
                writel(cr, si->uart_base + MXC_UARTUCR4);
                        
                cr = readl(si->uart_base + MXC_UARTUCR2);
                cr |= MXC_UARTUCR2_RXEN /*| MXC_UARTUCR2_TXEN*/;
                writel(cr, si->uart_base + MXC_UARTUCR2);

                local_irq_restore(flags);
                ret = 0;
                break;

        case 576000:
                /* TODO:configure Clock Controller and Over Sampling Factor */
                FIRITCR = FIRITCR | FIRITCR_SRF_MIR | FIRITCR_TDT_MIR |
                          FIRITCR_TM_MIR1;
                FIRIRCR = FIRIRCR | FIRIRCR_RDT_MIR | FIRIRCR_RM_MIR1;
                ret = 0;
                break;
                
        case 1152000:

                /*
                 * TODO:configure Clock Controller and Over Sampling Factor
                 * accordingly
                 */
                FIRITCR = FIRITCR | FIRITCR_SRF_MIR | FIRITCR_TDT_MIR | 
                          FIRITCR_TM_MIR2;
                FIRIRCR = FIRIRCR | FIRIRCR_RDT_MIR | FIRIRCR_RM_MIR1;
                ret = 0;
                break;
                
        case 4000000:

                /*
                 * TODO:configure Clock Controller and Over Sampling Factor
                 * accordingly
                 */
                FIRITCR = FIRITCR | FIRITCR_SRF_FIR | FIRITCR_TDT_FIR | 
                          FIRITCR_TM_FIR;
                FIRIRCR = FIRIRCR | FIRIRCR_RDT_FIR | FIRIRCR_RM_FIR;
                ret = 0;
                ret = mxc_irda_rx_alloc(si);
                mxc_irda_fir_dma_rx_start(si);
                break;
        default:
                printk("speed not supported by FIRI\n");
                break;
        }
        si->speed = speed;

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return ret;
}


static irqreturn_t mxc_irda_sir_irq(int irq, void *dev_id, struct pt_regs *regs)
{
        struct net_device *dev = dev_id;
        struct mxc_irda *si = dev->priv;
	unsigned long tx_fifo_empty;
        unsigned int sr1, sr2, cr;
        unsigned int data, status;

        IRDA_DEBUG(KERN_INFO, "IN %s():\n", __FUNCTION__);

        sr1 = readl(si->uart_base + MXC_UARTUSR1);
        sr2 = readl(si->uart_base + MXC_UARTUSR2);
#ifdef TEST
        printk("USR1:%x\n", sr1);
        printk("USR2:%x\n", sr2);
#endif
        cr = readl(si->uart_base + MXC_UARTUCR2);
        if( (cr & MXC_UARTUCR2_RXEN) && (cr & MXC_UARTUCR2_TXEN))
        {
                printk("rx and tx.\n");
        }
        if( readl(si->uart_base + MXC_UARTUCR2) & MXC_UARTUCR2_RXEN) {
                /* Clear the bits that triggered the interrupt
                 * */
                writel(sr1, si->uart_base + MXC_UARTUSR1);
                writel(sr2, si->uart_base + MXC_UARTUSR2);

                sr2 = readl(si->uart_base + MXC_UARTUSR2);
                while ((sr2 & MXC_UARTUSR2_RDR) == 1){
                        data = readl(si->uart_base + MXC_UARTURXD);
                        status = data & 0xf400;
                        if(status & MXC_UARTURXD_ERR) { //ERROR 
                                printk(KERN_ERR "Receive an incorrect data.=0x%x.\n",data);
                                si->stats.rx_errors++;
                                if ( status & 0x2000 ) //overrun
                                {
                                        si->stats.rx_fifo_errors++;
                                        sr2 = readl(si->uart_base + MXC_UARTUSR2);
                                        sr2 |= MXC_UARTUSR2_ORE;
                                        writel(sr2, si->uart_base + MXC_UARTUSR2);
                                        printk(KERN_ERR "Rx overrun.\n");
                                }
                                if ( status & 0x1000 ) //frame error
                                {
                                        si->stats.rx_frame_errors++;
                                        sr1 = readl(si->uart_base + MXC_UARTUSR1);
                                        sr1 |= MXC_UARTUSR1_FRAMERR;
                                        writel(sr1, si->uart_base + MXC_UARTUSR1);
                                        printk(KERN_ERR "Rx frame error.\n");
                                }
                                if ( status & 0x400 ) //parity
                                {
                                        sr1 = readl(si->uart_base + MXC_UARTUSR1);
                                        sr1 |= MXC_UARTUSR1_PARITYERR;
                                        writel(sr1, si->uart_base + MXC_UARTUSR1);
                                        printk(KERN_ERR "Rx parity error.\n");
                                }
                                /* Other: it is the Break char.
                                 * Do nothing for it. throw out the data.
                                 */
                                async_unwrap_char(dev, &si->stats, &si->rx_buff, (data&0xff));
                        }
                        else
                        {
                                // it is a correct data.
                                data &= 0xff;
#ifdef TEST
                                receive_fifo[receive_number++]=data;
#endif
                                async_unwrap_char(dev, &si->stats, &si->rx_buff, data);

                                dev->last_rx = jiffies;
                        }
                        sr2 = readl(si->uart_base + MXC_UARTUSR2);
                }//while
        }
        if (readl(si->uart_base + MXC_UARTUCR2) & MXC_UARTUCR2_TXEN) {
                //printk("TX\n");
                //interrupt handler for SIR TX.
                sr2 = readl(si->uart_base + MXC_UARTUSR2);
                tx_fifo_empty = sr2 & MXC_UARTUSR2_TXFE;
                if (tx_fifo_empty && si->tx_buff.len) 
                {
                        //DUMP_MSG("sir tx interrupt.\n");
                        do {
                                writel(*si->tx_buff.data++, si->uart_base + MXC_UARTUTXD);  
                                si->tx_buff.len -= 1;
                                sr1 = readl(si->uart_base + MXC_UARTUSR1);
                        } while ((sr1 & MXC_UARTUSR1_TRDY) && si->tx_buff.len);

                        if (si->tx_buff.len == 0)
                        {
                                si->stats.tx_packets++;
                                si->stats.tx_bytes += si->tx_buff.data -
                                        si->tx_buff.head;

                                /*
                                 * We need to wait for a while to make sure 
                                 * the transmitter has finished.
                                 */
                                do {
                                        status = readl(si->uart_base + MXC_UARTUSR2);
                                }while (!(status & MXC_UARTUSR2_TXDC));

                                /*
                                 * Ok, we've finished transmitting.  Now enable
                                 * the receiver.  Sometimes we get a receive IRQ
                                 * immediately after a transmit...
                                 *			 
                                 * Before start Rx, we should disable the Tx interrupt
                                 * because Tx FIFO empty will lead to another interrupt.
                                 */
                                /* Disable the Transmit complete interrupt bit */
                                cr = readl(si->uart_base + MXC_UARTUCR4);
                                cr &= ~MXC_UARTUCR4_TCEN;
                                writel(cr, si->uart_base + MXC_UARTUCR4);

                                cr = readl(si->uart_base + MXC_UARTUCR2);
                                cr &= ~MXC_UARTUCR2_TXEN;
                                writel(cr, si->uart_base + MXC_UARTUCR2);

                                //Clear Rx status.
                                sr1 = readl(si->uart_base + MXC_UARTUSR1);
                                sr1 |= (MXC_UARTUSR1_PARITYERR | MXC_UARTUSR1_FRAMERR);
                                writel(sr1, si->uart_base + MXC_UARTUSR1);
                                
                                sr2 = readl(si->uart_base + MXC_UARTUSR2);
                                sr2 |= MXC_UARTUSR2_ORE;
                                writel(sr2, si->uart_base + MXC_UARTUSR2);


                                //Enable Rx interrupt
                                cr = readl(si->uart_base + MXC_UARTUCR3);
                                cr |= (MXC_UARTUCR3_PARERREN | MXC_UARTUCR3_FRAERREN);
                                writel(cr, si->uart_base + MXC_UARTUCR3);

                                //Enable Receive Overrun and Data Ready interrupts.
                                cr = readl(si->uart_base + MXC_UARTUCR4);
                                cr |= (MXC_UARTUCR4_OREN | MXC_UARTUCR4_DREN);
                                writel(cr, si->uart_base + MXC_UARTUCR4);

                                //Enable Tx and Rx.
#if 0
                                enable_sir_tx;
#else
                                cr = readl(si->uart_base + MXC_UARTUCR2);
                                cr &= ~MXC_UARTUCR2_TXEN;
                                writel(cr, si->uart_base + MXC_UARTUCR2);
#endif
                                cr = readl(si->uart_base + MXC_UARTUCR2);
                                cr |= MXC_UARTUCR2_RXEN;
                                writel(cr, si->uart_base + MXC_UARTUCR2);

                                if (si->newspeed)
                                {
                                        printk("\n");
                                        mxc_irda_set_speed(si, si->newspeed);
                                        si->newspeed = 0;
                                }
                                /* I'm hungry! */
                                netif_wake_queue(dev);
                        }
                }

                if (si->tx_buff.len == 0)
                {
                        /* Disable the Transmit complete interrupt bit */
                        cr = readl(si->uart_base + MXC_UARTUCR4);
                        cr &= ~MXC_UARTUCR4_TCEN;
                        writel(cr, si->uart_base + MXC_UARTUCR4);
                }
        }

        return IRQ_HANDLED;
}

/*!
 * Receiver DMA callback routine.
 *
 * @param   id   pointer to device specific structure
 */
static void mxc_irda_fir_dma_rx_irq(void *id)
{

        /* we should never get here!! */
        struct net_device *dev = id;
        struct mxc_irda *si = dev->priv;
        /*struct sk_buff *skb = si->rxskb;
        unsigned int len = 0;
        int ret;*/
                
        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);

        mxc_dma_stop(si->rxdma_ch);
        /*if (!skb) {
                ERROR("%s: SKB is NULL!\n", __FUNCTION__);
                return;
        }*/

        /* TODO:Get the current data position.*/
        /*
        dma_addr = mxc_get_dma_pos(si->rxdma_ch);
        len = dma_addr - si->dma_rx_buff_phy;
        if (len > FIR_MAX_RXLEN) {
                len = FIR_MAX_RXLEN;
        }
        */
        /*len = 0;
        dma_unmap_single(si->dev, si->dma_rx_buff_phy, len, DMA_FROM_DEVICE);
        if (FIRIRSR & FIRIRSR_RPE) {
                si->rxskb = NULL;

                skb_put(skb, len);
                skb->dev = dev;
                skb->mac.raw = skb->data;
                skb->protocol = htons(ETH_P_IRDA);
                si->stats.rx_packets++;
                si->stats.rx_bytes += len;*/               

                /* Before we pass the buffer up, allocate a new one. */
        /*      ret = mxc_irda_rx_alloc(si);
                if (ret) {
                        ERROR("Not sufficient Memory for SK Buffer\n");
                        ERROR("disabling RX and RX DMA channel\n");
                        MESSAGE("Close the device and Re-open\n");
                        FIRIRCR = FIRIRCR | FIRIRCR_RE;
                        mxc_dma_stop(si->rxdma_ch);
                }

                netif_rx(skb);
                dev->last_rx = jiffies;
        } else {
                si->dma_rx_buff_phy = dma_map_single(si->dev, si->rxskb->data,
                                FIR_MAX_RXLEN,
                                DMA_FROM_DEVICE);
        }*/

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
}

/*!
 * This function is called by SDMA Interrupt Service Routine to indicate
 * requested DMA transfer is completed.
 *
 * @param   id   channel number 
 */
static void mxc_irda_fir_dma_tx_irq(void *id)
{
        struct net_device *dev = id;
        struct mxc_irda *si = dev->priv;
        struct sk_buff *skb = si->txskb;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);

        /* change speed if newspeed is set. */
        if (si->newspeed) {
                mxc_irda_set_speed(si, si->newspeed);
                si->newspeed = 0;
        }

        /*
         * Start reception. This disables the transmitter for
         * us. This will be using the existing RX buffer.
         */

        si->stats.tx_packets++;
        si->stats.tx_bytes += si->dma_tx_buff_len;

        mxc_irda_fir_dma_rx_start(si);

        /*
         * Make sure that the TX queue is available for sending
         * TX has priority over RX at all times.
         */
        netif_wake_queue(dev);
        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
}


/*!
 * This function is called by FIRI Interrupt Service Routine to handle packet
 * receive errors.
 * 
 * @param   si   FIRI device specific structure
 * @param   dev  net_device structure
 */
static void mxc_irda_fir_eif(struct mxc_irda *si, struct net_device *dev)
{
        struct sk_buff *skb = si->rxskb;
        /* dma_addr_t dma_addr; */
        unsigned int len = 0, stat, data;
        int ret;

        if (!skb) {
                ERROR("%s: SKB is NULL!\n", __FUNCTION__);
                return;
        }

        /* TODO:Get the current data position. */
        /*
        dma_addr = mxc_get_dma_pos(si->rxdma); 
        len = dma_addr - si->dma_rx_buff_phy;
        if (len > FIR_MAX_RXLEN) {
                len = FIR_MAX_RXLEN;
        }                                
        */
        dma_unmap_single(si->dev, si->dma_rx_buff_phy, len, DMA_FROM_DEVICE);

        do {
                /* Read Status, and then Data. */
                stat = FIRIRSR;
                rmb();
                data = FIRIRXFIFO;

                if (stat & (FIRIRSR_CRCE | FIRIRSR_RFO)) {
                        si->stats.rx_errors++;
                        if (stat & FIRIRSR_CRCE) {
                                si->stats.rx_crc_errors++;
                        }
                        if (stat & FIRIRSR_RFO) {
                                si->stats.rx_frame_errors++;
                        }
                } else {
                        skb->data[len++] = data;
                }
                /*
                 * If we hit the end of frame, there's
                 * no point in continuing.
                 */
                if (stat & FIRIRSR_RPE) {
                        break;
                }
        } while (FIRIRSR & FIRIRSR_RFP);

        if (stat & FIRIRSR_RPE) {
                si->rxskb = NULL;

                skb_put(skb, len);
                skb->dev = dev;
                skb->mac.raw = skb->data;
                skb->protocol = htons(ETH_P_IRDA);
                si->stats.rx_packets++;
                si->stats.rx_bytes += len;

                /* Before we pass the buffer up, allocate a new one. */
                ret = mxc_irda_rx_alloc(si);
                if (ret) {
                        ERROR("Not sufficient Memory for SK Buffer\n");
                        ERROR("disabling RX and RX DMA channel\n");
                        MESSAGE("Close the device and Re-open\n");
                        FIRIRCR = FIRIRCR | FIRIRCR_RE;
                        mxc_dma_stop(si->rxdma_ch);
                }

                netif_rx(skb);
                dev->last_rx = jiffies;
                FIRIRSR = FIRIRSR | FIRIRSR_RPE;
        } else {
                /* Remap the buffer. */
                si->dma_rx_buff_phy = dma_map_single(si->dev, si->rxskb->data,
                                FIR_MAX_RXLEN,
                                DMA_FROM_DEVICE);
        }
}


/*!
 * This is FIRI interrupt service routine.
 * This function handles TX interrupts, like Transmit Complete, Transmit Packet
 * End and Transmitter FIFO Under-run.
 * For receiver interrupt this function disables RX , process received packet,
 * and then restart RX. This also checks for any receive errors.
 *
 * @param   irq    the interrupt number
 * @param   dev_id driver private data
 * @param   regs   holds a snapshot of the processor's context before the 
 *                 processor entered the interrupt code
 *
 * @return  The function returns \b IRQ_HANDLED 
 *          \b IRQ_HANDLED is defined in \b include/linux/interrupt.h.
 */
static irqreturn_t mxc_irda_fir_irq(int irq, void *dev_id, struct pt_regs *regs)
{
        struct net_device *dev = dev_id;
        struct mxc_irda *si = dev->priv;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        /* Check TX status register */
        if (FIRITSR & (FIRITSR_TC | FIRITSR_TPE | FIRITSR_TFU)) {
                if (FIRITSR & FIRITSR_TC) {
                        MESSAGE("%s:Transmit completed and TX FIFO empty\n",
                                        __FUNCTION__);
                        FIRITSR = FIRITSR | FIRITSR_TC;
                }
                if (FIRITSR & FIRITSR_TPE) {
                        MESSAGE("%s:Transmit completed and TX FIFO empty\n",
                                         __FUNCTION__);
                        FIRITSR = FIRITSR | FIRITSR_TPE; 
                }
                if (FIRITSR & FIRITSR_TFU) {
                        MESSAGE("%s:Transmit FIFO Underrun\n",
                                         __FUNCTION__);
                        FIRITSR = FIRITSR | FIRITSR_TFU; 
                }
                return IRQ_HANDLED;
        }
        /* Stop RX DMA */
        mxc_dma_stop(si->rxdma_ch);
        /* Check RX status register */
        if (FIRIRSR & FIRIRSR_RFO) {
                if (FIRIRSR & FIRIRSR_RFO) {
                        MESSAGE("%s: RX FIFO underrun\n", __FUNCTION__);
                        FIRIRSR = FIRIRSR | FIRIRSR_RFO;
                }
        }

        /* Check for CRC/Framing errors - Throw away the packet completely */
        if (FIRIRSR & (FIRIRSR_CRCE | FIRIRSR_DDE)) {
                si->stats.rx_errors++;
                if (FIRIRSR & FIRIRSR_DDE) {
                        si->stats.rx_frame_errors++;
                }
                /*
                 * Clear selected status bits now, so we
                 * don't miss them next time around.
                 */
                FIRIRCR = FIRIRCR | FIRIRCR_RE;
                FIRIRSR = FIRIRSR_CRCE | FIRIRSR_DDE;
        }
        if (FIRIRSR & FIRIRSR_RPE) {
                MESSAGE("%s:Packet End interrupt\n", __FUNCTION__);
                mxc_irda_fir_eif(si, dev);
        }
        /* No matter what happens, we must restart reception. */
        mxc_irda_fir_dma_rx_start(si);

        FIRIRCR = FIRIRCR | FIRIRCR_RE;

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return IRQ_HANDLED;
}



/*!
 * This function is called by Linux IrDA network subsystem to
 * transmit the Infrared data packet. The TX DMA channel is configured
 * to transfer SK buffer data to FIRI TX FIFO along with DMA transfer
 * completion routine.
 *
 * @param   skb   The packet that is queued to be sent
 * @param   dev   net_device structure.
 *
 * @return  The function returns 0 on success and a non-zero value on
 *          failure.
 */
static int mxc_irda_hard_xmit(struct sk_buff *skb, struct net_device *dev)
{
        struct mxc_irda *si = dev->priv;
        int speed = irda_get_next_speed(skb);
        unsigned int cr;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        /*
         * Does this packet contain a request to change the interface
         * speed?  If so, remember it until we complete the transmission
         * of this frame.
         */
        //printk("speed %d\n", speed);
        if (speed != si->speed && speed != -1) {
                si->newspeed = speed;
        }

        /* If this is an empty frame, we can bypass a lot. */
        if (skb->len == 0) {
                if (si->newspeed) {
                        si->newspeed = 0;
                        mxc_irda_set_speed(si, speed);
                }
                dev_kfree_skb(skb);
                return 0;
        }

        if (IS_SIR(si)) {
                /* We must not be transmitting... */
                netif_stop_queue(dev);

                si->tx_buff.data = si->tx_buff.head;
                si->tx_buff.len  = async_wrap_skb(skb, si->tx_buff.data, 
                                                  si->tx_buff.truesize);
#ifdef TEST
                {
                        int i;
                        printk("\nReceived Raw data...number=%d.\n",receive_number);
                        for (i=0; i<receive_number;i++)
                        {
                                printk("%x ",receive_fifo[i]);
                        }
                        receive_number=0;
                        printk("\n");

                        printk("\nThe packet to be Tx-ed...number=%d.\n",si->tx_buff.len);
                        for (i=0; i<si->tx_buff.len;i++)
                        {
                                printk("%x ",*(si->tx_buff.head+i));
                        }
                        printk("\n");
                }
#endif
                /*
		 * Set the transmit interrupt enable.  This will fire
		 * off an interrupt immediately.  Note that we disable
		 * the receiver so we won't get spurious characteres
		 * received.
		 */
                cr = readl(si->uart_base + MXC_UARTUCR2);
                cr &= ~MXC_UARTUCR2_RXEN;
                writel(cr, si->uart_base + MXC_UARTUCR2);

                /* Disable the Transmit complete interrupt bit */
                cr = readl(si->uart_base + MXC_UARTUCR4);
                cr &= ~MXC_UARTUCR4_TCEN;
                writel(cr, si->uart_base + MXC_UARTUCR4);

                //Disable Parity error interrupt.
                //Disable Frame error interrupt.
                cr = readl(si->uart_base + MXC_UARTUCR3);
                cr &= ~(MXC_UARTUCR3_PARERREN | MXC_UARTUCR3_FRAERREN);
                writel(cr, si->uart_base + MXC_UARTUCR3);

                //Disable Receive Overrun and Data Ready interrupts.
                cr = readl(si->uart_base + MXC_UARTUCR4);
                cr &= ~(MXC_UARTUCR4_OREN | MXC_UARTUCR4_DREN);
                writel(cr, si->uart_base + MXC_UARTUCR4);

                cr = readl(si->uart_base + MXC_UARTUCR2);
                cr |= MXC_UARTUCR2_TXEN;
                writel(cr, si->uart_base + MXC_UARTUCR2);

                /* Enable the Transmit complete interrupt bit */
                cr = readl(si->uart_base + MXC_UARTUCR4);
                cr |= MXC_UARTUCR4_TCEN;
                writel(cr, si->uart_base + MXC_UARTUCR4);

                //Enable Parity error interrupt.
                //Enable Frame error interrupt.
                cr = readl(si->uart_base + MXC_UARTUCR3);
                cr |= (MXC_UARTUCR3_PARERREN | MXC_UARTUCR3_FRAERREN);
                writel(cr, si->uart_base + MXC_UARTUCR3);

                //Enable Receive Overrun and Data Ready interrupts.
                cr = readl(si->uart_base + MXC_UARTUCR4);
                cr |= (MXC_UARTUCR4_OREN | MXC_UARTUCR4_DREN);
                writel(cr, si->uart_base + MXC_UARTUCR4);

                dev_kfree_skb(skb);
        } else {
                unsigned long mtt = irda_get_mtt(skb);
                
                if (si->txskb) {
                        BUG();
                }
                netif_stop_queue(dev);
               
                si->txskb = skb;

                //si->dma_tx_buff_len = skb->len;
                si->dma_rx_buff_phy = dma_map_single(si->dev, si->rxskb->data,
                                skb->len, DMA_FROM_DEVICE);
                /*
                 * If we have a mean turn-around time, impose the specified
                 * specified delay.  We could shorten this by timing from
                 * the point we received the packet.
                 */
                if (mtt) {
                        udelay(mtt);
                }
                /* TODO:stop RX DMA,  disable */
                mxc_dma_stop(si->txdma_ch);
                //disable_fir_tx
                //clear tx status bit
                //enable_fir_tx
                //_reg_FIRI_FIRITSR = 0xff;
                //disable_fir_tx
                //mxc_irda_fir_dma_tx_start(si);

                /* Enable Transmitter */
                //FIRITCR = FIRITCR | FIRITCR_TE;
                //
                //
		// Set Data Length
	/*	_reg_FIRI_FIRITCTR = skb->len - 1;
		// Set trigger level
		firitcr_write(FIRITCR_TDT, FIR_DMA_TRIGGER(FIR_TX_TRIGGER_LEVEL) );
		disable_fir_rx;

		disable_dma(mi->txdma_channel);
		_reg_DMA_CCR(mi->txdma_channel) |= 0x8; //Request Enable
		_reg_DMA_SAR(mi->txdma_channel) = mi->txbuf_dma;
		_reg_DMA_CNTR(mi->txdma_channel) = skb->len;
		//enable_dma(mi->txdma_channel);
		//mx_dma_start(mi->txdma_channel);


//n		enable_dma(mi->txdma_channel);
		dbmx21_fir_tx_interrupt(ENABLE);
//		enable_fir_tx;

#if 1
		mx_dma_start(mi->txdma_channel);
		udelay(10);
		enable_fir_tx;
		
#else 
		enable_fir_tx;
		{
			int i;
			char *p=skb->data;
			//printk("Tx: ");
			for(i=0;i<skb->len;i++)
			{
				printk(" 0x%x ",*p);
				// *((char *)(&reg_FIRI_TFIFO))=*p;
				p++;
			}
			p=skb->data;
			for(i=0;i<skb->len;i++)
			{
				//printk(" 0x%x ",*p);
				*((char *)(&_reg_FIRI_TFIFO))=*p;
				p++;
			}
			//printk("\n");
		}
#endif*/

        }

        dev->trans_start = jiffies;

#ifdef TEST
        printk("UTS:%x\n", readl(si->uart_base + MXC_UARTUTS));
        printk("UFCR:%x\n", readl(si->uart_base + MXC_UARTUFCR));
        printk("UCR1:%x\n", readl(si->uart_base + MXC_UARTUCR1));
        printk("UCR2:%x\n", readl(si->uart_base + MXC_UARTUCR2));
        printk("UCR3:%x\n", readl(si->uart_base + MXC_UARTUCR3));
        printk("UCR4:%x\n", readl(si->uart_base + MXC_UARTUCR4));
        printk("UBIR:%x\n", readl( si->uart_base + MXC_UARTUBIR));
        printk("UBMR:%x\n", readl( si->uart_base + MXC_UARTUBMR));
        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
#endif
        return 0;
}

/*!
 * This function handles network interface ioctls passed to this driver..
 *
 * @param   dev   net device structure
 * @param   ifreq user request data
 * @param   cmd   command issued 
 *
 * @return  The function returns 0 on success and a non-zero value on
 *           failure.
 */
static int mxc_irda_ioctl(struct net_device *dev, struct ifreq *ifreq, int cmd)
{
        struct if_irda_req *rq = (struct if_irda_req *)ifreq;
        struct mxc_irda *si = dev->priv;
        int ret = -EOPNOTSUPP;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        switch (cmd) {
        /* This function will be used by IrLAP to change the speed */
        case SIOCSBANDWIDTH:
                MESSAGE("%s:with cmd SIOCSBANDWIDTH\n", __FUNCTION__);
                if (capable(CAP_NET_ADMIN)) {
                        /*
                         * We are unable to set the speed if the
                         * device is not running.
                         */
                        if (si->open) {
                                ret = mxc_irda_set_speed(si, rq->ifr_baudrate);
                        } else {
                                MESSAGE("mxc_ir_ioctl: SIOCSBANDWIDTH:\
                                         !netif_running\n");
                                ret = 0;
                        }
                }
                break;
                
        case SIOCSMEDIABUSY:
                MESSAGE("%s:with cmd SIOCSMEDIABUSY\n", __FUNCTION__);
                ret = -EPERM;
                if (capable(CAP_NET_ADMIN)) {
                        irda_device_set_media_busy(dev, TRUE);
                        ret = 0;
                }
                break;
                
        case SIOCGRECEIVING:
                MESSAGE("%s:with cmd SIOCGRECEIVING\n",__FUNCTION__);
                rq->ifr_receiving = IS_SIR(si) ? si->rx_buff.state != OUTSIDE_FRAME : 0;
                //rq->ifr_receiving = FIRIRSR & FIRIRSR_PAS;
                ret = 0;
                break;

        default:
                break;
        }

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return ret;
}


/*!
 * Kernel interface routine to get current statistics of the device 
 * which includes the number bytes/packets transmitted/received, 
 * receive errors, CRC errors, framing errors etc.
 *
 * @param  dev  the net_device structure
 *
 * @return This function returns IrDA network statistics
 */
static struct net_device_stats *mxc_irda_stats(struct net_device *dev)
{
        struct mxc_irda *si = dev->priv;
        return &si->stats;
}

/*!
 * This function enables FIRI port.
 * configures GPIO/IOMUX
 *
 * @param   si  FIRI port specific structure.
 *
 * @return  The function returns 0 on success and a non-zero value on
 *          failure.
 */
static int mxc_irda_startup(struct mxc_irda *si)
{
        int ret;
        unsigned int per_clk;
        unsigned int num, denom, baud, cr, ufcr=0;
        int d = 1;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
#ifdef CONFIG_ARCH_MXC91331 
        spba_take_ownership(SPBA_UART4, SPBA_MASTER_A);
#endif
        /*
         * Clear Status Registers 1 and 2
         **/
        writel(0xFFFF, si->uart_base + MXC_UARTUSR1);
        writel(0xFFFF, si->uart_base + MXC_UARTUSR2);

        /* Configure the IOMUX for the UART */
#ifdef MODULE
        {
                unsigned int pbc_bctrl1_set = 0;
                /* 
                 * IOMUX configs for Irda pins
                 */
                iomux_config_mux(PIN_IRDA_TX4, OUTPUTCONFIG_FUNC, INPUTCONFIG_NONE);
                iomux_config_mux(PIN_IRDA_RX4, INPUTCONFIG_NONE, INPUTCONFIG_FUNC);
                iomux_config_mux(PIN_GPIO6, OUTPUTCONFIG_FUNC, INPUTCONFIG_FUNC);
                gpio_config(0, 6, true, GPIO_INT_NONE);
                /* Clear the SD/Mode signal */
                gpio_set_data(0, 6, 0);
                /*
                 * Enable the Irda Transmitter
                 */
                pbc_bctrl1_set |= PBC_BCTRL1_IREN;
                writew(pbc_bctrl1_set, PBC_BASE_ADDRESS + PBC_BCTRL1_SET);
        }
#else
        gpio_uart_active(3, 0);
#endif
        per_clk = mxc_get_clocks(UART4_BAUD); 
        baud = per_clk / 16;
        if (baud > MAX_UART_BAUDRATE) {
                baud = MAX_UART_BAUDRATE;
                d = per_clk / ((baud * 16) + 1000);
                if (d > 6) {
                        d = 6;
                }
        }
        si->uartclk = per_clk / d;
        writel(si->uartclk / 1000, si->uart_base + MXC_UARTONEMS);

                
        writel(MXC_IRDA_RX_INV | MXC_UARTUCR4_IRSC, si->uart_base + MXC_UARTUCR4);
        writel(MXC_UART_IR_RXDMUX | MXC_IRDA_TX_INV | MXC_UARTUCR3_DSR, si->uart_base + MXC_UARTUCR3);
        writel(MXC_UARTUCR2_IRTS | MXC_UARTUCR2_CTS | MXC_UARTUCR2_WS | MXC_UARTUCR2_ATEN | MXC_UARTUCR2_TXEN | MXC_UARTUCR2_RXEN, si->uart_base + MXC_UARTUCR2);
        /* Wait till we are out of software reset */
        do {
                cr = readl(si->uart_base + MXC_UARTUCR2);
        } while(!(cr & MXC_UARTUCR2_SRST));
        
        ufcr |= (UART4_UFCR_TXTL << MXC_UARTUFCR_TXTL_OFFSET) |
                        MXC_UARTUFCR_RFDIV | UART4_UFCR_RXTL;
        writel(ufcr, si->uart_base + MXC_UARTUFCR);
        writel(MXC_UARTUCR1_UARTEN | MXC_UARTUCR1_IREN, si->uart_base + MXC_UARTUCR1); 

        baud = 9600;
        num = baud / 100 - 1;
        denom = si->uartclk / 1600 - 1;

        if ((denom < 65536) && (si->uartclk > 1600)) {
                writel(num, si->uart_base + MXC_UARTUBIR);
                writel(denom, si->uart_base + MXC_UARTUBMR);
        }
        printk("UBIR:%x\n", readl( si->uart_base + MXC_UARTUBIR));
        printk("UBMR:%x\n", readl( si->uart_base + MXC_UARTUBMR));
        printk("baud %d\n", baud);


        writel(0x0000, si->uart_base + MXC_UARTUTS); 

        /* configure FIRI device for speed */
        ret = mxc_irda_set_speed(si, si->speed = 9600);
        if (ret) {
                return ret;
        }
        printk("UTS:%x\n", readl(si->uart_base + MXC_UARTUTS));
        printk("UFCR:%x\n", readl(si->uart_base + MXC_UARTUFCR));
        printk("UCR1:%x\n", readl(si->uart_base + MXC_UARTUCR1));
        printk("UCR2:%x\n", readl(si->uart_base + MXC_UARTUCR2));
        printk("UCR3:%x\n", readl(si->uart_base + MXC_UARTUCR3));
        printk("UCR4:%x\n", readl(si->uart_base + MXC_UARTUCR4));

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return 0;
}

/*!
 * This function is called to disable the FIRI port
 *
 * @param   si  FIRI port specific structure.
 */
static void mxc_irda_shutdown(struct mxc_irda *si)
{
        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        /* Stop all DMA activity. */
        mxc_dma_stop(si->txdma_ch);
        mxc_dma_stop(si->rxdma_ch);

        /* TODO:Configure the IOMUX to disable the FIRI port. */
        //iomux_config_gpr(MUX_PGP_FIRI, false);
       
        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
}

/*!
 * When an ifconfig is issued which changes the device flag to include
 * IFF_UP this function is called. It is only called when the change
 * occurs, not when the interface remains up. The function grabs the interrupt
 * resources and registers FIRI interrupt service routines, requests for DMA
 * channels, configures the DMA channel. It then initializes  the IOMUX 
 * registers to configure the pins for FIRI signals and finally initializes the
 * various FIRI registers and enables the port for reception.
 *
 * @param   dev   net device structure that is being opened
 * 
 * @return  The function returns 0 for a successful open and non-zero value 
 *          on failure.
 */
static int mxc_irda_start(struct net_device *dev)
{
        struct mxc_irda *si = dev->priv;
        dma_channel_params params; 
        unsigned int sr1, sr2, cr;
        int err;

        si->speed = 9600;
        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);

        err = request_irq(si->uart_irq, mxc_irda_sir_irq, 0, dev->name, dev);
        if (err) {
                printk("%s:Failed to request UART IRQ\n", __FUNCTION__);
                goto err_irq1;
        }

        /*err = request_irq(si->firi_irq, mxc_irda_fir_irq, SA_INTERRUPT|SA_SHIRQ, dev->name, dev);
        if (err) {
                printk("%s:Failed to request FIRI IRQ\n", __FUNCTION__);
                goto err_irq2;
        }*/
        /*
         * The interrupt must remain disabled for now.
         */
        disable_irq(si->uart_irq);

        err = -EBUSY;
        si->rxdma_ch = 0;
        err = mxc_request_dma(&si->rxdma_ch, "MXC FIRI RX");
        if (err < 0) {
                ERROR("%s:err = %d\n", __FUNCTION__, err);
                goto err_rx_dma;
        } 

        /* Setup DMA Channel parameters */
        params.watermark_level = 4;
        params.peripheral_type = FIRI;
        params.transfer_type = per_2_emi;
        params.per_address = si->firi_res->start + 0x18;
        params.event_id = DMA_REQ_FIRI_RX;
        params.bd_number = 1;
        params.word_size = TRANSFER_32BIT;
        params.callback = mxc_irda_fir_dma_rx_irq;
        params.arg = dev;

        err = mxc_dma_setup_channel(si->rxdma_ch, &params);
        if (err < 0) {
                ERROR("%s:err = %d\n", __FUNCTION__, err);
                goto err_rx_dma;
        }

        si->txdma_ch = 0;
        err = mxc_request_dma(&si->txdma_ch, "MXC FIRI RX");
        if (err < 0){
                ERROR("%s:err = %d\n", __FUNCTION__, err);
                goto err_tx_dma;
        }

        params.watermark_level = 4;
        params.peripheral_type = FIRI;
        params.transfer_type = emi_2_per;
        params.per_address = si->firi_res->start + 0x14;
        params.event_id = DMA_REQ_FIRI_TX;
        params.bd_number = 1;
        params.word_size = TRANSFER_32BIT;
        params.callback = mxc_irda_fir_dma_tx_irq;
        params.arg = dev;

        err = mxc_dma_setup_channel(si->txdma_ch, &params);
        if(err < 0 ) {
                ERROR("%s:err = %d\n", __FUNCTION__, err);
                goto err_tx_dma;
        } 

        si->dma_rx_buff = dma_alloc_coherent(NULL, 4096, &si->dma_rx_buff_phy, GFP_KERNEL);
        if (!si->dma_rx_buff) {
                goto err_dma_rx_buff;
        }

        si->dma_tx_buff = dma_alloc_coherent(NULL, 4096, &si->dma_tx_buff_phy, GFP_KERNEL);
        if (!si->dma_tx_buff) {
                goto err_dma_tx_buff;
        }
        
        /* Setup the serial port port for the initial speed. */
        err = mxc_irda_startup(si);
        if (err) {
                goto err_startup;
        }

        /* Open a new IrLAP layer instance. */
        si->irlap = irlap_open(dev, &si->qos, "mxc");
        err = -ENOMEM;
        if (!si->irlap) {
                goto err_irlap;
        }

        /* Now enable the interrupt and start the queue */
        si->open = 1;

        sr2 = readl(si->uart_base + MXC_UARTUSR2);
        sr2 |= MXC_UARTUSR2_ORE;
        writel(sr2, si->uart_base + MXC_UARTUSR2);

        sr1 = readl(si->uart_base + MXC_UARTUSR1);
        sr1 |= (MXC_UARTUSR1_PARITYERR | MXC_UARTUSR1_FRAMERR);
        writel(sr1, si->uart_base + MXC_UARTUSR1);

        cr = readl(si->uart_base + MXC_UARTUCR2);
        cr |= MXC_UARTUCR2_RXEN;
        writel(cr, si->uart_base + MXC_UARTUCR2);

        enable_irq(si->uart_irq);
        netif_start_queue(dev);

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return 0;

err_irlap:
        si->open = 0;
        mxc_irda_shutdown(si);
err_startup:
        dma_free_coherent(NULL, IRDA_FRAME_SIZE_LIMIT, si->dma_tx_buff, si->dma_tx_buff_phy);
err_dma_tx_buff:
        dma_free_coherent(NULL, IRDA_FRAME_SIZE_LIMIT, si->dma_rx_buff, si->dma_rx_buff_phy);
err_dma_rx_buff:
        mxc_free_dma(si->txdma_ch);
err_tx_dma:
        mxc_free_dma(si->rxdma_ch);
err_rx_dma:
        free_irq(dev->irq, dev);
/*err_irq2:
        free_irq(FIRI_INT, dev);*/
err_irq1:
        IRDA_DEBUG(KERN_INFO, "OUT %s() Error:%d\n", __FUNCTION__, err);
        return err;
        
}

/*!
 * This function is called when IFF_UP flag has been cleared by the user via
 * the ifconfig irda0 down command. This function stops any further 
 * transmissions being queued, and then disables the interrupts. 
 * Finally it resets the device.
 * @param   dev   the net_device structure
 * 
 * @return  dev   the function always returns 0 indicating a success.
 */
static int mxc_irda_stop(struct net_device *dev)
{
        struct mxc_irda *si = dev->priv;

        disable_irq(si->uart_irq);
        mxc_irda_shutdown(si);

        /*
         * If we have been doing DMA receive, make sure we
         * tidy that up cleanly.
         */
        if (si->rxskb) {
                dma_unmap_single(si->dev, si->dma_rx_buff_phy, FIR_MAX_RXLEN,
                                 DMA_FROM_DEVICE);
                dev_kfree_skb(si->rxskb);
                si->rxskb = NULL;
        }

        /* Stop IrLAP */
        if (si->irlap) {
                irlap_close(si->irlap);
                si->irlap = NULL;
        }

        netif_stop_queue(dev);
        si->open = 0;

        /* Free resources */
        mxc_free_dma(si->txdma_ch);
        mxc_free_dma(si->rxdma_ch);
        free_irq(dev->irq, dev);

        return 0;
}

/*!
 * This function is called to put the FIRI in a low power state. Refer to the
 * document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   _dev   the device structure used to give information on which FIRI
 *                to suspend
 * @param   state the power state the device is entering
 * @param   level the stage in device suspension process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_irda_suspend(struct device *_dev, u32 state, u32 level)
{
        struct net_device *dev = dev_get_drvdata(_dev);
        struct mxc_irda *si;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        if (!dev || level != SUSPEND_DISABLE) {
                return 0;
        }

        si = dev->priv;
        if (si->open) {
                /* wait for Transfer to complete */
                while ((FIRITSR & FIRITSR_TC) || (FIRIRSR & FIRIRSR_RFP)) {
                        /* do nothing */
                }
                /* Stop the transmit queue */
                netif_device_detach(dev);
                disable_irq(dev->irq);
                mxc_irda_shutdown(si);
        }

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return 0;
}


/*!
 * This function is called to bring the FIRI back from a low power state. Refer
 * to the document driver-model/driver.txt in the kernel source tree for more
 * information.
 *
 * @param   _dev   the device structure used to give information on which FIRI 
 *                to resume
 * @param   level the stage in device resumption process that we want the
 *                device to be put in
 *
 * @return  The function always returns 0.
 */
static int mxc_irda_resume(struct device *_dev, u32 level)
{
        struct net_device *dev = dev_get_drvdata(_dev);
        struct mxc_irda *si;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
                
        if (!dev || level != RESUME_ENABLE) {
                return 0;
        }

        si = dev->priv;
        if (si->open) {

                /*
                 * If we missed a speed change, initialize at the new speed
                 * directly.  
                 */
                if (si->newspeed) {
                        si->speed = si->newspeed;
                        si->newspeed = 0;
                }

                mxc_irda_startup(si);
                enable_irq(dev->irq);
                /* This automatically wakes up the queue */
                netif_device_attach(dev);
        }

        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return 0;
}

static int mxc_irda_init_iobuf(iobuff_t *io, int size)
{
        io->head = kmalloc(size, GFP_KERNEL | GFP_DMA);
        if (io->head != NULL) {
                io->truesize = size;
                io->in_frame = FALSE;
                io->state    = OUTSIDE_FRAME;
                io->data     = io->head;
        }
        return io->head ? 0 : -ENOMEM;

}

/*!
 * This function is called during the driver binding process. 
 * This function requests for memory, initializes net_device structure and
 * registers with kernel. 
 *
 * @param   _dev   the device structure used to store device specific 
 *                information that is used by the suspend, resume and remove 
 *                functions
 *
 * @return  The function returns 0 on success and a non-zero value on failure
 */
static int mxc_irda_probe(struct device *_dev)
{
        struct platform_device *pdev = to_platform_device(_dev);
        struct net_device *dev;
        struct mxc_irda *si;
        struct resource *uart_res, *firi_res;
        int uart_irq, firi_irq;
        unsigned int baudrate_mask = 0;
        int err;

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);

        uart_res = &mxcir_resources[0];
        uart_irq = mxcir_resources[1].start;

        firi_res = &mxcir_resources[2];
        firi_irq = mxcir_resources[3].start;

        if (!uart_res || uart_irq == NO_IRQ || !firi_res || firi_irq == NO_IRQ) {
                printk("Unable to find resources\n");
                return -ENXIO;
        }

        err = request_mem_region(uart_res->start, SZ_16K, "MXC_IRDA") ? 0 : -EBUSY;
        if (err) {
                printk("Failed to request UART memory region\n");
                goto err_mem_1;
        }

        err = request_mem_region(firi_res->start, SZ_16K, "MXC_IRDA") ? 0 : -EBUSY;
        if (err) {
                printk("Failed to request FIRI  memory region\n");
                goto err_mem_2;
        }

        dev = alloc_irdadev(sizeof(struct mxc_irda));
        if (!dev) {
                goto err_mem_3;
        }

        si = netdev_priv(dev);
        si->dev = &pdev->dev;
        si->uart_res = uart_res;
        si->firi_res = firi_res;
        si->uart_irq = UART_INT;
        si->firi_irq = FIRI_INT;
        
        si->uart_base = ioremap(uart_res->start, SZ_16K);
        si->firi_base = ioremap(firi_res->start, SZ_16K);
        if (! (si->uart_base || si->firi_base)) {
                err = -ENOMEM;
                goto err_mem_3;
        }
        
        /*
         * Initialise the SIR buffers
         */
        err = mxc_irda_init_iobuf(&si->rx_buff, 14384); 
        if (err) {
                goto err_mem_4;
        }

        err = mxc_irda_init_iobuf(&si->tx_buff, 4000);
        if (err) {
                goto err_mem_5;
        }

        dev->hard_start_xmit    = mxc_irda_hard_xmit;
        dev->open               = mxc_irda_start;
        dev->stop               = mxc_irda_stop;
        dev->do_ioctl           = mxc_irda_ioctl;
        dev->get_stats          = mxc_irda_stats;

        irda_init_max_qos_capabilies(&si->qos);

        /*
         * We support 
         * SIR(9600, 19200,38400, 57600 and 115200 bps)
         * MIR(0.576 and 1.152 Mbps) and 
         * FIR(4 Mbps)
         * Min Turn Time set to 1ms or greater.
         */
        baudrate_mask |= IR_9600 |IR_19200|IR_38400|IR_57600|IR_115200;
        //baudrate_mask |= IR_4000000 << 8;

        si->qos.baud_rate.bits &= baudrate_mask;
        si->qos.min_turn_time.bits = 7;

        irda_qos_bits_to_value(&si->qos);

        err = register_netdev(dev);
        if (err == 0) {
                dev_set_drvdata(&pdev->dev, si);
        } else {
                kfree(si->tx_buff.head);
err_mem_5:
                kfree(si->rx_buff.head);
err_mem_4:
                free_netdev(dev);
err_mem_3:
                release_mem_region(FIRI_BASE_ADDR, 0x1C);
err_mem_2:
                release_mem_region(UART_BASE_ADDR, SZ_16K);
        }
err_mem_1:
        IRDA_DEBUG(KERN_INFO, "OUT %s() err:%d\n", __FUNCTION__, err);
        return err;
}

/*!
 * Dissociates the driver from the FIRI device. Removes the appropriate FIRI
 * port structure from the kernel.
 *
 * @param   _dev   the device structure used to give information on which FIRI
 *                to remove
 *
 * @return  The function always returns 0. 
 */
static int mxc_irda_remove(struct device *_dev)
{
        struct net_device *dev = dev_get_drvdata(_dev);

        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        if (dev) {
                unregister_netdev(dev);
                free_netdev(dev);
        }
        release_mem_region(FIRI_BASE_ADDR, 0x1C);
        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return 0;
}

/*!
 * This structure contains pointers to the power management callback functions.
 */
static struct device_driver mxcir_driver = {
        .name           = "mxc-ir",
        .bus            = &platform_bus_type,
        .probe          = mxc_irda_probe,
        .remove         = mxc_irda_remove,
        .suspend        = mxc_irda_suspend,
        .resume         = mxc_irda_resume,
};

/*!
 * Resource definition for the UART & FIRI
 */
static struct resource mxcir_resources[] = {
        [0] = {
                .start  = UART_BASE_ADDR,
                .end    = UART_BASE_ADDR + SZ_16K - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = UART_INT,
                .end    = UART_INT,
                .flags  = IORESOURCE_IRQ,
        },
        [2] = {
                .start  = FIRI_BASE_ADDR,
                .end    = FIRI_BASE_ADDR + SZ_16K - 1,
                .flags  = IORESOURCE_MEM,
        },
        [3] = {
                .start  = FIRI_INT,
                .end    = FIRI_INT,
                .flags  = IORESOURCE_IRQ,
        }
};

/*!
 * This is platform device structure for adding FIRI 
 */
static struct platform_device mxcir_device = {
        .name           = "mxc-ir",
        .id             = 0,
        .num_resources  = ARRAY_SIZE(mxcir_resources),
        .resource       = mxcir_resources,
};

/*!
 * This function is used to initialize the FIRI driver module. The function
 * registers the power management callback functions with the kernel and also
 * registers the FIRI callback functions.
 *
 * @return  The function returns 0 on success and a non-zero value on failure.
 */
static int __init mxc_irda_init(void)
{
        int ret;
        IRDA_DEBUG(KERN_INFO, "IN %s()\n", __FUNCTION__);
        ret = driver_register(&mxcir_driver);
        if (ret == 0) {
                ret = platform_device_register(&mxcir_device);
                if (ret) {
                        driver_unregister(&mxcir_driver);
                }
        }
        IRDA_DEBUG(KERN_INFO, "OUT %s()\n", __FUNCTION__);
        return ret;
}

/*!
 * This function is used to cleanup all resources before the driver exits.
 */ 
static void __exit mxc_irda_exit(void)
{
        driver_unregister(&mxcir_driver);
        platform_device_unregister(&mxcir_device);
}

module_init(mxc_irda_init);
module_exit(mxc_irda_exit);

MODULE_AUTHOR("Freescale Semiconductor");
MODULE_DESCRIPTION("MXC IrDA(SIR/FIR) driver");
MODULE_LICENSE("GPL");
