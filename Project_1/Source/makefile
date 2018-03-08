CC=gcc

CFLAGS = -g -c -Wall -pedantic

all: test

test: test.o sendto_dbg.o
	$(CC) -o test test.o sendto_dbg.o  

net: ncp rcv

ncp: ncp.o net_include.h sendto_dbg.o
	$(CC) -o ncp ncp.o sendto_dbg.o

net_client2: net_client2.o net_include.h sendto_dbg.o
	$(CC) -o net_client2 net_client2.o sendto_dbg.o

net_server: net_server.o net_include.h sendto_dbg.o
	$(CC) -o net_server net_server.o sendto_dbg.o

net_serverUDP: rcv2.o net_include.h sendto_dbg.o
	$(CC) -o rcv2 rcv2.o sendto_dbg.o

rcv: rcv.o net_include.h sendto_dbg.o
	$(CC) -o rcv rcv.o sendto_dbg.o

clean:
	rm *.o

%.o:    %.c
	$(CC) $(CFLAGS) $*.c
