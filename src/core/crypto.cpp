#include "crypto.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <memory>
#include <stdexcept>

namespace photoscrypto {

namespace {

constexpr std::uint64_t kScryptN = 1u << 18;
constexpr std::uint64_t kScryptR = 8;
constexpr std::uint64_t kScryptP = 1;
constexpr std::uint64_t kScryptMaxMem = 640ull * 1024 * 1024;
constexpr std::size_t kKeySize = 32;

constexpr const char* kDefaultMasterKey = "photoscrypto:v1.1:unprotected-payload-key";

using Key = std::array<unsigned char, kKeySize>;
using CipherCtx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;

struct KeyGuard {
    Key key{};
    ~KeyGuard() { secureWipe(key.data(), key.size()); }
};

void deriveKey(const std::string& password, const unsigned char* salt, Key& key) {
    if (EVP_PBE_scrypt(password.data(), password.size(), salt, kSaltSize, kScryptN, kScryptR, kScryptP,
                       kScryptMaxMem, key.data(), key.size()) != 1) {
        throw std::runtime_error("key derivation failed");
    }
}

CipherCtx newContext() {
    CipherCtx ctx(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!ctx) throw std::runtime_error("could not create cipher context");
    return ctx;
}

int toInt(std::size_t size) {
    if (size > static_cast<std::size_t>(INT32_MAX)) throw std::runtime_error("data too large");
    return static_cast<int>(size);
}

}  // namespace

void secureWipe(void* data, std::size_t size) {
    if (data != nullptr && size != 0) OPENSSL_cleanse(data, size);
}

Bytes encrypt(const Bytes& plaintext, const std::string& password, uint8_t flags, uint8_t maxAttempts) {
    const bool noPassword = password.empty();
    if (noPassword) {
        flags |= kFlagNoPassword;
    }

    Bytes out(kOverhead + plaintext.size());
    out[0] = flags;
    out[1] = maxAttempts;
    out[kAttemptsIndex] = 0;
    const unsigned char aad[2] = {flags, maxAttempts};
    unsigned char* salt = out.data() + kHeaderSize;
    unsigned char* iv = salt + kSaltSize;
    unsigned char* body = iv + kIvSize;
    
    if (RAND_bytes(salt, kSaltSize + kIvSize) != 1) throw std::runtime_error("random generator failed");

    const std::string effectivePassword = noPassword ? kDefaultMasterKey : password;

    KeyGuard guard;
    deriveKey(effectivePassword, salt, guard.key);

    CipherCtx ctx = newContext();
    int len = 0;
    int total = 0;
    if (EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, guard.key.data(), iv) != 1 ||
        EVP_EncryptUpdate(ctx.get(), nullptr, &len, aad, 2) != 1 ||
        EVP_EncryptUpdate(ctx.get(), body, &len, plaintext.data(), toInt(plaintext.size())) != 1) {
        throw std::runtime_error("encryption failed");
    }
    total = len;
    if (EVP_EncryptFinal_ex(ctx.get(), body + total, &len) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kTagSize, body + plaintext.size()) != 1) {
        throw std::runtime_error("encryption failed");
    }
    return out;
}

Bytes decrypt(const Bytes& blob, const std::string& password) {
    if (blob.size() < kOverhead) throw std::runtime_error("file is too short to be valid");
    const uint8_t flags = blob[0];
    const bool isUnprotected = (flags & kFlagNoPassword) != 0;

    if (!isUnprotected && password.empty()) {
        throw std::runtime_error("this file is password protected, password is required");
    }

    const std::string effectivePassword = isUnprotected ? kDefaultMasterKey : password;

    const unsigned char aad[2] = {blob[0], blob[1]};
    const unsigned char* salt = blob.data() + kHeaderSize;
    const unsigned char* iv = salt + kSaltSize;
    const unsigned char* body = iv + kIvSize;
    const std::size_t bodySize = blob.size() - kOverhead;
    Bytes tag(body + bodySize, body + bodySize + kTagSize);

    KeyGuard guard;
    deriveKey(effectivePassword, salt, guard.key);

    Bytes plain(bodySize);
    CipherCtx ctx = newContext();
    int len = 0;
    if (EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, guard.key.data(), iv) != 1 ||
        EVP_DecryptUpdate(ctx.get(), nullptr, &len, aad, 2) != 1 ||
        EVP_DecryptUpdate(ctx.get(), plain.data(), &len, body, toInt(bodySize)) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kTagSize, tag.data()) != 1 ||
        EVP_DecryptFinal_ex(ctx.get(), plain.data() + len, &len) != 1) {
        secureWipe(plain.data(), plain.size());
        throw std::runtime_error("wrong password or file has been modified");
    }
    return plain;
}

}  // namespace photoscrypto
