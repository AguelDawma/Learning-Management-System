#include "Submission.hpp"
#include "Errors.hpp"

std::string toString(SubmissionState s) {
    switch (s) {
        case SubmissionState::SUBMITTED: return "SUBMITTED";
        case SubmissionState::LATE: return "LATE";
        case SubmissionState::MARKED: return "MARKED";
        case SubmissionState::RETURNED: return "RETURNED";
        case SubmissionState::MODERATED: return "MODERATED";
        case SubmissionState::RELEASED: return "RELEASED";
    }
    return "SUBMITTED";
}

SubmissionState submissionStateFromString(const std::string& text) {
    if (text == "SUBMITTED") return SubmissionState::SUBMITTED;
    if (text == "LATE") return SubmissionState::LATE;
    if (text == "MARKED") return SubmissionState::MARKED;
    if (text == "RETURNED") return SubmissionState::RETURNED;
    if (text == "MODERATED") return SubmissionState::MODERATED;
    if (text == "RELEASED") return SubmissionState::RELEASED;
    throw ValidationError("Unknown submission state '" + text + "'");
}

Submission::Submission(int id, int assignmentId, int studentId, int version, std::string filePath,
                       std::string checksum, DateTime submittedAt, SubmissionState state)
    : id_(id), assignmentId_(assignmentId), studentId_(studentId), version_(version), filePath_(std::move(filePath)),
      checksum_(std::move(checksum)), submittedAt_(submittedAt), state_(state) {
    if (version_ < 1) throw ValidationError("Submission version must be at least 1");
}

int Submission::daysLate(DateTime effectiveDeadline) const {
    const std::int64_t diff = submittedAt_.epochSeconds() - effectiveDeadline.epochSeconds();
    if (diff <= 0) return 0;
    return static_cast<int>((diff + DateTime::kSecondsPerDay - 1) / DateTime::kSecondsPerDay);
}

// The transitions of the Submission state machine (design Figure 5). Mark edits
// (editMark, adjustMark, correctMark) keep the state, so they are not listed.
bool Submission::canTransition(SubmissionState from, SubmissionState to) {
    using S = SubmissionState;
    switch (from) {
        case S::LATE: return to == S::SUBMITTED || to == S::MARKED;
        case S::SUBMITTED: return to == S::MARKED;
        case S::MARKED: return to == S::RETURNED || to == S::MODERATED;
        case S::RETURNED: return to == S::MARKED;
        case S::MODERATED: return to == S::RELEASED;
        case S::RELEASED: return false;
    }
    return false;
}

void Submission::transitionTo(SubmissionState next) {
    if (!canTransition(state_, next)) {
        throw InvalidTransition("A submission cannot move from " + toString(state_) + " to " + toString(next));
    }
    state_ = next;
}
