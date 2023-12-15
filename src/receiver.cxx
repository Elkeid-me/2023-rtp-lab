#include "error_process.hxx"
#include "file_process.hxx"
#include "rtp_header.hxx"
#include "socket_process.hxx"
#include "tools.hxx"
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <vector>

void receiver_core_function(const char *port, const char *file_path,
                            std::size_t window_size, mode_type mode);

std::size_t handshake(int fd);

template <mode_type mode> bool process_new_packet(const rtp_packet &packet, int fd);

template <mode_type mode>
void receive_file(int fd, const char *file_path, std::size_t window_size,
                  std::uint32_t start_seq_num);

void terminate_connection(int fd, std::uint32_t fin_seq_num);

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

std::ofstream ofs;

std::vector<rtp_packet> packets_vec;
std::vector<std::uint8_t> ack_flags_vec;

std::size_t remain_file_size;

std::size_t window_size;
std::size_t file_window;

std::size_t window_left_seq_num;
std::size_t window_right_seq_num;
std::size_t fin_seq_num;

int ep_fd;
int timer_fd;

void receiver_core_function(const char *port, const char *file_path,
                            std::size_t window_size, mode_type mode)
{
    file_process::fd_wrapper socket{socket_process::open_receiver_socket(port)};

    file_process::fd_wrapper timer_wrapper(timerfd_create(0, 0));
    timer_fd = timer_wrapper.get_file_descriptor();

    if (timer_fd == -1)
        error_process::unix_error("`timerfd_create()` 错误: ");
    file_process::fd_wrapper epoll_wrapper(epoll_create1(0));
    ep_fd = epoll_wrapper.get_file_descriptor();
    if (ep_fd == -1)
        error_process::unix_error("`epoll_create1()` 错误: ");

    epoll_event ep_event_sock, ep_event_timer;
    ep_event_sock.events = EPOLLIN;
    ep_event_sock.data.fd = socket.get_file_descriptor();
    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, socket.get_file_descriptor(), &ep_event_sock) ==
        -1)
        error_process::unix_error("`epoll_ctl()` 错误: ");

    ep_event_timer.events = EPOLLIN;
    ep_event_timer.data.fd = timer_wrapper.get_file_descriptor();
    if (epoll_ctl(ep_fd, EPOLL_CTL_ADD, timer_wrapper.get_file_descriptor(),
                  &ep_event_timer) == -1)
        error_process::unix_error("`epoll_ctl()` 错误: ");

    std::size_t start_seq_num{handshake(socket.get_file_descriptor())};
    if (mode == mode_type::go_back_n)
        exit(1);
    receive_file<mode_type::selective_repeat>(socket.get_file_descriptor(), file_path,
                                              window_size, start_seq_num);
    terminate_connection(socket.get_file_descriptor(), fin_seq_num);
}

std::size_t handshake(int fd)
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

            log_debug(RECV_HEADER_LOG, header_buffer);

            if (header_buffer.is_valid() && header_buffer.get_flag() == SYN)
            {
                seq_num = header_buffer.get_seq_num();
                if (connect(fd, &src_addr, addrlen) == -1)
                    error_process::unix_error("`connect()` 错误: ");
                log_debug("包合法. 成功建立连接. 发送 SYN ACK");
                break;
            }
        }
        if (times > 50)
            logs::error("握手达到最大尝试次数");
    }
    send_and_wait_header(50, fd, {seq_num + 1, 0, SYN | ACK}, {seq_num + 1, 0, ACK});
    return seq_num + 1;
}

template <>
bool process_new_packet<mode_type::selective_repeat>(const rtp_packet &packet, int fd)
{
    if (packet.get_length() == 0 && packet.get_flag() == FIN)
    {
        if (packet.is_valid())
        {
            log_debug("收到 FIN");
            fin_seq_num = packet.get_seq_num();
            return true;
        }
    }
    if (packet.get_flag() != 0)
        return false;
    std::uint32_t seq_num{packet.get_seq_num()};
    std::size_t index{seq_num % window_size};
    if (seq_num >= window_right_seq_num)
    {
        log_debug("收到的 `seq_num` ", seq_num, " 大于窗口右边界 ", window_right_seq_num);
        return false;
    }
    if (seq_num < window_left_seq_num || ack_flags_vec[index])
    {
        // log_debug("ACK ", seq_num);
        if (rtp_header(seq_num, 0, ACK).send(fd) == -1)
            error_process::unix_error("发送包时出现了问题");
        return false;
    }

    if (!packet.is_valid())
        return false;

    ack_flags_vec[index] = true;
    packets_vec[index] = packet;
    if (rtp_header(seq_num, 0, ACK).send(fd) == -1)
        error_process::unix_error("发送包时出现了问题");
    // log_debug("ACK ", seq_num);

    if (seq_num != window_left_seq_num)
        return false;

    std::size_t _1st_nack_pkt;
    for (_1st_nack_pkt = window_left_seq_num; _1st_nack_pkt < window_right_seq_num;
         _1st_nack_pkt++)
    {
        std::size_t index{_1st_nack_pkt % window_size};

        if (!ack_flags_vec[index])
            break;

        ack_flags_vec[index] = false;
        ofs.write(packets_vec[index].get_buf(), packets_vec[index].get_length());
    }

    std::size_t difference{_1st_nack_pkt - window_left_seq_num};
    window_left_seq_num += difference;
    window_right_seq_num += difference;

    // log_debug("窗口变为 ", window_left_seq_num, ' ', window_right_seq_num);
    return false;
}

template <mode_type mode>
void receive_file(int fd, const char *file_path, std::size_t window_size,
                  std::uint32_t start_seq_num)
{
    ofs.open(file_path, std::ios::binary | std::ios::trunc);
    if (ofs.fail())
        logs::error("打开文件 `", file_path, "` 时出现了问题");

    ::window_size = window_size;
    packets_vec.resize(window_size);
    ack_flags_vec.resize(window_size, false);
    window_left_seq_num = start_seq_num;
    window_right_seq_num = window_left_seq_num + window_size;

    epoll_event ep_event{EPOLLIN};

    rtp_packet packet_buf;

    log_debug("开始接收文件");
    start_timer(timer_fd, 5000);
    socket_process::set_no_recv_timeout(fd);
    while (true)
    {
        if (epoll_wait(ep_fd, &ep_event, 1, -1) == -1)
            error_process::unix_error("`epoll_wait()` 错误: ");
        if (ep_event.data.fd != fd)
            logs::error("接收数据超时");
        else
        {
            if (recv(fd, &packet_buf, sizeof(rtp_packet), 0) == -1)
                error_process::unix_error("接收包时发生了问题: ");
            if (process_new_packet<mode>(packet_buf, fd))
                break;
            start_timer(timer_fd, 5000);
        }
    }
}

void terminate_connection(int fd, std::uint32_t fin_seq_num)
{
    send_and_wait<2>(50, fd, {fin_seq_num, 0, FIN | ACK});
}
