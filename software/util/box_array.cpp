#include "util/box_array.h"
#include "util/exception.h"
#include <cerrno>
#include <cstdlib>
#include <unistd.h>

void *BoxArrayUtils::allocate_aligned_memory(std::size_t length) {
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size < 0) {
		throw SystemError("sysconf(_SC_PAGESIZE)", errno);
	}

	void *ptr;
	int rc = posix_memalign(&ptr, static_cast<std::size_t>(page_size), length);
	if (rc != 0) {
		throw SystemError("posix_memalign", errno);
	}

	return ptr;
}

