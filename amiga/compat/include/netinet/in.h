/* ixemul 48.2's <netinet/in.h> plus the RFC 3493 IPv6 types it lacks, so
 * IPv6-aware code (libuv) compiles. The stack under ixemul is IPv4 only:
 * socket(AF_INET6, ...) fails at run time, which is the honest answer.
 * Values are 4.4BSD/NetBSD's. (Request R1 in the ledger: belongs in the
 * ixemul-vtcon SDK.) */
#ifndef AMIGA_COMPAT_NETINET_IN_H
#define AMIGA_COMPAT_NETINET_IN_H
#pragma GCC system_header

#include_next <netinet/in.h>
#include <sys/types.h>

#ifndef AF_INET6
#define AF_INET6	28
#endif
#ifndef PF_INET6
#define PF_INET6	AF_INET6
#endif

#ifndef _AMIGA_COMPAT_IN_PORT_T
#define _AMIGA_COMPAT_IN_PORT_T
typedef u_int16_t	in_port_t;
typedef u_int32_t	in_addr_t;
#endif

struct in6_addr {
	union {
		u_int8_t	__u6_addr8[16];
		u_int16_t	__u6_addr16[8];
		u_int32_t	__u6_addr32[4];
	} __u6_addr;
};
#define s6_addr		__u6_addr.__u6_addr8

struct sockaddr_in6 {
	u_int8_t	sin6_len;
	u_int8_t	sin6_family;
	u_int16_t	sin6_port;
	u_int32_t	sin6_flowinfo;
	struct in6_addr	sin6_addr;
	u_int32_t	sin6_scope_id;
};

struct ipv6_mreq {
	struct in6_addr	ipv6mr_multiaddr;
	unsigned int	ipv6mr_interface;
};

#define IN6ADDR_ANY_INIT	{{{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 }}}
#define IN6ADDR_LOOPBACK_INIT	{{{ 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1 }}}
extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

#define INET_ADDRSTRLEN		16
#define INET6_ADDRSTRLEN	46

#define IPPROTO_IPV6		41
#define IPV6_UNICAST_HOPS	4
#define IPV6_MULTICAST_IF	9
#define IPV6_MULTICAST_HOPS	10
#define IPV6_MULTICAST_LOOP	11
#define IPV6_JOIN_GROUP		12
#define IPV6_LEAVE_GROUP	13
#define IPV6_ADD_MEMBERSHIP	IPV6_JOIN_GROUP
#define IPV6_DROP_MEMBERSHIP	IPV6_LEAVE_GROUP
#define IPV6_V6ONLY		27

#endif
