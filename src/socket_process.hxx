#ifndef SOCKET_PROCESS_HXX
#define SOCKET_PROCESS_HXX

namespace socket_process
{
    int open_receiver_socket(const char *port);
    int open_sender_socket(const char *host_name, const char *port);
    void set_100ms_recv_timeout(int socket);
    void set_2s_recv_timeout(int socket);
    void set_5s_recv_timeout(int socket);
    void set_no_recv_timeout(int socket);
}

#endif
