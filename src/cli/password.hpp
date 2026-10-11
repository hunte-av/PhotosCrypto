#pragma once
#include <string>

namespace photoscrypto {

// Prompts the user for a password on /dev/tty without echoing characters.
// If allowEmpty is true and the user presses Enter without typing anything, returns "".
// If non-empty, validates minimum length and (if confirm is true) asks twice.
std::string promptPassword(bool confirm, bool allowEmpty = false);

// Reads one line from stdin.
std::string readPasswordFromStdin(bool allowEmpty = false);

}  // namespace photoscrypto
