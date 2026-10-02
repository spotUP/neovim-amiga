/* ixemul 48.2's <net/if.h> plus if_indextoname (compat/posix.c);
 * if_nametoindex is libixcompat's. */
#ifndef AMIGA_COMPAT_NET_IF_H
#define AMIGA_COMPAT_NET_IF_H
#pragma GCC system_header
#include <sys/types.h>
#include <sys/socket.h>
#include_next <net/if.h>
#ifndef IF_NAMESIZE
#define IF_NAMESIZE	16
#endif
unsigned int	if_nametoindex(const char *);
char		*if_indextoname(unsigned int, char *);
#endif
