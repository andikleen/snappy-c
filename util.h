#include <stdio.h>

void *xmalloc(size_t size);
char *basen(char *s);
int compare(char *a, char *b, size_t size);


#define err(x) perror(x), exit(1)

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned  u32;
typedef unsigned long long u64;



