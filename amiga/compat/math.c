/* C89/C99 math functions ixemul.library 48.2 has no vector for (fmod and
 * round are libixcompat's). Declared in include/math.h. IEEE 754 doubles
 * (big-endian), read through a union. (Request R1.) */
#include <math.h>
#include <string.h>

union amiga_dbl {
	double d;
	struct { unsigned long hi, lo; } w;
};

int
__amiga_fpclassify(double x)
{
	union amiga_dbl u;
	unsigned long e;

	u.d = x;
	e = (u.w.hi >> 20) & 0x7ff;
	if (e == 0x7ff)
		return ((u.w.hi & 0xfffff) | u.w.lo) ? FP_NAN : FP_INFINITE;
	if (e == 0)
		return ((u.w.hi & 0xfffff) | u.w.lo) ? FP_SUBNORMAL : FP_ZERO;
	return FP_NORMAL;
}

int
__amiga_signbit(double x)
{
	union amiga_dbl u;

	u.d = x;
	return (u.w.hi >> 31) != 0;
}

double
copysign(double x, double y)
{
	union amiga_dbl ux, uy;

	ux.d = x;
	uy.d = y;
	ux.w.hi = (ux.w.hi & 0x7fffffffUL) | (uy.w.hi & 0x80000000UL);
	return ux.d;
}

/* toward zero; an integral, infinite or NaN value comes back as it is */
double
trunc(double x)
{
	if (fpclassify(x) <= FP_INFINITE)
		return x;
	return x < 0 ? -floor(-x) : floor(x);
}

/* C99: a NaN argument loses to a number */
double
fmin(double x, double y)
{
	if (x != x)
		return y;
	if (y != y)
		return x;
	return x < y ? x : y;
}

double
fmax(double x, double y)
{
	if (x != x)
		return y;
	if (y != y)
		return x;
	return x > y ? x : y;
}

/* x - n*y, n the integer nearest x/y, ties to even (C99) */
double
remainder(double x, double y)
{
	double q, n, f;

	if (y == 0.0 || x != x || y != y || fpclassify(x) == FP_INFINITE)
		return (x * y) / (x * y);	/* NaN */
	if (fpclassify(y) == FP_INFINITE)
		return x;
	q = x / y;
	n = floor(q + 0.5);
	f = n - q;
	if (f == 0.5 && fmod(n, 2.0) != 0.0)
		n -= 1.0;			/* tie: to the even neighbour */
	return x - n * y;
}

/* the angle of (x, y) in [-pi, pi], from atan by quadrant */
double
atan2(double y, double x)
{
	static const double pi = 3.14159265358979323846;

	if (x != x || y != y)
		return x + y;			/* NaN */
	if (x > 0)
		return atan(y / x);
	if (x < 0)
		return y >= 0 ? atan(y / x) + pi : atan(y / x) - pi;
	if (y > 0)
		return pi / 2;
	if (y < 0)
		return -pi / 2;
	return 0.0;				/* atan2(0, 0) */
}
