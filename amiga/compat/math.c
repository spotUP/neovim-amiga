/* C89 math functions ixemul.library 48.2 has no vector for (fmod is
 * libixcompat's). (Request R1.) */
#include <math.h>

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
