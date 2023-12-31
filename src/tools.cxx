#include "tools.hxx"
#include "error_process.hxx"
#include "rtp_header.hxx"
#include "socket_process.hxx"
#include <charconv>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sys/timerfd.h>

static std::uint32_t crc32_for_byte(std::uint32_t r)
{
    for (int i{0}; i < 8; i++)
        r = (r & 1 ? 0 : 0xEDB88320U) ^ r >> 1;
    return r ^ 0xFF000000U;
}

static void crc32(const void *data, std::size_t n_bytes, std::uint32_t *crc)
{
    static std::uint32_t table[0x100];
    if (table[0] == 0)
    {
        for (std::size_t i{0}; i < 0x100; i++)
            table[i] = crc32_for_byte(i);
    }
    for (std::size_t i{0}; i < n_bytes; i++)
        *crc = table[(std::uint8_t)*crc ^ ((std::uint8_t *)data)[i]] ^ *crc >> 8;
}

// Computes checksum for `n_bytes` of data
//
// Hint 1: Before computing the checksum, you should set everything up
// and set the "checksum" field to 0. And when checking if a packet
// has the correct check sum, don't forget to set the "checksum" field
// back to 0 before invoking this function.
//
// Hint 2: `len + sizeof(rtp_header_t)` is the real length of a rtp
// data packet.
std::uint32_t compute_checksum(const void *pkt, std::size_t n_bytes)
{
    std::uint32_t crc{0};
    crc32(pkt, n_bytes, &crc);
    return crc;
}

std::pair<std::size_t, mode_type> parse_window_size_and_mode(const char *window_size_str,
                                                             const char *mode_str)
{
    std::size_t window_size;
    if (std::from_chars(window_size_str, window_size_str + std::strlen(window_size_str),
                        window_size)
            .ec != std::errc{})
        logs::error("window size `", window_size_str, "` 不合法");
    mode_type mode;
    switch (mode_str[0])
    {
    case '0':
        mode = mode_type::go_back_n;
        break;
    case '1':
        mode = mode_type::selective_repeat;
        break;
    default:
        mode = mode_type::unknown;
        logs::error("mode `", mode_str, "` 不合法");
        break;
    }

    return {window_size, mode};
}

std::ostream &operator<<(std::ostream &os, const mode_type &mode)
{
    switch (mode)
    {
    case mode_type::go_back_n:
        os << "回退 n";
        break;
    case mode_type::selective_repeat:
        os << "选择重传";
        break;
    case mode_type::unknown:
        os << "未知";
        break;
    }

    return os;
}

void send_and_wait_header(int attempt_times, int fd, const rtp_header &send_header,
                          const rtp_header &wait_header)
{
    int times{0};
    rtp_header header_buffer;
    for (times = 1; times <= attempt_times; times++)
    {
        if (send_header.send(fd) == -1)
            error_process::unix_error("`send()` 错误: ");
        log_debug(SEND_HEADER_LOG, send_header);

        if (header_buffer.recv(fd) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            log_debug("接收失败. 当前尝试次数: ", times);
            continue;
        }
        log_debug(RECV_HEADER_LOG, header_buffer);
        if (header_buffer == wait_header)
            break;
        log_debug("包不合法. 当前尝试次数: ", times);
    }
    if (times > 50)
        logs::error("超出尝试次数.");
}

static void send_and_wait_helper(int attempt_times, int fd, const rtp_header &send_header)
{
    int times{0};
    rtp_header header_buffer;
    for (times = 1; times <= 50; times++)
    {
        if (send_header.send(fd) == -1)
            error_process::unix_error("`send()` 错误: ");

        log_debug(SEND_HEADER_LOG, send_header);

        if (header_buffer.recv(fd) == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            break;

        log_debug("重发. 当前尝试次数: ", times);
    }
    if (times > 50)
        logs::error("超出尝试次数.");
    socket_process::set_100ms_recv_timeout(fd);
}

template <> void send_and_wait<2>(int attempt_times, int fd, const rtp_header &send_header)
{
    socket_process::set_2s_recv_timeout(fd);
    send_and_wait_helper(attempt_times, fd, send_header);
}

template <> void send_and_wait<5>(int attempt_times, int fd, const rtp_header &send_header)
{
    socket_process::set_5s_recv_timeout(fd);
    send_and_wait_helper(attempt_times, fd, send_header);
}

void start_timer(int timer_fd, std::int64_t time)
{
    itimerspec spec{{0, 0}, {time / 1000, time % 1000 * 1000'000}};
    if (timerfd_settime(timer_fd, 0, &spec, NULL) == -1)
        error_process::unix_error("`timerfd_settime()` 错误: ");
}
