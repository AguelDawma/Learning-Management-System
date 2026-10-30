#pragma once

#include "DateTime.hpp"
#include "LatePenaltyPolicy.hpp"

#include <cstddef>
#include <optional>
#include <string>

class Mark {
    int submissionId_;
    double rawMark_;
    std::string feedback_;
    int markedBy_;
    DateTime markedAt_;
    std::optional<int> moderatedBy_;
    std::optional<DateTime> releasedAt_;
    std::optional<std::string> returnComment_;
    int rowVersion_;

public:
    static constexpr std::size_t kMinFeedbackLength = 10;

    Mark(int submissionId, double rawMark, std::string feedback, int markedBy, DateTime markedAt,
         std::optional<int> moderatedBy = std::nullopt, std::optional<DateTime> releasedAt = std::nullopt,
         std::optional<std::string> returnComment = std::nullopt, int rowVersion = 1);

    // Throws ValidationError unless 0 <= raw <= maxMark.
    static void validateRaw(double raw, int maxMark);
    // Throws ValidationError unless the trimmed feedback has at least 10 characters (FR-16).
    static void validateFeedback(const std::string& feedback);

    int submissionId() const { return submissionId_; }
    double rawMark() const { return rawMark_; }
    const std::string& feedback() const { return feedback_; }
    int markedBy() const { return markedBy_; }
    DateTime markedAt() const { return markedAt_; }
    const std::optional<int>& moderatedBy() const { return moderatedBy_; }
    const std::optional<DateTime>& releasedAt() const { return releasedAt_; }
    const std::optional<std::string>& returnComment() const { return returnComment_; }
    int rowVersion() const { return rowVersion_; }

    void setRawMark(double raw) { rawMark_ = raw; }
    void setFeedback(std::string f) { feedback_ = std::move(f); }
    void setMarkedBy(int id, DateTime at) {
        markedBy_ = id;
        markedAt_ = at;
    }
    void setModeratedBy(int id) { moderatedBy_ = id; }
    void setReleasedAt(DateTime t) { releasedAt_ = t; }
    void setReturnComment(std::optional<std::string> c) { returnComment_ = std::move(c); }

    PenaltyResult finalise(const ILatePenaltyPolicy& policy, int maxMark, int daysLate) const {
        return policy.apply(rawMark_, maxMark, daysLate);
    }
};
