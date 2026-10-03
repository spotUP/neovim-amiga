/* <sched.h> for ixemul 48.2, which has none (the toolchain's fallback is
 * newlib's, whose types clash). One task per process: sched_yield has no
 * thread of ours to yield to and returns at once (compat/posix.c); there
 * is no POSIX scheduling policy. (Request R1.) */
#ifndef AMIGA_COMPAT_SCHED_H
#define AMIGA_COMPAT_SCHED_H
#pragma GCC system_header
struct sched_param {
	int	sched_priority;
};
int	sched_yield(void);
#endif
