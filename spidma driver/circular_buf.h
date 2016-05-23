#ifndef __CIRCULAR_BUF_H__
#define	__CIRCULAR_BUF_H__

#include <linux/slab.h>

struct circular_buffer{
	unsigned int size;
	unsigned int in;
	unsigned int out;
	char *buffer;
};
void init_circular_buf(struct circular_buffer *buf, int size);//yangxing
void clear_circular_buf(struct circular_buffer *buf);//yangxing
//void init_circular_buf(struct circular_buffer *buf);
void free_circular_buf(struct circular_buffer *buf);
int write_to_circular_buf(struct circular_buffer *buf, char __user *pdata, int count);
int read_from_circular_buf(struct circular_buffer *buf, char __user *pdata, int count);
int read_circular_buf_1(struct circular_buffer *buf, char *pdata);
int write_circular_buf_1(struct circular_buffer *buf, char data);
int can_circular_buf_write(struct circular_buffer *buf);
int can_circular_buf_read(struct circular_buffer *buf);
//yangxing
int write_circular_buf(struct circular_buffer *buf, char *pdata, int count);
int read_circular_buf(struct circular_buffer *buf, char *pdata, int count);
#endif
