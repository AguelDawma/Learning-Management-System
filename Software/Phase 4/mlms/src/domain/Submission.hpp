#pragma once

#include <string>

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

    int id() const { return id_; }
    SubmissionState state() const { return state_; }
    
    bool isLate(const std::string& effectiveDeadline) const {
        return submittedAt_ > effectiveDeadline;
    }
};