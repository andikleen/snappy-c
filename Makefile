CFLAGS := -Wall -g -O2 -DNDEBUG=1 

CFLAGS += -m32
LDFLAGS += -m32

all: scmd

snappy.o: snappy.c

scmd: scmd.o snappy.o map.o

clean: 
	rm -f scmd.o snappy.o scmd bench bench.o fuzzer.o fuzz map.o

src: src.lex
	flex src.lex
	gcc ${CFLAGS} -o src lex.yy.c

fuzz: fuzzer.o map.o

bench: bench.o map.o snappy.o ../comp/lzo.o ../comp/zlib.o

bench.o: CFLAGS += -I ../simple-pmu -D COMP



