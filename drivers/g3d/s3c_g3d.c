
/*
 * linux/drivers/video/s3c_g3d.c
 *
 * Revision 1.0  
 *
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    S3C PostProcessor driver 
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/errno.h> /* error codes */
#include <asm/div64.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <asm/arch/map.h>
#include <linux/miscdevice.h>

#include <linux/version.h>
//#include <linux/config.h>
#include <asm/dma.h>

#include <asm/mach/dma.h>
#include <asm/plat-s3c24xx/dma.h>
#include <linux/dma-mapping.h>

typedef struct{
	unsigned int pool_buffer_addr;
	unsigned int pool_buffer_size;
	unsigned int dma_buffer_addr;
	unsigned int dma_buffer_size;
}G3D_CONFIG_STRUCT;

typedef struct{
	unsigned int offset; // should be word aligned
	unsigned int size; // byte size, should be word aligned
}DMA_BLOCK_STRUCT;

typedef struct {
	ulong src;
	ulong dst;
	int len;
} s3c_3d_dma_info;

#define FGGB_PIPESTATE 0x00
#define FGGB_CACHECTL 0x04
#define FGGB_RST 0x8
#define FGGB_VERSION 0x10
#define FGGB_INTPENDING 0x40
#define FGGB_INTMASK 0x44
#define FGGB_PIPEMASK 0x48
#define FGGB_HOSTINTERFACE 0xc000

G3D_CONFIG_STRUCT g3d_config={
	0x56000000, // pool buffer addr
	0x1800000, //pool buffer size,24Mb
	0x57800000, //dma buffer addr
	0x800000 //dma buffer size
};

#define G3D_IOCTL_MAGIC 'S'
#define WAIT_FOR_FLUSH		 _IO(G3D_IOCTL_MAGIC, 100)
#define GET_CONFIG _IO(G3D_IOCTL_MAGIC, 101)
#define START_DMA_BLOCK _IO(G3D_IOCTL_MAGIC, 102)

#define PFX "s3c_g3d"
#define G3D_MINOR  				249	

static wait_queue_head_t waitq;
static struct resource *s3c_g3d_mem;
static void __iomem *s3c_g3d_base;
static int s3c_g3d_irq;
static struct clk *g3d_clock;
static struct clk *h_clk;

#define DMACH_3D_IN			34
void *dma_3d_done;
static struct s3c2410_dma_client s3c6410_3d_dma_client = {
	.name		= "s3c6410-3d-dma",
};



int interrupt_already_recevied;

unsigned int s3c_g3d_base_physical;

irqreturn_t s3c_g3d_isr(int irq, void *dev_id,
		struct pt_regs *regs)
{
	u32 mode;
	u32 pending;
	
	//printk("s3c_g3d interrupt ~~~~\n");
	//printk("s3c_g3d_isr interrupt !!!!!!!!\n");
	
//	mode = __raw_readl(s3c_g3d_base + S3C_VG3D_MODE);
//	mode &= ~(1 << 6);			/* Clear Source in POST Processor */
//	__raw_writel(mode, s3c_g3d_base + S3C_VG3D_MODE);

	pending = __raw_readl(s3c_g3d_base + FGGB_INTPENDING);
	__raw_writel(mode, s3c_g3d_base + FGGB_INTPENDING);
	
	if(pending>0){
		
	}
	
	interrupt_already_recevied = 1;
	wake_up_interruptible(&waitq);
	
	return IRQ_HANDLED;
}


void s3c_g3d_dma_finish(struct s3c2410_dma_chan *dma_ch, void *buf_id,
	int size, enum s3c2410_dma_buffresult result){
//	printk("3d dma transfer completed.\n");		
	complete(dma_3d_done);
}

int s3c_g3d_open(struct inode *inode, struct file *file) 
{ 	

	return 0;
}

int s3c_g3d_read(struct file *file, char *buf, size_t count,
		loff_t *f_pos)
{
	return 0;
}

int s3c_g3d_write(struct file *file, const char *buf, size_t 
		count, loff_t *f_pos)
{
	return 0;
}

static int s3c_g3d_ioctl(struct inode *inode, struct file *file,
		unsigned int cmd, unsigned long arg)
{

	u32 val;
	DMA_BLOCK_STRUCT dma_block;
	s3c_3d_dma_info dma_info;
	DECLARE_COMPLETION_ONSTACK(complete);

	switch(cmd) {
		case WAIT_FOR_FLUSH:
			
			//if fifo has already been flushed, return;
			val = __raw_readl(s3c_g3d_base+FGGB_PIPESTATE);
			//printk("read pipestate = 0x%x\n",val);
			if((val & arg) ==0)break;

			// enable interrupt
			interrupt_already_recevied = 0;
			__raw_writel(0x0001171f,s3c_g3d_base+FGGB_PIPEMASK);
			__raw_writel(1,s3c_g3d_base+FGGB_INTMASK);

			//printk("wait for flush (arg=0x%lx)\n",arg);
			

			while(1){
					wait_event_interruptible(waitq, (interrupt_already_recevied>0));
					__raw_writel(0,s3c_g3d_base+FGGB_INTMASK);
					interrupt_already_recevied = 0;
					//if(interrupt_already_recevied==0)interruptible_sleep_on(&waitq);
					val = __raw_readl(s3c_g3d_base+FGGB_PIPESTATE);
					//printk("in while read pipestate = 0x%x\n",val);
					if(val & arg){}
					else{
						 break;
					}
					__raw_writel(1,s3c_g3d_base+FGGB_INTMASK);
			}
			break;

		case GET_CONFIG: 
			copy_to_user((void *)arg,&g3d_config,sizeof(G3D_CONFIG_STRUCT));
			break;

		case START_DMA_BLOCK:
			copy_from_user(&dma_block,(void *)arg,sizeof(DMA_BLOCK_STRUCT));
			if(dma_block.offset%4!=0)
			{	
				printk("G3D: dma offset is not aligned by word\n");
				return -EINVAL;
			}
			if(dma_block.size%4!=0)
			{
				printk("G3D: dma size is not aligned by word\n");
				return -EINVAL;
			}
			if(dma_block.offset+dma_block.size >g3d_config.dma_buffer_size)
			{
				printk("G3D: offset+size exceeds dam buffer\n");
				return -EINVAL;
			}

			dma_info.src = g3d_config.dma_buffer_addr+dma_block.offset;
			dma_info.len = dma_block.size;
			dma_info.dst = s3c_g3d_base_physical+FGGB_HOSTINTERFACE;

		//	printk(" dma src=0x%x\n",dma_info.src);
		//	printk(" dma len =%u\n",dma_info.len);
		//	printk(" dma dst = 0x%x\n",dma_info.dst);
			
			dma_3d_done = &complete;
			
			if (s3c2410_dma_request(DMACH_3D_IN, &s3c6410_3d_dma_client, NULL)) 
			{
				printk(KERN_WARNING "Unable to get DMA channel.\n");
				return -EFAULT;
			}

			s3c2410_dma_set_buffdone_fn(DMACH_3D_IN, s3c_g3d_dma_finish);
			s3c2410_dma_devconfig(DMACH_3D_IN, S3C_DMA_MEM2G3D, 1, (u_long) dma_info.src);
			s3c2410_dma_config(DMACH_3D_IN, 4, 4);
			s3c2410_dma_setflags(DMACH_3D_IN, S3C2410_DMAF_AUTOSTART);
			//consistent_sync((void *) dma_info.dst, dma_info.len, DMA_FROM_DEVICE);
		//	s3c2410_dma_enqueue(DMACH_3D_IN, NULL, (dma_addr_t) virt_to_dma(NULL, dma_info.dst), dma_info.len);
			s3c2410_dma_enqueue(DMACH_3D_IN, NULL, (dma_addr_t) dma_info.dst, dma_info.len);

		//	printk("wait for end of dma operation\n");
			wait_for_completion(&complete);
		//	printk("dma operation is performed\n");

			s3c2410_dma_free(DMACH_3D_IN, &s3c6410_3d_dma_client);
		
			
			break;
		
		default:
			return -EINVAL;
	}
	return 0;
}


static struct file_operations s3c_g3d_fops = {
	.owner 	= THIS_MODULE,
	.ioctl 	= s3c_g3d_ioctl,
	.open 	= s3c_g3d_open,
	.read 	= s3c_g3d_read,
	.write 	= s3c_g3d_write,
};


static struct miscdevice s3c_g3d_dev = {
	.minor		= G3D_MINOR,
	.name		= "s3c-g3d",
	.fops		= &s3c_g3d_fops,
};

static int s3c_g3d_remove(struct platform_device *dev)
{
	//clk_disable(g3d_clock);

	free_irq(s3c_g3d_irq, NULL);
	if (s3c_g3d_mem != NULL) {
		pr_debug("s3c_g3d: releasing s3c_post_mem\n");
		iounmap(s3c_g3d_base);
		release_resource(s3c_g3d_mem);
		kfree(s3c_g3d_mem);
	}
	misc_deregister(&s3c_g3d_dev);
	return 0;
}

int s3c_g3d_probe(struct platform_device *pdev)
{

	struct resource *res;

	int ret;
	int i;

	printk("s3c_g3d probe\n");

	s3c_g3d_irq = platform_get_irq(pdev, 0);


	if(s3c_g3d_irq <= 0) {
		printk(KERN_ERR PFX "failed to get irq resouce\n");
		return -ENOENT;
	}

	/* get the memory region for the post processor driver */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL) {
		printk(KERN_ERR PFX "failed to get memory region resouce\n");
		return -ENOENT;
	}

	s3c_g3d_base_physical = (unsigned int)res->start;
	
	s3c_g3d_mem = request_mem_region(res->start, res->end-res->start+1, pdev->name);
	if(s3c_g3d_mem == NULL) {
		printk(KERN_ERR PFX "failed to reserve memory region\n");
		return -ENOENT;
	}


	s3c_g3d_base = ioremap(s3c_g3d_mem->start, s3c_g3d_mem->end - res->start + 1);
	if(s3c_g3d_base == NULL) {
		printk(KERN_ERR PFX "failed ioremap\n");
		return -ENOENT;
	}

	g3d_clock = clk_get(&pdev->dev, "post");
	if(g3d_clock == NULL) {
		printk(KERN_ERR PFX "failed to find post clock source\n");
		return -ENOENT;
	}

	clk_enable(g3d_clock);

	h_clk = clk_get(&pdev->dev, "hclk");
	if(h_clk == NULL) {
		printk(KERN_ERR PFX "failed to find h_clk clock source\n");
		return -ENOENT;
	}

	init_waitqueue_head(&waitq);

	ret = misc_register(&s3c_g3d_dev);
	if (ret) {
		printk (KERN_ERR "cannot register miscdev on minor=%d (%d)\n",
				G3D_MINOR, ret);
		return ret;
	}
	
	// device reset
	__raw_writel(1,s3c_g3d_base+FGGB_RST);
	for(i=0;i<1000;i++);
	__raw_writel(0,s3c_g3d_base+FGGB_RST);
	for(i=0;i<1000;i++);

	ret = request_irq(s3c_g3d_irq, s3c_g3d_isr, SA_INTERRUPT,
			pdev->name, NULL);
	if (ret) {
		printk("request_irq(PP) failed.\n");
		return ret;
	}
	
	printk("s3c_g3d version : 0x%x\n",__raw_readl(s3c_g3d_base + FGGB_VERSION));

	/* check to see if everything is setup correctly */	
	return 0;
}

static int s3c_g3d_suspend(struct platform_device *dev, pm_message_t state)
{
	//clk_disable(g3d_clock);
	return 0;
}
static int s3c_g3d_resume(struct platform_device *pdev)
{
	//clk_enable(g3d_clock);
	return 0;
}
static struct platform_driver s3c_g3d_driver = {
	.probe          = s3c_g3d_probe,
	.remove         = s3c_g3d_remove,
	.suspend        = s3c_g3d_suspend,
	.resume         = s3c_g3d_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-g3d",
	},
};

static char banner[] __initdata = KERN_INFO "S3C G3D Driver, (c) 2007 Samsung Electronics\n";

int __init  s3c_g3d_init(void)
{

	printk(banner);
	if(platform_driver_register(&s3c_g3d_driver)!=0)
	{
		printk("platform device register Failed \n");
		return -1;
	}

	return 0;
}

void  s3c_g3d_exit(void)
{
	platform_driver_unregister(&s3c_g3d_driver);
	printk("S3C G3D module exit\n");
}
module_init(s3c_g3d_init);
module_exit(s3c_g3d_exit);


MODULE_AUTHOR("highvolt.lee@samsung.com");
MODULE_DESCRIPTION("S3C G3D Device Driver");
MODULE_LICENSE("GPL");


