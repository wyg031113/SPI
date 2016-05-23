#include <linux/interrupt.h>//request_irq 

#include <linux/miscdevice.h> 
#include <linux/delay.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/init.h> 
#include <linux/fs.h> 
#include <linux/slab.h> 
#include <linux/pci.h> 
#include <asm/uaccess.h> 
#include <linux/ioctl.h> 
#include <mach/irqs.h> 
#include <mach/regs-irq.h> 
#include <linux/sched.h> 
#define MASTER

#define DEVICE_NAME "tiny210_spi" 

static void __iomem *base_addr0; 
static void __iomem *base_addr1; 
static void __iomem *base_addr2; 
static void __iomem *base_addr3; 
//static void __iomem *base_addr3; 
//static void __iomem *base_addr4; 
static void __iomem *base_addr5; 

#define s5pv210_CLK 0xE010046C 
#define s5pv210_GPB 0xE0200040 
#define s5pv210_SPI 0xE1300000 
/*#define s5pv210_CLK 0x1003C950
#define s5pv210_GPB 0x11400040 
#define s5pv210_SPI 0x13920000
#define s5pv210_CLK_DIV_ISP 0x1003c538
#define s5pv210_CLK_DIV_PERIL1 0x1003c554*/
#define s5pv210_CLK_SRC_MASK0 0xE0100000
#define s5pv210_CLK_DIV5 0xE0100314
/*****************************************************/ 
//#define S3C2440_CLKCON 部分 
#define CLK_GATE_IP3 (*(volatile unsigned int *)(base_addr0 + 0x00)) 
#define CLK_SRC_MASK0 (*(volatile unsigned int *)(base_addr5 + 0x0280)) 
#define CLK_DIV5 (*(volatile unsigned int *)(base_addr3)) 
/*#define CLK_GATE_IP_PERIL (*(volatile unsigned long *)(base_addr0 + 0x00)) 
#define CLK_DIV_ISP (*(volatile unsigned long *)(base_addr3 + 0x00)) 
#define CLK_DIV_PERIL1 (*(volatile unsigned long *)(base_addr4 + 0x00)) 
*/
//GPG 控制寄存器部分 GPB 
#define GPBCON (*(volatile unsigned int *)(base_addr1 + 0x00)) 
#define GPBDAT (*(volatile unsigned int *)(base_addr1 + 0x04)) 
#define GPBUP (*(volatile unsigned int *)(base_addr1 + 0x08)) 

//SPI 控制寄存器部分 
#define CH_CFG0 (*(volatile unsigned int *)(base_addr2 + 0x00)) 
#define CLK_CFG0 (*(volatile unsigned int *)(base_addr2 + 0x04)) 
#define MODE_CFG0 (*(volatile unsigned int *)(base_addr2 + 0x08)) 
#define CS_REG0 (*(volatile unsigned int *)(base_addr2 + 0x0c)) 
#define SPI_INT_EN0 (*(volatile unsigned int *)(base_addr2 + 0x10)) 
#define SPI_STATUS0 (*(volatile unsigned int *)(base_addr2 + 0x14)) 
#define SPI_TX_DATA0 (*(volatile unsigned int *)(base_addr2 + 0x18)) 
#define SPI_RX_DATA0 (*(volatile unsigned int *)(base_addr2 + 0x1c)) 
#define PACKET_CNT_REG0 (*(volatile unsigned int *)(base_addr2 + 0x20)) 
#define PENDING_CLR_REG0 (*(volatile unsigned int *)(base_addr2 + 0x24)) 
#define SWAP_CONFIG0 (*(volatile unsigned int *)(base_addr2 + 0x28)) 
#define FD_CLK_SEL0 (*(volatile unsigned int *)(base_addr2 + 0x2c)) 

#define SPI_TXRX_ON() (CH_CFG0 |= 0x3) 
#define SPI_TX_DONE (((SPI_STATUS >> 21) & 0x1) == 0x1) 
#define SPI_RX_num ((SPI_STATUS >>13) & 127) 

#define SPI_CS_LOW() (SLAVE_SEL &= ~0x01) 
#define SPI_CS_HIGH() (SLAVE_SEL |= 0x01) 
#define CHKBIT(d,n) (((d)&(1<<n))!=0)
#define GETDATA(d,s,len) ((d>>s) & ((1<<len)-1))
char buff[250]; 
char total_buf[1000]; 
volatile long data_num = 0; 
void show_status(void)
{
	printk("TX_Done:%d TRAILING_BYTE_IS_ZERO:%d  RX_LVL:%d  TX_LEL:%d RX_OVERRUN:%d RX_UNDERRUN:%d  TX_OVERRUN:%d TX_UNDERRUN:%d 	RX_RDY:%d TX_RDY:%d\n", 
			CHKBIT(SPI_STATUS0, 25), CHKBIT(SPI_STATUS0, 24),
		    GETDATA(SPI_STATUS0, 15, 9), GETDATA(SPI_STATUS0, 6, 9),
			CHKBIT(SPI_STATUS0, 5),CHKBIT(SPI_STATUS0, 4),
			CHKBIT(SPI_STATUS0, 3),CHKBIT(SPI_STATUS0, 2),
			CHKBIT(SPI_STATUS0, 1),CHKBIT(SPI_STATUS0, 0)
			);
}
void show_config(void)
{
	printk("CH_CFG:%x CLK_CFG:%x MOD_CFG:%x CS_REG:%x INT:%x PACKET_CNT:%x SWAP:%x FD_CLK_SEL:%x\n",
			CH_CFG0, CLK_CFG0, MODE_CFG0, CS_REG0, SPI_INT_EN0, PACKET_CNT_REG0, SWAP_CONFIG0, FD_CLK_SEL0);
	show_status();
}
#define HAVE_ERROR(s) (((s)&0x3c)!=0)
static int reg_init();
static int spi_open(struct inode *inode, struct file *filp) 
{
	reg_init();	
	return 0;
}
static int reg_init()
{ 
printk("Begin init...\n");
//CLK_SRC_MASK0 &= ~(1<<16);
//CLK_SRC_MASK0 |= (1<<16);
show_config();
#ifdef MASTER
CLK_GATE_IP3 |= (1 << 12); 
#else
CLK_GATE_IP3 |= (1 << 12); 
//CLK_DIV5 |= 0xf;
//CLK_GATE_IP3 &= ~(1 << 12); 
#endif
//CLK_DIV_ISP &= 0xFFFF000F;
//CLK_DIV_ISP |= 0x0000FFF0;
//CLK_DIV_PERIL1 &= 0xFFFF00F0;
//CLK_DIV_PERIL1 |= 0x0000FF0F;
//CLK_GATE_IP_PERIL |= 1 << 16; 
//设置GPIO 
GPBCON &= 0XFFFF0000; 
GPBCON |= 0X00002222; 
//GPEUP 设置;全部disable 
GPBUP &= ~(0xff << 0); 
//GPBDAT 设置位0 
GPBDAT &= ~(0xff << 0); 

//reset SPI status
CH_CFG0 |= (1 << 5);
CH_CFG0 &= ~(1 << 5);
 //1.set transfer type (CPOL & CPHA)
PENDING_CLR_REG0 = 0x1F;
//CPHA = 1;CPOL=1;为从模式 
CH_CFG0 &= 0;
CH_CFG0 = ((0 << 4) | (0 << 3) | (0 << 2)); 
//2. set Feedback Clock Selection register
FD_CLK_SEL0 = 0x0;

//3.set clock configuration register 
CLK_CFG0 &= 0; 
CLK_CFG0 |= (0<<9)|(1<<8)|0xf; 

//4.set SPI MODE configureation register
MODE_CFG0 &= 0;
MODE_CFG0 = ((0x0 << 29) | (0x0 << 19) | (0x0 << 17) | (0 << 11) | (0 << 5) | (0 << 2) | (0 << 1) | (0 << 0)); //fifo 接收数据100字节,就产生中断 
//printk("open...\n");

//5.set SPI_INT_EN register
SPI_INT_EN0 = 0x2;//(1 << 1);//打开RXFIFO中断 
//6.set PACKET_CNT_REG register if necessary
//PACKET_CNT_REG0=0;
//7.set Tx or Rx Channel on. and spi slave
CH_CFG0 |=   (1<<1) | (1<<0);
//8.set nSSout low to start Tx or Rx operation
//b.if auto chip selection bit is set, nSSout is controlled automatically.
CS_REG0 = (0x03); 
//SWAP_CONFIG0 = 0x00;
//SWAP_CONFIG0 = ((0<<6)|(1 << 5) | (0 << 4) | (1 << 1) | (0 << 0)); 
printk("\n\n SPI opened!!\n");
printk("\n\nafter open:SPI_STATUS0=%xH tmp====%d\n",SPI_STATUS0, (SPI_STATUS0>>15)&0x1ff);
show_config();
//int data = SPI_TX_DATA0;
return 0; 
} 

static ssize_t spi_read(struct file *filp,char __user *buf,size_t count,loff_t *f_ops) 
{ 
int err;
	//CH_CFG0 &= ~(1<<4);
while(count > data_num); //等待要读的数据 
	err = copy_to_user((void *)buf, (const void *)total_buf, count); 
	
data_num -= count; 
memcpy(total_buf,total_buf+count,data_num); 
printk("data_num:%d,%d \n",data_num,count); 
return count; 
} 
static ssize_t spi_write(struct file * filp, const char __user *buf, size_t count, loff_t *f_ops){
#ifndef MASTER
	return 0;
#else
if(HAVE_ERROR(SPI_STATUS0))
{
	printk("Error occured:STATUS:%x\n", SPI_STATUS0);
	show_status();
	CH_CFG0 |= (1 << 5);
	PENDING_CLR_REG0 = 0x1F;
	CH_CFG0 &= ~(1 << 5);
	printk("Error cleared! Status:%x\n\n", SPI_STATUS0);
	return IRQ_HANDLED;
}
	int i=8;	
	printk("\n\n xxxxwriting......\n");
	show_status();
	static unsigned int data = 0;
	while(i--){
		SPI_TX_DATA0 =data++;// (data++)%0x10;
	}
/*	i = 100;
	while(i--)
	{
		int d = SPI_RX_DATA0;
		if(d!=0)
		printk("\n\n read after write====%d\n",d);
	}
*/		
	printk("writing......\n");
	return 100;
#endif
}
static int spi_release(struct inode *inode, struct file *filp) 
{ 
CH_CFG0 = 0;
return 0; 
} 


static const struct file_operations spi_fops = 
{ 
.owner = THIS_MODULE, 
.open = spi_open, 
.release = spi_release, 
.read = spi_read, 
.write = spi_write,
}; 

static struct miscdevice misc = 
{ 
.minor = MISC_DYNAMIC_MINOR, 
.name = DEVICE_NAME, 
.fops = &spi_fops, 
}; 

static irqreturn_t spi_interrupt(int irq,void *dev_id) 
{ 
int i=0; 
//printk("\n\n data===%x", SPI_RX_DATA0);

if(HAVE_ERROR(SPI_STATUS0))
{
	printk("Error occured:STATUS:%x\n", SPI_STATUS0);
	show_status();
	CH_CFG0 |= (1 << 5);
	PENDING_CLR_REG0 = 0x1F;
	CH_CFG0 &= ~(1 << 5);
	printk("Error cleared! Status:%x\n\n", SPI_STATUS0);
	return IRQ_HANDLED;
}
int tmp = ((SPI_STATUS0 >> 15) & 0x1ff); 
//int tmp=32;
printk("before read:SPI_STATUS0=%xH temp====%d\n",SPI_STATUS0, tmp);
show_status();
printk("Data: ");
for(i=0;i<tmp;i++) { 
	//((int*)buff)[i] = SPI_RX_DATA0;
	printk("%2xH ", SPI_RX_DATA0);


#ifndef SPI_MASTER
//	SPI_TX_DATA0 = 0x44444444;
#endif
}
printk("\n");
 
//memcpy(total_buf+data_num,buff,tmp); 
//data_num += tmp; 
printk("\n\nafter read:SPI_STATUS0=%xH tmp====%d\n",SPI_STATUS0, (SPI_STATUS0>>15)&0x1ff);
return IRQ_HANDLED; 
} 



static int __init spi_init(void) 
{ 
int ret;
int err; 
printk("start init....\n");
//寄存器映射到内存地址中。 
base_addr0 = ioremap(s5pv210_CLK, 0x04); 
base_addr1 = ioremap(s5pv210_GPB, 0x10); 
base_addr2 = ioremap(s5pv210_SPI, 0x30); 
base_addr3 = ioremap(s5pv210_CLK_DIV5, 0x04); 
base_addr5 = ioremap(s5pv210_CLK_SRC_MASK0, 0x4); 
printk("after ioremap:ba0:%p ba1:%p, ba2:%p ba3:%p\n",base_addr0, base_addr1, base_addr2, base_addr3);
//base_addr3 = ioremap(s5pv210_CLK_DIV_ISP, 0x04); 
//base_addr4 = ioremap(s5pv210_CLK_DIV_PERIL1, 0x04); 

//复杂设备的注册 
ret = misc_register(&misc); 
err = request_irq(IRQ_SPI0,spi_interrupt,IRQF_DISABLED,DEVICE_NAME,NULL); 
printk("Before ret init\n");
reg_init();
printk(DEVICE_NAME "\tinitialized\n"); 

return ret; 

} 

static void __exit spi_exit(void) 
{ 
CH_CFG0 = 0;
iounmap(base_addr0); 
iounmap(base_addr1); 
iounmap(base_addr2); 
iounmap(base_addr5);
misc_deregister(&misc); 
free_irq(IRQ_SPI0, NULL);
printk("<1>spi_exit!\n"); 
} 

module_init(spi_init); 
 
module_exit(spi_exit); 

MODULE_LICENSE("GPL");
