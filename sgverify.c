/* Test sg interfaces */

#ifdef SG

#include <assert.h>
#include <sys/uio.h>
#include <stdlib.h>
#include "snappy.h"
#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include "util.h"
#include <sys/stat.h>
#include "map.h"

#define REPEAT 20
#define N 5000

size_t sum_iov(struct iovec *iov, int n)
{
	size_t len = 0;
	int i;
	for (i = 0; i < n; i++)
		len += iov[i].iov_len;
	return len;
}

static int rnd_seq;

int rand_seq(void)
{	
	++rnd_seq;
	return rand();
}

void *iov_to_buf(struct iovec *iov, int n, size_t *len)
{
	*len = sum_iov(iov, n);
	void *p = malloc(*len);
	int i;
	unsigned offset = 0;
	for (i = 0; i < n; i++) {
		memcpy(p + offset, iov[i].iov_base, iov[i].iov_len);
		offset += iov[i].iov_len;
	}
	return p;
}

int main(int ac, char **av)
{
	struct snappy_env env;
	snappy_init_env_sg(&env, true);

	srand(99);
#if 0
	int j;
	for (j = 0; j < 60016; j++)
		rand_seq();
#endif

	while (*++av) { 
		size_t st_size;
		char *map = mapfile(*av, O_RDONLY, &st_size);
		if (!map) {
			perror(*av);
			continue;
		}

		int k;
		for (k = 0; k < REPEAT; k++) { 
			struct iovec in_iov[N];
			int iv = 0;
			size_t size = st_size;
			size_t offset = 0;

			unsigned rnd_seq_start = rnd_seq;

			while (size > 0 && iv < N - 1) {
				size_t len = rand_seq() % size + 1;

				if (len > size)
					len = size;
				in_iov[iv].iov_base = malloc(len);
				in_iov[iv].iov_len = len;
				memcpy(in_iov[iv].iov_base, 
				       map + offset,
				       len);
				size -= len;
				offset += len;
				iv++;
			}
			in_iov[iv].iov_base = malloc(size);
			in_iov[iv].iov_len = size;
			iv++;
			
			assert (sum_iov(in_iov, iv) == st_size);		
				       		
			struct iovec out_iov[N];
			int ov = 0;
			size = snappy_max_compressed_length(st_size);
			while (size > 0 && ov < N - 1) {
				size_t len = rand_seq() % size + 1;
				if (len > size)
					len = size;
				out_iov[ov].iov_len = len;
				out_iov[ov].iov_base = malloc(len);
				size -= len;
				ov++;
			}
			out_iov[ov].iov_base = malloc(size);
			out_iov[ov].iov_len = size;
			ov++;
			
			assert (sum_iov(out_iov, ov) == 
				snappy_max_compressed_length(st_size));

			size_t outlen;
		
			int err = snappy_compress_iov(&env, in_iov, iv, st_size, 
						      out_iov, ov, &outlen);
			if (err < 0) 
				printf("compression of %s failed: %d\n", *av, err);

			char *obuf = malloc(st_size);

			if (!snappy_uncompress_iov(out_iov, ov, outlen, obuf))
				printf("uncompression of %s failed\n", *av);
		
			if (memcmp(obuf, map, st_size)) {
				printf("comparison of %s failed, olen %lu, orig %lu, rnd_seq %d\n", *av,
				       outlen, st_size, rnd_seq_start);
				int j;
				for (j = 0; j < st_size; j++)
					if (obuf[j] != map[j]) {
						printf("%d: %x vs %x\n",
						       j, 
						       ((unsigned char *)obuf)[j],
						       ((unsigned char *)map)[j]);
					}
			}
			
			int w;
			for (w = 0; w < iv; w++) 
				free(in_iov[w].iov_base);
			for (w = 0; w < ov; w++)
				free(out_iov[w].iov_base);
			free(obuf);
		}

		unmap_file(map, st_size);
	}
	return 0;
}	
	
#else
int main(void) { return 0; } 	
#endif
