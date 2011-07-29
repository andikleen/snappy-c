/* Simple command line tool for snappy */
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "snappy.h"

char *mapfile(char *file, size_t *size)
{
	int fd = open(file, O_RDONLY);
	if (fd < 0) 
		return NULL;
	
	struct stat st;
	if (fstat(fd, &st) >= 0) {
		size_t ps = sysconf(_SC_PAGE_SIZE);
		*size = (st.st_size + ps - 1) & ~(ps - 1);
		
		char *map = mmap(NULL, *size, PROT_READ, MAP_SHARED,
				 fd, 0);
		close(fd);
		if (map == (char *)-1)
			return NULL;
		*size = st.st_size;
		return map;		
	} 
	close(fd);
	return NULL;		
}

void usage(void)
{
	fprintf(stderr, 
		"scmd [-c|-d] [file] [outfile]\n"
		"Compress or uncompress file with snappy. Default is uncompression.\n"
		"When no file is specified read from standard input.\n"
		"When no output file is specified write to standard output\n");
	exit(1);
}

int main(int ac, char **av)
{
	enum { compress, uncompress } mode = uncompress;
	int opt;

	while ((opt = getopt(ac, av, "dc")) != -1) {
		switch (opt) { 
		case 'd':
			mode = uncompress;
			break;
		case 'c':
			mode = compress;
			break;
		default:
			usage();
		}
	}

	char *map;
	size_t size;
	if (!av[optind])
		usage();

	map = mapfile(av[optind], &size);
	if (!map) { 
		fprintf(stderr, "Cannot open %s: %s\n", av[1], strerror(errno));
		exit(1);
	}
	
	
	int err;
	char *out;	
	size_t outlen;
	if (mode == uncompress) {
		if (!snappy_uncompressed_length(map, size, &outlen)) {
			fprintf(stderr, "Cannot read length in %s\n", av[optind]);
			exit(1);
		}
	} else {	
		outlen = snappy_max_compressed_length(size);
	}
	
	out = malloc(outlen);
	if (!out)
		exit(ENOMEM);
	
	if (mode == compress) 
		err = snappy_compress(map, size, out, &outlen);
	else
		err = snappy_uncompress(map, size, out);

	if (err) {
		fprintf(stderr, "Cannot process %s\n", av[optind]);
		exit(1);
	}

	char *file;
	int fd;
	if (av[optind + 1]) {
		if (av[optind + 2])
			usage();
		file = av[optind + 1];
		fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (fd < 0) { 
			fprintf(stderr, "Cannot create %s: %s\n", file,
				strerror(errno));
			exit(1);
		}		
	} else {
		file = "<stdout>";
		fd = 1;
	}

	err = 0;
	if (write(fd, out, outlen) != outlen) {
		fprintf(stderr, "Cannot write to %s: %s\n", 
			file,
			strerror(errno));
		err = 1;
	}

	return err;
}
