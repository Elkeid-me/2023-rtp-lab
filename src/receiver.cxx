#include "error_process.hxx"
#include "file_process.hxx"
#include "rtp_header.hxx"
#include "socket_process.hxx"
#include "tools.hxx"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <sys/socket.h>

void receiver_core_function(const char *port, const char *file_path,
                            std::size_t window_size, mode_type mode);
void handshake(int fd);
void terminate_connection(int fd);

int main(int argc, char **argv)
{
    try
    {
        if (argc != 5)
        {
            logs::error("参数错误. 你可以这样使用: ", argv[0],
                        " [listen port] [file path] [window size] [mode]");
            return EXIT_FAILURE;
        }

        const char *port{argv[1]}, *file_path{argv[2]};

        auto [window_size, mode]{parse_window_size_and_mode(argv[3], argv[4])};

        log_debug("端口: ", port);
        log_debug("文件路径: ", file_path);
        log_debug("窗口大小: ", window_size);
        log_debug("模式: ", mode);

        receiver_core_function(port, file_path, window_size, mode);

        log_debug("Receiver: 正在退出");
        return 0;
    }
    catch (exceptions)
    {
        return EXIT_FAILURE;
    }
}

void receiver_core_function(const char *port, const char *file_path,
                            std::size_t window_size, mode_type mode)
{
    file_process::fd_wrapper socket{socket_process::open_receiver_socket(port)};
    handshake(socket.get_file_descriptor());

    // terminate_connection(socket.get_file_descriptor(), 0);
}

void handshake(int fd)
{
    std::uint32_t seq_num{0};
    rtp_header header_buffer;
    {
        int times{0};
        sockaddr src_addr;
        socklen_t addrlen{16};
        for (times = 1; times <= 50; times++)
        {
            if (recvfrom(fd, &header_buffer, sizeof(rtp_header), 0, &src_addr,
                         &addrlen) == -1 &&
                (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                log_debug("接收失败. 当前尝试次数: ", times);
                continue;
            }

            log_debug("\033[34m接收包\033[0m:", '\n', header_buffer);

            if (header_buffer.is_valid() && header_buffer.get_flag() == SYN)
            {
                seq_num = header_buffer.get_seq_num();
                if (connect(fd, &src_addr, addrlen) == -1)
                    error_process::unix_error("`connect()` error: ");
                log_debug("包合法. 成功建立连接. 发送 SYN ACK");
                break;
            }
        }
        if (times > 50)
            return;
    }

    send_and_wait_header(50, fd, {seq_num + 1, 0, SYN | ACK}, {seq_num + 1, 0, ACK});
}

void terminate_connection(int fd)
{
    // rtp_header header_buffer;

    // {
    //     int times{0};
    //     for (times = 1; times <= 50; times++)
    //     {
    //         if (header_buffer.recv(fd) == -1 && (errno == EAGAIN || errno ==
    //         EWOULDBLOCK))
    //         {
    //             log_debug("接收失败. 当前尝试次数: ", times);
    //             continue;
    //         }

    //         log_debug("\033[34m接收包\033[0m:", '\n', header_buffer);
    //         if (header_buffer.is_valid() && header_buffer.get_flag() == FIN &&
    //             header_buffer.get_seq_num() == fin_seq_num)
    //         {
    //             log_debug("包合法. 发送 FIN ACK");
    //             break;
    //         }
    //         log_debug("包不合法. 当前尝试次数: ", times);
    //     }
    //     if (times > 50)
    //         return;
    // }

    // send_and_wait<2>(50, fd, {fin_seq_num, 0, FIN | ACK});
}
