#include "util/chdir.h"
#include "util/exception.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	FileDescriptor::Ptr open_current_dir() {
		return FileDescriptor::create_open(".", O_RDONLY | O_DIRECTORY, 0);
	}
}

ScopedCHDir::ScopedCHDir(const char *dir) : old_dir(open_current_dir()) {
	if (chdir(dir) < 0) {
		throw SystemError("chdir", errno);
	}
}

ScopedCHDir::~ScopedCHDir() {
	if (fchdir(old_dir->fd()) < 0) {
		throw SystemError("fchdir", errno);
	}
}

