#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/gpio.h>
#include <mach/dma.h>
#include <mach/map.h>
#include <mach/irqs.h>
#include <mach/spi-clocks.h>
#include <plat/s3c64xx-spi.h>
#include <plat/gpio-cfg.h>
#include<linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/device.h>
static int s5pv210_spi_cfg_gpio(struct platform_device *pdev)
{
	unsigned int base;

	switch (pdev->id) {
	case 0:
		base = S5PV210_GPB(0);
		break;

	case 1:
		base = S5PV210_GPB(4);
		break;

	default:
		dev_err(&pdev->dev, "Invalid SPI Controller number!");
		return -EINVAL;
	}

	s3c_gpio_cfgall_range(base, 4,
			      S3C_GPIO_SFN(2), S3C_GPIO_PULL_UP);

	return 0;
}

static struct resource s5pv210_spi0_resource[] = {
	[0] = {
		.start = S5PV210_PA_SPI0,
		.end   = S5PV210_PA_SPI0 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_SPI0_TX,
		.end   = DMACH_SPI0_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_SPI0_RX,
		.end   = DMACH_SPI0_RX,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.start = IRQ_SPI0,
		.end   = IRQ_SPI0,
		.flags = IORESOURCE_IRQ,
	},
};

static struct s3c64xx_spi_info s5pv210_spi0_pdata = {
	.cfg_gpio = s5pv210_spi_cfg_gpio,
	.fifo_lvl_mask = 0x1ff,
	.rx_lvl_offset = 15,
	.high_speed = 1,
	.tx_st_done = 25,
};


static u64 spi_dmamask = DMA_BIT_MASK(32);

struct platform_device s5pv210_device_spi0 = {
	.name		  = "s3c64xx-spi",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s5pv210_spi0_resource),
	.resource	  = s5pv210_spi0_resource,
	.dev = {
		.dma_mask		= &spi_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data = &s5pv210_spi0_pdata,
	},
};

static int __init platform_device_init(void) 
{ 
        int ret = 0;
        /*采用platform_device_alloc分配一个platform_device对象 
          参数分别为platform_device的name,和id。 
        */ 
        /*注册设备,将平台设备注册到内核中*/ 
        ret = platform_device_register(&s5pv210_device_spi0);
        return ret; 
}

/*卸载处理函数*/ 
static void __exit platform_device_exit(void)
{ 
        platform_device_unregister(&s5pv210_device_spi0); 
}

module_init(platform_device_init); 
module_exit(platform_device_exit);
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("xidian");
