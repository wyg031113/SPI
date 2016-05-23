#include <stdio.h>
#include <fcntl.h>
#include "debug.h"
#define DEV_NAME "/dev/spi_slave"
#define BUF_LEN 256
char buf[BUF_LEN];
int main(int argc, char *argv[])
{
	int fd;
	int len;
	int i;
	int file;
	unsigned char cnt = 0;
	unsigned int total = 0;
	int n;
	if(argc != 2)
		err_quit("usage:%s len", argv[0]);
	n = atoi(argv[1]);
	CHECK(fd = open(DEV_NAME, O_RDWR));
	CHECK(file = open("rcv.dat", O_WRONLY|O_CREAT|O_TRUNC, 0666));
	while(n>0)
	{
		len = read(fd, buf, BUF_LEN);	
		if(len <= 0)
			break;
		n -= len;
		total += len;

		len = write(file, buf, len);
		//buf[len]  = 0;
	//	printf("write %d bytes to file:\n", len);

		if(len < 0)
			err_quit("write file failed!\n");
	}
		printf("\n");
		printf("recv:%d bytes\n", total);
	close(file);
	close(fd);
	return 0;
}
