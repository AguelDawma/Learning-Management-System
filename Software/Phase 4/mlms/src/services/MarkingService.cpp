#include "MarkingService.hpp"
#include "../domain/Errors.hpp"
#include "../domain/Validation.hpp"
#include "MarkingSheet.hpp"

Assignment MarkingService::requireTaughtAssignment(const User& lecturer, int assignmentId) {
    lecturer.require(Permission::MarkSubmission);
    auto a = assignments_.find(assignmentId);
    if (!a) throw NotFound("Assignment not found");
    if (!courses_.isLecturerOf(a->courseId(), lecturer.id())) {
        throw AccessDenied("You do not teach the course of this assignment");
    }
    return *a;
}

std::vector<MarkingRow> MarkingService::markingList(const User& lecturer, int assignmentId) {
    const auto a = requireTaughtAssignment(lecturer, assignmentId);
    return buildMarkingRows(a, submissions_, assignments_, marks_, users_, policy_);
}

void MarkingService::saveMark(const User& lecturer, int submissionId, double rawMark, const std::string& feedback,
                              const std::string& reason, std::optional<int> expectedRowVersion) {
    lecturer.require(Permission::MarkSubmission);
    Submission sub = requireActiveSubmission(submissions_, submissionId);
    const auto a = requireTaughtAssignment(lecturer, sub.assignmentId());
    Mark::validateRaw(rawMark, a.maxMark());
    Mark::validateFeedback(feedback);
    const std::string fb = trim(feedback);
    const std::string why = trim(reason);
    const DateTime now = clock_.now();
    const SubmissionState before = sub.state();

    auto tx = tx_.begin();
    if (sub.awaitingFirstMark()) {
        sub.transitionTo(SubmissionState::MARKED);
        marks_.insert(Mark(sub.id(), rawMark, fb, lecturer.id(), now));
        submissions_.updateState(sub.id(), sub.state());
        const std::string r = why.empty() ? "Initial marking" : why;
        notify({lecturer.id(), sub.id(), "raw_mark", std::nullopt, formatMark(rawMark), r, now});
        notify({lecturer.id(), sub.id(), "feedback", std::nullopt, fb, r, now});
        notify({lecturer.id(), sub.id(), "state", toString(before), toString(sub.state()), r, now});
    } else if (before == SubmissionState::MARKED || before == SubmissionState::RETURNED) {
        requireNotBlank(why, "The reason for changing the mark");
        auto existing = marks_.find(sub.id());
        if (!existing) throw NotFound("Mark not found");
        Mark m = *existing;
        if (expectedRowVersion && *expectedRowVersion != m.rowVersion()) {
            throw ConcurrencyError("This mark was changed by someone else since you opened it. Reload and try again.");
        }
        const bool rawChanged = m.rawMark() != rawMark;
        const bool fbChanged = m.feedback() != fb;
        if (before == SubmissionState::MARKED && !rawChanged && !fbChanged) {
            throw ValidationError("Nothing was changed");
        }
        if (rawChanged) notify({lecturer.id(), sub.id(), "raw_mark", formatMark(m.rawMark()), formatMark(rawMark), why, now});
        if (fbChanged) notify({lecturer.id(), sub.id(), "feedback", m.feedback(), fb, why, now});
        m.setRawMark(rawMark);
        m.setFeedback(fb);
        m.setMarkedBy(lecturer.id(), now);
        if (before == SubmissionState::RETURNED) {
            m.setReturnComment(std::nullopt);
            sub.transitionTo(SubmissionState::MARKED);
            submissions_.updateState(sub.id(), sub.state());
            notify({lecturer.id(), sub.id(), "state", toString(before), toString(sub.state()), why, now});
        }
        marks_.update(m);
    } else {
        throw InvalidTransition("This mark is with the Head of Department (" + toString(before) +
                                ") and can no longer be changed by the lecturer");
    }
    tx->commit();
}
