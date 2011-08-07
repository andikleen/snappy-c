#define _GNU_SOURCE 1
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "cycles.h"
#include "map.h"
#include "snappy.h"

#ifdef COMP
#include "../comp/zlib.h"
#include "../comp/lzo/lzo.h"
#endif

#define err(x) perror(x), exit(1)
#define u8 unsigned char

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
				printf("compression of %s failed: %d\n", *av, err);
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
		       *av, (unsigned long)size, 
		       (double)outlen / size, 
		       (double)(total_comp / N) / size,
		       (double)(total_uncomp / N) / size);


		free(out);

#ifdef COMP		
		out = malloc(outlen = lzo1x_worst_compress(size));
		if (!out) exit(ENOMEM);

		char lzo_wmem[LZO1X_MEM_COMPRESS];

		/* warmup */
		size_t size2 = size;
		lzo1x_1_compress((u8 *)map, size, (u8 *)out, &outlen, lzo_wmem);
		lzo1x_decompress_safe((u8 *)out, outlen, (u8 *)buf2, &size2);
		
		for (i = 0; i < N; i++) { 
			outlen = lzo1x_worst_compress(size);

			sync_core();
			a = unhalted_core();
			err = lzo1x_1_compress((u8*)map, size, (u8*)out, &outlen, lzo_wmem);
			b = unhalted_core();
			total_comp += b - a;
			sync_core();

			if (err) 
				printf("compression of %s failed: %d\n", *av, 
				       err);
			sync_core();
			a = unhalted_core();
			size2 = size;
			err = lzo1x_decompress_safe((u8*)out, outlen, (u8*)buf2, &size2);
			b = unhalted_core();			
			sync_core();
			total_uncomp += b - a;
			if (err)
				printf("uncompression of %s failed: %d\n", 
				       *av, err);

			
		}
		
		printf("lzo: %s: %lu bytes: ratio %.02f: comp %.02f uncomp %.02f cycles/byte\n", 
		       *av, (unsigned long)size, 
		       (double)outlen / size, 
		       (double)(total_comp / N) / size,
		       (double)(total_uncomp / N) / size);

		free(out);
		
		
#endif
		
		free(buf2);
		unmap_file(map, size);
		
	}
	return 0;
}
