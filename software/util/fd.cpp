#include "util/fd.h"
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

FileDescriptor::FileDescriptor(const char *file, int flags) : fd(open(file, flags, 0666)) {
	if (fd < 0) throw std::runtime_error("Cannot open file!");
}

FileDescriptor::FileDescriptor(int af, int type, int proto) : fd(socket(af, type, proto)) {
	if (fd < 0) throw std::runtime_error("Cannot create socket!");
}

void FileDescriptor::close() {
	if (fd != -1) {
		::close(fd);
		fd = -1;
	}
}

void FileDescriptor::set_blocking(bool block) const {
	long flags = fcntl(fd, F_GETFL);
	if (flags < 0) throw std::runtime_error("Cannot get file descriptor flags!");
	if (block)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) throw std::runtime_error("Cannot set file descriptor flags!");
}

