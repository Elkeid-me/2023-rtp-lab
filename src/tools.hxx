#ifndef TOOLS_HXX
#define TOOLS_HXX

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

enum class exceptions
{
    GENERAL_EXCEPTION
};

#define log_debug(...) ::logs::debug(__VA_ARGS__)

namespace logs
{
    template <typename... V> void info(V... args)
    {
        ((std::cout << "\033[32;1m[INFO]\033[0m ") << ... << args) << '\n';
    }

    template <typename... V> void debug(V... args)
    {
        ((std::cerr << "\033[32;1m[DEBUG]\033[0m ") << ... << args) << '\n';
    }

    template <typename... V> void error_no_exit(V... args)
    {
        ((std::cerr << "\033[31;1m[ERROR]\033[0m ") << ... << args) << '\n';
    }

    template <typename... V> void error(V... args)
    {
        error_no_exit(args...);
        log_debug("抛出 `exceptions::GENERAL_EXCEPTION`, 开始栈回溯");
        throw exceptions::GENERAL_EXCEPTION;
    }
}

std::uint32_t compute_checksum(const void *pkt, std::size_t n_bytes);

#endif
