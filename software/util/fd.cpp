#include "util/fd.h"
#include <unistd.h>

file_descriptor::file_descriptor(int fd) : fd(fd) {
}

file_descriptor::~file_descriptor() {
	close(fd);
}

