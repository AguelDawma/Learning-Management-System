#include "User.hpp"

bool Student::can(const std::string& permission) const {
    return permission == "view_courses" || permission == "submit_assignment" || permission == "view_marks";
}

bool Lecturer::can(const std::string& permission) const {
    return permission == "create_assignment" || permission == "grant_extension" || permission == "mark_submission";
}

bool HeadOfDepartment::can(const std::string& permission) const {
    return permission == "moderate_marks" || permission == "release_marks" || permission == "view_department_audit";
}

bool Administrator::can(const std::string& permission) const {
    return permission == "manage_users" || permission == "manage_courses" || permission == "manage_enrolments";
}