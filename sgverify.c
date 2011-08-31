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

#define REPEAT 100
#define N 1000

int main(int ac, char **av)
{
	struct snappy_env env;
	snappy_init_env_sg(&env, true);

	srand(1);

	while (*++av) { 
		int fd = open(*av, O_RDONLY);
		if (fd < 0) 
			err("open");

		size_t st_size;
		char *map = mapfile(*av, O_RDONLY, &st_size);

		int k;
		for (k = 0; k < REPEAT; k++) { 
			struct iovec in_iov[N];
			int iv = 0;
			size_t size = st_size;
			while (size > 0 && iv < N - 1) {
				size_t len = rand() % size;

				if (len > size)
					len = size;
				in_iov[iv].iov_base = malloc(len);
				in_iov[iv].iov_len = len;
				size -= len;
				memcpy(in_iov[iv].iov_base, map + st_size - size, 
				       len);
				iv++;
			}
			in_iov[iv].iov_base = malloc(size);
			in_iov[iv].iov_len = size;
			iv++;
				       		
			struct iovec out_iov[N];
			int ov = 0;
			size = snappy_max_compressed_length(st_size);
			while (size > 0 && ov < N - 1) {
				size_t len = rand() % size;
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

			size_t outlen;
		
			int err = snappy_compress_iov(&env, in_iov, iv, st_size, 
						      out_iov, ov, &outlen);
			if (err < 0) 
				printf("compression of %s failed: %d\n", *av, err);

			char *obuf = malloc(st_size);

			if (!snappy_uncompress_iov(out_iov, iv, outlen, obuf))
				printf("uncompression of %s failed\n", *av);
		
			if (!memcmp(obuf, map, st_size))
				printf("comparison of %s failed\n", *av);
			
			int w;
			for (w = 0; w < iv; w++) 
				free(in_iov[w].iov_base);
			for (w = 0; w < ov; w++)
				free(out_iov[w].iov_base);
		}

		unmap_file(map, st_size);
	}
	return 0;
}	
		
