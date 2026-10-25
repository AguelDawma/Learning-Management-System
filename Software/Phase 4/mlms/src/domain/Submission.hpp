#pragma once

#include <string>
#include <stdexcept>
#include <utility>

enum class SubmissionState {
    SUBMITTED,
    LATE,
    MARKED,
    RETURNED,
    MODERATED,
    RELEASED
};

class Submission {
    int id_;
    int assignmentId_;
    int studentId_;
    int version_;
    std::string filePath_;
    std::string checksum_;
    std::string submittedAt_;
    SubmissionState state_;

public:
    Submission(int id, int assignmentId, int studentId, int version, 
               std::string filePath, std::string checksum, 
               std::string submittedAt, SubmissionState state)
        : id_(id), assignmentId_(assignmentId), studentId_(studentId), 
          version_(version), filePath_(std::move(filePath)), 
          checksum_(std::move(checksum)), submittedAt_(std::move(submittedAt)), 
          state_(state) {}

    // Getters
    int id() const { return id_; }
    int assignmentId() const { return assignmentId_; }
    int studentId() const { return studentId_; }
    int version() const { return version_; }
    const std::string& filePath() const { return filePath_; }
    const std::string& checksum() const { return checksum_; }
    const std::string& submittedAt() const { return submittedAt_; }
    SubmissionState state() const { return state_; }

    // Business Logic: Check if submission is past the effective deadline
    bool isLate(const std::string& effectiveDeadline) const {
        return submittedAt_ > effectiveDeadline;
    }

    // Business Logic: State machine transition enforcement (FR-15)
    void transitionTo(SubmissionState newState) {
        bool valid = false;
        switch (state_) {
            case SubmissionState::SUBMITTED:
            case SubmissionState::LATE:
                if (newState == SubmissionState::MARKED) valid = true;
                break;
            case SubmissionState::MARKED:
                if (newState == SubmissionState::RETURNED || newState == SubmissionState::MODERATED) valid = true;
                break;
            case SubmissionState::RETURNED:
                if (newState == SubmissionState::MARKED) valid = true;
                break;
            case SubmissionState::MODERATED:
                if (newState == SubmissionState::RELEASED) valid = true;
                break;
            case SubmissionState::RELEASED:
                break;
        }

        if (!valid) {
            throw std::runtime_error("InvalidTransition: Submission state change not permitted.");
        }
        state_ = newState;
    }
};