#pragma once

#include "../data/Database.hpp"
#include "../data/Repositories.hpp"
#include "../infrastructure/Infrastructure.hpp"
#include "MarkEvents.hpp"
#include "Views.hpp"

#include <optional>
#include <string>
#include <vector>

// Lecturer marking (FR-13, FR-16). Marks stay hidden from students until the
// HoD releases them (resolution of Conflict 1).
class MarkingService : public MarkSubject {
    IMarkRepository& marks_;
    ISubmissionRepository& submissions_;
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    IUserRepository& users_;
    const ILatePenaltyPolicy& policy_;
    IClock& clock_;
    ITransactionManager& tx_;

    Assignment requireTaughtAssignment(const User& lecturer, int assignmentId);

public:
    MarkingService(IMarkRepository& marks, ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                   ICourseRepository& courses, IUserRepository& users, const ILatePenaltyPolicy& policy,
                   IClock& clock, ITransactionManager& tx)
        : marks_(marks), submissions_(submissions), assignments_(assignments), courses_(courses), users_(users),
          policy_(policy), clock_(clock), tx_(tx) {}

    std::vector<MarkingRow> markingList(const User& lecturer, int assignmentId);

    // What the student would receive for this raw mark (BR-02), for the marking screen.
    PenaltyResult previewPenalty(double rawMark, int maxMark, int daysLate) const {
        return policy_.apply(rawMark, maxMark, daysLate);
    }

    // First mark (SUBMITTED/LATE -> MARKED), edit (MARKED, reason required) or
    // re-mark after the HoD returned it (RETURNED -> MARKED, reason required).
    // expectedRowVersion is the row_version the lecturer loaded (optimistic locking).
    void saveMark(const User& lecturer, int submissionId, double rawMark, const std::string& feedback,
                  const std::string& reason, std::optional<int> expectedRowVersion = std::nullopt);
};
