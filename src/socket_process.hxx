#ifndef SOCKET_PROCESS_HXX
#define SOCKET_PROCESS_HXX

#include "file_process.hxx"
#include <memory>
namespace socket_process
{
    std::shared_ptr<file_process::fd_wrapper> open_receiver_socket(const char *port);
    std::shared_ptr<file_process::fd_wrapper> open_sender_socket(const char *host_name,
                                                                 const char *port);
}

#endif
