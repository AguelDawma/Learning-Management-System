#pragma once

#include <string>

std::string trim(const std::string& s);

// Throws ValidationError when the text is blank after trimming.
void requireNotBlank(const std::string& value, const std::string& what);

// Usernames: 3-32 characters of a-z, 0-9, '.', '_' or '-'.
void validateUsername(const std::string& username);

// Passwords: at least 8 characters with at least one letter and one digit.
void validatePasswordStrength(const std::string& password);
