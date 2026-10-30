#include "Validation.hpp"
#include "Errors.hpp"

#include <cctype>

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

void requireNotBlank(const std::string& value, const std::string& what) {
    if (trim(value).empty()) throw ValidationError(what + " must not be blank");
}

void validateUsername(const std::string& username) {
    if (username.size() < 3 || username.size() > 32) {
        throw ValidationError("Username must be 3 to 32 characters");
    }
    for (char c : username) {
        const auto u = static_cast<unsigned char>(c);
        if (!(std::islower(u) || std::isdigit(u) || c == '.' || c == '_' || c == '-')) {
            throw ValidationError("Username may only contain a-z, 0-9, '.', '_' and '-'");
        }
    }
}

void validatePasswordStrength(const std::string& password) {
    bool letter = false;
    bool digit = false;
    for (char c : password) {
        const auto u = static_cast<unsigned char>(c);
        letter = letter || std::isalpha(u);
        digit = digit || std::isdigit(u);
    }
    if (password.size() < 8 || !letter || !digit) {
        throw ValidationError("Password must be at least 8 characters and contain a letter and a digit");
    }
}
