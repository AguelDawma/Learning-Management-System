#include "AdminService.hpp"
#include "../domain/Errors.hpp"
#include "../domain/Validation.hpp"

#include <algorithm>
#include <cctype>

UserRecord AdminService::requireUser(int id, std::optional<Role> role) {
    auto u = users_.findById(id);
    if (!u) throw NotFound("User not found");
    if (role && u->role != *role) {
        throw ValidationError(u->fullName + " is not a " + toString(*role));
    }
    return *u;
}

std::vector<UserRecord> AdminService::listUsers(const User& admin) {
    admin.require(Permission::ManageUsers);
    return users_.listAll();
}

int AdminService::createUser(const User& admin, const std::string& username, const std::string& fullName, Role role,
                             const std::string& temporaryPassword) {
    admin.require(Permission::ManageUsers);
    const std::string name = trim(username);
    validateUsername(name);
    requireNotBlank(fullName, "Full name");
    validatePasswordStrength(temporaryPassword);
    if (users_.findByUsername(name)) throw ValidationError("The username '" + name + "' is already taken");

    UserRecord r;
    r.username = name;
    r.fullName = trim(fullName);
    r.role = role;
    r.passwordHash = hasher_.hash(temporaryPassword);
    r.active = true;
    r.mustChangePassword = true;
    return users_.add(r);
}

void AdminService::resetPassword(const User& admin, int userId, const std::string& temporaryPassword) {
    admin.require(Permission::ManageUsers);
    requireUser(userId, std::nullopt);
    validatePasswordStrength(temporaryPassword);
    users_.updatePassword(userId, hasher_.hash(temporaryPassword), true);
}

void AdminService::setActive(const User& admin, int userId, bool active) {
    admin.require(Permission::ManageUsers);
    if (userId == admin.id() && !active) throw ValidationError("You cannot deactivate your own account");
    requireUser(userId, std::nullopt);
    users_.setActive(userId, active);
}

std::vector<Department> AdminService::listDepartments(const User& admin) {
    admin.require(Permission::ManageCourses);
    return courses_.listDepartments();
}

int AdminService::createDepartment(const User& admin, const std::string& name) {
    admin.require(Permission::ManageCourses);
    const std::string n = trim(name);
    requireNotBlank(n, "Department name");
    for (const auto& d : courses_.listDepartments()) {
        if (d.name == n) throw ValidationError("A department called '" + n + "' already exists");
    }
    return courses_.addDepartment(n);
}

void AdminService::setHeadOfDepartment(const User& admin, int departmentId, std::optional<int> hodUserId) {
    admin.require(Permission::ManageCourses);
    if (!courses_.findDepartment(departmentId)) throw NotFound("Department not found");
    if (hodUserId) {
        const auto hod = requireUser(*hodUserId, Role::HOD);
        if (!hod.active) throw ValidationError(hod.fullName + " is deactivated");
        if (hod.departmentId != 0 && hod.departmentId != departmentId) {
            throw ValidationError(hod.fullName + " already heads another department");
        }
    }
    courses_.setHod(departmentId, hodUserId);
}

std::vector<Course> AdminService::listCourses(const User& admin) {
    admin.require(Permission::ManageCourses);
    return courses_.listCourses();
}

int AdminService::createCourse(const User& admin, const std::string& code, const std::string& title,
                               int departmentId) {
    admin.require(Permission::ManageCourses);
    std::string c = trim(code);
    std::transform(c.begin(), c.end(), c.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    requireNotBlank(c, "Course code");
    requireNotBlank(title, "Course title");
    if (!courses_.findDepartment(departmentId)) throw NotFound("Department not found");
    for (const auto& existing : courses_.listCourses()) {
        if (existing.code == c) throw ValidationError("A course with code '" + c + "' already exists");
    }
    return courses_.addCourse(Course{0, c, trim(title), departmentId});
}

void AdminService::assignLecturer(const User& admin, int courseId, int lecturerId) {
    admin.require(Permission::ManageCourses);
    if (!courses_.findCourse(courseId)) throw NotFound("Course not found");
    requireUser(lecturerId, Role::LECTURER);
    courses_.assignLecturer(courseId, lecturerId);
}

void AdminService::removeLecturer(const User& admin, int courseId, int lecturerId) {
    admin.require(Permission::ManageCourses);
    courses_.removeLecturer(courseId, lecturerId);
}

std::vector<UserRecord> AdminService::lecturersOf(const User& admin, int courseId) {
    admin.require(Permission::ManageCourses);
    return courses_.lecturersOf(courseId);
}

void AdminService::enrolStudent(const User& admin, int courseId, int studentId) {
    admin.require(Permission::ManageEnrolments);
    if (!courses_.findCourse(courseId)) throw NotFound("Course not found");
    requireUser(studentId, Role::STUDENT);
    courses_.enrol(courseId, studentId, clock_.now());
}

void AdminService::unenrolStudent(const User& admin, int courseId, int studentId) {
    admin.require(Permission::ManageEnrolments);
    courses_.unenrol(courseId, studentId);
}

std::vector<UserRecord> AdminService::studentsIn(const User& admin, int courseId) {
    admin.require(Permission::ManageEnrolments);
    return courses_.studentsIn(courseId);
}
