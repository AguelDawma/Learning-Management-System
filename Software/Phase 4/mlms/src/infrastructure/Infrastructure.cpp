#include "Infrastructure.hpp"
#include "../domain/Errors.hpp"

#include <argon2.h>

#include <array>
#include <chrono>
#include <random>
#include <vector>

DateTime SystemClock::now() const {
    const auto secs =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
    return DateTime(secs.count());
}

std::string Argon2Hasher::hash(const std::string& password) const {
    constexpr std::size_t kSaltLen = 16;
    constexpr std::size_t kHashLen = 32;
    std::array<std::uint8_t, kSaltLen> salt{};
    std::random_device rng; // OS CSPRNG on Windows (rand_s) and Linux (/dev/urandom)
    for (auto& b : salt) b = static_cast<std::uint8_t>(rng() & 0xFF);

    const std::size_t encodedLen =
        argon2_encodedlen(timeCost_, memoryKiB_, parallelism_, kSaltLen, kHashLen, Argon2_id);
    std::vector<char> encoded(encodedLen);
    const int rc = argon2id_hash_encoded(timeCost_, memoryKiB_, parallelism_, password.data(), password.size(),
                                         salt.data(), salt.size(), kHashLen, encoded.data(), encoded.size());
    if (rc != ARGON2_OK) throw MlmsError(std::string("Password hashing failed: ") + argon2_error_message(rc));
    return std::string(encoded.data());
}

bool Argon2Hasher::verify(const std::string& password, const std::string& encodedHash) const {
    return argon2id_verify(encodedHash.c_str(), password.data(), password.size()) == ARGON2_OK;
}
