#pragma once

#include "../domain/User.hpp"
#include "../domain/UserFactory.hpp"
#include <sqlite3.h>
#include <memory>
#include <optional>
#include <stdexcept>

class UserRepository {
    sqlite3* db_;

public:
    explicit UserRepository(sqlite3* db) : db_(db) {}

    std::unique_ptr<User> findByUsername(const std::string& username) {
        const char* sql = "SELECT user_id, username, full_name, role, password_hash, is_active, department_id "
                          "FROM app_user LEFT JOIN department ON department.hod_id = app_user.user_id "
                          "WHERE username = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement for findByUsername");
        }

        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

        std::unique_ptr<User> user = nullptr;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            std::string uname(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
            std::string fullName(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            std::string roleStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string passwordHash(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            bool active = sqlite3_column_int(stmt, 5) == 1;
            int deptId = sqlite3_column_int(stmt, 6);

            Role role = Role::STUDENT;
            if (roleStr == "LECTURER") role = Role::LECTURER;
            else if (roleStr == "HOD") role = Role::HOD;
            else if (roleStr == "ADMINISTRATOR") role = Role::ADMINISTRATOR;

            user = UserFactory::create(role, id, uname, fullName, passwordHash, active, deptId);
        }

        sqlite3_finalize(stmt);
        return user;
    }
};