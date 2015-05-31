#ifndef _LINUX_KDPMD_H
#define _LINUX_KDPMD_H

/* define wakeup sources for kdpmd */
#define KDPMD_TIMEOUT	0
#define KDPMD_DRVOPEN	1
#define KDPMD_DRVCLOSE	2
#define KDPMD_REMOVE	0xABCDABCD

#include <linux/wait.h>
#include <linux/errno.h>

extern struct semaphore kdpmd_sem;
extern void kdpmd_set_event(unsigned int, unsigned int);
extern void kdpmd_wakeup(void);
extern int kdpmd_wait(unsigned int);

extern inline void kdpmd_lock(void)
{
	down(&kdpmd_sem);
}

extern inline int kdpmd_lock_interruptible(void)
{
	if (down_interruptible(&kdpmd_sem))
		return -ERESTARTSYS;
	return 0;
}

extern inline int kdpmd_trylock(void)
{
	if (down_trylock(&kdpmd_sem))
		return -EBUSY;
	return 0;
}

extern inline void kdpmd_unlock(void)
{
	/* Check: do we need to resynchronize? */
	up(&kdpmd_sem);
}

#endif // _LINUX_KDPMD_H

