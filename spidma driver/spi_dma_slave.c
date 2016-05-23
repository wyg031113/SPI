#include <linux/module.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/sound.h>
#include <linux/soundcard.h>

#include <linux/pm.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/dma.h>
#include <asm/arch/dma.h>
#include <asm/arch/regs-gpio.h>
#include <asm/arch/regs-clock.h>
#include <linux/dma-mapping.h>
#include <asm/dma-mapping.h>
#include <asm/arch/hardware.h>
#include <asm/arch/map.h>

#include <linux/platform_device.h>
#include <asm/arch/regs-spi.h>
#include "spi_dma_slave.h"
#include "circular_buf.h"

#define MAX_DMA_CHANNELS 0

/* The S3C2410 has four internal DMA channels. */

#define MAX_S3C2410_DMA_CHANNELS S3C2410_DMA_CHANNELS

#define DMA_CH0 0
#define DMA_CH1 1
#define DMA_CH2 2
#define DMA_CH3 3
#define DMA_CH DMA_CH3

#define DMA_BUF_WR 1
#define DMA_BUF_RD 0

#define SPTDAT0_ADDR 0x59000010 //SPI0 Tx Data Register
#define SPRDAT0_ADDR 0x59000014 //SPI0 Rx Data Register
#define SPTDAT1_ADDR 0x59000030 //SPI1 Tx Data Register
#define SPRDAT1_ADDR 0x59000034 //SPI1 Rx Data Register
#define dma_wrreg(chan, reg, val) writel((val), (chan)->regs + (reg))

//循环缓冲队列
#define RTS_PIN S3C2410_GPC4//VM
#define CTS_PIN S3C2410_GPG8//EINT16
#define PIN_HIGH 1
#define PIN_LOW 0

#define SPI1_BUFSIZE (16384)
#define SEND_SPI_MAXSIZE (512)

#define OK_BUFLEN (32)//(64)
#define IC_BUFLEN (320)//(512)
#define DMA_MAXSIZE (4800)//(5120)

static struct circular_buffer readbuffer,writebuffer;
static int spi1_status;
static struct s3c2410_dma_client s3c2410spi_dma= {
.name = "SPI1",
};

static int spi1_eint_irq=IRQ_EINT16;
static int spi1_major=220;
//static  wait_queue_head_t	spi_wait;

#define SPI_NAME "spi1_dma"
#define SPI_NAME_VERBOSE "SPI1 DMA driver"

//#define DEBUG
#ifdef DEBUG
#define DPRINTK printk
#else
#define DPRINTK( x... )
#endif

typedef struct {
	int size; /* buffer size */
	char *start; /* point to actual buffer */
	dma_addr_t dma_addr; /* physical buffer address */
	struct semaphore sem; /* down before touching the buffer */
	int master; /* owner for buffer allocation, contain size when true */
} spi_buf_t;

typedef struct {
	spi_buf_t *buffers; /* pointer to spi buffer structures */
	spi_buf_t *buf; /* current buffer used by read/write */
	u_int buf_idx; /* index for the pointer above */
	u_int fragsize; /* fragment i.e. buffer size */
	u_int nbfrags; /* nbr of fragments */
	dmach_t dma_ch; /* DMA channel (channel1,3 for spi) */
	u_int dma_ok;
} spi_stream_t;

static spi_stream_t spi_stream; /* input */
static int spi_rd_refcount = 0;
static int spi_wr_refcount = 0;
#define spi_active (spi_rd_refcount|spi_wr_refcount)

static	spinlock_t		spi_lock=SPIN_LOCK_UNLOCKED;
//static unsigned long spi_irq_flag;

void spi1_slave_poll(void)
{
	//poll,dis-SCK,slave,high,A,normal
	__raw_writel(0, S3C2410_SPCON1);
}
void spi1_slave_receive(void)
{
	//DMA3,dis-SCK,slave,high,A,TAGD
	__raw_writel((S3C2410_SPCON_SMOD_DMA|S3C2410_SPCON_TAGD), S3C2410_SPCON1);
}
void spi1_slave_send(void)
{
	//DMA3,dis-SCK,slave,high,A,TAGD
	__raw_writel((S3C2410_SPCON_SMOD_DMA), S3C2410_SPCON1);
}

void init_slave_spi1(void)
{
	__raw_writel(0, S3C2410_SPPRE1);
	spi1_slave_receive();
	//dis-ENMUL,SBO,release
	__raw_writel(S3C2410_SPPIN_RESERVED, S3C2410_SPPIN1);

}
static void spi_clear_buf(spi_stream_t * s)
{
	DPRINTK("spi_clear_buf\n");
	
//	if(s->dma_ok){
//		s3c2410_dma_set_buffdone_fn(s->dma_ch, NULL);
//		s3c2410_dma_ctrl(s->dma_ch, S3C2410_DMAOP_FLUSH);
//		s3c2410_dma_set_buffdone_fn(s->dma_ch, spi_dma_done_callback);
//	}
//	
//	if (s->buffers) {
//		dma_free_coherent(NULL,	s->buffers[0].master,s->buffers[0].start,s->buffers[0].dma_addr);
//		kfree(s->buffers);
//		s->buffers = NULL;
//	}
//	
//	s->buf_idx = 0;
//	s->buf = NULL;
}

static int spi_setup_buf(spi_stream_t * s)
{
	int dmasize = 0;
	char *dmabuf = 0;
	dma_addr_t dmaphys = 0;
	spi_buf_t *b;
	
	if (s->buffers)
		return -EBUSY;
	//yangxing	
	s->nbfrags = 1;
	s->fragsize = DMA_MAXSIZE;
	
	s->buffers = (spi_buf_t *)kmalloc(sizeof(spi_buf_t) * s->nbfrags, GFP_KERNEL);
	if (!s->buffers)
		goto err;
	memset(s->buffers, 0, sizeof(spi_buf_t) * s->nbfrags);
	
	b = &s->buffers[0];	
	if (!dmasize) {
		dmasize = (s->nbfrags - 0) * s->fragsize;
		do {
			dmabuf = dma_alloc_coherent(NULL, (dmasize + 64), &dmaphys, GFP_KERNEL|GFP_DMA);
			if (!dmabuf)
				dmasize -= s->fragsize;
		} while (!dmabuf && dmasize);
		if (!dmabuf)
			goto err;
		b->master = dmasize;
		b->size = dmasize;//yangxing
	}
	
	b->start = dmabuf;
	b->dma_addr = dmaphys;
	DPRINTK("buf : start %p dma 0x%x dmasize=0x%x\n",  b->start, b->dma_addr, b->master);
		
	s->buf_idx = 0;
	s->buf = &s->buffers[0];
	return 0;
	
err:
	printk(SPI_NAME ": unable to allocate spi memory\n ");
	spi_clear_buf(s);
	return -ENOMEM;
}

static void spi_dma_done_callback(s3c2410_dma_chan_t *ch, void *buf, int size, s3c2410_dma_buffresult_t result)
{
	int i;		
	spi_buf_t *b = (spi_buf_t *) buf;
    s3c2410_dma_setflags(DMA_CH, S3C2410_DMAF_AUTOSTART);
	switch(spi1_status){
	case SPI_SEND_STATUS:
		s3c2410_gpio_setpin(RTS_PIN,PIN_HIGH);
		udelay(5);
		spi1_slave_receive();
		spi1_status = SPI_RECEIVE_STATUS;
		s3c2410_dma_devconfig(DMA_CH, S3C2410_DMASRC_HW, 3, SPRDAT1_ADDR);
		b->size =OK_BUFLEN;
		break;
	case SPI_NONE_STATUS:
		break;
	case SPI_RECEIVE_STATUS:
	default:
		spi1_slave_poll();
		while(!(__raw_readl(S3C2410_SPSTA1)&0x1));
		*(b->start + DMA_MAXSIZE)=__raw_readl(S3C2410_SPRDAT1);
		i = write_circular_buf(&readbuffer,(b->start + 1),DMA_MAXSIZE);
		spi1_slave_receive();
		spi1_status = SPI_RECEIVE_STATUS;
		s3c2410_dma_devconfig(DMA_CH, S3C2410_DMASRC_HW, 3, SPRDAT1_ADDR);
		b->size = DMA_MAXSIZE;
		break;
	}
}

//外部中断
static irqreturn_t spi1_eint_irq_handler(int irq, void *id, struct pt_regs *r)
{
	spi_stream_t *s = &spi_stream;
	spi_buf_t *b = s->buf;
	int ret;
//	unsigned long flags;
	
	DPRINTK("spi1_irq: EINT interrupt: \n");
	
	ret=read_circular_buf(&writebuffer,b->start,SEND_SPI_MAXSIZE);	
	if( ret ==0 ){
		return IRQ_HANDLED;
	}
	else if(ret > DMA_MAXSIZE) {
		ret = DMA_MAXSIZE - 1;
	}
	b->size = ret;

	s3c2410_gpio_setpin(RTS_PIN,PIN_LOW);
	spi1_status = SPI_NONE_STATUS;
	s3c2410_dma_ctrl(s->dma_ch, S3C2410_DMAOP_FLUSH);
	spi1_status = SPI_SEND_STATUS;;
	spi1_slave_send();
	
	s3c2410_dma_devconfig(s->dma_ch, S3C2410_DMASRC_MEM, 3, SPTDAT1_ADDR);//发送	
	s3c2410_dma_config(s->dma_ch, 1, (0xa2c00000|b->size));
	s3c2410_dma_set_buffdone_fn(s->dma_ch, spi_dma_done_callback);
	s3c2410_dma_setflags(s->dma_ch, S3C2410_DMAF_AUTOSTART);
       
	s3c2410_dma_enqueue(s->dma_ch, (void *) b, b->dma_addr, b->size);	
	b->size = 0;
	return IRQ_HANDLED;
}

static ssize_t smdk2410_spi_write(struct file *file, const char __user *buf,	
				size_t count, loff_t *ppos)
{
	int length = 0;
	unsigned long flags;
	
	spin_lock_irqsave(&spi_lock,flags);	
	length = write_to_circular_buf(&writebuffer, buf, count);
	spin_unlock_irqrestore(&spi_lock,flags);	
	
	s3c2410_gpio_setpin(RTS_PIN,PIN_LOW);

	DPRINTK("spi_write : end count=%d\n", length);
	return length;
}


static ssize_t smdk2410_spi_read(struct file *file, char __user *buf,	size_t count, loff_t *ppos)
{
//	spi_stream_t *s = &spi_stream;
	int ret = 0;

	if(count){
		ret=read_from_circular_buf(&readbuffer,buf,count);
		if(ret)
			DPRINTK("spi_read: return=%d\n", ret);
	}	
//	DPRINTK("spi_read: return=%d\n", ret);

	return ret;
}

static loff_t smdk2410_spi_llseek(struct file *file, loff_t offset,int origin)
{
	return -ESPIPE;
}

static int smdk2410_spi_ioctl(struct inode *inode, struct file *file,uint cmd, ulong arg)
{
	spi_stream_t *s = &spi_stream;
	spi_buf_t *b = s->buf;
	int i;
	unsigned long flags;
	
	switch (cmd) {
	case CMD_SPI_CANCEL:
		spi_stream.dma_ch = DMA_CH;		
		b->size = DMA_MAXSIZE;
		spin_lock_irqsave(&spi_lock,flags);			
		s3c2410_gpio_setpin(RTS_PIN,PIN_HIGH);		
		spi1_status = SPI_NONE_STATUS;
		s3c2410_dma_ctrl(s->dma_ch, S3C2410_DMAOP_FLUSH);
		spi1_status = SPI_RECEIVE_STATUS;;
		clear_circular_buf(&writebuffer);
		clear_circular_buf(&readbuffer);
		spi1_slave_receive();
		s3c2410_dma_devconfig(s->dma_ch, S3C2410_DMASRC_MEM, 3, SPTDAT1_ADDR);
		s3c2410_dma_config(s->dma_ch, 1, (0xa2c00000|b->size));
		s3c2410_dma_set_buffdone_fn(s->dma_ch, spi_dma_done_callback);
		s3c2410_dma_setflags(s->dma_ch, S3C2410_DMAF_AUTOSTART);			
		s3c2410_dma_enqueue(s->dma_ch, (void *) b, b->dma_addr, b->size);	
		b->size = 0;		
		spin_unlock_irqrestore(&spi_lock,flags);
		break;
	case CMD_SPI_RESET:
		{
			i = arg;
			if(i > 0){
				i = __raw_readl(S3C24XX_MISCCR);
				__raw_writel((i|S3C2410_MISCCR_nRSTCON), S3C24XX_MISCCR);
			}
			else{
				__raw_writel((i&(~S3C2410_MISCCR_nRSTCON)), S3C24XX_MISCCR);
			}
		}
		break;
	default:
		break;
	}
	
	return 0;
}


static int smdk2410_spi_open(struct inode *inode, struct file *file)
{
	int cold = !spi_active;
	
	DPRINTK("spi_open\n");
	if ((file->f_flags & O_ACCMODE) == O_RDWR) {
		if (spi_rd_refcount || spi_wr_refcount)
			return -EBUSY;
		spi_rd_refcount++;
		spi_wr_refcount++;
	} else
		return -EINVAL;
	
	if (cold) {		
	    if(request_irq(spi1_eint_irq, &spi1_eint_irq_handler, SA_TRIGGER_FALLING, "spi1_eint16",&spi1_eint_irq_handler))
		{
			DPRINTK("request irq %d: failed\n",spi1_eint_irq);
			return -EAGAIN;
		}
		s3c2410_gpio_setpin(RTS_PIN,PIN_HIGH);
		init_slave_spi1();
		spi1_status = SPI_RECEIVE_STATUS;
		clear_circular_buf(&writebuffer);
		clear_circular_buf(&readbuffer);
			
	}
	return 0;
}


static int smdk2410_spi_release(struct inode *inode, struct file *file)
{
	DPRINTK("spi_release\n");
	
	if (file->f_mode) {
		if (spi_rd_refcount == 1){
			s3c2410_gpio_setpin(RTS_PIN,PIN_HIGH);
			free_irq(spi1_eint_irq,&spi1_eint_irq_handler);
			spi_clear_buf(&spi_stream);
		}
		spi_rd_refcount = 0;
		spi_wr_refcount = 0;
	}
	
	return 0;
}


static struct file_operations smdk2410_spi_fops = {
	.owner		=THIS_MODULE,
	.open		=smdk2410_spi_open,
	.release	=smdk2410_spi_release,
	.read		=smdk2410_spi_read,
	.write		=smdk2410_spi_write,
	.ioctl		=smdk2410_spi_ioctl,
	.llseek		=smdk2410_spi_llseek,
};

static void init_spi1(void)
{
	
	s3c2410_gpio_cfgpin(S3C2410_GPG5,S3C2410_GPIO_SFN3);
	s3c2410_gpio_cfgpin(S3C2410_GPG6,S3C2410_GPIO_SFN3);
	s3c2410_gpio_cfgpin(S3C2410_GPG7,S3C2410_GPIO_SFN3);
	s3c2410_gpio_pullup(S3C2410_GPG5, 0);
	s3c2410_gpio_pullup(S3C2410_GPG6, 0);
	s3c2410_gpio_pullup(S3C2410_GPG7, 0);	
	s3c2410_gpio_cfgpin(S3C2410_GPG3,S3C2410_GPIO_SFN3);//nSS1
			
			
	s3c2410_gpio_cfgpin(RTS_PIN,S3C2410_GPIO_OUTPUT);
	s3c2410_gpio_setpin(RTS_PIN,PIN_HIGH);
	
}

static int __init spi_init_dma(spi_stream_t * s, char *desc)
{
    int ret ;
    s3c2410_dmasrc_t source;
    int hwcfg;
    unsigned long devaddr;
    dmach_t channel;
    int dcon;
    unsigned int flags = 0;
	int len = DMA_MAXSIZE;
	 
    if(s->dma_ch == DMA_CH){
       channel = 3;
       source = S3C2410_DMASRC_HW;
       hwcfg = 3;
       devaddr = SPRDAT1_ADDR;
       dcon = (1<<31)|(0<<30)|(1<<29)|(0<<28)|(0<<27)|(2<<24)|(1<<23)|(1<<22)|(0<<20)|len;//0xa0800000;
       flags = S3C2410_DMAF_AUTOSTART;

       s3c2410_dma_devconfig(channel, source, hwcfg, devaddr);
       
       s3c2410_dma_config(channel, 1, dcon);
       s3c2410_dma_set_buffdone_fn(channel, spi_dma_done_callback);
       s3c2410_dma_setflags(channel, flags);
       ret = s3c2410_dma_request(s->dma_ch, &s3c2410spi_dma, NULL);
       s->dma_ok = 1;
       return ret;
    }
    else if(s->dma_ch == DMA_CH1){
       channel =1;
       source =S3C2410_DMASRC_HW;
       hwcfg =3;
       devaddr = SPRDAT0_ADDR;
       dcon = (1<<31)|(0<<30)|(1<<29)|(0<<28)|(0<<27)|(2<<24)|(1<<23)|(1<<22)|(0<<20)|len;//0xa2900000;
       flags = S3C2410_DMAF_AUTOSTART;

       s3c2410_dma_devconfig(channel, source, hwcfg, devaddr);
       s3c2410_dma_config(channel, 1, dcon);
       s3c2410_dma_set_buffdone_fn(channel, spi_dma_done_callback);
       s3c2410_dma_setflags(channel, flags);
       ret = s3c2410_dma_request(s->dma_ch, &s3c2410spi_dma, NULL);
       s->dma_ok =1;
       return ret ;
    }
	else
		return 1;
}

static int spi_clear_dma(spi_stream_t * s,s3c2410_dma_client_t *client)
{
    s3c2410_dma_set_buffdone_fn(s->dma_ch, NULL);
    s3c2410_dma_free(s->dma_ch, client);
	return 0;
}
static int s3c2410spi_probe(struct platform_device *dev) 
{
	spi_stream_t *s;
	spi_buf_t *b;

	register_chrdev(spi1_major,"okolle_spi1",&smdk2410_spi_fops);
//
	if (spi_setup_buf(&spi_stream))
		return -ENOMEM;
    spi_stream.dma_ch = DMA_CH;
    if (spi_init_dma(&spi_stream, "SPI DMA")) {
        spi_clear_dma(&spi_stream,&s3c2410spi_dma);
        printk( KERN_WARNING SPI_NAME_VERBOSE": unable to get DMA channels\n" );
        return -EBUSY;
    }
    
	s = &spi_stream;
	b = s->buf;
	s3c2410_dma_enqueue(s->dma_ch, (void *) b, b->dma_addr, b->size);	
	b->size = 0;
    printk(SPI_NAME_VERBOSE " initialized\n");

    return 0;
}


static int s3c2410spi_remove(struct platform_device *dev) 
{
	unsigned long flags;
	spin_lock_irqsave(&spi_lock, flags);	
	spi_clear_dma(&spi_stream,&s3c2410spi_dma);
	spin_unlock_irqrestore(&spi_lock, flags);
	printk(SPI_NAME_VERBOSE " unloaded\n");
    return 0;
}
static int s3c2410spi_suspend(struct platform_device *pdev, pm_message_t  state)
{
	DPRINTK("spi_suspend\n");
	return 0;
}

static int s3c2410spi_resume(struct platform_device *pdev)
{
	DPRINTK("spi_resume\n");
	return 0;
}
static struct platform_driver s3c2410spi_driver = {
    .probe = s3c2410spi_probe,
    .remove = s3c2410spi_remove,
	.suspend	= s3c2410spi_suspend,
	.resume		= s3c2410spi_resume,
	.driver		= {
		.name	= "s3c2410-spi1",
		.owner	= THIS_MODULE,
	},
};

static int __init s3c2410_spi_init(void) {
	
	init_spi1();
	init_slave_spi1();
	
	init_circular_buf(&writebuffer,SPI1_BUFSIZE);
	init_circular_buf(&readbuffer,SPI1_BUFSIZE);
	
	spi1_status = SPI_RECEIVE_STATUS;
	
	memzero(&spi_stream, sizeof(spi_stream_t));
	DPRINTK("okolle_spi1 registered\n");
    return platform_driver_register(&s3c2410spi_driver);
}

static void __exit s3c2410_spi_exit(void) {
	platform_driver_unregister(&s3c2410spi_driver);
}


module_init(s3c2410_spi_init);
module_exit(s3c2410_spi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yangxing");
MODULE_DESCRIPTION("S3C2410 spi + dma driver");

