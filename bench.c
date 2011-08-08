#define _GNU_SOURCE 1
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "map.h"
#include "snappy.h"

#ifdef COMP
#include "../comp/zconf.h"
#include "../comp/zlib.h"
#include "../comp/lzo/lzo.h"
#include "../comp/lzf.h"
#include "../comp/quicklz.h"
#include "../comp/fastlz.h"
#endif

#ifdef SIMPLE_PMU
#include "cycles.h"
#define COUNT() unhalted_core()
#else
typedef unsigned long long counter_t;
#define COUNT() __builtin_ia32_rdtsc()
#define sync_core() asm volatile("lfence" ::: "memory")
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

int compare(char *a, char *b, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
		if (a[i] != b[i])
			return i;
	return -1;
}

#define BENCH(name, names, fn, arg)					\
	counter_t a, b, total_comp = 0, total_uncomp = 0;		\
	size_t orig_outlen = outlen;					\
	for (i = 0; i < N + 1; i++) {					\
	    outlen = orig_outlen;					\
	    sync_core();						\
            a = COUNT();						\
	    err = c_##name(map, size, out, &outlen, arg);		\
	    b = COUNT();						\
	    if (i > 0) 							\
		    total_comp += b - a;				\
	    sync_core();						\
	    if (err)							\
		    printf("%s: compression of %s failed: %d\n", names, fn, err); \
	    sync_core();						\
	    a = COUNT();						\
	    err = d_##name(out, outlen, buf2, size, arg);		\
	    b = COUNT();						\
            sync_core();						\
	    if (i > 0)							\
		    total_uncomp += b - a;				\
            if (err)							\
		    printf("%s: uncompression of %s failed: %d\n", names, fn, err); \
	    int o = compare(buf2, map, size); 				\
	    if (o >= 0)							\
		    printf("%s: final comparision failed at %d of %lu\n", names, o, (unsigned long)size); \
       }								\
       printf("%-6s: %s: %lu b: ratio %.02f: comp %3.02f uncomp %2.02f c/b\n", \
	      names, basen(fn), (unsigned long)size,			\
	      (double)outlen / size,					\
	      (double)(total_comp / N) / size,				\
	      (double)(total_uncomp / N) / size);		       


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

	BENCH(snappy, "snappy", fn, NULL);

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
	memset(lzo_wmem, 0, LZO1X_MEM_COMPRESS);

	BENCH(lzo, "lzo", fn, lzo_wmem);

	free(out);
	free(buf2);
}

struct state {
	struct z_stream_s comp;
	struct z_stream_s de;
};

static inline int c_zlib(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	struct state *s = a;
	int ret;
	s->comp.next_in = (unsigned char *)map;
	s->comp.avail_in = size;
	s->comp.total_in = 0;
	s->comp.next_out = (unsigned char *)out;
	s->comp.avail_out = *outlen;
	s->comp.total_out = 0;
	ret = zlib_deflate(&s->comp, Z_FINISH);
	if (ret != Z_STREAM_END)
		return ret;

	ret = zlib_deflateReset(&s->comp);
	if (ret != Z_OK) 
		return ret;

	*outlen = *outlen - s->comp.avail_out;
	return 0;
}

static inline int d_zlib(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	struct state *s = a;
	int ret;

	s->de.next_in = (unsigned char *)out;
	s->de.avail_in = outlen;
	s->de.next_out = (unsigned char *)buf2;
	s->de.avail_out = size;
	s->de.total_in = 0;
	s->de.total_out = 0;
	ret = zlib_inflate(&s->de, Z_FINISH);
	if (ret != Z_STREAM_END)
		return ret;
	ret = zlib_inflateReset(&s->de);
	if (ret != Z_OK) 
		return ret;
	return 0;
}

void test_zlib(char *map, size_t size, char *fn, int level)
{
	char name[30];
	int i;
	int err;       
	size_t outlen = size * 2; /* XXX? */
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);
	struct state state;
	memset(&state, 0, sizeof state);

	state.comp.workspace = calloc(zlib_deflate_workspacesize(MAX_WBITS, MAX_MEM_LEVEL), 1);
	
	err = zlib_deflateInit(&state.comp, level);
	if (err != Z_OK) {
		printf("zlib_deflateinit failed: %d\n", err);
		exit(1);
	}

	state.de.workspace = calloc(zlib_inflate_workspacesize(), 1);
	err = zlib_inflateInit(&state.de);
	if (err != Z_OK) {
		printf("zlib_inflateinit failed: %d\n", err);
		exit(1);
	}
	
	sprintf(name, "zlib%d", level);	
	BENCH(zlib, name, fn, &state);

	zlib_deflateEnd(&state.comp);
	zlib_inflateEnd(&state.de);

	free(state.de.workspace);
	free(state.comp.workspace);

	free(out);
	free(buf2);
}


static inline int c_lzf(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	size_t n;
	n = lzf_compress(map, size, out, *outlen);
	if (n == 0)
		return -1;
	*outlen = n;
	return 0;
}

static inline int d_lzf(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	if (lzf_decompress(out, outlen, buf2, size) == 0)
		return -1;
	return 0;
}

void test_lzf(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = size * 2;
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	BENCH(lzf, "lzf", fn, NULL);

	free(out);
	free(buf2);
}

static inline int c_quicklz(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	if (qlz_compress(map, out, size, a) == 0)
		return -1;

	*outlen = qlz_size_compressed(out);
	return 0;
}

static inline int d_quicklz(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	if (qlz_decompress(out, buf2, a) == 0)
		return -1;
	return 0;
}

void test_quicklz(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = size * 2;
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);
	qlz_state_compress state;
	memset(&state, 0, sizeof(qlz_state_compress));

	BENCH(quicklz, "qlz", fn, &state);

	free(out);
	free(buf2);
}


static inline int c_fastlz(char *map, size_t size, char *out, size_t *outlen, void *a)
{
	*outlen = fastlz_compress(map, size, out);
	return 0;
}

static inline int d_fastlz(char *out, size_t outlen, char *buf2, size_t size, void *a)
{
	if (fastlz_decompress(out, outlen, buf2, size) == 0)
		return -1;
	return 0;
}

void test_fastlz(char *map, size_t size, char *fn)
{
	int i;
	int err;       
	size_t outlen = size * 2;
	char *out = xmalloc(outlen);
	char *buf2 = xmalloc(size);

	BENCH(fastlz, "fastlz", fn, NULL);

	free(out);
	free(buf2);
}


int main(int ac, char **av)
{
#ifdef SIMPLE_PMU
	pin_cpu(NULL);
	if(perfmon_available() == 0) {
		printf("no perfmon support\n");
		exit(1);
	}
#endif

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
		test_zlib(map, size, *av, 1);
		test_zlib(map, size, *av, 3);
		//test_zlib(map, size, *av, 5);
		test_lzf(map, size, *av);
		test_quicklz(map, size, *av);
		test_fastlz(map, size, *av);
#endif		

		unmap_file(map, size);
		
	}
	return 0;
}
