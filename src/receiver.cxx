#include "error_process.hxx"
#include "file_process.hxx"
#include "rtp_header.hxx"
#include "socket_process.hxx"
#include "tools.hxx"
#include <cerrno>
#include <cstdlib>
#include <sys/socket.h>

void receiver_main_function(const char *port, const char *file_path,
                            std::size_t window_size, mode_type mode);

void handshake(int fd);

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

        receiver_main_function(port, file_path, window_size, mode);

        log_debug("Receiver: 正在退出");
        return 0;
    }
    catch (exceptions)
    {
        return EXIT_FAILURE;
    }
}

void receiver_main_function(const char *port, const char *file_path,
                            std::size_t window_size, mode_type mode)
{
    file_process::fd_wrapper socket{socket_process::open_receiver_socket(port)};
    handshake(socket.get_file_descriptor());
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

    {
        int times{0};
        const rtp_header header_synack(seq_num + 1, 0, SYN | ACK);
        for (times = 1; times <= 50; times++)
        {
            if (header_synack.send(fd) == -1)
                error_process::unix_error("`send()` error: ");

            log_debug("\033[33m发送包\033[0m:", '\n', header_synack);

            if (header_buffer.recv(fd) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                log_debug("接收失败. 当前尝试次数: ", times);
                continue;
            }

            log_debug("\033[34m接收包\033[0m:", '\n', header_buffer);
            if (header_buffer.is_valid() && header_buffer.get_flag() == (ACK) &&
                header_buffer.get_seq_num() == seq_num + 1)
            {
                log_debug("包合法. 握手完成. 开始接收文件");
                break;
            }
            log_debug("包不合法. 当前尝试次数: ", times);
        }
        if (times > 50)
            return;
    }
}
