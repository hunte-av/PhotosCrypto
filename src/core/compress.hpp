#pragma once
#include <cstdint>
#include <vector>

namespace photoscrypto {

using Bytes = std::vector<std::uint8_t>;

// Compresses data using zlib deflate with maximum compression level (Z_BEST_COMPRESSION).
Bytes compressData(const Bytes& input);

// Decompresses data using zlib inflate up to maxOutputSize.
// Throws std::runtime_error on corruption or size limit violation.
Bytes decompressData(const Bytes& input, std::size_t maxOutputSize);

}  // namespace photoscrypto
