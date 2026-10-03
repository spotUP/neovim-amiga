/* ixemul 48.2's <time.h> plus nanosleep (compat/posix.c). */
#ifndef AMIGA_COMPAT_TIME_H
#define AMIGA_COMPAT_TIME_H
#pragma GCC system_header
#include_next <time.h>
#include <sys/time.h>
int	nanosleep(const struct timespec *, struct timespec *);

/* POSIX clocks (compat/amiga-os.c): CLOCK_MONOTONIC is timer.device's
   E-clock (1.4 us), CLOCK_REALTIME is gettimeofday() */
#ifndef CLOCK_REALTIME
typedef int clockid_t;
#define CLOCK_REALTIME	0
#define CLOCK_MONOTONIC	1
#endif
int	clock_gettime(clockid_t, struct timespec *);
int	clock_getres(clockid_t, struct timespec *);
#endif
