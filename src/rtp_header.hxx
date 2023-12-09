#ifndef RTP_HEAD_HXX
#define RTP_HEAD_HXX

#include <bit>
#include <cstddef>
#include <cstdint>

constexpr std::size_t PAYLOAD_MAX{1461};

// flags in the rtp header
enum class rtp_header_flag : std::uint8_t
{
    RTP_SYN = 0b0001,
    RTP_ACK = 0b0010,
    RTP_FIN = 0b0100,
};

class [[gnu::packed]] rtp_header
{
private:
    std::uint32_t m_seq_num;
    std::uint16_t m_length;
    std::uint32_t m_checksum;
    rtp_header_flag m_flag;

public:
    constexpr rtp_header(std::uint32_t seq_num, std::uint16_t length,
                         std::uint32_t checksum, rtp_header_flag flag);
};

struct [[gnu::packed]] rtp_packet
{
    rtp_header rtp;            // header
    char payload[PAYLOAD_MAX]; // data
};

static_assert(std::endian::native == std::endian::little);
static_assert(sizeof(rtp_header) == 11);
static_assert(sizeof(rtp_packet) == 1472);

#endif // RTP_HEAD_HXX
