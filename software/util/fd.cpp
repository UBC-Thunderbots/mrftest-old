#include "util/fd.h"
#include "util/exception.h"
#include <cstdlib>
#include <fcntl.h>
#include <glibmm.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>

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

FileDescriptor::Ptr FileDescriptor::create_temp(const char *pattern) {
	const Ptr p(new FileDescriptor(pattern));
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

FileDescriptor::FileDescriptor(const char *pattern) {
	const std::string &tmpdir = Glib::get_tmp_dir();
	std::vector<char> filename(tmpdir.begin(), tmpdir.end());
	filename.push_back('/');
	for (const char *ptr = pattern; *ptr; ++ptr) {
		filename.push_back(*ptr);
	}
	filename.push_back('\0');
	fd_ = mkstemp(&filename[0]);
	if (fd_ < 0) {
		throw SystemError("mkstemp", errno);
	}
	if (unlink(&filename[0]) < 0) {
		int saved_errno = errno;
		::close(fd_);
		throw SystemError("unlink", saved_errno);
	}
}

