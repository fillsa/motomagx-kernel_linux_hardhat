/*
 *
 *           (c) Copyright Motorola 2006
 *
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Revision History:
 *
 * Date         Author    Comment
 * ----------   --------  -------------------
 * 11/16/2006   Motorola  Hardware Config Framework
 */
#ifndef __MOTHWCFG_H
#define __MOTHWCFG_H

struct mothwcfg_node;
/** 
 *    The services for creating, modifying, and deleting nodes will take
 *    hardware config node "handles." When a new node is created, a pointer
 *    to the handle structure of the new node will be returned and this pointer
 *    is then passed to the other services which will perform actions upon it
 *    such as adding a property or removing the node.
 * 
 *    The structure is declared here but not defined so that the details are
 *    hidden but type checking can still be done when passing node handles as
 *    arguments.
 *
 *    MOTHWCFGNODE typedefs a structure and not a pointer to a structure so
 *    the user can see that error checks on return values need to be done using
 *    pointer macros.
 */
typedef struct mothwcfg_node MOTHWCFGNODE;

/* prototypes for exported global functions */
int __init mothwcfg_init(void);
    /* Nodes */
MOTHWCFGNODE *mothwcfg_add_node(MOTHWCFGNODE *parent, const char * nodename);
MOTHWCFGNODE *mothwcfg_get_first_child(MOTHWCFGNODE *parent);
MOTHWCFGNODE *mothwcfg_get_next_sibling(MOTHWCFGNODE *nh);
MOTHWCFGNODE *mothwcfg_get_node_by_path(const char * name);
void mothwcfg_put_node (MOTHWCFGNODE *nh);
int mothwcfg_remove_node(MOTHWCFGNODE *nh);

    /* Attributes */
int mothwcfg_add_prop(MOTHWCFGNODE *nh, const char *name, const void *data, u32 size);
int mothwcfg_read_prop(MOTHWCFGNODE *nh, const char * propname, char * buf, u32 size);
int mothwcfg_update_prop(MOTHWCFGNODE *nh, const char *propname, const void *buf, u32 size);
int mothwcfg_remove_prop(MOTHWCFGNODE *nh, const char * propname);

#endif /* __MOTHWCFG_H */
