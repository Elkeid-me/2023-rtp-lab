#include "rtp_header.hxx"
#include <cstdint>

constexpr rtp_header::rtp_header(std::uint32_t seq_num, std::uint16_t length,
                                 std::uint32_t checksum, rtp_header_flag flag)
    : m_seq_num{seq_num}, m_length{length}, m_checksum{checksum}, m_flag{flag}
{
}
