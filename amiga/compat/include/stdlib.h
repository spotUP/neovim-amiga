/* ixemul's <stdlib.h> plus mkdtemp and atoll (compat/posix.c). lldiv_t, lldiv and
 * llabs are the SDK's since libixcompat took them (ixemul-vtcon c15c438): its
 * <stdlib.h> declares them, so redeclaring lldiv_t here is a conflicting type. */
#ifndef AMIGA_COMPAT_STDLIB_H
#define AMIGA_COMPAT_STDLIB_H
#pragma GCC system_header
#include_next <stdlib.h>
char	*mkdtemp(char *);
long long	atoll(const char *);
#endif
