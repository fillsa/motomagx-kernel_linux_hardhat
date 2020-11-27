/*
 * otg/otg/otg-list.h
 * @(#) balden@seth2.belcarratech.com|otg/otg/otg-list.h|20051116205001|26704
 *
 *      Copyright (c) 2004 Belcarra
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

#ifndef _OTG_LIST_H
#define _OTG_LIST_H 1

/*
	This file defines doubly linked list capabilities 
	needed. Many operating systems have built in capabilities
	which match and may possibly be borrowed.
        In that case this file will not be needed.

        The implementation here is for demonstration purposes.
        Notes: the operations here may require protection from
        interruption. This is OS-dependent and not shown here.
*/

/* A list node structure is the anchor of a list */
/* A pointer to a node is used to identify items in the list */
/* A node structure is embedded in the items */
struct otg_list_node { 
	struct otg_list_node *previous, *next;
};
typedef struct otg_list_node otg_list_node_t, *otg_list_node_ptr;

//A macro to define an list node within a structure
#define OTG_LIST_NODE     struct otg_list_node  
#define OTG_LIST_NODE_INIT(name) struct otg_list_node name = {&name, &name};

/*
 * Each item in the list is a C structure. Each item is a CONTAINER. Within
 * each container is a member of type OTG_LIST_NODE. A CONTAINER is also
 * called an ENTRY
 */
//Assume that an OTG_LIST_NODE pointer is embedded as structure
// Recover numerical offset of member "member" within a pointer to a "type"
#define __OTG_PTR_OFFSET(type, member) (u32) (   &(((type *) 0)->member) )
//Recover typed pointer from pointer to interior of type at the given offset
#define __OTG_PTR_CONTAINER(ptr, type, member) (type *) ((char *) ptr - __OTG_PTR_OFFSET(type,member))
// From a pointer to the embedded NODE within an ENTRY, calculate a pointer to the 
// ENTRY itself, properly typecast

#define OTG_LIST_ENTRY(pointer, type, member) __OTG_PTR_CONTAINER(pointer, type, member)

//Recover list header embedded in structure which is a member of a list
#define OTG_LIST_MEMBER(pointer, member) &(ptr->member)


//Add an entry at the logical end of the list

//#define INLINE inline
#define OTG_LIST_ADD_TAIL(new_entry, list_head) __otg_list_add_tail(new_entry, list_head)
static INLINE void __otg_list_add_tail(struct otg_list_node *new_entry, struct otg_list_node *list_head)
{
	LIST_NODE  *old_previous = list_head->previous;
	old_previous->next = new_entry;
        new_entry->next = list_head;
        new_entry->previous = old_previous;
        list_head -> previous = new_entry;

}

//Delete an entry from the surrounding list
#define OTG_LIST_DEL_ENTRY(del_entry)  __otg_list_del_entry(&(del_entry))

static void INLINE __otg_list_del_entry(struct otg_list_node *del_entry){
	//Remove from list 
	del_entry->previous->next = del_entry->next;
	del_entry->next->previous = del_entry->previous;
	//Invalidate the entry
	del_entry->previous = NULL;
	del_entry->next = NULL;
}

#define  OTG_LIST_FOR_EACH(cursor, list) for(cursor=(list)->next; cursor != (list); cursor=cursor->next)

#define LIST_NODE_INIT(name) OTG_LIST_NODE_INIT(name)
#define LIST_ENTRY(pointer,type,member) OTG_LIST_ENTRY(pointer,type,member)
#define LIST_ADD_TAIL(new_entry, list_head) OTG_LIST_ADD_TAIL(new_entry, list_head)
#define LIST_DEL_ENTRY(del_entry) OTG_LIST_DEL_ENTRY(del_entry)
#define LIST_FOR_EACH(cursor, list)  OTG_LIST_FOR_EACH(cursor, list)

#endif /*_OTG_LIST_H */
