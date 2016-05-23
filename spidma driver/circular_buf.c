#include <linux/slab.h>
#include <asm/types.h>
#include <asm/uaccess.h>

#include "circular_buf.h"

#ifdef DEBUG
	#define	mydebug(fmt,args...)	printk(fmt,##args)
#else
	#define	mydebug(fmt,args...)
#endif


void clear_circular_buf(struct circular_buffer *buf)//yangxing modify
{
	buf->in=buf->out=0;
}
//circular_buffer operations
void init_circular_buf(struct circular_buffer *buf, int size)//yangxing modify
{
	buf->size=size;//512;
	buf->in=buf->out=0;
	buf->buffer=kmalloc(buf->size,GFP_KERNEL);
}
void free_circular_buf(struct circular_buffer *buf){
	kfree(buf->buffer);
}
int write_to_circular_buf(struct circular_buffer *buf, char __user *pdata,int count)
{
	unsigned int l;
	unsigned int mask;
	int count_t = count;

	mask=buf->size-1;

	count_t	= min( count_t, mask& (mask - buf->in + buf->out ) );
	l	= min( count_t, buf->size - (buf->in & mask) );
	copy_from_user(buf->buffer + (buf->in & mask), pdata, l);
	copy_from_user(buf->buffer, pdata+l, count_t-l);

	buf->in +=count_t;
	return count_t;
}

int read_circular_buf_1(struct circular_buffer *buf, char *pdata)
{
	unsigned int mask=buf->size-1;
	if(buf->out!=buf->in){
		*pdata=buf->buffer[buf->out&mask];
		buf->out++;
		return 1;
	}
	return 0;
}
int can_circular_buf_write(struct circular_buffer *buf)
{
	if( (buf->in+1)==buf->out) return 0;
	return 1;
}
int can_circular_buf_read(struct circular_buffer *buf)
{
	if( buf->in==buf->out) return 0;
	return 1;
}
int write_circular_buf_1(struct circular_buffer *buf, char data)
{
	unsigned int mask=buf->size-1;
	if((buf->in+1)!=buf->out){
		buf->buffer[ buf->in&mask ]=data;
		buf->in++;
		return 1;
	}
	return 0;
}

int read_from_circular_buf(struct circular_buffer *buf, char __user *pdata, int count)
{
	unsigned int l;
	unsigned int mask;
	int count_t = count;

	mask =buf->size-1;

	count_t	= min(count_t, buf->in - buf->out);
	l	= min(count_t, buf->size - (buf->out &mask));
	
	copy_to_user(pdata, buf->buffer+ (buf->out & mask), l);
	copy_to_user(pdata+l, buf->buffer, count_t -l);

	buf->out+=count_t;
	return count_t;
}
//yangxing
int write_circular_buf(struct circular_buffer *buf, char *pdata,int count)
{
	unsigned int l;
	unsigned int mask;
	int count_t = count;

	mask=buf->size-1;

	count_t	= min( count_t, mask& (mask - buf->in + buf->out ) );
	l	= min( count_t, buf->size - (buf->in & mask) );

	memcpy(buf->buffer + (buf->in & mask), pdata, l);
	memcpy(buf->buffer, pdata+l, count_t-l);

	buf->in +=count_t;
	return count_t;
}
int read_circular_buf(struct circular_buffer *buf, char *pdata, int count)
{
	unsigned int l;
	unsigned int mask;
	uint count_t = count;

	mask =buf->size-1;

	count_t	= min(count_t, buf->in - buf->out);
	l	= min(count_t, buf->size - (buf->out &mask));
	
	memcpy(pdata, buf->buffer+ (buf->out & mask), l);
	memcpy(pdata+l, buf->buffer, count_t -1);

	buf->out+=count_t;
	return count_t;
}
