#include <stdio.h>
#include <fcntl.h>
#include "debug.h"
#define DEV_NAME "/dev/spi_slave"
#define BUF_LEN 4096
char buf[BUF_LEN];
int main()
{
	int fd;
	int len;
	int i;
	unsigned char cnt = 0;
	unsigned int total = 0;
	CHECK(fd = open(DEV_NAME, O_RDWR));
	while(1)
	{
		len = read(fd, buf, BUF_LEN);	
		if(len <= 0)
			break;
		total += len;
		for(i =0; i < len; i++)
		{
			printf("%2xh ", buf[i]);
			/*if(buf[i] != cnt++)
			{
				printf("buf[%d] = %d\n", i, buf[i]);
				break;
			}
			*/
		}
		printf("\n");
		printf("recv:%d bytes\n", total);
	}
	close(fd);
	return 0;
}
