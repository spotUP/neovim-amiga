/* compat_probe: libamigacompat's AmigaOS parts on the Amiga -- the atomic
 * builtins gcc calls for C11 atomics (4- and 8-byte) and clock_gettime's
 * two clocks. Prints ok/FAIL lines, exits 0 when all pass. Run on the rig. */
#include <stdatomic.h>
#include <stdio.h>
#include <time.h>

static int fails;
static void check(int ok, const char *what)
{
	printf("%s %s\n", ok ? "ok" : "FAIL", what);
	fails += !ok;
}

int main(void)
{
	atomic_int b = 0;
	_Atomic unsigned long long q = 0;
	int e = 0;
	struct timespec m1, m2, r, res;
	volatile long spin;

	check(atomic_compare_exchange_strong(&b, &e, 7) && b == 7, "cas succeeds on the expected value");
	e = 3;
	check(!atomic_compare_exchange_strong(&b, &e, 9) && e == 7, "cas fails and reports the value");
	check(atomic_fetch_add(&b, 2) == 7 && b == 9, "fetch_add");
	check(atomic_exchange(&b, 1) == 9 && b == 1, "exchange");
	atomic_fetch_add(&q, 0x100000000ULL);
	check(q == 0x100000000ULL, "8-byte fetch_add");
	check(clock_gettime(CLOCK_MONOTONIC, &m1) == 0, "monotonic clock");
	for (spin = 0; spin < 200000; spin++)
		;
	clock_gettime(CLOCK_MONOTONIC, &m2);
	check(m2.tv_sec > m1.tv_sec || (m2.tv_sec == m1.tv_sec && m2.tv_nsec > m1.tv_nsec),
	    "monotonic clock advances");
	check(clock_gettime(CLOCK_REALTIME, &r) == 0 && r.tv_sec > 1000000000L, "realtime clock");
	check(clock_getres(CLOCK_MONOTONIC, &res) == 0 && res.tv_sec == 0 && res.tv_nsec > 0 &&
	    res.tv_nsec < 2000, "monotonic resolution (E-clock, about 1.4 us)");
	printf("   monotonic %ld.%09ld, resolution %ld ns\n", (long)m2.tv_sec, m2.tv_nsec, res.tv_nsec);
	printf("%s: %d failed\n", fails ? "FAIL" : "PASS", fails);
	return fails != 0;
}
