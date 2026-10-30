#pragma once

#include "../data/Repositories.hpp"
#include "../infrastructure/Infrastructure.hpp"

#include <memory>
#include <string>

struct LoginResult {
    std::unique_ptr<User> user;
    bool mustChangePassword = false;
};

class AuthService {
    IUserRepository& users_;
    IPasswordHasher& hasher_;
    IClock& clock_;
    std::string dummyHash_;

public:
    static constexpr int kMaxFailedAttempts = 5;
    static constexpr int kLockoutMinutes = 15;

    AuthService(IUserRepository& users, IPasswordHasher& hasher, IClock& clock);

    // Throws AuthenticationError with a message safe to show the user.
    // Five wrong passwords lock the account for 15 minutes (FR-03).
    LoginResult login(const std::string& username, const std::string& password);

    // FR-04: required after an administrator reset before anything else.
    void changePassword(const User& user, const std::string& currentPassword, const std::string& newPassword);
};
