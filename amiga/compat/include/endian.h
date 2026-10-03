/* <endian.h> (glibc/BSD style) for ixemul 48.2, which has none. The 68k is
 * big-endian: host order is big-endian order. (Request R1.) */
#ifndef AMIGA_COMPAT_ENDIAN_H
#define AMIGA_COMPAT_ENDIAN_H
#pragma GCC system_header
#include <stdint.h>

#define __LITTLE_ENDIAN	1234
#define __BIG_ENDIAN	4321
#define __PDP_ENDIAN	3412
#define __BYTE_ORDER	__BIG_ENDIAN
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN	__LITTLE_ENDIAN
#define BIG_ENDIAN	__BIG_ENDIAN
#define PDP_ENDIAN	__PDP_ENDIAN
#define BYTE_ORDER	__BYTE_ORDER
#endif

#define htobe16(x) ((uint16_t)(x))
#define htobe32(x) ((uint32_t)(x))
#define htobe64(x) ((uint64_t)(x))
#define be16toh(x) ((uint16_t)(x))
#define be32toh(x) ((uint32_t)(x))
#define be64toh(x) ((uint64_t)(x))
#define htole16(x) __builtin_bswap16((uint16_t)(x))
#define htole32(x) __builtin_bswap32((uint32_t)(x))
#define htole64(x) __builtin_bswap64((uint64_t)(x))
#define le16toh(x) __builtin_bswap16((uint16_t)(x))
#define le32toh(x) __builtin_bswap32((uint32_t)(x))
#define le64toh(x) __builtin_bswap64((uint64_t)(x))
#endif
