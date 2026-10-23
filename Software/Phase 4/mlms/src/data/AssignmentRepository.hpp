#pragma once

#include "../domain/Assignment.hpp"
#include <sqlite3.h>
#include <memory>
#include <optional>
#include <stdexcept>

class AssignmentRepository {
    sqlite3* db_;

public:
    explicit AssignmentRepository(sqlite3* db) : db_(db) {}

    std::unique_ptr<Assignment> findById(int assignmentId) {
        const char* sql = "SELECT assignment_id, course_id, title, description, deadline, max_mark, is_published, created_by "
                          "FROM assignment WHERE assignment_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement for findById");
        }

        sqlite3_bind_int(stmt, 1, assignmentId);

        std::unique_ptr<Assignment> assignment = nullptr;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            int courseId = sqlite3_column_int(stmt, 1);
            std::string title(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            std::string description(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            std::string deadline(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
            int maxMark = sqlite3_column_int(stmt, 5);
            bool isPublished = sqlite3_column_int(stmt, 6) == 1;
            int createdBy = sqlite3_column_int(stmt, 7);

            assignment = std::make_unique<Assignment>(id, courseId, title, description, deadline, maxMark, isPublished, createdBy);
        }

        sqlite3_finalize(stmt);
        return assignment;
    }

    std::unique_ptr<Extension> findExtension(int assignmentId, int studentId) {
        const char* sql = "SELECT assignment_id, student_id, new_deadline, reason, granted_by, granted_at "
                          "FROM extension WHERE assignment_id = ? AND student_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement for findExtension");
        }

        sqlite3_bind_int(stmt, 1, assignmentId);
        sqlite3_bind_int(stmt, 2, studentId);

        std::unique_ptr<Extension> extension = nullptr;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int aId = sqlite3_column_int(stmt, 0);
            int sId = sqlite3_column_int(stmt, 1);
            std::string newDeadline(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
            std::string reason(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
            int grantedBy = sqlite3_column_int(stmt, 4);
            std::string grantedAt(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));

            extension = std::make_unique<Extension>(aId, sId, newDeadline, reason, grantedBy, grantedAt);
        }

        sqlite3_finalize(stmt);
        return extension;
    }
};