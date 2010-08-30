#ifndef UTIL_SOCKADDRS_H
#define UTIL_SOCKADDRS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

/**
 * Allows easy access to different types of network socket address structures without the need for casts.
 */
union SockAddrs {
	/**
	 * The generic address structure, for use with functions like \c bind() and \c connect().
	 */
	sockaddr sa;

	/**
	 * The IPv4 address structure.
	 */
	sockaddr_in in;

	/**
	 * The UNIX-domain address structure.
	 */
	sockaddr_un un;
};

/**
 * Returns INADDR_ANY properly translated to network byte order, avoiding the "old style cast" warning issued by using the constant directly.
 *
 * \return \c htonl(INADDR_ANY).
 */
in_addr_t get_inaddr_any();

#endif

