CFLAGS := -Wall -g -O2 -DNDEBUG=1

all: scmd

snappy.o: snappy.c

scmd: scmd.o snappy.o

clean: 
	rm -f scmd.o snappy.o scmd

src: src.lex
	flex src.lex
	gcc -o src lex.yy.c

