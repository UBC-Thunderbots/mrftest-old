// This warning is issued by the definition of MAP_FAILED.
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include "util/misc.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

void *get_map_failed() {
	return MAP_FAILED;
}

cmsghdr *cmsg_firsthdr(msghdr *msgh) {
	return CMSG_FIRSTHDR(msgh);
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

