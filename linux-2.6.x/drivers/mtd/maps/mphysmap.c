/*
 * $Id$
 *
 * Several mappings of NOR chips
 *
 * Copyright (c) 2001-2005	Jörn Engel <joern@wh.fh-wedelde>
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/map.h>
#include <linux/mtd/mtd.h>


#define NO_DEVICES 8
struct map_info maps[NO_DEVICES] = {
#if CONFIG_MTD_MULTI_PHYSMAP_1_WIDTH
	{
		.name		= CONFIG_MTD_MULTI_PHYSMAP_1_NAME,
		.phys		= CONFIG_MTD_MULTI_PHYSMAP_1_START,
		.size		= CONFIG_MTD_MULTI_PHYSMAP_1_LEN,
		.bankwidth	= CONFIG_MTD_MULTI_PHYSMAP_1_WIDTH,
	},
#endif
#if CONFIG_MTD_MULTI_PHYSMAP_2_WIDTH
	{
		.name		= CONFIG_MTD_MULTI_PHYSMAP_2_NAME,
		.phys		= CONFIG_MTD_MULTI_PHYSMAP_2_START,
		.size		= CONFIG_MTD_MULTI_PHYSMAP_2_LEN,
		.bankwidth	= CONFIG_MTD_MULTI_PHYSMAP_2_WIDTH,
	},
#endif
#if CONFIG_MTD_MULTI_PHYSMAP_3_WIDTH
	{
		.name		= CONFIG_MTD_MULTI_PHYSMAP_3_NAME,
		.phys		= CONFIG_MTD_MULTI_PHYSMAP_3_START,
		.size		= CONFIG_MTD_MULTI_PHYSMAP_3_LEN,
		.bankwidth	= CONFIG_MTD_MULTI_PHYSMAP_3_WIDTH,
	},
#endif
#if CONFIG_MTD_MULTI_PHYSMAP_4_WIDTH
	{
		.name		= CONFIG_MTD_MULTI_PHYSMAP_4_NAME,
		.phys		= CONFIG_MTD_MULTI_PHYSMAP_4_START,
		.size		= CONFIG_MTD_MULTI_PHYSMAP_4_LEN,
		.bankwidth	= CONFIG_MTD_MULTI_PHYSMAP_4_WIDTH,
	},
#endif
	{
		.name = NULL,
	},
};
DECLARE_MUTEX(map_mutex);


static int map_one(struct map_info *map)
{
	struct mtd_info *mtd;

	map->virt = ioremap(map->phys, map->size);
	if (!map->virt)
		return -EIO;

	simple_map_init(map);

	mtd = do_map_probe("cfi_probe", map);
	if (!mtd) {
		iounmap(map->virt);
		return -ENXIO;
	}

	map->map_priv_1 = (unsigned long)mtd;
	mtd->owner = THIS_MODULE;
	/* TODO: partitioning */
	return add_mtd_device(mtd);
}


static void unmap_one(struct map_info *map)
{
	struct mtd_info *mtd = (struct mtd_info*)map->map_priv_1;

	if (map->map_priv_2)
		kfree(map->name);

	if (!map->virt)
		return;

	BUG_ON(!mtd);
	BUG_ON(map->map_priv_2 > 1);

	del_mtd_device(mtd);
	map_destroy(mtd);
	iounmap(map->virt);

	map->map_priv_1 = 0;
	map->map_priv_2 = 0;
	map->virt = NULL;
}


static struct map_info *next_free_map(void)
{
	int i;
	for (i=0; i<NO_DEVICES; i++) {
		struct map_info *map = &maps[i];
		if (!map->virt)
			return map;
	}
	return NULL;
}


static int add_one_map(const char *name, unsigned long start,
		unsigned long len, int width)
{
	struct map_info *map = next_free_map();
	if (!map)
		return -ENOSPC;
	
	map->name = kmalloc(strlen(name)+1, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	strcpy(map->name, name);
	map->phys = start;
	map->size = len;
	map->bankwidth = width;
	map->map_priv_2 = 1; /* marker to free map->name */

	return map_one(map);
}


static int parse_ulong(unsigned long *num, const char *token)
{
	char *endp;
	unsigned long n;

	n = ustrtoul(token, &endp, 0);
	if (*endp)
		return -EINVAL;

	*num = n;
	return 0;
}


static int parse_uint(unsigned int *num, const char *token)
{
	char *endp;
	unsigned long n;

	n = ustrtoul(token, &endp, 0);
	if (*endp)
		return -EINVAL;
	if ((int)n != n)
		return -EINVAL;

	*num = n;
	return 0;
}


static int parse_name(char **pname, const char *token, int limit)
{
	size_t len;
	char *name;

	len = strlen(token) + 1;
	if (len > limit)
		return -ENOSPC;

	name = kmalloc(len, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	memcpy(name, token, len);

	*pname = name;
	return 0;
}


static inline void kill_final_newline(char *str)
{
	char *newline = strrchr(str, '\n');
	if (newline && !newline[1])
		*newline = 0;
}


#define parse_err(fmt, args...) do {	\
	printk(KERN_ERR fmt "\n", ## args);	\
	return 0;			\
} while(0)

/* mphysmap=name,start,len,width */
static int mphysmap_setup(const char *val, struct kernel_param *kp)
{
	char buf[64+14+14+14], *str = buf;
	char *token[4];
	char *name;
	unsigned long start;
	unsigned long len;
	unsigned int width;
	int i, ret;

	if (strnlen(val, sizeof(buf)) >= sizeof(buf))
		parse_err("parameter too long");

	strcpy(str, val);
	kill_final_newline(str);

	for (i=0; i<4; i++)
		token[i] = strsep(&str, ",");

	if (str)
		parse_err("too many arguments");
	if (!token[3])
		parse_err("not enough arguments");

	ret = parse_name(&name, token[0], 64);
	if (ret == -ENOMEM)
		parse_err("out of memory");
	if (ret == -ENOSPC)
		parse_err("name too long");
	if (ret)
		parse_err("illegal name: %d", ret);

	ret = parse_ulong(&start, token[1]);
	if (ret)
		parse_err("illegal start address");

	ret = parse_ulong(&len, token[2]);
	if (ret)
		parse_err("illegal length");

	ret = parse_uint(&width, token[3]);
	if (ret)
		parse_err("illegal bus width");

	down(&map_mutex);
	ret = add_one_map(name, start, len, width);
	up(&map_mutex);
	if (ret == -ENOSPC)
		parse_err("no free space for new map");
	if (ret)
		parse_err("error while mapping: %d", ret);

	return 0;
}


static int __init mphysmap_init(void)
{
	int i;
	down(&map_mutex);
	for (i=0; i<NO_DEVICES; i++)
		map_one(&maps[i]);
	up(&map_mutex);
	return 0;
}


static void __exit mphysmap_exit(void)
{
	int i;
	down(&map_mutex);
	for (i=0; i<NO_DEVICES; i++)
		unmap_one(&maps[i]);
	up(&map_mutex);
}


__module_param_call("", mphysmap, mphysmap_setup, NULL, NULL, 0600);

module_init(mphysmap_init);
module_exit(mphysmap_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jörn Engel <joern@wh.fh-wedelde>");
MODULE_DESCRIPTION("Generic configurable extensible MTD map driver");
