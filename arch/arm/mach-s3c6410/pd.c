#include <linux/version.h>
#include <linux/module.h>
#if defined(MODVERSIONS)
#include <linux/modversions.h>
#endif
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <asm/arch/pd.h>

LIST_HEAD(pd_head);
/* This is a device-to-powerdomain mapper table for fast searching */
struct pm_pdtype *pd_hashtbl[MAX_PMDEVS];

void init_pd_hashtbl(void)
{
	int i;

	for(i=0; i<MAX_PMDEVS; i++) {
		pd_hashtbl[i] = NULL;
	}
}

int get_free_devid(void)
{
	int i;

	for(i=1; i<MAX_PMDEVS; i++) {
		if (pd_hashtbl[i] == NULL)
			return i;	
	}
	return -EINVAL;
}

/*
 * Register a device with the power domain subsystem
 * struct pm_devtype * dev: device to register
 * const char * pd_name: name of the power domain to register
 */
int pd_register_dev(struct pm_devtype *dev, const char *pd_name)
{
	struct list_head *temp;
	struct pm_pdtype *pd;

	/* search for the power domain that this device belongs to */
	list_for_each(temp, &pd_head) {
		pd = list_entry(temp, struct pm_pdtype, entry);
		if (!strcmp(pd_name, pd->name)) {
			dev->devid = get_free_devid();
			if (dev->devid < 0) {
				printk(KERN_WARNING "Too many devices\n");
				return -EINVAL;
			}
			dev->pd = pd;
			pd_hashtbl[dev->devid] = pd;
			list_add_tail(&dev->entry, &pd->devhead);
			return dev->devid;
		}
	}
	/* We only get here when we can't find this device's power domain */
	printk(KERN_WARNING "can't find this device's pd\n");
	return -EINVAL;
}

int pd_unregister_dev(struct pm_devtype *dev)
{
	pd_hashtbl[dev->devid] = NULL;	// free devid
	dev->devid = 0;
	list_del_init(&dev->entry);
	return 0;
}

int pd_on(struct pm_pdtype *pd)
{
	struct list_head *temp;
	struct pm_devtype *pdev;
	int ret = 0;
	
	/* supply power */
	if (pd->on) {
		ret = pd->on(pd);	// redirect to machine dependent code

		/* resume all devices in this power domain */
		list_for_each(temp, &pd->devhead) {
			pdev = list_entry(temp, struct pm_devtype, entry);
			/* do initialization if needed */
			if (pdev->after_pdon)
				pdev->after_pdon(); 
		}
	}

	/* we assume pd is always on if no pd->on is defined */
	return ret;
}

int pd_off(struct pm_pdtype *pd)
{
	struct list_head *temp;
	struct pm_devtype *pdev;

	if (pd->off) {
		/* suspend all devices in this power domain */
		list_for_each(temp, &pd->devhead) {
			pdev = list_entry(temp, struct pm_devtype, entry);
			/* saving device context */
			if (pdev->before_pdoff)
				pdev->before_pdoff();
		}
		return pd->off(pd);
        }
	/* cannot turn off since no pd->off is defined */
	return -EINVAL;
}

int pd_sync(struct pm_pdtype *pd)
{
	pd_md_sync();
	printk(KERN_INFO "pdfw synchronized\n");

	return 0;
}

int __init pd_init(void)
{
	/* initialize machine dependent data structure */
	init_pd_hashtbl();
	pd_md_init(); 
	pd_md_sync();

	printk(KERN_INFO "Power Domain Framework initialized\n");

	return 0;
}

void __exit pd_exit(void)
{
	pd_md_cleanup();

	return;
}

#ifdef MODULE
module_init(pd_init);
module_exit(pd_exit);
#else
__initcall(pd_init);
#endif

EXPORT_SYMBOL(pd_register_dev);
EXPORT_SYMBOL(pd_unregister_dev);
EXPORT_SYMBOL(pd_on);
EXPORT_SYMBOL(pd_off);
EXPORT_SYMBOL(pd_sync);
EXPORT_SYMBOL(pd_head);
EXPORT_SYMBOL(pd_hashtbl);

MODULE_AUTHOR("Ikhwan Lee");
MODULE_LICENSE("Dual BSD/GPL");

