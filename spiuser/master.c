#include <stdio.h>
#include <fcntl.h>
#include "debug.h"
#define DEV_NAME "/dev/spi_master"
#define BUF_LEN (1024*1024*10)
char buf[BUF_LEN];
int main(int argc, char *argv[])
{
	int fd;
	int len;
	int i;
	int ret;
	int n = 0;
	int offset = 0;
	if(argc !=2 )
		err_quit("usage:%s send_data_num", argv[0]);
	n = atoi(argv[1]);	
	n = n < BUF_LEN ? n : BUF_LEN;
	CHECK(fd = open(DEV_NAME, O_WRONLY));
	//read(fd, buf, BUF_LEN);	
	for(i = 0; i < n; i++)
		buf[i] = (char)i;
	len = n;
	while(len > 0)
	{
		ret = write(fd, buf+offset, len);
		if(ret <= 0)
			break;
		len -= ret;
		offset += ret;
	}
	printf("write %d byte!\n", offset);
	close(fd);
	return 0;
}
