#include "util/fd.h"
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>

void file_descriptor::close() {
	if (fd != -1) {
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

