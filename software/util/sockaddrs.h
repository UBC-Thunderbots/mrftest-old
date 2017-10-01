#ifndef UTIL_SOCKADDRS_H
#define UTIL_SOCKADDRS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "util/noncopyable.h"

/**
 * \brief Stores a set of address information structures returned from a \c
 * getaddrinfo query.
 */
class AddrInfoSet final : public NonCopyable
{
   public:
    /**
     * \brief Invokes \c getaddrinfo and stores the result.
     *
     * \param[in] node the node name to look up.
     *
     * \param[in] service the service name to look up.
     *
     * \param[in] hints the hints structure to use.
     */
    explicit AddrInfoSet(
        const char *node, const char *service, const addrinfo *hints);

    /**
     * \brief Invokes \c freeaddrinfo.
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
 * \brief Returns INADDR_ANY properly translated to network byte order, avoiding
 * the "old style cast" warning issued by using the constant directly.
 *
 * \return \c htonl(INADDR_ANY).
 */
in_addr_t get_inaddr_any();

#endif
