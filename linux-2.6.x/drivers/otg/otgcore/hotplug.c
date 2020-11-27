/*
 * Copyright (c) 2005-2007 Motorola, Inc. All Rights Reserved
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 12/12/2005         Motorola         Initial distribution
 * 03/16/2006         Motorola         Vendor specific command
 * 05/08/2006         Motorola         Hotplug fix
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
 * 11/23/2007         Motorola         Changes for OSS compliance.
 *
 *  This program is licensed under a BSD license with the following terms:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *  Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *
 *  Neither the name of Motorola nor the names of its contributors may be
 *  used to endorse or promote products derived from this software without
 *  specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* OS-specific #includes */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/ctype.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/utsname.h>
#include <linux/kmod.h>
#include <asm/uaccess.h>
#include <asm/system.h>


/* OTG specific */
#include <otg/otg-compat.h>
#include <otg/otg-module.h>
#include <otg/usbp-chap9.h>
#include <otg/usbp-bus.h>
#include <otg/usbp-func.h>
#include <otg/otg-utils.h>
#include <otg/otg-trace.h>


#include <otg/hotplug.h>


#ifdef CONFIG_OTG_GENERIC_HOTPLUG

/* Hotplug bottom half:
 *
 *  This is a scheduled task that will send do a hotplug call. Because it is
 *  called from the task scheduler context it needs to verify that the
 *  device is still usable.
 *
 *      static int hotplug_attach (u32 ip, u32 mask, u32 router, int attach)
 *      static void hotplug_bh (void *data)
 *      void generic_os_hotplug (void)
 *
 */




/*
 * hotplug_init
 */
void hotplug_init(struct usb_hotplug_private * gen_priv)
{
	RETURN_UNLESS(gen_priv); // might be redundant
	gen_priv->hotplug_status = hotplug_detached;
	gen_priv->hotplug_callback= NULL;
	PREPARE_WORK_ITEM(gen_priv->hotplug_bh, hotplug_bh, gen_priv);


}


/*
 *  call_hotplug - call hotplug
                   A function driver Always have to Pass the  number  of varible
				   number of  arguments followed by environment varibles which
				   should be of type  char * . That is all  Parameters after
				   numVarArgs have to char * .
				   
 */
int call_hotplug(struct usb_hotplug_private * gen_priv, int numVarArgs, ...)
{
   va_list argp;
   char *argv[3];
   char **envp = NULL;
   int i = 0 ;
   int j = 0 ;
   int 	retval; 
  
   // account for standard variables that always need to be
   // in the environment (3)
   int  num_standard_env_vari = 3;
   
	
   RETURN_EINVAL_IF(!gen_priv); // might be redundant
   RETURN_EINVAL_IF(!hotplug_path[0]);

  
   TRACE_MSG2(USBD,"Before calling agent %s hotplug_status = %d",
			             	gen_priv->dev_name,gen_priv->hotplug_status);


   argv[0] = hotplug_path;

   // agent name . agent script names end with .agent
   // For expamle acm_if.agent
   argv[1] = gen_priv->dev_name;  
   argv[2] = NULL;
        
   // allocate space for the environment
   // this is total number of variable number of  arguments plus
   // number of standard environment varibles plus one for terminating NULL
   envp = CKMALLOC((numVarArgs + num_standard_env_vari + 1) * sizeof(char *), GFP_KERNEL);

   if (envp)
   {
       i = 0;
       
	   // populate the standard variables (corresponding to the
	   // "3" in the line above)
       envp[i++] = "HOME=/";
       envp[i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";

	   if( gen_priv->hotplug_status == hotplug_attached)
	   { 
		   envp[i++] = "ACTION=attach";
	   }
	   else
	   {
		   envp[i++] = "ACTION=detach";
	   }
       // end of the standard variables

	   j = 0 ;

       va_start(argp, numVarArgs);

       // loop through the remaining arguments and stick them
       // in the environment
      	   
       while( j < numVarArgs )
	   {
		   envp[i++] = va_arg(argp, char *) ;
		   j++;
	   }
            
       // terminate the list
       envp[i] = NULL;

       va_end(argp);
    }
    else
	{
		printk("\n %s Line=%d  memory  allocation failed "
				,__FUNCTION__,__LINE__);

		return -ENOMEM ; 
	}

	// execute the command
    retval = call_usermodehelper (argv[0], argv, envp, 0);

    // free the environment space
    lkfree(envp);
	
    TRACE_MSG3(USBD,"agent %s hotplug_status = %d  retval from call_usermodehelper = %d",
			             	gen_priv->dev_name,gen_priv->hotplug_status,retval);

    return retval;
}


/* hotplug_bh - bottom half handler to call hotplug script to signal ATTACH or DETACH
 *
 * Check connected status and load/unload as appropriate.
 *
 * It should not be possible for this to be called more than once at a time
 * as it is only called via schedule_task() which protects against a second
 * invocation.
 */
void hotplug_bh (void *data)
{
        struct usb_hotplug_private * gen_priv = (struct usb_hotplug_private *) data;

        RETURN_UNLESS(gen_priv);  // might be redundant

        // provide a way to call a hotplug function without an
        // instance
        if (gen_priv->hotplug_status == hotplug_unknown)
        {
            call_hotplug (gen_priv,0);
            return;
        }

        if ((gen_priv->flags & HOTPLUG_FUNC_ENABLED) && (gen_priv->function_instance && (USBD_OK == usbd_get_device_status(gen_priv->function_instance)) && (STATE_CONFIGURED == usbd_get_device_state(gen_priv->function_instance)))) 
        {
                if (hotplug_attached != gen_priv->hotplug_status) {
                        gen_priv->hotplug_status = hotplug_attached;

						if( gen_priv->hotplug_callback == NULL)
						{
							call_hotplug (gen_priv,0);
						}
						else
						{
							gen_priv->hotplug_callback(gen_priv);
						}
				}
                return;
        }

        if (hotplug_detached != gen_priv->hotplug_status) {
                gen_priv->hotplug_status = hotplug_detached;

				if( gen_priv->hotplug_callback == NULL)
				{
				    call_hotplug (gen_priv,0);
				}
				else
				{
					gen_priv->hotplug_callback(gen_priv);
				}
        }
}

/*
 * generic_os_hotplug - schedule a call to hotplug bottom half
 */

bool g_usb_bus_disable=false;
bool g_usb_bus_disable_ing(){return g_usb_bus_disable;}
void g_usb_bus_disable_set(){g_usb_bus_disable = true;}
void g_usb_bus_disable_clr(){g_usb_bus_disable = false;}


OTG_EXPORT_SYMBOL(g_usb_bus_disable_ing);
OTG_EXPORT_SYMBOL(g_usb_bus_disable_set);
OTG_EXPORT_SYMBOL(g_usb_bus_disable_clr);

void generic_os_hotplug(struct usb_hotplug_private *gen_priv)
{
	struct usbd_function_instance * fi;
	struct usbd_bus_instance *fibus;

        RETURN_UNLESS(gen_priv);
	
	fi = gen_priv->function_instance; 
	RETURN_UNLESS(fi);

	fibus = fi->bus; 
	RETURN_UNLESS(fibus);
	if((fibus->status == USBD_CLOSING)||(fibus->status == USBD_OPENING)){printk("\n USB BUS BUSY, hotplug disable\n"); return;}

        if(g_usb_bus_disable_ing()){ printk("\n BUS DISABLING RETURN\n"); return;}	       
        SET_WORK_ARG(gen_priv->hotplug_bh, gen_priv);

        TRACE_MSG2(USBD,"scheduling work item for agent %s  hotplug_status = %d",
			             	gen_priv->dev_name,gen_priv->hotplug_status);
		
	printk("generic_os_hotplug: status=%d\n", gen_priv->hotplug_status);
        SCHEDULE_WORK(gen_priv->hotplug_bh);
}

#else
/*
 * Do Nothin but Printk's as CONFIG_OTG_GENERIC_HOTPLUG is not defined and some function 
 * driver has their specific HOTPLUG defined.
 */
void hotplug_init(struct usb_hotplug_private * gen_priv)
{
	printk(KERN_INFO"Define CONFIG_OTG_GENERIC_HOTPLUG; or undefine the function specific hotplug\n");
//        // Do nothing, hotplug not configured.
}

int call_hotplug(struct usb_hotplug_private * gen_priv,int numVarArgs, ...)
{
	printk(KERN_INFO"Define CONFIG_OTG_GENERIC_HOTPLUG; or undefine the function specific hotplug\n");

}

void hotplug_bh (void *data)
{
	printk(KERN_INFO"Define CONFIG_OTG_GENERIC_HOTPLUG; or undefine the function specific hotplug\n");
//        // Do nothing, hotplug not configured.
}

void generic_os_hotplug(struct usb_hotplug_private *gen_priv)
{
	printk(KERN_INFO"Define CONFIG_OTG_GENERIC_HOTPLUG; or undefine the function specific hotplug\n");
//        // Do nothing, hotplug not configured.
}

#endif			/* CONFIG_OTG_GENERIC_HOTPLUG */

OTG_EXPORT_SYMBOL(hotplug_bh);
OTG_EXPORT_SYMBOL(hotplug_init);
OTG_EXPORT_SYMBOL(generic_os_hotplug);
OTG_EXPORT_SYMBOL(call_hotplug);
