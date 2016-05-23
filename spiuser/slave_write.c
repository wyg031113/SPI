#include <stdio.h>
#include <fcntl.h>
#include "debug.h"
#define DEV_NAME "/dev/spi_slave"
#define BUF_LEN (1024*1024*4)
char buf[BUF_LEN];
int main(int argc, char *argv[])
{
	int fd;
	int len;
	unsigned int i,n;
	unsigned char cnt = 0;
	unsigned int total = 0;
	int offset = 0;
	if(argc != 2)
		err_quit("usage:%s send_num\n", argv[0]);
	n = atoi(argv[1]);
	CHECK(fd = open(DEV_NAME, O_RDWR));
	for(i = 0; i < n; i++)
		buf[i] = (unsigned char)i;
	while(n > 0)
	{
		len = write(fd, buf, n); 
		if(len <= 0)
			break;
		offset += len;
		n -= len;
	}
	printf("total write:%d bytes\n", offset);
	close(fd);
	return 0;
}
