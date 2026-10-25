#pragma once

#include "../domain/Submission.hpp"
#include <sqlite3.h>
#include <vector>
#include <optional>

class SubmissionRepository {
    sqlite3* db_;

    // Helper to convert SubmissionState enum to string representation for SQLite storage
    std::string stateToString(SubmissionState state) const {
        switch (state) {
            case SubmissionState::SUBMITTED: return "SUBMITTED";
            case SubmissionState::LATE: return "LATE";
            case SubmissionState::MARKED: return "MARKED";
            case SubmissionState::RETURNED: return "RETURNED";
            case SubmissionState::MODERATED: return "MODERATED";
            case SubmissionState::RELEASED: return "RELEASED";
        }
        return "SUBMITTED";
    }

    SubmissionState stringToState(const std::string& str) const {
        if (str == "LATE") return SubmissionState::LATE;
        if (str == "MARKED") return SubmissionState::MARKED;
        if (str == "RETURNED") return SubmissionState::RETURNED;
        if (str == "MODERATED") return SubmissionState::MODERATED;
        if (str == "RELEASED") return SubmissionState::RELEASED;
        return SubmissionState::SUBMITTED;
    }

public:
    explicit SubmissionRepository(sqlite3* db) : db_(db) {}

    void save(const Submission& sub) {
        const char* sql = "INSERT INTO submission (submission_id, assignment_id, student_id, version, file_path, checksum, submitted_at, state) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, sub.id());
            sqlite3_bind_int(stmt, 2, sub.assignmentId());
            sqlite3_bind_int(stmt, 3, sub.studentId());
            sqlite3_bind_int(stmt, 4, sub.version());
            sqlite3_bind_text(stmt, 5, sub.filePath().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 6, sub.checksum().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 7, sub.submittedAt().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 8, stateToString(sub.state()).c_str(), -1, SQLITE_STATIC);
            
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
};