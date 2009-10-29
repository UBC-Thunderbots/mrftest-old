#include "util/fd.h"
#include <unistd.h>

void file_descriptor::close() {
	if (fd != -1) {
		::close(fd);
		fd = -1;
	}
}

