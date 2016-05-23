本驱动程序在linux2.6.17中测试通过，并使用于产品中。
yangxing msn:lelma_yx@hotmail.com
希望对SPI操作的朋友有所帮助。

一、工作方式：
从设备：SPI为MASTER模式
S3C2410：SPI为SLAVE模式＋DMA
二、工作流程
1.S3C2410从接收：当从设备发送数据时，S3C2410利用DMA方式收数，收到指定长度的数据，则进入DMA中断，将接收的数据拷出。
2.S3C2410从发送：当S3C2410需要发送，首先由RTS请求发送，然后等待从设备应答CTS，当从设备应答CTS时，进入外部中断，启动DMA发送，发送完成，再次进入从接收状态。
三、文件位置
spi_dma_slave.c
spi_dma_slave.h
circular_buf.c
circular_buf.h
等文件存放入driver/char/目录
dma.c存放于arch/arm/mach-s3c2410/目录
dma.h存放于include/arm-asm/mach-s3c2410/目录
四、使用环境
1.arm-linux-gcc-3.4.1