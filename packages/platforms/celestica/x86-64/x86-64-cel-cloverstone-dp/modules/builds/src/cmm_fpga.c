/*
 * cmm_fpga.c - driver for Cloverstone CMM board FPGA.
 *
 * Author: 
 *
 * Copyright (C) 2020 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DISCLAIMER: THIS FILE IS INTENEDED TO PRESERVE THE SYSFS NODES FOR 
 * BACKWARD COMPATIBLE WITH OLD DIAGNOSTIC PACKAGE.  THE MODULE *REMAP*
 * THE PCI MEMORY REGION 0 OF THE FPGA PCI DEVICE, YOU CAN SEE THE 
 * WARNING MESSAGE IN KERNEL LOG. PLEASE *DO NOT* FOLLOW THIS DESIGN.
 *  
 *  /
 *   \--sys
 *       \--devices
 *            \--platform
 *                \--cls_switch
 *                    |--FPGA_CMM
 *
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define MOD_VERSION "1.0.0"
#define FPGA_PCI_DEVICE_ID      0x7011 /*0x7021*/
#define FPGA_PCI_BAR_NUM        0
#define SWITCH_CPLD_ADAP_NUM    4

#define CLASS_NAME "cls_fpga"
#define DRIVER_NAME "cmm_fpga"
#define FPGA_PCI_NAME "cls_fpga_pci"
#define DEVICE_NAME "fwupgrade"

#if 1
  #define PRINT_DEBUG(fmt, ...)		printk(KERN_INFO fmt, ##__VA_ARGS__)   
#else
  #define PRINT_DEBUG(fmt, ...) 
#endif 

static int fpga_pci_probe(struct pci_dev *pdev);
static void fpga_pci_remove(void);


/*
========================================
FPGA PCIe BAR 0 Registers
========================================
Misc Control    0x00000000 - 0x000000FF.
I2C_CH1         0x00000100 - 0x00000110
I2C_CH2         0x00000200 - 0x00000210.
I2C_CH3         0x00000300 - 0x00000310.
I2C_CH4         0x00000400 - 0x00000410.
I2C_CH5         0x00000500 - 0x00000510.
I2C_CH6         0x00000600 - 0x00000610.
I2C_CH7         0x00000700 - 0x00000710.
I2C_CH8         0x00000800 - 0x00000810.
I2C_CH9         0x00000900 - 0x00000910.
I2C_CH10        0x00000A00 - 0x00000A10.
I2C_CH11        0x00000B00 - 0x00000B10.
I2C_CH12        0x00000C00 - 0x00000C10.
I2C_CH13        0x00000D00 - 0x00000D10.
SPI Master      0x00001200 - 0x00001300.
DPLL SPI Master 0x00001320 - 0x0000132F.
PORT XCVR       0x00004000 - 0x00004FFF.
*/

/* MISC       */
#define FPGA_VERSION            0x0000
#define FPGA_VERSION_MJ_MSK     0xff00
#define FPGA_VERSION_MN_MSK     0x00ff
#define FPGA_SCRATCH            0x0004
#define FPGA_PORT_XCVR_READY    0x000c

#define I2C_MASTER_CH_1             1
#define I2C_MASTER_CH_2             2
#define I2C_MASTER_CH_3             3

/* FPGA FRONT PANEL PORT MGMT */
#define SFF_PORT_CTRL_BASE          0x4000
#define SFF_PORT_STATUS_BASE        0x4004
#define SFF_PORT_INT_STATUS_BASE    0x4008
#define SFF_PORT_INT_MASK_BASE      0x400c

#define PORT_XCVR_REGISTER_SIZE     0x1000

static struct class*  fpgafwclass  = NULL; // < The device-driver class struct pointer

struct fpga_device {
    /* data mmio region */
    void __iomem *data_base_addr;
    resource_size_t data_mmio_start;
    resource_size_t data_mmio_len;
};

static struct fpga_device fpga_dev = {
    .data_base_addr = 0,
    .data_mmio_start = 0,
    .data_mmio_len = 0,
};

struct cmm_fpga_data {
    struct mutex fpga_lock;
    void __iomem * fpga_read_addr;
};

struct cmm_fpga_data *fpga_data;

/*
 * Kernel object for other module drivers.
 * Other module can use these kobject as a parent.
 */

static struct kobject *fpga = NULL;

/**
 * Show the value of the register set by 'set_fpga_reg_address'
 * If the address is not set by 'set_fpga_reg_address' first,
 * The version register is selected by default.
 * @param  buf     register value in hextring
 * @return         number of bytes read, or an error code
 */
static ssize_t get_fpga_reg_value(struct device *dev, struct device_attribute *devattr,
                                  char *buf)
{
    // read data from the address
    uint32_t data;
    data = ioread32(fpga_data->fpga_read_addr);
    return sprintf(buf, "0x%8.8x\n", data);
}
/**
 * Store the register address
 * @param  buf     address wanted to be read value of
 * @return         number of bytes stored, or an error code
 */
static ssize_t set_fpga_reg_address(struct device *dev, struct device_attribute *devattr,
                                    const char *buf, size_t count)
{
    uint32_t addr;
    char *last;

    addr = (uint32_t)strtoul(buf, &last, 16);
    if (addr == 0 && buf == last) {
        return -EINVAL;
    }
    fpga_data->fpga_read_addr = fpga_dev.data_base_addr + addr;
    return count;
}
/**
 * Show value of fpga scratch register
 * @param  buf     register value in hexstring
 * @return         number of bytes read, or an error code
 */
static ssize_t get_fpga_scratch(struct device *dev, struct device_attribute *devattr,
                                char *buf)
{
    return sprintf(buf, "0x%8.8x\n", ioread32(fpga_dev.data_base_addr + FPGA_SCRATCH) & 0xffffffff);
}
/**
 * Store value of fpga scratch register
 * @param  buf     scratch register value passing from user space
 * @return         number of bytes stored, or an error code
 */
static ssize_t set_fpga_scratch(struct device *dev, struct device_attribute *devattr,
                                const char *buf, size_t count)
{
    uint32_t data;
    char *last;
    data = (uint32_t)strtoul(buf, &last, 16);
    if (data == 0 && buf == last) {
        return -EINVAL;
    }
    iowrite32(data, fpga_dev.data_base_addr + FPGA_SCRATCH);
    return count;
}
/**
 * Store a value in a specific register address
 * @param  buf     the value and address in format '0xhhhh 0xhhhhhhhh'
 * @return         number of bytes sent by user space, or an error code
 */
static ssize_t set_fpga_reg_value(struct device *dev, struct device_attribute *devattr,
                                  const char *buf, size_t count)
{
    // register are 4 bytes
    uint32_t addr;
    uint32_t value;
    uint32_t mode = 8;
    char *tok;
    char clone[count];
    char *pclone = clone;
    char *last;

    strcpy(clone, buf);

    mutex_lock(&fpga_data->fpga_lock);
    tok = strsep((char**)&pclone, " ");
    if (tok == NULL) {
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    addr = (uint32_t)strtoul(tok, &last, 16);
    if (addr == 0 && tok == last) {
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if (tok == NULL) {
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    value = (uint32_t)strtoul(tok, &last, 16);
    if (value == 0 && tok == last) {
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if (tok == NULL) {
        mode = 32;
    } else {
        mode = (uint32_t)strtoul(tok, &last, 10);
        if (mode == 0 && tok == last) {
            mutex_unlock(&fpga_data->fpga_lock);
            return -EINVAL;
        }
    }
    if (mode == 32) {
        iowrite32(value, fpga_dev.data_base_addr + addr);
    } else if (mode == 8) {
        iowrite8(value, fpga_dev.data_base_addr + addr);
    } else {
        mutex_unlock(&fpga_data->fpga_lock);
        return -EINVAL;
    }
    mutex_unlock(&fpga_data->fpga_lock);
    return count;
}

/**
 * Read all FPGA XCVR register in binary mode.
 * @param  buf   Raw transceivers port startus and control register values
 * @return       number of bytes read, or an error code
 */
static ssize_t dump_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr, char *buf,
                         loff_t off, size_t count)
{
    unsigned long i = 0;
    ssize_t status;
    u8 read_reg;

    if ( off + count > PORT_XCVR_REGISTER_SIZE ) {
        return -EINVAL;
    }
    mutex_lock(&fpga_data->fpga_lock);
    while (i < count) {
        read_reg = ioread8(fpga_dev.data_base_addr + SFF_PORT_CTRL_BASE + off + i);
        buf[i++] = read_reg;
    }
    status = count;
    mutex_unlock(&fpga_data->fpga_lock);
    return status;
}

/**
 * Show FPGA port XCVR ready status
 * @param  buf  1 if the functin is ready, 0 if not.
 * @return      number of bytes read, or an error code
 */
static ssize_t ready_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u32 data;
    unsigned int REGISTER = FPGA_PORT_XCVR_READY;

    mutex_lock(&fpga_data->fpga_lock);
    data = ioread32(fpga_dev.data_base_addr + REGISTER);
    mutex_unlock(&fpga_data->fpga_lock);
    return sprintf(buf, "%d\n", (data >> 0) & 1U);
}

/* FPGA attributes */
static DEVICE_ATTR( getreg, 0600, get_fpga_reg_value, set_fpga_reg_address);
static DEVICE_ATTR( scratch, 0600, get_fpga_scratch, set_fpga_scratch);
static DEVICE_ATTR( setreg, 0200, NULL , set_fpga_reg_value);
static DEVICE_ATTR_RO(ready);
static BIN_ATTR_RO( dump, PORT_XCVR_REGISTER_SIZE);

static struct bin_attribute *fpga_bin_attrs[] = {
    &bin_attr_dump,
    NULL,
};

static struct attribute *fpga_attrs[] = {
    &dev_attr_getreg.attr,
    &dev_attr_scratch.attr,
    &dev_attr_setreg.attr,
    &dev_attr_ready.attr,
    NULL,
};

static struct attribute_group fpga_attr_grp = {
    .attrs = fpga_attrs,
    .bin_attrs = fpga_bin_attrs,
};

static void cmm_fpga_dev_release( struct device * dev)
{
    return;
}

static struct platform_device cmm_fpga_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = 0,
    .dev = {
        .release = cmm_fpga_dev_release,
    }
};

static int cmm_fpga_drv_probe(struct platform_device *pdev)
{
    int ret = 0;
    int portid_count;
    struct i2c_adapter *cpld_bus_adap;

	PRINT_DEBUG("cloverstone-dp cmm_fpga_drv_probe");

    struct pci_dev *pci_dev = pci_get_device(PCI_VENDOR_ID_XILINX, 
                                             FPGA_PCI_DEVICE_ID, 
                                             NULL);
    if (pci_dev){
        fpga_pci_probe(pci_dev);
        pci_dev_put(pci_dev);
    } else {
        ret = -ENODEV;
		PRINT_DEBUG("pci_get_device id:0x%x fail.", FPGA_PCI_DEVICE_ID);
        goto err_exit;
    }

    fpga_data = devm_kzalloc(&pdev->dev, sizeof(struct cmm_fpga_data),
                             GFP_KERNEL);

    if (!fpga_data){
        ret = -ENOMEM;
        goto err_exit;
    }

    /* The device class need to be instantiated before this function called */
    BUG_ON(fpgafwclass == NULL);

    fpga = kobject_create_and_add("FPGA", &pdev->dev.kobj);
    if (!fpga) {
        ret = -ENOMEM;
        goto err_exit;
    }

    ret = sysfs_create_group(fpga, &fpga_attr_grp);
    if (ret != 0) {
        printk(KERN_ERR "Cannot create FPGA sysfs attributes\n");
        goto err_remove_fpga;
    }

    // Set default read address to VERSION
    fpga_data->fpga_read_addr = fpga_dev.data_base_addr + FPGA_VERSION;
    mutex_init(&fpga_data->fpga_lock);

    return 0;

err_remove_grp_fpga:
    sysfs_remove_group(fpga, &fpga_attr_grp);
err_remove_fpga:
    kobject_put(fpga);
err_exit:
    return ret;
}

static int cmm_fpga_drv_remove(struct platform_device *pdev)
{
    int portid_count;

    sysfs_remove_group(fpga, &fpga_attr_grp);
    kobject_put(fpga);
    device_destroy(fpgafwclass, MKDEV(0, 0));
    fpga_pci_remove();
    return 0;
}

/* move this on top of platform_probe() */
static int fpga_pci_probe(struct pci_dev *pdev)
{
    int err;
    struct device *dev = &pdev->dev;
    uint32_t fpga_version;

    /* Skip the reqions request and mmap the resource */ 
    /* bar0: data mmio region */
    fpga_dev.data_mmio_start = pci_resource_start(pdev, FPGA_PCI_BAR_NUM);
    fpga_dev.data_mmio_len = pci_resource_len(pdev, FPGA_PCI_BAR_NUM);
    fpga_dev.data_base_addr = ioremap_nocache(fpga_dev.data_mmio_start, 
                                                fpga_dev.data_mmio_len);
    if (!fpga_dev.data_base_addr) {
        dev_err(dev, "cannot iomap region of size %lu\n",
                (unsigned long)fpga_dev.data_mmio_len);
        err = PTR_ERR(fpga_dev.data_base_addr);
        goto err_exit;
    }
    dev_info(dev, "data_mmio iomap base = 0x%lx \n",
             (unsigned long)fpga_dev.data_base_addr);
    dev_info(dev, "data_mmio_start = 0x%lx data_mmio_len = %lu\n",
             (unsigned long)fpga_dev.data_mmio_start,
             (unsigned long)fpga_dev.data_mmio_len);

    printk(KERN_INFO "FPGA PCIe driver probe OK.\n");
    printk(KERN_INFO "FPGA ioremap registers of size %lu\n", 
            (unsigned long)fpga_dev.data_mmio_len);
    printk(KERN_INFO "FPGA Virtual BAR %d at %8.8lx - %8.8lx\n", 
            FPGA_PCI_BAR_NUM,
            (unsigned long)fpga_dev.data_base_addr,
            (unsigned long)(fpga_dev.data_base_addr + fpga_dev.data_mmio_len));
    printk(KERN_INFO "");
    fpga_version = ioread32(fpga_dev.data_base_addr);
    printk(KERN_INFO "FPGA VERSION : %8.8x\n", fpga_version);

    fpgafwclass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(fpgafwclass)) {
        printk(KERN_ALERT "Failed to register device class\n");
        err = PTR_ERR(fpgafwclass);
        goto mem_unmap;
    }
    return 0;

mem_unmap:
    iounmap(fpga_dev.data_base_addr);
err_exit:
    return err;
}

static void fpga_pci_remove(void)
{
    iounmap(fpga_dev.data_base_addr);
    class_unregister(fpgafwclass);
    class_destroy(fpgafwclass);
};


static struct platform_driver cmm_fpga_drv = {
    .probe  = cmm_fpga_drv_probe,
    .remove = __exit_p(cmm_fpga_drv_remove),
    .driver = {
        .name   = DRIVER_NAME,
    },
};

int cmm_fpga_init(void)
{
    platform_device_register(&cmm_fpga_dev);
    platform_driver_register(&cmm_fpga_drv);
    return 0;
}

void cmm_fpga_exit(void)
{
    platform_driver_unregister(&cmm_fpga_drv);
    platform_device_unregister(&cmm_fpga_dev);
}

module_init(cmm_fpga_init);
module_exit(cmm_fpga_exit);

MODULE_AUTHOR("Celestica Inc.");
MODULE_DESCRIPTION("CMM FPGA Sysfs Nodes");
MODULE_VERSION(MOD_VERSION);
MODULE_LICENSE("GPL");