#include "MarkingSheet.hpp"
#include "../domain/Errors.hpp"

#include <map>

std::vector<MarkingRow> buildMarkingRows(const Assignment& assignment, ISubmissionRepository& submissions,
                                         IAssignmentRepository& assignments, IMarkRepository& marks,
                                         IUserRepository& users, const ILatePenaltyPolicy& policy) {
    std::map<int, Extension> extensions;
    for (const auto& e : assignments.findExtensions(assignment.id())) extensions.emplace(e.studentId, e);

    std::vector<MarkingRow> rows;
    for (const auto& sub : submissions.findActiveByAssignment(assignment.id())) {
        const auto ext = extensions.find(sub.studentId());
        const Extension* extPtr = ext == extensions.end() ? nullptr : &ext->second;
        const DateTime effective = assignment.effectiveDeadline(extPtr);
        const auto student = users.findById(sub.studentId());

        MarkingRow row{sub,
                       student ? student->fullName : "(unknown)",
                       student ? student->username : "",
                       effective,
                       extPtr != nullptr,
                       sub.daysLate(effective),
                       marks.find(sub.id()),
                       std::nullopt};
        if (row.mark) row.result = row.mark->finalise(policy, assignment.maxMark(), row.daysLate);
        rows.push_back(std::move(row));
    }
    return rows;
}

Submission requireActiveSubmission(ISubmissionRepository& submissions, int submissionId) {
    auto sub = submissions.find(submissionId);
    if (!sub) throw NotFound("Submission not found");
    auto active = submissions.findActive(sub->assignmentId(), sub->studentId());
    if (!active || active->id() != sub->id()) {
        throw InvalidTransition("Only the student's latest submission (version " +
                                std::to_string(active ? active->version() : 0) + ") can be marked");
    }
    return *sub;
}
