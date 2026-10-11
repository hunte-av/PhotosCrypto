#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace photoscrypto {

using Bytes = std::vector<std::uint8_t>;

// Encodes bytes into a single line of text using a private, shuffled 64-symbol
// alphabet (letters, digits and symbols). The output is not standard Base64 and
// does not reveal that it is a binary encoding of a file.
std::string bytesToText(const Bytes& data);

// Reverses bytesToText. Whitespace is ignored. Throws std::runtime_error on
// unknown symbols or an impossible length.
Bytes textToBytes(const std::string& text);

// Guesses an image extension from magic bytes (fallback only).
std::string detectExtension(const Bytes& data);

}  // namespace photoscrypto
