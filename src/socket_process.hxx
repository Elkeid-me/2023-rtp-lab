#ifndef SOCKET_PROCESS_HXX
#define SOCKET_PROCESS_HXX

namespace socket_process
{
    int open_receiver_socket(const char *port);
    int open_sender_socket(const char *host_name, const char *port);
}

#endif
