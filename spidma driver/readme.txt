������������linux2.6.17�в���ͨ������ʹ���ڲ�Ʒ�С�
yangxing msn:lelma_yx@hotmail.com
ϣ����SPI��������������������

һ��������ʽ��
���豸��SPIΪMASTERģʽ
S3C2410��SPIΪSLAVEģʽ��DMA
������������
1.S3C2410�ӽ��գ������豸��������ʱ��S3C2410����DMA��ʽ�������յ�ָ�����ȵ����ݣ������DMA�жϣ������յ����ݿ�����
2.S3C2410�ӷ��ͣ���S3C2410��Ҫ���ͣ�������RTS�����ͣ�Ȼ��ȴ����豸Ӧ��CTS�������豸Ӧ��CTSʱ�������ⲿ�жϣ�����DMA���ͣ�������ɣ��ٴν���ӽ���״̬��
�����ļ�λ��
spi_dma_slave.c
spi_dma_slave.h
circular_buf.c
circular_buf.h
���ļ������driver/char/Ŀ¼
dma.c�����arch/arm/mach-s3c2410/Ŀ¼
dma.h�����include/arm-asm/mach-s3c2410/Ŀ¼
�ġ�ʹ�û���
1.arm-linux-gcc-3.4.1