#ifndef RTP_HEAD_HXX
#define RTP_HEAD_HXX

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <sys/types.h>

constexpr std::size_t PAYLOAD_MAX{1461};

// flags in the rtp header
constexpr std::uint8_t SYN{0b0001};
constexpr std::uint8_t ACK{0b0010};
constexpr std::uint8_t FIN{0b0100};

class [[gnu::packed]] rtp_header
{
protected:
    std::uint32_t m_seq_num;
    std::uint16_t m_length;
    mutable std::uint32_t m_checksum;
    std::uint8_t m_flag;

public:
    rtp_header(std::uint32_t seq_num, std::uint16_t length, std::uint8_t flag);
    rtp_header() = default;
    friend std::ostream &operator<<(std::ostream &, const rtp_header &);
    [[nodiscard]] ssize_t send(int fd) const;
    [[nodiscard]] ssize_t recv(int fd);
    bool is_valid() const;

    std::uint32_t get_seq_num() const;
    std::uint16_t get_length() const;
    std::uint8_t get_flag() const;

    friend auto operator<=>(const rtp_header &lhs, const rtp_header &rhs) = default;
};

class [[gnu::packed]] rtp_packet : rtp_header
{
private:
    char payload[PAYLOAD_MAX]; // data
public:
    rtp_packet(std::uint32_t seq_num, std::uint16_t length, std::uint8_t flag,
               const void *buf);
};

#endif // RTP_HEAD_HXX
