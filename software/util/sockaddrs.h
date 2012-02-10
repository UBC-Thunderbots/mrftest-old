#ifndef UTIL_SOCKADDRS_H
#define UTIL_SOCKADDRS_H

#include "util/noncopyable.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

/**
 * \brief Stores a set of address information structures returned from a \code getaddrinfo query.
 */
class AddrInfoSet : public NonCopyable {
	public:
		/**
		 * \brief Invokes \code getaddrinfo and stores the result.
		 *
		 * \param[in] node the node name to look up.
		 *
		 * \param[in] service the service name to look up.
		 *
		 * \param[in] hints the hints structure to use.
		 */
		AddrInfoSet(const char *node, const char *service, const addrinfo *hints);

		/**
		 * \brief Invokes \code freeaddrinfo.
		 */
		~AddrInfoSet();

		/**
		 * \brief Returns the first \c addrinfo structure.
		 *
		 * \return the first structure.
		 */
		const addrinfo *first() const;

	private:
		addrinfo *info;
};

/**
 * \brief Returns INADDR_ANY properly translated to network byte order, avoiding the "old style cast" warning issued by using the constant directly.
 *
 * \return \c htonl(INADDR_ANY).
 */
in_addr_t get_inaddr_any();

#endif

