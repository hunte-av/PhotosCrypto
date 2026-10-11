#include "password.hpp"

#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

#include "../core/crypto.hpp"
#include "../core/operations.hpp"

namespace photoscrypto {

namespace {

int g_ttyFd = -1;
termios g_savedTermios{};

extern "C" void restoreAndExit(int signal) {
    if (g_ttyFd >= 0) tcsetattr(g_ttyFd, TCSANOW, &g_savedTermios);
    _exit(128 + signal);
}

// Disables terminal echo for its lifetime and restores it on scope exit or Ctrl+C.
class EchoOff {
public:
    explicit EchoOff(int fd) {
        if (tcgetattr(fd, &g_savedTermios) != 0) throw std::runtime_error("cannot read terminal settings");
        g_ttyFd = fd;
        std::signal(SIGINT, restoreAndExit);
        std::signal(SIGTERM, restoreAndExit);
        termios silent = g_savedTermios;
        silent.c_lflag &= static_cast<tcflag_t>(~ECHO);
        silent.c_lflag |= ECHONL;
        if (tcsetattr(fd, TCSAFLUSH, &silent) != 0) throw std::runtime_error("cannot disable echo");
    }
    ~EchoOff() {
        tcsetattr(g_ttyFd, TCSAFLUSH, &g_savedTermios);
        std::signal(SIGINT, SIG_DFL);
        std::signal(SIGTERM, SIG_DFL);
        g_ttyFd = -1;
    }
    EchoOff(const EchoOff&) = delete;
    EchoOff& operator=(const EchoOff&) = delete;
};

std::string readLine(std::FILE* stream) {
    std::string line;
    for (int ch = std::fgetc(stream); ch != EOF && ch != '\n'; ch = std::fgetc(stream)) {
        line.push_back(static_cast<char>(ch));
    }
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;
}

std::string readHidden(std::FILE* tty, const char* prompt) {
    std::fputs(prompt, tty);
    std::fflush(tty);
    EchoOff guard(fileno(tty));
    return readLine(tty);
}

void requireLength(std::string& password) {
    if (password.size() < kMinPasswordLength) {
        secureWipe(password.data(), password.size());
        throw std::runtime_error("password must be at least " + std::to_string(kMinPasswordLength) +
                                 " characters");
    }
}

}  // namespace

std::string promptPassword(bool confirm, bool allowEmpty) {
    struct FileCloser {
        void operator()(std::FILE* f) const { std::fclose(f); }
    };
    std::unique_ptr<std::FILE, FileCloser> tty(std::fopen("/dev/tty", "r+"));
    if (!tty) throw std::runtime_error("no terminal available; use --password-stdin or --no-password");

    const char* promptMsg = allowEmpty ? "Password (leave empty for no password): " : "Password: ";
    std::string password = readHidden(tty.get(), promptMsg);

    if (allowEmpty && password.empty()) {
        return "";
    }

    requireLength(password);
    if (confirm) {
        std::string again = readHidden(tty.get(), "Repeat password: ");
        const bool match = (again == password);
        secureWipe(again.data(), again.size());
        if (!match) {
            secureWipe(password.data(), password.size());
            throw std::runtime_error("passwords do not match");
        }
    }
    return password;
}

std::string readPasswordFromStdin(bool allowEmpty) {
    std::string password = readLine(stdin);
    if (allowEmpty && password.empty()) {
        return "";
    }
    requireLength(password);
    return password;
}

}  // namespace photoscrypto
