#include "file_process.hxx"
#include "rtp_header.hxx"
#include "socket_process.hxx"
#include "tools.hxx"
#include <cstdint>
#include <cstdlib>
#include <random>
#include <sys/epoll.h>
#include <sys/socket.h>

void sender_core_function(const char *hose_name, const char *port, const char *file_path,
                          std::size_t window_size, mode_type mode);
void handshake(int fd, std::uint32_t seq_num);
void terminate_connection(int fd, std::uint32_t fin_seq_num);

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

        sender_core_function(host_name, port, file_path, window_size, mode);

        log_debug("Sender: 退出");
        return 0;
    }
    catch (exceptions)
    {
        return EXIT_FAILURE;
    }
}

void sender_core_function(const char *hose_name, const char *port, const char *file_path,
                          std::size_t window_size, mode_type mode)
{
    file_process::fd_wrapper socket(socket_process::open_sender_socket(hose_name, port));
    const std::uint32_t seq_num{std::random_device{}()};
    handshake(socket.get_file_descriptor(), seq_num);
    // terminate_connection(socket.get_file_descriptor(), 0);
}

void handshake(int fd, std::uint32_t seq_num)
{
    send_and_wait_header(50, fd, {seq_num, 0, SYN}, {seq_num + 1, 0, SYN | ACK});
    send_and_wait<2>(50, fd, {seq_num + 1, 0, ACK});
}

void terminate_connection(int fd, std::uint32_t fin_seq_num)
{
    send_and_wait_header(50, fd, {fin_seq_num, 0, FIN}, {fin_seq_num, 0, FIN | ACK});
}
