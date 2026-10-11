#include "compress.hpp"

#include <zlib.h>
#include <algorithm>
#include <stdexcept>

namespace photoscrypto {

Bytes compressData(const Bytes& input) {
    if (input.empty()) return {};

    z_stream strm{};
    if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK) {
        throw std::runtime_error("compression initialization failed");
    }

    Bytes output;
    output.resize(deflateBound(&strm, static_cast<uLong>(input.size())));

    strm.next_in = const_cast<Bytef*>(input.data());
    strm.avail_in = static_cast<uInt>(input.size());
    strm.next_out = output.data();
    strm.avail_out = static_cast<uInt>(output.size());

    int ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        throw std::runtime_error("compression execution failed");
    }

    output.resize(strm.total_out);
    deflateEnd(&strm);
    return output;
}

Bytes decompressData(const Bytes& input, std::size_t maxOutputSize) {
    if (input.empty()) return {};

    z_stream strm{};
    if (inflateInit(&strm) != Z_OK) {
        throw std::runtime_error("decompression initialization failed");
    }

    Bytes output;
    constexpr std::size_t kInitialChunk = 128 * 1024;
    output.resize(std::min(kInitialChunk, maxOutputSize));

    strm.next_in = const_cast<Bytef*>(input.data());
    strm.avail_in = static_cast<uInt>(input.size());

    int ret = Z_OK;
    while (ret != Z_STREAM_END) {
        if (strm.total_out >= output.size()) {
            if (output.size() >= maxOutputSize) {
                inflateEnd(&strm);
                throw std::runtime_error("decompressed data exceeds maximum allowed size");
            }
            std::size_t nextSize = std::min(output.size() * 2, maxOutputSize);
            output.resize(nextSize);
        }

        strm.next_out = output.data() + strm.total_out;
        strm.avail_out = static_cast<uInt>(output.size() - strm.total_out);

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&strm);
            throw std::runtime_error("corrupted compressed stream");
        }
    }

    output.resize(strm.total_out);
    inflateEnd(&strm);
    return output;
}

}  // namespace photoscrypto
