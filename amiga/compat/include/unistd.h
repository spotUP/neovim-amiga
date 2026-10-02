/* ixemul 48.2's <unistd.h> plus POSIX calls it has no vectors for
 * (compat/posix.c). (Request R1: belong in the ixemul-vtcon SDK.) */
#ifndef AMIGA_COMPAT_UNISTD_H
#define AMIGA_COMPAT_UNISTD_H
#pragma GCC system_header
#include_next <unistd.h>
#include <sys/types.h>

/* no such sysconf name in ixemul: sysconf() answers -1 and callers fall
   back to their own buffer size, as POSIX allows */
#ifndef _SC_GETPW_R_SIZE_MAX
#define _SC_GETPW_R_SIZE_MAX	0x7f01
#endif
#ifndef _SC_GETGR_R_SIZE_MAX
#define _SC_GETGR_R_SIZE_MAX	0x7f02
#endif

ssize_t	pread(int, void *, size_t, off_t);
ssize_t	pwrite(int, const void *, size_t, off_t);
int	lchown(const char *, uid_t, gid_t);
int	ttyname_r(int, char *, size_t);
#endif
