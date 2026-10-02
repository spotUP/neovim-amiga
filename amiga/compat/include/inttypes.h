/* <inttypes.h> for ixemul 48.2, which has none (the toolchain's fallback is
 * newlib's, whose types clash with ixemul's). Widths per ixemul's
 * machine/types.h: int32_t is int, int64_t long long, intptr_t long; its
 * printf takes ll, q, hh, j, z, t. strtoimax/strtoumax/imaxabs in
 * compat/posix.c. (Request R1.) */
#ifndef AMIGA_COMPAT_INTTYPES_H
#define AMIGA_COMPAT_INTTYPES_H
#pragma GCC system_header
#include <stdint.h>

#define PRId8	"d"
#define PRIi8	"i"
#define PRIo8	"o"
#define PRIu8	"u"
#define PRIx8	"x"
#define PRIX8	"X"
#define PRId16	"d"
#define PRIi16	"i"
#define PRIo16	"o"
#define PRIu16	"u"
#define PRIx16	"x"
#define PRIX16	"X"
#define PRId32	"d"
#define PRIi32	"i"
#define PRIo32	"o"
#define PRIu32	"u"
#define PRIx32	"x"
#define PRIX32	"X"
#define PRId64	"lld"
#define PRIi64	"lli"
#define PRIo64	"llo"
#define PRIu64	"llu"
#define PRIx64	"llx"
#define PRIX64	"llX"
#define PRIdMAX	"lld"
#define PRIiMAX	"lli"
#define PRIoMAX	"llo"
#define PRIuMAX	"llu"
#define PRIxMAX	"llx"
#define PRIXMAX	"llX"
#define PRIdPTR	"ld"
#define PRIiPTR	"li"
#define PRIoPTR	"lo"
#define PRIuPTR	"lu"
#define PRIxPTR	"lx"
#define PRIXPTR	"lX"
#define PRIdLEAST8	PRId8
#define PRIdLEAST16	PRId16
#define PRIdLEAST32	PRId32
#define PRIdLEAST64	PRId64
#define PRIuLEAST8	PRIu8
#define PRIuLEAST16	PRIu16
#define PRIuLEAST32	PRIu32
#define PRIuLEAST64	PRIu64
#define PRIdFAST8	PRId32
#define PRIdFAST16	PRId32
#define PRIdFAST32	PRId32
#define PRIdFAST64	PRId64
#define PRIuFAST8	PRIu32
#define PRIuFAST16	PRIu32
#define PRIuFAST32	PRIu32
#define PRIuFAST64	PRIu64

#define SCNd8	"hhd"
#define SCNu8	"hhu"
#define SCNd16	"hd"
#define SCNu16	"hu"
#define SCNx16	"hx"
#define SCNd32	"d"
#define SCNi32	"i"
#define SCNu32	"u"
#define SCNx32	"x"
#define SCNd64	"lld"
#define SCNi64	"lli"
#define SCNu64	"llu"
#define SCNx64	"llx"
#define SCNdMAX	"lld"
#define SCNuMAX	"llu"
#define SCNdPTR	"ld"
#define SCNuPTR	"lu"
#define SCNxPTR	"lx"

typedef struct { intmax_t quot, rem; } imaxdiv_t;
intmax_t	imaxabs(intmax_t);
imaxdiv_t	imaxdiv(intmax_t, intmax_t);
intmax_t	strtoimax(const char *, char **, int);
uintmax_t	strtoumax(const char *, char **, int);
#endif
