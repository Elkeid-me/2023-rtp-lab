#include "rtp_header.hxx"
#include "tools.hxx"
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <ios>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(rtp_header) == 11);
static_assert(sizeof(rtp_packet) == 1472);

rtp_header::rtp_header(std::uint32_t seq_num, std::uint16_t length, std::uint8_t flag)
    : m_seq_num{seq_num}, m_length{length}, m_checksum{0}, m_flag{flag}
{
    m_checksum = compute_checksum(this, sizeof(rtp_header));
}

[[nodiscard]] ssize_t rtp_header::send(int fd) const
{
    return ::send(fd, this, sizeof(rtp_header), 0);
}

[[nodiscard]] ssize_t rtp_header::recv(int fd)
{
    std::memset(this, 0, sizeof(rtp_header));
    return ::recv(fd, this, sizeof(rtp_header), 0);
}

bool rtp_header::is_valid() const
{
    if (m_length > PAYLOAD_MAX)
        return false;

    if (m_flag & ~(SYN | ACK | FIN))
        return false;

    std::uint32_t original_checksum{m_checksum};
    m_checksum = 0;
    std::uint32_t new_checksum{compute_checksum(this, sizeof(rtp_header) + m_length)};
    m_checksum = original_checksum;
    if (new_checksum != original_checksum)
        return false;

    return true;
}

std::uint32_t rtp_header::get_seq_num() const { return m_seq_num; }
std::uint16_t rtp_header::get_length() const { return m_length; }
std::uint8_t rtp_header::get_flag() const { return m_flag; }

rtp_packet::rtp_packet(std::uint32_t seq_num, std::uint16_t length, std::uint8_t flag,
                       const void *buf)
    : rtp_header(seq_num, length, flag)
{
    assert(length <= PAYLOAD_MAX);
    std::memcpy(payload, buf, length);
    m_checksum = 0;
    m_checksum = compute_checksum(this, sizeof(rtp_header) + m_length);
}

std::ostream &operator<<(std::ostream &os, const rtp_header &rh)
{
    std::ios_base::fmtflags f{os.flags()};
    os << "seq_num: " << rh.m_seq_num << '\n'
       << "length: " << rh.m_length << '\n'
       << "checksum: " << std::hex << std::showbase << rh.m_checksum << '\n';
    os.flags(f);
    os << "flag: ";
    if (rh.m_flag & SYN)
        os << "SYN";
    if (rh.m_flag & ACK)
        os << " ACK";
    if (rh.m_flag & FIN)
        os << " FIN";
    return os;
}
