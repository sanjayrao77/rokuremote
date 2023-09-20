MULTICASTIP=192.168.1.8 
CFLAGS=-g -O2 -Wall
all: rokuremote

rokuremote: main.o interfaces.o discover.o getch.o automute.o common/blockmem.o common/blockspool.o common/block_hget.o common/someutils.o common/net.o
	gcc -o $@ $^
rokuremote_staticip: main-static.o discover.o getch.o automute.o common/blockmem.o common/blockspool.o common/block_hget.o common/someutils.o common/net.o
	gcc -o $@ $^
main-static.o: main.c
	gcc -o main-static.o -c main.c ${CFLAGS} -DMULTICASTIP=\"${MULTICASTIP}\"
clean:
	rm -f *.o common/*.o core rokuremote rokuremote_staticip
