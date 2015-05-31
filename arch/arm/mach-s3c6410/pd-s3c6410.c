#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/list.h>
#include <asm/io.h>
#include <asm/arch/regs-s3c6400-clock.h>
#include <asm/arch/pd.h>
#include "pd-s3c6410.h"

struct pdfw_md pd_md;

static unsigned long pd_get_gate_reg(struct pm_pdtype *pd)
{
	return (unsigned long)S3C_NORMAL_CFG;
}

static unsigned long pd_get_blk_pwr_stat_reg(struct pm_pdtype *pd)
{
	return (unsigned long)S3C_BLK_PWR_STAT;
}

int s3c_pd_enable(struct pm_pdtype *pd)
{
	unsigned long gate_reg;
	unsigned long stable_reg;
	unsigned long flags;
	unsigned long reg;
	unsigned int tmp;

	gate_reg = pd_get_gate_reg(pd);
	stable_reg = pd_get_blk_pwr_stat_reg(pd);
	
	local_irq_save(flags);

	reg = __raw_readl(gate_reg);
	reg |= pd->ctrlbit;
	__raw_writel(reg, gate_reg);

	/* here we have to wait until power stabilizes */
	while (1) {
		tmp = __raw_readl(stable_reg);
		if (tmp && pd->stblregbit) break;
	}

	local_irq_restore(flags);
	pd->state = PDSTATE_ON;

	return 0;
}

int s3c_pd_disable(struct pm_pdtype *pd)
{
	unsigned long gate_reg;
	unsigned long flags;
	unsigned long reg;

	gate_reg = pd_get_gate_reg(pd);
	
	local_irq_save(flags);

	reg = __raw_readl(gate_reg);
	reg &= ~(pd->ctrlbit);
	__raw_writel(reg, gate_reg);

	local_irq_restore(flags);
	pd->state = PDSTATE_OFF;

	return 0;
}

int s3c_pd_state(struct pm_pdtype *pd)
{
	unsigned long gate_reg;
	unsigned long reg;

	gate_reg = pd_get_gate_reg(pd);
	
	reg = __raw_readl(gate_reg);

	if (reg & pd->ctrlbit)
		return PDSTATE_ON;
	else
		return PDSTATE_OFF;
}

struct pm_pdtype domain_etm = {
	name:"domain_etm",
	on: s3c_pd_enable,
	off: s3c_pd_disable,
	get_hwstate: s3c_pd_state,
	ctrlbit: S3C_PWRGATE_DOMAIN_ETM,
	stblregbit: S3C_BLK_ETM,
};

struct pm_pdtype domain_v = {
	name:"domain_v",
	on: s3c_pd_enable,
	off: s3c_pd_disable,
	get_hwstate: s3c_pd_state,
	ctrlbit: S3C_PWRGATE_DOMAIN_V,
	stblregbit: S3C_BLK_V,
};

struct pm_pdtype domain_f = {
	name:"domain_f",
	on: s3c_pd_enable,
	off: s3c_pd_disable,
	get_hwstate: s3c_pd_state,
	ctrlbit: S3C_PWRGATE_DOMAIN_F,
	stblregbit: S3C_BLK_F,
};

struct pm_pdtype domain_p = {
	name:"domain_p",
	on: s3c_pd_enable,
	off: s3c_pd_disable,
	get_hwstate: s3c_pd_state,
	ctrlbit: S3C_PWRGATE_DOMAIN_P,
	stblregbit: S3C_BLK_P,
};

struct pm_pdtype domain_i = {
	name:"domain_i",
	on: s3c_pd_enable,
	off: s3c_pd_disable,
	get_hwstate: s3c_pd_state,
	ctrlbit: S3C_PWRGATE_DOMAIN_I,
	stblregbit: S3C_BLK_I,
};

struct pm_pdtype domain_s = {
	name:"domain_s",
	on: s3c_pd_enable,
	off: s3c_pd_disable,
	get_hwstate: s3c_pd_state,
	ctrlbit: S3C_PWRGATE_DOMAIN_S,
	stblregbit: S3C_BLK_S,
};

void init_pd(struct pm_pdtype *pd)
{
	/* Set initial power state to ON:
	   power is required until driver probe() are done */
	pd->state = PDSTATE_ON;
	INIT_LIST_HEAD(&(pd->entry));
	INIT_LIST_HEAD(&(pd->devhead));
}

/*
 * Initialize power domain structs
 */
void s3c6400_init_powerdomain(void)
{
	init_pd(&domain_etm);
	init_pd(&domain_v);
	init_pd(&domain_f);
	init_pd(&domain_p);
	init_pd(&domain_i);
	init_pd(&domain_s);

	list_add_tail(&domain_etm.entry, &pd_head);
	list_add_tail(&domain_v.entry, &pd_head);
	list_add_tail(&domain_f.entry, &pd_head);
	list_add_tail(&domain_p.entry, &pd_head);
	list_add_tail(&domain_i.entry, &pd_head);
	list_add_tail(&domain_s.entry, &pd_head);
}

void s3c6400_cleanup_powerdomain(void)
{
}

/* solve discrepancies between actual system state
   and what is recorded in the pdfw data structures */	
void s3c6400_sync_powerdomain(void)
{
	struct list_head *t1, *t2;
	struct pm_pdtype *pd;
	struct pm_devtype *pdev;

	list_for_each(t1, &pd_head) {
		pd = list_entry(t1, struct pm_pdtype, entry);
		if (pd->get_hwstate && (pd->state != pd->get_hwstate(pd))) {
			printk(KERN_WARNING "%s:PDSTATE not in sync\n",pd->name);
			pd->state = pd->get_hwstate(pd);
			if (pd->state == PDSTATE_ON) {
				list_for_each(t2, &pd->devhead) {
					pdev = list_entry(t2, struct pm_devtype, entry);
					if (pdev->after_pdon)
						pdev->after_pdon();
				}
			}
		}
	}
}

#ifdef CONFIG_S3C6400_PDFW_PROC
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

static struct proc_dir_entry* s3c6400_pd_proc = NULL;

int s3c6400_pd_write_proc(struct file* file, const char* buffer,
                        unsigned long count, void* data)
{
        char buf[100];
        unsigned int tmp;
        int size = count;

        if (size >= sizeof(buf))
                size = sizeof(buf) - 1;

        memset(buf, 0, sizeof(buf));
        copy_from_user(buf, buffer, size);
        buf[size] = '\0';
        tmp = simple_strtol(buf, NULL, 16);

        __raw_writel(tmp, S3C_NORMAL_CFG);

	s3c6400_sync_powerdomain();

        return count;
}

int s3c6400_pd_read_proc(char* page, char** start, off_t off, int count,
                        int *eof, void *data)
{
        char *i = page;
        unsigned int reg;

        reg = __raw_readl(S3C_NORMAL_CFG);
        i += sprintf(i, "0x%X\n", reg);
        *start = NULL;
        *eof = 1;

        return (i - page);
}
#endif /* CONFIG_S3C6400_PDFW_PROC */

int __init s3c6400_pdfw_init(void)
{
#ifdef CONFIG_S3C6400_PDFW_PROC
        struct proc_dir_entry *tmp;

        s3c6400_pd_proc = proc_mkdir("pd", NULL);
        if (s3c6400_pd_proc != NULL) {
                tmp = create_proc_entry("gatereg", 0, s3c6400_pd_proc);
                if (tmp) {
                        tmp->write_proc = s3c6400_pd_write_proc;
                        tmp->read_proc = s3c6400_pd_read_proc;
                }
        }
#endif
	pd_md.init = s3c6400_init_powerdomain;
	pd_md.cleanup = s3c6400_cleanup_powerdomain;
	pd_md.sync = s3c6400_sync_powerdomain;

	printk(KERN_INFO "s3c6400_pdfw_init done\n");

	return 0;
}

arch_initcall(s3c6400_pdfw_init);

