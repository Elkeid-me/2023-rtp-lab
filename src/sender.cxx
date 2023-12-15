#include "error_process.hxx"
#include "file_process.hxx"
#include "rtp_header.hxx"
#include "socket_process.hxx"
#include "tools.hxx"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <vector>

void sender_core_function(const char *hose_name, const char *port, const char *file_path,
                          std::size_t window_size, mode_type mode);
void handshake(int fd, std::uint32_t seq_num);
void terminate_connection(int fd, std::uint32_t fin_seq_num);

template <mode_type mode>
void send_file(int fd, const char *file_path, std::size_t window_size,
               std::uint32_t start_seq_num);

template <mode_type mode> void resend(int fd);

template <mode_type mode> [[nodiscard]] bool process_ack(std::uint32_t seq_num);

void send_window(int fd);

int main(int argc, char **argv)
{
    try
    {
        std::ios::sync_with_stdio(false);
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

std::ifstream ifs;

std::vector<rtp_packet> packets_vec;
std::vector<std::uint8_t> ack_flags_vec;

std::size_t remain_file_size;
std::size_t n_need_ack_window;

std::size_t window_size;
std::size_t file_window;

std::size_t window_left_seq_num;
std::size_t window_right_seq_num;
std::size_t window_left_unsend_seq_num;

int ep_fd;
int timer_fd;

void sender_core_function(const char *hose_name, const char *port, const char *file_path,
                          std::size_t window_size, mode_type mode)
{
    file_process::fd_wrapper socket(socket_process::open_sender_socket(hose_name, port));
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

    std::uint32_t seq_num{std::random_device{}()};
    if (seq_num > std::numeric_limits<std::uint16_t>::max())
        seq_num /= (std::numeric_limits<std::uint8_t>::max() + 1);
    handshake(socket.get_file_descriptor(), seq_num);
    log_debug("握手完成");
    switch (mode)
    {
    case mode_type::go_back_n:
        send_file<mode_type::go_back_n>(socket.get_file_descriptor(), file_path,
                                        window_size, seq_num + 1);
        break;
    case mode_type::selective_repeat:
        send_file<mode_type::selective_repeat>(socket.get_file_descriptor(), file_path,
                                               window_size, seq_num + 1);
        break;
    case mode_type::unknown:
        logs::error("未知的模式");
        break;
    }
    log_debug("文件发送完成");
    terminate_connection(socket.get_file_descriptor(), seq_num + 1 + file_window);
}

template <mode_type mode>
void send_file(int fd, const char *file_path, std::size_t window_size,
               std::uint32_t start_seq_num)
{
    ifs.open(file_path, std::ios::binary);
    if (ifs.fail())
        logs::error("打开文件 `", file_path, "` 时出现了问题");

    remain_file_size = std::filesystem::file_size(file_path);
    log_debug("文件大小: ", remain_file_size);
    file_window = remain_file_size / PAYLOAD_MAX;

    if (remain_file_size % PAYLOAD_MAX > 0)
        file_window += 1;
    ::window_size = window_size;

    packets_vec.resize(window_size);
    ack_flags_vec.resize(window_size, false);

    window_left_seq_num = start_seq_num;
    window_left_unsend_seq_num = window_left_seq_num;
    window_right_seq_num =
        window_left_seq_num + (file_window > window_size ? window_size : file_window);

    n_need_ack_window = file_window;

    epoll_event ep_event{EPOLLIN};
    int attemp_times{0};
    rtp_header header_buf;

    log_debug("开始发送文件");
    while (true)
    {
        if (n_need_ack_window == 0)
            return;
        send_window(fd);
        if (epoll_wait(ep_fd, &ep_event, 1, -1) == -1)
            error_process::unix_error("`epoll_wait()` 错误: ");

        if (ep_event.data.fd != fd)
        {
            attemp_times++;
            if (attemp_times > 500)
                logs::error("发送数据达到最大尝试次数");
            resend<mode>(fd);
            start_timer(timer_fd, 100);
        }
        else
        {
            if (header_buf.recv(fd) == -1 && (errno != EAGAIN && errno != EWOULDBLOCK))
                error_process::unix_error("接收包发生了错误: ");
            if (header_buf.get_length() == 0 && header_buf.is_valid() &&
                header_buf.get_flag() == ACK)
            {
                if (process_ack<mode>(header_buf.get_seq_num()))
                {
                    attemp_times = 0;
                    start_timer(timer_fd, 100);
                }
            }
        }
    }
}

template <>
[[nodiscard]] bool process_ack<mode_type::selective_repeat>(std::uint32_t seq_num)
{
    if (seq_num < window_left_seq_num || seq_num >= window_right_seq_num)
    {
        log_debug("`process_ack()`: 接收到的 `seq_num`: ", seq_num, " 超出当前窗口 [",
                  window_left_seq_num, ", ", window_right_seq_num - 1, ']');
        return false;
    }

    if (ack_flags_vec[seq_num % window_size])
        return false;

    log_debug("ACK ", seq_num);
    ack_flags_vec[seq_num % window_size] = true;

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
    }

    std::size_t difference{_1st_nack_pkt - window_left_seq_num};
    window_left_seq_num += difference;
    n_need_ack_window -= difference;
    window_left_unsend_seq_num = window_right_seq_num;
    window_right_seq_num += difference;
    if (window_right_seq_num - window_left_seq_num > n_need_ack_window)
        window_right_seq_num = window_left_seq_num + n_need_ack_window;

    log_debug("窗口变为 ", window_left_seq_num, ' ', window_right_seq_num, ' ',
              window_left_unsend_seq_num);
    return true;
}

template <> [[nodiscard]] bool process_ack<mode_type::go_back_n>(std::uint32_t seq_num)
{
    if (seq_num <= window_left_seq_num || seq_num > window_right_seq_num)
    {
        log_debug("`process_ack()`: 接收到的 `seq_num`: ", seq_num, " 超出当前窗口 [",
                  window_left_seq_num, ", ", window_right_seq_num - 1, ']');
        return false;
    }

    for (std::size_t i{window_left_seq_num}; i < seq_num; i++)
    {
        log_debug("ACK ", i);
        ack_flags_vec[i % window_size] = true;
    }

    std::size_t difference{seq_num - window_left_seq_num};
    window_left_seq_num += difference;
    n_need_ack_window -= difference;
    window_left_unsend_seq_num = window_right_seq_num;
    window_right_seq_num += difference;
    if (window_right_seq_num - window_left_seq_num > n_need_ack_window)
        window_right_seq_num = window_left_seq_num + n_need_ack_window;

    log_debug("窗口变为 ", window_left_seq_num, ' ', window_right_seq_num, ' ',
              window_left_unsend_seq_num);
    return true;
}

template <mode_type mode> void resend(int fd)
{
    for (std::size_t i{window_left_seq_num}; i < window_right_seq_num; i++)
    {
        std::size_t index{i % window_size};
        if constexpr (mode == mode_type::selective_repeat)
        {
            if (!ack_flags_vec[index])
            {
                if (packets_vec[index].send(fd) == -1)
                    error_process::unix_error("发送包失败: ");
            }
        }
        else
        {
            if (packets_vec[index].send(fd) == -1)
                error_process::unix_error("发送包失败: ");
        }
    }
}

void send_window(int fd)
{
    bool send_{false};
    for (std::size_t seq_num{window_left_unsend_seq_num}; seq_num < window_right_seq_num;
         seq_num++)
    {
        send_ = true;
        std::size_t index{seq_num % window_size};
        std::size_t payload_size{remain_file_size > PAYLOAD_MAX ? PAYLOAD_MAX
                                                                : remain_file_size};
        ifs.read(packets_vec[index].get_buf(), payload_size);

        remain_file_size -= payload_size;

        packets_vec[index].make_packet(seq_num, payload_size, 0);
        if (packets_vec[index].send(fd) == -1)
            error_process::unix_error("发送包失败: ");
    }
    window_left_unsend_seq_num = window_right_seq_num;
    if (send_)
        start_timer(timer_fd, 100);
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
