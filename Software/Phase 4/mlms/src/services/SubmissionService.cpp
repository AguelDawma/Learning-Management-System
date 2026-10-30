#include "SubmissionService.hpp"
#include "../domain/Errors.hpp"

Assignment SubmissionService::requireOpenAssignment(const User& student, int assignmentId) {
    student.require(Permission::SubmitAssignment);
    auto a = assignments_.find(assignmentId);
    if (!a || !a->isPublished()) throw NotFound("Assignment not found");
    if (!courses_.isEnrolled(a->courseId(), student.id())) {
        throw AccessDenied("You are not enrolled in the course of this assignment");
    }
    return *a;
}

SubmissionPlan SubmissionService::plan(const Assignment& a, int studentId, DateTime now) {
    const auto ext = assignments_.findExtension(a.id(), studentId);
    const auto existing = submissions_.findActive(a.id(), studentId);

    SubmissionPlan p;
    p.now = now;
    p.effectiveDeadline = a.effectiveDeadline(ext ? &*ext : nullptr);
    p.nextVersion = existing ? existing->version() + 1 : 1;
    const bool onTime = now <= p.effectiveDeadline;

    if (existing && !existing->awaitingFirstMark()) {
        p.kind = SubmissionPlan::Kind::Refused;
        p.refusal = "This work has already been marked, so it can no longer be replaced.";
    } else if (onTime) {
        p.kind = existing ? SubmissionPlan::Kind::Resubmission : SubmissionPlan::Kind::FirstOnTime;
    } else if (existing) {
        p.kind = SubmissionPlan::Kind::Refused;
        p.refusal = "The deadline has passed. Your earlier version will be marked (BR-03).";
    } else {
        p.kind = SubmissionPlan::Kind::Late;
        p.daysLate = Submission(0, a.id(), studentId, 1, "", "", now, SubmissionState::LATE).daysLate(p.effectiveDeadline);
    }
    return p;
}

std::vector<StudentAssignmentView> SubmissionService::myAssignments(const User& student) {
    student.require(Permission::ViewOwnCourses);
    std::vector<StudentAssignmentView> out;
    for (const auto& course : courses_.coursesForStudent(student.id())) {
        for (const auto& a : assignments_.findByCourse(course.id)) {
            if (!a.isPublished()) continue;
            StudentAssignmentView v{a, course, assignments_.findExtension(a.id(), student.id()), a.deadline(),
                                    submissions_.findActive(a.id(), student.id()), std::nullopt};
            v.effectiveDeadline = a.effectiveDeadline(v.extension ? &*v.extension : nullptr);
            if (v.active && v.active->state() == SubmissionState::RELEASED) {
                if (auto mark = marks_.find(v.active->id())) {
                    const int days = v.active->daysLate(v.effectiveDeadline);
                    const auto r = mark->finalise(policy_, a.maxMark(), days);
                    v.result = ReleasedResult{mark->rawMark(), r.penalty, r.finalMark, days, mark->feedback(),
                                              mark->releasedAt().value_or(DateTime())};
                }
            }
            out.push_back(std::move(v));
        }
    }
    return out;
}

std::vector<Course> SubmissionService::myCourses(const User& student) {
    student.require(Permission::ViewOwnCourses);
    return courses_.coursesForStudent(student.id());
}

SubmissionPlan SubmissionService::preview(const User& student, int assignmentId) {
    const auto a = requireOpenAssignment(student, assignmentId);
    return plan(a, student.id(), clock_.now());
}

Receipt SubmissionService::submit(const User& student, int assignmentId, const std::filesystem::path& file) {
    const auto a = requireOpenAssignment(student, assignmentId);
    files_.validate(file);

    // The time always comes from the host clock, never from the student (FR-12).
    const auto p = plan(a, student.id(), clock_.now());
    if (p.kind == SubmissionPlan::Kind::Refused) throw SubmissionRefused(p.refusal);

    const auto state = p.kind == SubmissionPlan::Kind::Late ? SubmissionState::LATE : SubmissionState::SUBMITTED;
    const StoredFile stored = files_.store(file, assignmentId, student.id(), p.nextVersion);
    try {
        auto tx = tx_.begin();
        Submission s(0, assignmentId, student.id(), p.nextVersion, stored.relativePath, stored.checksum, p.now, state);
        const int id = submissions_.add(s);
        tx->commit();
        return Receipt{id, p.nextVersion, p.now, state, p.daysLate, stored.checksum};
    } catch (...) {
        files_.remove(stored.relativePath); // keep disk and database consistent
        throw;
    }
}

std::vector<Submission> SubmissionService::myVersions(const User& student, int assignmentId) {
    student.require(Permission::ViewOwnCourses);
    return submissions_.findVersions(assignmentId, student.id());
}
