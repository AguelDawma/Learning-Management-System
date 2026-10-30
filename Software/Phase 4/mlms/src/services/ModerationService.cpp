#include "ModerationService.hpp"
#include "../domain/Errors.hpp"
#include "../domain/Validation.hpp"
#include "MarkingSheet.hpp"

int ModerationService::requireDepartment(const User& hod) {
    hod.require(Permission::ModerateMarks);
    const auto* h = dynamic_cast<const HeadOfDepartment*>(&hod);
    if (h == nullptr || h->departmentId() == 0) {
        throw AccessDenied("You have not been assigned to a department yet. Please contact the administrator.");
    }
    return h->departmentId();
}

Assignment ModerationService::requireDepartmentAssignment(const User& hod, int assignmentId) {
    const int dept = requireDepartment(hod);
    auto a = assignments_.find(assignmentId);
    if (!a) throw NotFound("Assignment not found");
    auto c = courses_.findCourse(a->courseId());
    if (!c || c->departmentId != dept) throw AccessDenied("This assignment belongs to another department (BR-06)");
    return *a;
}

std::pair<Submission, Mark> ModerationService::load(const User& hod, int submissionId) {
    Submission sub = requireActiveSubmission(submissions_, submissionId);
    requireDepartmentAssignment(hod, sub.assignmentId());
    auto m = marks_.find(submissionId);
    if (!m) throw InvalidTransition("This submission has not been marked yet");
    return {sub, *m};
}

std::vector<AssignmentSummary> ModerationService::departmentAssignments(const User& hod) {
    const int dept = requireDepartment(hod);
    std::vector<AssignmentSummary> out;
    for (const auto& c : courses_.coursesForDepartment(dept)) {
        for (const auto& a : assignments_.findByCourse(c.id)) {
            out.push_back(AssignmentSummary{a, c, static_cast<int>(submissions_.findActiveByAssignment(a.id()).size())});
        }
    }
    return out;
}

std::vector<MarkingRow> ModerationService::moderationSheet(const User& hod, int assignmentId) {
    const auto a = requireDepartmentAssignment(hod, assignmentId);
    return buildMarkingRows(a, submissions_, assignments_, marks_, users_, policy_);
}

int ModerationService::approve(const User& hod, const std::vector<int>& submissionIds) {
    hod.require(Permission::ModerateMarks);
    if (submissionIds.empty()) throw ValidationError("Select at least one marked submission");
    const DateTime now = clock_.now();
    auto tx = tx_.begin();
    for (int id : submissionIds) {
        auto [sub, mark] = load(hod, id);
        const auto before = sub.state();
        sub.transitionTo(SubmissionState::MODERATED);
        mark.setModeratedBy(hod.id());
        marks_.update(mark);
        submissions_.updateState(id, sub.state());
        notify({hod.id(), id, "state", toString(before), toString(sub.state()), "Approved by Head of Department", now});
    }
    tx->commit();
    return static_cast<int>(submissionIds.size());
}

void ModerationService::returnForRemarking(const User& hod, int submissionId, const std::string& comment) {
    hod.require(Permission::ModerateMarks);
    requireNotBlank(comment, "The comment for the lecturer");
    const DateTime now = clock_.now();
    auto tx = tx_.begin();
    auto [sub, mark] = load(hod, submissionId);
    const auto before = sub.state();
    sub.transitionTo(SubmissionState::RETURNED);
    mark.setReturnComment(trim(comment));
    marks_.update(mark);
    submissions_.updateState(submissionId, sub.state());
    notify({hod.id(), submissionId, "state", toString(before), toString(sub.state()), trim(comment), now});
    tx->commit();
}

void ModerationService::adjustMark(const User& hod, int submissionId, double newRawMark, const std::string& reason,
                                   std::optional<int> expectedRowVersion) {
    hod.require(Permission::ModerateMarks);
    requireNotBlank(reason, "The reason for adjusting the mark");
    const DateTime now = clock_.now();
    auto tx = tx_.begin();
    auto [sub, mark] = load(hod, submissionId);
    if (sub.state() != SubmissionState::MODERATED && sub.state() != SubmissionState::RELEASED) {
        throw InvalidTransition("Only moderated or released marks can be adjusted. Return the mark to the "
                                "lecturer instead.");
    }
    const auto a = assignments_.find(sub.assignmentId());
    Mark::validateRaw(newRawMark, a->maxMark());
    if (expectedRowVersion && *expectedRowVersion != mark.rowVersion()) {
        throw ConcurrencyError("This mark was changed by someone else since you opened it. Reload and try again.");
    }
    if (mark.rawMark() == newRawMark) throw ValidationError("The new mark is the same as the current mark");
    const std::string field = sub.state() == SubmissionState::RELEASED ? "raw_mark (corrected after release)"
                                                                        : "raw_mark";
    notify({hod.id(), submissionId, field, formatMark(mark.rawMark()), formatMark(newRawMark), trim(reason), now});
    mark.setRawMark(newRawMark);
    mark.setModeratedBy(hod.id());
    marks_.update(mark);
    tx->commit();
}

int ModerationService::release(const User& hod, const std::vector<int>& submissionIds) {
    hod.require(Permission::ReleaseMarks);
    if (submissionIds.empty()) throw ValidationError("Select at least one moderated submission");
    const DateTime now = clock_.now();
    auto tx = tx_.begin();
    for (int id : submissionIds) {
        auto [sub, mark] = load(hod, id);
        const auto before = sub.state();
        sub.transitionTo(SubmissionState::RELEASED);
        mark.setReleasedAt(now);
        marks_.update(mark);
        submissions_.updateState(id, sub.state());
        notify({hod.id(), id, "state", toString(before), toString(sub.state()), "Released to student", now});
    }
    tx->commit();
    return static_cast<int>(submissionIds.size());
}
