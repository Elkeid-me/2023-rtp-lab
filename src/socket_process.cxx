#include "error_process.hxx"
#include "file_process.hxx"
#include <memory>
#include <netdb.h>
#include <sys/socket.h>

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

static constexpr int optval{1};

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
                set_socket_option(receiver_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                                  sizeof(int));
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

namespace socket_process
{
    std::shared_ptr<file_process::fd_wrapper> open_receiver_socket(const char *port)
    {
        int ret{::open_receiver_socket(port)};
        if (ret < 0)
            error_process::unix_error("`open_receiver_socket()` 错误: ");
        return std::make_shared<file_process::fd_wrapper>(ret);
    }

    std::shared_ptr<file_process::fd_wrapper> open_sender_socket(const char *host_name,
                                                                 const char *port)
    {
        int ret{::open_sender_socket(host_name, port)};
        if (ret < 0)
            error_process::unix_error("`open_sender_socket()` 错误: ");
        return std::make_shared<file_process::fd_wrapper>(ret);
    }
}