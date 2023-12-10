#ifndef RTP_HEAD_HXX
#define RTP_HEAD_HXX

#include "tools.hxx"
#include <cstddef>
#include <cstdint>
#include <ostream>

constexpr std::size_t PAYLOAD_MAX{1461};

// flags in the rtp header
enum class rtp_header_flag : std::uint8_t
{
    SYN = 0b0001,
    ACK = 0b0010,
    FIN = 0b0100,
};

class [[gnu::packed]] rtp_header
{
private:
    std::uint32_t m_seq_num;
    std::uint16_t m_length;
    std::uint32_t m_checksum;
    rtp_header_flag m_flag;

public:
    rtp_header(std::uint32_t seq_num, std::uint16_t length, rtp_header_flag flag);
    friend std::ostream &operator<<(std::ostream &, const rtp_header &);
};

struct [[gnu::packed]] rtp_packet
{
    rtp_header rtp;            // header
    char payload[PAYLOAD_MAX]; // data
};

#endif // RTP_HEAD_HXX
