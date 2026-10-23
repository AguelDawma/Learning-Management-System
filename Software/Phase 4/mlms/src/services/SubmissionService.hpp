#pragma once

#include "../data/AssignmentRepository.hpp"
#include "../data/SubmissionRepository.hpp"
#include "../domain/Submission.hpp"
#include <memory>
#include <stdexcept>

class SubmissionService {
    AssignmentRepository& assignmentRepo_;
    SubmissionRepository& submissionRepo_;

public:
    SubmissionService(AssignmentRepository& assignmentRepo, SubmissionRepository& submissionRepo)
        : assignmentRepo_(assignmentRepo), submissionRepo_(submissionRepo) {}

    bool isSubmissionLate(int assignmentId, int studentId, const std::string& submissionTimestamp) {
        auto assignment = assignmentRepo_.findById(assignmentId);
        if (!assignment) {
            throw std::runtime_error("Assignment not found");
        }

        auto extension = assignmentRepo_.findExtension(assignmentId, studentId);
        std::string effectiveDeadline = assignment->effectiveDeadline(extension.get());

        // Simple string comparison for ISO timestamps
        return submissionTimestamp > effectiveDeadline;
    }
};