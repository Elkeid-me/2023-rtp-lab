#include "error_process.hxx"
#include "file_process.hxx"
#include "rtp_header.hxx"
#include "socket_process.hxx"
#include "tools.hxx"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <sys/epoll.h>
#include <sys/socket.h>

std::mt19937 mt19937(std::random_device{}());

void sender_main_function(const char *hose_name, const char *port, const char *file_path,
                          std::size_t window_size, mode_type mode);
void handshake(int fd);

int main(int argc, char **argv)
{
    try
    {
        if (argc != 6)
            logs::error(
                "参数错误. 你可以这样使用: ", argv[0],
                " [receiver ip] [receiver port] [file path] [window size] [mode]");

        const char *host_name{argv[1]}, *port{argv[2]}, *file_path{argv[3]};

        auto [window_size, mode]{parse_window_size_and_mode(argv[4], argv[5])};

        log_debug("接收端地址: ", host_name);
        log_debug("接收端端口: ", port);
        log_debug("文件路径: ", file_path);
        log_debug("窗口大小: ", window_size);
        log_debug("模式: ", mode);

        sender_main_function(host_name, port, file_path, window_size, mode);

        log_debug("Sender: 退出");
        return 0;
    }
    catch (exceptions)
    {
        return EXIT_FAILURE;
    }
}

void sender_main_function(const char *hose_name, const char *port, const char *file_path,
                          std::size_t window_size, mode_type mode)
{
    file_process::fd_wrapper socket(socket_process::open_sender_socket(hose_name, port));
    handshake(socket.get_file_descriptor());
}

void handshake(int fd)
{
    const std::uint32_t seq_num{static_cast<std::uint32_t>(mt19937())};
    rtp_header header_buffer;

    {
        const rtp_header header_syn(seq_num, 0, SYN);
        int times{0};
        for (times = 1; times <= 50; times++)
        {
            if (header_syn.send(fd) == -1)
                error_process::unix_error("`send()` error: ");

            log_debug("\033[33m发送包\033[0m:", '\n', header_syn);
            if (header_buffer.recv(fd) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                log_debug("接收失败. 当前尝试次数: ", times);
                continue;
            }

            log_debug("\033[34m接收包\033[0m:", '\n', header_buffer);
            if (header_buffer.is_valid() && header_buffer.get_flag() == (SYN | ACK) &&
                header_buffer.get_seq_num() == seq_num + 1)
            {
                log_debug("包合法. 发送 ACK");
                break;
            }
            log_debug("包不合法. 当前尝试次数: ", times);
        }
        if (times > 50)
            return;
    }

    {
        socket_process::set_2s_recv_timeout(fd);
        const rtp_header header_ack(seq_num + 1, 0, ACK);
        int times{0};
        for (times = 1; times <= 50; times++)
        {
            if (header_ack.send(fd) == -1)
                error_process::unix_error("`send()` error: ");

            log_debug("\033[33m发送包\033[0m:", '\n', header_ack);

            if (header_buffer.recv(fd) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                log_debug("2s 延时已过, 可以认为 receiver 收到 ACK. 开始发送文件");
                break;
            }
        }
        if (times > 50)
            return;
        socket_process::set_100ms_recv_timeout(fd);
    }
}
