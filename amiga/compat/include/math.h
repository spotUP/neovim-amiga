/* ixemul 48.2's <math.h> plus the C99 parts Neovim and Lua need: the
 * classification macros, INFINITY/NAN, and trunc/fmin/fmax/remainder/
 * copysign (compat/math.c; round and fmod are libixcompat's). ixemul has
 * isnan() and isinf() as functions; the macros below keep using them.
 * (Request R1.) */
#ifndef AMIGA_COMPAT_MATH_H
#define AMIGA_COMPAT_MATH_H
#pragma GCC system_header
#include_next <math.h>

#ifndef FP_NAN
#define FP_NAN		0
#define FP_INFINITE	1
#define FP_ZERO		2
#define FP_SUBNORMAL	3
#define FP_NORMAL	4
#endif
int	__amiga_fpclassify(double);
#ifndef fpclassify
#define fpclassify(x)	__amiga_fpclassify((double)(x))
#endif
#ifndef isfinite
#define isfinite(x)	(fpclassify(x) > FP_INFINITE)
#endif
#ifndef isnormal
#define isnormal(x)	(fpclassify(x) == FP_NORMAL)
#endif
#ifndef signbit
int	__amiga_signbit(double);
#define signbit(x)	__amiga_signbit((double)(x))
#endif
/* the SDK spells it 1e500, which overflows with a warning */
#undef HUGE_VAL
#define HUGE_VAL	(__builtin_huge_val())
#ifndef INFINITY
#define INFINITY	(__builtin_inff())
#endif
#ifndef NAN
#define NAN		(__builtin_nanf(""))
#endif

double	trunc(double);
double	round(double);
double	fmin(double, double);
double	fmax(double, double);
double	remainder(double, double);
double	copysign(double, double);
double	atan2(double, double);
#endif
