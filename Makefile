CFLAGS := -Wall -g -O2 -DNDEBUG=1

all: scmd

snappy.o: snappy.c

scmd: scmd.o snappy.o map.o

clean: 
	rm -f scmd.o snappy.o scmd

src: src.lex
	flex src.lex
	gcc ${CFLAGS} -o src lex.yy.c

fuzz: fuzzer.o map.o

bench: bench.o map.o snappy.o

bench.o: CFLAGS += -I ../simple-pmu

