#include "Assignment.hpp"
#include "Errors.hpp"
#include "Validation.hpp"

Assignment::Assignment(int id, int courseId, std::string title, std::string description, DateTime deadline,
                       int maxMark, bool published, int createdBy)
    : id_(id), courseId_(courseId), title_(trim(title)), description_(std::move(description)), deadline_(deadline),
      maxMark_(maxMark), published_(published), createdBy_(createdBy) {
    if (title_.empty()) throw ValidationError("Assignment title must not be blank");
    if (maxMark_ < kMinMaxMark || maxMark_ > kMaxMaxMark) {
        throw ValidationError("Maximum mark must be between 1 and 1000");
    }
}

DateTime Assignment::effectiveDeadline(const Extension* ext) const {
    return ext != nullptr ? ext->newDeadline : deadline_;
}
