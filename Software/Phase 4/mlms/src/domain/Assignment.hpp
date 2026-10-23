#pragma once

#include <string>
#include <optional>

class Extension {
    int assignmentId_;
    int studentId_;
    std::string newDeadline_;
    std::string reason_;
    int grantedBy_;
    std::string grantedAt_;

public:
    Extension(int assignmentId, int studentId, std::string newDeadline, 
              std::string reason, int grantedBy, std::string grantedAt)
        : assignmentId_(assignmentId), studentId_(studentId), 
          newDeadline_(std::move(newDeadline)), reason_(std::move(reason)), 
          grantedBy_(grantedBy), grantedAt_(std::move(grantedAt)) {}

    const std::string& newDeadline() const { return newDeadline_; }
    const std::string& reason() const { return reason_; }
};

class Assignment {
    int id_;
    int courseId_;
    std::string title_;
    std::string description_;
    std::string deadline_;
    int maxMark_;
    bool isPublished_;
    int createdBy_;

public:
    Assignment(int id, int courseId, std::string title, std::string description, 
               std::string deadline, int maxMark, bool isPublished, int createdBy)
        : id_(id), courseId_(courseId), title_(std::move(title)), 
          description_(std::move(description)), deadline_(std::move(deadline)), 
          maxMark_(maxMark), isPublished_(isPublished), createdBy_(createdBy) {}

    int id() const { return id_; }
    int courseId() const { return courseId_; }
    std::string title() const { return title_; }
    std::string description() const { return description_; }
    std::string deadline() const { return deadline_; }
    int maxMark() const { return maxMark_; }
    bool isPublished() const { return isPublished_; }
    int createdBy() const { return createdBy_; }
    
    // Implements BR-01: Effective deadline is the extension deadline if one exists, otherwise original.
    std::string effectiveDeadline(const Extension* ext) const {
        if (ext != nullptr) {
            return ext->newDeadline();
        }
        return deadline_;
    }
};