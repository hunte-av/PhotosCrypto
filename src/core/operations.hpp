#pragma once
#include <string>
#include <vector>

#include "error.hpp"
#include "codec.hpp"
#include "crypto.hpp"

namespace photoscrypto {

constexpr std::size_t kMinPasswordLength = 6;
constexpr int kMinAttempts = 1;
constexpr int kMaxAttempts = 10;
constexpr int kDefaultAttempts = 3;
constexpr std::size_t kMaxPhotoSize = 64u * 1024 * 1024;

struct DecodedData {
    Bytes photo;
    std::string extension;
};

void validatePassword(const std::string& password);

enum class PasswordStrength { Weak, Fair, Strong };

// Heuristic strength rating based on length and character variety.
PasswordStrength passwordStrength(const std::string& password);

// 5-step strength score from 0 (empty) to 5 (maximum security).
int passwordStrengthScore(const std::string& password);

// Checks if the encoded file requires a password or was saved without one.
bool isPasswordProtected(const std::string& textPath);

// Encrypts `photoPath` with optional `password` and writes the encoded symbol text to `textPath`.
// Automatically compresses the payload with zlib for minimal file size.
// If password is empty, encodes with default authenticated encryption without requiring a password on decode.
void encodeFile(const std::string& photoPath, const std::string& textPath, 
                const std::string& password, bool overwrite, bool selfDestruct, int maxAttempts);

// Decodes a file to memory, returning the photo bytes and its original extension.
DecodedData decodeToMemory(const std::string& textPath, const std::string& password);

// Decrypts `textPath` and writes it to `outputPath`.
std::string decodeFile(const std::string& textPath, const std::string& outputPath,
                       const std::string& password, bool overwrite);

}  // namespace photoscrypto
