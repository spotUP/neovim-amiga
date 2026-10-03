/* <wctype.h> for ixemul 48.2, which has none (the toolchain's fallback is
 * newlib's, whose types clash with ixemul's). ixemul has no wide-character
 * classification at all: these are implemented on utf8proc's Unicode
 * tables in amiga/wide (libamigawide.a, the Neovim 0.12 build, which links
 * utf8proc anyway). Without that library a caller fails at link time,
 * never silently (Neovim 0.4.4 calls none of them). (Request R1.) */
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
