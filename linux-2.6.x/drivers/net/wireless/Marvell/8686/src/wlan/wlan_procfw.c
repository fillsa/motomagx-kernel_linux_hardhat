/** @file wlan_procfw.c
  * @brief This file used to download firmware from proc
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
/********************************************************
Change log:
	
********************************************************/
#include 	"include.h"
#include <linux/vmalloc.h>

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief helper proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return 	   number of data
 */
static int
wlan_helper_write(struct file *f, const char *buf, unsigned long cnt,
                  void *data)
{
    struct net_device *netdev = data;
    wlan_private *priv = netdev->priv;
    char *pdata;

    MODULE_GET;
    if (priv->adapter->fwstate != FW_STATE_LOADING) {
        MODULE_PUT;
        return cnt;
    }
    if (priv->adapter->helper_len) {
        vfree(priv->adapter->helper);
        priv->adapter->helper_len = 0;
    }
    pdata = (char *) vmalloc(cnt);
    if (pdata == NULL) {
        MODULE_PUT;
        return cnt;
    }
    if (copy_from_user(pdata, buf, cnt)) {
        vfree(pdata);
        MODULE_PUT;
        return 0;
    }
    priv->adapter->helper = pdata;
    priv->adapter->helper_len = cnt;
    MODULE_PUT;
    return cnt;
}

/** 
 *  @brief alloc/realloc fw buf
 *
 *  @param priv	   pointer to wlan_private structure
 *  @param cnt     extra buf length for allocate
 *  @return 	   buf pointer
 */
char *
wlan_allocate_fw_buf(wlan_private * priv, unsigned long cnt)
{
    char *pdata;
    unsigned long newsize;

    if (priv->adapter->fmimage_len) {
        newsize = priv->adapter->fmimage_len + cnt;
        pdata = (char *) vmalloc(newsize);
        if (pdata == NULL)
            return NULL;
        memcpy(pdata, priv->adapter->fmimage, priv->adapter->fmimage_len);
        vfree(priv->adapter->fmimage);
        priv->adapter->fmimage = pdata;
        pdata = priv->adapter->fmimage + priv->adapter->fmimage_len;
    } else {
        pdata = (char *) vmalloc(cnt);
        if (pdata == NULL)
            return NULL;
        priv->adapter->fmimage = pdata;
    }
    return pdata;
}

/** 
 *  @brief fwimage proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return 	   number of data
 */
static int
wlan_fwimage_write(struct file *f, const char *buf, unsigned long cnt,
                   void *data)
{
    struct net_device *netdev = data;
    wlan_private *priv = netdev->priv;
    char *pdata;

    MODULE_GET;
    if (priv->adapter->fwstate != FW_STATE_LOADING) {
        MODULE_PUT;
        return cnt;
    }
    pdata = wlan_allocate_fw_buf(priv, cnt);
    if (pdata == NULL) {
        MODULE_PUT;
        return cnt;
    }
    if (copy_from_user(pdata, buf, cnt)) {
        MODULE_PUT;
        return 0;
    }
    priv->adapter->fmimage_len += cnt;
    MODULE_PUT;
    return cnt;
}

/** 
 *  @brief loading proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return 	   number of data
 */
static int
wlan_loading_write(struct file *f, const char *buf, unsigned long cnt,
                   void *data)
{
    struct net_device *netdev = data;
    wlan_private *priv = netdev->priv;
    char databuf[10];
    int loading;
    MODULE_GET;
    if (cnt > 10) {
        MODULE_PUT;
        return cnt;
    }
    if (copy_from_user(databuf, buf, cnt)) {
        MODULE_PUT;
        return 0;
    }
    loading = string_to_number(databuf);
    switch (loading) {
    case FW_STATE_LOADING:
        if (priv->adapter->helper_len) {
            vfree(priv->adapter->helper);
            priv->adapter->helper_len = 0;
            priv->adapter->helper = NULL;
        }
        if (priv->adapter->fmimage_len) {
            vfree(priv->adapter->fmimage);
            priv->adapter->fmimage_len = 0;
            priv->adapter->fmimage = NULL;
        }
        priv->adapter->fwstate = FW_STATE_LOADING;
        break;
    case FW_STATE_DONE:
        priv->adapter->fwstate = FW_STATE_DONE;
        if (priv->adapter->helper_len && priv->adapter->fmimage_len) {
            wlan_setup_station_hw(priv);
            vfree(priv->adapter->helper);
            vfree(priv->adapter->fmimage);
            priv->adapter->helper_len = 0;
            priv->adapter->fmimage_len = 0;
        }
        break;
    case FW_STATE_ERROR:
        PRINTM(MSG, "proc: call wlan_fatal_error_handle\n");
        wlan_fatal_error_handle(priv);
        break;
    default:
        break;
    }
    MODULE_PUT;
    return cnt;
}

/** 
 *  @brief loading proc read function
 *
 *  @param page	   pointer to buffer
 *  @param s       read data starting position
 *  @param off     offset
 *  @param cnt     counter 
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return 	   number of output data
 */
static int
wlan_loading_read(char *page, char **s, off_t off, int cnt, int *eof,
                  void *data)
{
    char *p = page;
    struct net_device *netdev = data;
    wlan_private *priv = netdev->priv;
    MODULE_GET;
    p += sprintf(p, "%d\n", priv->adapter->fwstate);
    MODULE_PUT;
    return p - page;
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief create firmware proc file
 *
 *  @param priv	   pointer wlan_private
 *  @param dev     pointer net_device
 *  @return 	   N/A
 */
void
wlan_firmware_entry(wlan_private * priv, struct net_device *dev)
{
    struct proc_dir_entry *r;

    if (priv->proc_entry == NULL)
        return;
    priv->proc_firmware = proc_mkdir("firmware", priv->proc_entry);
    if (priv->proc_firmware) {
        r = create_proc_entry("loading", 0644, priv->proc_firmware);
        if (r) {
            r->data = dev;
            r->read_proc = wlan_loading_read;
            r->write_proc = wlan_loading_write;
            r->owner = THIS_MODULE;
        } else
            PRINTM(MSG, "Fail to create proc loading\n");
        r = create_proc_entry("helper", 0420, priv->proc_firmware);
        if (r) {
            r->data = dev;
            r->write_proc = wlan_helper_write;
            r->owner = THIS_MODULE;
        } else
            PRINTM(MSG, "Fail to create proc helper\n");
        r = create_proc_entry("fwimage", 0420, priv->proc_firmware);
        if (r) {
            r->data = dev;
            r->write_proc = wlan_fwimage_write;
            r->owner = THIS_MODULE;
        } else
            PRINTM(MSG, "Fail to create proc fwimage\n");
    }
}

/** 
 *  @brief remove firmware proc file
 *
 *  @param priv	   pointer wlan_private
 *  @return 	   N/A
 */
void
wlan_firmware_remove(wlan_private * priv)
{
    if (priv->adapter->helper_len) {
        vfree(priv->adapter->helper);
        priv->adapter->helper_len = 0;
        priv->adapter->helper = NULL;
    }
    if (priv->adapter->fmimage_len) {
        vfree(priv->adapter->fmimage);
        priv->adapter->fmimage_len = 0;
        priv->adapter->fmimage = NULL;
    }
    if (priv->proc_firmware) {
        remove_proc_entry("loading", priv->proc_firmware);
        remove_proc_entry("fwimage", priv->proc_firmware);
        remove_proc_entry("helper", priv->proc_firmware);
    }
    if (priv->proc_entry) {
        remove_proc_entry("firmware", priv->proc_entry);
    }
    return;
}
