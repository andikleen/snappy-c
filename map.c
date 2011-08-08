#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "map.h"

#define roundup(x,y) (((x) + (y) - 1) & ~((y) - 1))

char *mapfile(char *file, int oflags, size_t *size)
{
	int fd = open(file, oflags, 0644);
	if (fd < 0) 
		return NULL;
	
	struct stat st;
	if (fstat(fd, &st) >= 0 && (*size = st.st_size) > 0) {
		size_t ps = sysconf(_SC_PAGE_SIZE);
		*size =  roundup(st.st_size, ps);
		char *map = mmap(NULL, *size, 
				 PROT_READ|((oflags & O_WRONLY)?PROT_WRITE:0), 
				 MAP_SHARED,
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

void unmap_file(char *map, size_t size)
{
	size_t ps = sysconf(_SC_PAGE_SIZE);
	munmap(map, roundup(size, ps));
}
