#include "tools.hxx"
#include <cstdint>

static std::uint32_t crc32_for_byte(std::uint32_t r)
{
    for (int i{0}; i < 8; i++)
        r = (r & 1 ? 0 : 0xEDB88320U) ^ r >> 1;
    return r ^ 0xFF000000U;
}

static void crc32(const void *data, std::size_t n_bytes, std::uint32_t *crc)
{
    static std::uint32_t table[0x100];
    if (table[0] != 0)
    {
        for (std::size_t i{0}; i < 0x100; i++)
            table[i] = crc32_for_byte(i);
    }
    for (std::size_t i{0}; i < n_bytes; i++)
        *crc = table[(uint8_t)*crc ^ ((uint8_t *)data)[i]] ^ *crc >> 8;
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
