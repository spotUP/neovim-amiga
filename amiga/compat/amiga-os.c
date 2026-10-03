/* The parts of libamigacompat that call AmigaOS itself (NDK headers only:
 * devices/timer.h has its own struct timeval, which must not meet
 * ixemul's in one file).
 *
 * - clock_gettime/clock_getres: CLOCK_MONOTONIC from timer.device's
 *   E-clock (about 709 kHz PAL / 716 kHz NTSC, 1.4 us), CLOCK_REALTIME from
 *   ixemul's gettimeofday (called through a tiny shim in posix.c).
 * - The gcc atomic builtins m68k-amigaos has no inline expansion for:
 *   gcc emits calls to __atomic_*_N / __sync_*_N (libatomic) for every
 *   read-modify-write, and there is no libatomic for this target. An ixemul
 *   process has one thread; what can interleave with it is a signal (an exec
 *   task exception) or an interrupt, and neither runs between Disable() and
 *   Enable(). The 68020's CAS is not used: it is a read-modify-write bus
 *   cycle, which the Amiga's chip bus does not carry. (Request R1.)
 */
#include <exec/types.h>
#include <exec/execbase.h>
#include <devices/timer.h>
#include <proto/exec.h>
/* our own timer.device base, file-local under its own name: the 0.4.4
 * libuv (vendor/libuv amiga-os.c) defines a global TimerBase */
#define __NOLIBBASE__
#define TimerBase amiga_compat_TimerBase
#include <proto/timer.h>
#include <string.h>
#include <stdlib.h>

/* ---- clocks --------------------------------------------------------- */

struct amiga_ts { long tv_sec; long tv_nsec; };	/* = ixemul's struct timespec */

static struct Device *TimerBase;
static struct timerequest amiga_tr;
static int amiga_tr_state;	/* 0 not tried, 1 open, -1 failed */

extern int __amiga_realtime(struct amiga_ts *);	/* posix.c */
extern int *__amiga_errno(void);		/* posix.c */

static void
amiga_timer_close(void)
{
	if (amiga_tr_state == 1) {
		CloseDevice((struct IORequest *)&amiga_tr);
		amiga_tr_state = 0;
		TimerBase = NULL;
	}
}

/* timer.device is opened once, for ReadEClock; no I/O is ever sent */
static int
amiga_eclock(unsigned long long *ticks, unsigned long *hz)
{
	struct EClockVal ev;

	if (amiga_tr_state == 0) {
		memset(&amiga_tr, 0, sizeof amiga_tr);
		if (OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_ECLOCK,
		    (struct IORequest *)&amiga_tr, 0) == 0) {
			TimerBase = amiga_tr.tr_node.io_Device;
			amiga_tr_state = 1;
			atexit(amiga_timer_close);
		} else
			amiga_tr_state = -1;
	}
	if (amiga_tr_state != 1)
		return -1;
	*hz = ReadEClock(&ev);
	*ticks = ((unsigned long long)ev.ev_hi << 32) | ev.ev_lo;
	return 0;
}

#define AMIGA_EINVAL 22
#define AMIGA_ENOSYS 78

int
clock_gettime(int clk, struct amiga_ts *ts)
{
	unsigned long long t;
	unsigned long hz;

	if (clk == 0)
		return __amiga_realtime(ts);
	if (clk != 1) {
		*__amiga_errno() = AMIGA_EINVAL;
		return -1;
	}
	if (amiga_eclock(&t, &hz) != 0) {
		*__amiga_errno() = AMIGA_ENOSYS;
		return -1;
	}
	ts->tv_sec = (long)(t / hz);
	ts->tv_nsec = (long)(((t % hz) * 1000000000ULL) / hz);
	return 0;
}

int
clock_getres(int clk, struct amiga_ts *ts)
{
	unsigned long long t;
	unsigned long hz;

	if (clk == 0) {
		ts->tv_sec = 0;
		ts->tv_nsec = 20000000;	/* gettimeofday: the 50 Hz tick */
		return 0;
	}
	if (clk != 1 || amiga_eclock(&t, &hz) != 0) {
		*__amiga_errno() = clk != 1 ? AMIGA_EINVAL : AMIGA_ENOSYS;
		return -1;
	}
	ts->tv_sec = 0;
	ts->tv_nsec = (long)((1000000000UL + hz - 1) / hz);
	return 0;
}

/* ---- atomics -------------------------------------------------------- */

#define ATOMIC_N(N, T)							\
T __atomic_load_##N(const volatile void *p, int m)			\
{ T v; (void)m; Disable(); v = *(const volatile T *)p; Enable(); return v; } \
void __atomic_store_##N(volatile void *p, T v, int m)			\
{ (void)m; Disable(); *(volatile T *)p = v; Enable(); }		\
T __atomic_exchange_##N(volatile void *p, T v, int m)			\
{ T o; (void)m; Disable(); o = *(volatile T *)p; *(volatile T *)p = v; \
  Enable(); return o; }						\
_Bool __atomic_compare_exchange_##N(volatile void *p, void *e, T d,	\
    _Bool w, int s, int f)						\
{ int ok; (void)w; (void)s; (void)f; Disable();			\
  if (*(volatile T *)p == *(T *)e) { *(volatile T *)p = d; ok = 1; }	\
  else { *(T *)e = *(volatile T *)p; ok = 0; }			\
  Enable(); return ok; }						\
T __atomic_fetch_add_##N(volatile void *p, T v, int m)			\
{ T o; (void)m; Disable(); o = *(volatile T *)p; *(volatile T *)p = o + v; \
  Enable(); return o; }						\
T __atomic_fetch_sub_##N(volatile void *p, T v, int m)			\
{ T o; (void)m; Disable(); o = *(volatile T *)p; *(volatile T *)p = o - v; \
  Enable(); return o; }						\
T __atomic_fetch_and_##N(volatile void *p, T v, int m)			\
{ T o; (void)m; Disable(); o = *(volatile T *)p; *(volatile T *)p = o & v; \
  Enable(); return o; }						\
T __atomic_fetch_or_##N(volatile void *p, T v, int m)			\
{ T o; (void)m; Disable(); o = *(volatile T *)p; *(volatile T *)p = o | v; \
  Enable(); return o; }						\
T __atomic_add_fetch_##N(volatile void *p, T v, int m)			\
{ T n; (void)m; Disable(); n = *(volatile T *)p + v; *(volatile T *)p = n; \
  Enable(); return n; }						\
T __atomic_sub_fetch_##N(volatile void *p, T v, int m)			\
{ T n; (void)m; Disable(); n = *(volatile T *)p - v; *(volatile T *)p = n; \
  Enable(); return n; }						\
T __sync_val_compare_and_swap_##N(volatile void *p, T o, T d)		\
{ T v; Disable(); v = *(volatile T *)p; if (v == o) *(volatile T *)p = d; \
  Enable(); return v; }						\
_Bool __sync_bool_compare_and_swap_##N(volatile void *p, T o, T d)	\
{ int ok; Disable(); ok = *(volatile T *)p == o; if (ok) *(volatile T *)p = d; \
  Enable(); return ok; }						\
T __sync_fetch_and_add_##N(volatile void *p, T v)			\
{ T o; Disable(); o = *(volatile T *)p; *(volatile T *)p = o + v;	\
  Enable(); return o; }						\
T __sync_fetch_and_sub_##N(volatile void *p, T v)			\
{ T o; Disable(); o = *(volatile T *)p; *(volatile T *)p = o - v;	\
  Enable(); return o; }						\
T __sync_add_and_fetch_##N(volatile void *p, T v)			\
{ T n; Disable(); n = *(volatile T *)p + v; *(volatile T *)p = n;	\
  Enable(); return n; }						\
T __sync_sub_and_fetch_##N(volatile void *p, T v)			\
{ T n; Disable(); n = *(volatile T *)p - v; *(volatile T *)p = n;	\
  Enable(); return n; }						\
T __sync_lock_test_and_set_##N(volatile void *p, T v)			\
{ T o; Disable(); o = *(volatile T *)p; *(volatile T *)p = v;		\
  Enable(); return o; }

ATOMIC_N(1, unsigned char)
ATOMIC_N(2, unsigned short)
ATOMIC_N(4, unsigned int)
ATOMIC_N(8, unsigned long long)

void
__sync_synchronize(void)
{
}
