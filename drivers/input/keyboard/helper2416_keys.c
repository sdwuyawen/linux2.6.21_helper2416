/*
 * Driver for keys on GPIO lines capable of polling.
 *
 * Copyright 2011 chenjun@jyxtec.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/gpio_keys.h>
#include <linux/kthread.h>

#include <asm/gpio.h>

static struct task_struct *helper2416_keys_threadp;
static int helper2416_keys_thread(void *data)
{
	struct platform_device  * pdev = (struct platform_device *)data;
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_keys_button * buttons = pdata->buttons;
	struct input_dev *input = platform_get_drvdata(pdev);
	int	i;
	int	state;
	
	while(!kthread_should_stop())
	{
		for(i=0; i<pdata->nbuttons; i++)
		{
			state = (gpio_get_value(buttons[i].gpio) ? 1 : 0) ^ (buttons[i].active_low);
			if(test_bit(buttons[i].keycode, input->key) != state)
			{
				input_report_key(input, buttons[i].keycode, state);
				input_sync(input);
			}
			
		}
		msleep(62);	// about 1/16
	}
	return 0;
}

static int __devinit helper2416_keys_probe(struct platform_device *pdev)
{
	struct gpio_keys_platform_data *pdata = pdev->dev.platform_data;
	struct input_dev *input;
	int i, error;

	input = input_allocate_device();
	if (!input)
		return -ENOMEM;

	platform_set_drvdata(pdev, input);

	input->evbit[0] = BIT(EV_KEY);

	input->name = pdev->name;
	input->phys = "helper2416-keys/input0";
	input->cdev.dev = &pdev->dev;
	input->private = pdata;

	input->id.bustype = BUS_HOST;
	input->id.vendor = 0x0001;
	input->id.product = 0x0001;
	input->id.version = 0x0100;

	for (i = 0; i < pdata->nbuttons; i++) {
		int code = pdata->buttons[i].keycode;
		s3c2410_gpio_cfgpin(pdata->buttons[i].gpio, pdata->buttons[i].mode);
		s3c2410_gpio_pullup(pdata->buttons[i].gpio, 2);
		set_bit(code, input->keybit);
	}

	error = input_register_device(input);
	if (error) {
		printk(KERN_ERR "Unable to register gpio-keys input device\n");
		goto fail;
	}
	helper2416_keys_threadp = kthread_run(helper2416_keys_thread, pdev, "helper2416_keypad_thread");

	return 0;

 fail:
	input_free_device(input);

	return error;
}

static int __devexit helper2416_keys_remove(struct platform_device *pdev)
{
	struct input_dev *input = platform_get_drvdata(pdev);

	input_unregister_device(input);
	kthread_stop(helper2416_keys_threadp);
	return 0;
}

struct platform_driver helper2416_keys_device_driver = {
	.probe		= helper2416_keys_probe,
	.remove		= __devexit_p(helper2416_keys_remove),
	.driver		= {
		.name	= "helper2416_keypad",
	}
};

static int __init helper2416_keys_init(void)
{
	return platform_driver_register(&helper2416_keys_device_driver);
}

static void __exit helper2416_keys_exit(void)
{
	platform_driver_unregister(&helper2416_keys_device_driver);
}

//module_init(helper2416_keys_init);
subsys_initcall(helper2416_keys_init);
module_exit(helper2416_keys_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("spacexplorer <chenjun@jyxtec.com>");
MODULE_DESCRIPTION("Keypad driver for Helper2416");
