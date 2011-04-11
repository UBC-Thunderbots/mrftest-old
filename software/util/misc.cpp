// This warning is issued by the definitions of these macros.
#pragma GCC diagnostic ignored "-Wold-style-cast"

// This macro causes the GNU strerror_r to be defined instead of the XSI one.
#undef _GNU_SOURCE

#include "util/misc.h"
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

void *get_map_failed() {
	return MAP_FAILED;
}

cmsghdr *cmsg_firsthdr(msghdr *msgh) {
	return CMSG_FIRSTHDR(msgh);
}

cmsghdr *cmsg_nxthdr(msghdr *msgh, cmsghdr *cmsgh) {
	return CMSG_NXTHDR(msgh, cmsgh);
}

size_t cmsg_space(size_t length) {
	return CMSG_SPACE(length);
}

size_t cmsg_len(size_t length) {
	return CMSG_LEN(length);
}

void *cmsg_data(cmsghdr *cmsg) {
	return CMSG_DATA(cmsg);
}

bool wifexited(int status) {
	return !!WIFEXITED(status);
}

int wexitstatus(int status) {
	return WEXITSTATUS(status);
}

int xsi_strerror_r(int err, char *buf, size_t buflen) {
	return strerror_r(err, buf, buflen);
}

