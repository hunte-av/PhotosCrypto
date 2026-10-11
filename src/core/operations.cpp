#include "operations.hpp"

#include <filesystem>
#include <string>
#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "codec.hpp"
#include "crypto.hpp"
#include "compress.hpp"
#include "fileio.hpp"

namespace photoscrypto {
namespace fs = std::filesystem;

namespace {
// 4 symbols per 3 bytes, plus some overhead slack.
constexpr std::size_t kMaxTextSize = (kMaxPhotoSize + kOverhead + 260) / 3 * 4 + 4096;

bool isSpace(char ch) { return ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t'; }
}

void validatePassword(const std::string& password) {
    if (password.size() < kMinPasswordLength) {
        throw Error(ErrorCode::PasswordTooShort, "Password must be at least " + std::to_string(kMinPasswordLength) + " characters");
    }
}

PasswordStrength passwordStrength(const std::string& password) {
    bool lower = false, upper = false, digit = false, other = false;
    for (unsigned char ch : password) {
        if (std::islower(ch)) lower = true;
        else if (std::isupper(ch)) upper = true;
        else if (std::isdigit(ch)) digit = true;
        else other = true;
    }
    const int classes = lower + upper + digit + other;
    const std::size_t len = password.size();
    if (len < 8 || (classes == 1 && len < 12)) return PasswordStrength::Weak;
    if (len >= 16 || (len >= 12 && classes >= 3)) return PasswordStrength::Strong;
    return PasswordStrength::Fair;
}

int passwordStrengthScore(const std::string& password) {
    if (password.empty()) return 0;
    if (password.size() < 6) return 1;

    bool lower = false, upper = false, digit = false, symbol = false;
    for (unsigned char ch : password) {
        if (std::islower(ch)) lower = true;
        else if (std::isupper(ch)) upper = true;
        else if (std::isdigit(ch)) digit = true;
        else symbol = true;
    }

    const int classes = (lower ? 1 : 0) + (upper ? 1 : 0) + (digit ? 1 : 0) + (symbol ? 1 : 0);
    const std::size_t len = password.size();

    if (classes == 1) {
        return (len >= 12) ? 3 : 2;
    }
    if (classes == 2) {
        if (len < 8) return 2;
        if (len < 12) return 3;
        return 4;
    }
    if (classes == 3) {
        if (len < 8) return 3;
        if (len < 10) return 4;
        return 5;
    }
    if (classes >= 4) {
        if (len < 8) return 3;
        if (len < 10) return 4;
        return 5;
    }
    return 2;
}

bool isPasswordProtected(const std::string& textPath) {
    int fd = ::open(textPath.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return true;
    char buf[64];
    ssize_t n = ::read(fd, buf, sizeof(buf));
    ::close(fd);
    if (n < 4) return true;

    std::string sample;
    for (ssize_t i = 0; i < n; ++i) {
        if (!isSpace(buf[i])) sample.push_back(buf[i]);
        if (sample.size() == 16) break;
    }
    if (sample.size() < 4) return true;
    sample.resize((sample.size() / 4) * 4);
    try {
        Bytes blob = textToBytes(sample);
        if (blob.empty()) return true;
        return (blob[0] & kFlagNoPassword) == 0;
    } catch (...) {
        return true;
    }
}

void encodeFile(const std::string& photoPath, const std::string& textPath, const std::string& password, bool overwrite, bool selfDestruct, int maxAttempts) {
    if (maxAttempts < kMinAttempts || maxAttempts > kMaxAttempts) {
        throw Error(ErrorCode::Internal, "attempt limit must be between " + std::to_string(kMinAttempts) + " and " + std::to_string(kMaxAttempts));
    }

    if (password.empty()) {
        if (selfDestruct) {
            throw Error(ErrorCode::Internal, "Self-destruct requires password protection");
        }
    } else {
        validatePassword(password);
    }
    
    Bytes photo;
    try {
        photo = readFileLimited(photoPath, kMaxPhotoSize);
    } catch (const std::exception& e) {
        throw Error(ErrorCode::Io, e.what());
    }
    
    if (photo.empty()) {
        throw Error(ErrorCode::EmptyInput, "Input file is empty");
    }

    std::string ext = fs::path(photoPath).extension().string();
    if (ext.size() > 255) ext = ""; // Handle extremely long/invalid extensions safely

    // Format: ExtLen (1 byte) | Ext (ExtLen bytes) | PhotoData
    Bytes plain(1 + ext.size() + photo.size());
    plain[0] = static_cast<uint8_t>(ext.size());
    if (!ext.empty()) {
        std::memcpy(plain.data() + 1, ext.data(), ext.size());
    }
    std::memcpy(plain.data() + 1 + ext.size(), photo.data(), photo.size());

    uint8_t flags = 0;
    if (selfDestruct) flags |= kFlagSelfDestruct;

    // Automatic compression layer (zlib)
    Bytes payloadToEncrypt;
    try {
        Bytes compressed = compressData(plain);
        if (!compressed.empty() && compressed.size() < plain.size()) {
            flags |= kFlagCompressed;
            payloadToEncrypt = std::move(compressed);
        } else {
            payloadToEncrypt = std::move(plain);
        }
    } catch (...) {
        payloadToEncrypt = std::move(plain);
    }

    Bytes blob;
    try {
        blob = encrypt(payloadToEncrypt, password, flags, selfDestruct ? static_cast<uint8_t>(maxAttempts) : uint8_t{0});
    } catch (const std::exception& e) {
        secureWipe(payloadToEncrypt.data(), payloadToEncrypt.size());
        throw Error(ErrorCode::Internal, e.what());
    }
    secureWipe(payloadToEncrypt.data(), payloadToEncrypt.size());

    const std::string text = bytesToText(blob);
    
    try {
        writeFilePrivate(textPath, text.data(), text.size(), overwrite);
    } catch (const std::exception& e) {
        std::string err = e.what();
        if (err.find("already exists") != std::string::npos) {
            throw Error(ErrorCode::OutputExists, err);
        }
        throw Error(ErrorCode::Io, err);
    }
}

DecodedData decodeToMemory(const std::string& textPath, const std::string& password) {
    Bytes raw;
    try {
        raw = readFileLimited(textPath, kMaxTextSize);
    } catch (const std::exception& e) {
        throw Error(ErrorCode::Io, e.what());
    }

    Bytes blob;
    try {
        blob = textToBytes(std::string(raw.begin(), raw.end()));
    } catch (const std::exception& e) {
        throw Error(ErrorCode::InvalidText, e.what());
    }

    if (blob.size() < kOverhead) {
        throw Error(ErrorCode::InvalidText, "Empty or invalid file content");
    }

    const uint8_t flags = blob[0];
    const bool isUnprotected = (flags & kFlagNoPassword) != 0;
    const bool selfDestruct = (flags & kFlagSelfDestruct) != 0;
    const int maxAttempts = blob[1];

    if (!isUnprotected) {
        validatePassword(password);
    }

    Bytes decrypted;
    try {
        decrypted = decrypt(blob, password);
    } catch (const std::exception& e) {
        if (selfDestruct) {
            const int used = blob[kAttemptsIndex] + 1;
            std::error_code ec;
            if (used >= maxAttempts) {
                fs::remove(textPath, ec);
                throw Error(ErrorCode::WrongPasswordOrModified, "Wrong password.\n\nToo many failed attempts: the file has been destroyed.");
            }
            blob[kAttemptsIndex] = static_cast<uint8_t>(used);
            const std::string updated = bytesToText(blob);
            try {
                writeFilePrivate(textPath, updated.data(), updated.size(), true);
            } catch (const std::exception&) {
            }
            throw Error(ErrorCode::WrongPasswordOrModified, "Wrong password. Attempts left before the file is destroyed: " + std::to_string(maxAttempts - used));
        }
        throw Error(ErrorCode::WrongPasswordOrModified, e.what());
    }

    Bytes plain;
    if ((flags & kFlagCompressed) != 0) {
        try {
            plain = decompressData(decrypted, kMaxPhotoSize + 260);
        } catch (const std::exception& e) {
            secureWipe(decrypted.data(), decrypted.size());
            throw Error(ErrorCode::InvalidText, std::string("Decompression error: ") + e.what());
        }
        secureWipe(decrypted.data(), decrypted.size());
    } else {
        plain = std::move(decrypted);
    }

    if (plain.empty()) {
        throw Error(ErrorCode::InvalidText, "Decrypted data is empty");
    }

    uint8_t extLen = plain[0];
    if (1 + static_cast<std::size_t>(extLen) > plain.size()) {
        secureWipe(plain.data(), plain.size());
        throw Error(ErrorCode::InvalidText, "Corrupted internal file structure");
    }

    std::string ext(reinterpret_cast<char*>(plain.data() + 1), extLen);
    Bytes photo(plain.begin() + 1 + extLen, plain.end());
    secureWipe(plain.data(), plain.size());

    return {std::move(photo), std::move(ext)};
}

std::string decodeFile(const std::string& textPath, const std::string& outputPath, const std::string& password, bool overwrite) {
    DecodedData decoded = decodeToMemory(textPath, password);
    
    fs::path out = outputPath.empty() ? fs::path(textPath).replace_extension() : fs::path(outputPath);
    if (!out.has_extension() && !decoded.extension.empty()) {
        out += decoded.extension;
    } else if (!out.has_extension()) {
        out += detectExtension(decoded.photo);
    }

    try {
        writeFilePrivate(out.string(), decoded.photo.data(), decoded.photo.size(), overwrite);
    } catch (const std::exception& e) {
        secureWipe(decoded.photo.data(), decoded.photo.size());
        std::string err = e.what();
        if (err.find("already exists") != std::string::npos) {
            throw Error(ErrorCode::OutputExists, err);
        }
        throw Error(ErrorCode::Io, err);
    }
    
    secureWipe(decoded.photo.data(), decoded.photo.size());
    return out.string();
}

}  // namespace photoscrypto
