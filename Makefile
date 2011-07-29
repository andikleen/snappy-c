CFLAGS := -Wall

all: scmd

snappy.o: snappy.c

scmd: scmd.o snappy.o

clean: 
	rm -f scmd.o snappy.o scmd
