#ifdef __FreeBSD__
#  include <sys/endian.h>
#elif defined(__APPLE_CC_) || defined(__MACH__)  /* MacOS/X support */
#  include <machine/endian.h>

#if    __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN
#  define	htole16(x) (x)
#  define	le32toh(x) (x)
#elif  __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN
#  define	htole16(x) __DARWIN_OSSwapInt16(x)
#  define	le32toh(x) __DARWIN_OSSwapInt32(x)
#else
#  error "Endianness is undefined"
#endif


#else
#  include <endian.h>
#endif

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/uio.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned u32;
typedef unsigned long long u64;

#define BUG_ON(x) assert(!(x))

#define get_unaligned(x) (*(x))
#define get_unaligned_le32(x) (le32toh(*(u32 *)(x)))
#define put_unaligned(v,x) (*(x) = (v))
#define put_unaligned_le16(v,x) (*(u16 *)(x) = htole16(v))

#define vmalloc(x) malloc(x)
#define vfree(x) free(x)

#define EXPORT_SYMBOL(x)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

#define min_t(t,x,y) ((x) < (y) ? (x) : (y))
#define max_t(t,x,y) ((x) > (y) ? (x) : (y))

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define __LITTLE_ENDIAN__ 1
#endif

#define BITS_PER_LONG (__SIZEOF_LONG__ * 8)
