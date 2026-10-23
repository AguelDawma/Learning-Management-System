#pragma once

#include "../domain/Submission.hpp"
#include <sqlite3.h>
#include <memory>
#include <stdexcept>

class SubmissionRepository {
    sqlite3* db_;

    SubmissionState parseState(const std::string& stateStr) {
        if (stateStr == "LATE") return SubmissionState::LATE;
        if (stateStr == "MARKED") return SubmissionState::MARKED;
        if (stateStr == "RETURNED") return SubmissionState::RETURNED;
        if (stateStr == "MODERATED") return SubmissionState::MODERATED;
        if (stateStr == "RELEASED") return SubmissionState::RELEASED;
        return SubmissionState::SUBMITTED;
    }

public:
    explicit SubmissionRepository(sqlite3* db) : db_(db) {}

    std::unique_ptr<Submission> findById(int submissionId) {
        const char* sql = "SELECT submission_id, assignment_id, student_id, version, file_path, checksum, submitted_at, state "
                          "FROM submission WHERE submission_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement for findById");
        }

        sqlite3_bind_int(stmt, 1, submissionId);

        std::unique_ptr<Submission> submission = nullptr;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            int assignmentId = sqlite3_column_int(stmt, 1);
            int studentId = sqlite3_column_int(stmt, 2);
            int version = sqlite3_column_int(stmt, 3);
            std::string filePath(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            std::string checksum(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
            std::string submittedAt(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
            std::string stateStr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));

            submission = std::make_unique<Submission>(
                id, assignmentId, studentId, version, filePath, checksum, submittedAt, parseState(stateStr)
            );
        }

        sqlite3_finalize(stmt);
        return submission;
    }
};