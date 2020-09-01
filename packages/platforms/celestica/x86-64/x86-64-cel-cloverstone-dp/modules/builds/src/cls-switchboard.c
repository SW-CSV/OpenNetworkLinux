/*
 * cls-switchboard.c - PCI device driver for Silverstone Switch board FPGA.
 *
 * Author: Pradchaya Phucharoen
 *
 * Copyright (C) 2019 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/acpi.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/platform_data/pca954x.h>
#include "i2c-ocores.h"
#include "xcvr-cls.h"

#define MOD_VERSION "2.1.0-6"
#define DRV_NAME "cls-switchboard"

#define I2C_MUX_CHANNEL(_ch, _adap_id, _deselect) \
	[_ch] = { .adap_id = _adap_id, .deselect_on_exit = _deselect }

#define FPGA_PCIE_DEVICE_ID	0x7011
#define FPGA_TYPE_ADDR		0x404
#define FPGA_LC_TYPE		0x5a
#define FPGA_CMM_TYPE		0x5b

#define MMIO_BAR			0
#define I2C_BUS_LC1_OFS		15
#define I2C_BUS_LC2_OFS		55
#define I2C_BUSID_OFS		8

/* I2C ocore configurations */
#define OCORE_REGSHIFT		2
#define OCORE_IP_CLK_khz	62500
#define OCORE_BUS_CLK_khz	100
#define OCORE_REG_IO_WIDTH	1

/* Optical port xcvr configuration */
#define XCVR_REG_SHIFT		2
#define XCVR_NUM_PORT		34
#define XCVR_PORT_REG_SIZE	0x10

/* i2c_bus_config - an i2c-core resource and platform data
 *  @id - I2C bus device ID, for identification.
 *  @res - resources for an i2c-core device.
 *  @num_res - size of the resources.
 *  @pdata - a platform data of an i2c-core device.
 */
struct i2c_bus_config {
	int id;
	struct resource *res;
	ssize_t num_res;
	struct ocores_i2c_platform_data pdata;
}; 

/* switchbrd_priv - switchboard private data */
struct switchbrd_priv {
	void __iomem *iomem;
	unsigned long base;
	int num_i2c_bus;
	const char *i2c_devname;	
	const char *xcvr_devname;	
	struct platform_device **i2cbuses_pdev;
	struct platform_device *regio_pdev;
	struct platform_device *spiflash_pdev;
	struct platform_device *xcvr_pdev;
};

/* I2C bus speed param */
static int bus_clock_master_1 = 100;
module_param(bus_clock_master_1, int, 0660);
MODULE_PARM_DESC(bus_clock_master_1, 
	"I2C master 1 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_2 = 100;
module_param(bus_clock_master_2, int, 0660);
MODULE_PARM_DESC(bus_clock_master_2, 
	"I2C master 2 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_3 = 100;
module_param(bus_clock_master_3, int, 0660);
MODULE_PARM_DESC(bus_clock_master_3, 
	"I2C master 3 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_4 = 100;
module_param(bus_clock_master_4, int, 0660);
MODULE_PARM_DESC(bus_clock_master_4, 
	"I2C master 4 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_5 = 100;
module_param(bus_clock_master_5, int, 0660);
MODULE_PARM_DESC(bus_clock_master_5, 
	"I2C master 5 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_6 = 100;
module_param(bus_clock_master_6, int, 0660);
MODULE_PARM_DESC(bus_clock_master_6, 
	"I2C master 6 bus speed in KHz 50/80/100/200/400");

static int bus_clock_master_7 = 100;
module_param(bus_clock_master_7, int, 0660);
MODULE_PARM_DESC(bus_clock_master_7, 
	"I2C master 7 bus speed in KHz 50/80/100/200/400");

// NOTE:  Cloverstone i2c channel mapping is very wierd!!!
/* PCA9548 channel config on MASTER BUS */
static struct pca954x_platform_mode i2c_mux_71_LC1[] = {
	I2C_MUX_CHANNEL(0, I2C_BUS_LC1_OFS +  1, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC1_OFS +  2, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC1_OFS +  3, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC1_OFS +  4, true),
	I2C_MUX_CHANNEL(4, I2C_BUS_LC1_OFS +  5, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC1_OFS +  6, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC1_OFS +  7, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC1_OFS +  8, true),
};

static struct pca954x_platform_mode i2c_mux_72_LC1[] = {
	I2C_MUX_CHANNEL(0, I2C_BUS_LC1_OFS + 9, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC1_OFS + 10, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC1_OFS + 11, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC1_OFS + 12, true),
	I2C_MUX_CHANNEL(4, I2C_BUS_LC1_OFS + 13, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC1_OFS + 14, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC1_OFS + 15, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC1_OFS + 16, true),
};

static struct pca954x_platform_mode i2c_mux_73_LC1[] = {
	I2C_MUX_CHANNEL(4, I2C_BUS_LC1_OFS + 33, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC1_OFS + 34, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC1_OFS + 35, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC1_OFS + 36, true),
	I2C_MUX_CHANNEL(0, I2C_BUS_LC1_OFS + 37, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC1_OFS + 38, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC1_OFS + 39, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC1_OFS + 40, true),
};
	
static struct pca954x_platform_mode i2c_mux_75_LC1[] = {
	I2C_MUX_CHANNEL(4, I2C_BUS_LC1_OFS + 21, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC1_OFS + 22, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC1_OFS + 23, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC1_OFS + 24, true),
	I2C_MUX_CHANNEL(0, I2C_BUS_LC1_OFS + 17, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC1_OFS + 18, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC1_OFS + 19, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC1_OFS + 20, true),
};

static struct pca954x_platform_mode i2c_mux_74_LC1[] = {
	I2C_MUX_CHANNEL(0, I2C_BUS_LC1_OFS + 29, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC1_OFS + 30, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC1_OFS + 31, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC1_OFS + 32, true),
	I2C_MUX_CHANNEL(4, I2C_BUS_LC1_OFS + 25, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC1_OFS + 26, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC1_OFS + 27, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC1_OFS + 28, true),
};
	
static struct pca954x_platform_mode i2c_mux_71_LC2[] = {
	I2C_MUX_CHANNEL(0, I2C_BUS_LC2_OFS +  1, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC2_OFS +  2, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC2_OFS +  3, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC2_OFS +  4, true),
	I2C_MUX_CHANNEL(4, I2C_BUS_LC2_OFS +  5, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC2_OFS +  6, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC2_OFS +  7, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC2_OFS +  8, true),
};

static struct pca954x_platform_mode i2c_mux_72_LC2[] = {
	I2C_MUX_CHANNEL(0, I2C_BUS_LC2_OFS + 9, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC2_OFS + 10, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC2_OFS + 11, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC2_OFS + 12, true),
	I2C_MUX_CHANNEL(4, I2C_BUS_LC2_OFS + 13, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC2_OFS + 14, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC2_OFS + 15, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC2_OFS + 16, true),
};

static struct pca954x_platform_mode i2c_mux_73_LC2[] = {
	I2C_MUX_CHANNEL(4, I2C_BUS_LC2_OFS + 33, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC2_OFS + 34, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC2_OFS + 35, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC2_OFS + 36, true),
	I2C_MUX_CHANNEL(0, I2C_BUS_LC2_OFS + 37, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC2_OFS + 38, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC2_OFS + 39, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC2_OFS + 40, true),
};
	
static struct pca954x_platform_mode i2c_mux_75_LC2[] = {
	I2C_MUX_CHANNEL(4, I2C_BUS_LC2_OFS + 21, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC2_OFS + 22, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC2_OFS + 23, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC2_OFS + 24, true),
	I2C_MUX_CHANNEL(0, I2C_BUS_LC2_OFS + 17, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC2_OFS + 18, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC2_OFS + 19, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC2_OFS + 20, true),
};

static struct pca954x_platform_mode i2c_mux_74_LC2[] = {
	I2C_MUX_CHANNEL(0, I2C_BUS_LC2_OFS + 29, true),
	I2C_MUX_CHANNEL(1, I2C_BUS_LC2_OFS + 30, true),
	I2C_MUX_CHANNEL(2, I2C_BUS_LC2_OFS + 31, true),
	I2C_MUX_CHANNEL(3, I2C_BUS_LC2_OFS + 32, true),
	I2C_MUX_CHANNEL(4, I2C_BUS_LC2_OFS + 25, true),
	I2C_MUX_CHANNEL(5, I2C_BUS_LC2_OFS + 26, true),
	I2C_MUX_CHANNEL(6, I2C_BUS_LC2_OFS + 27, true),
	I2C_MUX_CHANNEL(7, I2C_BUS_LC2_OFS + 28, true),
};

static struct pca954x_platform_data om_muxes[] = {
	{
		.modes = i2c_mux_71_LC1,
		.num_modes = ARRAY_SIZE(i2c_mux_71_LC1),
	},
	{
		.modes = i2c_mux_72_LC1,
		.num_modes = ARRAY_SIZE(i2c_mux_72_LC1),
	},
	{
		.modes = i2c_mux_73_LC1,
		.num_modes = ARRAY_SIZE(i2c_mux_73_LC1),
	},
	{
		.modes = i2c_mux_74_LC1,
		.num_modes = ARRAY_SIZE(i2c_mux_74_LC1),
	},
	{
		.modes = i2c_mux_75_LC1,
		.num_modes = ARRAY_SIZE(i2c_mux_75_LC1),
	},
	{
		.modes = i2c_mux_71_LC2,
		.num_modes = ARRAY_SIZE(i2c_mux_71_LC2),
	},
	{
		.modes = i2c_mux_72_LC2,
		.num_modes = ARRAY_SIZE(i2c_mux_72_LC2),
	},
	{
		.modes = i2c_mux_73_LC2,
		.num_modes = ARRAY_SIZE(i2c_mux_73_LC2),
	},
	{
		.modes = i2c_mux_74_LC2,
		.num_modes = ARRAY_SIZE(i2c_mux_74_LC2),
	},
	{
		.modes = i2c_mux_75_LC2,
		.num_modes = ARRAY_SIZE(i2c_mux_75_LC2),
	},
};

/* Optical Module bus 1-7 i2c muxes info */
static struct i2c_board_info i2c_info[] = {
	{
		I2C_BOARD_INFO("pca9548", 0x71),
		.platform_data = &om_muxes[0],
	},
	{
		I2C_BOARD_INFO("pca9548", 0x72),
		.platform_data = &om_muxes[1],
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),
		.platform_data = &om_muxes[2],
	},
	{
		I2C_BOARD_INFO("pca9548", 0x74),
		.platform_data = &om_muxes[3],
	},
	{
		I2C_BOARD_INFO("pca9548", 0x75),
		.platform_data = &om_muxes[4],
	},
};

/* RESOURCE SEPERATES BY FUNCTION */
/* Resource IOMEM for i2c bus 1 */
static struct resource cls_i2c_res_1[] = {
	{
		.start = 0x800, .end = 0x81F,
		.flags = IORESOURCE_MEM,}, 
};

/* Resource IOMEM for i2c bus 2 */
static struct resource  cls_i2c_res_2[] = {
	{
		.start = 0x820, .end = 0x83F,
		.flags = IORESOURCE_MEM,}, 
};

/* Resource IOMEM for i2c bus 3 */
static struct resource  cls_i2c_res_3[] = {
	{
		.start = 0x840, .end = 0x85F,
		.flags = IORESOURCE_MEM,}, 
};

/* Resource IOMEM for i2c bus 4 */
static struct  resource cls_i2c_res_4[] = {
	{
		.start = 0x860, .end = 0x87F,
		.flags = IORESOURCE_MEM,}, 
};

/* Resource IOMEM for i2c bus 5 */
static struct resource  cls_i2c_res_5[] = {
	{
		.start = 0x880, .end = 0x89F,
		.flags = IORESOURCE_MEM,}, 
};

/* Resource IOMEM for i2c bus 6 */
static struct resource  cls_i2c_res_6[] = {
	{
		.start = 0x8a0, .end = 0x8bF,
		.flags = IORESOURCE_MEM,}, 
};

/* Resource IOMEM for i2c bus 6 */
static struct resource  cls_i2c_res_7[] = {
{
	.start = 0x8c0, .end = 0x8dF,
	.flags = IORESOURCE_MEM,}, 
};

#if 0
/* Resource IOMEM for reg access */
static struct resource reg_io_res[] = {
	{       
		.start = 0x00, .end = 0xFF,
		.flags = IORESOURCE_MEM,},
};
		
/* Resource IOMEM for spi flash firmware upgrade */
static struct resource spi_flash_res[] = {
	{       
		.start = 0x1200, .end = 0x121F,
		.flags = IORESOURCE_MEM,},
};
#endif

/* Resource IOMEM for front panel XCVR */
static struct resource xcvr_res[] = {
	{       
		.start = 0x4000, .end = 0x421F,
		.flags = IORESOURCE_MEM,},
};

static struct i2c_bus_config i2c_bus_configs[] = {
	{
		.id = 1,
		.res = cls_i2c_res_1,
		.num_res = ARRAY_SIZE(cls_i2c_res_1),
		.pdata = {
			.reg_shift = OCORE_REGSHIFT,
			.reg_io_width = OCORE_REG_IO_WIDTH,
			.clock_khz = OCORE_IP_CLK_khz,
			.bus_khz = OCORE_BUS_CLK_khz,
			.big_endian = false,
			.num_devices = 1,
			.devices = &i2c_info[0],
		},
	},
	{
		.id = 2, 
		.res = cls_i2c_res_2, 
		.num_res = ARRAY_SIZE(cls_i2c_res_2), 
		.pdata = {
			.reg_shift = OCORE_REGSHIFT,
			.reg_io_width = OCORE_REG_IO_WIDTH,
			.clock_khz = OCORE_IP_CLK_khz,
			.bus_khz = OCORE_BUS_CLK_khz,
			.big_endian = false,
			.num_devices = 1,
			.devices = &i2c_info[1],
		},
	},
	{
		.id = 3, 
		.res = cls_i2c_res_3, 
		.num_res = ARRAY_SIZE(cls_i2c_res_3), 
		.pdata = {
			.reg_shift = OCORE_REGSHIFT,
			.reg_io_width = OCORE_REG_IO_WIDTH,
			.clock_khz = OCORE_IP_CLK_khz,
			.bus_khz = OCORE_BUS_CLK_khz,
			.big_endian = false,
			.num_devices = 1,
			.devices = &i2c_info[2],
		},
	},
	{
		.id = 4, 
		.res = cls_i2c_res_4, 
		.num_res = ARRAY_SIZE(cls_i2c_res_4), 
		.pdata = {
			.reg_shift = OCORE_REGSHIFT,
			.reg_io_width = OCORE_REG_IO_WIDTH,
			.clock_khz = OCORE_IP_CLK_khz,
			.bus_khz = OCORE_BUS_CLK_khz,
			.big_endian = false,
			.num_devices = 1,
			.devices = &i2c_info[3],
		},
	},
	{
		.id = 5, 
		.res = cls_i2c_res_5, 
		.num_res = ARRAY_SIZE(cls_i2c_res_5), 
		.pdata = {
			.reg_shift = OCORE_REGSHIFT,
			.reg_io_width = OCORE_REG_IO_WIDTH,
			.clock_khz = OCORE_IP_CLK_khz,
			.bus_khz = OCORE_BUS_CLK_khz,
			.big_endian = false,
			.num_devices = 1,
			.devices = &i2c_info[4],
		},
	},
	{
		.id = 6, 
		.res = cls_i2c_res_6, 
		.num_res = ARRAY_SIZE(cls_i2c_res_6), 
		.pdata = {
			.reg_shift = OCORE_REGSHIFT,
			.reg_io_width = OCORE_REG_IO_WIDTH,
			.clock_khz = OCORE_IP_CLK_khz,
			.bus_khz = OCORE_BUS_CLK_khz,
			.big_endian = false,
			.num_devices = 0,
			.devices = NULL,
		},
	},
	{
		.id = 7, 
		.res = cls_i2c_res_7, 
		.num_res = ARRAY_SIZE(cls_i2c_res_7), 
		.pdata = {
			.reg_shift = OCORE_REGSHIFT,
			.reg_io_width = OCORE_REG_IO_WIDTH,
			.clock_khz = OCORE_IP_CLK_khz,
			.bus_khz = OCORE_BUS_CLK_khz,
			.big_endian = false,
			.num_devices = 0,
			.devices = NULL,
		},
	},
};

/* xcvr front panel mapping */
static struct port_info front_panel_ports[] = {
	{"QSFP1",   1, QSFP},
	{"QSFP2",   2, QSFP},
	{"QSFP3",   3, QSFP},
	{"QSFP4",   4, QSFP},
	{"QSFP5",   5, QSFP},
	{"QSFP6",   6, QSFP},
	{"QSFP7",   7, QSFP},
	{"QSFP8",   8, QSFP},
	{"QSFP9",   9, QSFP},
	{"QSFP10", 10, QSFP},
	{"QSFP11", 11, QSFP},
	{"QSFP12", 12, QSFP},
	{"QSFP13", 13, QSFP},
	{"QSFP14", 14, QSFP},
	{"QSFP15", 15, QSFP},
	{"QSFP16", 16, QSFP},
	{"QSFP17", 17, QSFP},
	{"QSFP18", 18, QSFP},
	{"QSFP19", 19, QSFP},
	{"QSFP20", 20, QSFP},
	{"QSFP21", 21, QSFP},
	{"QSFP22", 22, QSFP},
	{"QSFP23", 23, QSFP},
	{"QSFP24", 24, QSFP},
	{"QSFP25", 25, QSFP},
	{"QSFP26", 26, QSFP},
	{"QSFP27", 27, QSFP},
	{"QSFP28", 28, QSFP},
	{"QSFP29", 29, QSFP},
	{"QSFP30", 30, QSFP},
	{"QSFP31", 31, QSFP},
	{"QSFP32", 32, QSFP},
	{"QSFP33", 33, QSFP},
	{"QSFP34", 34, QSFP},
	{"QSFP35", 35, QSFP},
	{"QSFP36", 36, QSFP},
	{"QSFP37", 37, QSFP},
	{"QSFP38", 38, QSFP},
	{"QSFP39", 39, QSFP},
	{"QSFP40", 40, QSFP},
	/* END OF LIST */
};
	
static struct cls_xcvr_platform_data xcvr_data[] = {
	{
		.port_reg_size = 0x10,
		.num_ports = ARRAY_SIZE(front_panel_ports),
		.devices = front_panel_ports,
	},
};

static int fpga_init_index = 0;

// TODO: Add a platform configuration struct, and use probe as a factory,
//	 so xcvr, fwupgrade device can configured as options.

static int cls_fpga_probe(struct pci_dev *dev, const struct pci_device_id *id)
{	
	int err;
	int num_i2c_bus, i;
	unsigned long rstart;
	void __iomem *base_addr;
	struct switchbrd_priv *priv;
	struct platform_device **i2cbuses_pdev;
	//struct platform_device *regio_pdev;
	struct platform_device *xcvr_pdev;
	uint32_t fpga_type;
	
	err = pci_enable_device(dev);
	if (err){
		dev_err(&dev->dev,  "Failed to enable PCI device\n");
		goto err_exit;
	}

	/* Check for valid MMIO address */
	base_addr = pci_iomap(dev, MMIO_BAR, 0);
	if (!base_addr) {
		dev_err(&dev->dev,  "Failed to map PCI device mem\n");
		err = -ENODEV;
		goto err_disable_device;
	}

	fpga_type = ioread32(base_addr + FPGA_TYPE_ADDR);
	printk("fpga Type:0x%8.8x\n",fpga_type);
	if (fpga_type == FPGA_CMM_TYPE) {
		err = 0;
		goto err_exit;
	}

	rstart = pci_resource_start(dev, MMIO_BAR);
	if (!rstart) {
		dev_err(&dev->dev, "Switchboard base address uninitialized, "
			"check FPGA\n");
		err = -ENODEV;
		goto err_unmap;
	}

	dev_dbg(&dev->dev, "BAR%d res: 0x%lx-0x%llx\n", MMIO_BAR, 
		rstart, pci_resource_end(dev, MMIO_BAR));

	priv = devm_kzalloc(&dev->dev, 
				sizeof(struct switchbrd_priv), GFP_KERNEL);
	if (!priv){
		err = -ENOMEM;
		goto err_unmap;
	}

	pci_set_drvdata(dev, priv);
	num_i2c_bus = ARRAY_SIZE(i2c_bus_configs);
	i2cbuses_pdev = devm_kzalloc(
				&dev->dev, 
				num_i2c_bus * sizeof(struct platform_device*), 
				GFP_KERNEL);

	xcvr_res[0].start += rstart;
	xcvr_res[0].end += rstart;

#if 0
	reg_io_res[0].start += rstart;
	reg_io_res[0].end += rstart;

	regio_pdev = platform_device_register_resndata(
			&dev->dev, "cls-swbrd-io", 
			-1,
			reg_io_res, ARRAY_SIZE(reg_io_res),
			NULL, 0);

	if (IS_ERR(regio_pdev)) {
		dev_err(&dev->dev, "Failed to register cls-swbrd-io\n");
		err = PTR_ERR(regio_pdev);
		goto err_disable_device;
	}
#endif

	switch(fpga_init_index)
	{
		case 0:
			priv->i2c_devname = "ocores-i2c-cls0";
			priv->xcvr_devname = "cls-xcvr0";
			break;
		case 1:
			priv->i2c_devname = "ocores-i2c-cls1";
			priv->xcvr_devname = "cls-xcvr1";
			break;
		default:
			break; 
	}	
	xcvr_pdev = platform_device_register_resndata(
											NULL,
											priv->xcvr_devname, 
											-1,
											xcvr_res,
											ARRAY_SIZE(xcvr_res),
											&xcvr_data,
											sizeof(xcvr_data));	
	if (IS_ERR(xcvr_pdev)) {
		dev_err(&dev->dev, "Failed to register xcvr node\n");
		err = PTR_ERR(xcvr_pdev);
		goto err_unmap;
	}

	for(i = 0; i < num_i2c_bus; i++){

		i2c_bus_configs[i].res[0].start += rstart;
		i2c_bus_configs[i].res[0].end += rstart;
	
		if (fpga_init_index == 1) {
			i2c_bus_configs[i].id = I2C_BUSID_OFS + i;
			if (i < 5) {
				i2c_info[i].platform_data = &om_muxes[5 + i] ;
			}
		}

		switch (i + 1) {
		case 1:
			i2c_bus_configs[i].pdata.bus_khz = bus_clock_master_1;
			break;
		case 2:
			i2c_bus_configs[i].pdata.bus_khz = bus_clock_master_2;
			break;
		case 3:
			i2c_bus_configs[i].pdata.bus_khz = bus_clock_master_3;
			break;
		case 4:
			i2c_bus_configs[i].pdata.bus_khz = bus_clock_master_4;
			break;
		case 5:
			i2c_bus_configs[i].pdata.bus_khz = bus_clock_master_5;
			break;
		case 6:
			i2c_bus_configs[i].pdata.bus_khz = bus_clock_master_6;
			break;
		case 7:
			i2c_bus_configs[i].pdata.bus_khz = bus_clock_master_7;
			break;
		default:
			i2c_bus_configs[i].pdata.bus_khz = OCORE_BUS_CLK_khz;
		}

		dev_dbg(&dev->dev, "i2c-bus.%d: 0x%llx - 0x%llx\n",
			i2c_bus_configs[i].id, 
			i2c_bus_configs[i].res[0].start, 
			i2c_bus_configs[i].res[0].end);

	i2cbuses_pdev[i] = platform_device_register_resndata(
								&dev->dev, 
								priv->i2c_devname, 
								i2c_bus_configs[i].id,
								i2c_bus_configs[i].res,
								i2c_bus_configs[i].num_res,
								&i2c_bus_configs[i].pdata,
								sizeof(i2c_bus_configs[i].pdata));
		if (IS_ERR(i2cbuses_pdev[i])) {
			dev_err(&dev->dev, "Failed to register ocores-i2c-cls.%d\n", 
				i2c_bus_configs[i].id);
			err = PTR_ERR(i2cbuses_pdev[i]);
			goto err_unregister_ocore;
		}
	}
	
	priv->iomem = base_addr;
	priv->base = rstart;
	priv->num_i2c_bus = num_i2c_bus;
	priv->i2cbuses_pdev = i2cbuses_pdev;
	//priv->regio_pdev = regio_pdev;
	priv->xcvr_pdev = xcvr_pdev;
	fpga_init_index += 1;
	
	return 0;

err_unregister_ocore:
	for(i = 0; i < num_i2c_bus; i++){
		if(priv->i2cbuses_pdev[i]){
			platform_device_unregister(priv->i2cbuses_pdev[i]);
		}
	}
	platform_device_unregister(xcvr_pdev);
//err_unregister_regio:
	//platform_device_unregister(regio_pdev);
err_unmap:
	pci_iounmap(dev, base_addr);
err_disable_device:
	pci_disable_device(dev);
err_exit:
	return err;
}

static void cls_fpga_remove(struct pci_dev *dev)
{
	int i;
	struct switchbrd_priv *priv = pci_get_drvdata(dev);

	for(i = 0; i < priv->num_i2c_bus; i++){
		if(priv->i2cbuses_pdev[i])
			platform_device_unregister(priv->i2cbuses_pdev[i]);
	}
	platform_device_unregister(priv->xcvr_pdev);
	//platform_device_unregister(priv->regio_pdev);
	pci_iounmap(dev, priv->iomem);
	pci_disable_device(dev);
	return;
};

static const struct pci_device_id pci_clsswbrd[] = {
	{  PCI_VDEVICE(XILINX, FPGA_PCIE_DEVICE_ID) },
	{0, }
};

MODULE_DEVICE_TABLE(pci, pci_clsswbrd);

static struct pci_driver cls_pci_driver = {
	.name = DRV_NAME,
	.id_table = pci_clsswbrd,
	.probe = cls_fpga_probe,
	.remove = cls_fpga_remove,
};

module_pci_driver(cls_pci_driver);

MODULE_AUTHOR("Pradchaya P.<pphuchar@celestica.com>");
MODULE_DESCRIPTION("Celestica cloverstone switchboard driver");
MODULE_VERSION(MOD_VERSION);
MODULE_LICENSE("GPL");

