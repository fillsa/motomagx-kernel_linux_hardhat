/*
 * otg/ocd/otg-i2c/i2c-l26.c -- Linux 2.6 I2C access
 * @(#) balden@seth2.belcarratech.com|otg/ocd/otglib/i2c-l26.c|20051116205000|39053
 *
 *      Copyright (c) 2004-2005 Belcarra
 *
 * By: 
 *      Stuart Lynne <sl@lbelcarra.com>, 
 *      Bruce Balden <balden@belcarra.com>
 *
 * Copyright 2005-2006 Motorola, Inc.
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
/*!
 * @file otg/ocd/otglib/i2c-l26.c
 * @brief Linux I2C I/O via generic i2c device.
 *
 * Writes are queued and performed in a bottom half handler.
 *
 * @ingroup ISP1301TCD
 */

#include <otg/otg-compat.h>

#include <asm/irq.h>
#include <asm/system.h>
#include <asm/io.h>
#include <linux/i2c.h>

#include <otg/pcd-include.h>
#include <linux/pci.h>
#include <otghw/isp1301-hardware.h>
#include <otghw/isp1301.h>

/* ********************************************************************************************* */

/*
 * N.B. i2c functions must not be called from interrupt handlers
 */

static struct file *i2c_file;
static struct i2c_client *i2c_client;
static int initstate_i2c;
static int initstate_region;
#define MAX_I2C 16

/*! i2c_configure
 * Attempt to find and open generic i2c device
 * @return - non-zero for failure
 */
int  i2c_configure(char *name, int addr)
{
        char filename[20];
        struct i2c_adapter *ad;
        int tmp;

        RETURN_ZERO_IF(initstate_i2c);

#if 1 
        for (tmp=0 ; tmp<MAX_I2C; tmp++){
                ad = i2c_get_adapter(tmp);
//                printk(KERN_INFO"SHP - for tmp = %d name = %s\n", tmp, ad->name);
                if (!strncmp(ad->name, name, strlen(name)))
	                        break;
	        }
        if (tmp == MAX_I2C) {                           // Nothing found
                printk(KERN_ERR"%s: cannot find I2C driver", __FUNCTION__);
	                return -ENODEV;
        }
        i2c_client = kmalloc(sizeof(*i2c_client), GFP_KERNEL);
        i2c_client->adapter = (struct i2c_adapter *) ad;
//        printk(KERN_INFO"i2c_client name = %s\n", i2c_client->adapter->name);
#endif
	
#if 0										
        /*find the I2C driver we need 
         */
        for (tmp = 0; tmp < MAX_I2C; tmp++) {

                sprintf(filename, "/dev/i2c/%d", tmp);

                printk(KERN_INFO"%s: %s\n", __FUNCTION__, filename);

                UNLESS (IS_ERR(i2c_file = filp_open(filename, O_RDWR, 0))) {

                        //printk(KERN_INFO"%s: %s found\n", __FUNCTION__, filename);

                        /*found some driver */
                        i2c_client = (struct i2c_client *)i2c_file->private_data;

                        printk(KERN_INFO"%s: \"%s\" found \"%s\"\n", __FUNCTION__, name, i2c_client->adapter->name);
                        if (strlen(i2c_client->adapter->name) >= 8) {
                                if (!strncmp(i2c_client->adapter->name, name, strlen(name)))
                                        break;  /*we found our driver! */
                        }
                        i2c_client = NULL;
                        filp_close(i2c_file, NULL);
                }
                printk(KERN_INFO"%s: %s %d\n", __FUNCTION__, filename, i2c_file);
        }
        if (tmp == MAX_I2C) {                           // Nothing found
                printk(KERN_ERR"%s: cannot find I2C driver", __FUNCTION__);
                return -ENODEV;
        }
#endif 

        i2c_client->addr = addr;
        initstate_i2c = 1;
        return 0;
}



/*! i2c_close
 * Close i2c device fd.
 */
void  i2c_close(void)
{
#if 1
	kfree(i2c_client);
#endif
#if 0		
        if (initstate_i2c)
                filp_close(i2c_file, NULL);
#endif	
        initstate_i2c = 0;
}

/*! i2c_readb
 * Read byte from i2c device
 * @param subaddr
 */
u8 i2c_readb(u8 subaddr)
{
        u8 buf = 0;
        i2c_master_send(i2c_client, &subaddr, 1);
        i2c_master_recv(i2c_client, &buf, 1);
        //TRACE_MSG2(TCD, "addr: %02x buf:  %02x", subaddr, buf);
        return buf;
}

/*! i2c_readw
 * Read word from i2c device
 * @param subaddr
 */
u16 i2c_readw(u8 subaddr)
{
        u16 buf = 0;
        i2c_master_send(i2c_client, &subaddr, 1);
        i2c_master_recv(i2c_client, (u8 *)&buf, 2);
        //TRACE_MSG2(TCD, "addr: %02x buf:  %04x", subaddr, buf);
        return buf;
}

/*! i2c_readl
 * Read long from i2c device
 * @param subaddr
 */
u32 i2c_readl(u8 subaddr)
{
        u32 buf = 0;
        i2c_master_send(i2c_client, &subaddr, 1);
        i2c_master_recv(i2c_client, (u8 *)&buf, 4);
        //TRACE_MSG2(TCD, "addr: %02x buf:  %08x", subaddr, buf);
        return buf;
}



/* ********************************************************************************************* */

struct WORK_STRUCT i2c_io_work_struct;                              // work structure for queued i2c writes

#define I2C_MAX_WRITE           100

int i2c_Write_Queued;
u8 i2c_Write_Data[I2C_MAX_WRITE];
u8 i2c_Write_Addr[I2C_MAX_WRITE];

/*! i2c_writeb_direct() - internal
 * Writw byte to i2c device
 * @param subaddr
 * @param buf
 */
void i2c_writeb_direct(u8 subaddr, u8 buf)
{
        char tmpbuf[2];

        tmpbuf[0] = subaddr;    /*register number */
        tmpbuf[1] = buf;        /*register data */
        i2c_master_send(i2c_client, &tmpbuf[0], 2);
}

/*! 
 * i2c_io_bh() - bottom half handler to use i2c_write on queued data
 * i2c_write operations are queued so that they can be done in this bottom
 * half handler.
 *
 * XXX the memcpy's could be eliminated with head/tail pointers.
 *
 * @param arg
 */
void i2c_io_bh(void *arg)
{
        while (i2c_Write_Queued > 0) {
                u8 write = i2c_Write_Data[0];
                u8 port = i2c_Write_Addr[0];
                unsigned long flags;                                    // atomic update of counter and saved values
                local_irq_save (flags);
                i2c_Write_Queued--;
                memcpy(i2c_Write_Data, i2c_Write_Data + 1, sizeof(i2c_Write_Data) - 1);
                memcpy(i2c_Write_Addr, i2c_Write_Addr + 1, sizeof(i2c_Write_Addr) - 4);
                local_irq_restore (flags);

                i2c_writeb_direct(port, write);                                 // perform write

                //TRACE_MSG3(TCD,"port: %02x data: %02x result: %02x", port, write, i2c_readb(port & 0xfe));

        }
}


/*!
 * i2c_writeb() - queue write operation
 * @param port
 * @param data byte to write
 */
void i2c_writeb(u8 port, u8 data)
{
        unsigned long flags;                                    // atomic update of counter and saved values
        RETURN_IF(i2c_Write_Queued < 0);                                // check if terminating
        if ((i2c_Write_Queued == I2C_MAX_WRITE)) {
                TRACE_MSG3(TCD, "TOO MANY QUEUED CANNOT WRITE port: %02x data: %02x active: %d", port, data, i2c_Write_Queued);
                return;
        }
        local_irq_save (flags);
        i2c_Write_Addr[i2c_Write_Queued] = port;
        i2c_Write_Data[i2c_Write_Queued] = data;
        i2c_Write_Queued++;
        local_irq_restore (flags);
        //TRACE_MSG3(TCD, "port: %02x data: %02x active: %d", port, data, i2c_Write_Queued);
        SCHEDULE_WORK(i2c_io_work_struct);
}



int i2c_mod_init(void)
{
        PREPARE_WORK_ITEM(i2c_io_work_struct, &i2c_io_bh, NULL);
        return 0;
}

void i2c_mod_exit(void)
{
        while (PENDING_WORK_ITEM(i2c_io_work_struct) /*|| PENDING_WORK_ITEM(i2c_xcvr_work_struct)*/ ) {
                printk(KERN_ERR"%s: waiting for bh\n", __FUNCTION__);
                schedule_timeout(10 * HZ);
        }
}


