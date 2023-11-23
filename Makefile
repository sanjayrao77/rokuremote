MULTICASTIP=192.168.1.19
CFLAGS=-g -O2 -Wall
all: rokuremote

rokuremote: main.o userstate.o options.o menus.o action.o interfaces.o discover.o getch.o automute.o joystick.o notify.o common/blockmem.o common/blockspool.o common/block_hget.o common/someutils.o common/net.o
	gcc -o $@ $^
rokuremote_staticip: main-static.o userstate.o options.o menus.o action.o discover.o getch.o automute.o joystick.o notify.o common/blockmem.o common/blockspool.o common/block_hget.o common/someutils.o common/net.o
	gcc -o $@ $^
main-static.o: main.c
	gcc -o main-static.o -c main.c ${CFLAGS} -DMULTICASTIP=\"${MULTICASTIP}\"
clean:
	rm -f *.o common/*.o core rokuremote rokuremote_staticip
monitor: clean
	scp -pr * monitor:src/rokuremote/
extra: clean
	scp -pr * extra:src/rokuremote/
air: clean
	scp -pr * air:src/rokuremote/
