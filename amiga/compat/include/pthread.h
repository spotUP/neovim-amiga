/* <pthread.h> for ixemul 48.2, which has no threads (the toolchain's
 * fallback is another C library's, whose types clash with ixemul's). Only
 * the calls that mean something with one thread per process: the thread's
 * signal mask is the process's, pthread_self names the one thread, and
 * pthread_exit ends it, i.e. the process. Nothing to create, join or lock:
 * code that needs that has no business on this platform and fails to
 * compile here. (compat/posix.c; request R1.) */
#ifndef AMIGA_COMPAT_PTHREAD_H
#define AMIGA_COMPAT_PTHREAD_H
#pragma GCC system_header
#include <signal.h>
typedef int pthread_t;
int	pthread_sigmask(int, const sigset_t *, sigset_t *);
pthread_t	pthread_self(void);
int	pthread_equal(pthread_t, pthread_t);
void	pthread_exit(void *) __attribute__((noreturn));
#endif
