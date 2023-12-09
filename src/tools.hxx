#ifndef TOOLS_HXX
#define TOOLS_HXX

#include <cstddef>
#include <cstdint>
#include <iostream>

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
        ((std::cout << "\033[31;1m[ERROR]\033[0m ") << ... << args) << '\n';
    }
}

std::uint32_t compute_checksum(const void *pkt, std::size_t n_bytes);

#endif
