#pragma once

#include "../data/UserRepository.hpp"
#include <argon2.h>
#include <string>
#include <memory>

class AuthService {
    UserRepository& userRepo_;

public:
    explicit AuthService(UserRepository& userRepo) : userRepo_(userRepo) {}

    std::unique_ptr<User> authenticate(const std::string& username, const std::string& password) {
        auto user = userRepo_.findByUsername(username);
        if (!user || !user->isActive()) {
            return nullptr;
        }

        int result = argon2id_verify(user->passwordHash().c_str(), password.c_str(), password.length());
        if (result == ARGON2_OK) {
            return user;
        }
        return nullptr;
    }
};