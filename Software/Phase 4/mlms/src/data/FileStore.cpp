#include "FileStore.hpp"
#include "../domain/Errors.hpp"
#include "../infrastructure/Sha256.hpp"

#include <algorithm>
#include <cctype>
#include <system_error>

namespace fs = std::filesystem;

namespace {

std::string lowerExtension(const fs::path& p) {
    std::string ext = p.extension().u8string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

// Keeps only characters that are safe in a file name on every platform.
std::string safeFileName(const fs::path& p) {
    std::string name = p.filename().u8string();
    for (char& c : name) {
        const auto u = static_cast<unsigned char>(c);
        if (!(std::isalnum(u) || c == '.' || c == '-' || c == '_')) c = '_';
    }
    return name.empty() ? std::string("file") : name;
}

} // namespace

std::set<std::string> LocalFileStore::defaultExtensions() {
    return {".pdf", ".doc", ".docx", ".odt", ".txt", ".rtf", ".zip", ".pptx", ".xlsx",
            ".cpp", ".hpp", ".h",    ".c",   ".py",  ".java", ".png", ".jpg", ".jpeg"};
}

LocalFileStore::LocalFileStore(fs::path root, std::uintmax_t maxBytes, std::set<std::string> allowedExtensions)
    : root_(std::move(root)), maxBytes_(maxBytes), allowedExtensions_(std::move(allowedExtensions)) {}

void LocalFileStore::validate(const fs::path& source) const {
    std::error_code ec;
    if (source.empty() || !fs::is_regular_file(source, ec)) {
        throw ValidationError("Please choose an existing file to submit");
    }
    const auto size = fs::file_size(source, ec);
    if (ec) throw ValidationError("The file could not be read");
    if (size == 0) throw ValidationError("The file is empty");
    if (size > maxBytes_) {
        throw ValidationError("The file is larger than the " + std::to_string(maxBytes_ / (1024 * 1024)) +
                              " MB limit");
    }
    if (allowedExtensions_.count(lowerExtension(source)) == 0) {
        std::string list;
        for (const auto& e : allowedExtensions_) list += (list.empty() ? "" : " ") + e;
        throw ValidationError("This file type is not accepted. Allowed types: " + list);
    }
}

StoredFile LocalFileStore::store(const fs::path& source, int assignmentId, int studentId, int version) {
    validate(source);
    const fs::path relative = fs::path("a" + std::to_string(assignmentId)) / ("s" + std::to_string(studentId)) /
                              fs::u8path("v" + std::to_string(version) + "_" + safeFileName(source));
    const fs::path target = root_ / relative;
    std::error_code ec;
    fs::create_directories(target.parent_path(), ec);
    if (ec) throw FileStoreError("Could not create the submissions folder: " + ec.message());
    // Never overwrite: an existing file belongs to an already saved version.
    if (!fs::copy_file(source, target, fs::copy_options::none, ec) || ec) {
        throw FileStoreError("Could not store the file: " + ec.message());
    }
    StoredFile out;
    out.relativePath = relative.generic_u8string();
    out.checksum = sha256File(target);
    out.size = fs::file_size(target, ec);
    return out;
}

void LocalFileStore::remove(const std::string& relativePath) noexcept {
    std::error_code ec;
    fs::remove(resolve(relativePath), ec);
}

fs::path LocalFileStore::resolve(const std::string& relativePath) const { return root_ / fs::u8path(relativePath); }
