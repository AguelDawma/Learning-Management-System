#pragma once

#include "DateTime.hpp"

#include <string>

struct Extension {
    int assignmentId = 0;
    int studentId = 0;
    DateTime newDeadline;
    std::string reason;
    int grantedBy = 0;
    DateTime grantedAt;
};

class Assignment {
    int id_;
    int courseId_;
    std::string title_;
    std::string description_;
    DateTime deadline_;
    int maxMark_;
    bool published_;
    int createdBy_;

public:
    static constexpr int kMinMaxMark = 1;
    static constexpr int kMaxMaxMark = 1000;

    // Throws ValidationError for a blank title or a max mark outside 1..1000 (FR-09).
    Assignment(int id, int courseId, std::string title, std::string description, DateTime deadline, int maxMark,
               bool published, int createdBy);

    int id() const { return id_; }
    int courseId() const { return courseId_; }
    const std::string& title() const { return title_; }
    const std::string& description() const { return description_; }
    DateTime deadline() const { return deadline_; }
    int maxMark() const { return maxMark_; }
    bool isPublished() const { return published_; }
    int createdBy() const { return createdBy_; }

    void setId(int id) { id_ = id; }
    void setPublished(bool p) { published_ = p; }

    // BR-01: the student's extension deadline replaces the original one.
    DateTime effectiveDeadline(const Extension* ext) const;
};
