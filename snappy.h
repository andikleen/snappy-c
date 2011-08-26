#ifndef _LINUX_SNAPPY_H
#define _LINUX_SNAPPY_H 1

#include <stdbool.h>
#include <stddef.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;
typedef unsigned long long u64;

/* Only needed for compression. This preallocates the worst case */
struct snappy_env {
	u16 *hash_table;
	void *scratch;
	void *scratch_output;
};

int snappy_init_env(struct snappy_env *env);
void snappy_free_env(struct snappy_env *env);
bool snappy_uncompress(const char *compressed, size_t n, char *uncompressed);
int snappy_compress(struct snappy_env *env,
		    const char *input,
		    size_t input_length,
		    char *compressed,
		    size_t *compressed_length);
bool snappy_uncompressed_length(const char *buf, size_t len, size_t *result);
size_t snappy_max_compressed_length(size_t source_len);



#endif
