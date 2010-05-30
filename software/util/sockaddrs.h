#ifndef UTIL_SOCKADDRS_H
#define UTIL_SOCKADDRS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

union sockaddrs {
	sockaddr sa;
	sockaddr_in in;
	sockaddr_un un;
};

/**
 * Returns INADDR_ANY properly translated to network byte order, avoiding the
 * "old style cast" warning issued by using the constant directly.
 *
 * \return htonl(INADDR_ANY)
 */
in_addr_t get_inaddr_any();

#endif

