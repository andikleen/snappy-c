#include <stdlib.h>
#include "snappy.h"
#include "map.h"
#include <stdio.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>

#define err(x) perror(x), exit(1)

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);
	if (!ptr) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}
	return ptr;
}

int compare(char *a, char *b, size_t size)
{
	int i;
	for (i = 0; i < size; i++)
		if (a[i] != b[i])
			return i;
	return -1;
}


int main(int ac, char **av)
{
	int failed = 0;

	while (*++av) { 
		size_t size;
		char *map = mapfile(*av, O_RDONLY, &size);
		if (!map) {
			if (size > 0) {
				perror(*av);
				failed = 1;
			}
			continue;
		}

		size_t outlen;
		int err;       
		char *out = xmalloc(snappy_max_compressed_length(size));
		char *buf2 = xmalloc(size);

		err = snappy_compress(map, size, out, &outlen);		
		if (err) {
			failed = 1;
			printf("compression of %s failed: %d\n", *av, err);
			goto next;
		}
		err = snappy_uncompress(out, outlen, buf2);

		if (err) {
			failed = 1;
			printf("uncompression of %s failed: %d\n", *av, err);
			goto next;
		}
		if (memcmp(buf2, map, size)) {
			int o = compare(buf2, map, size);			
			if (o >= 0) {
				failed = 1;
				printf("final comparision of %s failed at %d of %lu\n", *av, o, size);
			}
		}

	next:
		unmap_file(map, size);
		free(out);
		free(buf2);
	}
	return failed;
}
