/*
 *
 * Copyright (C) 2006-2007 Motorola, Inc.
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
 * 09/25/2007   Motorola  Added new  struct 
 *                        flattree_parent_node_stack and modified function
 *                        flattree_add_tree() using iteration rather than 
 *                        recursion
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/err.h>
#include <asm/bootinfo.h>

#include "motflattree.h"

/** ROUND_TO_WORD
 *    rounds x up to the nearest word boundary.
 */
#define ROUND_TO_WORD(x) ( ((x) + 3) & (~3) )

#define DOSWAB(cp,x) (((cp)->mustswab)?swab32(x):(x))

static char * __init flattree_get_name(struct flattree_context *cp, int index);
static int __init flattree_read_prop(struct flattree_context *cp,
                                     struct flattree_prop *read_prop);

typedef struct flattree_parent_node_stack {
    MOTHWCFGNODE *parent;
    struct list_head list;
} parent_stack_t;

/** This function checks to make sure that the input stream looks like a
 *   valid serialized input stream by checking that the values in the header
 *   are as expected.
 *
 * @param cp A pointer to a flattree_context structure.
 * @param input A pointer to the beginning of the serialized tree.
 *
 * @return
 *   This function returns 0 on success or a negative error code if the input
 *   stream is not a serialized flat tree or if it is otherwise corrupted.
 *
 * @note
 *   \li The fthdr and mustswab values of the context are initialized by this
 *   function.
 */
int __init flattree_check_header(struct flattree_context *cp, char *input)
{
    u32 totalsize;
    u32 off_struct;
    u32 off_strs;
    u32 size_strs;
    u32 version;
    char lastchar;

    if(cp == NULL)
        return -EINVAL;

    cp->fthdr = (struct flattree_config_header *) input;

    /* Ensure this is a mothwcfg input file. */
    if (cp->fthdr->magic == FLATTREE_BEGIN_SERIALIZED_OTHEREND)
        cp->mustswab = 1;
    else if (cp->fthdr->magic == FLATTREE_BEGIN_SERIALIZED)
        cp->mustswab = 0;
    else
    {
        printk(KERN_WARNING "%s: Not a valid serialized input stream: magic == 0x%08X\n",
               __FUNCTION__, cp->fthdr->magic);
        return -EINVAL;
    }

    totalsize  = DOSWAB(cp, cp->fthdr->totalsize);
    off_struct = DOSWAB(cp, cp->fthdr->off_struct);
    off_strs   = DOSWAB(cp, cp->fthdr->off_strs);
    size_strs  = DOSWAB(cp, cp->fthdr->size_strs);
    version    = DOSWAB(cp, cp->fthdr->version);
    lastchar   = *( ((char *)cp->fthdr) + (off_strs+size_strs-1) );

    /* Sanity tests on mothwcfg input file. Make sure section
     * offsets are within expected range.
     */
    if ( (off_struct >= totalsize)
          || (off_strs >= totalsize)
          || (version < FLATTREE_MIN_COMPAT_VERSION)
          || ( (off_strs + size_strs) > totalsize )
          || (lastchar != '\0')  )
    {
        printk(KERN_WARNING "%s: Input stream is corrupted; invalid header\n", __FUNCTION__);
        return -EINVAL;
    }

    /* Skip to the beginning of the flat tree and change the value of the
     * pointer for the caller.
     */
    cp->curinput = (u32 *)((u32) input + off_struct);

    return 0;
}

/** This function checks to make sure that the root node and flat tree input
 *   stream are closed off with the expected magic numbers.
 *
 * @param cp A pointer to a flattree_context structure.
 *
 * @return
 *   This function returns 0 on success or a negative error code if the
 *   serialized flat tree does not have the expected end values.
 */
int __init flattree_check_footer(struct flattree_context *cp)
{
    u32 tag;

    tag = DOSWAB(cp, *(cp->curinput));
    if (FLATTREE_END_NODE != tag)
    {
        printk(KERN_WARNING "\
%s: ill-formated input stream: expected 0x%08X, found 0x%08X\n\n",
            __FUNCTION__, FLATTREE_END_NODE, tag );
        return -EINVAL;
    }

    /* Skip past END NODE tag for the root node. */
    cp->curinput += 1;

    /* The input stream is ill-formatted if END SERIALIZED tag is not
     * found.
     */
    tag = DOSWAB(cp, *(cp->curinput));
    if (FLATTREE_END_SERIALIZED != tag)
    {
        printk(KERN_WARNING "\n\
Product Config: ill-formated input stream: expected 0x%08X, found 0x%08X\n\n",
            FLATTREE_END_SERIALIZED, tag);
        return -EINVAL;
    }
    cp->curinput += 1;

    return 0;
}

/** Process node name if node tag is detected.
 *
 * @param cp A pointer to a flattree_context structure.
 *
 * @return
 *   On success, a pointer to the new node's name.<br>
 *   On failure, a negative error value.
 */
char * __init flattree_read_node(struct flattree_context *cp)
{
    char * nodename;
    u32 tag;

    tag = DOSWAB(cp, *(cp->curinput));
    if (tag != FLATTREE_BEGIN_NODE)
        return (ERR_PTR(-EINVAL));

    cp->curinput += 1;

    nodename = (char *) cp->curinput;

    /* Skip past the node name in the input. There are always 1 or more NULL
     * characters at the end of the string, enough to pad to a word boundary.
     */
    cp->curinput = (u32 *) (nodename + ROUND_TO_WORD(strlen(nodename) + 1));

    return nodename;
}

/** This function parses the mothwcfg input file using iteration method, creates each
 *     node along with its properties, and registers the new node with its
 *     parent.
 *
 * @param cp A pointer to a flattree_context structure.
 * @param parent a pointer to parent of the new node. A get should have been done
 *            on parent prior to calling this function.
 *
 * @return
 *   On success: 0<br>
 *   On failure: a negative error value.
 *
 * @note
 *    \li This function expects a get to have been done on parent.
 */
int __init flattree_add_tree(struct flattree_context *cp, MOTHWCFGNODE *parent)
{
    char * nodename;
    MOTHWCFGNODE *hwcfgnode;
    int result = 0;
    u32 tag;

    /* The depth of the tree*/
    int depth_of_input_tree = 1;
    /* The parent number for each node.*/ 
    int parent_stack_depth = 1;

    LIST_HEAD(parent_stack_head);
    parent_stack_t *tmp;

    tmp = kmalloc(sizeof(parent_stack_t), GFP_KERNEL);
    if ( !tmp ) 
    {
        printk(KERN_WARNING "\
	%s: couldn't kmalloc parent_stack_t for parent node\n", __FUNCTION__);
        return -ENOMEM;
    }
    tmp->parent = parent;
    INIT_LIST_HEAD( &tmp->list);

    list_add_tail(&tmp->list, &parent_stack_head);

    /* Read from input stream until the next thing is not a BEGIN NODE tag.
     *
     * flattree_read_node() returns -EINVAL if it encounters something that
     * does not match the BEGIN NODE tag. In this case, that is how we tell
     * that we are at the end.
     */
     while ( !IS_ERR( (nodename = flattree_read_node(cp)) ) )
     {
         ++depth_of_input_tree;
         if (!list_empty(&parent_stack_head))
             parent = list_entry(parent_stack_head.prev, parent_stack_t, list)->parent;
         hwcfgnode = mothwcfg_add_node(parent, nodename);

	 if (IS_ERR(hwcfgnode))
         {
             result = PTR_ERR(hwcfgnode);         
             goto error;
         }

 	 /* After the node's name should follow the node's (possibly empty) list
          * of properties.
          */
         result = flattree_add_all_props(cp, hwcfgnode);
         if (result < 0)
         {
             mothwcfg_put_node(hwcfgnode);
             printk(KERN_WARNING "%s: Failed to add properties for %s node: result == %d\n", __FUNCTION__, nodename, result);
             goto error;
         }
         
	 /* Input sequence is ill-formatted if END NODE tag is not found. */
         tag = DOSWAB(cp, *(cp->curinput));

	 if (FLATTREE_BEGIN_NODE == tag)
         {
             if (parent_stack_depth != depth_of_input_tree)
             {
                 tmp = kmalloc(sizeof(parent_stack_t), GFP_KERNEL);
                 if ( !tmp )
                 {
                     printk(KERN_WARNING "%s: couldn't kmalloc parent_stack_t for parent node\n", __FUNCTION__);
	             goto error;
                 }
                 tmp->parent = hwcfgnode;
                 INIT_LIST_HEAD( &tmp->list);

                 list_add_tail(&tmp->list, &parent_stack_head);
                 ++parent_stack_depth;
             }
             continue;
         }
         while (FLATTREE_END_NODE == tag)
         {
             --depth_of_input_tree;

             /* If depth_of_input_tree reaches 0, we are at the end of the "/"
              * node. Don't do "mothwcfg_put_node" the "/" node. Let the caller
              * do that.
              */
             if (depth_of_input_tree <= 0)
             {
                  result = 0;
                  goto error;
             }

             cp->curinput += 1;
             
	     tag = DOSWAB(cp, *(cp->curinput));
             if ( depth_of_input_tree == parent_stack_depth )
             {
                 mothwcfg_put_node(hwcfgnode);
                 continue;
             }
             else
             {
                 tmp = list_entry(parent_stack_head.prev, parent_stack_t, list);
                 mothwcfg_put_node(tmp->parent);
                 kfree(tmp);
                 list_del(parent_stack_head.prev);
                 --parent_stack_depth;
             }
         }
    }

error:
    list_for_each_entry(tmp, &parent_stack_head, list)
    {
         kfree(tmp);
    }
    
    return result;
}

/** Returns a pointer to the beginning of the name string that is offset by
 *     the specified index in the name strings table.
 *
 * @param cp A pointer to a flattree_context structure.
 * @param index offset into strings character array; index represents the beginning
 *          of the name
 *
 * @return
 *   On success: Pointer to the requested name string.<br>
 *   On failure: -EINVAL to indicate that index is an invalid parameter
 *
 * @note
 *  \li The name strings table is a character array that contains all of the
 *      property names.
 *  \li This function assumes that the fthdr context field has been initialized.
 */
static char * __init flattree_get_name(struct flattree_context *cp, int index)
{
    char * strings_begin = (char *)cp->fthdr + DOSWAB(cp, cp->fthdr->off_strs);
    u32 size_strs = DOSWAB(cp, cp->fthdr->size_strs);

    if ( (index >= 0) && (index < size_strs) )
        return strings_begin + index;

    return ERR_PTR(-EINVAL);
}

/** Reads a property from the hardware config input stream and initializes
 *     the output variables with the name, value, and size of the property.
 *
 * @param cp A pointer to a flattree_context structure.
 * @param read_prop: Pointer to a structure which will be filled in with the size of
 *              the property's value and with pointers to its name and value.
 *
 * @return
 *   0: means no property found at the current offset<br>
 *   1: means a property was found at the current offset<br>
 *  \<0: means a read error occurred
 */
static int __init flattree_read_prop(struct flattree_context *cp,
                                     struct flattree_prop *read_prop)
{
    u32 index;
    u32 tag;

    tag = DOSWAB(cp, *(cp->curinput));
    if (tag == FLATTREE_ATTRIBUTE)
    {
        /* Skip past the "ATTRIBUTE" magic number. */
        cp->curinput += 1;

        /* Get size and skip past it. */
        read_prop->size = DOSWAB(cp, *(cp->curinput));

        cp->curinput += 1;

        /* Next should be index into strings array. */
        index = DOSWAB(cp, *(cp->curinput));
        read_prop->name = flattree_get_name(cp, index);
        if (IS_ERR(read_prop->name))
        {
            printk(KERN_WARNING "%s: Attribute name overruns table: index == %d\n",
                   __FUNCTION__, index);
            return PTR_ERR(read_prop->name);
        }

        /* Skip past name index. */
        cp->curinput += 1;

        read_prop->value = (char *) cp->curinput;

        /* Skip past value in input stream. Value in input stream has been
         * padded to a 32-bit boundary.
         */
        cp->curinput = (u32 *) ((char *)(cp->curinput)
                                        + ROUND_TO_WORD(read_prop->size));

        /* property was read */
        return 1;
    }

    return 0;
}

/** Reads each property from the mothwcfg input file and associates the
 *     property with the given node.
 *
 * @param  cp A pointer to a flattree_context structure.
 * @param  hwcfgnode Associate properties with this node.
 *
 * @return
 *   On success: 0<br>
 *   On failure: a negative error value
 */
int __init flattree_add_all_props(struct flattree_context *cp, MOTHWCFGNODE *hwcfgnode)
{
    int result;
    struct flattree_prop a;

    /* flattree_read_prop() will return 1 if there are attributes to be
     * processed from the input stream. It will return 0 if there are no more
     * attributes to process. Calling flattree_read_prop() when there are no
     * properties for the node is not an error, it just returns 0.  However,
     * it is possible for there to be an error while reading the input stream,
     * so flattree_read_prop() may return a negative value.
     */
    while ((result = flattree_read_prop(cp, &a)) > 0)
    {
        /* Now add the property just read in. */
        result = mothwcfg_add_prop(hwcfgnode, a.name, a.value, a.size);
        if (result < 0)
            break;
    }

    return result;
}
