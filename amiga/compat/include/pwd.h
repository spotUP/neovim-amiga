/* ixemul 48.2's <pwd.h> plus getpwuid_r/getpwnam_r (compat/posix.c). */
#ifndef AMIGA_COMPAT_PWD_H
#define AMIGA_COMPAT_PWD_H
#include_next <pwd.h>
#include <sys/types.h>
int	getpwuid_r(uid_t, struct passwd *, char *, size_t, struct passwd **);
int	getpwnam_r(const char *, struct passwd *, char *, size_t, struct passwd **);
#endif
