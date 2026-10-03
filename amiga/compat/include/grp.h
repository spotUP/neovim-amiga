/* ixemul 48.2's <grp.h> plus getgrgid_r/getgrnam_r (compat/posix.c). */
#ifndef AMIGA_COMPAT_GRP_H
#define AMIGA_COMPAT_GRP_H
#pragma GCC system_header
#include_next <grp.h>
#include <sys/types.h>
int	getgrgid_r(gid_t, struct group *, char *, size_t, struct group **);
int	getgrnam_r(const char *, struct group *, char *, size_t, struct group **);
#endif
