#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "../core/operations.hpp"
#include "password.hpp"

#ifndef PHOTOSCRYPTO_VERSION
#define PHOTOSCRYPTO_VERSION "1.1.0"
#endif

namespace fs = std::filesystem;
using namespace photoscrypto;

namespace {

constexpr const char* kUsage =
    "photoscrypto " PHOTOSCRYPTO_VERSION " - encrypt and compress a photo into a compact symbol text file\n"
    "\n"
    "Usage:\n"
    "  photoscrypto encode <photo> <output.txt> [options]\n"
    "  photoscrypto decode <input.txt> [output] [options]\n"
    "\n"
    "Options:\n"
    "  --force            overwrite the output file if it exists\n"
    "  --no-password      encode without password protection (skip password prompt)\n"
    "  --password-stdin   read the password from the first line of stdin\n"
    "  --max-attempts N   (encode) destroy the file after N wrong passwords, N = 1-10\n"
    "  -h, --help         show this help\n"
    "  -V, --version      show version\n"
    "\n"
    "Features:\n"
    "  * Automatic zlib payload compression for maximum storage optimization\n"
    "  * Optional password protection (leave empty or use --no-password to skip)\n"
    "  * The original file extension is stored securely inside and restored on decode.\n";

struct Options {
    std::string command;
    std::vector<std::string> positional;
    bool force = false;
    bool noPassword = false;
    bool passwordStdin = false;
    bool selfDestruct = false;
    int maxAttempts = kDefaultAttempts;
};

Options parseArgs(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--force") opt.force = true;
        else if (arg == "--no-password") opt.noPassword = true;
        else if (arg == "--password-stdin") opt.passwordStdin = true;
        else if (arg == "--max-attempts") {
            if (++i >= argc) throw Error(ErrorCode::Internal, "--max-attempts needs a number");
            try { opt.maxAttempts = std::stoi(argv[i]); } catch (const std::exception&) { throw Error(ErrorCode::Internal, "invalid attempt limit"); }
            opt.selfDestruct = true;
        }
        else if (arg == "-h" || arg == "--help") opt.command = "help";
        else if (arg == "-V" || arg == "--version") opt.command = "version";
        else if (arg.size() > 1 && arg[0] == '-') throw Error(ErrorCode::Internal, "unknown option: " + arg);
        else if (opt.command.empty()) opt.command = arg;
        else opt.positional.push_back(arg);
    }
    return opt;
}

// Holds a password and wipes it from memory on destruction.
struct Secret {
    std::string value;
    ~Secret() { if (!value.empty()) value[0] = '\0'; }
};

int runEncode(const Options& opt) {
    if (opt.positional.size() != 2) throw Error(ErrorCode::Internal, "encode needs <photo> <output.txt>");

    Secret password;
    if (!opt.noPassword) {
        std::string p = opt.passwordStdin ? readPasswordFromStdin(/*allowEmpty=*/true)
                                          : promptPassword(/*confirm=*/true, /*allowEmpty=*/true);
        password.value = std::move(p);
    }

    encodeFile(opt.positional[0], opt.positional[1], password.value, opt.force, opt.selfDestruct, opt.maxAttempts);
    std::cout << "Encoded -> " << opt.positional[1] << "\n";
    return 0;
}

int runDecode(const Options& opt) {
    if (opt.positional.empty() || opt.positional.size() > 2) {
        throw Error(ErrorCode::Internal, "decode needs <input.txt> [output]");
    }

    Secret password;
    const bool isProtected = isPasswordProtected(opt.positional[0]);
    if (isProtected) {
        std::string p = opt.passwordStdin ? readPasswordFromStdin(/*allowEmpty=*/false)
                                          : promptPassword(/*confirm=*/false, /*allowEmpty=*/false);
        password.value = std::move(p);
    } else {
        std::cout << "[Note] File is not password protected, decoding directly...\n";
    }

    std::string out = decodeFile(opt.positional[0], opt.positional.size() == 2 ? opt.positional[1] : "", password.value, opt.force);
    std::cout << "Decoded -> " << out << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options opt = parseArgs(argc, argv);
        if (opt.command == "encode") return runEncode(opt);
        if (opt.command == "decode") return runDecode(opt);
        if (opt.command == "version") return std::cout << "photoscrypto " PHOTOSCRYPTO_VERSION "\n", 0;
        std::cout << kUsage;
        return (opt.command.empty() || opt.command == "help") ? 0 : 1;
    } catch (const Error& e) {
        std::cerr << "photoscrypto: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
