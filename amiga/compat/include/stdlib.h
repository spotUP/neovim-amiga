/* ixemul 48.2's <stdlib.h> plus mkdtemp (compat/posix.c). */
#ifndef AMIGA_COMPAT_STDLIB_H
#define AMIGA_COMPAT_STDLIB_H
#pragma GCC system_header
#include_next <stdlib.h>
char	*mkdtemp(char *);
#endif
