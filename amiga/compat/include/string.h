/* ixemul 48.2's <string.h> plus strnlen (compat/posix.c). */
#ifndef AMIGA_COMPAT_STRING_H
#define AMIGA_COMPAT_STRING_H
#include_next <string.h>
size_t	strnlen(const char *, size_t);
#endif
