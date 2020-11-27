/*
 * Copyright 2006 Motorola, Inc. All Rights Reserved
 *
 * Changelog:
 * Date               Author           Comment
 * -----------------------------------------------------------------------------
 * 02/12/2006         Motorola         Initial distribution
 * 05/08/2006         Motorola         hotplug fix
 * 03/16/2006         Motorola         vendor specific command
 * 10/18/2006         Motorola         Add Open Src Software language
 * 12/12/2006         Motorola         Changes for Open Src compliance.
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


#define HOTPLUG_FUNC_ENABLED 0x10

typedef enum generic_hotplug_status {
	hotplug_detached,
	hotplug_attached,
        hotplug_unknown,        
        } generic_hotplug_status_t;


extern void hotplug_bh (void *);

struct usb_hotplug_private {
	int flags;
	WORK_ITEM hotplug_bh;
	char *dev_name;
	generic_hotplug_status_t hotplug_status;
	struct usbd_function_instance * function_instance;

    // a function that's called from within the hotplug bh

    int (*hotplug_callback)(struct usb_hotplug_private * gen_priv);

 };

extern void generic_os_hotplug(struct usb_hotplug_private *);
extern void hotplug_init(struct usb_hotplug_private *);
extern int call_hotplug(struct usb_hotplug_private * gen_priv, int numVarArgs, ...);


