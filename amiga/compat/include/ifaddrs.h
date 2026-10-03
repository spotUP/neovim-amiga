/* <ifaddrs.h> for ixemul 48.2, which keeps no interface list: getifaddrs
 * fails with ENOSYS (compat/posix.c), so callers take their "no
 * interfaces known" path. (Request R1.) */
#ifndef AMIGA_COMPAT_IFADDRS_H
#define AMIGA_COMPAT_IFADDRS_H
#pragma GCC system_header
#include <sys/types.h>
#include <sys/socket.h>
struct ifaddrs {
	struct ifaddrs	*ifa_next;
	char		*ifa_name;
	unsigned int	 ifa_flags;
	struct sockaddr	*ifa_addr;
	struct sockaddr	*ifa_netmask;
	struct sockaddr	*ifa_dstaddr;
	void		*ifa_data;
};
#define ifa_broadaddr	ifa_dstaddr
int	getifaddrs(struct ifaddrs **);
void	freeifaddrs(struct ifaddrs *);
#endif
