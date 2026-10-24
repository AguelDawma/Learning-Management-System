#include <QApplication>
#include <sqlite3.h>
#include <argon2.h>
#include <QDebug>
#include "data/UserRepository.hpp"
#include "data/AssignmentRepository.hpp"
#include "services/AuthService.hpp"
#include "ui/LoginWindow.hpp"

std::string hashPassword(const std::string& password) {
    char encoded[256];
    uint8_t salt[16] = {0}; 
    argon2id_hash_encoded(1, 65536, 1, password.c_str(), password.length(), 
                          salt, 16, 32, encoded, sizeof(encoded));
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
        ");"
        "CREATE TABLE assignment ("
        "  assignment_id INTEGER PRIMARY KEY,"
        "  course_id INTEGER NOT NULL,"
        "  title TEXT NOT NULL,"
        "  description TEXT,"
        "  deadline TEXT NOT NULL,"
        "  max_mark INTEGER NOT NULL,"
        "  is_published INTEGER NOT NULL,"
        "  created_by INTEGER REFERENCES app_user(user_id)"
        ");";

    sqlite3_exec(db, schema, nullptr, nullptr, nullptr);

    // Insert a test user
    std::string hash = hashPassword("password123");
    std::string insertUser = "INSERT INTO app_user(user_id, username, full_name, role, password_hash, is_active) "
                             "VALUES (1, 'student1', 'Jane Doe', 'STUDENT', '" + hash + "', 1);";
    sqlite3_exec(db, insertUser.c_str(), nullptr, nullptr, nullptr);

    // Insert a test lecturer user ("lecturer1" / "password123")
    std::string lecturerHash = hashPassword("password123");
    std::string insertLecturer = "INSERT INTO app_user(user_id, username, full_name, role, password_hash, is_active) "
                                 "VALUES (2, 'lecturer1', 'Dr. Smith', 'LECTURER', '" + lecturerHash + "', 1);";
    sqlite3_exec(db, insertLecturer.c_str(), nullptr, nullptr, nullptr);

    // Initialize repositories and test assignment insertion
    UserRepository userRepo(db);
    AssignmentRepository assignmentRepo(db);

    Assignment testAssignment(1, 101, "C++ Basics", "Introduction to C++17", "2026-06-01", 100, true, 1);
    assignmentRepo.save(testAssignment);

    AuthService authService(userRepo);

    LoginWindow loginWindow(authService, db);
    loginWindow.show();

    int result = app.exec();

    sqlite3_close(db);
    return result;
}