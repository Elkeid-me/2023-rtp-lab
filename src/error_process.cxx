#include "error_process.hxx"
#include <cstring>
#include <iostream>
#include <netdb.h>

namespace error_process
{
    void unix_error(const char *msg) { std::cerr << msg << std::strerror(errno) << '\n'; }

    void posix_error(int err_code, const char *msg)
    {
        std::cerr << msg << std::strerror(err_code) << '\n';
    }

    void gai_error(int code, const char *msg)
    {
        std::cerr << msg << gai_strerror(code) << '\n';
    }
};
