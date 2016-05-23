#include <stdio.h>
#include <fcntl.h>
#include "debug.h"
#define DEV_NAME "/dev/spi_master"
#define BUF_LEN (2048)
char buf[BUF_LEN];
int main(int argc, char *argv[])
{
	int fd;
	int file;
	int len;
	int i;
	int ret;
	int n = 0;
	int offset = 0;
	if(argc !=2 )
		err_quit("usage:%s file_name", argv[0]);
	CHECK(fd = open(DEV_NAME, O_WRONLY));
	CHECK(file = open(argv[1], O_RDONLY));
	printf("ok, begin send!\n");
	int total = 0;
	while(1)
	{
		len = read(file, buf, BUF_LEN);
	//	buf[len] = 0;
		total += len;
		printf("read %d bytes from file! total:%d\n", len, total);
		if(len <= 0)
			break;
		int offset = 0;
		while(len > 0)
		{
			int ret = write(fd, buf+offset ,len);
			printf("write %d bytes to spi!\n", ret);
			if(ret <= 0)
				err_quit("write spi failed!");
			offset += ret;
			len -= ret;
		}
		//usleep(10000);
	}
	printf("write %d byte!\n", offset);
	close(file);
	close(fd);
	return 0;
}
