#include "util/exception.h"
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>
#include "util/misc.h"

namespace
{
std::string get_system_error_string(const char *call, int err)
{
    std::vector<char> buffer(32);
    while (xsi_strerror_r(err, &buffer[0], buffer.size()) < 0)
    {
        int foo = errno;
        errno   = foo;
        if (errno == ERANGE)
        {
            buffer.resize(buffer.size() * 2);
        }
        else
        {
            throw ErrorMessageError();
        }
    }
    std::string s;
    s.reserve(std::strlen(call) + 2 + std::strlen(&buffer[0]));
    s.append(call);
    s.append(": ");
    s.append(&buffer[0]);
    return s;
}

std::string get_eai_error_string(const char *call, int err)
{
    const char *msg = gai_strerror(err);
    std::string s;
    s.reserve(std::strlen(call) + 2 + std::strlen(msg));
    s.append(call);
    s.append(": ");
    s.append(msg);
    return s;
}
}

ErrorMessageError::ErrorMessageError()
    : std::runtime_error("Error fetching error message")
{
}

SystemError::SystemError(const char *call, int err)
    : std::runtime_error(get_system_error_string(call, err)), error_code(err)
{
}

FileNotFoundError::FileNotFoundError() : SystemError("open", ENOENT)
{
}

EAIError::EAIError(const char *call, int err)
    : std::runtime_error(get_eai_error_string(call, err)), error_code(err)
{
}
