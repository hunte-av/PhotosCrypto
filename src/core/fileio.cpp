#include "fileio.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace photoscrypto {

namespace {

[[noreturn]] void fail(const std::string& what, const std::string& path) {
    throw std::runtime_error(what + " '" + path + "': " + std::strerror(errno));
}

// Closes the descriptor on scope exit.
class Fd {
public:
    explicit Fd(int fd) : fd_(fd) {}
    ~Fd() { if (fd_ >= 0) ::close(fd_); }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    int get() const { return fd_; }
    int release() { int fd = fd_; fd_ = -1; return fd; }
private:
    int fd_;
};

}  // namespace

Bytes readFileLimited(const std::string& path, std::size_t maxSize) {
    Fd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC));
    if (fd.get() < 0) fail("cannot open", path);

    struct stat st{};
    if (::fstat(fd.get(), &st) != 0) fail("cannot stat", path);
    if (!S_ISREG(st.st_mode)) throw std::runtime_error("not a regular file: '" + path + "'");
    if (static_cast<std::uint64_t>(st.st_size) > maxSize) {
        throw std::runtime_error("file too large: '" + path + "' (limit " + std::to_string(maxSize) + " bytes)");
    }

    Bytes data(static_cast<std::size_t>(st.st_size));
    std::size_t done = 0;
    while (done < data.size()) {
        ssize_t n = ::read(fd.get(), data.data() + done, data.size() - done);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) fail("cannot read", path);
        if (n == 0) break;  // file shrank while reading
        done += static_cast<std::size_t>(n);
    }
    data.resize(done);
    return data;
}

void writeFilePrivate(const std::string& path, const void* data, std::size_t size, bool force) {
    const int flags = O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC | (force ? O_TRUNC : O_EXCL);
    Fd fd(::open(path.c_str(), flags, S_IRUSR | S_IWUSR));
    if (fd.get() < 0) {
        if (errno == EEXIST) throw std::runtime_error("'" + path + "' already exists (use --force to overwrite)");
        fail("cannot create", path);
    }

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    std::size_t done = 0;
    while (done < size) {
        ssize_t n = ::write(fd.get(), bytes + done, size - done);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) {
            ::unlink(path.c_str());
            fail("cannot write", path);
        }
        done += static_cast<std::size_t>(n);
    }
    if (::fsync(fd.get()) != 0 || ::close(fd.release()) != 0) fail("cannot finish writing", path);
}

}  // namespace photoscrypto
