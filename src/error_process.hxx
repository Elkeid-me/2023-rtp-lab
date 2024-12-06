#ifndef ERROR_PROCESS_HXX
#define ERROR_PROCESS_HXX

namespace error_process
{
    void unix_error(const char *msg);
    void posix_error(int code, const char *msg);
    void gai_error(int code, const char *msg);
}

#endif
