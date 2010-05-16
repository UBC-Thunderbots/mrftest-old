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

#endif

