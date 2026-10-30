#pragma once

#include "../data/Database.hpp"
#include "../data/FileStore.hpp"
#include "../data/Repositories.hpp"
#include "../infrastructure/Infrastructure.hpp"
#include "Views.hpp"

#include <filesystem>
#include <string>
#include <vector>

// What will happen if the student submits now (the alt fragment of Figure 7).
struct SubmissionPlan {
    enum class Kind { FirstOnTime, Resubmission, Late, Refused };
    Kind kind = Kind::FirstOnTime;
    DateTime now;
    DateTime effectiveDeadline;
    int daysLate = 0;
    int nextVersion = 1;
    std::string refusal;
};

struct Receipt {
    int submissionId = 0;
    int version = 0;
    DateTime submittedAt;
    SubmissionState state = SubmissionState::SUBMITTED;
    int daysLate = 0;
    std::string checksum;
};

class SubmissionService {
    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    IMarkRepository& marks_;
    IFileStore& files_;
    const ILatePenaltyPolicy& policy_;
    IClock& clock_;
    ITransactionManager& tx_;

    Assignment requireOpenAssignment(const User& student, int assignmentId);
    SubmissionPlan plan(const Assignment& a, int studentId, DateTime now);

public:
    SubmissionService(ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                      ICourseRepository& courses, IMarkRepository& marks, IFileStore& files,
                      const ILatePenaltyPolicy& policy, IClock& clock, ITransactionManager& tx)
        : submissions_(submissions), assignments_(assignments), courses_(courses), marks_(marks), files_(files),
          policy_(policy), clock_(clock), tx_(tx) {}

    // Published assignments of the student's courses with status and, once
    // released by the HoD, the mark and feedback (FR-17). Marks are hidden before release.
    std::vector<StudentAssignmentView> myAssignments(const User& student);
    std::vector<Course> myCourses(const User& student);

    SubmissionPlan preview(const User& student, int assignmentId);

    // BR-01..BR-03: on time -> SUBMITTED (a resubmission adds a new version);
    // late -> LATE with days late; a late replacement of existing work is refused.
    Receipt submit(const User& student, int assignmentId, const std::filesystem::path& file);

    std::vector<Submission> myVersions(const User& student, int assignmentId);
    std::filesystem::path pathOf(const Submission& s) const { return files_.resolve(s.filePath()); }
    const std::set<std::string>& allowedExtensions() const { return files_.allowedExtensions(); }
};
