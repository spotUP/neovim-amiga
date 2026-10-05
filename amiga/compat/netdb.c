/* RFC 3493 getaddrinfo / freeaddrinfo / gai_strerror / getnameinfo for
 * ixemul 48.2, which has only the 4.2BSD resolver calls. IPv4 only: the
 * stack under ixemul (bsdsocket.library) is IPv4. Declared in
 * compat/include/netdb.h. (Request R1: belongs in libixcompat.) */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;

static int
port_of(const char *serv, int socktype, int flags, unsigned short *port)
{
	char *end;
	long n;
	struct servent *se;

	*port = 0;
	if (serv == NULL || *serv == '\0')
		return 0;
	n = strtol(serv, &end, 10);
	if (*end == '\0') {
		if (n < 0 || n > 65535)
			return EAI_SERVICE;
		*port = htons((unsigned short)n);
		return 0;
	}
	if (flags & AI_NUMERICSERV)
		return EAI_NONAME;
	se = getservbyname(serv, socktype == SOCK_DGRAM ? "udp" : "tcp");
	if (se == NULL)
		return EAI_SERVICE;
	*port = (unsigned short)se->s_port;	/* network order already */
	return 0;
}

static struct addrinfo *
one(struct in_addr a, unsigned short port, int socktype, int protocol)
{
	struct addrinfo *ai;
	struct sockaddr_in *sin;

	ai = calloc(1, sizeof *ai + sizeof *sin);
	if (ai == NULL)
		return NULL;
	sin = (struct sockaddr_in *)(ai + 1);
	sin->sin_len = sizeof *sin;
	sin->sin_family = AF_INET;
	sin->sin_port = port;
	sin->sin_addr = a;
	ai->ai_family = AF_INET;
	ai->ai_socktype = socktype;
	ai->ai_protocol = protocol;
	ai->ai_addrlen = sizeof *sin;
	ai->ai_addr = (struct sockaddr *)sin;
	return ai;
}

void
freeaddrinfo(struct addrinfo *ai)
{
	struct addrinfo *next;

	for (; ai != NULL; ai = next) {
		next = ai->ai_next;
		free(ai->ai_canonname);
		free(ai);
	}
}

int
getaddrinfo(const char *node, const char *serv, const struct addrinfo *hints,
    struct addrinfo **res)
{
	static const int types[2] = { SOCK_STREAM, SOCK_DGRAM };
	struct addrinfo h, *head = NULL, **tail = &head, *ai;
	struct in_addr addrs[16];
	const char *canon = NULL;
	unsigned short port;
	int naddrs = 0, i, t, err;

	*res = NULL;
	memset(&h, 0, sizeof h);
	if (hints != NULL)
		h = *hints;
	if (h.ai_family != AF_UNSPEC && h.ai_family != AF_INET)
		return EAI_FAMILY;
	if (h.ai_socktype != 0 && h.ai_socktype != SOCK_STREAM &&
	    h.ai_socktype != SOCK_DGRAM)
		return EAI_SOCKTYPE;
	if (node == NULL && serv == NULL)
		return EAI_NONAME;

	if (node == NULL) {
		addrs[0].s_addr = htonl((h.ai_flags & AI_PASSIVE) ?
		    INADDR_ANY : INADDR_LOOPBACK);
		naddrs = 1;
	} else if (inet_aton(node, &addrs[0])) {
		naddrs = 1;
		canon = node;
	} else if (h.ai_flags & AI_NUMERICHOST) {
		return EAI_NONAME;
	} else {
		struct hostent *he = gethostbyname(node);

		if (he == NULL) {
			switch (h_errno) {
			case HOST_NOT_FOUND:	return EAI_NONAME;
			case TRY_AGAIN:		return EAI_AGAIN;
			case NO_DATA:		return EAI_NODATA;
			default:		return EAI_FAIL;
			}
		}
		if (he->h_addrtype != AF_INET)
			return EAI_FAMILY;
		for (i = 0; he->h_addr_list[i] != NULL && naddrs < 16; i++)
			memcpy(&addrs[naddrs++], he->h_addr_list[i],
			    sizeof addrs[0]);
		canon = he->h_name;
		if (naddrs == 0)
			return EAI_NODATA;
	}

	for (i = 0; i < naddrs; i++) {
		for (t = 0; t < 2; t++) {
			if (h.ai_socktype != 0 && h.ai_socktype != types[t])
				continue;
			if ((err = port_of(serv, types[t], h.ai_flags, &port))) {
				freeaddrinfo(head);
				return err;
			}
			ai = one(addrs[i], port, types[t], types[t] ==
			    SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP);
			if (ai == NULL) {
				freeaddrinfo(head);
				return EAI_MEMORY;
			}
			*tail = ai;
			tail = &ai->ai_next;
		}
	}
	if ((h.ai_flags & AI_CANONNAME) && head != NULL && canon != NULL &&
	    (head->ai_canonname = strdup(canon)) == NULL) {
		freeaddrinfo(head);
		return EAI_MEMORY;
	}
	*res = head;
	return 0;
}

AMIGA_GAI_CONST char *
gai_strerror(int e)
{
	switch (e) {
	case 0:			return "Success";
	case EAI_ADDRFAMILY:	return "Address family for name not supported";
	case EAI_AGAIN:		return "Temporary failure in name resolution";
	case EAI_BADFLAGS:	return "Invalid value for ai_flags";
	case EAI_FAIL:		return "Non-recoverable failure in name resolution";
	case EAI_FAMILY:	return "ai_family not supported";
	case EAI_MEMORY:	return "Memory allocation failure";
	case EAI_NODATA:	return "No address associated with name";
	case EAI_NONAME:	return "Name or service not known";
	case EAI_SERVICE:	return "Service not supported for ai_socktype";
	case EAI_SOCKTYPE:	return "ai_socktype not supported";
	case EAI_SYSTEM:	return strerror(errno);
	case EAI_OVERFLOW:	return "Argument buffer overflow";
	default:		return "Unknown error";
	}
}

static int
put(char *buf, socklen_t len, const char *s)
{
	if (strlen(s) >= (size_t)len)
		return EAI_OVERFLOW;
	strcpy(buf, s);
	return 0;
}

int
getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host,
    socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
	const struct sockaddr_in *sin = (const struct sockaddr_in *)sa;
	char num[16];
	int err;

	if (sa == NULL || sa->sa_family != AF_INET)
		return EAI_FAMILY;
	if (salen < (socklen_t)sizeof *sin)
		return EAI_FAMILY;

	if (host != NULL && hostlen > 0) {
		struct hostent *he = NULL;

		if (!(flags & NI_NUMERICHOST))
			he = gethostbyaddr((const char *)&sin->sin_addr,
			    sizeof sin->sin_addr, AF_INET);
		if (he != NULL) {
			if ((err = put(host, hostlen, he->h_name)))
				return err;
		} else if (flags & NI_NAMEREQD) {
			return EAI_NONAME;
		} else if ((err = put(host, hostlen,
		    inet_ntoa(sin->sin_addr)))) {
			return err;
		}
	}
	if (serv != NULL && servlen > 0) {
		struct servent *se = NULL;

		if (!(flags & NI_NUMERICSERV))
			se = getservbyport(sin->sin_port,
			    (flags & NI_DGRAM) ? "udp" : "tcp");
		if (se != NULL)
			return put(serv, servlen, se->s_name);
		snprintf(num, sizeof num, "%u", ntohs(sin->sin_port));
		return put(serv, servlen, num);
	}
	return 0;
}
