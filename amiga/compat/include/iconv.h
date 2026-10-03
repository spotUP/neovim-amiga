/* <iconv.h> for ixemul 48.2, which has no iconv. iconv_open() fails with
 * EINVAL for every pair (compat/posix.c): callers such as Neovim then use
 * their own conversions (Neovim converts UTF-8, Latin-1 and UCS-2/4 by
 * itself; other 'fileencoding's are not converted). A GNU libiconv
 * cross-build is the later, complete answer (ledger: not implemented). */
#ifndef AMIGA_COMPAT_ICONV_H
#define AMIGA_COMPAT_ICONV_H
#pragma GCC system_header
#include <stddef.h>
typedef void *iconv_t;
iconv_t	iconv_open(const char *, const char *);
size_t	iconv(iconv_t, char **, size_t *, char **, size_t *);
int	iconv_close(iconv_t);
#endif
