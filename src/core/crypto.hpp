#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace photoscrypto {

using Bytes = std::vector<std::uint8_t>;

// Flags in header (blob[0]):
constexpr uint8_t kFlagSelfDestruct = 1 << 0; // 0x01: File self-destructs after max attempts
constexpr uint8_t kFlagCompressed   = 1 << 1; // 0x02: Data payload is compressed with zlib
constexpr uint8_t kFlagNoPassword   = 1 << 2; // 0x04: File is encoded without user password

// Encrypted blob layout:
//   flags[1] | maxAttempts[1] | attempts[1] | salt[16] | iv[12] | ciphertext[n] | tag[16]
constexpr std::size_t kSaltSize = 16;
constexpr std::size_t kIvSize = 12;
constexpr std::size_t kTagSize = 16;
constexpr std::size_t kHeaderSize = 3;
constexpr std::size_t kAttemptsIndex = 2;
constexpr std::size_t kOverhead = kHeaderSize + kSaltSize + kIvSize + kTagSize;

Bytes encrypt(const Bytes& plaintext, const std::string& password, uint8_t flags, uint8_t maxAttempts);

// Throws std::runtime_error if the password is wrong or the data was modified.
Bytes decrypt(const Bytes& blob, const std::string& password);

// Overwrites memory so secrets do not linger.
void secureWipe(void* data, std::size_t size);

}  // namespace photoscrypto
