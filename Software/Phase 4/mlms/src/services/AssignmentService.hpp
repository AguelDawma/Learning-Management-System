#pragma once

#include "../data/Database.hpp"
#include "../data/Repositories.hpp"
#include "../infrastructure/Infrastructure.hpp"
#include "MarkEvents.hpp"
#include "Views.hpp"

#include <string>
#include <vector>

// Lecturer use cases for assignments and individual extensions.
class AssignmentService : public MarkSubject {
    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
    IClock& clock_;
    ITransactionManager& tx_;

    Assignment requireOwnAssignment(const User& lecturer, int assignmentId);

public:
    AssignmentService(IAssignmentRepository& assignments, ICourseRepository& courses,
                      ISubmissionRepository& submissions, IClock& clock, ITransactionManager& tx)
        : assignments_(assignments), courses_(courses), submissions_(submissions), clock_(clock), tx_(tx) {}

    std::vector<Course> myCourses(const User& lecturer);
    std::vector<AssignmentSummary> myAssignments(const User& lecturer);

    // FR-09: deadline in the future, max mark 1..1000, lecturer teaches the course.
    int create(const User& lecturer, int courseId, const std::string& title, const std::string& description,
               DateTime deadline, int maxMark, bool publish);
    void setPublished(const User& lecturer, int assignmentId, bool published);

    std::vector<ExtensionRow> extensionsFor(const User& lecturer, int assignmentId);
    // FR-10 / BR-01: one extension per student per assignment, with a mandatory
    // reason. A LATE submission made before the new deadline returns to SUBMITTED.
    void grantExtension(const User& lecturer, int assignmentId, int studentId, DateTime newDeadline,
                        const std::string& reason);
};
