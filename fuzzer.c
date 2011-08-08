#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include "map.h"

int seed;
int bits = 1;
int fuzz = 20;

void fuzzfile(char *fn)
{
	size_t size;

	char *file = mapfile_flag(fn, O_RDWR, &size, MAP_PRIVATE);
	if (!file)
		return;
	
	int i;
	for (i = 0; i < fuzz; i++) {
		unsigned r = rand();

		int mode = r & 3;
		r >>= 2;
		int offset = r % (size*8);

		int w;
		switch (mode) { 
		case 0:
		case 1: 
			for (w = offset; w < offset + bits; w++)
				file[w / 8] ^= (1U << (w%8));
			break;
		case 2: 
			for (w = offset; w < offset + bits; w++)
				file[w / 8] |= (1U << (w%8));
			break;
		case 3:  {
			unsigned val = rand();
			for (w = offset; w < offset + bits; w++)
				file[w / 8] = (file[w / 8] & ~(1U << (w%8))) |
					      ((val >> (w-offset)) << (w%8));
			break;
		}
		}	
	}
	munmap(file, size);
}

int main(int ac, char **av)
{
	int opt;

	seed = time(0);
	while ((opt = getopt(ac, av, "f:s:b:")))
		switch (opt) {
		case 'f':
			fuzz = atoi(optarg);
			break;
		default:
			exit(1);
		}

	printf("seed %u\n", seed);
	srand(seed);

	for (; av[optind]; optind++)
		fuzzfile(av[optind]);

	return 0;
}
