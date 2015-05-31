#ifndef _LINUX_PDFW_H
#define _LINUX_PDFW_H
#include <linux/list.h>

#define PDSTATE_OFF	0
#define PDSTATE_ON	1

/* device drivers must update their pm_devtype.state to one of below
   values when changing their power state */
#define DEV_IDLE	0
#define DEV_RUNNING	1

/* 30 devices : note that devid start from 1 */
#define MAX_PMDEVS      31	

extern struct list_head pd_head;
extern struct pm_pdtype *pd_hashtbl[MAX_PMDEVS];

/* device struct for devices under a power domain */
struct pm_devtype {
	struct list_head entry;
	//struct device *dev;
	struct pm_pdtype *pd;
	const char *name;
	unsigned int devid;
	int state;
	int (*before_pdoff)(void);
	int (*after_pdon)(void);
};

/* power domain struct */
struct pm_pdtype {
	struct list_head entry;
	struct list_head devhead;
	const char *name;
	int state;
	unsigned long ctrlbit;
	unsigned long stblregbit;
	int (*on)(struct pm_pdtype*);
	int (*off)(struct pm_pdtype*);
	int (*get_hwstate)(struct pm_pdtype*);
};

/* struct for machine-dependent functions */
struct pdfw_md {
        void (*init)(void);
        void (*cleanup)(void);
        void (*sync)(void);
};

extern int pd_register_dev(struct pm_devtype * dev, const char * name);
extern int pd_unregister_dev(struct pm_devtype * dev);
extern int pd_on(struct pm_pdtype *pd);
extern int pd_off(struct pm_pdtype *pd);
extern int pd_sync(struct pm_pdtype *pd);

/* machine-dependent function wrappers */
extern struct pdfw_md pd_md;

static inline void pd_md_init(void)
{
	if (pd_md.init)
		pd_md.init();
}

static inline void pd_md_cleanup(void)
{
	if (pd_md.cleanup)
		pd_md.cleanup();
}

static inline void pd_md_sync(void)
{
	if (pd_md.sync)
		pd_md.sync();
}
#endif // _LINUX_PDFW_H

