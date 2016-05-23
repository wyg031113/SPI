#include <stdio.h>
#include <fcntl.h>
#include "debug.h"
#define DEV_NAME "/dev/spi_master"
#define BUF_LEN (1024*1024*4)
#include <time.h>
#include <stdlib.h>
char buf[BUF_LEN];
int main()
{
	int fd;
	int len;
	int i;
	unsigned char cnt = 0;
	unsigned int total = 0;
	CHECK(fd = open(DEV_NAME, O_RDWR));
	long all = 0;
	long start = time(0);
	while(1)
	{
		
		len = read(fd, buf, BUF_LEN);	
			
		if(len < 0)
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
		printf("use time:%ds\n", time(0)-start);
	}
	close(fd);
	return 0;
}
