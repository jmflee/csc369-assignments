CC = gcc
CFLAGS = -Wall -g

all: ext2_ls ext2_rm ext2_cp ext2_ln ext2_mkdir 

ext2_ls: ext2_ls.o ext2_utils.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_rm: ext2_rm.o ext2_utils.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_mkdir: ext2_mkdir.o ext2_utils.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_ln: ext2_ln.o ext2_utils.o
	$(CC) $(CFLAGS) -o $@ $^

ext2_cp: ext2_cp.o ext2_utils.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o ext2_ls ext2_rm ext2_cp ext2_ln ext2_mkdir 
