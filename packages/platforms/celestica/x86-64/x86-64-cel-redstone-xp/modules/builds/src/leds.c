/*
 * leds.c - Driver for cel front panel LEDs
 *
 * Copyright (C) 2017 Celestica Corp.
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
 */

#include <linux/leds.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include "cel_sysfs_core.h"


#define DRIVER_NAME     "cel_leds"
#define FRONT_LED_ADDR  0x303

static struct cel_sysfs_obj *leds_obj;

static int cel_led_blink_stat(struct led_classdev *led_cdev,
                                unsigned long *delay_on,
                                unsigned long *delay_off)
{
    unsigned char led;

    if (!(*delay_on == 0 && *delay_off == 0) &&
        !(*delay_on == 250 && *delay_off == 250) &&
        !(*delay_on == 500 && *delay_off == 500))
            return -EINVAL;

    led = inb(FRONT_LED_ADDR);
    led &= 0xfc;

    if ((*delay_on == 250) && (*delay_off == 250))
        led |= 0x02;
    else if ((*delay_on == 500) && (*delay_off == 500))
        led |= 0x01;

    outb(led, FRONT_LED_ADDR);

    return 0;
}

static ssize_t cel_led_blink_show_stat(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
    struct led_classdev *leddev = dev_get_drvdata(dev);
    unsigned char led;
    const char *msg;

    led = inb(FRONT_LED_ADDR);
    led &= 0x03;

    switch (led)
    {
    case 0:
        msg = "No blinking, turn on";
        break;
    case 1:
        msg = "1 Hz is blinking";
        break;
    case 2:
        msg = "4 Hz is blinking";
        break;
    case 3:
        msg = "No blinking, turn off";
        break;
    default:
        msg = "Unknown error";
        break;
    }

    return sprintf(buf, "%s\n", msg);
}

static ssize_t cel_led_blink_store_stat(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
    int ret;
    struct led_classdev *leddev = dev_get_drvdata(dev);
    unsigned long blink_state;
    unsigned char led;

    ret = kstrtoul(buf, 10, &blink_state);
    if (ret)
        return ret;

    led = inb(FRONT_LED_ADDR);
    led &= 0xfc;

    switch (blink_state)
    {
    case 0:
        led |= 0x03;
        break;
    case 1:
        break;
    case 250:
        led |= 0x02;
        break;
    case 500:
        led |= 0x01;
        break;
    default:
        return -EINVAL;
        break;
    }

    outb(led, FRONT_LED_ADDR);

    return size;
}
static DEVICE_ATTR(blink, 0644, cel_led_blink_show_stat, cel_led_blink_store_stat);

static void cel_led_brightness_set_stat(struct led_classdev *led_cdev,
                                        enum led_brightness brightness)
{
    unsigned char led;

    led = inb(FRONT_LED_ADDR);
    led &= 0xfc;

    if (!brightness)
        led |= 0x03;

    outb( led, FRONT_LED_ADDR);
}

enum led_brightness cel_led_brightness_get_p2(struct led_classdev *led_cdev)
{
    unsigned char led;

    led = inb(FRONT_LED_ADDR);

    return (led & 0x08) ? LED_OFF : LED_FULL;
}

static void cel_led_brightness_set_p2(struct led_classdev *led_cdev,
                                        enum led_brightness brightness)
{
    unsigned char led;

    led = inb(FRONT_LED_ADDR);
    led &= 0xf7;

    if (!brightness)
        led |= 0x08;

    outb( led, FRONT_LED_ADDR);
}

enum led_brightness cel_led_brightness_get_p1(struct led_classdev *led_cdev)
{
    unsigned char led;

    led = inb(FRONT_LED_ADDR);

    return (led & 0x04) ? LED_OFF : LED_FULL;
}

static void cel_led_brightness_set_p1(struct led_classdev *led_cdev,
                                        enum led_brightness brightness)
{
    unsigned char led;

    led = inb(FRONT_LED_ADDR);
    led &= 0xfb;

    if (!brightness)
        led |= 0x04;

    outb( led, FRONT_LED_ADDR);
}

static struct led_classdev cel_leds[] = {
    {
	    .name           = "cel:green:p-1",
	    .brightness     = LED_OFF,
	    .max_brightness = 1,
	    .brightness_get = cel_led_brightness_get_p1,
	    .brightness_set = cel_led_brightness_set_p1,
    },
    {
        .name           = "cel:green:p-2",
        .brightness     = LED_OFF,
        .max_brightness = 1,
        .brightness_get = cel_led_brightness_get_p2,
        .brightness_set = cel_led_brightness_set_p2,
    },
    {
        .name           = "cel:green:stat",
        .brightness     = LED_OFF,
        .max_brightness = 1,
        .brightness_set = cel_led_brightness_set_stat,
        .blink_set      = cel_led_blink_stat,
        .flags          = LED_CORE_SUSPENDRESUME,
    },
};

static struct resource cel_led_resources[] = {
    {
        .flags  = IORESOURCE_IO,
    },
};

static void cel_led_dev_release( struct device * dev)
{
    return;
}

static struct platform_device cel_lpc_dev = {
    .name           = DRIVER_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cel_led_resources),
    .resource       = cel_led_resources,
    .dev = {
	    .release = cel_led_dev_release,
    }
};

static int cel_led_drv_probe(struct platform_device *pdev)
{
    int i, ret;

    for (i = 0; i < ARRAY_SIZE(cel_leds); i++) {
        ret = led_classdev_register(&pdev->dev, &cel_leds[i]);
        if (ret < 0)
            goto exit;
    }

    ret = device_create_file(&pdev->dev, &dev_attr_blink);
    if (ret)
    {
        for (i = 0; i < ARRAY_SIZE(cel_leds); i++)
            led_classdev_unregister(&cel_leds[i]);
    }
	leds_obj = cel_sysfs_kobject_init();
	if(!leds_obj) {
		printk("create kobj %s error", LEDS_DIR_NAME);
		return -ENODEV;
	}
	ret = sysfs_create_link(&leds_obj->kobj, &pdev->dev.kobj, LEDS_DIR_NAME);
	if (ret) {
		printk("Cannot create the link: err=%d\n", ret);
		kobject_put(leds_obj);
		return -ENODEV;
	}
exit:
        return ret;
}

static int cel_led_drv_remove(struct platform_device *pdev)
{
    int i;

	sysfs_remove_link(&leds_obj->kobj, LEDS_DIR_NAME);
	kobject_put(leds_obj);
    for (i = 0; i < ARRAY_SIZE(cel_leds); i++)
        led_classdev_unregister(&cel_leds[i]);

    device_remove_file(&pdev->dev, &dev_attr_blink);

    return 0;
}

static struct platform_driver cel_led_drv = {
    .probe  = cel_led_drv_probe,
    .remove = __exit_p(cel_led_drv_remove),
    .driver = {
        .name   = DRIVER_NAME,
    },
};

int cel_led_init(void)
{
    platform_device_register(&cel_lpc_dev);
    platform_driver_register(&cel_led_drv);

    return 0;
}

void cel_led_exit(void)
{
    platform_driver_unregister(&cel_led_drv);
    platform_device_unregister(&cel_lpc_dev);
}

module_init(cel_led_init);
module_exit(cel_led_exit);

MODULE_AUTHOR("Larry Ming <laming@celestica.com");
MODULE_DESCRIPTION("Celestica LEDs Driver");
MODULE_LICENSE("GPL");
