#pragma once

#include "../domain/Assignment.hpp"
#include "../domain/AuditEntry.hpp"
#include "../domain/LatePenaltyPolicy.hpp"
#include "../domain/Mark.hpp"
#include "../domain/Organisation.hpp"
#include "../domain/Submission.hpp"

#include <optional>
#include <string>

// Read models the services hand to the Presentation layer, so screens never
// need SQL or business rules of their own.

struct AssignmentSummary {
    Assignment assignment;
    Course course;
    int submissionCount = 0;
};

struct ReleasedResult {
    double rawMark = 0;
    double penalty = 0;
    double finalMark = 0;
    int daysLate = 0;
    std::string feedback;
    DateTime releasedAt;
};

struct StudentAssignmentView {
    Assignment assignment;
    Course course;
    std::optional<Extension> extension;
    DateTime effectiveDeadline;
    std::optional<Submission> active;
    std::optional<ReleasedResult> result; // only once the HoD has released the mark
};

struct MarkingRow {
    Submission submission;
    std::string studentName;
    std::string studentUsername;
    DateTime effectiveDeadline;
    bool hasExtension = false;
    int daysLate = 0;
    std::optional<Mark> mark;
    std::optional<PenaltyResult> result;
};

struct ExtensionRow {
    int studentId = 0;
    std::string studentName;
    std::optional<Extension> extension;
};

struct AuditView {
    AuditEntry entry;
    std::string changedBy;
    std::string studentName;
    std::string assignmentTitle;
};
