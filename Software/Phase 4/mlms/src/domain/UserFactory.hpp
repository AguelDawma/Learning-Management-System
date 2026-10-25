#pragma once

#include "User.hpp"
#include <memory>
#include <stdexcept>

class UserFactory {
public:
    static std::unique_ptr<User> create(Role role, int id, const std::string& username, 
                                        const std::string& fullName, const std::string& passwordHash, 
                                        bool active, int departmentId = 0) {
        switch (role) {
            case Role::STUDENT:
                return std::make_unique<Student>(id, username, fullName, passwordHash, active);
            case Role::LECTURER:
                return std::make_unique<Lecturer>(id, username, fullName, passwordHash, active);
            case Role::HOD:
                return std::make_unique<HeadOfDepartment>(id, username, fullName, passwordHash, active, departmentId);
            case Role::ADMINISTRATOR:
                return std::make_unique<Administrator>(id, username, fullName, passwordHash, active);
            default:
                throw std::invalid_argument("Unknown user role");
        }
    }
};