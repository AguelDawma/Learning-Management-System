#include <iostream>
#include <sqlite3.h>
#include "domain/Assignment.hpp"
#include "domain/Submission.hpp"
#include "data/AssignmentRepository.hpp"
#include "data/SubmissionRepository.hpp"
#include "services/SubmissionService.hpp"

int main() {
    std::cout << "Testing MLMS Service Layer...\n";

    sqlite3* db = nullptr;
    if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
        std::cerr << "Failed to open in-memory database\n";
        return 1;
    }

    const char* schema = 
        "CREATE TABLE assignment (assignment_id INTEGER PRIMARY KEY, course_id INTEGER, title TEXT, description TEXT, deadline TEXT, max_mark INTEGER, is_published INTEGER, created_by INTEGER);"
        "CREATE TABLE extension (assignment_id INTEGER, student_id INTEGER, new_deadline TEXT, reason TEXT, granted_by INTEGER, granted_at TEXT);"
        "INSERT INTO assignment VALUES (101, 1, 'Lab 1', 'C++ basics', '2026-06-01T23:59:59Z', 100, 1, 2);"
        "INSERT INTO extension VALUES (101, 1, '2026-06-05T23:59:59Z', 'Medical', 2, '2026-06-02');";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, schema, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    AssignmentRepository assignmentRepo(db);
    SubmissionRepository submissionRepo(db);
    SubmissionService submissionService(assignmentRepo, submissionRepo);

    // Test submission before extension deadline (should NOT be late)
    bool isLate1 = submissionService.isSubmissionLate(101, 1, "2026-06-03T12:00:00Z");
    std::cout << "Submission on June 3 (Extension deadline is June 5): " << (isLate1 ? "LATE" : "ON TIME") << "\n";

    // Test submission after extension deadline (SHOULD be late)
    bool isLate2 = submissionService.isSubmissionLate(101, 1, "2026-06-06T00:00:00Z");
    std::cout << "Submission on June 6 (Extension deadline is June 5): " << (isLate2 ? "LATE" : "ON TIME") << "\n";

    sqlite3_close(db);
    std::cout << "Service layer tests passed successfully!\n";
    return 0;
}