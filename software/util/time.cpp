#include "util/time.h"
#include "util/exception.h"

void timespec_now(timespec *result, clockid_t clk) {
	if (clock_gettime(clk, result) < 0) {
		throw SystemError("clock_gettime", errno);
	}
}

