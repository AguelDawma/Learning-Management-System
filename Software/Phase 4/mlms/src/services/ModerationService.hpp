#pragma once

#include "../data/Database.hpp"
#include "../data/Repositories.hpp"
#include "../infrastructure/Infrastructure.hpp"
#include "MarkEvents.hpp"
#include "Views.hpp"

#include <optional>
#include <string>
#include <vector>

// Head of Department moderation and release (FR-19, FR-20). Every action is
// limited to courses in the HoD's own department (BR-06).
class ModerationService : public MarkSubject {
    IMarkRepository& marks_;
    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    IUserRepository& users_;
    const ILatePenaltyPolicy& policy_;
    IClock& clock_;
    ITransactionManager& tx_;

    int requireDepartment(const User& hod);
    Assignment requireDepartmentAssignment(const User& hod, int assignmentId);
    // Loads an active submission in the HoD's department together with its mark.
    std::pair<Submission, Mark> load(const User& hod, int submissionId);

public:
    ModerationService(IMarkRepository& marks, ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                      ICourseRepository& courses, IUserRepository& users, const ILatePenaltyPolicy& policy,
                      IClock& clock, ITransactionManager& tx)
        : marks_(marks), submissions_(submissions), assignments_(assignments), courses_(courses), users_(users),
          policy_(policy), clock_(clock), tx_(tx) {}

    std::vector<AssignmentSummary> departmentAssignments(const User& hod);
    std::vector<MarkingRow> moderationSheet(const User& hod, int assignmentId);

    // MARKED -> MODERATED for every id, all or nothing. Returns the number approved.
    int approve(const User& hod, const std::vector<int>& submissionIds);
    // MARKED -> RETURNED with a mandatory comment for the lecturer.
    void returnForRemarking(const User& hod, int submissionId, const std::string& comment);
    // Changes the raw mark of a MODERATED or RELEASED submission, with a reason.
    void adjustMark(const User& hod, int submissionId, double newRawMark, const std::string& reason,
                    std::optional<int> expectedRowVersion = std::nullopt);
    // MODERATED -> RELEASED for every id, all or nothing; students can then see the mark.
    int release(const User& hod, const std::vector<int>& submissionIds);
};
