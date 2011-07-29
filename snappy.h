#include <stdbool.h>
#include <stdint.h>

#define uint32 uint32_t
#define uint16 uint16_t
#define uint64 uint64_t

bool snappy_uncompress(const char* compressed, size_t n, char* uncompressed) ;
int snappy_compress(const char* input,
		    size_t input_length,
		    char* compressed,
		    size_t* compressed_length);
bool snappy_uncompressed_length(const char *buf, size_t len, size_t* result);
size_t snappy_max_compressed_length(size_t source_len);


