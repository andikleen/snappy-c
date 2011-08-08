CFLAGS := -Wall -g -O2 -DNDEBUG=1 

CFLAGS += -m32
LDFLAGS += -m32

all: scmd verify

snappy.o: snappy.c

scmd: scmd.o snappy.o map.o util.o

CLEAN := scmd.o snappy.o scmd bench bench.o fuzzer.o fuzzer map.o verify.o \
	 verify util.o

clean: 
	rm -f ${CLEAN}

src: src.lex
	flex src.lex
	gcc ${CFLAGS} -o src lex.yy.c


OTHER := ../comp/lzo.o ../comp/zlib.o ../comp/lzf.o ../comp/quicklz.o \
	 ../comp/fastlz.o

fuzzer.o: CFLAGS += -D COMP

fuzzer: fuzzer.o map.o util.o snappy.o ${OTHER}

bench: bench.o map.o snappy.o util.o ${OTHER} 

bench.o: CFLAGS += -I ../simple-pmu -D COMP # -D SIMPLE_PMU

verify: verify.o map.o snappy.o util.o


