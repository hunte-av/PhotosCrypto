#pragma once
#include <stdexcept>
#include <string>

namespace photoscrypto {

// Error categories so that front-ends (CLI / GUI) can show their own, translated messages.
enum class ErrorCode {
    WrongPasswordOrModified,
    InvalidText,
    FileTooLarge,
    EmptyInput,
    OutputExists,
    PasswordTooShort,
    PasswordMismatch,
    Io,
    Internal,
};

class Error : public std::runtime_error {
public:
    Error(ErrorCode code, const std::string& message) : std::runtime_error(message), code_(code) {}
    ErrorCode code() const noexcept { return code_; }

private:
    ErrorCode code_;
};

}  // namespace photoscrypto
