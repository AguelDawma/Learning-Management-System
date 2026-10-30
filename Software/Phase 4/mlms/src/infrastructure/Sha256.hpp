#pragma once

#include <filesystem>
#include <string>

// FIPS 180-4 SHA-256, used for the submission checksum. Returns 64 lowercase hex characters.
std::string sha256Hex(const std::string& data);
std::string sha256File(const std::filesystem::path& file); // throws FileStoreError
