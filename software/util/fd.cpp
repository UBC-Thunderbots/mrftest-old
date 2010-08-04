#include "util/exception.h"
#include "util/fd.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

FileDescriptor::Ptr FileDescriptor::create_from_fd(int fd) {
	const Ptr p(new FileDescriptor(fd));
	return p;
}

FileDescriptor::Ptr FileDescriptor::create_open(const char *file, int flags, mode_t mode) {
	const Ptr p(new FileDescriptor(file, flags, mode));
	return p;
}

FileDescriptor::Ptr FileDescriptor::create_socket(int pf, int type, int proto) {
	const Ptr p(new FileDescriptor(pf, type, proto));
	return p;
}

FileDescriptor::~FileDescriptor() {
	try {
		close();
	} catch (...) {
		// Swallow.
	}
}

void FileDescriptor::close() {
	if (fd_ >= 0) {
		if (::close(fd_) < 0) {
			throw SystemError("close", errno);
		}
		fd_ = -1;
	}
}

int FileDescriptor::fd() const {
	return fd_;
}

void FileDescriptor::set_blocking(bool block) const {
	long flags = fcntl(fd_, F_GETFL);
	if (flags < 0) {
		throw SystemError("fcntl", errno);
	}
	if (block) {
		flags &= ~O_NONBLOCK;
	} else {
		flags |= O_NONBLOCK;
	}
	if (fcntl(fd_, F_SETFL, flags) < 0) {
		throw SystemError("fcntl", errno);
	}
}

FileDescriptor::FileDescriptor(int fd) : fd_(fd) {
	if (fd_ < 0) {
		throw std::invalid_argument("Invalid file descriptor");
	}
}

FileDescriptor::FileDescriptor(const char *file, int flags, mode_t mode) : fd_(open(file, flags, mode)) {
	if (fd_ < 0) {
		if (errno == ENOENT) {
			throw FileNotFoundError();
		} else {
			throw SystemError("open", errno);
		}
	}
}

FileDescriptor::FileDescriptor(int af, int type, int proto) : fd_(socket(af, type, proto)) {
	if (fd_ < 0) {
		throw SystemError("socket", errno);
	}
}

