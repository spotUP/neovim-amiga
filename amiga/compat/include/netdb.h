/* ixemul 48.2's <netdb.h> plus RFC 3493 getaddrinfo/getnameinfo, which it
 * lacks. netdb.c implements them on gethostbyname/gethostbyaddr and
 * getservbyname/getservbyport: IPv4 only (the stack under ixemul is IPv4).
 * Values are 4.4BSD/NetBSD's. (Request R1: belongs in libixcompat.) */
#ifndef AMIGA_COMPAT_NETDB_H
#define AMIGA_COMPAT_NETDB_H

#include_next <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

struct addrinfo {
	int		 ai_flags;
	int		 ai_family;
	int		 ai_socktype;
	int		 ai_protocol;
	socklen_t	 ai_addrlen;
	char		*ai_canonname;
	struct sockaddr	*ai_addr;
	struct addrinfo	*ai_next;
};

#define AI_PASSIVE	0x00000001
#define AI_CANONNAME	0x00000002
#define AI_NUMERICHOST	0x00000004
#define AI_NUMERICSERV	0x00000008
#define AI_ADDRCONFIG	0x00000400
#define AI_V4MAPPED	0x00000800
#define AI_ALL		0x00000100

#define NI_NOFQDN	0x00000001
#define NI_NUMERICHOST	0x00000002
#define NI_NAMEREQD	0x00000004
#define NI_NUMERICSERV	0x00000008
#define NI_DGRAM	0x00000010
#define NI_MAXHOST	1025
#define NI_MAXSERV	32

#define EAI_ADDRFAMILY	 1
#define EAI_AGAIN	 2
#define EAI_BADFLAGS	 3
#define EAI_FAIL	 4
#define EAI_FAMILY	 5
#define EAI_MEMORY	 6
#define EAI_NODATA	 7
#define EAI_NONAME	 8
#define EAI_SERVICE	 9
#define EAI_SOCKTYPE	10
#define EAI_SYSTEM	11
#define EAI_BADHINTS	12
#define EAI_PROTOCOL	13
#define EAI_OVERFLOW	14

int	 getaddrinfo(const char *, const char *, const struct addrinfo *,
	    struct addrinfo **);
void	 freeaddrinfo(struct addrinfo *);
const char *gai_strerror(int);
int	 getnameinfo(const struct sockaddr *, socklen_t, char *, socklen_t,
	    char *, socklen_t, int);

#endif
