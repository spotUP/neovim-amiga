/* ixemul 48.2's <time.h> plus nanosleep (compat/posix.c). */
#ifndef AMIGA_COMPAT_TIME_H
#define AMIGA_COMPAT_TIME_H
#include_next <time.h>
#include <sys/time.h>
int	nanosleep(const struct timespec *, struct timespec *);
#endif
