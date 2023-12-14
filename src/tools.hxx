#ifndef TOOLS_HXX
#define TOOLS_HXX

#include "rtp_header.hxx"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

enum class exceptions
{
    general_exception
};

enum class mode_type
{
    go_back_n,
    selective_repeat,
    unknown
};

std::ostream &operator<<(std::ostream &os, const mode_type &mode);

#define log_debug(...) ::logs::debug(__VA_ARGS__)

namespace logs
{
    template <typename... V> void info(V... args)
    {
        ((std::cout << "\033[32;1m[INFO]\033[0m ") << ... << args) << '\n';
    }

    template <typename... V> void debug(V... args)
    {
        ((std::cout << "\033[32;1m[DEBUG]\033[0m ") << ... << args) << '\n';
    }

    template <typename... V> void error(V... args)
    {
        ((std::cout << "\033[31m[ERROR]\033[0m ") << ... << args) << '\n';
        log_debug("抛出 `exceptions::GENERAL_EXCEPTION`, 开始栈回溯");
        throw exceptions::general_exception;
    }
}

std::uint32_t compute_checksum(const void *pkt, std::size_t n_bytes);

std::pair<std::size_t, mode_type> parse_window_size_and_mode(const char *window_size,
                                                             const char *mode);

// 此函数会尝试发送 `send_header`，然后等待 `wait_header`.
void send_and_wait_header(int attemp_times, int fd, const rtp_header &send_header,
                          const rtp_header &wait_header);
// 此函数会尝试发送 `send_header`，然后等待 `wait_seconds` 秒内没有新的接收.
// wait_seconds 只能是 2 或者 5
template <int wait_seconds>
void send_and_wait(int attemp_times, int fd, const rtp_header &send_header);

constexpr char SEND_HEADER_LOG[]{"\033[33m发送包\033[0m:\n"};
constexpr char RECV_HEADER_LOG[]{"\033[33m接收包\033[0m:\n"};
#endif
