#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>

struct StoredFile {
    std::string relativePath; // UTF-8, relative to the store root; saved in submission.file_path
    std::string checksum;     // SHA-256, 64 lowercase hex characters
    std::uintmax_t size = 0;
};

class IFileStore {
public:
    virtual ~IFileStore() = default;
    // FR-12: throws ValidationError for a missing, empty, too large or disallowed file.
    virtual void validate(const std::filesystem::path& source) const = 0;
    // Copies the file into the store; older versions are never overwritten.
    virtual StoredFile store(const std::filesystem::path& source, int assignmentId, int studentId, int version) = 0;
    virtual void remove(const std::string& relativePath) noexcept = 0;
    virtual std::filesystem::path resolve(const std::string& relativePath) const = 0;
    virtual const std::set<std::string>& allowedExtensions() const = 0;
};

// Keeps submissions under <root>/a<assignment>/s<student>/v<version>_<name>.
class LocalFileStore : public IFileStore {
    std::filesystem::path root_;
    std::uintmax_t maxBytes_;
    std::set<std::string> allowedExtensions_;

public:
    static constexpr std::uintmax_t kDefaultMaxBytes = 20ull * 1024 * 1024;
    static std::set<std::string> defaultExtensions();

    explicit LocalFileStore(std::filesystem::path root, std::uintmax_t maxBytes = kDefaultMaxBytes,
                            std::set<std::string> allowedExtensions = defaultExtensions());

    void validate(const std::filesystem::path& source) const override;
    StoredFile store(const std::filesystem::path& source, int assignmentId, int studentId, int version) override;
    void remove(const std::string& relativePath) noexcept override;
    std::filesystem::path resolve(const std::string& relativePath) const override;

    std::uintmax_t maxBytes() const { return maxBytes_; }
    const std::set<std::string>& allowedExtensions() const override { return allowedExtensions_; }
};
