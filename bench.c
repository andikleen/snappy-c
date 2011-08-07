#define _GNU_SOURCE 1
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "cycles.h"
#include "map.h"
#include "snappy.h"

#ifdef COMP
#include "../comp/zlib.h"
#include "../comp/lzo/lzo.h"
#endif

#define err(x) perror(x), exit(1)
#define u8 unsigned char

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	return ptr;
}

char *basen(char *s)
{
	char *p = strrchr(s, '/');
	if (p) 
		return p + 1;
	return s;
}

#define N 10

#define BENCH(name, fn, arg)						\
	counter_t a, b, total_comp = 0, total_uncomp = 0;		\
	for (i = 0; i < N + 1; i++) {					\
	    sync_core();						\
            a = unhalted_core();					\
	    err = c_##name(map, size, out, &outlen, arg);		\
	    b = unhalted_core();					\
	    if (i > 0) 							\
		    total_comp += b - a;				\
	    sync_core();						\
	    if (err)							\
		    printf("%s: compression of %s failed: %d\n", #name, fn, err); \
	    sync_core();						\
	    a = unhalted_core();					\
	    err = d_##name(out, outlen, buf2, size, arg);		\
	    b = unhalted_core();					\
            sync_core();						\
	    if (i > 0)							\
		    total_uncomp += b - a;				\
            if (err)							\
		    printf("%s: uncompression of %s failed: %d\n", #name, fn, err); \
       }								\
       printf("%-6s: %s: %lu bytes: ratio %.02f: comp %2.02f uncomp %2.02f c/b\n", \
	      #name, basen(fn), (unsigned long)size,			\
	      (double)outlen / size,					\
	      (double)(total_comp / N) / size,				\
	      (double)(total_uncomp / N) / size);			\


static inline int c_snappy(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	return snappy_compress(map, size, out, outlen);
}

static inline int d_snappy(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	return snappy_uncompress(out, outlen, buf2);
}

void test_snappy(char *map, size_t size, char *fn)
{
	int i;
	size_t outlen;
	int err;       
	char *out = xmalloc(snappy_max_compressed_length(size));
	char *buf2 = xmalloc(size);

	BENCH(snappy, fn, NULL);

	free(out);
	free(buf2);
}

static inline int c_lzo(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	return lzo1x_1_compress((u8*)map, size, (u8*)out, outlen, a);
}

static inline int d_lzo(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
        return lzo1x_decompress_safe((u8*)out, outlen, (u8*)buf2, &size);
}

void test_lzo(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = lzo1x_worst_compress(size);
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	char lzo_wmem[LZO1X_MEM_COMPRESS];

	BENCH(lzo, fn, lzo_wmem);

	free(out);
	free(buf2);
}

int main(int ac, char **av)
{
	pin_cpu(NULL);
	if(perfmon_available() == 0) {
		printf("no perfmon support\n");
		exit(1);
	}

	while (*++av) { 
		size_t size;
		char *map = mapfile(*av, O_RDONLY, &size);
		if (!map)
			err(*av);
		
		int i, v;
		for (i = 0; i < size; i += 4096)
			v = ((volatile char *)map)[i];

		test_snappy(map, size, *av);


#ifdef COMP		
		test_lzo(map, size, *av);
#endif		

		unmap_file(map, size);
		
	}
	return 0;
}
