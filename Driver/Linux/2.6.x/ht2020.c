/**
 * HT2020 driver.
 *
 * Copyright (C) 2006 Atmark Techno, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#if !defined(CONFIG_MACH_ARMADILLO460)
#include <linux/config.h>
#include <asm/arch/platform.h>
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#define DRIVER_NAME	"ht2020"
#define DRIVER_AUTHOR	"Atmark Techno, Inc."
#define DRIVER_VERSION 	"v2.0"
#define DRIVER_DESC 	"HT2020 driver for Armadillo-460"
#define DRIVER_INFO	DRIVER_NAME ": " DRIVER_DESC ", " DRIVER_VERSION \
			", (C) 2001-2007 " DRIVER_AUTHOR

#define HT2020_PORTS_MAX (16)

#define HT2020_MAJOR		(222)

#define HT2020_MINOR_PER_PORT	(4) /* one minor for each channel */
#define HT2020_MINOR_NR		(HT2020_PORTS_MAX * HT2020_MINOR_PER_PORT)

#if defined(CONFIG_MACH_ARMADILLO460)
#define ISAIO8_BASE    0xf2000000
#define ISAIO16_BASE   0xf4000000
#endif

static int io[HT2020_PORTS_MAX];
static int io_count;
module_param_array(io, int, &io_count, S_IRUGO);
MODULE_PARM_DESC(io, "I/O base address(es), required for loading modules");

static struct cdev ht2020_cdev;

/* selectable HT2020 I/O port addresses */
static unsigned int ht2020_portlist[HT2020_PORTS_MAX] = {
	0x100, 0x104, 0x108, 0x10c, 0x110, 0x114, 0x118, 0x11c,
	0x120, 0x124, 0x128, 0x12c, 0x130, 0x134, 0x138, 0x13c,
};

static int ht2020_foundlist[HT2020_PORTS_MAX] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
};

#define portaddr_io8(port)	ht2020_portlist[port] + ISAIO8_BASE
#define portaddr_io16(port)	ht2020_portlist[port] + ISAIO16_BASE

#define ht_inb(addr)		__raw_readb(__io(addr))
#define ht_outb(buf, addr)	__raw_writeb(buf, __io(addr))

static ssize_t ht2020_read(struct file *file, char *buf, size_t count,
		loff_t *f_pos)
{
	unsigned int minor = iminor(file->f_dentry->d_inode);
	int port = minor / 4;
	int offset = minor % 4;
	char val;

	/* check for a valid minor */
	if (port >= HT2020_PORTS_MAX || offset >= 3)
		return -ENODEV;

	/* only allow access to enabled ports */
	if (!ht2020_foundlist[port])
		return -ENODEV;

	/* check for end of file */
	if (*f_pos >= 1)
		return 0;

	val = ht_inb(portaddr_io8(port) + offset);
	if (put_user(val, buf))
		return -EFAULT;

	*f_pos += 1;
	return 1;
}

static ssize_t ht2020_write(struct file *file, const char *buf, size_t count,
		loff_t *f_pos)
{
	unsigned int minor = iminor(file->f_dentry->d_inode);
	int port = minor / 4;
	int offset = minor % 4;
	char val;

	/* check for a valid minor */
	if (port >= HT2020_PORTS_MAX || offset >= 3)
		return -ENODEV;

	/* only allow access to enabled ports */
	if (!ht2020_foundlist[port])
		return -ENODEV;

	/* check for writes past the data area */
	if (*f_pos >= 1)
		return -EOVERFLOW;

	if (get_user(val, buf))
		return -EFAULT;

	ht_outb(val, portaddr_io8(port) + offset);

	*f_pos += 1;
	return 1;
}

static struct file_operations ht2020_fops = {
	.owner 	= THIS_MODULE,
	.read	= ht2020_read,
	.write	= ht2020_write,
};

/*
 * ht2020_probe - check a port for HT2020 
 *
 * This checks the four channels of an I/O port. It expects the
 * HT2020 to be in reset state, and so the probe will of course 
 * fail if it isn't. 
 */
static int ht2020_probe(unsigned int port)
{
	unsigned long port_addr = portaddr_io8(port);

	if (ht_inb(port_addr))
		return -1;

	if (ht_inb(port_addr + 1))
		return -1;

	if (ht_inb(port_addr + 2))
		return -1;

	if (ht_inb(port_addr + 3) != 0xff)
		return -1;

	return 0;
}

static void release_mem_all_found_ports(void)
{
	int i;

	for (i = 0; i < HT2020_PORTS_MAX; i++) {
		if (ht2020_foundlist[i]) {
			release_mem_region(portaddr_io16(i), 4);
			release_mem_region(portaddr_io8(i), 4);
		}
	}
}

int __init ht2020_init(void)
{
	int ret, i, j, found_count = 0;
	dev_t dev_no = MKDEV(HT2020_MAJOR, 0);

#ifdef MODULE
	if (!io_count) {
		printk(KERN_WARNING DRIVER_NAME
			": You must supply I/O base address(es) "
			"with \"io=0xNNN\" value(s)\n");
		return -ENXIO;
	}
#endif

	/* acquire device number */
	ret = register_chrdev_region(dev_no, HT2020_MINOR_NR, DRIVER_NAME);
	if (ret < 0) {
		printk(KERN_WARNING DRIVER_NAME ": unable to get major %d.\n",
		       HT2020_MAJOR);
		return ret;
	}

	if (io_count) {
		/* enable any specified and valid addresses without probing */
		for (i = 0; i < io_count; i++) {
			for (j = 0; j < HT2020_PORTS_MAX; j++) {
				if (io[i] != ht2020_portlist[j])
					continue;
				ht2020_foundlist[j] = 1;
				found_count++;
				printk(KERN_INFO DRIVER_NAME
					": access enabled at 0x%0X\n",
					ht2020_portlist[j]);
			}
		}
		if (!found_count) {
			printk(KERN_WARNING DRIVER_NAME ": I/O base address(es)"
				" were not valid\n");
			goto probe_fail;
		}
	} else {
		/* probe all possible addresses */
		for (i = 0; i < HT2020_PORTS_MAX; i++) {
			if (ht2020_probe(i))
				continue;
			ht2020_foundlist[i] = 1;
			found_count++;
			printk(KERN_INFO DRIVER_NAME ": HT2020 found at 0x%0X\n",
				ht2020_portlist[i]);
		}
		if (!found_count) {
			printk(KERN_WARNING DRIVER_NAME ": HT2020 was not found"
				" at reset state at any I/O address.\n");
			goto probe_fail;
		}
	}

	/* acquire port memory addresses */
	for (i = 0; i < HT2020_PORTS_MAX; i++) {

		if (!ht2020_foundlist[i])
			continue;

		if (!request_mem_region(portaddr_io8(i), 4, DRIVER_NAME)) {
			printk(KERN_WARNING DRIVER_NAME
			       ": I/O port 0x%x-0x%x was not free.\n",
			       portaddr_io8(i), portaddr_io8(i) + 3);
			ht2020_foundlist[i] = 0;
			found_count--;
			if (!found_count)
				goto probe_fail;			
			continue;
		}

		if (!request_mem_region(portaddr_io16(i), 4, DRIVER_NAME)) {
			printk(KERN_WARNING DRIVER_NAME
			       ": I/O port 0x%x-0x%x was not free.\n",
			       portaddr_io16(i), portaddr_io16(i) + 3);
			release_region(ISAIO8_BASE + ht2020_portlist[i], 4);
			ht2020_foundlist[i] = 0;
			found_count--;
			if (!found_count)
				goto probe_fail;
		}

	}

	/* register device */
	cdev_init(&ht2020_cdev, &ht2020_fops);
	ht2020_cdev.owner = THIS_MODULE;
	ht2020_cdev.ops = &ht2020_fops;
	ret = cdev_add(&ht2020_cdev, dev_no, HT2020_MINOR_NR);
	if (ret) {
		printk(KERN_WARNING DRIVER_NAME ": error adding device\n");
		goto fail;
	}

	printk(KERN_INFO DRIVER_INFO "\n");
	return 0;

 probe_fail:
	ret = -EIO;
 fail:
	unregister_chrdev_region(dev_no, HT2020_MINOR_NR);
	return ret;
}

void __exit ht2020_cleanup(void)
{
	dev_t dev_no = MKDEV(HT2020_MAJOR, 0);

	cdev_del(&ht2020_cdev);
	unregister_chrdev_region(dev_no, HT2020_MINOR_NR);
	release_mem_all_found_ports();
}

module_init(ht2020_init);
module_exit(ht2020_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_VERSION(DRIVER_VERSION);
MODULE_DESCRIPTION(DRIVER_DESC);

