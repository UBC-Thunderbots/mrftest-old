#include "util/chdir.h"
#include "util/exception.h"
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

ScopedCHDir::ScopedCHDir(const char *dir) : old_dir(FileDescriptor::create_open(".", O_RDONLY | O_DIRECTORY, 0)) {
	if (chdir(dir) < 0) {
		throw SystemError("chdir", errno);
	}
}

ScopedCHDir::~ScopedCHDir() {
	if (fchdir(old_dir.fd()) < 0) {
		throw SystemError("fchdir", errno);
	}
}

