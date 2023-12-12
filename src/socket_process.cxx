#include "socket_process.hxx"
#include "error_process.hxx"
#include "file_process.hxx"
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>

static addrinfo *get_addr_info(const char *host, const char *service,
                               const addrinfo *hints);

static void set_socket_option(int s, int level, int optname, const void *optval,
                              int optlen);

static int open_receiver_socket(const char *port);

static int open_sender_socket(const char *hostname, const char *port);

namespace socket_process
{
    void set_100ms_recv_timeout(int socket)
    {
        constexpr timeval SO_RCVTIMEO_OPTVAL_100ms{0, 100'000};
        set_socket_option(socket, SOL_SOCKET, SO_RCVTIMEO, &SO_RCVTIMEO_OPTVAL_100ms,
                          sizeof(timeval));
    }

    void set_2s_recv_timeout(int socket)
    {
        constexpr timeval SO_RCVTIMEO_OPTVAL_2s{2};
        set_socket_option(socket, SOL_SOCKET, SO_RCVTIMEO, &SO_RCVTIMEO_OPTVAL_2s,
                          sizeof(timeval));
    }

    int open_receiver_socket(const char *port)
    {
        int ret{::open_receiver_socket(port)};
        if (ret < 0)
            error_process::unix_error("`open_receiver_socket()` 错误: ");
        return ret;
    }

    int open_sender_socket(const char *host_name, const char *port)
    {
        int ret{::open_sender_socket(host_name, port)};
        if (ret < 0)
            error_process::unix_error("`open_sender_socket()` 错误: ");
        return ret;
    }
}

static addrinfo *get_addr_info(const char *host, const char *service,
                               const addrinfo *hints)
{
    addrinfo *result;

    int ecode{getaddrinfo(host, service, hints, &result)};
    if (ecode == 0)
        return result;

    error_process::gai_error(ecode, "`getaddrinfo()` 错误: ");
    return nullptr;
}

static void set_socket_option(int s, int level, int optname, const void *optval,
                              int optlen)
{
    if (setsockopt(s, level, optname, optval, optlen) < 0)
        error_process::unix_error("`setsockopt` 错误: ");
}

static constexpr int SO_REUSEADDR_OPTVAL{1};

static int open_receiver_socket(const char *port)
{
    addrinfo hint{AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV,
                  AF_UNSPEC,
                  SOCK_DGRAM,
                  0,
                  0,
                  nullptr,
                  nullptr,
                  nullptr};
    addrinfo *addr_list_head{get_addr_info(nullptr, port, &hint)};

    int receiver_fd = 0;
    if (addr_list_head != nullptr)
    {
        addrinfo *ptr{addr_list_head};
        while (ptr != nullptr)
        {
            receiver_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (receiver_fd >= 0)
            {
                set_socket_option(receiver_fd, SOL_SOCKET, SO_REUSEADDR,
                                  &SO_REUSEADDR_OPTVAL, sizeof(int));

                socket_process::set_100ms_recv_timeout(receiver_fd);
                if (bind(receiver_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
                    break;

                file_process::close(receiver_fd);
            }
            ptr = ptr->ai_next;
        }
        freeaddrinfo(addr_list_head);
        if (ptr == nullptr)
            return -1;
        return receiver_fd;
    }

    return -1;
}

static int open_sender_socket(const char *hostname, const char *port)
{
    addrinfo hint{AI_ADDRCONFIG | AI_NUMERICSERV,
                  AF_UNSPEC,
                  SOCK_DGRAM,
                  0,
                  0,
                  nullptr,
                  nullptr,
                  nullptr};
    addrinfo *addr_list_head{get_addr_info(hostname, port, &hint)};

    int sender_fd = 0;
    if (addr_list_head != nullptr)
    {
        addrinfo *ptr{addr_list_head};
        while (ptr != nullptr)
        {
            sender_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (sender_fd >= 0)
            {
                socket_process::set_100ms_recv_timeout(sender_fd);

                if (connect(sender_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
                    break;
                file_process::close(sender_fd);
            }
            ptr = ptr->ai_next;
        }
        freeaddrinfo(addr_list_head);
        if (ptr == nullptr)
            return -1;
        return sender_fd;
    }

    return -1;
}
