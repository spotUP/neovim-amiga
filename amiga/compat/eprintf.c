/* __eprintf: what ixemul's <assert.h> calls on a failed assertion. libgcc
 * has one, but it writes through newlib's _impure_ptr, which an ixemul
 * program does not have; this one uses ixemul's stderr. */
#include <stdio.h>
#include <stdlib.h>

void
__eprintf(const char *fmt, const char *file, int line, const char *expr)
{
	fprintf(stderr, fmt, file, (unsigned)line, expr);
	fflush(stderr);
	abort();
}
