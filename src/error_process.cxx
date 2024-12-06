#include "error_process.hxx"
#include "tools.hxx"
#include <cstring>
#include <netdb.h>

namespace error_process
{
    void unix_error(const char *msg) { logs::error(msg, std::strerror(errno)); }

    void posix_error(int err_code, const char *msg)
    {
        logs::error(msg, std::strerror(err_code));
    }

    void gai_error(int code, const char *msg) { logs::error(msg, gai_strerror(code)); }
}
