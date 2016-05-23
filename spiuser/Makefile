all:master slave slave_write master_read sender rcver
CC=arm-linux-gcc
CFLAGS=-ggdb3 -O0
LIBS=

master:master.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	scp master 192.168.0.73:/
sender:sender.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	scp sender 192.168.0.73:/

rcver:rcver.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	scp rcver 192.168.0.121:/
master_read:master_read.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	scp master_read 192.168.0.74:/
slave:slave.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	scp slave 192.168.0.121:/
slave_write:slave_write.c
	$(CC) $(CFLAGS) $< $(LIBS) -o $@
	scp slave_write 192.168.0.121:/

clean:
	rm -rf *.o master slave slave_write master_read
