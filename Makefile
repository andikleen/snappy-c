CFLAGS := -Wall -g -O2 -DNDEBUG=1

all: scmd

snappy.o: snappy.c

scmd: scmd.o snappy.o

clean: 
	rm -f scmd.o snappy.o scmd
