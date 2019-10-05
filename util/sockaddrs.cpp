// This warning is issued by the definition of INADDR_ANY.
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include "util/sockaddrs.h"
#include <sys/types.h>
#include "util/exception.h"

AddrInfoSet::AddrInfoSet(
    const char *node, const char *service, const addrinfo *hints)
{
    info   = nullptr;
    int rc = getaddrinfo(node, service, hints, &info);
    if (rc != 0)
    {
        throw EAIError("getaddrinfo", rc);
    }
}

AddrInfoSet::~AddrInfoSet()
{
    freeaddrinfo(info);
}

const addrinfo *AddrInfoSet::first() const
{
    return info;
}

in_addr_t get_inaddr_any()
{
    return htonl(INADDR_ANY);
}
