#pragma once

#include "../domain/DateTime.hpp"

#include <string>

class IClock {
public:
    virtual ~IClock() = default;
    virtual DateTime now() const = 0;
};

// Reads the host computer's clock (UTC). Submission times always come from
// here, never from user input (FR-12).
class SystemClock : public IClock {
public:
    DateTime now() const override;
};

class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;
    // Returns a self-describing encoded hash that includes the random salt.
    virtual std::string hash(const std::string& password) const = 0;
    virtual bool verify(const std::string& password, const std::string& encodedHash) const = 0;
};

// Argon2id via the reference libargon2 (winner of the Password Hashing
// Competition, RFC 9106). Parameters follow the OWASP minimum: 19 MiB memory,
// 2 iterations, 1 lane - about 0.1 s per login on old lab computers.
class Argon2Hasher : public IPasswordHasher {
    unsigned timeCost_;
    unsigned memoryKiB_;
    unsigned parallelism_;

public:
    explicit Argon2Hasher(unsigned timeCost = 2, unsigned memoryKiB = 19456, unsigned parallelism = 1)
        : timeCost_(timeCost), memoryKiB_(memoryKiB), parallelism_(parallelism) {}
    std::string hash(const std::string& password) const override;
    bool verify(const std::string& password, const std::string& encodedHash) const override;
};
