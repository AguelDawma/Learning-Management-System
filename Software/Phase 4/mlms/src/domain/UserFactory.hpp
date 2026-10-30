#pragma once

#include "DateTime.hpp"
#include "User.hpp"

#include <memory>
#include <optional>
#include <string>

// One row of app_user (plus the department an HoD heads), as read by the repository.
struct UserRecord {
    int id = 0;
    std::string username;
    std::string fullName;
    Role role = Role::STUDENT;
    std::string passwordHash;
    bool active = true;
    int failedAttempts = 0;
    std::optional<DateTime> lockedUntil;
    bool mustChangePassword = false;
    int departmentId = 0;
};

// Factory pattern: builds the right User subclass from the role column so no
// other code needs a switch on role.
class UserFactory {
public:
    static std::unique_ptr<User> create(Role role, const UserRecord& row);
};
