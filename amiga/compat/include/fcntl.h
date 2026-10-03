/* ixemul 48.2's <fcntl.h>. It has no O_NOFOLLOW; 0 here, so a program
 * that asks not to follow a symbolic link opens through it (AmigaOS soft
 * links are rare, and ixemul's open() cannot refuse them). Listed in the
 * ledger's "not implemented". (Request R1: a real O_NOFOLLOW.) */
#ifndef AMIGA_COMPAT_FCNTL_H
#define AMIGA_COMPAT_FCNTL_H
#pragma GCC system_header
#include_next <fcntl.h>
#ifndef O_NOFOLLOW
#define O_NOFOLLOW	0
#endif

/* POSIX 2008's F_DUPFD_CLOEXEC, which ixemul's fcntl() does not know:
 * fcntl() is routed through __amiga_fcntl (compat/posix.c), which does it
 * as F_DUPFD + F_SETFD FD_CLOEXEC (one thread: nothing can fork between)
 * and passes every other command to ixemul. */
#ifndef F_DUPFD_CLOEXEC
#define F_DUPFD_CLOEXEC	0x7f20
int	__amiga_fcntl(int, int, ...);
#define fcntl __amiga_fcntl
#endif
#endif
