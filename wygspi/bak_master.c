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


#define DEVICE_NAME "tiny210_spi" 

static void __iomem *base_addr0; 
static void __iomem *base_addr1; 
static void __iomem *base_addr2; 
static void __iomem *base_addr3; 
static void __iomem *base_addr4; 

//#define s5pv210_CLK 0xE010046C 
//#define s5pv210_GPB 0xE0200040 
//#define s5pv210_SPI 0xE1300000 
#define s5pv210_CLK 0x1003C950
#define s5pv210_GPB 0x11400040 
#define s5pv210_SPI 0x13920000
#define s5pv210_CLK_DIV_ISP 0x1003c538
#define s5pv210_CLK_DIV_PERIL1 0x1003c554
/*****************************************************/ 
//S3C2440_CLKCON 部分 
//#define CLK_GATE_IP3 (*(volatile unsigned long *)(base_addr0 + 0x00)) 
#define CLK_GATE_IP_PERIL (*(volatile unsigned long *)(base_addr0 + 0x00)) 
#define CLK_DIV_ISP (*(volatile unsigned long *)(base_addr3 + 0x00)) 
#define CLK_DIV_PERIL1 (*(volatile unsigned long *)(base_addr4 + 0x00)) 

//GPG 控制寄存器部分 GPB 
#define GPBCON (*(volatile unsigned long *)(base_addr1 + 0x00)) 
#define GPBDAT (*(volatile unsigned long *)(base_addr1 + 0x04)) 
#define GPBUP (*(volatile unsigned long *)(base_addr1 + 0x08)) 

//SPI 控制寄存器部分 
#define CH_CFG0 (*(volatile unsigned long *)(base_addr2 + 0x00)) 
#define CLK_CFG0 (*(volatile unsigned long *)(base_addr2 + 0x04)) 
#define MODE_CFG0 (*(volatile unsigned long *)(base_addr2 + 0x08)) 
#define CS_REG0 (*(volatile unsigned long *)(base_addr2 + 0x0c)) 
#define SPI_INT_EN0 (*(volatile unsigned long *)(base_addr2 + 0x10)) 
#define SPI_STATUS0 (*(volatile unsigned long *)(base_addr2 + 0x14)) 
#define SPI_TX_DATA0 (*(volatile unsigned long *)(base_addr2 + 0x18)) 
#define SPI_RX_DATA0 (*(volatile unsigned long *)(base_addr2 + 0x1c)) 
#define PACKET_CNT_REG0 (*(volatile unsigned long *)(base_addr2 + 0x20)) 
#define PENDING_CLR_REG0 (*(volatile unsigned long *)(base_addr2 + 0x24)) 
#define SWAP_CONFIG0 (*(volatile unsigned long *)(base_addr2 + 0x28)) 
#define FD_CLK_SEL0 (*(volatile unsigned long *)(base_addr2 + 0x2c)) 

#define SPI_TXRX_ON() (CH_CFG0 |= 0x3) 
#define SPI_TX_DONE (((SPI_STATUS >> 21) & 0x1) == 0x1) 
#define SPI_RX_num ((SPI_STATUS >>13) & 127) 

#define SPI_CS_LOW() (SLAVE_SEL &= ~0x01) 
#define SPI_CS_HIGH() (SLAVE_SEL |= 0x01) 

char buff[250]; 
char total_buf[1000]; 
volatile long data_num = 0; 

static int spi_open(struct inode *inode, struct file *filp) 
{ 
//使能时钟控制寄存器CLKCON 18位使能SPI 
//CLK_GATE_IP3 |= 1 << 12; 
CLK_DIV_ISP &= 0xFFFF000F;
CLK_DIV_ISP |= 0x0000FFF0;
CLK_DIV_PERIL1 &= 0xFFFF00F0;
CLK_DIV_PERIL1 |= 0x0000FF0F;
CLK_GATE_IP_PERIL |= 1 << 16; 
//设置GPIO 
GPBCON &= 0XFFFF0000; 
GPBCON |= 0X00002222; 
//GPEUP 设置;全部disable 
GPBUP &= ~(0xff << 0); 
//GPBDAT 设置位0 
GPBDAT &= ~(0xff << 0); 

//CPHA = 1;CPOL=1;为从模式 
CH_CFG0 &= 0;
CH_CFG0 = ((0 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0)); 


//时钟设为PCLK 
//CLK_CFG0 &= 0; 
//CLK_CFG0 |= (1<<8)|0xff; 

//设置SPI模式 
MODE_CFG0 &= 0;
MODE_CFG0 = ((0x2 << 29) | (0 << 19) | (0x2 << 17) | (8 << 11) | ( 8<< 5) | (0 << 2) | (0 << 1) | (0 << 0)); //fifo 接收数据100字节,就产生中断 

CS_REG0 = (0x03); 
SPI_INT_EN0 |= (1 << 1);//打开RXFIFO中断 
SWAP_CONFIG0 = ((1 << 4) | (1 << 5) | (1 << 0) | (1 << 1)); 
printk("\n\n SPI opened!!\n");
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
//	CH_CFG0 |= (1<<4);
	int i=16;	
	PENDING_CLR_REG0 = 0xF;
	PENDING_CLR_REG0 = 0x0;	
	printk("\n\n xxxxwriting......\n");
	while(i--){
		SPI_TX_DATA0 = 0XCCCCCCCC;
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
}
static int spi_release(struct inode *inode, struct file *filp) 
{ 
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
int tmp = ((SPI_STATUS0 >> 15) & 0x1ff); 
printk("\n\n SPI_STATUS0=%x temp====%d\n",SPI_STATUS0, tmp);
for(i=0;i<tmp;i+=4) { 
	//((int*)buff)[i] = SPI_RX_DATA0;
	printk("\n\n data===%x", SPI_RX_DATA0);
}
 
//memcpy(total_buf+data_num,buff,tmp); 
//data_num += tmp; 
printk("\n\nSPI_STATUS0= %d , %d , %d\n\n",((SPI_STATUS0 >> 15) & 0x1ff),tmp,data_num); 
return IRQ_HANDLED; 
} 



static int __init spi_init(void) 
{ 
int ret;
int err; 
//寄存器映射到内存地址中。 
base_addr0 = ioremap(s5pv210_CLK, 0x04); 
base_addr1 = ioremap(s5pv210_GPB, 0x10); 
base_addr2 = ioremap(s5pv210_SPI, 0x30); 
base_addr3 = ioremap(s5pv210_CLK_DIV_ISP, 0x04); 
base_addr4 = ioremap(s5pv210_CLK_DIV_PERIL1, 0x04); 

//复杂设备的注册 
ret = misc_register(&misc); 
err = request_irq(IRQ_SPI0,spi_interrupt,IRQF_DISABLED,DEVICE_NAME,NULL); 
printk(DEVICE_NAME "\tinitialized\n"); 
return ret; 

} 

static void __exit spi_exit(void) 
{ 
iounmap(base_addr0); 
iounmap(base_addr1); 
iounmap(base_addr2); 
misc_deregister(&misc); 
free_irq(IRQ_SPI0, NULL);
printk("<1>spi_exit!\n"); 
} 

module_init(spi_init); 
 
module_exit(spi_exit); 

MODULE_LICENSE("GPL");
