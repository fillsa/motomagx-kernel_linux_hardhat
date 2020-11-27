/*
 *  fx2lp_i2c.c
 *
 *  Copyright 2006 Motorola, Inc.
 *
 *  Adds ability to download firmware to FX2LP USB High Speed chip over
 *  I2C bus. This occurs during kernel init with the MXC I2C bus acting
 *  as an I2C slave in the transaction.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Date         Author          Comment
 * ===========  ==============  ==============================================
 * 04-Oct-2006  Motorola        Initial revision.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/bootinfo.h>
#include <asm/arch/clock.h>
#include <asm/arch/epit.h>
#include <asm/arch/hardware.h>

extern void gpio_i2c_active(int i2c_num);
extern void gpio_i2c_inactive(int i2c_num);

extern void gpio_usb_hs_reset_set_data(__u32 enable);

static void set_next_timeout(void);
int start_fx2lp_fw_dl(void);

//#define DEBUG_FX2LP_I2C 1
#undef DEBUG_FX2LP_I2C
#ifdef DEBUG_FX2LP_I2C
#define DBG_PRINTK(fmt, args...) printk(fmt,##args)
#else
#define DBG_PRINTK(fmt, args...)
#endif

/* Used for putting the fx2lp in/out of reset */
#define TURN_CHIP_ON   1
#define TURN_CHIP_OFF  0

#define I2C_NUM 0

/* Address offsets of the I2C registers */
#define MXC_IADR                0x00 /* Address Register */
#define MXC_IFDR                0x04 /* Freq div register */
#define MXC_I2CR                0x08 /* Control regsiter */
#define MXC_I2SR                0x0C /* Status register */
#define MXC_I2DR                0x10 /* Data I/O register */

/* Bit definitions of I2CR */
#define MXC_I2CR_IEN            0x0080
#define MXC_I2CR_IIEN           0x0040
#define MXC_I2CR_MSTA           0x0020
#define MXC_I2CR_MTX            0x0010
#define MXC_I2CR_TXAK           0x0008
#define MXC_I2CR_RSTA           0x0004

/* Bit definitions of I2SR */
#define MXC_I2SR_ICF            0x0080
#define MXC_I2SR_IAAS           0x0040
#define MXC_I2SR_IBB            0x0020
#define MXC_I2SR_IAL            0x0010
#define MXC_I2SR_SRW            0x0004
#define MXC_I2SR_IIF            0x0002
#define MXC_I2SR_RXAK           0x0001

#define I2C_MEMBASE             (volatile void*)IO_ADDRESS(I2C_BASE_ADDR)

/* the eigth byte in the firmware tells the fx2lp to operate the i2c
   bus at 400 kbps */
#define CONFIG_BYTE_400KHZ      8

/* used for timeout calculations */
#define MICROSECONDS_PER_TICK   (USEC_PER_SEC / EPIT_COUNTER_TICK_RATE)

#ifdef DEBUG_FX2LP_I2C
/* timeout value in microseconds for fx2lp reset */
#define FX2LP_RESET_TIMEOUT     10000
/* timeout value in microseconds for i2c transfers */
#define I2C_TRANSFER_TIMEOUT    10000
#else
/* timeout value in microseconds for fx2lp reset */
#define FX2LP_RESET_TIMEOUT     330
/* timeout value in microseconds for i2c transfers */
#define I2C_TRANSFER_TIMEOUT    360
#endif /* DEBUG_FX2LP_I2C */

/* slave address fx2lp sends master requests to */
#define I2C_SLAVE_ADDRESS       0x51

/* i2c module input clock is mcu_ipg_clk = 26Mhz */
/* IFDR divider value for 100kbps operations for initial 8 byte transfer */
#define I2C_100KHZ_DIVIDER      0x0f
/* IFDR divider value for 400kbps operations */
#define I2C_400KHZ_DIVIDER      0x07

/* value for clearing status register to default values */
#define I2C_CLEAR_STATUS_REG    0x00

static unsigned long next_timeout;

static void set_next_timeout(void) {
    *_reg_EPIT_EPITCR &= ~EPITCR_EN;
    next_timeout = EPIT_MAX - (I2C_TRANSFER_TIMEOUT / MICROSECONDS_PER_TICK);
    *_reg_EPIT_EPITCR |= EPITCR_EN;
}

int start_fx2lp_fw_dl(void) {
    unsigned int volatile sr = 0;
    unsigned int volatile cr = 0;
    unsigned int index = 0;
    unsigned int ret = 0;
    unsigned int offset = 8;
    char __iomem* fw;
    char __iomem* tmp;
    unsigned int fw_address = mot_usb_firmware_address();
    unsigned int fw_size = mot_usb_firmware_size();

    if(!fw_address || !fw_size) {
        printk(KERN_ERR "fx2lp: atag values look invalid\n");
        return 1;
    }

    if(!(fw = ioremap(fw_address, fw_size))) {
        printk(KERN_ERR "fx2lp: unable to relocate memory holding firmware\n");
        return 1;
    }

    DBG_PRINTK("fx2lp: firmware pointer is %p\n", fw);

    /* use this to "reset" our pointer when we get the address */
    tmp = fw;

    /* set up mxc i2c bus */
    writeb((I2C_SLAVE_ADDRESS << 1), I2C_MEMBASE + MXC_IADR);
    writeb(I2C_100KHZ_DIVIDER, I2C_MEMBASE + MXC_IFDR);
    writeb(I2C_CLEAR_STATUS_REG, I2C_MEMBASE + MXC_I2SR);

    gpio_i2c_active(I2C_NUM);
    mxc_clks_enable(I2C_CLK);

    /* slave mode is set by default */
    writeb(MXC_I2CR_IEN, I2C_MEMBASE + MXC_I2CR);

    /* initialize EPIT hardware
     * - set 32K clock source and initial counter value to 0xffffffff */
    writel(EPITCR_CLKSRC_32K | EPITCR_ENMOD, _reg_EPIT_EPITCR);

    next_timeout = EPIT_MAX - (FX2LP_RESET_TIMEOUT / MICROSECONDS_PER_TICK);

    mxc_clks_enable(EPIT1_CLK);

    DBG_PRINTK("fx2lp: reset chip and start download\n");

    gpio_usb_hs_reset_set_data(TURN_CHIP_ON);

    /* start EPIT counter */
    *_reg_EPIT_EPITCR |= EPITCR_EN;

    while(1) {
        sr = readb(I2C_MEMBASE + MXC_I2SR);

        if(MXC_I2SR_IIF & sr) {
            if(MXC_I2SR_IAL & sr) {
                /* arbitration lost? this is a problem */
                printk(KERN_ERR "fx2lp: arbitration lost\n");
                ret = 1;
                break;
            }
            else if(MXC_I2SR_IAAS & sr) {
                DBG_PRINTK("fx2lp: slave address match\n");

                if(MXC_I2SR_SRW & sr) {
                    /* slave write */
                    writeb(I2C_CLEAR_STATUS_REG, I2C_MEMBASE + MXC_I2SR);

                    cr = readb(I2C_MEMBASE + MXC_I2CR);
                    cr |= MXC_I2CR_MTX;
                    writeb(cr, I2C_MEMBASE + MXC_I2CR);

                    DBG_PRINTK("fx2lp:write:0x%02x\n", readb(fw + index));

                    /* initial write is in response to a probe and it doesn't
                       really matter what we write for it */
                    writeb(readb(fw + index++), I2C_MEMBASE + MXC_I2DR);
                    set_next_timeout();
                }
                else {
                    /* slave read */
                    writeb(I2C_CLEAR_STATUS_REG, I2C_MEMBASE + MXC_I2SR);
                    
                    cr = readb(I2C_MEMBASE + MXC_I2CR);
                    cr &= ~MXC_I2CR_MTX;
                    writeb(cr, I2C_MEMBASE + MXC_I2CR);

                    DBG_PRINTK("fx2lp:reading first byte\n");

                    /* the fx2lp sends out 3 bytes...the first byte is a
                       control byte that we don't care about and the last
                       two bytes will contain our address */
                    (void)readb(I2C_MEMBASE + MXC_I2DR);
                    index = 0;
                    set_next_timeout();
                }
            }
            else {
                /* transfer complete interrupt */
                if(index == CONFIG_BYTE_400KHZ) {
                    DBG_PRINTK("fx2lp: switching to 400kbps\n");

                    /* bit 0 in previous byte tells fx2lp to operate at
                       400kbps instead of 100kbps, the bit should be set to 1 */
                    writeb(I2C_400KHZ_DIVIDER, I2C_MEMBASE + MXC_IFDR);
                }

                writeb(I2C_CLEAR_STATUS_REG, I2C_MEMBASE + MXC_I2SR);
                cr = readb(I2C_MEMBASE + MXC_I2CR);

                if(MXC_I2CR_MTX & cr) {
                    /* we're in transmit mode */
                    if(!(MXC_I2SR_RXAK & sr)) {
                        /* ACK received from master */
                        DBG_PRINTK("fx2lp:write:0x%02x\n", readb(fw + index));

                        writeb(readb(fw + index++), I2C_MEMBASE + MXC_I2DR);
                        set_next_timeout();
                    }
                    else {
                        DBG_PRINTK("fx2lp: NACK received from master\n");

                        /* give up i2c bus */
                        cr = readb(I2C_MEMBASE + MXC_I2CR);
                        cr &= ~MXC_I2CR_MTX;
                        writeb(cr, I2C_MEMBASE + MXC_I2CR);
                        (void)readb(I2C_MEMBASE + MXC_I2DR);

                        set_next_timeout();

                        /* index can be 1 after the probe so we need to check
                           for a size greater than 1...when firmware download is
                           complete the fx2lp will send a STOP */
                        if(index > 1) {
                            /* last byte has been transferred */
                            DBG_PRINTK("fx2lp: wait for STOP\n");

                            do {
                                sr = readb(I2C_MEMBASE + MXC_I2SR);
                            } while ((MXC_I2SR_IBB & sr) &&
                                    (next_timeout <= *_reg_EPIT_EPITCNR));

                            if(!(MXC_I2SR_IBB & sr)) {
                                DBG_PRINTK("fx2lp: got the STOP\n");
                            }
                            else {
                                printk(KERN_ERR "fx2lp: i2c STOP timed out\n");
                                ret = 1;
                            }

                            break;
                        }
                    }
                }
                else {
                    /* in receive mode */
                    /* the SCM-A11 is considered a two-byte addressable
                       eeprom on the bus, grab the two bytes of address */
                    index |= (readb(I2C_MEMBASE + MXC_I2DR) << offset);

                    if(!offset) {
                        fw = tmp + index;
                        DBG_PRINTK("fx2lp: index set to %0x\n", index);
                    }

                    /* update offset so we grab the next byte correctly */
                    offset = 0;
                    set_next_timeout();
                }
            }
        }
        else if(next_timeout >= *_reg_EPIT_EPITCNR) {
            if(index ? printk(KERN_ERR "fx2lp: i2c transfer timed out\n") :
                       printk(KERN_ERR "fx2lp: chip reset timed out\n"));

            ret = 1;
            break;
        }
    }

    mxc_clks_disable(EPIT1_CLK);
    mxc_clks_disable(I2C_CLK);
    gpio_i2c_inactive(I2C_NUM);

    if(ret) {
        /* if firmware download didn't work then put the fx2lp back in reset */
        gpio_usb_hs_reset_set_data(TURN_CHIP_OFF);
    }

    iounmap(fw);

    return ret;
}
