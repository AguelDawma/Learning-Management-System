#include <QApplication>
#include <sqlite3.h>
#include <argon2.h>
#include "data/UserRepository.hpp"
#include "services/AuthService.hpp"
#include "ui/LoginWindow.hpp"

// Helper to hash a password using argon2 for testing
std::string hashPassword(const std::string& password) {
    char encoded[64];
    uint8_t salt[16] = {0}; // deterministic salt for testing
    argon2id_hash_encoded(1, 65536, 1, password.c_str(), password.length(), 
                          salt, 16, 32, encoded, 64);
    return std::string(encoded);
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    sqlite3* db = nullptr;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        return 1;
    }

    const char* schema = 
        "CREATE TABLE app_user ("
        "  user_id INTEGER PRIMARY KEY,"
        "  username TEXT NOT NULL UNIQUE,"
        "  full_name TEXT NOT NULL,"
        "  role TEXT NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  is_active INTEGER NOT NULL DEFAULT 1"
        ");"
        "CREATE TABLE department ("
        "  department_id INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  hod_id INTEGER REFERENCES app_user(user_id)"
        ");";

    sqlite3_exec(db, schema, nullptr, nullptr, nullptr);

    // Insert a test user with hashed password ("password123")
    std::string hash = hashPassword("password123");
    std::string insertUser = "INSERT INTO app_user(user_id, username, full_name, role, password_hash, is_active) "
                             "VALUES (1, 'student1', 'Jane Doe', 'STUDENT', '" + hash + "', 1);";
    sqlite3_exec(db, insertUser.c_str(), nullptr, nullptr, nullptr);

    UserRepository userRepo(db);
    AuthService authService(userRepo);

    LoginWindow loginWindow(authService);
    loginWindow.show();

    int result = app.exec();

    sqlite3_close(db);
    return result;
}