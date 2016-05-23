#include <stdio.h>
#include <memory.h>

#define info(format, ...) {printf("%s(%d):", __FILE__, __LINE__);printf(format, ##__VA_ARGS__);}

#ifndef NDEBUG
#define debugmsg(format, ...){info(format, ##__VA_ARGS__);}
#else
#define debugmsg(format, ...)
#endif
#define RX_BUF_SIZE (1024*1024*16)
#define TX_BUF_SIZE (64)
#define REG_RX_FIFO_SIZE 256
#define REG_TX_FIFO_SIZE 256
#define DEFAULT_RX_RDY_LVL 0
#define DEFAULT_TX_RDY_LVL 50
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
typedef unsigned char uint8;
typedef unsigned int uint32;
char rxbuf[RX_BUF_SIZE];
char txbuf[TX_BUF_SIZE];
struct circle_buffer //环形缓冲区大小必须是2^n
{
	uint32 head;
	uint32 tail;
	uint8  *buf;
	uint32 size;
};
struct circle_buffer cb = 
{
	.buf = rxbuf, 
	.size = RX_BUF_SIZE,
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
static inline int put_byte_to_buf(struct circle_buffer *cb, uint8 b)
{
	if(!cirbuf_full(cb))
	{
		cb->buf[cb->head & (cb->size - 1)] = b;
		cb->head++;
		return 0;
		//debugmsg("head: %d\n", cb->head);
	}
	else
	{
		info("error:buf is full!!\n");
		return -1;
	}
}
static inline uint8  get_byte_from_buf(struct circle_buffer *cb)
{
	uint8 byte = 0;
	if(!cirbuf_empty(cb))
	{
		byte = cb->buf[cb->tail & (cb->size - 1)];
		cb->tail++;
	}
	else
		info("error:buff is empty\n");
	return byte;
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
	debugmsg("real_copy: %u bytes, buff_free:%u bytes, buff_used:%u bytes, buff_size:%u byte head:%u tail:%u\n", 
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
	debugmsg("real_copy: %u bytes, buff_free:%u bytes, buff_used:%u bytes, buff_size:%u byte head:%u tail:%u\n", 
			len, cirbuf_get_free(cb), cb->size - cirbuf_get_free(cb), cb->size, cb->head, cb->tail);
	return len;
}
#define SIZE (1024*1024*10)
unsigned char tx[SIZE];
unsigned char rx[SIZE];
int main()
{
	int i;
	int j;
	int ret;
	for(i = 0; i < SIZE; i++)
		tx[i] = (uint8)i;
	while(1)
	{
	for(j = 0; j < 2; j++)	
	{
		//copy_cirbuf_from_user(&cb, tx, SIZE);
		int k;
		for(k = 0; k < SIZE; k++)
			if(put_byte_to_buf(&cb, (uint8)k) != 0)
				return -1;
		for(k = 0; k < SIZE; k++)
			get_byte_from_buf(&cb);
		//ret = copy_cirbuf_to_user(&cb, rx,SIZE /2 );
		//for(i = 0 ;i < ret; i++)
		//	printf("%-4d", rx[i]);
	debugmsg("real_copy: %u bytes, buff_free:%u bytes, buff_used:%u bytes, buff_size:%u byte head:%u tail:%u\n", 
			0, cirbuf_get_free(&cb), cb.size - cirbuf_get_free(&cb), cb.size, cb.head, cb.tail);
		printf("\n");
	}
		usleep(50000);
	}
	
	unsigned int head = 1500;
	unsigned int tail = 2000;
	unsigned int size = 1024;
	printf("free:%u\n", ((head - tail)&(size-1)));
	return 0;	
}
