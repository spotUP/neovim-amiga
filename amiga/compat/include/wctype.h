/* <wctype.h> for ixemul 48.2, which has none (the toolchain's fallback is
 * newlib's, whose types clash with ixemul's). ixemul has no wide-character
 * classification at all, so these are declarations only: a caller links
 * against nothing and fails at link time, never silently. Neovim 0.4 uses
 * them only under __STDC_ISO_10646__/USE_WCHAR_FUNCTIONS, which ixemul does
 * not define; it has its own Unicode tables. (Request R1.) */
#ifndef AMIGA_COMPAT_WCTYPE_H
#define AMIGA_COMPAT_WCTYPE_H
#pragma GCC system_header
#include <wchar.h>
typedef int wctype_t;
typedef int wctrans_t;
int	iswalnum(wint_t);
int	iswalpha(wint_t);
int	iswcntrl(wint_t);
int	iswdigit(wint_t);
int	iswgraph(wint_t);
int	iswlower(wint_t);
int	iswprint(wint_t);
int	iswpunct(wint_t);
int	iswspace(wint_t);
int	iswupper(wint_t);
int	iswxdigit(wint_t);
wint_t	towlower(wint_t);
wint_t	towupper(wint_t);
#endif
