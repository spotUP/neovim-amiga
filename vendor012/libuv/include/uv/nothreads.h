/* Thread types for a platform without threads (AmigaOS with ixemul.library,
 * which runs one task per process; the host test build defines
 * UV_NO_THREADS to exercise the same code). Implemented in
 * src/unix/nothreads.c: mutexes, rwlocks and once are real state checks,
 * a semaphore counts, a condition wait can only time out, and
 * uv_thread_create fails with UV_ENOSYS. */
#ifndef UV_NOTHREADS_H
#define UV_NOTHREADS_H

#include <signal.h>

#define UV_ONCE_INIT 0

typedef int uv_once_t;
typedef int uv_thread_t;
typedef struct { int locked; int recursive; } uv_mutex_t;
typedef struct { int readers; int writer; } uv_rwlock_t;
typedef struct { unsigned int count; } uv_sem_t;
typedef struct { int unused; } uv_cond_t;
typedef struct { void* value; } uv_key_t;

/* (pthread_sigmask -> sigprocmask is src/unix/internal.h's: a public
   header must not rename a libc function for its users) */

/* Static "dynamic" libraries (src/unix/dl.c on AmigaOS, or UV_STATIC_DL):
 * the program registers what it links, uv_dlopen/uv_dlsym serve it. */
typedef struct uv_static_sym_s {
  const char* name;
  void* addr;
} uv_static_sym_t;

typedef struct uv_static_lib_s {
  const char* file;               /* the file name uv_dlopen matches */
  const uv_static_sym_t* syms;    /* ends with { NULL, NULL } */
} uv_static_lib_t;

/* libs ends with { NULL, NULL }; it must stay valid (static) */
void uv_static_dl_register(const uv_static_lib_t* libs);

#endif /* UV_NOTHREADS_H */
