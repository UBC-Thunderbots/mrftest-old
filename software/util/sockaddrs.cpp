// This warning is issued by the definition of INADDR_ANY.
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include "util/sockaddrs.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

in_addr_t get_inaddr_any() {
	return htonl(INADDR_ANY);
}

