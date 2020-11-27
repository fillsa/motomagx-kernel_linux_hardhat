/*
 *	Fusion Kernel Module
 *
 *	(c) Copyright 2002-2003  Convergence GmbH
 *
 *      Written by Denis Oliver Kropp <dok@directfb.org>
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#ifndef __FUSIONEE_H__
#define __FUSIONEE_H__

#include <linux/poll.h>
#include <linux/fusion.h>

#include "fusiondev.h"
#include "types.h"


/* module init/cleanup */

int  fusionee_init   (FusionDev *dev);
void fusionee_deinit (FusionDev *dev);


/* internal functions */

int fusionee_new           (FusionDev         *dev,
                            int               *id);

int fusionee_send_message  (FusionDev         *dev,
                            int                id,
                            int                recipient,
                            FusionMessageType  msg_type,
                            int                msg_id,
                            int                msg_size,
                            const void        *msg_data);

int fusionee_get_messages  (FusionDev         *dev,
                            int                id,
                            void              *buf,
                            int                buf_size,
                            bool               block);

unsigned
int fusionee_poll          (FusionDev         *dev,
                            int                id,
                            struct file       *file,
                            poll_table        *wait);

int fusionee_kill          (FusionDev         *dev,
                            int                id,
                            int                target,
                            int                signal,
                            int                timeout_ms);

int fusionee_destroy       (FusionDev         *dev,
                            int                id);

#endif
