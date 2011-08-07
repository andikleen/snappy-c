#define _GNU_SOURCE 1
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "cycles.h"
#include "map.h"
#include "snappy.h"

#define err(x) perror(x), exit(1)

int main(int ac, char **av)
{
	pin_cpu(NULL);
	if(perfmon_available() == 0) {
		printf("no perfmon support\n");
		exit(1);
	}

	while (*++av) { 
		int err;
		size_t size;
		char *map = mapfile(*av, O_RDONLY, &size);
		if (!map)
			err(*av);

		char *out = malloc(snappy_max_compressed_length(size));
		if (!out)
			exit(ENOMEM);

		char *buf2 = malloc(size);
		if (!buf2)
			exit(ENOMEM);
		
		int i, v;
		for (i = 0; i < size; i += 4096)
			v = ((volatile char *)map)[i];

		/* warmup */
		size_t outlen;
		err = snappy_compress(map, size, out, &outlen);
		if (err) 
			printf("compression of %s failed: %d\n", *av, err);
		
		err = snappy_uncompress(out, outlen, buf2);
		if (err)
			printf("uncompression of %s failed: %d\n", *av, err);
		
		counter_t a, b, total_comp = 0, total_uncomp = 0;

#define N 10
		for (i = 0; i < N; i++) { 
			sync_core();
			a = unhalted_core();
			int err = snappy_compress(map, size, out, &outlen);
			b = unhalted_core();
			total_comp += b - a;
			sync_core();

			if (err) 
				printf("compression of %s failed: %d", *av, err);
			sync_core();
			a = unhalted_core();
			err = snappy_uncompress(out, outlen, buf2);
			b = unhalted_core();			
			sync_core();
			total_uncomp += b - a;
			if (err)
				printf("uncompression of %s failed: %d\n", *av, err);

			
		}
		
		printf("snappy: %s: %lu bytes: ratio %.02f: comp %.02f uncomp %.02f cycles/byte\n", 
		       *av, size, 
		       (double)outlen / size, 
		       (double)(total_comp / N) / size,
		       (double)(total_uncomp / N) / size);

		
		
	}
	return 0;
}
