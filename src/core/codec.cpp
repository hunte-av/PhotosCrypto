#include "codec.hpp"

#include <array>
#include <cstring>
#include <stdexcept>

namespace photoscrypto {

namespace {

// Private shuffled alphabet: 64 unique symbols.
constexpr char kAlphabet[65] = "I5op$lNeH9h71bxX4qJLBj=EF*UA+nOc0iy8?V#2r!dR^gt3%mk6MCus&KYa~Wzv";

using ReverseTable = std::array<std::int8_t, 256>;

ReverseTable buildReverseTable() {
    ReverseTable table{};
    table.fill(-1);
    for (int i = 0; i < 64; ++i) {
        table[static_cast<unsigned char>(kAlphabet[i])] = static_cast<std::int8_t>(i);
    }
    return table;
}

bool isSpace(char ch) { return ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t'; }

bool startsWith(const Bytes& data, std::size_t offset, const char* sig, std::size_t len) {
    return data.size() >= offset + len && std::memcmp(data.data() + offset, sig, len) == 0;
}

}  // namespace

std::string bytesToText(const Bytes& data) {
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    std::size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const std::uint32_t v = (std::uint32_t{data[i]} << 16) | (std::uint32_t{data[i + 1]} << 8) | data[i + 2];
        out.push_back(kAlphabet[(v >> 18) & 63]);
        out.push_back(kAlphabet[(v >> 12) & 63]);
        out.push_back(kAlphabet[(v >> 6) & 63]);
        out.push_back(kAlphabet[v & 63]);
    }
    const std::size_t rest = data.size() - i;
    if (rest == 1) {
        const std::uint32_t v = std::uint32_t{data[i]} << 16;
        out.push_back(kAlphabet[(v >> 18) & 63]);
        out.push_back(kAlphabet[(v >> 12) & 63]);
    } else if (rest == 2) {
        const std::uint32_t v = (std::uint32_t{data[i]} << 16) | (std::uint32_t{data[i + 1]} << 8);
        out.push_back(kAlphabet[(v >> 18) & 63]);
        out.push_back(kAlphabet[(v >> 12) & 63]);
        out.push_back(kAlphabet[(v >> 6) & 63]);
    }
    return out;
}

Bytes textToBytes(const std::string& text) {
    static const ReverseTable table = buildReverseTable();
    Bytes out;
    out.reserve(text.size() / 4 * 3 + 2);
    std::uint32_t acc = 0;
    int count = 0;
    for (char ch : text) {
        if (isSpace(ch)) continue;
        const int value = table[static_cast<unsigned char>(ch)];
        if (value < 0) throw std::runtime_error("file contains unknown symbols");
        acc = (acc << 6) | static_cast<std::uint32_t>(value);
        if (++count == 4) {
            out.push_back(static_cast<std::uint8_t>(acc >> 16));
            out.push_back(static_cast<std::uint8_t>(acc >> 8));
            out.push_back(static_cast<std::uint8_t>(acc));
            acc = 0;
            count = 0;
        }
    }
    if (count == 1) throw std::runtime_error("file is truncated or corrupted");
    if (count == 2) {
        out.push_back(static_cast<std::uint8_t>(acc >> 4));
    } else if (count == 3) {
        out.push_back(static_cast<std::uint8_t>(acc >> 10));
        out.push_back(static_cast<std::uint8_t>(acc >> 2));
    }
    return out;
}

std::string detectExtension(const Bytes& data) {
    if (startsWith(data, 0, "\xFF\xD8\xFF", 3)) return ".jpg";
    if (startsWith(data, 0, "\x89PNG\r\n\x1A\n", 8)) return ".png";
    if (startsWith(data, 0, "RIFF", 4) && startsWith(data, 8, "WEBP", 4)) return ".webp";
    if (startsWith(data, 4, "ftypavif", 8)) return ".avif";
    if (startsWith(data, 4, "ftypheic", 8)) return ".heic";
    if (startsWith(data, 0, "GIF8", 4)) return ".gif";
    if (startsWith(data, 0, "BM", 2)) return ".bmp";
    if (startsWith(data, 0, "II*\0", 4) || startsWith(data, 0, "MM\0*", 4)) return ".tif";
    return ".bin";
}

}  // namespace photoscrypto
