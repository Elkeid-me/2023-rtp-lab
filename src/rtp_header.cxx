#include "rtp_header.hxx"
#include <bit>
#include <cstdint>
#include <ios>
#include <iostream>
#include <string>
#include <unordered_map>

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(rtp_header) == 11);
static_assert(sizeof(rtp_packet) == 1472);

const std::unordered_map<rtp_header_flag, std::string> RTP_HEADER_FLAG_NAME{
    {rtp_header_flag::SYN, "SYN"},
    {rtp_header_flag::ACK, "ACK"},
    {rtp_header_flag::FIN, "FIN"}};

std::ostream &operator<<(std::ostream &os, const rtp_header &rh)
{
    std::ios_base::fmtflags f{os.flags()};
    os << "seq_num: " << rh.m_seq_num << '\n'
       << "length: " << rh.m_length << '\n'
       << "checksum: " << std::hex << std::showbase << rh.m_checksum << '\n';
    os.flags(f);

    if (auto it{RTP_HEADER_FLAG_NAME.find(rh.m_flag)}; it != RTP_HEADER_FLAG_NAME.end())
        os << "flag: " << it->second;
    else
    {
        std::ios_base::fmtflags f{os.flags()};
        os << "Unknown flag: " << std::hex << std::showbase
           << static_cast<std::uint32_t>(rh.m_flag);
        os.flags(f);
    }
    return os;
}
