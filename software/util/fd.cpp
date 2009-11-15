#include "util/fd.h"
#include <stdexcept>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

file_descriptor::file_descriptor(const char *file, int flags) : fd(open(file, flags, 0666)) {
	if (fd < 0) throw std::runtime_error("Cannot open file!");
	std::printf("constructed file %d\n", fd);
}

file_descriptor::file_descriptor(int af, int type, int proto) : fd(socket(af, type, proto)) {
	if (fd < 0) throw std::runtime_error("Cannot create socket!");
	std::printf("constructed socket %d\n", fd);
}

void file_descriptor::close() {
	if (fd != -1) {
		std::printf("closed descriptor %d\n", fd);
		::close(fd);
		fd = -1;
	}
}

void file_descriptor::set_blocking(bool block) const {
	long flags = fcntl(fd, F_GETFL);
	if (flags < 0) throw std::runtime_error("Cannot get file descriptor flags!");
	if (block)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) throw std::runtime_error("Cannot set file descriptor flags!");
}

