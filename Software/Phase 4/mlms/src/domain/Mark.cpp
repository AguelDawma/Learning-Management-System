#include "Mark.hpp"
#include "Errors.hpp"
#include "Validation.hpp"

#include <cmath>

Mark::Mark(int submissionId, double rawMark, std::string feedback, int markedBy, DateTime markedAt,
           std::optional<int> moderatedBy, std::optional<DateTime> releasedAt,
           std::optional<std::string> returnComment, int rowVersion)
    : submissionId_(submissionId), rawMark_(rawMark), feedback_(std::move(feedback)), markedBy_(markedBy),
      markedAt_(markedAt), moderatedBy_(moderatedBy), releasedAt_(releasedAt),
      returnComment_(std::move(returnComment)), rowVersion_(rowVersion) {}

void Mark::validateRaw(double raw, int maxMark) {
    if (!std::isfinite(raw) || raw < 0.0 || raw > maxMark) {
        throw ValidationError("Mark must be between 0 and " + std::to_string(maxMark));
    }
}

void Mark::validateFeedback(const std::string& feedback) {
    if (trim(feedback).size() < kMinFeedbackLength) {
        throw ValidationError("Written feedback must be at least 10 characters");
    }
}
