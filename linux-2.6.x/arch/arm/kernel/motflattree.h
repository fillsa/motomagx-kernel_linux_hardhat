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

#ifndef __MOT_FLATTREE_H
#define __MOT_FLATTREE_H

#include <asm/setup.h>
#include <asm/mothwcfg.h>

/* MAGIC NUMBERS
 *   The following constants define the magic numbers expected to represent the
 *   beginning and end of node sections and property sections in the flat tree
 *   hardware configuration and system information input file.
 */
/* Beginning magic number defined in asm/setup.h so that LBL can access it. */
#define FLATTREE_END_SERIALIZED             0x00000009

/* Beginning and end tags for each node. */
#define FLATTREE_BEGIN_NODE        0x00000001
#define FLATTREE_END_NODE          0x00000002

/* Blob must have been created with at least this version of dtc. */
#define FLATTREE_MIN_COMPAT_VERSION 0x00000010

/* Beginning tag for each property. No end is needed since a size is included
 * as part of the property section.
 */
#define FLATTREE_ATTRIBUTE         0x00000003

/* Input stream structures. */
/* Header for the flat tree input stream. */
struct flattree_config_header {
    unsigned int magic;
    unsigned int totalsize;
    unsigned int off_struct;
    unsigned int off_strs;
    unsigned int off_rsvmap;
    unsigned int version;
    unsigned int last_comp_ver;
    unsigned int boot_cpu_id;
    unsigned int size_strs;
};

/* input_prop: structure used for reading properties from the hardware config
 * input stream.
 */
struct flattree_prop {
    char * name;
    char * value;
    u32 size;
};

/* flattree_context:
 *      A pointer to this structure must be passed to all flattree functions.
 *   fthdr:
 *      This field contains a pointer to the header data of the serialized tree.
 *      The header contains offsets and sizes of the different sections in the
 *      tree and is used to access the different sections. Since it is at the
 *      beginning, offsets are calculated from the address of fthdr.
 *  curinput:
 *      The current offset into the input stream as it is being converted from
 *      the flat tree into the kernel node tree.
 *  mustswab:
 *      This field keeps track of whether or not the input stream is the
 *      opposite endianness from the compiled code. If so, mustswab is 1,
 *      otherwise, it is 0.
 */
struct flattree_context {
    struct flattree_config_header * fthdr;
    u32 * curinput;
    bool mustswab;
};

/* Prototypes for unflattening the serialized input file and creating the nodes
 * and properties in the tree.
 */
char * __init flattree_read_node(struct flattree_context *cp);
int __init flattree_add_tree(struct flattree_context *cp, MOTHWCFGNODE *parent);
int __init flattree_add_all_props(struct flattree_context *cp, MOTHWCFGNODE *hwcfgnode);
int __init flattree_check_header(struct flattree_context *cp,char *input);
int __init flattree_check_footer(struct flattree_context *cp);

#endif /* __MOT_FLATTREE_H */

