#include <stdio.h>
#include <memory.h>

#define info(format, ...) {printf("%s(%d):", __FILE__, __LINE__);printf(format, ##__VA_ARGS__);}
#define NDEBUG
#ifndef NDEBUG
#define debugmsg(format, ...){info(format, ##__VA_ARGS__);}
#else
#define debugmsg(format, ...)
#endif
#define BUF_SIZE (1024)//环形缓冲区大小必须是2^n
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define SEG_SIZE 1024
typedef unsigned char uint8;
typedef unsigned int uint32;
char buf[BUF_SIZE];
char seg_read[SEG_SIZE];
char seg_write[SEG_SIZE];
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdlib.h>
struct circle_buffer
{
	uint32 head;
	uint32 tail;
	uint8  *buf;
	uint32 size;
};
struct circle_buffer cb = 
{
	.buf = buf, 
	.size = BUF_SIZE,
	.head = 0,
	.tail = 0
};
static void cirbuf_init(struct circle_buffer *cb, uint8 *buf, int size)
{
	cb->head = cb->tail = 0;
	cb->buf = buf;
	cb->size = size;
}

static inline int cirbuf_empty(struct circle_buffer *cb)
{
	return (cb->head == cb->tail);
}
static inline int cirbuf_full(struct circle_buffer *cb)
{
	return cb->size == cb->head - cb->tail;
}

static inline size_t cirbuf_get_free(struct circle_buffer *cb)
{
	return cb->size - cb->head + cb->tail;
}

/*copy cirbuf's data to user space, use copy to user, so can't call this in an interrupt routing*/


static ssize_t copy_cirbuf_to_user(struct circle_buffer *cb, uint8 *user, size_t len)
{
	uint32 l = 0;
	uint32 head = cb->head;
	uint32 tail = cb->tail;

	len = MIN(len, head - tail);
	l =  MIN(len, cb->size - (tail & (cb->size-1)));
	memcpy(user, cb->buf + (tail & (cb->size-1)), l);
	memcpy(user + l, cb->buf, len -l);
	cb->tail += len;
	debugmsg("to user:real_copy: %u bytes, buff_free:%u bytes, buff_used:%u bytes, buff_size:%u byte head:%u tail:%u\n", 
			len, cirbuf_get_free(cb), cb->size - cirbuf_get_free(cb), cb->size, cb->head, cb->tail);
	return len;
}

static ssize_t copy_cirbuf_from_user(struct circle_buffer *cb, const uint8  *user, size_t len)
{
	uint32 l = 0;
	uint32 head = cb->head;
	uint32 tail = cb->tail;

	len = MIN(len, cb->size - head + tail);
	l = MIN(len, cb->size - (head &(cb->size-1)));
	memcpy(cb->buf + (head & (cb->size-1)), user, l);
	memcpy(cb->buf, user + l, len - l);
	cb->head += len;
	debugmsg("to buf:real_copy: %u bytes, buff_free:%u bytes, buff_used:%u bytes, buff_size:%u byte head:%u tail:%u\n", 
			len, cirbuf_get_free(cb), cb->size - cirbuf_get_free(cb), cb->size, cb->head, cb->tail);
	return len;
}
sem_t ndata;
sem_t nempty;
volatile int pstop = 0;
volatile int cstop = 0;
void stop_cons_and_exit(void)
{
	cstop = 1;
	sem_post(&ndata);
	pthread_exit(NULL);
}
void *productor(void *arg)
{
	char *src_file = (char *)arg;
	printf("I am productor, read file!\n");
	int fd;
	if((fd = open(src_file, O_RDONLY))==-1)
	{
		printf("Open file %s failed!\n", src_file);
		stop_cons_and_exit();
	}
	while(1)
	{
		int ret = read(fd, seg_read, SEG_SIZE);
		if(ret<=0)
			break;
		else
		{
			sem_wait(&nempty);
			if(pstop)
				break;
			int real = copy_cirbuf_from_user(&cb, seg_read, ret);
			if(real != ret)
			{
				printf("copy cirbuf failed!!!!!!!\n");
				close(fd);
				stop_cons_and_exit();
			}
			sem_post(&ndata);
		}
	}

	sem_post(&ndata);
	close(fd);
	return NULL;
}

void stop_pro_and_exit(void)
{
	pstop = 1;
	sem_post(&nempty);
	pthread_exit(NULL);
}
void *consumer(void *arg)
{
	char *dst_file = (char *)arg;
	printf("I am consumer, write file!\n");
	int fd;
	int ret;
	if((fd = open(dst_file, O_WRONLY|O_CREAT, 0666))==-1)
	{
		printf("Open file %s failed!\n", dst_file);
		stop_pro_and_exit();
	}
	while(1)
	{
		sem_wait(&ndata);
		if(cirbuf_empty(&cb))
			break;
		ret  = copy_cirbuf_to_user(&cb, seg_write, SEG_SIZE);
		//printf("reg write: %d bytes\n", len);
		sem_post(&nempty);
		int real = write(fd, seg_write, ret);
		if(real<=0 || real != ret)
		{
			printf("write file failed!\n");
			close(fd);
			stop_pro_and_exit();
		}
	}	
	close(fd);
	return NULL;
}
int main(int argc, char *argv[])
{
	int ret;
	if(argc != 3)
	{
		printf("Usage:copy src_file_name dst_file_name\n");
		exit(1);
	}
	cirbuf_init(&cb, buf, BUF_SIZE);

	ret = sem_init(&ndata, 0, 0);
	if(ret == -1)
	{
		printf("open sem ndata failed!\n");
		goto out1;
	}

	ret = sem_init(&nempty, 0, (BUF_SIZE)/SEG_SIZE);
	if(ret == -1)
	{
		printf("open sem nempyt failed!\n");
		goto out2;
	}	


	pthread_t tid1, tid2;
	if(pthread_create(&tid1, NULL, productor, (void*)argv[1]) != 0)
	{
		printf("create productor thread failed!\n");
		goto out;
	}
	if(pthread_create(&tid2, NULL, consumer,(void*) argv[2]) != 0)
	{
		printf("create productor thread failed!\n");
		goto out;
	}
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
out:
	sem_destroy(&nempty);
out2:
	sem_destroy(&ndata);
out1:
	printf("finish.\n");
	return 0;
}
