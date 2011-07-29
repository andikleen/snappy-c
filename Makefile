CFLAGS := -Wall

all: scmd

snappy.o: snappy.c

scmd: scmd.o snappy.o