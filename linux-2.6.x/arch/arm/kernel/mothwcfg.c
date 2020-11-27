/*
 *
 *           (c) Copyright Motorola 2006-2007
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
 * 04/26/2007   Motorola  Moved work after init header
 * 09/25/2007   Motorola  Modified locked_remove_node() using iteration
 *                        rather than recursion and add two new struct,
 *                        struct node_sibling_struct and struct 
 *                        flattree_node_stemma
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/err.h>
#include <linux/slab.h>
#ifdef CONFIG_MOT_FEAT_BOOTINFO
#include <asm/bootinfo.h>
#endif /* CONFIG_MOT_FEAT_BOOTINFO */

#include <linux/stat.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/bootmem.h>

#include <asm/mothwcfg.h>
#include "motflattree.h"

extern void mot_free_flat_dev_tree_mem(unsigned long start);

/**
 * @page design_page Hardware Configuration Tree Design
 * The hardware and system properties configuration tree framework requires that
 * a tree of nodes be created and maintained in a searchable hierarchy of nodes
 * where each node can have both children and properties. Also, this tree must
 * be accessible from both kernel and user space.
 *
 * In order to meet these requirements, this implementation uses kobjects as
 * the basis for the tree structure. This is so that existing kernel data
 * structures and services can be used to create the tree, add and remove
 * nodes, update properties, search the tree, and create readable nodes in the
 * /sys filesystem which is accessible from user space. kobjects are the basis
 * for the kset and subsystem kernel data structures which are used in this
 * implementation to build the configuration framework tree.
 *
 * <B> KOBJECTS and ATTRIBUTES: </B>
 *
 *   @note It may seem that the words "attribute" and "property" are used
 *         interchangeably in these comments. They are not. "attributes" refer
 *         to Linux kernel data structures associated with Linux kernel
 *         kobjects. "properties" refer to the concept of device properties
 *         associated with device and system nodes in a hardware configuration
 *         tree.
 *
 *   Each kobject represents a node in the tree and has a directory entry
 *   somewhere in the /sys filesystem. (Actually, kobjects can exist outside
 *   of the /sys filesystem, and we will make use of that in this design, but
 *   we will ignore this fact for now.) Each kobject can be the parent of other
 *   kobjects (i.e., it can have children which show up as subdirectories of the
 *   kobject in the /sys filesystem) and can have attributes associated with it.
 *   Attributes of a kobject show up as files under the kobject's directory.
 * <br>
 *   The kobject "attribute" structure is defined by the kernel as follows:<br>
 * <pre>
 *       struct attribute {
 *           char            * name;
 *           struct module   * owner;
 *           mode_t          mode;
 *       };
 * </pre>
 *   The name holds the string that is used to identify the file under the
 *   kobject's directory in /sys. The owner is the module name, but this does
 *   not apply in our case. The mode is the file permissions for the attribute
 *   file. Attributes are used in this design to represent device properties,
 *   and we do not want these properties to be modified from user space.
 *   Therefore, in our case, we will always set mode to be read-only.
 *
 * <B>   ATTRIBUTE STRUCTURE vs. BINARY ATTRIBUTE STRUCTURE </B><br>
 *
 *       This implementation uses the sysfs attribute infrastructure as opposed
 *       to the sysfs binary attribute infrastructure. This section attempts to
 *       explain why this choice was made despite that sysfs binary attributes
 *       seem to fit the requirements better in many ways than sysfs
 *       attributes.
 *
 *       One drawback of using the kernel "attribute" structure is that it is
 *       not possible to associate attribute accessor methods (covered later)
 *       with individual attributes. This is because the accessor methods are
 *       associated with the kobject, not with the individual attributes.
 *
 *       Another problem is that the accessor methods for attributes described
 *       by the "attribute" structure only allow data of up to 4K (the size
 *       of 1 memory page). If data is larger than that, there is no way to
 *       provide that to the caller in user space.
 *
 *       Another problem with using the "attribute" structure is that, by
 *       convention, when reading attribute data from user space, the output
 *       should be text that is presented in human-readable format.
 *
 *       The sysfs way of dealing with these concerns is to provide methods
 *       and data structures for binary attributes. And, in our implementation,
 *       binary attributes would be perfect, except that the accessor methods
 *       of binary attributes *MUST* have a one-to-one correspondence with the
 *       binary attribute structure. This prevents a single accessor method from
 *       being used for multiple attributes because there is no way in the
 *       interface for a binary accessor method to determine which attribute it
 *       is being called for. Each binary accessor method "just knows" that
 *       since it is running, it must access the data for the one specific
 *       binary attribute it was meant for. Therefore, using binary attribute
 *       structures as the basis for our device properties would make it
 *       impossible to build a node/property tree using arbitrary input data.
 *
 *       This implementation builds quasi-binary attribute structures out of
 *       "attribute" structures. However, it does not address the concern that
 *       the size of the data when accessed from user space is limited to 4K.
 *<br>
 *   So, using the "attribute" structure with the kobject and the supporting
 *   kernel functions helps us meet some of our goals: it associates the
 *   property with the node, it makes a file with the property's name in the
 *   node's directory, and it provides a means to associate accessor methods
 *   with properties in a way that allows us to build the node/property tree at
 *   run-time.
 *
 *   However, not all of our goals are met with this mechanism, so some
 *   extensions must be implemented. The following "mothwcfg_props" structure
 *   contains the kobject "attribute" structure in the "attr" field. It also
 *   contains additional fields to accommodate the requirements of the system
 *   configuration tree.
 *
 * <B>   PROPERTY'S VALUE and SIZE </B><br>
 *       One drawback of the "attribute" structure is that it does not contain
 *       either a data or size field. This is because the common use case for
 *       sysfs attributes is for the accessor method to "just know" everything
 *       there is about the requested attribute based on the attribute's name.
 *       However, in our case, we are implementing generic properties with
 *       common accessor functions, so we need to separately associate both size
 *       and data with each property. The "data" field in the "mothwcfg_props"
 *       structure is an empty array of unsigned chars whose size will be
 *       calculated and added when the structure is kmalloc-ed.
 *
 *<B>   SEARCHING the PROPERTIES </B><br>
 *       Although attributes are associated with a kobject, the only way to
 *       search an attribute list from within the kobject is to access the
 *       kobject's directory entry, check each child's type for "attribute,"
 *       compare each one for the requested name, and, if found, convert the
 *       child's file structure to an "attribute" structure.
 * 
 *       This would work for properties in the hardware configuration tree,
 *       except that there are some cases where a device node can exist outside
 *       of the tree. In These cases, there would not be a searchable directory
 *       entry for the node. To accommodate these cases, we keep a separate
 *       linked list of quickly searchable mothwcfg_props structures for each
 *       node that remains available even when the node is not in the tree.
 *
 *   <B> ALLOCATION of SPACE for the PROPERTY NAME STRING </B><br>
 *       Before associating the attribute structure with a kobject and creating
 *       the attribute file in the /sys filesystem, a pointer to a name string
 *       must be assigned to the "name" field in the attribute structure. Since
 *       this implementation is generic and doesn't know the name string until
 *       run time, the space for the name string must be allocated. Rather than
 *       kmalloc space for the "mothwcfg_props" structure and then do a separate
 *       kmalloc for the name string, the "mothwcfg_props" structure contains a
 *       "name" field of its own which is a character array of
 *       MOTHWCFG_PROP_NAME_LENGTH bytes. A pointer to this "name" is then
 *       assigned to the "name" field in the attribute structure.
 *
 *       According to the config schema, an attribute's name is 31 characters.
 *       Therefore, MOTHWCFG_PROP_LENGTH is 32 to account for the NULL character.
 *
 * <B>KSETS</B><br>
 *   Kobjects are just the building blocks of the sysfs and are not useful
 *   unless they can be linked in to the /sys filesystem somewhere. They also
 *   have some deficiencies as far as this implementation is concerned.
 *
 *   One deficiency is that while any given kobject can trace its parentage
 *   back up the tree, a given kobject can't tell what child nodes (that
 *   is, directories) it has unless you go through its directory entry in the
 *   sysfs.
 *
 *   Another deficiency is that locking is necessary to keep consistency when
 *   modifying a group of related kobjects such as those in the hardware and
 *   system configuration tree.
 *
 *   In order to allow searching of a node's children and provide consistency
 *   when reading and updating the tree, each node in the configuration tree
 *   will be a kset.
 *
 *   Ksets contain kobjects and, when created, will have a directory in the
 *   sysfs tree as well as associated attributes, just like kobjects do. In
 *   addition, ksets contain a linked list of the kobjects in the kset. Any
 *   kobject that is located anywhere in the tree could be in a kset, but for
 *   this implementation, only the direct child nodes of a kset's kobject are in
 *   each kset.
 *
 *   Ksets also provide a pointer to a subsystem (covered below). The subsystem
 *   is the root node of the tree, links the tree into the /sys filesystem, and
 *   provides a read/write semaphore for locking around accesses and
 *   modifications of nodes in the subsystem's tree.
 *
 *   <B>IS THE KSET OVERHEAD NECESSARY?</B><br>
 *       It should be noted that by making each node a kset instead of just
 *       kobjects costs 20 bytes per node. One may wonder if there is a way
 *       to use kobjects and accomplish the same goals of the hardware
 *       configuration tree.
 *
 *       It may actually be possible to save 12 of the 20 bytes per node by
 *       creating our own container structure for each kobject. This container
 *       structure would contain a childlist and would use the entry field of
 *       the kobject structure as the sibling list for each node. However, this
 *       is very non-standard, and should only be considered if RAM space on
 *       the phone is very critical.
 * 
 *   <B>ACCESSING PROPERTIES via ACCESSOR METHODS</B><br>
 *       If a kobject will ever have any attributes, a "show" method must be
 *       associated with the kobject. This method will be used to read the data
 *       of an property whenever its file is opened and read from user space. A
 *       separate accessor will be called from within the kernel. A "store"
 *       method may also be associated with the kobject, but in this
 *       implementation, stores are not allowed from user space. A separate
 *       store method will be provided for kernel space use. A pointer to the
 *       kernel "attribute" structure is passed to the "show" and "store"
 *       methods so that they can determine which property to access.
 *
 *       In order to associate an attribute accessor function with a kobject,
 *       the "type" field of either the kobject or the kset must be assigned a
 *       pointer to a "kobj_type" structure. If a kobject is a member of a kset
 *       (as in this implementation) , the pointer in the parent kset's "type"
 *       field overrides the "type" field in the kobject. Since this
 *       implementation always uses ksets to create nodes, the "type" field in
 *       the "kobject" structure is ignored, and the accessor referenced by the
 *       parent's "ktype" structure is used to access the kobject's attributes.
 *
 *       The "kobj_type" structure must be filled in with pointers to the
 *       appropriate accessor functions prior to assigning it's address to the
 *       "type" field in the kset.
 *
 *       The "mothwcfg_ktype" structure contains the common "show" method for
 *       all attributes in the subsystem. This can be overridden in a specific
 *       node if the need arises, but this implementation uses the same accessor
 *       functions for all attributes.
 *
 *   <B>SEPARATE LIST OF NODE PROPERTIES</B><br>
 *       As mentioned earlier (see "SEARCHING the PROPERTIES" above), a list of
 *       node properties is kept with each node.  This property list is so that
 *       the property structures for a node can be accessed even when the node
 *       is not in the hardware configuration tree. In order to keep the
 *       separate property list, the mothwcfg_node structure was created with
 *       two fields: "proplist" and "kset". The proplist field is a pointer to
 *       the "entry" field in the mothwcfg_props structure of the first
 *       property in the list. The "kset" field is a kset representing the node.
 *
 * <B>RELEASE FUNCTIONS</B><br>
 *   Pointers to release functions are also provided for each node. The release
 *   function is called when the reference count on the node goes to 0.
 *
 * <B>SUBSYSTEMS</B><br>
 *   Subsystems express a sysfs concept of a top level node in the sysfs tree.
 *   Each subtree in the /sys filesystem should be part of a subsystem so that
 *   node creation, deletion, and modification can be serialized with a
 *   read/write semaphore provided by the subsystem structure. In fact, a
 *   subsystem is just a kset with a read/write semaphore. However, the
 *   subsystem methods are the ones which link the whole thing into the /sys
 *   directory structure.
 *
 *   In our implementation, the subsystem is called /sys/mothwcfg and has root
 *   properties. /sys/mothwcfg acts as the root node for the hardware and system
 *   configuration tree. Since a subsystem is a special node, it needs a
 *   separate kobj_type structure. In our implementation, we are most concerned
 *   about having the appropriate release method.
 *
 *   The subsystem can have different accessor functions for its properties, but
 *   in this case the common ones are used.
 *
 *   The subsystem must have a unique release function.
 *
 * <B>SUBSYSTEM STRUCTURE SIMILAR TO NODE STRUCTURE</B><br>
 * The mothwcfg_subsys structure is similar to the mothwcfg_node structure in
 * that it keeps a separate list of properties with the subsystem. This
 * structure type must be defined so that the root node can be treated like
 * any other node in the hardware configuration tree.
 *
 * @note This implementation is a little risky since it relies on the
 *    kset field in 'struct subsystem' being first. In order to treat the root
 *    node the same as any other node, the mothwcfg_subsys structure must be
 *    mapped to a mothwcfg_node structure. This will work automatically when
 *    node handles are passed into the hardware configuration services as long
 *    as the "kset" and "subsystem" structures match field-for-field up to the
 *    end of the "kset" structure. They do match now, but if the kset field in the
 *    subsystem structure is ever moved within that structure, this
 *    implementation will break. A check has been added that will cause the
 *    initialization to fail if these two structures ever diverge.<br><br>
 *    This implementation was chosen so that the root node could be the
 *    subsystem and root container for the hardware configuration tree. This
 *    allows us to have the root in sysfs be /sys/mothwcfg. If we want to play
 *    it safe and not do the mapping of node handles, we could make the root
 *    node be separate from the subsystem as follows: /sys/mothwcfg/root.
 *
 *
 * <B>LOCKING</B><br>
 *   Locking is done by the kset services when a node is added to or removed
 *   from a kset or when searching a kset's members. In addition, the sysfs
 *   makes sure that the directory entries of each node and its children are
 *   consistent by utilizing a directory lock for each directory in sysfs.
 *   However, we have additional requirements, because we want to atomically
 *   check the validity of a node and perform actions on it. Therefore, a read/
 *   write lock is created for the purpose of synchronization.
 *
 *   The decision was made to use one global read/write lock for all accesses to
 *   any node in the tree. Each node could have had an individual lock, but the
 *   added space and overhead is not worth it since the read case is by far the
 *   most common and modifications will be done rarely.
 *
 *   Although the root node's subsystem contains a read/write semaphore, the
 *   decision was made to not use that for the services defined in this file
 *   because some of the kset services will be acquiring that lock as well. If
 *   the services in this file call kset services while the subsystem semaphore
 *   is already held, hangs would occur.
 *
 *   <B>LOCKING DURING NODE RELEASE</B><br>
 *       Locking the mothwcfg_rwsem is not necessary during release because the
 *       release methods are called by kobject services which, in turn, are
 *       called by mothwcfg services, which will already have the lock held.
 */

/* MOTHWCFG_PROP_NAME_LENGTH length of a valid property name. */
#define MOTHWCFG_PROP_NAME_LENGTH 32

/** Structure which defines each property in a node. */
struct mothwcfg_props {
    struct list_head entry; /**< Links this property to the next one for this node. */
    struct attribute attr; /**< Embedded kernel kobject attribute structure. */
    char name[MOTHWCFG_PROP_NAME_LENGTH]; /**< Property name. */
    unsigned int size; /**< Size of the property's data. */
    unsigned char data[]; /**< Beginning of property's data */
};

/** Structure which defines each node. */
struct mothwcfg_node {
    struct list_head proplist; /**< Beginning of the linked list of properties for this node. */
    struct kset kset; /**< Embedded kobject kset structure for this node. */
};

/* The next two structs are used to erase the recursion of old code
 *
 * struct node_sibling_struct is used to list all the nodes that position on the
 * same height of the tree.
 *
 * struct flattree_node_stemma is used to list all the node_sibling_struct.
 */
typedef struct node_sibling_struct {
	    struct mothwcfg_node *sibling;
	        struct node_sibling_struct *next;
} sibling_node_t;

typedef struct  flattree_node_stemma {
	    sibling_node_t *generation;
	        struct list_head list;
} node_stemma_t;

static void mothwcfg_release (struct kobject * kobj_p);
static ssize_t mothwcfg_prop_show (struct kobject * kobj_p,
                                   struct attribute * kattr_p, char * buffer);
struct sysfs_ops mothwcfg_sysfs_ops = {
    .show = mothwcfg_prop_show,
    .store = NULL,
};

struct kobj_type mothwcfg_ktype = {
    .release = mothwcfg_release,
    .default_attrs = NULL,
    .sysfs_ops = &mothwcfg_sysfs_ops,
};

static void mothwcfg_subsys_release (struct kobject * kobj_p);
static int mothwcfg_hotplug_filter(struct kset *kset, struct kobject *kobj);

static struct kobj_type mothwcfg_subsys_ktype = {
    .release = mothwcfg_subsys_release,
    .default_attrs = NULL,
    .sysfs_ops = &mothwcfg_sysfs_ops,
};

/** This is the structure that will be used to create the root node for the tree
 *  and be a subdirectory of /sys.
 */
struct mothwcfg_subsys {
    struct list_head proplist; /**< Beginning of the linked list of properties for this node. */
    struct subsystem subsys; /**< Embedded kobject subsystem structure for this node. */
};

struct kset_hotplug_ops mothwcfg_hotplug_ops = {
    .filter = mothwcfg_hotplug_filter,
    .name = NULL,
    .hotplug = NULL,
};

static struct mothwcfg_subsys mothwcfg_root = {
    .subsys = {
        .kset = {
            .kobj = { .name = "mothwcfg" },
            .ktype = &mothwcfg_subsys_ktype,
            .hotplug_ops = NULL,
        },
    },
};

static struct rw_semaphore mothwcfg_rwsem;

/*---------------------------- LOCAL CONSTANTS -------------------------------*/
#define DBGPRINTK(...)

/* HAS_BEEN_REMOVED marks a node that has been removed from the tree but still
 *     has references to it.
 */
#define HAS_BEEN_REMOVED ((struct subsystem *) 0x65656565)

/* NODENAMELEN is the max length of a node name: <unit-name>@<unit-instance>.
 *      The length of unit-name is 31 and the length of unit-instance is 8.
 */
#define NODENAMELEN 41

/*---------------------------- LOCAL MACROS ----------------------------------*/
#define locked_put_node(n) kset_put(&(n)->kset)

/*---------------------------- LOCAL PROTOTYPES ------------------------------*/
static struct mothwcfg_node * locked_get_first_child(struct mothwcfg_node * p);
static void locked_remove_node(struct mothwcfg_node * n);
static struct mothwcfg_node * locked_get_next_sibling(struct mothwcfg_node * n);

/*-------------------------- FUNCTION DEFINITIONS ----------------------------*/
static inline struct mothwcfg_props * locked_find_prop(struct mothwcfg_node * n,
                                                       const char * name)
{
    struct mothwcfg_props * prop_p;

    list_for_each_entry(prop_p, &n->proplist, entry)
    {
        if (!strncmp(prop_p->name, name, MOTHWCFG_PROP_NAME_LENGTH))
            return prop_p;
    }
    return NULL;
}

static inline struct mothwcfg_node * mothwcfg_get_node(struct mothwcfg_node * n)
{
    kset_get(&n->kset);
    return n;
}

/** Converts a pointer to the kobjet's entry field into a pointer to a node. A mothwcfg_node contains a kset that contains a kobject that contains a pointer to a list_head struct.
 * @param k A pointer to the "entry" field in a kobject structure.
 */
static inline struct mothwcfg_node * kentry2node(struct list_head *k)
{
    struct mothwcfg_node * n;
    struct kset * ks;
    struct kobject * ko;
    
    ko = container_of(k, struct kobject, entry);
    ks = container_of(ko, struct kset, kobj);
    n = container_of(ks, struct mothwcfg_node, kset);
    return n;
}

static void mothwcfg_subsys_release (struct kobject * kobj_p)
{
    /* Nothing to do since the root node structure is not kmalloc-ed. */
    DBGPRINTK("\n%s: / == 0x%08X, kset_p == 0x%08X\n",
           __FUNCTION__, (u32) &mothwcfg_root.subsys.kset, (u32) to_kset(kobj_p));
    return;
}

/** Called when the reference count goes to 0, this function removes all
 *    properties and frees all memory associated with the node and its
 *    properties.
 * @param kobj_p pointer to the kobject of the node being released
 * @note
 *  \li No locking is necessary since it will be called in the context of
 *      mothwcfg_put_node() which already holds the read lock. The write lock is
 *      not necessary because this code will only be called if the reference
 *      count goes to 0.
 *  \li This function assumes properties have been removed from sysfs since the
 *      release count for this node won't ever go to 0 unless it is removed from
 *      the sysfs tree.
 */
static void mothwcfg_release (struct kobject * kobj_p)
{
    struct mothwcfg_node * n =
                    container_of(to_kset(kobj_p), struct mothwcfg_node, kset);
    struct mothwcfg_props *prop, *tmpprop;

    DBGPRINTK("\n%s: / == 0x%08X, n == 0x%08X, n's name == \"%s\", n's refcount == %d\n\n",
           __FUNCTION__, (u32) &mothwcfg_root.subsys.kset, (u32) to_kset(kobj_p),
           (char *) kobj_p->name, kobj_p->kref.refcount.counter);

    n->kset.subsys = NULL;

    list_for_each_entry_safe(prop, tmpprop, &n->proplist, entry)
    {
        list_del(&prop->entry);
        kfree(prop);
    }

    kfree(n);
    return;
}

/** Called when the property is read from user space.
 * @param kobj_p pointer to the kobject of the node being read.
 * @param kattr_p pointer to the attribute structure of the property.
 * @param buffer pointer to buffer location to place properties data.
 */
static ssize_t mothwcfg_prop_show (struct kobject * kobj_p,
                                   struct attribute * kattr_p, char * buffer)
{
    struct mothwcfg_props * prop_p;
    u32 size;

    prop_p = (struct mothwcfg_props *)
                    container_of(kattr_p, struct mothwcfg_props, attr);
    DBGPRINTK("%s: Showing property %s of %s\n",
              __FUNCTION__, kobj_p->k_name, prop_p->attr.name);

    /* buffer is only PAGE_SIZE bytes long--don't overwrite it, but don't copy
     * more than is necessary, either.
     */
    if (prop_p->size <= PAGE_SIZE)
    {
        size = prop_p->size;
    }
    else
    {
        DBGPRINTK("%s: Data truncated for %s's property %s.\n",
               __FUNCTION__, kobj_p->k_name, prop_p->attr.name);
        size = PAGE_SIZE;
    }

    memcpy(buffer, prop_p->data, size);
    return size;
}

static int mothwcfg_hotplug_filter(struct kset *kset, struct kobject *kobj)
{
    DBGPRINTK("\n%s: No hotplug for %s\n", __FUNCTION__, kobject_name(kobj));
    return 0;
}

/** Initializes the Motorola hardware config tree.
 *    This function creates the root node of the system hardware configuration
 *    tree, which is /sys/mothwcfg and is a subsystem which contains all of the
 *    remaining nodes in the tree. Special processing is done as part of this
 *    function to create the root node and set up its properties. After that,
 *    this function calls flattree_add_tree() to add each child node and its
 *    children.
 * @return On success: 0. On failure: a negative error value.
 * @note
 *  \li If this function is successful, the tree will be created and populated.
 */
int __init mothwcfg_init(void)
{
    char * flattree_input = NULL;
    struct flattree_context context;
    int retval = 0;
    char * nodename;
    MOTHWCFGNODE *n;

    init_rwsem(&mothwcfg_rwsem);

    /* "show" methods for properties are obtained from either the
     * containing kset or from the kobject if none exist for the kset. In
     * case of the root subsystem kobject, there is no containing kset, so
     * initialize the subsystem's kobject.
     */
    mothwcfg_root.subsys.kset.kobj.ktype = &mothwcfg_subsys_ktype;

    /* This creates the directory /sys/mothwcfg, which is the root of the
     * hardware configuration tree. The root node is a subsystem, but it is
     * also a kset.
     */
    retval = subsystem_register(&mothwcfg_root.subsys);
    if (retval < 0)
    {
        printk(KERN_WARNING
               "%s: Failed to register root node: name == %s: retval == %d\n",
               __FUNCTION__, "/sys/mothwcfg", retval);
        goto cleanup;
    }

    INIT_LIST_HEAD(&mothwcfg_root.proplist);

    flattree_input = (char *) mot_flat_dev_tree_address();
    DBGPRINTK("\n%s: flattree_input == 0x%08X, *flattree_input == 0x%08X\n",
                __FUNCTION__, (u32) flattree_input,
                (flattree_input)?*((u32 *) flattree_input):flattree_input);

    if (!flattree_input)
    {
        printk(KERN_WARNING "%s: No address for system configuration input stream\n",
                __FUNCTION__);
        return -EINVAL;
    }

    if (offsetof(struct subsystem, kset) != 0)
    {
        printk(KERN_ERR "%s: Internal structure dependency failure: "
                        "cannot create hardware config tree.\n", __FUNCTION__);
        retval = -EFAULT;
        goto cleanup;
    }

    retval = flattree_check_header(&context, flattree_input);
    if (retval != 0)
    {
        printk(KERN_WARNING "%s: Serialized tree header is invalid\n",
               __FUNCTION__);
        goto cleanup;
    }

    /* Get the root node from the input stream.
     * The node name for root should be NULL.
     */
    nodename = flattree_read_node(&context);
    if (IS_ERR(nodename))
    {
        printk(KERN_WARNING "%s: Couldn't read root node name from mothwcfg input.\n",
               __FUNCTION__);
        retval = PTR_ERR(nodename);
        goto cleanup;
    }

    if (nodename && *nodename != '\0')
    {
        printk(KERN_WARNING "%s: Root node must not have a name: %s\n", __FUNCTION__, nodename);
        retval =  -EINVAL;
        goto cleanup;
    }

    /* Now that the root node has been created, get a reference to it so that
     * properties and children nodes can be added to it.
     */
    n = mothwcfg_get_node( (MOTHWCFGNODE*) &mothwcfg_root);

    retval = flattree_add_all_props(&context,(MOTHWCFGNODE*)&mothwcfg_root);
    if (retval < 0)
    {
        mothwcfg_put_node(n);
        printk(KERN_WARNING
               "%s: Failed to add properties for root node: "
               "name == %s: retval == %d\n",
                __FUNCTION__, "/sys/mothwcfg", retval);
        goto cleanup;
    }

    retval = flattree_add_tree(&context, n);
    mothwcfg_put_node(n);
    if (retval < 0)
    {
        printk(KERN_WARNING "%s: Failed adding nodes and properties %d\n",
               __FUNCTION__, retval);
        goto cleanup;
    }

    /* Should be at end of input stream. */
    retval = flattree_check_footer(&context);
    if (retval < 0)
    {
        printk(KERN_WARNING "%s: Serialized input file has bad footer: %d\n",
               __FUNCTION__, retval);
        goto cleanup;
    }

cleanup:
    mot_free_flat_dev_tree_mem((unsigned long) flattree_input);
    return retval;
}

/** This function creates a new node for the specified nodename, adds it to
 *     the tree under the specified parent node, and does an explicit "get" on
 *     the new node.
 * @param parent Node handle of the parent of the new node. A "get" must have been
 *            done on parent prior to the call to mothwcfg_add_node().
 * @param nodename NULL-terminated string that is the name to be assigned to the
 *              new node.
 * @return On success: Returns the node handle of the new node.<br>
 *         On failure: a negative error value.
 *                     IS_PTR and PTR_ERR must be used to check for and handle
 *                     errors from this function.
 * @note
 *  \li It is the responsibility of the caller to do a put on the returned node.
 */
MOTHWCFGNODE *mothwcfg_add_node(MOTHWCFGNODE *parent, const char * nodename)
{
    struct mothwcfg_node * n;
    struct mothwcfg_node * p = parent;
    int retval = 0;

    if (parent == NULL || nodename == NULL || *nodename == '\0')
        retval = -EINVAL;

    /* Allocate new node outside of critical section. */
    if (NULL == (n = kmalloc(sizeof(struct mothwcfg_node), GFP_KERNEL)))
    {
        printk(KERN_WARNING "%s: couldn't kmalloc mothwcfg_node for %s\n",
               __FUNCTION__, nodename);
        return ERR_PTR(-ENOMEM);
    }
    memset((void *) n, 0, sizeof(struct mothwcfg_node));

    /* kobject_set_name() copies the name from "nodename", so there is
     * no need to allocate space for the name.
     */
    if ((retval = kobject_set_name(&(n->kset.kobj), nodename)) < 0)
    {
        printk(KERN_WARNING "%s: failed to set kobj name %s: retval == %d\n",
            __FUNCTION__, nodename, retval);
        kfree(n);
        return ERR_PTR(retval);
    }

    INIT_LIST_HEAD(&n->proplist);
    n->kset.ktype = &mothwcfg_ktype;
    n->kset.subsys = &mothwcfg_root.subsys;

    down_write(&mothwcfg_rwsem);

    if (p->kset.subsys == HAS_BEEN_REMOVED)
        retval = -EIDRM;
    else if (p->kset.subsys != &mothwcfg_root.subsys)
        retval = -EINVAL;
    if (retval < 0)
    {
        up_write(&mothwcfg_rwsem);
        kfree(n);
        return ERR_PTR(retval);
    }

    /* The "kset" field of "n" is a kset which has a kobject, and that
     * kobject can belong to a separate kset. n's kobject can also be a
     * child of another kobject which is embedded in a kset structure. In
     * this case, the parent of n and the kset to which n belongs is the
     * same thing. So, to make n belong to (and be a child of) the parent
     * kset, point n's kobject's kset field to the parent kset.
     */
    n->kset.kobj.kset = &p->kset;

    if ((retval = kset_register(&n->kset)) < 0)
    {
        up_write(&mothwcfg_rwsem);
        printk(KERN_WARNING "%s: %s register failed: %d\n",
               __FUNCTION__, nodename, retval);
        kfree(n);
        return ERR_PTR(retval);
    }

    /* Do a get on the node so that the caller can do other operations without
     * the node disappearing out from under them.
     */
    n = mothwcfg_get_node(n);

    up_write(&mothwcfg_rwsem);

    return n;
}
EXPORT_SYMBOL(mothwcfg_add_node);

/** Find the first child node of the given node, increment its reference
 *     count, and return it.
 * @param parent handle of the node for which the first child is requested. This
 *             function assumes that the caller of this function holds a
 *             reference to parent.
 * @return
 *     On success: node handle of first child of parent<br>
 *     On failure:  NULL
 * @note
 *  \li It is the responsibility of the caller to do a put on the returned node.
 */
MOTHWCFGNODE *mothwcfg_get_first_child(MOTHWCFGNODE *parent)
{
    struct mothwcfg_node * p = parent;
    struct mothwcfg_node * n = NULL;

    down_read(&mothwcfg_rwsem);

    if ( (parent != NULL) && (p->kset.subsys == &mothwcfg_root.subsys) )
        n = locked_get_first_child(p);

    up_read(&mothwcfg_rwsem);
    return n;
}
EXPORT_SYMBOL(mothwcfg_get_first_child);

/** This function exists to provide the get first child functionality
 *      within a critical section.
 *
 *      Each kset keeps a list of the kobjects in its kset, which are the
 *      children of the node in this implementation. However, there is not
 *      a kset interface that will just return the first child.
 *
 *      Once we know that parent has children, convert the first child pointer
 *      into a mothwcfg_node structure pointer, get a reference to the first
 *      child node, and return it.
 * @param p Pointer to parent node for which to access first child.
 * @return On success: first child of p<br>
 *         On failure: NULL
 * @note
 *   \li This function must be called with lock held on mothwcfg_rwsem.
 *   \li This function assumes p has been validated.
 */
static struct mothwcfg_node * locked_get_first_child(struct mothwcfg_node *p)
{
    struct mothwcfg_node * n = NULL;

    if (!list_empty(&p->kset.list))
        n = mothwcfg_get_node(kentry2node(p->kset.list.next));

    return n;
}

/** Find the next sibling of the given node handle, increment its reference
 *     count, and return it.
 * @param nh handle of the node for which the next sibling is requested
 * @return
 *     On success: node handle of next sibling<br>
 *     On failure:  NULL
 * @note
 *  \li   It is the responsibility of the caller to do a put on the returned node.
 */
MOTHWCFGNODE *mothwcfg_get_next_sibling(MOTHWCFGNODE *nh)
{
    struct mothwcfg_node * n = nh;
    struct mothwcfg_node * next = NULL;
    struct kset * p;

    down_read(&mothwcfg_rwsem);

    p = to_kset(n->kset.kobj.parent);

    if ( (nh != NULL)
         && (n->kset.subsys == &mothwcfg_root.subsys)
         && (p != NULL)
         && (p->subsys == &mothwcfg_root.subsys)   )
            next = locked_get_next_sibling(n);

    up_read(&mothwcfg_rwsem);
    return next;
}
EXPORT_SYMBOL(mothwcfg_get_next_sibling);

/** This function exists to provide the get next sibling functionality
 *      within a critical section.
 *
 *      If n has no siblings, return NULL. Otherwise, convert the sibling to a
 *      node handle, get a reference to it, and return the handle.
 *
 * @param n node for which the next sibling is requested
 *
 * @return
 *     On success: node handle of next sibling<br>
 *     On failure:  NULL
 *
 * @note
 *   \li This function must be called with lock held on mothwcfg_rwsem.
 *   \li This function assumes n and n's parent have been validated.
 */
static struct mothwcfg_node * locked_get_next_sibling(struct mothwcfg_node * n)
{
    struct kset * p;
    struct mothwcfg_node * sib = NULL;

    p = to_kset(n->kset.kobj.parent);

    /* The next thing in the sibling list is either another node or we are back
     * at the beginning of the list.
     */
    if (n->kset.kobj.entry.next != &p->list)
        sib = mothwcfg_get_node(kentry2node(n->kset.kobj.entry.next));

    return sib;
}

/** Follow the specified path to find the node, increment the nodes reference
 *     count, and return the node handle.
 * @param name Null-terminated string of the full path of the requested node
 * @return
 *     On success: The MOTHWCFGNODE handle of the requested node<br>
 *     On failure:  NULL
 * @note
 * \li     It is the responsibility of the caller to do a put on the returned node.
 */
MOTHWCFGNODE *mothwcfg_get_node_by_path(const char * name)
{
    struct kset * parent_ks_p = &mothwcfg_root.subsys.kset;
    struct mothwcfg_node * n = NULL;
    char sname[NODENAMELEN], *s;
    const char *cur;
    struct kobject * ko;

    DBGPRINTK("\n\nrequested name == \"%s\"\n\n",name);

    /* String must begin with a /. */
    if (name == NULL || *name != '/')
        return NULL;

    /* Handle special case if they are asking for the root node. */
    if (!strcmp(name, "/"))
    {
        n = mothwcfg_get_node((MOTHWCFGNODE *)&mothwcfg_root);
        return n;
    }

    down_read(&mothwcfg_rwsem);

    /* A search of the children of each node in the path must be done in order
     * to traverse the tree.
     */
    cur = name;
    while (1)
    {
        sname[0] = '\0';
        s = sname;
        cur++; /* skip past '/' */
        while (*cur && *cur != '/' && s - sname < NODENAMELEN)
            *s++ = *cur++;

        /* Check for invalid node name. */
        if (sname[0] == '\0' || s - sname >= NODENAMELEN)
            break;

        *s = '\0';

        /* At this point, cur and prev are both pointing to the next element
         * in the path.
         */

        /* Search using kset_find_obj(), which does a "get" on the node if it
         * finds it.
         */
        ko = kset_find_obj(parent_ks_p, sname);

        /* If no kobject, this element in the path was not found. */
        if (!ko)
            break;

        /* If this is the one we were looking for, convert it and return. */
        if (*cur == '\0')
        {
            n = container_of(to_kset(ko), struct mothwcfg_node, kset);
            break;
        }

        /* Otherwise, put this one back and keep going with the next element. */
        parent_ks_p = to_kset(ko);
        /* ko is now the parent. Go ahead and do a put on it here even though it
         * will be referenced the next time through the loop. This is safe since
         * the read lock is held at this point.
         */
        kobject_put(ko);
    }

    up_read(&mothwcfg_rwsem);

    return n;
}
EXPORT_SYMBOL(mothwcfg_get_node_by_path);

/** Releases a reference to a hardware configuration tree node.
 * @param nh       Node handle of the node whose reference count is to be
 *                 decremented. A "get" must have been done on node prior to
 *                 calling this function.
 * @note
 * \li     A read lock is used here because if the reference count goes to 0 as a
 *     result of this call, the mothwcfg_release() method will be called in
 *     this context.
 */
void mothwcfg_put_node (MOTHWCFGNODE *nh)
{
    struct mothwcfg_node * n = nh;

    down_read(&mothwcfg_rwsem);

    if ( (nh != NULL) &&
         ( (n->kset.subsys == HAS_BEEN_REMOVED) ||
           (n->kset.subsys == &mothwcfg_root.subsys) ) )
        locked_put_node(n);

    up_read(&mothwcfg_rwsem);
    return;
}
EXPORT_SYMBOL(mothwcfg_put_node);

/** This function unlinks nh and all of its children from the hardware
 *     configuration tree. It also unlinks nh and its children from each other
 *     so that there is no parent/child relationship any more in the subtree
 *     beginning with nh.
 * @param    nh handle of node to be removed
 * @return
 *     On success: 0<br>
 *     On failure:  a negative error value
 * @note
 * \li     This function does not do a "put" on the node.
 */
int mothwcfg_remove_node(MOTHWCFGNODE *nh)
{
    struct mothwcfg_node * n = nh;
    u32 retval = 0;

    down_write(&mothwcfg_rwsem);

    if (nh == NULL)
        retval = -EINVAL;
    else if (n->kset.subsys == HAS_BEEN_REMOVED)
        retval = -EIDRM;
    else if (n->kset.subsys != &mothwcfg_root.subsys)
        retval = -EINVAL;
    else /* Parameter is valid. Call internal function with locks held. */
        locked_remove_node(n);

    up_write(&mothwcfg_rwsem);

    return retval;
}
EXPORT_SYMBOL(mothwcfg_remove_node);
     

/** This function exists to provide the remove functionality within a
 *      critical section.
 * @param    n node to be removed
 * @note
 *  \li This function must be called with write lock held on mothwcfg_rwsem.
 *  \li This function assumes n has been validated.
 *  \li This function assumes all children nodes are valid.
 *  \li For each child node, we use iteration method to list them by using 
 *      two struct struct node_sibling_struct and struct flattree_node_stemma 
 */

static void locked_remove_node(struct mothwcfg_node * n)
{
    MOTHWCFGNODE *child1, *next;
    struct mothwcfg_props *prop;
    node_stemma_t *prev_stemma;

    LIST_HEAD(top_of_stemma);

    sibling_node_t *sibling_tmp1 = NULL, *sibling_tmp2 = NULL;
    sibling_node_t *sibling_node = kmalloc(sizeof(sibling_node_t), GFP_KERNEL);
    if (! sibling_node)
    {
        DBGPRINTK(KERN_WARNING "%s: Failed to alloc memory for *sibling_node\n",
		__FUNCTION__);
        goto error;
    }
    sibling_node->sibling = n;
    sibling_node->next = NULL;

    node_stemma_t *node_stemma = kmalloc(sizeof(node_stemma_t), GFP_KERNEL);
    if (!node_stemma)
    {
        DBGPRINTK(KERN_WARNING "%s: Failed to alloc memory for *node_stemma\n",
		__FUNCTION__);
        goto error;
    }

    node_stemma->generation = sibling_node;
    sibling_node = NULL;                    // we will use this variable later
    INIT_LIST_HEAD(& node_stemma->list);   

    /* make this one to be the first member of the generation list */
    list_add_tail(&node_stemma->list, &top_of_stemma); 


    DBGPRINTK("%s: Entering \"%s\", refcount == %d\n",
        __FUNCTION__, n->kset.kobj.k_name, n->kset.kobj.kref.refcount.counter);

    child1 = locked_get_first_child(n);
    DBGPRINTK("----%s: first childnode of \"%s\" == 0x%08X\n",
            __FUNCTION__, n->kset.kobj.k_name, (u32) child1);


    /* Collect the node of the same height of the tree into one list, and then list all these lists */
next_generation:
    prev_stemma = list_entry(top_of_stemma.prev, struct flattree_node_stemma, list);
    sibling_node_t *prev_sibling_node = prev_stemma->generation;

    for(;prev_sibling_node != NULL; prev_sibling_node = prev_sibling_node->next)
    {
        struct mothwcfg_node *tmp_sibling = prev_sibling_node->sibling;
        child1 = locked_get_first_child(tmp_sibling);

        if (child1)
        {
            sibling_tmp2 = kmalloc(sizeof(sibling_node_t), GFP_KERNEL);
            if (!sibling_tmp2)
            {
                DBGPRINTK(KERN_WARNING "%s: Failed to alloc memory for sibling_tmp2\n",
			__FUNCTION__);
                goto error;
            }

            sibling_tmp2->sibling = child1;
            sibling_tmp2->next = NULL;
            if ( sibling_tmp1 )
                sibling_tmp1->next = sibling_tmp2;
            sibling_tmp1 = sibling_tmp2;
            /* make sibling_node to be the first member of the nodes that stay 
             *                on the same height of the tree*/
            if ( !sibling_node ) 
                sibling_node = sibling_tmp2;
            next = locked_get_next_sibling(child1);
            while (next)
            {
                sibling_tmp2 = kmalloc(sizeof(sibling_node_t), GFP_KERNEL);
                if (!sibling_tmp2)
                {
                    DBGPRINTK(KERN_WARNING "%s: Failed to alloc memory for sibling_tmp2\n",
				__FUNCTION__);
                    goto error;
                }
                sibling_tmp2->sibling = next;
                sibling_tmp2->next = NULL;
                sibling_tmp1->next = sibling_tmp2;
                sibling_tmp1 = sibling_tmp2;
                next = locked_get_next_sibling(next);
            }
        }

    }

    if ( sibling_node )
    {
        node_stemma = kmalloc(sizeof(node_stemma_t), GFP_KERNEL);
        if (!node_stemma)
        {
            DBGPRINTK(KERN_WARNING "%s: Failed to alloc memory for node_stemma\n",
			__FUNCTION__);
            goto error;
        }
        node_stemma->generation = sibling_node;
        INIT_LIST_HEAD(&node_stemma->list);
        list_add_tail(&node_stemma->list, &top_of_stemma);

        sibling_node = sibling_tmp1 = sibling_tmp2 = NULL; 
        goto next_generation;
    } 
    /* sibling_node == NULL means that we have come to the heightest node, so we can do 
     *        the real work now. */

    node_stemma_t *node_stemma_next;
    list_for_each_entry_reverse(node_stemma, &top_of_stemma, list)
    {
	sibling_node = node_stemma->generation;
	while (sibling_node)
	{
            list_for_each_entry(prop, &sibling_node->sibling->proplist, entry)
            {
        	sysfs_remove_file(&sibling_node->sibling->kset.kobj, &prop->attr);
            }

            DBGPRINTK("%s: Unregistering node \"%s\"\n",
		__FUNCTION__,
		sibling_node->sibling->kset.kobj.k_name);

            kset_unregister(&sibling_node->sibling->kset);

            /* The subsys field is used to indicate that this node has been removed from
             * the sysfs tree.
             */
            sibling_node->sibling->kset.subsys = HAS_BEEN_REMOVED;

            DBGPRINTK("----%s: After unregistering \"%s\", refcount == %d\n",
		__FUNCTION__,
		sibling_node->sibling->kset.kobj.k_name,
		sibling_node->sibling->kset.kobj.kref.refcount.counter);
	   
	    sibling_tmp1 = sibling_node->next;
	    locked_put_node(sibling_node->sibling);
	    sibling_node = sibling_tmp1;
	}
    }

error:
    list_for_each_entry_safe(node_stemma, node_stemma_next, &top_of_stemma, list)
    {
	sibling_node = node_stemma->generation;
        while (sibling_node)
        {
            sibling_tmp1 = sibling_node->next;
            kfree(sibling_node);
            sibling_node = sibling_tmp1;
        }
        kfree(node_stemma);
    }

    return;
}

/** associates the properties with the given node.
 * @param  nh   handle of a hardware configuration node -- the new property will
 *         be associated with this node. A get must have been done on nh prior
 *         to calling this function.
 * @param   name name of property
 * @param   data data contained in property
 * @param   size size of data
 * @return
 *   On success: 0<br>
 *   On failure: a negative error value
 */
int mothwcfg_add_prop(MOTHWCFGNODE *nh, const char *name, const void *data, u32 size)
{
    struct mothwcfg_node * n = nh;
    struct mothwcfg_props * prop_p;
    int retval = 0;

    if ((nh == NULL) || (size > PAGE_SIZE)
        || (name == NULL) || (strlen(name) >= MOTHWCFG_PROP_NAME_LENGTH) )
        return -EINVAL;

    /* Allocate space for the mothwcfg property structure, including enough
     * space to contain the property's data. This space will be freed if/when
     * the node is removed.
     */
    prop_p = kmalloc(sizeof(struct mothwcfg_props) + size, GFP_KERNEL);
    if (NULL == prop_p)
    {
        printk(KERN_WARNING
               "%s: alloc of size %d for %s property struct failed\n",
               __FUNCTION__, sizeof(struct mothwcfg_props) + size, name);
        return -ENOMEM;
    }

    memset((void *)prop_p, 0, sizeof(struct mothwcfg_props) + size);

    /* Part of the mothwcfg_props structure is a character array of 32 bytes.
     * This property's name is copied to the array and a pointer to this array
     * is assigned to the attr structure's name pointer. The attr structure
     * within the mothwcfg_props structure is used to create the sysfs property
     * file for this node.
     */
    strncpy(prop_p->name, name, MOTHWCFG_PROP_NAME_LENGTH-1);
    prop_p->attr.mode = S_IRUGO;
    prop_p->size = size;
    prop_p->attr.name = prop_p->name;
    INIT_LIST_HEAD(&prop_p->entry);

    memcpy(prop_p->data, data, size);

    down_write(&mothwcfg_rwsem);

    /* Must check for existing property since sysfs_create_file() doesn't. */
    if (locked_find_prop(n, name))
        retval = -EEXIST;
    else if (n->kset.subsys == &mothwcfg_root.subsys)
    {
        retval = sysfs_create_file(&n->kset.kobj, &prop_p->attr);
        if (retval == 0)
            list_add_tail(&prop_p->entry, &n->proplist);
    }
    else
    {
        if (n->kset.subsys == HAS_BEEN_REMOVED)
            retval = -EIDRM;
        else
            retval = -EINVAL;
    }

    up_write(&mothwcfg_rwsem);

    if (retval < 0)
    {
        kfree(prop_p);
        printk(KERN_WARNING "%s: Failed to create property %s in node %s.\n",
               __FUNCTION__, name, kobject_name(&n->kset.kobj));
    }
    return retval;
}
EXPORT_SYMBOL(mothwcfg_add_prop);

/** This function copies the data from the specified property into the
 *     buffer. By passing NULL for the buf parameter, this function may be
 *     used to determine the size of propname's data.
 * @param      nh         handle of the node that holds the property -- a get must
 *                        been done on this handle prior to calling this function
 * @param  propname   name of the property the caller wants the data from
 * @param  size       size of buf.
 * @param buf        a pointer to memory that will hold the data -- may be NULL
 * @return
 *     On success: size in bytes of propname's data<br>
 *     On failure: a negative error value
 * @note
 * \li If buf is NULL, size is ignored.
 * \li If the specified size is less than the size returned, the data is
 *     truncated.
 */
int mothwcfg_read_prop(MOTHWCFGNODE *nh, const char * propname, char * buf, u32 size)
{
    struct mothwcfg_node * n = nh;
    struct mothwcfg_props * prop_p;

    down_read(&mothwcfg_rwsem);

    if ( (nh == NULL) ||
         ( (n->kset.subsys != HAS_BEEN_REMOVED) &&
           (n->kset.subsys != &mothwcfg_root.subsys) ) )
    {
        up_read(&mothwcfg_rwsem);
        return -EINVAL;
    }

    prop_p = locked_find_prop(n, propname);

    if (prop_p == NULL)
    {
        up_read(&mothwcfg_rwsem);
        return -ENOENT;
    }

    if (buf == NULL)
    {
        up_read(&mothwcfg_rwsem);
        return (int) prop_p->size;
    }

    /* If size of buf is greater than size of the data, use the size associated
     * with the data so we don't copy whatever is beyond the end of prop_p->data
     * into buf.
     */
    if (size > prop_p->size)
        size = prop_p->size;
    memcpy(buf, prop_p->data, size);
    up_read(&mothwcfg_rwsem);

    return (int) prop_p->size;
}
EXPORT_SYMBOL(mothwcfg_read_prop);

/** This function removes the specified property and frees all associated
 *     memory.
 * @param     nh         handle of the node that holds the property -- a get must
 *                       have been done on this node prior to calling this function
 * @param     propname   name of the property the caller wants the data from
 * @return
 *     On success: 0<br>
 *     On failure: a negative error value
 */
int mothwcfg_remove_prop(MOTHWCFGNODE *nh, const char * propname)
{
    struct mothwcfg_node * n = nh;
    struct mothwcfg_props * prop_p;
    int retval = 0;

    down_write(&mothwcfg_rwsem);

    if (nh == NULL)
        retval = -EINVAL;
    else if (n->kset.subsys == HAS_BEEN_REMOVED)
        retval = -EIDRM;
    else if (n->kset.subsys != &mothwcfg_root.subsys)
        retval = -EINVAL;

    if (retval < 0)
    {
        up_write(&mothwcfg_rwsem);
        return retval;
    }

    prop_p = locked_find_prop(n, propname);

    if (prop_p == NULL)
    {
        up_write(&mothwcfg_rwsem);
        return -ENOENT;
    }

    DBGPRINTK("%s: Removing property \"%s\" of node \"%s\"\n",
              __FUNCTION__, prop_p->attr.name, n->kset.kobj.k_name);

    sysfs_remove_file(&n->kset.kobj, &prop_p->attr);
    list_del(&prop_p->entry);
    up_write(&mothwcfg_rwsem);

    kfree(prop_p);

    DBGPRINTK("----%s: After removal of \"%s\" for \"%s\", refcount == %d\n",
              __FUNCTION__, prop_p->attr.name, n->kset.kobj.k_name,
              n->kset.kobj.kref.refcount.counter);

    return 0;
}
EXPORT_SYMBOL(mothwcfg_remove_prop);
