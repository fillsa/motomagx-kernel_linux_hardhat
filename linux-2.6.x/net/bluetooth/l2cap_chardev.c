/*
 * 
 * net/bluetooth/l2cap_chardev.c
 *  This file contains implementation for character device driver for l2cap
 *  socket interface. 
 *  The character driver will provide a character device file interface which
 *  can be used as a any other file and perform file operations. 
 *  The user will have option of directly reading and writing to the socket file
 *  descriptor or use the character device file  and perform read and write to the 
 *  character device. The use of character device file for read,
 *  write, poll  will have the same behavior as though reading, writing or polling
 *  to the socket itself.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  Copyright (C) 2006 - Motorola
 *
 *  Date         Author           Comment
 *  -----------  --------------   --------------------------------
 *  2006-Feb-12  Motorola         Implementation for character device driver for l2cap socket interface
 *  2006-Jun-28  Motorola         added an extra parameter 'loff_t *ppos' to l2cap_char_dev_read() and 
 *				  l2cap_char_dev_write()
 *  2006-Nov-02  Motorola         Check socket state before read/write/poll operation.
 *  2006-Nov-15  Motorola         Enable debug using define
 *  2007-Mar-29  Motorola         Hold socket reference count till file is closed.
 */



#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/l2cap.h>

#ifndef CONFIG_BT_L2CAP_DEBUG
#undef  BT_DBG
#define BT_DBG(D...)
#endif

#define L2CAP_CHAR_DEV_FIRST_MINOR        0
#define L2CAP_MAX_CHAR_DEV                16 
#define L2CAP_CHAR_DEV_NAME               "l2cap"
#define SUCCESS                           0
#define L2CAP_CHAR_DEV_AVAIL              NULL 

typedef enum {
    L2CAP_CHAR_DEV_INIT = 0,
    L2CAP_CHAR_DEV_CREATED,
    L2CAP_CHAR_DEV_OPEN,
    L2CAP_CHAR_DEV_CLOSED
}   L2CAP_CHAR_DEV_STATE;

/*
States for L2cap char device file.

The state transition table is: 

Action --->             Create	                Open	               Close	             Release

State                   ----------------------  --------------------   -------------------   ----------

L2CAP_CHAR_DEV_INIT	L2CAP_CHAR_DEV_CREATED	Fail	               Fail	             Fail
L2CAP_CHAR_DEV_CREATED	N/A	                L2CAP_CHAR_DEV_OPEN    L2CAP_CHAR_DEV_INIT   L2CAP_CHAR_DEV_INIT
L2CAP_CHAR_DEV_OPEN	N/A	                Fail	               L2CAP_CHAR_DEV_CLOSE  Fail
L2CAP_CHAR_DEV_CLOSED	L2CAP_CHAR_DEV_CREATED	Fail	               L2CAP_CHAR_DEV_INIT   L2CAP_CHAR_DEV_INIT

*/

struct l2cap_char_dev {
    int                   devId;
    L2CAP_CHAR_DEV_STATE  state;
    struct sock           *sk;
    struct cdev           cdev;
};

static struct l2cap_char_dev *    __l2cap_get_dev(int devId);
static int                        __reserve_avail_devid(struct l2cap_char_dev *pl2capChar);
static void                       __set_dev_status(int devId, struct l2cap_char_dev *pl2cap_char_dev);
static void                       __init_dev_status(void);
static int                        __l2cap_char_dev_rw(struct file *filp,char *buffer,size_t length, char rw);

static int                        L2cap_char_dev_major_num;
struct class_simple               *l2cap_class;
struct l2cap_char_dev             *l2cap_char_dev_list[L2CAP_MAX_CHAR_DEV];
static rwlock_t                   l2cap_char_dev_lock = RW_LOCK_UNLOCKED;


static void __set_dev_status(int devId, struct l2cap_char_dev *pl2cap_char_dev)
{
    write_lock(&l2cap_char_dev_lock);
    
    l2cap_char_dev_list[devId] = pl2cap_char_dev;
    
    write_unlock(&l2cap_char_dev_lock);
}

static int __reserve_avail_devid(struct l2cap_char_dev *pl2capChar)
{
    int i, ret = -ENODEV;
    
    write_lock(&l2cap_char_dev_lock);
    
    for(i=0;i<L2CAP_MAX_CHAR_DEV;i++) {
        if (l2cap_char_dev_list[i] == L2CAP_CHAR_DEV_AVAIL) {
            ret =i;
            l2cap_char_dev_list[i] = pl2capChar;
            break;
         }
    }
    write_unlock(&l2cap_char_dev_lock);
    return ret;
}

static struct l2cap_char_dev *__l2cap_get_dev(int devId)
{
    struct l2cap_char_dev *dev = NULL;

     if(!(devId < 0 || devId >= L2CAP_MAX_CHAR_DEV)) {

         read_lock(&l2cap_char_dev_lock);
 
         dev =  (struct l2cap_char_dev *)l2cap_char_dev_list[devId];
 
         read_unlock(&l2cap_char_dev_lock);
         
    }

    return dev; 
}

static void __init_dev_status()
{
    write_lock(&l2cap_char_dev_lock);
     
    memset(l2cap_char_dev_list, 0 , sizeof(l2cap_char_dev_list));
    
    write_unlock(&l2cap_char_dev_lock);
}

static int init_l2cap_class(void)
{
     l2cap_class = class_simple_create(THIS_MODULE,"l2cap");
     if (IS_ERR(l2cap_class)) {
         return PTR_ERR(l2cap_class);
     }
     return SUCCESS;
}

/* -------- File operations ---------- */

static int l2cap_char_dev_open(struct inode *inode, struct file *filp)
{

    unsigned int           devId;
    struct l2cap_char_dev  *dev;

    /* Get minor number = device number */
    devId = iminor(inode);

    BT_DBG("devId is: %d",devId);
        
    /* Get the device from dev num */
    dev = __l2cap_get_dev(devId);

    /* if no device created, return -ENODEV or -ENODATA */
    if (!dev) {
        BT_DBG(" NO l2cap device created");
        return -ENODEV;
    }
      
    /* check dev->STATE to be in L2CAP_CHAR_DEV_CREATED */
    if (dev->state != L2CAP_CHAR_DEV_CREATED) {
        BT_DBG("l2capdev state not L2CAP_CHAR_DEV_CREATED, state: %d\n",dev->state);
        return -EBADFD;
    }
   
    /* Check valid socket underneath-in good state */
    if (dev->sk->sk_state != BT_CONNECTED) {
       BT_DBG("socket not BT_CONNECTED");    
    }

    /* Update l2cap char dev state */
    dev->state = L2CAP_CHAR_DEV_OPEN;
    
    BT_DBG("sk %p sk->sk_refcnt %d", dev->sk, dev->sk->sk_refcnt);   
    
    /*increment socket refernce count*/
    sock_hold(dev->sk);

    BT_DBG("sk %p sk->sk_refcnt %d", dev->sk, dev->sk->sk_refcnt);   
 
    /*increment module reference count */
    __module_get(THIS_MODULE);

    return SUCCESS;    

}

/* This is performed when close is called on file descriptor reference count to the file is zero */

static int l2cap_char_dev_release(struct inode *inode, struct file *filp)
{
    unsigned int            devId;
    struct l2cap_char_dev   *dev;
    
    /* get minor device num */
    devId = iminor(inode);
    
    /* Get the device from dev num */
    dev = __l2cap_get_dev(devId);
    if (!dev) {
        BT_DBG(" NO l2cap device created");
        return -ENODEV;
    }    
    
    /*decrease the socket reference count */
    sock_put(dev->sk);

    BT_DBG("sk %p sk->refcnt %d",dev->sk, dev->sk->sk_refcnt);

    dev->state = L2CAP_CHAR_DEV_CLOSED;
    cdev_del(&dev->cdev);
        
    /*  free memory malloc'd in _create */
    kfree(dev);
    
    /*  Put device number to free */
    __set_dev_status(devId,L2CAP_CHAR_DEV_AVAIL);
    
     /*remove the class entry  - cannot perform this operation here as /dev/l2cap is being created statically*/
    //class_simple_device_remove(MKDEV(L2cap_char_dev_major_num,devId));    

    BT_DBG("Release dev ID %d",devId);

    module_put(THIS_MODULE);
    
    return SUCCESS;

}
static int __l2cap_char_dev_rw(struct file *filp,char *buffer,size_t length, char rw)
{

    unsigned int            devId;
    struct l2cap_char_dev   *l2capdevice;
    struct socket           *sock = NULL;
    struct iovec            iov;
    struct msghdr           msg;
    int                     flags ;
    
    /* get the monir device number */
    devId = iminor(filp->f_dentry->d_inode);
    
    l2capdevice = __l2cap_get_dev(devId);
    if (!(l2capdevice)) {
        BT_DBG(" NO l2cap device created");
        return -ENODEV;
    }
    
    BT_DBG("l2capdevice exists");

    if(!(l2capdevice->sk)) {
      BT_DBG("l2capdevice->sk does not exists");
      return -ENOTCONN;
    }

    /* check whether the socket it still open */
    if (l2capdevice->sk->sk_state != BT_CONNECTED) {
       BT_DBG("socket state is not BT_CONNECTED");
    }
    
    sock = l2capdevice->sk->sk_socket;
    if(!(sock)) {
        BT_DBG("socket does not exists");
           return -ENOTCONN;
    } 
    
    BT_DBG("sk %p sk->sk_socket %p",l2capdevice->sk, l2capdevice->sk->sk_socket); 
    if(length==0) {
        return 0;
    }
 
    msg.msg_name          = NULL;
    msg.msg_namelen       = 0;
    msg.msg_control       = NULL;
    msg.msg_controllen    = 0;
    msg.msg_iovlen        = 1;
    msg.msg_iov           = &iov;
    iov.iov_len           = length;
    iov.iov_base          = buffer;    
    flags                 = (filp->f_flags & O_NONBLOCK) ? MSG_DONTWAIT :0;


    if (rw == READ) {
        return sock_recvmsg(sock, &msg, length, flags);
    }
    else {
        msg.msg_flags = flags;
        return sock_sendmsg(sock, &msg, length);
    }   
}

static int l2cap_char_dev_read(struct file *filp, char *buffer, size_t length, loff_t *ppos)
{
    BT_DBG("Read");
    return __l2cap_char_dev_rw(filp,buffer,length,READ);
}

static int l2cap_char_dev_write(struct file *filp, char *buffer, size_t length, loff_t *ppos)
{
    BT_DBG("write");
    return __l2cap_char_dev_rw(filp,buffer,length,WRITE);    
}

static unsigned int l2cap_char_dev_poll(struct file *filp, poll_table *wait)
{
    unsigned int           devId;
    struct l2cap_char_dev  *l2capdevice;
    struct socket          *sock = NULL;

    devId = iminor(filp->f_dentry->d_inode);
    l2capdevice = __l2cap_get_dev(devId);
    if (!(l2capdevice)) {
        BT_DBG("No l2cap device is created for this dev id");
        return -ENODEV;
    }
    
    if(!(l2capdevice->sk)) {
      BT_DBG("l2capdevice->sk does not exists\n");
      return -ENOTCONN;
    }
 
    if (l2capdevice->sk->sk_state != BT_CONNECTED) {
       BT_DBG("socket state is not BT_CONNECTED");
    }

    sock = l2capdevice->sk->sk_socket;
    if(!(sock)) {
        BT_DBG("socket does not exists");
           return -ENOTCONN;
    } 
    return sock->ops->poll(filp,sock,wait);
}

static int l2cap_char_dev_fsync(struct file *filp, struct dentry *dentry, int datasync)
{
    unsigned int           devId;
    struct l2cap_char_dev  *l2capdevice;

    devId = iminor(filp->f_dentry->d_inode);
    BT_DBG("flush device id: %d",devId);
    
    l2capdevice = __l2cap_get_dev(devId);
    if (!(l2capdevice)) {
        BT_DBG("No l2cap device is created for this dev id");
        return -ENODEV;
    }
    
    if(!(l2capdevice->sk)) {
      BT_DBG("l2capdevice->sk does not exists\n");
      return -ENOTCONN;
    }
    
    skb_queue_purge(&l2capdevice->sk->sk_write_queue);
    
    return 0;
}


/* ---------- Device IOCTLS ------------ */

static struct file_operations l2cap_fops = {
    .owner    = THIS_MODULE,
    .open    = l2cap_char_dev_open,
    .release = l2cap_char_dev_release,
    .read    = l2cap_char_dev_read,
    .write   = l2cap_char_dev_write,
    .poll    = l2cap_char_dev_poll,
    .fsync    = l2cap_char_dev_fsync,
};

static int l2cap_create_dev(struct sock *sk, int *arg)
{
    struct l2cap_char_dev *l2capdevice;
    int                    devId;
    int                    result;
    dev_t                  dev;
    
    
    if(sk->sk_state != BT_CONNECTED) {
       BT_DBG("Socket not connected");
       return -EBADFD;
    }

    BT_DBG("sk %p", sk);

    l2capdevice = kmalloc(sizeof(struct l2cap_char_dev),GFP_KERNEL);
    if (!l2capdevice) {
       return -ENOMEM;
    }
    memset(l2capdevice,0,sizeof(struct l2cap_char_dev));        

    devId = __reserve_avail_devid(l2capdevice);
    if(devId == -ENODEV) {
        BT_DBG("Invalid device ID");
        kfree(l2capdevice);
        return -1;
    }
    
    /* Initialize the l2cap device info */
    l2capdevice->state  = L2CAP_CHAR_DEV_CREATED;
    l2capdevice->sk     = sk;
    l2capdevice->devId  = devId;

    cdev_init(&l2capdevice->cdev,&l2cap_fops);
    l2capdevice->cdev.owner = THIS_MODULE;
   
    /* creates the char device for the given device id as minor number and l2cap device major number */
    dev = MKDEV(L2cap_char_dev_major_num,devId);
    
    class_simple_device_add(l2cap_class,dev,NULL,"l2cap%d",devId);
    
    result = cdev_add(&l2capdevice->cdev,dev,1);
    if(result) {
    
        BT_DBG("Error %d adding l2cap device /dev/l2cap%d", result,devId);
        
        kfree(l2capdevice);
        __set_dev_status(devId,L2CAP_CHAR_DEV_AVAIL);
        class_simple_device_remove(MKDEV(L2cap_char_dev_major_num,devId));  
    
        return result;
    }
    
    BT_DBG("Successfully added /dev/l2cap%d ",devId);    
    
    return devId;
}

static int l2cap_release_dev(int *arg)
{
    struct l2cap_char_dev *l2capdevice;
    int                   devId;

    if(copy_from_user(&devId,arg,sizeof(devId))) {
        printk("copy from user failed --\n");
        return -EFAULT;
    }
    
    BT_DBG("ioctl release devId: %d",devId);
    
    l2capdevice = __l2cap_get_dev(devId);
    
    if(!(l2capdevice)) {
        BT_DBG("No l2cap device is created for this dev id");
        return -ENODEV;
    }
    
    if(l2capdevice->state == L2CAP_CHAR_DEV_OPEN) {
        BT_DBG("Cannot release device, the l2cap device is still open");
        return -EINVAL;
    }

    
    if(l2capdevice->state != L2CAP_CHAR_DEV_CLOSED) {
        
        cdev_del(&l2capdevice->cdev);
        
        /*  free memory malloc'd in _create */
        kfree(l2capdevice);
        
        /*remove the class entry */
        class_simple_device_remove(MKDEV(L2cap_char_dev_major_num,devId));    
    
    }
    
    /*  Put device number to free */
    __set_dev_status(devId,L2CAP_CHAR_DEV_AVAIL);

    return SUCCESS;
}

int l2cap_char_dev_ioctl(struct sock *sk, unsigned int cmd, int *arg)
{
    switch(cmd) {
    case L2CAPCREATEDEV:
        return l2cap_create_dev(sk,arg);
    case L2CAPRELEASEDEV:
        return l2cap_release_dev(arg);
    default :
        return -EINVAL;
    }
}

/* L2cap device module initialization */
int l2cap_char_dev_init(void)
{
    int   result;
    dev_t l2cap_device;
 
    result = alloc_chrdev_region(&l2cap_device,L2CAP_CHAR_DEV_FIRST_MINOR, L2CAP_MAX_CHAR_DEV,L2CAP_CHAR_DEV_NAME);

    if (result < 0 ) {
        BT_ERR("Failed to alloate device numbers, err : %d\n",result);
        return result;
    }

    L2cap_char_dev_major_num = MAJOR(l2cap_device);
    
    /* create directory in sysfs in /sys/class */
    result = init_l2cap_class();
    if (result < 0) {
        BT_DBG("Error creating l2cap class , cannot create device file \n");    
        return result;
    }

    /*initialize status of device id */
    __init_dev_status();

    BT_INFO("L2CAP device layer initialized, major l2cap char device no. %d",L2cap_char_dev_major_num);
    return SUCCESS;
}

/* L2cap device module cleanup */
void l2cap_char_dev_exit(void)
{
    /* remove l2cap class entry */
    class_simple_destroy(l2cap_class);

    /* Free device numbers */
    unregister_chrdev_region(MKDEV(L2cap_char_dev_major_num,L2CAP_CHAR_DEV_FIRST_MINOR),L2CAP_MAX_CHAR_DEV);
    BT_INFO("Removed L2CAP device layer");
}
