#pragma once

#include "../data/UserRepository.hpp"
#include <argon2.h>
#include <string>

class AuthService {
    UserRepository& userRepo_;

public:
    explicit AuthService(UserRepository& userRepo) : userRepo_(userRepo) {}

    bool authenticate(const std::string& username, const std::string& password) {
        auto user = userRepo_.findByUsername(username);
        if (!user || !user->isActive()) {
            return false;
        }

        int result = argon2id_verify(user->passwordHash().c_str(), password.c_str(), password.length());
        return (result == ARGON2_OK);
    }
};