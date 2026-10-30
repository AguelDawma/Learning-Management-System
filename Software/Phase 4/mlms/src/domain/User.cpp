#include "User.hpp"
#include "Errors.hpp"

std::string toString(Role role) {
    switch (role) {
        case Role::STUDENT: return "STUDENT";
        case Role::LECTURER: return "LECTURER";
        case Role::HOD: return "HOD";
        case Role::ADMINISTRATOR: return "ADMINISTRATOR";
    }
    return "STUDENT";
}

Role roleFromString(const std::string& text) {
    if (text == "STUDENT") return Role::STUDENT;
    if (text == "LECTURER") return Role::LECTURER;
    if (text == "HOD") return Role::HOD;
    if (text == "ADMINISTRATOR") return Role::ADMINISTRATOR;
    throw ValidationError("Unknown role '" + text + "'");
}

void User::require(Permission p) const {
    if (!active_ || !can(p)) {
        throw AccessDenied("Your role (" + toString(role()) + ") is not allowed to perform this action");
    }
}

bool Student::can(Permission p) const {
    return p == Permission::ViewOwnCourses || p == Permission::SubmitAssignment ||
           p == Permission::ViewReleasedMarks;
}

bool Lecturer::can(Permission p) const {
    return p == Permission::ViewOwnCourses || p == Permission::CreateAssignment ||
           p == Permission::GrantExtension || p == Permission::MarkSubmission;
}

bool HeadOfDepartment::can(Permission p) const {
    return p == Permission::ModerateMarks || p == Permission::ReleaseMarks || p == Permission::ViewAuditLog;
}

bool Administrator::can(Permission p) const {
    return p == Permission::ManageUsers || p == Permission::ManageCourses || p == Permission::ManageEnrolments ||
           p == Permission::ViewAuditLog;
}
