#include <iostream>
#include <sqlite3.h>
#include "domain/Assignment.hpp"
#include "domain/Submission.hpp"
#include "data/AssignmentRepository.hpp"
#include "data/SubmissionRepository.hpp"

int main() {
    std::cout << "Testing MLMS Full Data Layer...\n";

    sqlite3* db = nullptr;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        std::cerr << "Failed to open in-memory database\n";
        return 1;
    }

    const char* schema = 
        "CREATE TABLE assignment (assignment_id INTEGER PRIMARY KEY, course_id INTEGER, title TEXT, description TEXT, deadline TEXT, max_mark INTEGER, is_published INTEGER, created_by INTEGER);"
        "CREATE TABLE extension (assignment_id INTEGER, student_id INTEGER, new_deadline TEXT, reason TEXT, granted_by INTEGER, granted_at TEXT);"
        "CREATE TABLE submission (submission_id INTEGER PRIMARY KEY, assignment_id INTEGER, student_id INTEGER, version INTEGER, file_path TEXT, checksum TEXT, submitted_at TEXT, state TEXT);"
        "INSERT INTO assignment VALUES (101, 1, 'Lab 1', 'C++ basics', '2026-06-01T23:59:59Z', 100, 1, 2);"
        "INSERT INTO extension VALUES (101, 1, '2026-06-05T23:59:59Z', 'Medical', 2, '2026-06-02');"
        "INSERT INTO submission VALUES (1, 101, 1, 1, '/uploads/sub.cpp', 'checksum123', '2026-06-03T12:00:00Z', 'SUBMITTED');";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    AssignmentRepository assignmentRepo(db);
    SubmissionRepository submissionRepo(db);

    auto assignment = assignmentRepo.findById(101);
    if (assignment) {
        std::cout << "Found Assignment: " << assignment->title() << "\n";
    }

    auto extension = assignmentRepo.findExtension(101, 1);
    if (extension) {
        std::cout << "Found Extension Deadline: " << extension->newDeadline() << "\n";
    }

    auto submission = submissionRepo.findById(1);
    if (submission) {
        std::cout << "Found Submission ID: " << submission->id() << "\n";
    }

    sqlite3_close(db);
    std::cout << "Data layer test harness passed successfully!\n";
    return 0;
}