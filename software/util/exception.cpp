#include "util/exception.h"
#include "util/misc.h"
#include <cerrno>
#include <netdb.h>
#include <string>
#include <string.h>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>

namespace {
	std::string get_system_error_string(const std::string &call, int err) {
		std::vector<char> buffer(32);
		while (xsi_strerror_r(err, &buffer[0], buffer.size()) < 0) {
			int foo = errno;
			errno = foo;
			if (errno == ERANGE) {
				buffer.resize(buffer.size() * 2);
			} else {
				throw ErrorMessageError();
			}
		}
		return call + ": " + std::string(&buffer[0]);
	}

	std::string get_eai_error_string(const std::string &call, int err) {
		return call + ": " + gai_strerror(err);
	}
}

ErrorMessageError::ErrorMessageError() : std::runtime_error("Error fetching error message") {
}

SystemError::SystemError(const char *call, int err) : std::runtime_error(get_system_error_string(call, err)), error_code(err) {
}

FileNotFoundError::FileNotFoundError() : SystemError("open", ENOENT) {
}

EAIError::EAIError(const char *call, int err) : std::runtime_error(get_eai_error_string(call, err)), error_code(err) {
}

