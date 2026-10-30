#include "AssignmentService.hpp"
#include "../domain/Errors.hpp"
#include "../domain/Validation.hpp"

#include <algorithm>

Assignment AssignmentService::requireOwnAssignment(const User& lecturer, int assignmentId) {
    auto a = assignments_.find(assignmentId);
    if (!a) throw NotFound("Assignment not found");
    if (!courses_.isLecturerOf(a->courseId(), lecturer.id())) {
        throw AccessDenied("You do not teach the course of this assignment");
    }
    return *a;
}

std::vector<Course> AssignmentService::myCourses(const User& lecturer) {
    lecturer.require(Permission::CreateAssignment);
    return courses_.coursesForLecturer(lecturer.id());
}

std::vector<AssignmentSummary> AssignmentService::myAssignments(const User& lecturer) {
    lecturer.require(Permission::CreateAssignment);
    std::vector<AssignmentSummary> out;
    for (const auto& c : courses_.coursesForLecturer(lecturer.id())) {
        for (const auto& a : assignments_.findByCourse(c.id)) {
            const int count = static_cast<int>(submissions_.findActiveByAssignment(a.id()).size());
            out.push_back(AssignmentSummary{a, c, count});
        }
    }
    return out;
}

int AssignmentService::create(const User& lecturer, int courseId, const std::string& title,
                              const std::string& description, DateTime deadline, int maxMark, bool publish) {
    lecturer.require(Permission::CreateAssignment);
    if (!courses_.findCourse(courseId)) throw NotFound("Course not found");
    if (!courses_.isLecturerOf(courseId, lecturer.id())) throw AccessDenied("You do not teach this course");
    if (deadline <= clock_.now()) throw ValidationError("The deadline must be in the future");
    const Assignment a(0, courseId, title, trim(description), deadline, maxMark, publish, lecturer.id());
    return assignments_.add(a);
}

void AssignmentService::setPublished(const User& lecturer, int assignmentId, bool published) {
    lecturer.require(Permission::CreateAssignment);
    const auto a = requireOwnAssignment(lecturer, assignmentId);
    if (!published && !submissions_.findActiveByAssignment(a.id()).empty()) {
        throw ValidationError("Students have already submitted work, so the assignment cannot be hidden");
    }
    assignments_.setPublished(assignmentId, published);
}

std::vector<ExtensionRow> AssignmentService::extensionsFor(const User& lecturer, int assignmentId) {
    lecturer.require(Permission::GrantExtension);
    const auto a = requireOwnAssignment(lecturer, assignmentId);
    std::vector<ExtensionRow> rows;
    for (const auto& s : courses_.studentsIn(a.courseId())) {
        rows.push_back(ExtensionRow{s.id, s.fullName, assignments_.findExtension(assignmentId, s.id)});
    }
    return rows;
}

void AssignmentService::grantExtension(const User& lecturer, int assignmentId, int studentId, DateTime newDeadline,
                                       const std::string& reason) {
    lecturer.require(Permission::GrantExtension);
    const auto a = requireOwnAssignment(lecturer, assignmentId);
    if (!courses_.isEnrolled(a.courseId(), studentId)) throw ValidationError("The student is not enrolled in this course");
    requireNotBlank(reason, "The reason for the extension");
    if (newDeadline <= a.deadline()) throw ValidationError("The new deadline must be after the original deadline");

    auto active = submissions_.findActive(assignmentId, studentId);
    if (active && !active->awaitingFirstMark()) {
        throw ValidationError("The submission has already been marked, so its deadline can no longer change");
    }
    if (active && active->state() == SubmissionState::SUBMITTED && active->submittedAt() > newDeadline) {
        throw ValidationError("The new deadline is earlier than the student's on-time submission");
    }

    const DateTime now = clock_.now();
    auto tx = tx_.begin();
    assignments_.saveExtension(Extension{assignmentId, studentId, newDeadline, trim(reason), lecturer.id(), now});
    if (active && active->state() == SubmissionState::LATE && active->submittedAt() <= newDeadline) {
        active->transitionTo(SubmissionState::SUBMITTED);
        submissions_.updateState(active->id(), SubmissionState::SUBMITTED);
        notify(MarkChangeEvent{lecturer.id(), active->id(), "state", toString(SubmissionState::LATE),
                               toString(SubmissionState::SUBMITTED), "Extension granted: " + trim(reason), now});
    }
    tx->commit();
}
