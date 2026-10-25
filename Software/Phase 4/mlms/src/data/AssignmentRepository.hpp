#pragma once

#include "../domain/Assignment.hpp"
#include <sqlite3.h>
#include <vector>

class AssignmentRepository {
    sqlite3* db_;

public:
    explicit AssignmentRepository(sqlite3* db) : db_(db) {}

    void save(const Assignment& assignment) {
        const char* sql = "INSERT INTO assignment (assignment_id, course_id, title, description, deadline, max_mark, is_published, created_by) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, assignment.id());
            sqlite3_bind_int(stmt, 2, assignment.courseId());
            sqlite3_bind_text(stmt, 3, assignment.title().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, assignment.description().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, assignment.deadline().c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 6, assignment.maxMark());
            sqlite3_bind_int(stmt, 7, assignment.isPublished() ? 1 : 0);
            sqlite3_bind_int(stmt, 8, assignment.createdBy());
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    std::vector<Assignment> findAll() {
        std::vector<Assignment> assignments;
        const char* sql = "SELECT assignment_id, course_id, title, description, deadline, max_mark, is_published, created_by FROM assignment;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                int id = sqlite3_column_int(stmt, 0);
                int courseId = sqlite3_column_int(stmt, 1);
                std::string title(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
                std::string desc(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
                std::string deadline(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
                int maxMark = sqlite3_column_int(stmt, 5);
                bool isPublished = sqlite3_column_int(stmt, 6) != 0;
                int createdBy = sqlite3_column_int(stmt, 7);
                
                assignments.emplace_back(id, courseId, title, desc, deadline, maxMark, isPublished, createdBy);
            }
            sqlite3_finalize(stmt);
        }
        return assignments;
    }
};