#pragma once
#include <string>

#include "codec.hpp"

namespace photoscrypto {

// Reads a regular file, refusing anything larger than `maxSize` bytes.
Bytes readFileLimited(const std::string& path, std::size_t maxSize);

// Writes data with mode 0600. Refuses to overwrite unless `force` is set; never follows symlinks.
void writeFilePrivate(const std::string& path, const void* data, std::size_t size, bool force);

}  // namespace photoscrypto
