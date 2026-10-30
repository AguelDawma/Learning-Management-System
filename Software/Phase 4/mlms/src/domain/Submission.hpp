#pragma once

#include "DateTime.hpp"

#include <string>

enum class SubmissionState { SUBMITTED, LATE, MARKED, RETURNED, MODERATED, RELEASED };

std::string toString(SubmissionState s);
SubmissionState submissionStateFromString(const std::string& text); // throws ValidationError

class Submission {
    int id_;
    int assignmentId_;
    int studentId_;
    int version_;
    std::string filePath_;
    std::string checksum_;
    DateTime submittedAt_;
    SubmissionState state_;

public:
    Submission(int id, int assignmentId, int studentId, int version, std::string filePath, std::string checksum,
               DateTime submittedAt, SubmissionState state);

    int id() const { return id_; }
    int assignmentId() const { return assignmentId_; }
    int studentId() const { return studentId_; }
    int version() const { return version_; }
    const std::string& filePath() const { return filePath_; }
    const std::string& checksum() const { return checksum_; }
    DateTime submittedAt() const { return submittedAt_; }
    SubmissionState state() const { return state_; }

    void setId(int id) { id_ = id; }

    // Whole days late, rounded up: one second late counts as one day (BR-02).
    int daysLate(DateTime effectiveDeadline) const;

    static bool canTransition(SubmissionState from, SubmissionState to);
    // Moves along the life cycle; throws InvalidTransition for any other change (FR-15).
    void transitionTo(SubmissionState next);

    // Still waiting for its first mark from the lecturer.
    bool awaitingFirstMark() const {
        return state_ == SubmissionState::SUBMITTED || state_ == SubmissionState::LATE;
    }
};
