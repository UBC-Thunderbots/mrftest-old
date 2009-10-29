#ifndef UTIL_SOCKADDRS_H
#define UTIL_SOCKADDRS_H

#include <sys/socket.h>
#include <sys/un.h>

union sockaddrs {
	sockaddr sa;
	sockaddr_un un;
};

#endif

