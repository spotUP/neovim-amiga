/* libuv's thread API on a platform with one thread per process (AmigaOS
 * with ixemul.library; see include/uv/nothreads.h). It replaces thread.c.
 *
 * Every primitive keeps its real state, and every call that would block a
 * multi-threaded program forever (locking a mutex this thread already holds,
 * waiting on a condition nobody else can signal, a semaphore at zero) aborts
 * with a message instead of hanging silently: with one thread such a wait is
 * a guaranteed deadlock, so it is a bug in the caller. The one wait that can
 * end on its own, uv_cond_timedwait, sleeps for its timeout. */

#include "uv.h"
#include "internal.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static void uv__nothreads_deadlock(const char* what) {
  fprintf(stderr, "libuv: %s would wait forever (no other thread)\n", what);
  abort();
}


int uv_thread_create(uv_thread_t* tid, void (*entry)(void* arg), void* arg) {
  (void) tid;
  (void) entry;
  (void) arg;
  return UV_ENOSYS;
}


int uv_thread_create_ex(uv_thread_t* tid,
                        const uv_thread_options_t* params,
                        void (*entry)(void* arg),
                        void* arg) {
  (void) params;
  return uv_thread_create(tid, entry, arg);
}


uv_thread_t uv_thread_self(void) {
  return 0;
}


int uv_thread_join(uv_thread_t* tid) {
  (void) tid;
  return UV_ESRCH;  /* no thread was ever created */
}


int uv_thread_equal(const uv_thread_t* t1, const uv_thread_t* t2) {
  return *t1 == *t2;
}


int uv_mutex_init(uv_mutex_t* mutex) {
  mutex->locked = 0;
  mutex->recursive = 0;
  return 0;
}


int uv_mutex_init_recursive(uv_mutex_t* mutex) {
  mutex->locked = 0;
  mutex->recursive = 1;
  return 0;
}


void uv_mutex_destroy(uv_mutex_t* mutex) {
  if (mutex->locked)
    abort();  /* pthread_mutex_destroy: EBUSY, which libuv aborts on */
}


void uv_mutex_lock(uv_mutex_t* mutex) {
  if (mutex->locked && !mutex->recursive)
    uv__nothreads_deadlock("uv_mutex_lock on a held mutex");
  mutex->locked++;
}


int uv_mutex_trylock(uv_mutex_t* mutex) {
  if (mutex->locked && !mutex->recursive)
    return UV_EBUSY;
  mutex->locked++;
  return 0;
}


void uv_mutex_unlock(uv_mutex_t* mutex) {
  if (mutex->locked == 0)
    abort();  /* unlocking a mutex nobody holds */
  mutex->locked--;
}


int uv_rwlock_init(uv_rwlock_t* rwlock) {
  rwlock->readers = 0;
  rwlock->writer = 0;
  return 0;
}


void uv_rwlock_destroy(uv_rwlock_t* rwlock) {
  if (rwlock->readers || rwlock->writer)
    abort();
}


void uv_rwlock_rdlock(uv_rwlock_t* rwlock) {
  if (rwlock->writer)
    uv__nothreads_deadlock("uv_rwlock_rdlock under a write lock");
  rwlock->readers++;
}


int uv_rwlock_tryrdlock(uv_rwlock_t* rwlock) {
  if (rwlock->writer)
    return UV_EBUSY;
  rwlock->readers++;
  return 0;
}


void uv_rwlock_rdunlock(uv_rwlock_t* rwlock) {
  if (rwlock->readers == 0)
    abort();
  rwlock->readers--;
}


void uv_rwlock_wrlock(uv_rwlock_t* rwlock) {
  if (rwlock->writer || rwlock->readers)
    uv__nothreads_deadlock("uv_rwlock_wrlock on a held lock");
  rwlock->writer = 1;
}


int uv_rwlock_trywrlock(uv_rwlock_t* rwlock) {
  if (rwlock->writer || rwlock->readers)
    return UV_EBUSY;
  rwlock->writer = 1;
  return 0;
}


void uv_rwlock_wrunlock(uv_rwlock_t* rwlock) {
  if (!rwlock->writer)
    abort();
  rwlock->writer = 0;
}


void uv_once(uv_once_t* guard, void (*callback)(void)) {
  if (*guard)
    return;
  *guard = 1;
  callback();
}


int uv_sem_init(uv_sem_t* sem, unsigned int value) {
  sem->count = value;
  return 0;
}


void uv_sem_destroy(uv_sem_t* sem) {
  (void) sem;
}


void uv_sem_post(uv_sem_t* sem) {
  sem->count++;
}


void uv_sem_wait(uv_sem_t* sem) {
  if (sem->count == 0)
    uv__nothreads_deadlock("uv_sem_wait at zero");
  sem->count--;
}


int uv_sem_trywait(uv_sem_t* sem) {
  if (sem->count == 0)
    return UV_EAGAIN;
  sem->count--;
  return 0;
}


int uv_cond_init(uv_cond_t* cond) {
  cond->unused = 0;
  return 0;
}


void uv_cond_destroy(uv_cond_t* cond) {
  (void) cond;
}


/* Nobody can be waiting: signalling wakes no one, as it would with no
 * waiters on a threaded platform. */
void uv_cond_signal(uv_cond_t* cond) {
  (void) cond;
}


void uv_cond_broadcast(uv_cond_t* cond) {
  (void) cond;
}


void uv_cond_wait(uv_cond_t* cond, uv_mutex_t* mutex) {
  (void) cond;
  (void) mutex;
  uv__nothreads_deadlock("uv_cond_wait");
}


/* Sleeps for the timeout with the mutex released, as a threaded waiter
 * would when nobody signals. A signal that ends the sleep early returns 0:
 * a spurious wakeup, which every condition wait may have, so callers
 * re-check their predicate either way. */
int uv_cond_timedwait(uv_cond_t* cond, uv_mutex_t* mutex, uint64_t timeout) {
  struct timeval tv;
  int r;

  (void) cond;
  tv.tv_sec = (long) (timeout / 1000000000u);
  tv.tv_usec = (long) ((timeout % 1000000000u) / 1000u);
  uv_mutex_unlock(mutex);
  r = select(0, NULL, NULL, NULL, &tv);
  uv_mutex_lock(mutex);
  if (r < 0 && errno == EINTR)
    return 0;
  return UV_ETIMEDOUT;
}


int uv_barrier_init(uv_barrier_t* barrier, unsigned int count) {
  struct _uv_barrier* b;

  if (barrier == NULL || count == 0)
    return UV_EINVAL;
  b = uv__malloc(sizeof(*b));
  if (b == NULL)
    return UV_ENOMEM;
  b->in = 0;
  b->out = 0;
  b->threshold = count;
  barrier->b = b;
  return 0;
}


/* The one thread is the last to arrive only when the barrier counts one. */
int uv_barrier_wait(uv_barrier_t* barrier) {
  if (barrier == NULL || barrier->b == NULL)
    return UV_EINVAL;
  if (barrier->b->threshold != 1)
    uv__nothreads_deadlock("uv_barrier_wait for more than one thread");
  return 1;
}


void uv_barrier_destroy(uv_barrier_t* barrier) {
  uv__free(barrier->b);
  barrier->b = NULL;
}


int uv_key_create(uv_key_t* key) {
  key->value = NULL;
  return 0;
}


void uv_key_delete(uv_key_t* key) {
  key->value = NULL;
}


void* uv_key_get(uv_key_t* key) {
  return key->value;
}


void uv_key_set(uv_key_t* key, void* value) {
  key->value = value;
}
