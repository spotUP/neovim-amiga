/* ixemul 48.2's <stdlib.h> plus mkdtemp and the C99 long long functions
 * (compat/posix.c). */
#ifndef AMIGA_COMPAT_STDLIB_H
#define AMIGA_COMPAT_STDLIB_H
#pragma GCC system_header
#include_next <stdlib.h>
char	*mkdtemp(char *);
long long	llabs(long long);
long long	atoll(const char *);
typedef struct { long long quot, rem; } lldiv_t;
lldiv_t	lldiv(long long, long long);
#endif
